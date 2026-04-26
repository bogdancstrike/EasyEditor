package com.videoeditor.app.media

import android.content.Context
import com.videoeditor.app.R
import java.util.concurrent.ConcurrentHashMap
import kotlin.math.pow

enum class LutPreset(val displayName: String, val rawCubeResId: Int? = null) {
    NONE("No Look"),
    DLOG_TO_REC709("DLog to Rec709", R.raw.dlog_to_rec709),
    SUPER8_DAYLIGHT("Daylight", R.raw.super8_daylight),
    SUPER8_DAYLIGHT_SHARP("Daylight Sharp", R.raw.super8_daylight_sharp),
    SUPER8_NIGHT("Night", R.raw.super8_night),
    WARM("Warm"),
    COOL("Cool"),
    CINEMATIC("Cinematic"),
    ORANGE_TEAL("Orange & Teal"),
    FADED("Faded Film"),
}

private const val LUT_SIZE = 33
private val LUT_DATA_CACHE = ConcurrentHashMap<LutPreset, LutData>()

data class LutData(val size: Int, val rgb: ByteArray)

/** Generates or loads an RGB8 LUT (b-major order, as expected by GL_TEXTURE_3D). */
fun LutPreset.generateLutData(context: Context? = null): LutData? {
    if (this == LutPreset.NONE) return null
    LUT_DATA_CACHE[this]?.let { return it }
    rawCubeResId?.let { resId ->
        requireNotNull(context) { "Context is required to load cube LUT resources" }
        return LUT_DATA_CACHE.getOrPut(this) {
            context.resources.openRawResource(resId).bufferedReader().use(::parseCube)
        }
    }
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
    return LUT_DATA_CACHE.getOrPut(this) {
        LutData(LUT_SIZE, buildLut(transform))
    }
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

private fun parseCube(reader: java.io.BufferedReader): LutData {
    var size: Int? = null
    val values = ArrayList<Byte>()
    reader.lineSequence().forEach { rawLine ->
        val line = rawLine.trim()
        if (line.isEmpty() || line.startsWith("#")) return@forEach
        val parts = line.split(Regex("\\s+"))
        when (parts.firstOrNull()) {
            "TITLE", "DOMAIN_MIN", "DOMAIN_MAX", "LUT_1D_SIZE" -> return@forEach
            "LUT_3D_SIZE" -> {
                size = parts.getOrNull(1)?.toIntOrNull()
                return@forEach
            }
        }
        if (parts.size >= 3) {
            values += parts[0].toFloat().clamp().toByte255()
            values += parts[1].toFloat().clamp().toByte255()
            values += parts[2].toFloat().clamp().toByte255()
        }
    }

    val lutSize = requireNotNull(size) { "Missing LUT_3D_SIZE in cube file" }
    val expected = lutSize * lutSize * lutSize * 3
    require(values.size == expected) {
        "Invalid cube LUT: expected $expected RGB values for size $lutSize, got ${values.size}"
    }
    return LutData(lutSize, values.toByteArray())
}

private fun Float.toByte255(): Byte = (this * 255f + 0.5f).toInt().coerceIn(0, 255).toByte()

private fun Float.clamp() = coerceIn(0f, 1f)

/** Mild S-curve: lift blacks, crush highlights slightly */
private fun sCurve(x: Float): Float {
    return (x - 0.5f) * 1.05f + 0.5f + 0.015f * (4f * (x - 0.5f).pow(3f))
}
