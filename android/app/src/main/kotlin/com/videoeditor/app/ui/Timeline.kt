package com.videoeditor.app.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import com.videoeditor.app.engine.NativeBridge
import com.videoeditor.app.engine.TimelineClip as EngineTimelineClip
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
    selectedClipIndex: Int? = null,
    onSeek: (Long) -> Unit = {},
    onClipSelected: (Int) -> Unit = {},
) {
    val clips by NativeBridge.clips.collectAsState()
    val totalDurationMs = clips.sumOf { it.durationMs }.coerceAtLeast(1L)

    Surface(
        modifier = modifier,
        color = MaterialTheme.colorScheme.surface,
        shadowElevation = 0.dp,
    ) {
        BoxWithConstraints(modifier = Modifier.fillMaxSize()) {
            val density = LocalDensity.current
            val timelineWidthPx = with(density) { maxWidth.toPx() }.coerceAtLeast(1f)
            val trackWidth = maxWidth - 24.dp
            val playheadFraction = currentTimeMs.coerceIn(0L, totalDurationMs).toFloat() / totalDurationMs
            val playheadX = with(density) { (timelineWidthPx * playheadFraction).toDp() }
            fun seekFromX(xPx: Float) {
                val clamped = xPx.coerceIn(0f, timelineWidthPx)
                onSeek((clamped / timelineWidthPx * totalDurationMs).toLong())
            }

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
                        .pointerInput(totalDurationMs, timelineWidthPx) {
                            detectTapGestures { offset -> seekFromX(offset.x) }
                        }
                        .pointerInput(totalDurationMs, timelineWidthPx) {
                            detectDragGestures(
                                onDragStart = { offset -> seekFromX(offset.x) },
                                onDrag = { change, _ -> seekFromX(change.position.x) },
                            )
                        },
                ) {
                    if (clips.isEmpty()) {
                        Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                            Text(
                                text = "Add clips from Media",
                                style = MaterialTheme.typography.bodySmall,
                                color = MaterialTheme.colorScheme.onSurfaceVariant,
                            )
                        }
                    } else {
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(vertical = 10.dp, horizontal = 12.dp)
                                .height(66.dp),
                            verticalAlignment = Alignment.CenterVertically,
                        ) {
                            clips.forEachIndexed { index, clip ->
                                TimelineClip(
                                    clip = clip,
                                    index = index + 1,
                                    isSelected = selectedClipIndex == index,
                                    totalDurationMs = totalDurationMs,
                                    timelineWidth = trackWidth,
                                    onSelected = { onClipSelected(index) },
                                )
                            }
                        }
                    }
                }
            }

            // Playhead line
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
fun TimelineClip(
    clip: EngineTimelineClip,
    index: Int,
    isSelected: Boolean,
    totalDurationMs: Long,
    timelineWidth: androidx.compose.ui.unit.Dp,
    onSelected: () -> Unit,
) {
    val clipWidth = (timelineWidth * (clip.durationMs.toFloat() / totalDurationMs)).coerceAtLeast(84.dp)
    val borderAlpha = if (isSelected) 1f else 0.35f
    Box(
        modifier = Modifier
            .width(clipWidth)
            .fillMaxHeight()
            .padding(horizontal = 2.dp)
            .clip(RoundedCornerShape(10.dp))
            .background(NeonPink.copy(alpha = if (isSelected) 0.22f else 0.12f))
            .border(2.dp, NeonPink.copy(alpha = borderAlpha), RoundedCornerShape(10.dp))
            .clickable(onClick = onSelected),
        contentAlignment = Alignment.CenterStart,
    ) {
        Text(
            text = "Clip %02d".format(index),
            style = MaterialTheme.typography.labelSmall,
            color = NeonPink.copy(alpha = 0.9f),
            modifier = Modifier.padding(start = 10.dp),
        )
    }
}
