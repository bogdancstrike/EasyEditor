package com.videoeditor.app.engine

import android.content.Context
import android.media.MediaMetadataRetriever
import android.net.Uri
import android.util.Log
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

data class TimelineClip(val uri: String, val durationMs: Long)

object NativeBridge {
    init {
        System.loadLibrary("videoeditor")
    }

    @Volatile
    private var projectHandle: Long = 0

    private val lock = Any()

    private val _clips = MutableStateFlow<List<TimelineClip>>(emptyList())
    val clips: StateFlow<List<TimelineClip>> = _clips.asStateFlow()

    fun initProject(name: String) {
        if (projectHandle == 0L) {
            synchronized(lock) {
                if (projectHandle == 0L) {
                    projectHandle = nativeCreateProject(name)
                }
            }
        }
    }

    fun addMediaAsset(context: Context, uri: Uri): Boolean {
        if (projectHandle == 0L) initProject("PoC Project")

        val retriever = MediaMetadataRetriever()
        return try {
            retriever.setDataSource(context, uri)
            val durationStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION)
            val widthStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH)
            val heightStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT)

            if (durationStr == null || widthStr == null || heightStr == null) {
                Log.e("NativeBridge", "Missing metadata for $uri")
                return false
            }

            val durationMs = durationStr.toLong()
            val width = widthStr.toInt()
            val height = heightStr.toInt()
            val status = nativeAddAsset(projectHandle, uri.toString(), durationMs, width, height)
            if (status == 0) {
                synchronized(lock) {
                    _clips.value = _clips.value + TimelineClip(uri.toString(), durationMs)
                }
                true
            } else {
                false
            }
        } catch (e: Exception) {
            Log.e("NativeBridge", "Error extracting metadata for $uri", e)
            false
        } finally {
            retriever.release()
        }
    }

    fun renderFrame(surface: android.view.Surface, timeMs: Long): Int {
        if (projectHandle == 0L) return -1
        return nativeRenderFrame(projectHandle, surface, timeMs)
    }

    fun getTimelineClips(): List<TimelineClip> = _clips.value

    fun getProjectDurationMs(): Long = _clips.value.sumOf { it.durationMs }

    external fun nativeVersion(): String
    external fun nativeVulkanSmokeTest(): String

    private external fun nativeCreateProject(name: String): Long
    private external fun nativeAddAsset(handle: Long, path: String, durationMs: Long, width: Int, height: Int): Int
    private external fun nativeRenderFrame(handle: Long, surface: android.view.Surface, timeMs: Long): Int
}
