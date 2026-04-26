package com.videoeditor.app.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.videoeditor.app.ui.theme.DarkBackground

@Composable
fun EditorScreen() {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(DarkBackground)
    ) {
        // Top Shelf (Utilities)
        UtilityShelf(
            modifier = Modifier
                .fillMaxWidth()
                .height(56.dp)
        )

        // Center Stage (Media + Preview)
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f)
        ) {
            MediaPool(
                modifier = Modifier
                    .width(180.dp)
                    .fillMaxHeight()
            )
            PreviewStage(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight()
            )
        }

        // Command Center (Timeline)
        Timeline(
            modifier = Modifier
                .fillMaxWidth()
                .height(260.dp)
        )
    }
}
