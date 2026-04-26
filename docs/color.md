# Color Pipeline

> **Read this before touching anything that produces pixels.** Color management is the single area where amateur and professional editors visibly diverge. We commit to a working space and stay in it. See [`architecture.md` §10](architecture.md#10-color-adjustments--doing-them-right).

---

## Working space

- **Intermediate buffers:** `RGBA16F` (half-float), **linear scene-referred** light.
- **Color encoding for adjustments:** linear. Gamma is decoded on input, re-encoded on output.
- **Maximum value:** unbounded (no clamping until the final encode). This preserves headroom for stacked LUTs and effects.

| Stage | Encoding |
|---|---|
| Source decode | sRGB / Rec.709 / Rec.2020 PQ / HLG (per asset metadata) |
| Decode-to-linear | EOTF applied; result is linear scene-referred |
| Adjustments / LUTs / blends | **all in linear RGBA16F** |
| Final OETF | applied once, just before encode/present |
| Output encode | matches export color space (Rec.709 default; Rec.2020 PQ for HDR) |

---

## Adjustment order (canonical)

This order is fixed. Changing it without an ADR is a regression.

```
sourceEOTF→linear
  → exposure   (multiplicative; stops)
  → white balance (matrix in CIE XYZ or RGB scaling)
  → contrast   (around middle gray pivot 0.18)
  → tone curve (highlights / shadows / whites / blacks via luminance masks)
  → saturation (Rec.709 luma)
  → vibrance   (skin-tone-aware, weaker on saturated colors)
  → LUT stack  (transform LUT first, then look LUTs)
  → linear→outputOETF
```

**Why this order matters.** Doing exposure after a LUT means the LUT's color response is no longer what the colorist intended. Doing saturation in non-linear space hue-shifts highlights. The order is part of the color contract.

---

## LUT semantics

- **Transform LUT** (e.g., D-Log → Rec.709): converts a log encoding to a display-referred representation. **Always first** in the per-clip stack.
- **Look LUT** (e.g., "Iceland Cinematic"): creative grade. Stacks after transform LUTs.
- **Project LUT**: applies once after the entire timeline composite. Use sparingly; intended for final output color matching.

A LUT is sampled with **hardware trilinear** interpolation from a 3D texture. The default LUT size is 33 (Adobe convention); we support 17, 33, 65 — anything else is an import error.

---

## Stacking and the headroom problem

Stacking two display-referred LUTs (each clamped to [0,1]) loses highlight detail. Mitigations:

1. We never clamp intermediates. RGBA16F preserves > 1.0 values produced by exposure or by LUT extrapolation.
2. UI surfaces a soft warning when two `look`-typed LUTs are stacked.
3. Future v2 work: adopt full ACES or scene-linear sRGB-extended workflow with OCIO. See [ADR-0007](adr/0007-color-management.md).

---

## HDR (Phase 4+)

- Capture pipeline: detect HDR metadata on import (HLG, PQ, HDR10).
- Working space stays linear scene-referred; HDR is "just a brighter peak."
- Output: Rec.2020 PQ with embedded mastering display metadata for YouTube 4K HDR; HLG for direct-share workflows.
- **Do not crush HDR sources to SDR by default.** Provide explicit "Tone-map to SDR" with a quality preview.

---

## Tests

Color pipeline is covered by:

- **Unit tests** on EOTF/OETF math (round-trip identity within float epsilon).
- **Pixel comparison** against golden images — SSIM ≥ 0.995. See `docs/architecture.md` §18.
- **Round-trip** export → re-import → re-export, expecting no drift beyond a published delta budget.

Goldens live under `test_assets/golden/` and are regenerated only by an explicit script, never silently.
