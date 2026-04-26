package com.videoeditor.app.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.AutoFixHigh
import androidx.compose.material.icons.filled.ColorLens
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Tune
import androidx.compose.material.icons.filled.VideoLibrary
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.videoeditor.app.media.LutPreset

enum class ToolTab(val label: String, val icon: ImageVector) {
    MEDIA("Media", Icons.Default.VideoLibrary),
    EFFECTS("Effects", Icons.Default.AutoFixHigh),
    LOOK("Look", Icons.Default.ColorLens),
    ADJUST("Adjust", Icons.Default.Tune),
}

@Composable
fun FloatingToolDock(
    selectedTool: ToolTab?,
    onToolSelected: (ToolTab) -> Unit,
    modifier: Modifier = Modifier,
) {
    Surface(
        modifier = modifier,
        shape = RoundedCornerShape(18.dp),
        color = MaterialTheme.colorScheme.surface.copy(alpha = 0.94f),
        tonalElevation = 4.dp,
        shadowElevation = 16.dp,
    ) {
        Column(
            modifier = Modifier
                .width(56.dp)
                .padding(vertical = 6.dp),
            verticalArrangement = Arrangement.spacedBy(2.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            ToolTab.entries.forEach { tab ->
                FloatingToolItem(
                    tab = tab,
                    isSelected = selectedTool == tab,
                    onClick = { onToolSelected(tab) },
                )
            }
        }
    }
}

@Composable
private fun FloatingToolItem(tab: ToolTab, isSelected: Boolean, onClick: () -> Unit) {
    val color = if (isSelected) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onSurfaceVariant
    Box(
        modifier = Modifier
            .size(48.dp)
            .clip(RoundedCornerShape(14.dp))
            .background(if (isSelected) MaterialTheme.colorScheme.primary.copy(alpha = 0.12f) else Color.Transparent)
            .clickable(onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        Icon(
            tab.icon,
            contentDescription = tab.label,
            tint = color,
            modifier = Modifier.size(24.dp),
        )
    }
}

@Composable
fun ToolPanel(
    selectedTab: ToolTab,
    selectedLut: LutPreset = LutPreset.NONE,
    onLutSelected: (LutPreset) -> Unit = {},
    selectedClipIndex: Int? = null,
    onClipLutSelected: (LutPreset) -> Unit = {},
    onClose: () -> Unit = {},
    modifier: Modifier = Modifier,
) {
    Surface(
        modifier = modifier,
        shape = RoundedCornerShape(18.dp),
        color = MaterialTheme.colorScheme.surface.copy(alpha = 0.96f),
        shadowElevation = 18.dp,
        tonalElevation = 4.dp,
    ) {
        Column(modifier = Modifier.fillMaxSize()) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(52.dp)
                    .padding(start = 16.dp, end = 8.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Icon(
                    selectedTab.icon,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.primary,
                    modifier = Modifier.size(20.dp),
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text(
                    selectedTab.label,
                    style = MaterialTheme.typography.titleSmall,
                    fontWeight = FontWeight.SemiBold,
                    color = MaterialTheme.colorScheme.onSurface,
                )
                Spacer(modifier = Modifier.weight(1f))
                IconButton(onClick = onClose, modifier = Modifier.size(36.dp)) {
                    Icon(
                        Icons.Default.Close,
                        contentDescription = "Collapse panel",
                        modifier = Modifier.size(18.dp),
                        tint = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
            }
            HorizontalDivider(color = MaterialTheme.colorScheme.outline.copy(alpha = 0.6f))
            when (selectedTab) {
                ToolTab.MEDIA -> MediaPool(modifier = Modifier.fillMaxSize().padding(top = 4.dp))
                ToolTab.LOOK -> LutSelectorPanel(
                    selectedLut = selectedLut,
                    onLutSelected = onLutSelected,
                    selectedClipIndex = selectedClipIndex,
                    onClipLutSelected = onClipLutSelected,
                    modifier = Modifier.fillMaxSize(),
                )
                else -> PlaceholderPanel(label = "${selectedTab.label} — Phase 2", modifier = Modifier.fillMaxSize())
            }
        }
    }
}

@Composable
private fun LutSelectorPanel(
    selectedLut: LutPreset,
    onLutSelected: (LutPreset) -> Unit,
    selectedClipIndex: Int?,
    onClipLutSelected: (LutPreset) -> Unit,
    modifier: Modifier = Modifier,
) {
    Column(modifier = modifier.padding(horizontal = 16.dp)) {
        Text(
            "Project Look",
            style = MaterialTheme.typography.titleSmall,
            fontWeight = FontWeight.SemiBold,
            modifier = Modifier.padding(vertical = 10.dp),
        )
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(12.dp),
            contentPadding = PaddingValues(bottom = 12.dp),
        ) {
            items(LutPreset.entries) { preset ->
                LutPresetChip(
                    preset = preset,
                    isSelected = selectedLut == preset,
                    onClick = { onLutSelected(preset) },
                )
            }
        }
        Text(
            text = selectedClipIndex?.let { "Clip %02d Look".format(it + 1) } ?: "Select a timeline clip for clip look",
            style = MaterialTheme.typography.titleSmall,
            color = if (selectedClipIndex == null) MaterialTheme.colorScheme.onSurfaceVariant else MaterialTheme.colorScheme.onSurface,
            fontWeight = FontWeight.SemiBold,
            modifier = Modifier.padding(top = 8.dp, bottom = 10.dp),
        )
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(12.dp),
            contentPadding = PaddingValues(bottom = 12.dp),
        ) {
            items(LutPreset.entries) { preset ->
                LutPresetChip(
                    preset = preset,
                    isSelected = false,
                    onClick = {
                        if (selectedClipIndex != null) onClipLutSelected(preset)
                    },
                )
            }
        }
    }
}

@Composable
private fun LutPresetChip(preset: LutPreset, isSelected: Boolean, onClick: () -> Unit) {
    val primary = MaterialTheme.colorScheme.primary
    val surface = MaterialTheme.colorScheme.surfaceVariant

    Box(
        modifier = Modifier
            .size(width = 72.dp, height = 72.dp)
            .clip(RoundedCornerShape(12.dp))
            .background(if (isSelected) primary.copy(alpha = 0.15f) else surface)
            .border(
                width = if (isSelected) 2.dp else 1.dp,
                color = if (isSelected) primary else Color.Transparent,
                shape = RoundedCornerShape(12.dp),
            )
            .clickable(onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        // Color swatch
        Box(
            modifier = Modifier
                .size(40.dp)
                .clip(RoundedCornerShape(8.dp))
                .background(lutSwatchColor(preset)),
        )
        Text(
            text = preset.displayName,
            style = MaterialTheme.typography.labelSmall,
            color = if (isSelected) primary else MaterialTheme.colorScheme.onSurfaceVariant,
            modifier = Modifier
                .align(Alignment.BottomCenter)
                .padding(bottom = 6.dp),
            fontSize = 9.sp,
            fontWeight = if (isSelected) FontWeight.Bold else FontWeight.Normal,
        )
    }
}

private fun lutSwatchColor(preset: LutPreset): Color = when (preset) {
    LutPreset.NONE -> Color(0xFF64748B)
    LutPreset.DLOG_TO_REC709 -> Color(0xFF78A85C)
    LutPreset.SUPER8_DAYLIGHT -> Color(0xFFE1B45E)
    LutPreset.SUPER8_DAYLIGHT_SHARP -> Color(0xFFE68A30)
    LutPreset.SUPER8_NIGHT -> Color(0xFF27416F)
    LutPreset.WARM -> Color(0xFFE8825A)
    LutPreset.COOL -> Color(0xFF5A8AE8)
    LutPreset.CINEMATIC -> Color(0xFF6B5A7A)
    LutPreset.ORANGE_TEAL -> Color(0xFF4AADA0)
    LutPreset.FADED -> Color(0xFFA09080)
}

@Composable
private fun PlaceholderPanel(label: String, modifier: Modifier = Modifier) {
    Box(modifier = modifier, contentAlignment = Alignment.Center) {
        Text(label, style = MaterialTheme.typography.bodyMedium, color = MaterialTheme.colorScheme.onSurfaceVariant)
    }
}
