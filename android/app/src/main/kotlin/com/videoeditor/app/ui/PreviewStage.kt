package com.videoeditor.app.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.FastForward
import androidx.compose.material.icons.filled.FastRewind
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.videoeditor.app.media.LutPreset

@Composable
fun PreviewStage(
    modifier: Modifier = Modifier,
    isPlaying: Boolean = false,
    currentTimeMs: Long = 0L,
    lutPreset: LutPreset = LutPreset.NONE,
    onPlayingChange: (Boolean) -> Unit = {},
    onSeek: (Long) -> Unit = {},
    onTimeUpdate: (Long) -> Unit = {},
) {
    Box(modifier = modifier.background(Color.Black)) {
        MediaCodecPreview(
            modifier = Modifier.fillMaxSize(),
            isPlaying = isPlaying,
            currentTimeMs = currentTimeMs,
            lutPreset = lutPreset,
            onTimeUpdate = onTimeUpdate,
        )

        // Playback controls overlay
        Row(
            modifier = Modifier
                .align(Alignment.BottomCenter)
                .padding(bottom = 12.dp)
                .clip(RoundedCornerShape(32.dp))
                .background(Color.Black.copy(alpha = 0.45f))
                .padding(horizontal = 12.dp, vertical = 6.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(4.dp),
        ) {
            IconButton(
                onClick = { onSeek((currentTimeMs - 5_000L).coerceAtLeast(0L)) },
                modifier = Modifier.size(40.dp),
            ) {
                Icon(
                    Icons.Default.FastRewind,
                    contentDescription = "Skip back",
                    tint = Color.White.copy(alpha = 0.85f),
                    modifier = Modifier.size(22.dp),
                )
            }

            FilledIconButton(
                onClick = { onPlayingChange(!isPlaying) },
                modifier = Modifier.size(48.dp),
                colors = IconButtonDefaults.filledIconButtonColors(
                    containerColor = MaterialTheme.colorScheme.primary,
                ),
                shape = CircleShape,
            ) {
                Icon(
                    if (isPlaying) Icons.Default.Pause else Icons.Default.PlayArrow,
                    contentDescription = if (isPlaying) "Pause" else "Play",
                    modifier = Modifier.size(26.dp),
                )
            }

            IconButton(
                onClick = { onSeek(currentTimeMs + 5_000L) },
                modifier = Modifier.size(40.dp),
            ) {
                Icon(
                    Icons.Default.FastForward,
                    contentDescription = "Skip forward",
                    tint = Color.White.copy(alpha = 0.85f),
                    modifier = Modifier.size(22.dp),
                )
            }
        }
    }
}
