package com.videoeditor.app.engine

object NativeBridge {
    init {
        System.loadLibrary("videoeditor")
    }

    external fun nativeVersion(): String
    external fun nativeVulkanSmokeTest(): String
}
