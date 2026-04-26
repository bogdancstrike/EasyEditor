package com.videoeditor.app.engine

import android.content.Context
import android.media.MediaMetadataRetriever
import android.net.Uri
import android.util.Log
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

data class MediaAsset(val uri: String, val durationMs: Long, val width: Int, val height: Int)
data class TimelineClip(val uri: String, val durationMs: Long, val lutPresetOrdinal: Int = 0)

object NativeBridge {
    init {
        System.loadLibrary("videoeditor")
    }

    @Volatile
    private var projectHandle: Long = 0

    private val lock = Any()

    private val _mediaAssets = MutableStateFlow<List<MediaAsset>>(emptyList())
    val mediaAssets: StateFlow<List<MediaAsset>> = _mediaAssets.asStateFlow()

    private val _clips = MutableStateFlow<List<TimelineClip>>(emptyList())
    val clips: StateFlow<List<TimelineClip>> = _clips.asStateFlow()

    private val undoStack = ArrayDeque<List<TimelineClip>>()
    private val redoStack = ArrayDeque<List<TimelineClip>>()

    fun initProject(name: String) {
        if (projectHandle == 0L) {
            synchronized(lock) {
                if (projectHandle == 0L) {
                    projectHandle = nativeCreateProject(name)
                }
            }
        }
    }

    fun importMediaAsset(context: Context, uri: Uri): Boolean {
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
            val asset = MediaAsset(uri.toString(), durationMs, width, height)
            synchronized(lock) {
                if (_mediaAssets.value.none { it.uri == asset.uri }) {
                    _mediaAssets.value = _mediaAssets.value + asset
                }
            }
            true
        } catch (e: Exception) {
            Log.e("NativeBridge", "Error extracting metadata for $uri", e)
            false
        } finally {
            retriever.release()
        }
    }

    fun addMediaAssetToTimeline(asset: MediaAsset): Boolean {
        if (projectHandle == 0L) initProject("PoC Project")

        val status = nativeAddAsset(projectHandle, asset.uri, asset.durationMs, asset.width, asset.height)
        if (status != 0) return false

        synchronized(lock) {
            replaceTimelineLocked(_clips.value + TimelineClip(asset.uri, asset.durationMs))
        }
        return true
    }

    fun removeTimelineClip(index: Int) {
        synchronized(lock) {
            if (index !in _clips.value.indices) return
            replaceTimelineLocked(_clips.value.filterIndexed { clipIndex, _ -> clipIndex != index })
        }
    }

    fun moveTimelineClip(fromIndex: Int, toIndex: Int) {
        synchronized(lock) {
            val current = _clips.value.toMutableList()
            if (fromIndex !in current.indices || toIndex !in current.indices || fromIndex == toIndex) return
            val clip = current.removeAt(fromIndex)
            current.add(toIndex, clip)
            replaceTimelineLocked(current)
        }
    }

    fun setTimelineClipLut(index: Int, lutPresetOrdinal: Int) {
        synchronized(lock) {
            replaceTimelineLocked(_clips.value.mapIndexed { clipIndex, clip ->
                if (clipIndex == index) clip.copy(lutPresetOrdinal = lutPresetOrdinal) else clip
            })
        }
    }

    fun undoTimelineEdit(): Boolean = synchronized(lock) {
        if (undoStack.isEmpty()) return@synchronized false
        redoStack.addLast(_clips.value)
        _clips.value = undoStack.removeLast()
        true
    }

    fun redoTimelineEdit(): Boolean = synchronized(lock) {
        if (redoStack.isEmpty()) return@synchronized false
        undoStack.addLast(_clips.value)
        _clips.value = redoStack.removeLast()
        true
    }

    fun canUndo(): Boolean = synchronized(lock) { undoStack.isNotEmpty() }

    fun canRedo(): Boolean = synchronized(lock) { redoStack.isNotEmpty() }

    private fun replaceTimelineLocked(next: List<TimelineClip>) {
        val current = _clips.value
        if (current == next) return
        undoStack.addLast(current)
        redoStack.clear()
        _clips.value = next
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
