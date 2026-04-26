package com.videoeditor.app.media

import kotlin.math.pow

enum class LutPreset(val displayName: String) {
    NONE("No Look"),
    WARM("Warm"),
    COOL("Cool"),
    CINEMATIC("Cinematic"),
    ORANGE_TEAL("Orange & Teal"),
    FADED("Faded Film"),
}

private const val LUT_SIZE = 33

/** Generates a 33×33×33 RGB8 LUT (b-major order, as expected by GL_TEXTURE_3D). */
fun LutPreset.generateLutData(): ByteArray? {
    if (this == LutPreset.NONE) return null
    val transform: (Float, Float, Float) -> Triple<Float, Float, Float> = when (this) {
        LutPreset.WARM -> { r, g, b ->
            Triple((r * 1.10f + 0.02f).clamp(), (g * 1.02f).clamp(), (b * 0.82f).clamp())
        }
        LutPreset.COOL -> { r, g, b ->
            Triple((r * 0.85f).clamp(), (g * 0.97f).clamp(), (b * 1.15f + 0.02f).clamp())
        }
        LutPreset.CINEMATIC -> { r, g, b ->
            val lum = 0.299f * r + 0.587f * g + 0.114f * b
            val sat = 0.78f
            val ro = (lum + (r - lum) * sat).clamp()
            val go = (lum + (g - lum) * sat).clamp()
            val bo = (lum + (b - lum) * sat).clamp()
            Triple(sCurve(ro), sCurve(go), sCurve(bo))
        }
        LutPreset.ORANGE_TEAL -> { r, g, b ->
            val lum = 0.299f * r + 0.587f * g + 0.114f * b
            val shadow = (1f - lum).pow(2f)
            val highlight = lum.pow(2f)
            Triple(
                (r + highlight * 0.07f).clamp(),
                g.clamp(),
                (b + shadow * 0.08f - highlight * 0.04f).clamp(),
            )
        }
        LutPreset.FADED -> { r, g, b ->
            Triple(
                (r * 0.9f + 0.08f).clamp(),
                (g * 0.9f + 0.06f).clamp(),
                (b * 0.85f + 0.10f).clamp(),
            )
        }
        else -> { r, g, b -> Triple(r, g, b) }
    }
    return buildLut(transform)
}

private fun buildLut(fn: (Float, Float, Float) -> Triple<Float, Float, Float>): ByteArray {
    val data = ByteArray(LUT_SIZE * LUT_SIZE * LUT_SIZE * 3)
    var idx = 0
    for (bIdx in 0 until LUT_SIZE) {
        for (gIdx in 0 until LUT_SIZE) {
            for (rIdx in 0 until LUT_SIZE) {
                val rf = rIdx.toFloat() / (LUT_SIZE - 1)
                val gf = gIdx.toFloat() / (LUT_SIZE - 1)
                val bf = bIdx.toFloat() / (LUT_SIZE - 1)
                val (ro, go, bo) = fn(rf, gf, bf)
                data[idx++] = (ro * 255f + 0.5f).toInt().coerceIn(0, 255).toByte()
                data[idx++] = (go * 255f + 0.5f).toInt().coerceIn(0, 255).toByte()
                data[idx++] = (bo * 255f + 0.5f).toInt().coerceIn(0, 255).toByte()
            }
        }
    }
    return data
}

private fun Float.clamp() = coerceIn(0f, 1f)

/** Mild S-curve: lift blacks, crush highlights slightly */
private fun sCurve(x: Float): Float {
    return (x - 0.5f) * 1.05f + 0.5f + 0.015f * (4f * (x - 0.5f).pow(3f))
}
