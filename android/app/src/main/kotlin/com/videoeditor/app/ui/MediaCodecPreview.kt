package com.videoeditor.app.ui

import android.graphics.SurfaceTexture
import android.view.Surface
import android.view.TextureView
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.viewinterop.AndroidView
import com.videoeditor.app.engine.NativeBridge
import com.videoeditor.app.engine.TimelineClip
import com.videoeditor.app.media.LutPreset
import com.videoeditor.app.media.VideoPreviewEngine

@Composable
fun MediaCodecPreview(
    modifier: Modifier = Modifier,
    isPlaying: Boolean = false,
    currentTimeMs: Long = 0L,
    lutPreset: LutPreset = LutPreset.NONE,
    onTimeUpdate: (Long) -> Unit = {},
) {
    val context = LocalContext.current
    var currentSurface by remember { mutableStateOf<Surface?>(null) }
    var engine by remember { mutableStateOf<VideoPreviewEngine?>(null) }

    // Observe clip changes from NativeBridge
    val clips by NativeBridge.clips.collectAsState()

    // Build the engine when the display Surface is ready; tear it down on dispose
    DisposableEffect(currentSurface) {
        val surf = currentSurface ?: return@DisposableEffect onDispose {}
        val e = VideoPreviewEngine(context, surf)
        e.setClips(NativeBridge.getTimelineClips())
        e.setLut(lutPreset)
        engine = e
        onDispose {
            e.stop()
            e.release()
            engine = null
        }
    }

    // Propagate clip changes to existing engine (no rebuild needed)
    LaunchedEffect(clips) {
        engine?.setClips(clips)
    }

    // Propagate LUT changes
    LaunchedEffect(lutPreset) {
        engine?.setLut(lutPreset)
    }

    // Start / stop
    LaunchedEffect(isPlaying) {
        if (isPlaying) {
            engine?.play(currentTimeMs, onTimeUpdate)
        } else {
            engine?.stop()
        }
    }

    LaunchedEffect(currentTimeMs, clips, lutPreset) {
        if (!isPlaying && clips.isNotEmpty()) {
            engine?.renderAt(currentTimeMs, onTimeUpdate)
        }
    }

    Box(modifier = modifier.background(Color.Black)) {
        AndroidView(
            modifier = Modifier.fillMaxSize(),
            factory = { ctx ->
                TextureView(ctx).apply {
                    surfaceTextureListener = object : TextureView.SurfaceTextureListener {
                        override fun onSurfaceTextureAvailable(st: SurfaceTexture, w: Int, h: Int) {
                            currentSurface = Surface(st)
                        }
                        override fun onSurfaceTextureSizeChanged(st: SurfaceTexture, w: Int, h: Int) = Unit
                        override fun onSurfaceTextureDestroyed(st: SurfaceTexture): Boolean {
                            engine?.stop()
                            currentSurface?.release()
                            currentSurface = null
                            return true
                        }
                        override fun onSurfaceTextureUpdated(st: SurfaceTexture) = Unit
                    }
                }
            },
        )
    }
}
