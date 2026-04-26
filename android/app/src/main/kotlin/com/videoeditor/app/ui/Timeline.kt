package com.videoeditor.app.ui

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.KeyboardArrowLeft
import androidx.compose.material.icons.filled.KeyboardArrowRight
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.ContentScale
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
    onClipRemove: (Int) -> Unit = {},
    onClipMoveLeft: (Int) -> Unit = {},
    onClipMoveRight: (Int) -> Unit = {},
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
            val horizontalInset = 12.dp
            val trackStartPx = with(density) { horizontalInset.toPx() }
            val trackWidthPx = (timelineWidthPx - trackStartPx * 2).coerceAtLeast(1f)
            val trackWidth = maxWidth - 24.dp
            val playheadFraction = currentTimeMs.coerceIn(0L, totalDurationMs).toFloat() / totalDurationMs
            val playheadX = with(density) { (trackStartPx + trackWidthPx * playheadFraction).toDp() }
            fun seekFromX(xPx: Float) {
                val clamped = xPx.coerceIn(trackStartPx, trackStartPx + trackWidthPx)
                onSeek(((clamped - trackStartPx) / trackWidthPx * totalDurationMs).toLong())
            }

            Column {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(24.dp)
                        .background(MaterialTheme.colorScheme.surfaceVariant)
                        .padding(horizontal = horizontalInset),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.SpaceBetween,
                ) {
                    val tickCount = 5
                    repeat(tickCount) { tick ->
                        val tickTime = (totalDurationMs * tick / (tickCount - 1)).coerceAtMost(totalDurationMs)
                        Text(
                            text = formatRulerTime(tickTime),
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                    }
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
                                .padding(vertical = 12.dp, horizontal = horizontalInset)
                                .height(76.dp),
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
                                    onRemove = { onClipRemove(index) },
                                    onMoveLeft = { onClipMoveLeft(index) },
                                    onMoveRight = { onClipMoveRight(index) },
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
    onRemove: () -> Unit,
    onMoveLeft: () -> Unit,
    onMoveRight: () -> Unit,
) {
    val clipWidth = (timelineWidth * (clip.durationMs.toFloat() / totalDurationMs)).coerceAtLeast(84.dp)
    val borderAlpha = if (isSelected) 1f else 0.35f
    val frame = rememberVideoFrameBitmap(clip.uri)
    Box(
        modifier = Modifier
            .width(clipWidth)
            .fillMaxHeight()
            .padding(horizontal = 2.dp)
            .clip(RoundedCornerShape(10.dp))
            .background(NeonPink.copy(alpha = if (isSelected) 0.22f else 0.12f))
            .border(2.dp, NeonPink.copy(alpha = borderAlpha), RoundedCornerShape(10.dp))
            .clickable(onClick = onSelected),
    ) {
        Row(modifier = Modifier.fillMaxSize()) {
            repeat(5) {
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxHeight()
                        .padding(end = 1.dp),
                ) {
                    if (frame != null) {
                        Image(
                            bitmap = frame.asImageBitmap(),
                            contentDescription = null,
                            modifier = Modifier.fillMaxSize(),
                            contentScale = ContentScale.Crop,
                        )
                    } else {
                        Box(
                            modifier = Modifier
                                .fillMaxSize()
                                .background(
                                    Brush.linearGradient(
                                        listOf(
                                            NeonPink.copy(alpha = 0.25f),
                                            ElectricBlue.copy(alpha = 0.18f),
                                        ),
                                    ),
                                ),
                        )
                    }
                }
            }
        }
        Box(
            modifier = Modifier
                .matchParentSize()
                .background(Color.Black.copy(alpha = if (isSelected) 0.12f else 0.24f)),
        )
        Text(
            text = "Clip %02d".format(index),
            style = MaterialTheme.typography.labelSmall,
            color = Color.White,
            modifier = Modifier
                .align(Alignment.BottomStart)
                .padding(start = 8.dp, bottom = 6.dp)
                .clip(RoundedCornerShape(5.dp))
                .background(Color.Black.copy(alpha = 0.55f))
                .padding(horizontal = 6.dp, vertical = 2.dp),
        )
        if (isSelected) {
            Row(
                modifier = Modifier
                    .align(Alignment.TopEnd)
                    .padding(3.dp)
                    .clip(RoundedCornerShape(10.dp))
                    .background(Color.Black.copy(alpha = 0.58f)),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                TimelineClipButton(Icons.Default.KeyboardArrowLeft, "Move clip left", onMoveLeft)
                TimelineClipButton(Icons.Default.Delete, "Remove clip", onRemove)
                TimelineClipButton(Icons.Default.KeyboardArrowRight, "Move clip right", onMoveRight)
            }
        }
    }
}

@Composable
private fun TimelineClipButton(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    description: String,
    onClick: () -> Unit,
) {
    IconButton(onClick = onClick, modifier = Modifier.size(28.dp)) {
        Icon(
            icon,
            contentDescription = description,
            tint = Color.White,
            modifier = Modifier.size(18.dp),
        )
    }
}

private fun formatRulerTime(ms: Long): String {
    val totalSec = ms / 1000
    return "%02d:%02d".format(totalSec / 60, totalSec % 60)
}
