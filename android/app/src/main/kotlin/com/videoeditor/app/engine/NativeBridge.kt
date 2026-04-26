package com.videoeditor.app.engine

import android.content.Context
import android.media.MediaMetadataRetriever
import android.net.Uri
import android.util.Log

object NativeBridge {
    init {
        System.loadLibrary("videoeditor")
    }

    @Volatile
    private var projectHandle: Long = 0

    private val lock = Any()

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
        if (projectHandle == 0L) {
            initProject("PoC Project")
        }

        val retriever = MediaMetadataRetriever()
        return try {
            retriever.setDataSource(context, uri)
            val durationStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION)
            val widthStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH)
            val heightStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT)

            if (durationStr == null || widthStr == null || heightStr == null) {
                Log.e("NativeBridge", "Failed to retrieve complete metadata for $uri (duration: $durationStr, width: $widthStr, height: $heightStr)")
                return false
            }

            val durationMs = durationStr.toLong()
            val width = widthStr.toInt()
            val height = heightStr.toInt()

            val result = nativeAddAsset(projectHandle, uri.toString(), durationMs, width, height)
            result == 0 // 0 is VX_OK
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

    external fun nativeVersion(): String
    external fun nativeVulkanSmokeTest(): String

    private external fun nativeCreateProject(name: String): Long
    private external fun nativeAddAsset(handle: Long, path: String, durationMs: Long, width: Int, height: Int): Int
    private external fun nativeRenderFrame(handle: Long, surface: android.view.Surface, timeMs: Long): Int
}
