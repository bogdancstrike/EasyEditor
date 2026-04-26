package com.videoeditor.app.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable

private val AppDarkColorScheme = darkColorScheme(
    primary = NeonPink,
    onPrimary = BrandOnAccent,
    primaryContainer = DeepPink,
    secondary = ElectricBlue,
    onSecondary = BrandOnAccent,
    tertiary = WarmAmber,
    onTertiary = DarkBackground,
    background = DarkBackground,
    onBackground = DarkTextPrimary,
    surface = DarkSurface,
    surfaceVariant = DarkSurfaceVariant,
    onSurface = DarkTextPrimary,
    onSurfaceVariant = DarkTextSecondary,
    outline = DarkBorder,
    error = DestructiveRed,
)

private val AppLightColorScheme = lightColorScheme(
    primary = NeonPink,
    onPrimary = BrandOnAccent,
    primaryContainer = DeepPink,
    secondary = ElectricBlue,
    onSecondary = BrandOnAccent,
    tertiary = WarmAmber,
    onTertiary = LightTextPrimary,
    background = LightBackground,
    onBackground = LightTextPrimary,
    surface = LightSurface,
    surfaceVariant = LightSurfaceVariant,
    onSurface = LightTextPrimary,
    onSurfaceVariant = LightTextSecondary,
    outline = LightBorder,
    error = DestructiveRed,
)

@Composable
fun VideoEditorTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    content: @Composable () -> Unit,
) {
    MaterialTheme(
        colorScheme = if (darkTheme) AppDarkColorScheme else AppLightColorScheme,
        content = content,
    )
}
