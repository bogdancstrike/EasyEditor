# 0010 — Immersive Studio Design Strategy

- **Status:** Accepted
- **Date:** 2026-04-26
- **Tags:** `ui`, `ux`, `design`

## Context

The application requires a user interface that appeals to both professional editors and mobile content creators. Existing mobile editors either skew too simple (losing precision) or too complex (mimicking desktop UIs poorly on small screens).

We need a design language that feels "Industrial yet Immersive"—combining the reliability and precision of tools like DaVinci Resolve with the fluid, rounded, and vibrant aesthetics of modern mobile platforms.

## Decision

We adopt the **Immersive Studio** design strategy. This strategy is defined by three pillars:

1.  **Industrial Precision (The "Studio" Aspect):**
    *   **Layout:** A "Studio" arrangement (Layout #2 in our proposals) with clear zones for Media, Preview, and Timeline.
    *   **Color Palette:** A deep-dark OLED-optimized background (`#0F172A`) to minimize eye strain and make the video content "pop."
    *   **Typography:** High-readability sans-serif (Inter) for technical clarity.
    *   **Controls:** Moving playhead behavior (Standard Mobile) to align with touch expectations, but with frame-accurate precision controls.

2.  **Immersive Fluidity (The "Modern" Aspect):**
    *   **Visuals:** Subtle rounded corners (8dp–12dp) on all containers and clips to soften the industrial feel.
    *   **Effects:** Use of glassmorphism (background blurs) for floating menus and secondary overlays to maintain depth.
    *   **Accents:** Vibrant neon accents (Pink `#EC4899` for primary actions, Blue `#2563EB` for the playhead/selection) to provide energy and focus.

3.  **Content-First Philosophy:**
    *   UI chrome is minimal and recedes when not in use.
    *   Video preview remains as large as possible while maintaining access to primary tools.

## Alternatives considered

*   **Desktop Mirror:** Mimicking a full desktop layout. Rejected due to poor ergonomics on mobile touch screens.
*   **Pure Minimalist:** Extreme reduction of UI. Rejected as it obscures pro-level precision and features needed for Phase 1–3.
*   **Floating/Immersive Canvas:** Entirely floating UI. Rejected for the main editing interface as it can clutter the video preview during complex edits.

## Consequences

*   **Consistency:** Native shells (Compose and SwiftUI) will use shared color tokens and spacing systems to ensure parity.
*   **Complexity:** Implementing high-quality blurs and vibrant neons on Android requires careful attention to performance, especially on older devices.
*   **Ergonomics:** The Studio layout provides a familiar "home" for users, reducing the learning curve for complex editing tasks.
