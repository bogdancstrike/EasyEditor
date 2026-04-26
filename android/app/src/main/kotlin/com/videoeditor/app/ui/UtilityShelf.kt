package com.videoeditor.app.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.automirrored.filled.ArrowForward
import androidx.compose.material.icons.filled.Share
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

@Composable
fun UtilityShelf(modifier: Modifier = Modifier) {
    Row(
        modifier = modifier
            .background(MaterialTheme.colorScheme.background)
            .padding(horizontal = 8.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            "VxEditor",
            style = MaterialTheme.typography.titleSmall,
            color = MaterialTheme.colorScheme.primary,
            modifier = Modifier.padding(horizontal = 12.dp)
        )
        
        Spacer(modifier = Modifier.weight(1f))
        
        IconButton(onClick = { /* Undo */ }) {
            Icon(
                Icons.AutoMirrored.Filled.ArrowBack, 
                contentDescription = "Undo",
                tint = Color.White.copy(alpha = 0.6f)
            )
        }
        IconButton(onClick = { /* Redo */ }) {
            Icon(
                Icons.AutoMirrored.Filled.ArrowForward, 
                contentDescription = "Redo",
                tint = Color.White.copy(alpha = 0.6f)
            )
        }
        
        Spacer(modifier = Modifier.width(16.dp))
        
        IconButton(
            onClick = { /* Export */ },
            modifier = Modifier.background(MaterialTheme.colorScheme.secondary.copy(alpha = 0.1f))
        ) {
            Icon(
                Icons.Default.Share, 
                contentDescription = "Export",
                tint = MaterialTheme.colorScheme.secondary
            )
        }
    }
}
