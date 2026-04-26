package com.videoeditor.app.media

import android.content.ContentValues
import android.content.Context
import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.media.MediaMuxer
import android.net.Uri
import android.os.Build
import android.provider.MediaStore
import android.util.Log
import com.videoeditor.app.engine.NativeBridge
import com.videoeditor.app.engine.TimelineClip
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File

private const val TAG = "ExportService"
private const val BUFFER_TIMEOUT_US = 10_000L

data class ExportResult(val success: Boolean, val uri: Uri? = null, val error: String? = null)

enum class ExportPreset { BEST, INSTAGRAM, CUSTOM }
enum class ExportCodec(val mimeType: String, val label: String) {
    H264(MediaFormat.MIMETYPE_VIDEO_AVC, "H.264"),
    H265(MediaFormat.MIMETYPE_VIDEO_HEVC, "H.265"),
}

data class ExportResolution(val width: Int, val height: Int, val label: String) {
    companion object {
        val Original = ExportResolution(0, 0, "Original")
        val Reel1080p = ExportResolution(1080, 1920, "1080 x 1920")
        val Landscape1080p = ExportResolution(1920, 1080, "1920 x 1080")
        val Square1080p = ExportResolution(1080, 1080, "1080 x 1080")
    }
}

data class ExportSettings(
    val preset: ExportPreset = ExportPreset.BEST,
    val resolution: ExportResolution = ExportResolution.Original,
    val fps: Int = 30,
    val codec: ExportCodec = ExportCodec.H264,
    val bitrateMbps: Int = 12,
) {
    companion object {
        fun best() = ExportSettings()
        fun instagram() = ExportSettings(
            preset = ExportPreset.INSTAGRAM,
            resolution = ExportResolution.Reel1080p,
            fps = 30,
            codec = ExportCodec.H264,
            bitrateMbps = 8,
        )
    }
}

/**
 * Exports the current project timeline by transcoding each clip in sequence.
 *
 * This is the Phase 1 export path: pure MediaExtractor→MediaCodec transcode.
 * It concatenates clips in timeline order without LUT processing (Phase 2).
 * When GlesBackend::present() is implemented, the render-graph export path
 * will replace or augment this approach.
 *
 * @param onProgress called with [0.0, 1.0] progress fraction
 */
suspend fun exportProject(
    context: Context,
    settings: ExportSettings = ExportSettings.best(),
    onProgress: (Float) -> Unit,
): ExportResult = withContext(Dispatchers.IO) {
    val clips = NativeBridge.getTimelineClips()
    if (clips.isEmpty()) {
        return@withContext ExportResult(success = false, error = "No clips in timeline")
    }

    val tempFile = File(context.cacheDir, "export_${System.currentTimeMillis()}.mp4")

    try {
        transcodeClips(context, clips, tempFile, settings, onProgress)
        val uri = saveToMediaStore(context, tempFile)
        ExportResult(success = true, uri = uri)
    } catch (e: Exception) {
        Log.e(TAG, "Export failed", e)
        ExportResult(success = false, error = e.message)
    } finally {
        tempFile.delete()
    }
}

