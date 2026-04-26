package com.videoeditor.app.media

import android.content.Context
import android.graphics.SurfaceTexture
import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.net.Uri
import android.opengl.EGL14
import android.opengl.EGLConfig
import android.opengl.EGLContext
import android.opengl.EGLDisplay
import android.opengl.EGLExt
import android.opengl.EGLSurface
import android.opengl.GLES11Ext
import android.opengl.GLES30
import android.os.SystemClock
import android.util.Log
import android.view.Surface
import com.videoeditor.app.engine.TimelineClip
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.math.max
import java.util.concurrent.atomic.AtomicBoolean

private const val TAG = "VideoPreviewEngine"

private const val VERTEX_SHADER = """#version 300 es
in vec4 aPosition;
in vec2 aTexCoord;
out vec2 vTexCoord;
uniform mat4 uTexMatrix;
void main() {
    gl_Position = aPosition;
    vTexCoord = (uTexMatrix * vec4(aTexCoord, 0.0, 1.0)).xy;
}
"""

// uLutPreset: 0=none 1=warm 2=cool 3=cinematic 4=orangeteal 5=faded
private const val FRAGMENT_SHADER = """#version 300 es
#extension GL_OES_EGL_image_external_essl3 : require
precision mediump float;
uniform samplerExternalOES sVideo;
uniform sampler3D sLut;
uniform int uUseLut;
in vec2 vTexCoord;
out vec4 outColor;
void main(){
    vec4 c=texture(sVideo,vTexCoord);
    vec3 col=clamp(c.rgb,0.0,1.0);
    if(uUseLut==1) {
        col=texture(sLut,col).rgb;
    }
    outColor=vec4(col,c.a);
}
"""

private val QUAD_VERTS = floatArrayOf(
    -1f, -1f, 0f, 0f,
     1f, -1f, 1f, 0f,
    -1f,  1f, 0f, 1f,
     1f,  1f, 1f, 1f,
)

/**
 * Drives multi-clip video playback with per-project GLES LUT.
 *
 * Decoder → SurfaceTexture (OES) → GLES LUT shader → display/encoder Surface.
 * All GL work (including EGL context) lives on a single dedicated background thread.
 */
