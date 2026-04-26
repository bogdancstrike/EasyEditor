# Phase 1 — MVP Editor UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a modern, intuitive "Immersive Studio" mock UI for the video editor on Android, ready to be wired to real C++ engine functionalities.

**Architecture:** A Jetpack Compose-based UI following the Studio Layout (ADR-0010). It uses a custom Material3 theme for OLED-friendly colors and vibrant accents. The UI is decomposed into modular components: `MediaPool`, `PreviewStage`, `Timeline`, and `UtilityShelf`.

**Tech Stack:** Jetpack Compose, Material3, Kotlin.

---

### Task 1: Design System & Theming

**Files:**
- Create: `android/app/src/main/kotlin/com/videoeditor/app/ui/theme/Color.kt`
- Create: `android/app/src/main/kotlin/com/videoeditor/app/ui/theme/Theme.kt`
- Modify: `android/app/src/main/kotlin/com/videoeditor/app/MainActivity.kt`

- [ ] **Step 1: Define Color Tokens**
Create `Color.kt` with OLED background and neon accents.
```kotlin
package com.videoeditor.app.ui.theme

import androidx.compose.ui.graphics.Color

val DarkBackground = Color(0xFF0F172A)
val SurfaceColor = Color(0xFF1E293B)
val BorderColor = Color(0x14FFFFFF)
val NeonPink = Color(0xFFEC4899)
val ElectricBlue = Color(0xFF2563EB)
val TextPrimary = Color(0xFFFFFFFF)
val TextSecondary = Color(0xFF94A3B8)
```

- [ ] **Step 2: Create Custom Theme**
Implement `VideoEditorTheme`.
```kotlin
package com.videoeditor.app.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable

private val DarkColorScheme = darkColorScheme(
    primary = NeonPink,
    secondary = ElectricBlue,
    background = DarkBackground,
    surface = SurfaceColor,
    onBackground = TextPrimary,
    onSurface = TextPrimary,
    outline = BorderColor
)

@Composable
fun VideoEditorTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = DarkColorScheme,
        content = content
    )
}
```

- [ ] **Step 3: Update MainActivity to use the new theme**
```kotlin
package com.videoeditor.app

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import com.videoeditor.app.ui.theme.VideoEditorTheme
import com.videoeditor.app.ui.EditorScreen

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            VideoEditorTheme {
                EditorScreen()
            }
        }
    }
}
```

- [ ] **Step 4: Commit**
```bash
git add android/app/src/main/kotlin/com/videoeditor/app/ui/theme/*.kt
git add android/app/src/main/kotlin/com/videoeditor/app/MainActivity.kt
git commit -m "ui: implement custom OLED theme and neon accents"
```

---

### Task 2: Main Layout Scaffold (Studio Layout)

**Files:**
- Create: `android/app/src/main/kotlin/com/videoeditor/app/ui/EditorScreen.kt`

- [ ] **Step 1: Implement basic Studio Layout**
```kotlin
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
        UtilityShelf(modifier = Modifier.fillMaxWidth().height(48.dp))

        // Center Stage (Media + Preview)
        Row(modifier = Modifier.fillMaxWidth().weight(1f)) {
            MediaPool(modifier = Modifier.width(160.dp).fillMaxHeight())
            PreviewStage(modifier = Modifier.weight(1f).fillMaxHeight())
        }

        // Command Center (Timeline)
        Timeline(modifier = Modifier.fillMaxWidth().height(240.dp))
    }
}
```

- [ ] **Step 2: Add placeholder components**
Add no-op implementations of `UtilityShelf`, `MediaPool`, `PreviewStage`, and `Timeline` in the same file for now.

- [ ] **Step 3: Commit**
```bash
git add android/app/src/main/kotlin/com/videoeditor/app/ui/EditorScreen.kt
git commit -m "ui: implement main studio layout scaffold"
```

---

### Task 3: Immersive Media Pool

**Files:**
- Create: `android/app/src/main/kotlin/com/videoeditor/app/ui/MediaPool.kt`

- [ ] **Step 1: Implement Media Pool with modern grid**
```kotlin
package com.videoeditor.app.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

@Composable
fun MediaPool(modifier: Modifier = Modifier) {
    Column(
        modifier = modifier
            .background(MaterialTheme.colorScheme.surface)
            .border(1.dp, MaterialTheme.colorScheme.outline)
            .padding(8.dp)
    ) {
        Text(
            text = "MEDIA",
            style = MaterialTheme.typography.labelSmall,
            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.6f),
            modifier = Modifier.padding(bottom = 8.dp)
        )
        LazyVerticalGrid(
            columns = GridCells.Fixed(1),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            items(5) { index ->
                Box(
                    modifier = Modifier
                        .aspectRatio(16/9f)
                        .clip(RoundedCornerShape(8.dp))
                        .background(Color.DarkGray)
                        .border(1.dp, Color.White.copy(alpha = 0.1f), RoundedCornerShape(8.dp))
                )
            }
        }
    }
}
```

