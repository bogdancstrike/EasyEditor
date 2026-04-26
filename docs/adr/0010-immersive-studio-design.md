# 0010 — Mobile Studio Design Strategy

- **Status:** Accepted
- **Date:** 2026-04-26
- **Tags:** `ui`, `ux`, `design`

## Context

The application targets mobile creators who expect a fast CapCut-style workflow but still need professional concepts: media bins, timeline selection, LUT transforms, frame-accurate scrubbing, and export presets. Earlier Phase 1 screens proved the engine/UI integration, but the interface looked more like a generic desktop editor compressed onto a phone. It reserved too much vertical space for panels, used a pink-heavy palette, and made export a single action instead of a deliberate finalization step.

Reference screenshots supplied on 2026-04-26 show the target direction:

- A dark editor page with the video as the visual anchor, compact top controls, a cyan export button, bottom timeline, and contextual bottom tools.
- A rounded export settings sheet with a quick resolution pill, tabs for Video/GIF, sliders, toggles, and a file-size estimate.
- A white/light home/tools surface with large rounded action tiles, cyan accents, and a prominent "New project" panel.

## Decision

We adopt the **Mobile Studio** design strategy: content-first dark editing, bright utility surfaces, and export as an explicit settings flow.

### Pages And Major Surfaces

1. **Editor Page**
   - Default background is near-black (`#111113`) so preview content owns the screen.
   - Top bar is minimal: close/back, optional project state, `1080P` quick settings pill, and cyan `Export` button.
   - Preview sits centered and large. Portrait clips are framed with a thin cyan selection outline when active.
   - Playback controls are compact and float near the preview/timeline boundary.
   - Timeline sits at the bottom with a fixed center playhead. Users scrub by dragging/tapping the timeline; the cursor moves over clips and preview updates.
   - Contextual editing tools sit in a bottom strip or bottom sheet, not in a permanent desktop-style sidebar.

2. **Media/Tool Drawer**
   - Media, Look, Effects, and Adjust open as rounded floating panels.
   - Imported media appears in a media bin first. Users explicitly add clips to the timeline; imported media is not automatically sequenced.
   - Tool panels use dense icon grids with short labels, modeled after mobile creator tools.

3. **Look Panel**
   - Project-wide LUT selection appears first.
   - If a timeline clip is selected, clip-level LUT override appears beneath the project looks.
   - Built-in looks include technical transforms and creative looks: `DLog to Rec709`, `Daylight`, `Daylight Sharp`, `Night`, plus synthetic Phase 1 looks.

4. **Export Settings Sheet**
   - Pressing `Export` opens a modal bottom sheet rather than exporting immediately.
   - Three modes are available:
     - `Best`: app chooses high-quality defaults from the timeline.
     - `Instagram`: vertical social preset, 1080x1920, 30 fps, H.264.
     - `Custom`: user controls resolution, fps, codec, and bitrate.
   - The sheet shows estimated output size, output destination, and a final export action.
   - Advanced controls use segmented buttons, chips, sliders, and switches. No raw text fields for common values.

5. **Completion Surface**
   - Export success should lead to a concise "Ready to share" state with thumbnail, saved destination, and share actions. Phase 1 may keep a success dialog, but Phase 2 should replace it with this page/sheet.

### Visual Language

- **Primary accent:** cyan (`#22D3EE`) for export, selection outlines, and technical controls.
- **Secondary accent:** pink (`#E83E8C`) for creative looks and destructive attention only when cyan would conflict.
- **Light surfaces:** warm off-white (`#F7F7F4`) with pale gray cards (`#EDEDE7`) and black text.
- **Dark surfaces:** near-black background (`#111113`), dark panels (`#1A1B1F`), and elevated cards (`#24262D`).
- **Shape:** 18dp–28dp rounded sheets/cards for mobile friendliness; small timeline clips remain 8dp–10dp for precision.
- **Typography:** system Material typography with no negative letter spacing. Labels are short and scannable.
- **Motion:** panels slide from context edges; export sheet uses bottom-sheet motion; scrubbing should feel immediate and not animated.

### Interaction Rules

- Importing media never mutates the timeline.
- Timeline selection controls contextual panels.
- Project-level LUT applies to the whole sequence unless a clip has a clip-level override.
- Export always goes through settings confirmation.
- The app must remain usable in both light and dark theme, but editing defaults to the dark canvas.

## Alternatives considered

- **Desktop Mirror:** Mimicking a full desktop layout. Rejected due to poor ergonomics on mobile touch screens.
- **Pure Minimalist:** Extreme reduction of UI. Rejected as it hides precision controls needed for Phase 1–3.
- **All-white creator app:** Friendly for home/tools pages but rejected for the editor because color/video judgment benefits from a dark surround.
- **Floating-only canvas:** Rejected as the default because persistent floating chrome can cover the preview during precision work.

## Consequences

- **Consistency:** Native shells will use shared tokens for dark canvas, light surfaces, cyan export actions, and rounded panels.
- **Complexity:** Export settings, media bins, and contextual panels add state, but they match user expectations for a real editor.
- **Ergonomics:** The editor becomes touch-first: import, choose clips, scrub, apply look, export.
- **Performance:** Rounded/elevated panels are acceptable; expensive blur effects remain optional until profiling proves they are safe.
