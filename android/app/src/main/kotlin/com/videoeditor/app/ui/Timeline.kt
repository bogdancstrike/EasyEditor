package com.videoeditor.app.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.videoeditor.app.ui.theme.ElectricBlue
import com.videoeditor.app.ui.theme.NeonPink

@Composable
fun Timeline(modifier: Modifier = Modifier) {
    Box(
        modifier = modifier
            .background(MaterialTheme.colorScheme.surface)
            .border(1.dp, Color.White.copy(alpha = 0.05f))
    ) {
        Column {
            // Time Ruler
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(32.dp)
                    .background(Color.Black.copy(alpha = 0.2f)),
                contentAlignment = Alignment.CenterStart
            ) {
                // Bottom border for ruler
                Box(modifier = Modifier.fillMaxWidth().height(1.dp).align(Alignment.BottomCenter).background(Color.White.copy(alpha = 0.05f)))
                
                Text(
                    "00:00:00:00",
                    style = MaterialTheme.typography.labelSmall,
                    color = Color.White.copy(alpha = 0.4f),
                    modifier = Modifier.padding(start = 12.dp)
                )
            }

            // Track Area
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .horizontalScroll(rememberScrollState())
            ) {
                Row(
                    modifier = Modifier
                        .padding(vertical = 24.dp, horizontal = 12.dp)
                        .height(80.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    // Mock Clips
                    TimelineClip(width = 240.dp, label = "Clip 01")
                    TimelineClip(width = 180.dp, label = "Clip 02")
                    TimelineClip(width = 320.dp, label = "Clip 03")
                }
            }
        }

        // Static Moving Playhead (Positioned at fixed offset for mock)
        Box(
            modifier = Modifier
                .fillMaxHeight()
                .width(2.dp)
                .align(Alignment.CenterStart)
                .offset(x = 120.dp)
                .background(ElectricBlue)
        )
        
        // Playhead Cap
        Box(
            modifier = Modifier
                .size(width = 8.dp, height = 32.dp)
                .align(Alignment.TopStart)
                .offset(x = 117.dp)
                .clip(RoundedCornerShape(bottomStart = 4.dp, bottomEnd = 4.dp))
                .background(ElectricBlue)
        )
    }
}

@Composable
fun TimelineClip(width: androidx.compose.ui.unit.Dp, label: String) {
    Box(
        modifier = Modifier
            .width(width)
            .fillMaxHeight()
            .padding(horizontal = 2.dp)
            .clip(RoundedCornerShape(12.dp))
            .background(NeonPink.copy(alpha = 0.15f))
            .border(1.dp, NeonPink.copy(alpha = 0.4f), RoundedCornerShape(12.dp)),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.labelSmall,
            color = NeonPink.copy(alpha = 0.8f)
        )
    }
}
