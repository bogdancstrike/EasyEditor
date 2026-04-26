package com.videoeditor.app.ui

import androidx.compose.animation.*
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.videoeditor.app.engine.NativeBridge
import com.videoeditor.app.media.LutPreset
import com.videoeditor.app.media.ExportSettings
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
    var selectedClipIndex by rememberSaveable { mutableStateOf<Int?>(null) }
    var isPlaying by remember { mutableStateOf(false) }
    var currentTimeMs by remember { mutableStateOf(0L) }
    var selectedLut by rememberSaveable { mutableStateOf(LutPreset.NONE) }

    var isExporting by remember { mutableStateOf(false) }
    var isExportSettingsOpen by remember { mutableStateOf(false) }
    var exportProgress by remember { mutableStateOf(0f) }
    var exportMessage by remember { mutableStateOf<String?>(null) }

    fun runExport(settings: ExportSettings) {
        scope.launch {
            isExportSettingsOpen = false
            isExporting = true
            exportProgress = 0f
            val result = exportProject(context, settings) { progress ->
                exportProgress = progress
            }
            isExporting = false
            exportMessage = if (result.success) "Saved to Movies/VxEditor" else (result.error ?: "Unknown error")
        }
    }

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

    if (isExportSettingsOpen) {
        ExportSettingsSheet(
            onDismiss = { isExportSettingsOpen = false },
            onExport = ::runExport,
        )
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(MaterialTheme.colorScheme.background),
    ) {
        UtilityShelf(
            darkTheme = darkTheme,
            onToggleTheme = onToggleTheme,
            onUndo = {
                isPlaying = false
                if (NativeBridge.undoTimelineEdit()) {
                    selectedClipIndex = null
                    currentTimeMs = currentTimeMs.coerceIn(0L, NativeBridge.getProjectDurationMs().coerceAtLeast(1L))
                }
            },
            onRedo = {
                isPlaying = false
                if (NativeBridge.redoTimelineEdit()) {
                    selectedClipIndex = null
                    currentTimeMs = currentTimeMs.coerceIn(0L, NativeBridge.getProjectDurationMs().coerceAtLeast(1L))
                }
            },
            onExport = { isExportSettingsOpen = true },
        )

        Box(
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f),
        ) {
            PreviewStage(
                modifier = Modifier.fillMaxSize(),
                isPlaying = isPlaying,
                currentTimeMs = currentTimeMs,
                lutPreset = selectedLut,
                onPlayingChange = { isPlaying = it },
                onSeek = { timeMs ->
                    isPlaying = false
                    currentTimeMs = timeMs.coerceIn(0L, NativeBridge.getProjectDurationMs().coerceAtLeast(1L))
                },
                onTimeUpdate = { currentTimeMs = it },
            )

            FloatingToolDock(
                selectedTool = selectedTool,
                onToolSelected = { tab -> selectedTool = if (selectedTool == tab) null else tab },
                modifier = Modifier
                    .align(Alignment.CenterStart)
                    .padding(start = 12.dp),
            )

            androidx.compose.animation.AnimatedVisibility(
                visible = selectedTool != null,
                enter = slideInHorizontally(initialOffsetX = { -it }) + fadeIn(),
                exit = slideOutHorizontally(targetOffsetX = { -it }) + fadeOut(),
                modifier = Modifier
                    .align(Alignment.CenterStart)
                    .padding(start = 84.dp, top = 16.dp, bottom = 16.dp),
            ) {
                ToolPanel(
                    selectedTab = selectedTool ?: ToolTab.MEDIA,
                    selectedLut = selectedLut,
                    onLutSelected = { selectedLut = it },
                    selectedClipIndex = selectedClipIndex,
                    onClipLutSelected = { lut ->
                        selectedClipIndex?.let { NativeBridge.setTimelineClipLut(it, lut.ordinal) }
                    },
                    onClose = { selectedTool = null },
                    modifier = Modifier
                        .widthIn(min = 288.dp, max = 340.dp)
                        .fillMaxHeight(),
                )
            }
        }

        Timeline(
            modifier = Modifier
                .fillMaxWidth()
                .height(136.dp),
            currentTimeMs = currentTimeMs,
            selectedClipIndex = selectedClipIndex,
            onSeek = { timeMs ->
                isPlaying = false
                currentTimeMs = timeMs.coerceIn(0L, NativeBridge.getProjectDurationMs().coerceAtLeast(1L))
            },
            onClipSelected = { index ->
                selectedClipIndex = index
                selectedTool = ToolTab.LOOK
            },
            onClipRemove = { index ->
                isPlaying = false
                NativeBridge.removeTimelineClip(index)
                selectedClipIndex = null
                currentTimeMs = currentTimeMs.coerceIn(0L, NativeBridge.getProjectDurationMs().coerceAtLeast(1L))
            },
            onClipMoveLeft = { index ->
                isPlaying = false
                NativeBridge.moveTimelineClip(index, index - 1)
                selectedClipIndex = (index - 1).coerceAtLeast(0)
            },
            onClipMoveRight = { index ->
                isPlaying = false
                NativeBridge.moveTimelineClip(index, index + 1)
                selectedClipIndex = (index + 1).coerceAtMost(NativeBridge.getTimelineClips().lastIndex)
            },
        )
    }
}
