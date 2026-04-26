package com.videoeditor.app.ui

import android.graphics.SurfaceTexture
import android.os.Handler
import android.os.Looper
import android.view.Surface
import android.view.TextureView
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import com.videoeditor.app.R
import com.videoeditor.app.media.MediaCodecPlayer

@Composable
fun MediaCodecPreview(
    modifier: Modifier = Modifier,
) {
    val context = LocalContext.current
    val mainHandler = remember { Handler(Looper.getMainLooper()) }
    val player = remember { MediaCodecPlayer(context.applicationContext) }
    var decodeStatus by remember { mutableStateOf("MediaCodec pending") }

    DisposableEffect(Unit) {
        onDispose {
            player.stop()
        }
    }

    Column(modifier = modifier) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .aspectRatio(16f / 9f)
                .background(Color.Black),
        ) {
            AndroidView(
                modifier = Modifier.matchParentSize(),
                factory = { viewContext ->
                    TextureView(viewContext).apply {
                        surfaceTextureListener = object : TextureView.SurfaceTextureListener {
                            override fun onSurfaceTextureAvailable(
                                surfaceTexture: SurfaceTexture,
                                width: Int,
                                height: Int,
                            ) {
                                Thread(
                                    {
                                        val surface = Surface(surfaceTexture)
                                        player.playLoopRawResource(
                                            rawResourceId = R.raw.phase0_h264_sample,
                                            outputSurface = surface,
                                        ) { status ->
                                            mainHandler.post {
                                                decodeStatus = status
                                            }
                                        }
                                        surface.release()
                                    },
                                    "VxPlaybackLoop",
                                ).start()
                            }

                            override fun onSurfaceTextureSizeChanged(
                                surfaceTexture: SurfaceTexture,
                                width: Int,
                                height: Int,
                            ) = Unit

                            override fun onSurfaceTextureDestroyed(surfaceTexture: SurfaceTexture): Boolean {
                                player.stop()
                                return true
                            }

                            override fun onSurfaceTextureUpdated(surfaceTexture: SurfaceTexture) = Unit
                        }
                    }
                },
            )
        }
        Text(
            modifier = Modifier.padding(top = 8.dp),
            text = decodeStatus,
            color = Color(0xFF4D5B5A),
            style = MaterialTheme.typography.bodySmall,
        )
    }
}