- [ ] **Step 2: Commit**
```bash
git add android/app/src/main/kotlin/com/videoeditor/app/ui/MediaPool.kt
git commit -m "ui: implement immersive media pool mock"
```

---

### Task 4: Preview Stage with Moving Playhead Controls

**Files:**
- Create: `android/app/src/main/kotlin/com/videoeditor/app/ui/PreviewStage.kt`
- Modify: `android/app/src/main/kotlin/com/videoeditor/app/ui/EditorScreen.kt`

- [ ] **Step 1: Implement PreviewStage with playback UI**
```kotlin
package com.videoeditor.app.ui

import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

@Composable
fun PreviewStage(modifier: Modifier = Modifier) {
    Box(modifier = modifier.padding(16.dp)) {
        // Video Preview Placeholder (Will use MediaCodecPreview later)
        MediaCodecPreview(modifier = Modifier.fillMaxSize())

        // Playback Controls Overlay
        Box(
            modifier = Modifier
                .align(Alignment.BottomCenter)
                .padding(bottom = 16.dp)
        ) {
            IconButton(
                onClick = { /* Play/Pause Mock */ },
                colors = IconButtonDefaults.filledIconButtonColors(
                    containerColor = MaterialTheme.colorScheme.primary
                )
            ) {
                Icon(Icons.Default.PlayArrow, contentDescription = "Play")
            }
        }
    }
}
```

- [ ] **Step 2: Update EditorScreen to import PreviewStage**

- [ ] **Step 3: Commit**
```bash
git add android/app/src/main/kotlin/com/videoeditor/app/ui/PreviewStage.kt
git commit -m "ui: implement preview stage with playback controls"
```

---

### Task 5: Pro Timeline with Modern Clips

**Files:**
- Create: `android/app/src/main/kotlin/com/videoeditor/app/ui/Timeline.kt`

- [ ] **Step 1: Implement Timeline with Ruler and Clips**
```kotlin
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
import androidx.compose.ui.unit.dp
import com.videoeditor.app.ui.theme.ElectricBlue
import com.videoeditor.app.ui.theme.NeonPink

@Composable
fun Timeline(modifier: Modifier = Modifier) {
    Box(
        modifier = modifier
            .background(MaterialTheme.colorScheme.surface)
            .border(1.dp, MaterialTheme.colorScheme.outline)
    ) {
        Column {
            // Time Ruler
            Box(modifier = Modifier.fillMaxWidth().height(24.dp).background(MaterialTheme.colorScheme.background)) {
                Text(
                    "00:00:00",
                    style = MaterialTheme.typography.labelSmall,
                    color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.4f),
                    modifier = Modifier.padding(start = 8.dp)
                )
            }

            // Track Area
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .horizontalScroll(rememberScrollState())
            ) {
                Row(modifier = Modifier.padding(vertical = 16.dp).height(60.dp)) {
                    // Mock Clips
                    TimelineClip(width = 200.dp)
                    TimelineClip(width = 150.dp)
                    TimelineClip(width = 300.dp)
                }
            }
        }

        // Static Moving Playhead (Positioned at 30% for mock)
        Box(
            modifier = Modifier
                .fillMaxHeight()
                .width(2.dp)
                .align(Alignment.CenterStart)
                .offset(x = 100.dp)
                .background(ElectricBlue)
        )
    }
}

@Composable
fun TimelineClip(width: androidx.compose.ui.unit.Dp) {
    Box(
        modifier = Modifier
            .width(width)
            .fillMaxHeight()
            .padding(horizontal = 2.dp)
            .clip(RoundedCornerShape(8.dp))
            .background(NeonPink.copy(alpha = 0.2f))
            .border(1.dp, NeonPink, RoundedCornerShape(8.dp))
    )
}
```

- [ ] **Step 2: Commit**
```bash
git add android/app/src/main/kotlin/com/videoeditor/app/ui/Timeline.kt
git commit -m "ui: implement pro timeline with modern clips"
```

---

### Task 6: Final Polish & Assembly

**Files:**
- Modify: `android/app/src/main/kotlin/com/videoeditor/app/ui/EditorScreen.kt`

- [ ] **Step 1: Implement UtilityShelf**
Add Top Bar with Undo/Redo/Export icons.

- [ ] **Step 2: Clean up EditorScreen**
Remove placeholder components and ensure all sub-components are correctly integrated.

- [ ] **Step 3: Final Build Verification**
Run `./gradlew assembleDebug` to ensure no compilation errors.

- [ ] **Step 4: Commit**
```bash
git add android/app/src/main/kotlin/com/videoeditor/app/ui/*.kt
git commit -m "ui: finalize phase 1 mock editor assembly"
```
