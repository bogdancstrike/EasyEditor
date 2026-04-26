package com.videoeditor.app.media

import android.content.Context
import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.os.SystemClock
import android.view.Surface
import androidx.annotation.RawRes
import java.util.concurrent.atomic.AtomicBoolean

class MediaCodecPlayer(
    private val context: Context,
) {
    private val isPlaying = AtomicBoolean(false)

    fun stop() {
        isPlaying.set(false)
    }

    fun playLoopRawResource(
        @RawRes rawResourceId: Int,
        outputSurface: Surface,
        onStatusUpdate: (String) -> Unit,
    ) {
        isPlaying.set(true)
        while (isPlaying.get()) {
            try {
                playOnce(rawResourceId, outputSurface, onStatusUpdate)
            } catch (e: Exception) {
                onStatusUpdate("Playback error: ${e.message}")
                break
            }
        }
    }

    private fun playOnce(
        @RawRes rawResourceId: Int,
        outputSurface: Surface,
        onStatusUpdate: (String) -> Unit,
    ) {
        val extractor = MediaExtractor()
        context.resources.openRawResourceFd(rawResourceId).use { descriptor ->
            extractor.setDataSource(
                descriptor.fileDescriptor,
                descriptor.startOffset,
                descriptor.length,
            )
        }

        val trackIndex = findVideoTrack(extractor)
        extractor.selectTrack(trackIndex)
        val inputFormat = extractor.getTrackFormat(trackIndex)
        val mimeType = requireNotNull(inputFormat.getString(MediaFormat.KEY_MIME)) {
            "Video track has no MIME type"
        }
        val width = inputFormat.getInteger(MediaFormat.KEY_WIDTH)
        val height = inputFormat.getInteger(MediaFormat.KEY_HEIGHT)

        val codec = MediaCodec.createDecoderByType(mimeType)
        val decoderName = codec.name
        var started = false

        try {
            codec.configure(inputFormat, outputSurface, null, 0)
            codec.start()
            started = true

            onStatusUpdate("Playing: ${width}x$height $mimeType ($decoderName)")

            val bufferInfo = MediaCodec.BufferInfo()
            var sawInputEndOfStream = false
            var sawOutputEndOfStream = false
            var startRealtimeMs = -1L

            while (!sawOutputEndOfStream && isPlaying.get()) {
                if (!sawInputEndOfStream) {
                    val inputIndex = codec.dequeueInputBuffer(BUFFER_TIMEOUT_US)
                    if (inputIndex >= 0) {
                        val inputBuffer = requireNotNull(codec.getInputBuffer(inputIndex))
                        val sampleSize = extractor.readSampleData(inputBuffer, 0)
                        
                        if (sampleSize < 0) {
                            codec.queueInputBuffer(
                                inputIndex,
                                0,
                                0,
                                0L,
                                MediaCodec.BUFFER_FLAG_END_OF_STREAM
                            )
                            sawInputEndOfStream = true
                        } else {
                            codec.queueInputBuffer(
                                inputIndex,
                                0,
                                sampleSize,
                                extractor.sampleTime,
                                extractor.sampleFlags
                            )
                            extractor.advance()
                        }
                    }
                }

                val outputIndex = codec.dequeueOutputBuffer(bufferInfo, BUFFER_TIMEOUT_US)
                if (outputIndex >= 0) {
                    if (bufferInfo.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM != 0) {
                        sawOutputEndOfStream = true
                    }

                    val isCodecConfig = bufferInfo.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG != 0
                    val shouldRender = bufferInfo.size > 0 && !isCodecConfig

                    if (shouldRender) {
                        if (startRealtimeMs < 0) {
                            startRealtimeMs = SystemClock.elapsedRealtime()
                        } else {
                            val ptsMs = bufferInfo.presentationTimeUs / 1000
                            val elapsedMs = SystemClock.elapsedRealtime() - startRealtimeMs
                            val sleepTimeMs = ptsMs - elapsedMs
                            if (sleepTimeMs > 0) {
                                SystemClock.sleep(sleepTimeMs)
                            }
                        }
                    }

                    codec.releaseOutputBuffer(outputIndex, shouldRender)
                }
            }
        } finally {
            if (started) {
                codec.stop()
            }
            codec.release()
            extractor.release()
        }
    }

    private fun findVideoTrack(extractor: MediaExtractor): Int {
        for (index in 0 until extractor.trackCount) {
            val format = extractor.getTrackFormat(index)
            val mimeType = format.getString(MediaFormat.KEY_MIME)
            if (mimeType?.startsWith("video/") == true) {
                return index
            }
        }
        throw IllegalArgumentException("Resource does not contain a video track")
    }

    private companion object {
        const val BUFFER_TIMEOUT_US = 10_000L
    }
}
