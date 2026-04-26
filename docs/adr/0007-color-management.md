# 0007 — Linear scene-referred working space; OCIO-ready

- **Status:** Accepted
- **Date:** 2026-04-26
- **Tags:** `color`, `core`

## Context

Casual editors apply exposure, contrast, saturation in non-linear sRGB. The result hue-shifts in highlights and looks "muddy" against professional output. Layering creative LUTs on top of clamped 0–1 buffers loses highlight detail.

Professional pipelines work in **linear scene-referred light** with floating-point intermediates, applying the EOTF on input and OETF on output. This is a meaningful differentiator for the product.

We do not yet need full OpenColorIO (OCIO) — but adding HDR, ACES, or wide-gamut content later would require it, and retrofitting OCIO into a non-linear pipeline is painful.

## Decision

- **Working space:** linear scene-referred, RGBA16F intermediates, **no clamping** until final encode.
- **Adjustment order is fixed** (see `docs/color.md`): exposure → WB → contrast → tone → saturation → vibrance → LUT stack.
- **LUT semantics distinguish transform LUTs from look LUTs.** Transform LUTs (e.g., D-Log → Rec.709) sit first; look LUTs stack on top.
- **We architect for OCIO** but ship v1 with a hand-written linear/sRGB/Rec.709 path. The color-pipeline interface is shaped so OCIO can drop in at v2 without API changes upstream.
- **Default output:** Rec.709 SDR. Rec.2020 PQ HDR is a Phase 4 deliverable.

## Alternatives considered

- **Display-referred sRGB (8-bit) end to end.** Simpler, faster, fundamentally limited. Rejected; this is the consumer-app trap we want to avoid.
- **Adopt OCIO immediately.** Adds a build dependency and a learning curve before we have HDR or ACES content. Defer.
- **Custom ACES.** Reinvents OCIO badly. Rejected.

## Consequences

- **Positive:** Color stacking behaves correctly; the LUT-pack workflow we're building the product around actually works.
- **Positive:** When HDR ships, we don't rewrite the pipeline.
- **Negative:** RGBA16F intermediates cost 2× memory bandwidth vs RGBA8. Mitigated by aggressive texture pooling and by dropping to 8-bit only for final preview output.
- **Negative:** Engineers must learn the linear-light discipline. Documented in `docs/color.md` and enforced in code review.

## References

- `docs/color.md`
- `docs/architecture.md` §10
- Darktable's `colorbalancergb` and `tonecurve` modules (open source; reference implementations)
- ACES, OpenColorIO docs
