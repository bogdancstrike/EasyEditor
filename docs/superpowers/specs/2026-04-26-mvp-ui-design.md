# Design Spec: Phase 1 — MVP Editor UI

- **Status:** Draft
- **Date:** 2026-04-26
- **Topic:** Phase 1 UI/UX Implementation

## 1. Vision & Aesthetic
We are implementing the **Immersive Studio** direction (ADR-0010). 
- **Mood:** Professional, Precise, Immersive.
- **Colors:** OLED Black (`#0F172A`), Neon Pink (`#EC4899`), Electric Blue (`#2563EB`).
- **Shapes:** Rounded corners (12dp), crisp borders (1dp), subtle glows.

## 2. Layout Structure (Studio)
The screen is divided into three main zones:
1.  **Top Shelf (Utilities):** Undo/Redo, Export, Settings.
2.  **Center Stage (Media & Preview):** 
    *   Left side: Media Pool (horizontal scroll or grid).
    *   Right side: Large Video Preview.
3.  **Command Center (Timeline):** 
    *   Bottom half of the screen.
    *   Top edge: Time ruler.
    *   Center: Tracks and clips.
    *   Overlay: Moving playhead.

## 3. Interactive Components
- **The Timeline:**
    *   Single-track focus for Phase 1.
    *   Clips are rounded rectangles with neon-pink accents.
    *   Drag edges to trim (visual feedback: glowing border).
    *   Pinch to zoom (scales the `pixelsPerSecond` logic).
- **The Toolbar:**
    *   Contextual actions: Cut, Split, Delete.
    *   Floating glassmorphism style.

## 4. Animation & Motion
- **Entry:** Subtle staggered reveal of UI components.
- **Interactions:** 200ms spring-based transitions for button presses and panel shifts.
- **Playback:** Smooth frame-by-frame updates (mastered by the core C++ engine).

## 5. Technical Implementation (Android)
- **Framework:** Jetpack Compose.
- **Theming:** Custom Material3 Theme with mapped OLED color tokens.
- **Canvas:** `TextureView` for the video preview (backed by `MediaCodecPlayer`).
- **State Management:** ViewModel driving the UI from core project updates.