private fun transcodeClips(
    context: Context,
    clips: List<TimelineClip>,
    outputFile: File,
    settings: ExportSettings,
    onProgress: (Float) -> Unit,
) {
    // Probe the first clip to determine output resolution/format.
    val (sourceWidth, sourceHeight, sourceMimeType) = probeFirstVideoTrack(context, clips[0].uri)
        ?: throw IllegalStateException("Unable to read first clip format")
    val outWidth = settings.resolution.width.takeIf { it > 0 } ?: sourceWidth
    val outHeight = settings.resolution.height.takeIf { it > 0 } ?: sourceHeight
    val mimeType = when (settings.preset) {
        ExportPreset.BEST -> sourceMimeType.takeIf { it == MediaFormat.MIMETYPE_VIDEO_HEVC } ?: settings.codec.mimeType
        ExportPreset.INSTAGRAM, ExportPreset.CUSTOM -> settings.codec.mimeType
    }

    val muxer = MediaMuxer(outputFile.absolutePath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4)
    var muxerStarted = false
    var videoTrackIndex = -1
    var muxerPtsOffsetUs = 0L

    try {
        val totalDurationMs = clips.sumOf { it.durationMs }.coerceAtLeast(1L)
        val encoderFormat = MediaFormat.createVideoFormat(mimeType, outWidth, outHeight).apply {
            setInteger(MediaFormat.KEY_COLOR_FORMAT, android.media.MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface)
            setInteger(MediaFormat.KEY_BIT_RATE, settings.bitrateMbps * 1_000_000)
            setInteger(MediaFormat.KEY_FRAME_RATE, settings.fps)
            setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1)
        }
        val encoder = MediaCodec.createEncoderByType(mimeType)
        encoder.configure(encoderFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
        val encoderSurface = encoder.createInputSurface()
        encoder.start()
        val encoderInfo = MediaCodec.BufferInfo()

        fun drainEncoder(endOfStream: Boolean) {
            if (endOfStream) {
                encoder.signalEndOfInputStream()
            }
            while (true) {
                val outIdx = encoder.dequeueOutputBuffer(encoderInfo, if (endOfStream) BUFFER_TIMEOUT_US else 0)
                when {
                    outIdx == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED -> {
                        check(!muxerStarted) { "Encoder output format changed after muxer start" }
                        videoTrackIndex = muxer.addTrack(encoder.outputFormat)
                        muxer.start()
                        muxerStarted = true
                    }
                    outIdx >= 0 -> {
                        if ((encoderInfo.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == 0 &&
                            encoderInfo.size > 0 &&
                            muxerStarted &&
                            videoTrackIndex >= 0
                        ) {
                            val buf = encoder.getOutputBuffer(outIdx)!!
                            encoderInfo.presentationTimeUs += muxerPtsOffsetUs
                            muxer.writeSampleData(videoTrackIndex, buf, encoderInfo)
                        }
                        val sawEos = encoderInfo.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM != 0
                        encoder.releaseOutputBuffer(outIdx, false)
                        if (sawEos) return
                    }
                    else -> return
                }
            }
        }

        try {
            clips.forEachIndexed { clipIdx, clip ->
                val clipProgressBase = clips.take(clipIdx).sumOf { it.durationMs }.toFloat() / totalDurationMs
                val clipWeight = clip.durationMs.toFloat() / totalDurationMs

                val extractor = MediaExtractor()
                var decoder: MediaCodec? = null
                var lastPtsUs = 0L
                var decodedAnyFrame = false
            try {
                extractor.setDataSource(context, Uri.parse(clip.uri), null)
                val trackIndex = findVideoTrack(extractor)
                    ?: run {
                        Log.w(TAG, "No video track in ${clip.uri}, skipping")
                        return@forEachIndexed
                    }
                extractor.selectTrack(trackIndex)
                val inputFormat = extractor.getTrackFormat(trackIndex)

                val decoderMime = inputFormat.getString(MediaFormat.KEY_MIME)
                    ?: run {
                        Log.w(TAG, "No MIME type for track in ${clip.uri}, skipping")
                        return@forEachIndexed
                    }

                // Set up decoder → encoder surface
                decoder = MediaCodec.createDecoderByType(decoderMime)
                decoder.configure(inputFormat, encoderSurface, null, 0)
                decoder.start()

                val decoderInfo = MediaCodec.BufferInfo()
                var sawInputEos = false
                var sawOutputEos = false

                while (!sawOutputEos) {
                    // Feed decoder
                    if (!sawInputEos) {
                        val inputIdx = decoder.dequeueInputBuffer(BUFFER_TIMEOUT_US)
                        if (inputIdx >= 0) {
                            val buf = decoder.getInputBuffer(inputIdx)!!
                            val sampleSize = extractor.readSampleData(buf, 0)
                            if (sampleSize < 0) {
                                decoder.queueInputBuffer(inputIdx, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                                sawInputEos = true
                            } else {
                                decoder.queueInputBuffer(inputIdx, 0, sampleSize, extractor.sampleTime, extractor.sampleFlags)
                                extractor.advance()
                            }
                        }
                    }

                    // Drain decoder → encoder surface
                    val outIdx = decoder.dequeueOutputBuffer(decoderInfo, BUFFER_TIMEOUT_US)
                    if (outIdx >= 0) {
                        if (decoderInfo.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM != 0) {
                            sawOutputEos = true
                        }
                        val render = decoderInfo.size > 0 && (decoderInfo.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == 0
                        decoder.releaseOutputBuffer(outIdx, render)
                        if (render) {
                            lastPtsUs = decoderInfo.presentationTimeUs
                            decodedAnyFrame = true
                        }
                        // Drain encoder output → muxer
                        drainEncoder(endOfStream = false)
                        // Report progress
                        if (clip.durationMs > 0) {
                            val clipProgress = (lastPtsUs / 1000).toFloat() / clip.durationMs
                            onProgress(clipProgressBase + clipWeight * clipProgress.coerceIn(0f, 1f))
                        }
                    }
                }

                if (decodedAnyFrame) {
                    muxerPtsOffsetUs += lastPtsUs + (1_000_000L / settings.fps.coerceAtLeast(1)) // add one frame
                }
            } finally {
                decoder?.stop()
                decoder?.release()
                extractor.release()
            }
            }

            drainEncoder(endOfStream = true)
        } finally {
            encoderSurface.release()
            encoder.stop()
            encoder.release()
        }

        onProgress(1f)
    } finally {
        if (muxerStarted) muxer.stop()
        muxer.release()
    }
}

private fun findVideoTrack(extractor: MediaExtractor): Int? {
    for (i in 0 until extractor.trackCount) {
        val fmt = extractor.getTrackFormat(i)
        if (fmt.getString(MediaFormat.KEY_MIME)?.startsWith("video/") == true) return i
    }
    return null
}

private data class VideoSpec(val width: Int, val height: Int, val mime: String)

private fun probeFirstVideoTrack(context: Context, uri: String): VideoSpec? {
    val extractor = MediaExtractor()
    return try {
        extractor.setDataSource(context, Uri.parse(uri), null)
        val idx = findVideoTrack(extractor) ?: return null
        val fmt = extractor.getTrackFormat(idx)
        val width = if (fmt.containsKey(MediaFormat.KEY_WIDTH)) fmt.getInteger(MediaFormat.KEY_WIDTH) else 1920
        val height = if (fmt.containsKey(MediaFormat.KEY_HEIGHT)) fmt.getInteger(MediaFormat.KEY_HEIGHT) else 1080
        val mime = MediaFormat.MIMETYPE_VIDEO_AVC
        VideoSpec(width, height, mime)
    } catch (e: Exception) {
        Log.e(TAG, "Failed to probe $uri", e)
        null
    } finally {
        extractor.release()
    }
}

private fun saveToMediaStore(context: Context, tempFile: File): Uri {
    val fileName = "VxEditor_${System.currentTimeMillis()}.mp4"
    val values = ContentValues().apply {
        put(MediaStore.Video.Media.DISPLAY_NAME, fileName)
        put(MediaStore.Video.Media.MIME_TYPE, "video/mp4")
        put(MediaStore.Video.Media.RELATIVE_PATH, "Movies/VxEditor")
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            put(MediaStore.Video.Media.IS_PENDING, 1)
        }
    }

    val uri = context.contentResolver.insert(MediaStore.Video.Media.EXTERNAL_CONTENT_URI, values)
        ?: throw IllegalStateException("MediaStore insert failed")

    context.contentResolver.openOutputStream(uri)?.use { out ->
        tempFile.inputStream().use { it.copyTo(out) }
    }

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
        values.clear()
        values.put(MediaStore.Video.Media.IS_PENDING, 0)
        context.contentResolver.update(uri, values, null, null)
    }

    return uri
}
