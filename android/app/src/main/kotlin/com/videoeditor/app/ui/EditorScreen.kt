package com.videoeditor.app.ui

import androidx.compose.animation.*
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.videoeditor.app.media.LutPreset
import com.videoeditor.app.media.exportProject
import kotlinx.coroutines.launch

@Composable
fun EditorScreen(
    darkTheme: Boolean = true,
    onToggleTheme: () -> Unit = {},
) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()

    var selectedTool by rememberSaveable { mutableStateOf<ToolTab?>(null) }
    var isPlaying by remember { mutableStateOf(false) }
    var currentTimeMs by remember { mutableStateOf(0L) }
    var selectedLut by rememberSaveable { mutableStateOf(LutPreset.NONE) }

    var isExporting by remember { mutableStateOf(false) }
    var exportProgress by remember { mutableStateOf(0f) }
    var exportMessage by remember { mutableStateOf<String?>(null) }

    if (isExporting) {
        AlertDialog(
            onDismissRequest = {},
            title = { Text("Exporting…") },
            text = {
                Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                    LinearProgressIndicator(
                        progress = { exportProgress },
                        modifier = Modifier.fillMaxWidth(),
                    )
                    Text(
                        "%.0f%%".format(exportProgress * 100),
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
            },
            confirmButton = {},
            shape = RoundedCornerShape(16.dp),
        )
    }

    exportMessage?.let { msg ->
        AlertDialog(
            onDismissRequest = { exportMessage = null },
            title = { Text(if (msg.startsWith("Saved")) "Export complete" else "Export failed") },
            text = { Text(msg) },
            confirmButton = {
                TextButton(onClick = { exportMessage = null }) { Text("OK") }
            },
            shape = RoundedCornerShape(16.dp),
        )
    }

    Column(modifier = Modifier.fillMaxSize()) {
        UtilityShelf(
            darkTheme = darkTheme,
            onToggleTheme = onToggleTheme,
            onExport = {
                scope.launch {
                    isExporting = true
                    exportProgress = 0f
                    val result = exportProject(context) { progress ->
                        exportProgress = progress
                    }
                    isExporting = false
                    exportMessage = if (result.success) "Saved to Movies/VxEditor" else (result.error ?: "Unknown error")
                }
            },
        )

        Box(modifier = Modifier.fillMaxWidth().weight(1f)) {
            PreviewStage(
                modifier = Modifier.fillMaxSize(),
                isPlaying = isPlaying,
                lutPreset = selectedLut,
                onPlayingChange = { isPlaying = it },
                onTimeUpdate = { currentTimeMs = it },
            )
        }

        Timeline(
            modifier = Modifier.fillMaxWidth().height(100.dp),
            currentTimeMs = currentTimeMs,
            onSeek = { timeMs ->
                currentTimeMs = timeMs
                isPlaying = false
            },
        )

        AnimatedVisibility(
            visible = selectedTool != null,
            enter = slideInVertically(initialOffsetY = { it }) + fadeIn(),
            exit = slideOutVertically(targetOffsetY = { it }) + fadeOut(),
        ) {
            ToolPanel(
                selectedTab = selectedTool ?: ToolTab.MEDIA,
                selectedLut = selectedLut,
                onLutSelected = { selectedLut = it },
                modifier = Modifier.fillMaxWidth().height(220.dp),
            )
        }

        ToolTabBar(
            selectedTool = selectedTool,
            onToolSelected = { tab -> selectedTool = if (selectedTool == tab) null else tab },
        )
    }
}