class VideoPreviewEngine(
    private val context: Context,
    private val outputSurface: Surface,
) {
    private val isPlaying = AtomicBoolean(false)

    @Volatile private var renderThread: Thread? = null
    @Volatile private var clips: List<TimelineClip> = emptyList()
    @Volatile private var lutPreset = LutPreset.NONE
    @Volatile private var activeLutPreset = LutPreset.NONE

    // GL resources — only touched on the render thread
    private var eglDisplay: EGLDisplay = EGL14.EGL_NO_DISPLAY
    private var eglContext: EGLContext = EGL14.EGL_NO_CONTEXT
    private var eglSurface: EGLSurface = EGL14.EGL_NO_SURFACE
    private var program = 0
    private var oesTexId = 0
    private var lutTexId = 0
    private var loadedLutPreset: LutPreset? = null
    private var quadVbo = 0
    private var surfaceTex: SurfaceTexture? = null
    private var decoderSurface: Surface? = null
    private val texMatrix = FloatArray(16)

    fun setClips(clips: List<TimelineClip>) {
        this.clips = clips
    }

    fun setLut(preset: LutPreset) {
        lutPreset = preset
    }

    fun play(startTimeMs: Long = 0L, onTimeUpdate: (Long) -> Unit) {
        if (isPlaying.getAndSet(true)) return
        val t = Thread({ runPlayback(startTimeMs, onTimeUpdate) }, "VideoRender")
        t.isDaemon = true
        renderThread = t
        t.start()
    }

    fun stop() {
        isPlaying.set(false)
    }

    fun release() {
        stop()
        renderThread?.join(2000)
        renderThread = null
    }

    fun renderAt(timeMs: Long, onTimeUpdate: (Long) -> Unit = {}) {
        stop()
        renderThread?.join(500)
        val t = Thread({ runSingleFrame(timeMs, onTimeUpdate) }, "VideoSeekRender")
        t.isDaemon = true
        renderThread = t
        t.start()
    }

    // ── Playback loop ─────────────────────────────────────────────────────────

    private fun runPlayback(startTimeMs: Long, onTimeUpdate: (Long) -> Unit) {
        try {
            initEgl()
            initGl()

            val localClips = clips
            if (localClips.isEmpty()) {
                Log.d(TAG, "No clips to play")
                return
            }

            var timeOffset = 0L
            for (clip in localClips) {
                if (!isPlaying.get()) break
                val clipEnd = timeOffset + clip.durationMs
                if (clipEnd <= startTimeMs) {
                    timeOffset = clipEnd
                    continue
                }
                activeLutPreset = clip.lutPresetOrdinal
                    .takeIf { it != 0 }
                    ?.let { LutPreset.entries.getOrNull(it) }
                    ?: lutPreset
                playClip(
                    uri = Uri.parse(clip.uri),
                    timeOffsetMs = timeOffset,
                    startAtMs = max(0L, startTimeMs - timeOffset),
                    onTimeUpdate = onTimeUpdate,
                )
                timeOffset += clip.durationMs
            }
        } catch (e: Exception) {
            Log.e(TAG, "Playback error", e)
        } finally {
            isPlaying.set(false)
            releaseGl()
            releaseEgl()
        }
    }

    private fun runSingleFrame(timeMs: Long, onTimeUpdate: (Long) -> Unit) {
        try {
            initEgl()
            initGl()
            val target = resolveTimelineTime(timeMs) ?: return
            activeLutPreset = target.clip.lutPresetOrdinal
                .takeIf { it != 0 }
                ?.let { LutPreset.entries.getOrNull(it) }
                ?: lutPreset
            renderFrameAt(Uri.parse(target.clip.uri), target.localTimeMs)
            onTimeUpdate(target.timelineTimeMs)
        } catch (e: Exception) {
            Log.e(TAG, "Seek render error", e)
        } finally {
            releaseGl()
            releaseEgl()
        }
    }

    private data class TimelineTarget(
        val clip: TimelineClip,
        val timelineTimeMs: Long,
        val localTimeMs: Long,
    )

    private fun resolveTimelineTime(timeMs: Long): TimelineTarget? {
        val localClips = clips
        if (localClips.isEmpty()) return null
        val duration = localClips.sumOf { it.durationMs }.coerceAtLeast(1L)
        val clampedTime = timeMs.coerceIn(0L, duration - 1)
        var offset = 0L
        for (clip in localClips) {
            val end = offset + clip.durationMs
            if (clampedTime < end) {
                return TimelineTarget(clip, clampedTime, clampedTime - offset)
            }
            offset = end
        }
        val last = localClips.last()
        return TimelineTarget(last, duration - 1, (last.durationMs - 1).coerceAtLeast(0L))
    }

    private fun playClip(uri: Uri, timeOffsetMs: Long, startAtMs: Long, onTimeUpdate: (Long) -> Unit) {
        val extractor = MediaExtractor()
        extractor.setDataSource(context, uri, null)

        val trackIdx = (0 until extractor.trackCount).firstOrNull { i ->
            extractor.getTrackFormat(i).getString(MediaFormat.KEY_MIME)?.startsWith("video/") == true
        } ?: run { extractor.release(); return }

        extractor.selectTrack(trackIdx)
        if (startAtMs > 0L) {
            extractor.seekTo(startAtMs * 1000L, MediaExtractor.SEEK_TO_PREVIOUS_SYNC)
        }
        val format = extractor.getTrackFormat(trackIdx)
        val mime = format.getString(MediaFormat.KEY_MIME) ?: run { extractor.release(); return }

        val decoder = MediaCodec.createDecoderByType(mime)
        decoder.configure(format, decoderSurface, null, 0)
        decoder.start()

        val bufInfo = MediaCodec.BufferInfo()
        var inputEos = false
        var outputEos = false
        var startRealtime = -1L
        var startPtsMs = -1L
        val frameLock = Object()
        var frameReady = false

        surfaceTex?.setOnFrameAvailableListener {
            synchronized(frameLock) { frameReady = true; frameLock.notifyAll() }
        }

        try {
            while (!outputEos && isPlaying.get()) {
                if (!inputEos) {
                    val inIdx = decoder.dequeueInputBuffer(10_000L)
                    if (inIdx >= 0) {
                        val buf = decoder.getInputBuffer(inIdx)!!
                        val sz = extractor.readSampleData(buf, 0)
                        if (sz < 0) {
                            decoder.queueInputBuffer(inIdx, 0, 0, 0L, MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                            inputEos = true
                        } else {
                            decoder.queueInputBuffer(inIdx, 0, sz, extractor.sampleTime, extractor.sampleFlags)
                            extractor.advance()
                        }
                    }
                }

                val outIdx = decoder.dequeueOutputBuffer(bufInfo, 10_000L)
                if (outIdx >= 0) {
                    if (bufInfo.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM != 0) outputEos = true
                    val shouldRender = bufInfo.size > 0 &&
                            (bufInfo.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == 0

                    if (shouldRender) {
                        if (startRealtime < 0L) {
                            startRealtime = SystemClock.elapsedRealtime()
                            startPtsMs = bufInfo.presentationTimeUs / 1000L
                        }
                        val ptsMs = bufInfo.presentationTimeUs / 1000L
                        val elapsed = SystemClock.elapsedRealtime() - startRealtime
                        val delay = (ptsMs - startPtsMs) - elapsed
                        if (delay > 0L) SystemClock.sleep(delay)
                    }

                    decoder.releaseOutputBuffer(outIdx, shouldRender)

                    if (shouldRender) {
                        synchronized(frameLock) {
                            if (!frameReady) frameLock.wait(200)
                            frameReady = false
                        }
                        renderFrame()
                        onTimeUpdate(timeOffsetMs + bufInfo.presentationTimeUs / 1000L)
                    }
                }
            }
        } finally {
            surfaceTex?.setOnFrameAvailableListener(null)
            decoder.stop()
            decoder.release()
            extractor.release()
        }
    }

    private fun renderFrameAt(uri: Uri, targetMs: Long) {
        val extractor = MediaExtractor()
        extractor.setDataSource(context, uri, null)
        val trackIdx = (0 until extractor.trackCount).firstOrNull { i ->
            extractor.getTrackFormat(i).getString(MediaFormat.KEY_MIME)?.startsWith("video/") == true
        } ?: run { extractor.release(); return }

        extractor.selectTrack(trackIdx)
        extractor.seekTo(targetMs * 1000L, MediaExtractor.SEEK_TO_PREVIOUS_SYNC)
        val format = extractor.getTrackFormat(trackIdx)
        val mime = format.getString(MediaFormat.KEY_MIME) ?: run { extractor.release(); return }
        val decoder = MediaCodec.createDecoderByType(mime)
        decoder.configure(format, decoderSurface, null, 0)
        decoder.start()

        val bufInfo = MediaCodec.BufferInfo()
        val frameLock = Object()
        var frameReady = false
        var inputEos = false
        var rendered = false

        surfaceTex?.setOnFrameAvailableListener {
            synchronized(frameLock) { frameReady = true; frameLock.notifyAll() }
        }

        try {
            while (!rendered) {
                if (!inputEos) {
                    val inIdx = decoder.dequeueInputBuffer(10_000L)
                    if (inIdx >= 0) {
                        val buf = decoder.getInputBuffer(inIdx)!!
                        val sz = extractor.readSampleData(buf, 0)
                        if (sz < 0) {
                            decoder.queueInputBuffer(inIdx, 0, 0, 0L, MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                            inputEos = true
                        } else {
                            decoder.queueInputBuffer(inIdx, 0, sz, extractor.sampleTime, extractor.sampleFlags)
                            extractor.advance()
                        }
                    }
                }

                val outIdx = decoder.dequeueOutputBuffer(bufInfo, 10_000L)
                if (outIdx >= 0) {
                    val shouldRender = bufInfo.size > 0 &&
                            (bufInfo.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == 0
                    decoder.releaseOutputBuffer(outIdx, shouldRender)
                    if (shouldRender) {
                        synchronized(frameLock) {
                            if (!frameReady) frameLock.wait(200)
                            frameReady = false
                        }
                        renderFrame()
                        rendered = true
                    }
                    if (bufInfo.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM != 0) break
                }
            }
        } finally {
            surfaceTex?.setOnFrameAvailableListener(null)
            decoder.stop()
            decoder.release()
            extractor.release()
        }
    }

    // ── EGL ───────────────────────────────────────────────────────────────────

    private fun initEgl() {
        eglDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY)
        check(eglDisplay != EGL14.EGL_NO_DISPLAY) { "No EGL display" }
        val v = IntArray(2)
        check(EGL14.eglInitialize(eglDisplay, v, 0, v, 1))

        val attribs = intArrayOf(
            EGL14.EGL_RENDERABLE_TYPE, EGLExt.EGL_OPENGL_ES3_BIT_KHR,
            EGL14.EGL_SURFACE_TYPE, EGL14.EGL_WINDOW_BIT,
            EGL14.EGL_RED_SIZE, 8, EGL14.EGL_GREEN_SIZE, 8, EGL14.EGL_BLUE_SIZE, 8, EGL14.EGL_ALPHA_SIZE, 8,
            EGL14.EGL_NONE,
        )
        val configs = arrayOfNulls<EGLConfig>(1)
        val numCfg = IntArray(1)
        check(EGL14.eglChooseConfig(eglDisplay, attribs, 0, configs, 0, 1, numCfg, 0) && numCfg[0] > 0)
        val cfg = configs[0]!!

        val ctxAttribs = intArrayOf(EGL14.EGL_CONTEXT_CLIENT_VERSION, 3, EGL14.EGL_NONE)
        eglContext = EGL14.eglCreateContext(eglDisplay, cfg, EGL14.EGL_NO_CONTEXT, ctxAttribs, 0)
        check(eglContext != EGL14.EGL_NO_CONTEXT) { "No EGL context" }

        eglSurface = EGL14.eglCreateWindowSurface(eglDisplay, cfg, outputSurface, intArrayOf(EGL14.EGL_NONE), 0)
        check(eglSurface != EGL14.EGL_NO_SURFACE) { "No EGL surface" }
        check(EGL14.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
    }

    private fun releaseEgl() {
        if (eglDisplay != EGL14.EGL_NO_DISPLAY) {
            EGL14.eglMakeCurrent(eglDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT)
            if (eglSurface != EGL14.EGL_NO_SURFACE) EGL14.eglDestroySurface(eglDisplay, eglSurface)
            if (eglContext != EGL14.EGL_NO_CONTEXT) EGL14.eglDestroyContext(eglDisplay, eglContext)
            EGL14.eglTerminate(eglDisplay)
        }
        eglDisplay = EGL14.EGL_NO_DISPLAY
        eglContext = EGL14.EGL_NO_CONTEXT
        eglSurface = EGL14.EGL_NO_SURFACE
    }

    // ── GL ────────────────────────────────────────────────────────────────────

    private fun initGl() {
        program = buildProgram(VERTEX_SHADER, FRAGMENT_SHADER)

        val texIds = IntArray(1)
        GLES30.glGenTextures(1, texIds, 0)
        oesTexId = texIds[0]

        GLES30.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, oesTexId)
        GLES30.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_LINEAR)
        GLES30.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR)
        GLES30.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0)

        GLES30.glGenTextures(1, texIds, 0)
        lutTexId = texIds[0]

        surfaceTex = SurfaceTexture(oesTexId)
        decoderSurface = Surface(surfaceTex)

        val vboIds = IntArray(1)
        GLES30.glGenBuffers(1, vboIds, 0)
        quadVbo = vboIds[0]
        val buf = ByteBuffer.allocateDirect(QUAD_VERTS.size * 4)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer()
            .apply { put(QUAD_VERTS); position(0) }
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, quadVbo)
        GLES30.glBufferData(GLES30.GL_ARRAY_BUFFER, QUAD_VERTS.size * 4, buf, GLES30.GL_STATIC_DRAW)
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, 0)
    }

    private fun releaseGl() {
        decoderSurface?.release()
        surfaceTex?.release()
        decoderSurface = null
        surfaceTex = null
        if (program != 0) GLES30.glDeleteProgram(program)
        GLES30.glDeleteTextures(1, intArrayOf(oesTexId), 0)
        GLES30.glDeleteTextures(1, intArrayOf(lutTexId), 0)
        GLES30.glDeleteBuffers(1, intArrayOf(quadVbo), 0)
        loadedLutPreset = null
    }

    private fun renderFrame() {
        surfaceTex?.updateTexImage()
        surfaceTex?.getTransformMatrix(texMatrix)

        val w = IntArray(1); val h = IntArray(1)
        EGL14.eglQuerySurface(eglDisplay, eglSurface, EGL14.EGL_WIDTH, w, 0)
        EGL14.eglQuerySurface(eglDisplay, eglSurface, EGL14.EGL_HEIGHT, h, 0)
        GLES30.glViewport(0, 0, w[0], h[0])
        GLES30.glClearColor(0f, 0f, 0f, 1f)
        GLES30.glClear(GLES30.GL_COLOR_BUFFER_BIT)

        GLES30.glUseProgram(program)

        GLES30.glActiveTexture(GLES30.GL_TEXTURE0)
        GLES30.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, oesTexId)
        GLES30.glUniform1i(GLES30.glGetUniformLocation(program, "sVideo"), 0)
        val useLut = bindActiveLutTexture()
        GLES30.glUniform1i(GLES30.glGetUniformLocation(program, "uUseLut"), if (useLut) 1 else 0)
        GLES30.glUniformMatrix4fv(GLES30.glGetUniformLocation(program, "uTexMatrix"), 1, false, texMatrix, 0)

        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, quadVbo)
        val stride = 4 * 4
        val posLoc = GLES30.glGetAttribLocation(program, "aPosition")
        GLES30.glEnableVertexAttribArray(posLoc)
        GLES30.glVertexAttribPointer(posLoc, 2, GLES30.GL_FLOAT, false, stride, 0)
        val uvLoc = GLES30.glGetAttribLocation(program, "aTexCoord")
        GLES30.glEnableVertexAttribArray(uvLoc)
        GLES30.glVertexAttribPointer(uvLoc, 2, GLES30.GL_FLOAT, false, stride, 2 * 4)
        GLES30.glDrawArrays(GLES30.GL_TRIANGLE_STRIP, 0, 4)
        GLES30.glDisableVertexAttribArray(posLoc)
        GLES30.glDisableVertexAttribArray(uvLoc)
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, 0)

        EGL14.eglSwapBuffers(eglDisplay, eglSurface)
    }

    private fun bindActiveLutTexture(): Boolean {
        if (activeLutPreset == LutPreset.NONE) return false
        if (loadedLutPreset != activeLutPreset) {
            val lut = activeLutPreset.generateLutData(context) ?: return false
            GLES30.glBindTexture(GLES30.GL_TEXTURE_3D, lutTexId)
            GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_LINEAR)
            GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR)
            GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_WRAP_S, GLES30.GL_CLAMP_TO_EDGE)
            GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_WRAP_T, GLES30.GL_CLAMP_TO_EDGE)
            GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_WRAP_R, GLES30.GL_CLAMP_TO_EDGE)
            val data = ByteBuffer.allocateDirect(lut.rgb.size)
                .order(ByteOrder.nativeOrder())
                .put(lut.rgb)
                .apply { position(0) }
            GLES30.glTexImage3D(
                GLES30.GL_TEXTURE_3D,
                0,
                GLES30.GL_RGB,
                lut.size,
                lut.size,
                lut.size,
                0,
                GLES30.GL_RGB,
                GLES30.GL_UNSIGNED_BYTE,
                data,
            )
            loadedLutPreset = activeLutPreset
        }
        GLES30.glActiveTexture(GLES30.GL_TEXTURE1)
        GLES30.glBindTexture(GLES30.GL_TEXTURE_3D, lutTexId)
        GLES30.glUniform1i(GLES30.glGetUniformLocation(program, "sLut"), 1)
        return true
    }

    // ── Shader helpers ────────────────────────────────────────────────────────

    private fun compileShader(type: Int, src: String): Int {
        val shader = GLES30.glCreateShader(type)
        GLES30.glShaderSource(shader, src)
        GLES30.glCompileShader(shader)
        val status = IntArray(1)
        GLES30.glGetShaderiv(shader, GLES30.GL_COMPILE_STATUS, status, 0)
        if (status[0] == 0) {
            val log = GLES30.glGetShaderInfoLog(shader)
            GLES30.glDeleteShader(shader)
            error("Shader compile failed: $log")
        }
        return shader
    }

    private fun buildProgram(vtx: String, frag: String): Int {
        val vs = compileShader(GLES30.GL_VERTEX_SHADER, vtx)
        val fs = compileShader(GLES30.GL_FRAGMENT_SHADER, frag)
        val prog = GLES30.glCreateProgram()
        GLES30.glAttachShader(prog, vs)
        GLES30.glAttachShader(prog, fs)
        GLES30.glLinkProgram(prog)
        GLES30.glDeleteShader(vs)
        GLES30.glDeleteShader(fs)
        val status = IntArray(1)
        GLES30.glGetProgramiv(prog, GLES30.GL_LINK_STATUS, status, 0)
        if (status[0] == 0) error("Program link failed: ${GLES30.glGetProgramInfoLog(prog)}")
        return prog
    }
}
