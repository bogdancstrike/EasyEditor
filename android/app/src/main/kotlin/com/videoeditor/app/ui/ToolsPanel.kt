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
fun ToolTabBar(
    selectedTool: ToolTab?,
    onToolSelected: (ToolTab) -> Unit,
    modifier: Modifier = Modifier,
) {
    Surface(
        modifier = modifier.fillMaxWidth(),
        color = MaterialTheme.colorScheme.surface,
        shadowElevation = 8.dp,
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .height(56.dp)
                .navigationBarsPadding(),
            horizontalArrangement = Arrangement.SpaceEvenly,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            ToolTab.entries.forEach { tab ->
                ToolTabItem(
                    tab = tab,
                    isSelected = selectedTool == tab,
                    onClick = { onToolSelected(tab) },
                )
            }
        }
    }
}

@Composable
private fun ToolTabItem(tab: ToolTab, isSelected: Boolean, onClick: () -> Unit) {
    val color = if (isSelected) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onSurfaceVariant
    Column(
        modifier = Modifier
            .clip(RoundedCornerShape(12.dp))
            .clickable(onClick = onClick)
            .padding(horizontal = 16.dp, vertical = 8.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(4.dp),
    ) {
        Icon(tab.icon, contentDescription = tab.label, tint = color, modifier = Modifier.size(22.dp))
        Text(tab.label, color = color, fontSize = 10.sp, fontWeight = if (isSelected) FontWeight.SemiBold else FontWeight.Normal)
    }
}

@Composable
fun ToolPanel(
    selectedTab: ToolTab,
    selectedLut: LutPreset = LutPreset.NONE,
    onLutSelected: (LutPreset) -> Unit = {},
    modifier: Modifier = Modifier,
) {
    Surface(
        modifier = modifier.fillMaxWidth(),
        shape = RoundedCornerShape(topStart = 16.dp, topEnd = 16.dp),
        color = MaterialTheme.colorScheme.surface,
        shadowElevation = 12.dp,
        tonalElevation = 2.dp,
    ) {
        Column(modifier = Modifier.fillMaxSize()) {
            Box(
                modifier = Modifier
                    .align(Alignment.CenterHorizontally)
                    .padding(top = 8.dp)
                    .size(width = 32.dp, height = 4.dp)
                    .clip(RoundedCornerShape(2.dp))
                    .background(MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.3f)),
            )
            when (selectedTab) {
                ToolTab.MEDIA -> MediaPool(modifier = Modifier.fillMaxSize().padding(top = 4.dp))
                ToolTab.LOOK -> LutSelectorPanel(
                    selectedLut = selectedLut,
                    onLutSelected = onLutSelected,
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
    modifier: Modifier = Modifier,
) {
    Column(modifier = modifier.padding(horizontal = 16.dp)) {
        Text(
            "Look",
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
