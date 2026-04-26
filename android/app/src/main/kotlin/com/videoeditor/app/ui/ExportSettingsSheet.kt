package com.videoeditor.app.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.FileUpload
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilterChip
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ModalBottomSheet
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.material3.rememberModalBottomSheetState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.videoeditor.app.media.ExportCodec
import com.videoeditor.app.media.ExportPreset
import com.videoeditor.app.media.ExportResolution
import com.videoeditor.app.media.ExportSettings

@OptIn(ExperimentalMaterial3Api::class, ExperimentalLayoutApi::class)
@Composable
fun ExportSettingsSheet(
    onDismiss: () -> Unit,
    onExport: (ExportSettings) -> Unit,
) {
    val sheetState = rememberModalBottomSheetState(skipPartiallyExpanded = true)
    var preset by remember { mutableStateOf(ExportPreset.BEST) }
    var resolution by remember { mutableStateOf(ExportResolution.Original) }
    var codec by remember { mutableStateOf(ExportCodec.H264) }
    var fps by remember { mutableFloatStateOf(30f) }
    var bitrate by remember { mutableFloatStateOf(12f) }

    fun settings(): ExportSettings = when (preset) {
        ExportPreset.BEST -> ExportSettings.best().copy(
            fps = fps.toInt(),
            codec = codec,
            bitrateMbps = bitrate.toInt(),
        )
        ExportPreset.INSTAGRAM -> ExportSettings.instagram()
            .copy(
                resolution = resolution,
                fps = fps.toInt(),
                codec = codec,
                bitrateMbps = bitrate.toInt(),
            )
        ExportPreset.CUSTOM -> ExportSettings(
            preset = ExportPreset.CUSTOM,
            resolution = resolution,
            fps = fps.toInt(),
            codec = codec,
            bitrateMbps = bitrate.toInt(),
        )
    }

    ModalBottomSheet(
        onDismissRequest = onDismiss,
        sheetState = sheetState,
        containerColor = MaterialTheme.colorScheme.surface,
        shape = RoundedCornerShape(topStart = 28.dp, topEnd = 28.dp),
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 20.dp, vertical = 8.dp),
            verticalArrangement = Arrangement.spacedBy(18.dp),
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    "Export settings",
                    style = MaterialTheme.typography.headlineSmall,
                    fontWeight = FontWeight.Bold,
                    color = MaterialTheme.colorScheme.onSurface,
                )
                Spacer(modifier = Modifier.weight(1f))
                Text(
                    "Movies/VxEditor",
                    style = MaterialTheme.typography.labelMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            }

            FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                ExportPreset.entries.forEach { item ->
                    FilterChip(
                        selected = preset == item,
                        onClick = {
                            preset = item
                            if (item == ExportPreset.INSTAGRAM) {
                                resolution = ExportResolution.Reel1080p
                                fps = 30f
                                codec = ExportCodec.H264
                                bitrate = 8f
                            }
                        },
                        label = { Text(item.label()) },
                    )
                }
            }

            HorizontalDivider(color = MaterialTheme.colorScheme.outline.copy(alpha = 0.5f))

            SettingBlock(title = "Resolution", value = resolution.label) {
                FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    listOf(
                        ExportResolution.Original,
                        ExportResolution.Reel1080p,
                        ExportResolution.Landscape1080p,
                        ExportResolution.Square1080p,
                    ).forEach { item ->
                        FilterChip(
                            selected = resolution == item,
                            onClick = { resolution = item },
                            label = { Text(item.label) },
                        )
                    }
                }
            }

            SettingBlock(title = "Frame rate", value = "${fps.toInt()} fps") {
                Slider(
                    value = fps,
                    onValueChange = { fps = fpsStep(it) },
                    valueRange = 24f..60f,
                    steps = 3,
                )
                Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                    listOf("24", "30", "50", "60").forEach {
                        Text(it, style = MaterialTheme.typography.labelSmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
                    }
                }
            }

            SettingBlock(title = "Codec", value = codec.label) {
                FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    ExportCodec.entries.forEach { item ->
                        FilterChip(
                            selected = codec == item,
                            onClick = { codec = item },
                            label = { Text(item.label) },
                        )
                    }
                }
            }

            SettingBlock(title = "Bitrate", value = "${bitrate.toInt()} Mbps") {
                Slider(
                    value = bitrate,
                    onValueChange = { bitrate = it.toInt().coerceIn(6, 60).toFloat() },
                    valueRange = 6f..60f,
                )
            }

            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(MaterialTheme.colorScheme.surfaceVariant, RoundedCornerShape(18.dp))
                    .padding(14.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(
                    "Estimated file size",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
                Spacer(modifier = Modifier.weight(1f))
                Text(
                    estimateSizeLabel(settings()),
                    style = MaterialTheme.typography.titleSmall,
                    fontWeight = FontWeight.SemiBold,
                    color = MaterialTheme.colorScheme.onSurface,
                )
            }

            Button(
                onClick = { onExport(settings()) },
                modifier = Modifier
                    .fillMaxWidth()
                    .height(54.dp),
                shape = RoundedCornerShape(18.dp),
                colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.primary),
            ) {
                Icon(Icons.Default.FileUpload, contentDescription = null)
                Spacer(modifier = Modifier.width(8.dp))
                Text("Export video", fontWeight = FontWeight.Bold)
            }

            Spacer(modifier = Modifier.height(8.dp))
        }
    }
}

@Composable
private fun SettingBlock(
    title: String,
    value: String,
    content: @Composable () -> Unit,
) {
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(title, style = MaterialTheme.typography.titleSmall, fontWeight = FontWeight.SemiBold)
            Spacer(modifier = Modifier.weight(1f))
            Text(value, style = MaterialTheme.typography.labelMedium, color = MaterialTheme.colorScheme.onSurfaceVariant)
        }
        content()
    }
}

private fun ExportPreset.label(): String = when (this) {
    ExportPreset.BEST -> "Best"
    ExportPreset.INSTAGRAM -> "Instagram"
    ExportPreset.CUSTOM -> "Custom"
}

private fun fpsStep(value: Float): Float {
    val options = listOf(24f, 30f, 50f, 60f)
    return options.minBy { kotlin.math.abs(it - value) }
}

private fun estimateSizeLabel(settings: ExportSettings): String {
    val assumedSeconds = 15
    val mb = settings.bitrateMbps * assumedSeconds / 8
    return "~$mb MB"
}
