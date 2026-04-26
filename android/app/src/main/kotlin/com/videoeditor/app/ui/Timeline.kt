package com.videoeditor.app.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.videoeditor.app.ui.theme.ElectricBlue
import com.videoeditor.app.ui.theme.NeonPink

private fun formatTimecode(ms: Long): String {
    val totalSec = ms / 1000
    val min = totalSec / 60
    val sec = totalSec % 60
    val frames = (ms % 1000) / 33
    return "%02d:%02d:%02d".format(min, sec, frames)
}

@Composable
fun Timeline(
    modifier: Modifier = Modifier,
    currentTimeMs: Long = 0L,
    onSeek: (Long) -> Unit = {},
) {
    Surface(
        modifier = modifier,
        color = MaterialTheme.colorScheme.surface,
        shadowElevation = 0.dp,
    ) {
        Box(modifier = Modifier.fillMaxSize()) {
            Column {
                // Time ruler
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(24.dp)
                        .background(MaterialTheme.colorScheme.surfaceVariant),
                    contentAlignment = Alignment.CenterStart,
                ) {
                    Text(
                        text = formatTimecode(currentTimeMs),
                        style = MaterialTheme.typography.labelSmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                        modifier = Modifier.padding(start = 12.dp),
                    )
                }

                // Track area
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .horizontalScroll(rememberScrollState()),
                ) {
                    Row(
                        modifier = Modifier
                            .padding(vertical = 8.dp, horizontal = 12.dp)
                            .height(56.dp),
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        TimelineClip(widthDp = 200, label = "Clip 01")
                        TimelineClip(widthDp = 140, label = "Clip 02")
                        TimelineClip(widthDp = 260, label = "Clip 03")
                    }
                }
            }

            // Playhead line
            val playheadX = 80.dp
            Box(
                modifier = Modifier
                    .fillMaxHeight()
                    .width(2.dp)
                    .align(Alignment.CenterStart)
                    .offset(x = playheadX)
                    .background(ElectricBlue),
            )
            // Playhead cap
            Box(
                modifier = Modifier
                    .size(width = 10.dp, height = 14.dp)
                    .align(Alignment.TopStart)
                    .offset(x = playheadX - 4.dp)
                    .clip(RoundedCornerShape(bottomStart = 3.dp, bottomEnd = 3.dp))
                    .background(ElectricBlue),
            )
        }
    }
}

@Composable
fun TimelineClip(widthDp: Int, label: String) {
    Box(
        modifier = Modifier
            .width(widthDp.dp)
            .fillMaxHeight()
            .padding(horizontal = 2.dp)
            .clip(RoundedCornerShape(10.dp))
            .background(NeonPink.copy(alpha = 0.12f))
            .border(1.dp, NeonPink.copy(alpha = 0.35f), RoundedCornerShape(10.dp)),
        contentAlignment = Alignment.CenterStart,
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.labelSmall,
            color = NeonPink.copy(alpha = 0.9f),
            modifier = Modifier.padding(start = 10.dp),
        )
    }
}
