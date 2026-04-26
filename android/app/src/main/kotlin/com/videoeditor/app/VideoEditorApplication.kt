package com.videoeditor.app

import android.app.Application
import android.content.Context
import io.sentry.android.core.SentryAndroid
import io.sentry.SentryLevel

class VideoEditorApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        
        // Disabled in debug, opt-in in release
        // TODO: Read this from user preferences once SharedPreferences/DataStore is implemented
        val userOptedInToCrashReporting = false 
        
        // Assuming BuildConfig.DEBUG exists, but since we didn't enable buildConfig feature,
        // we can check ApplicationInfo flags, or we can just rely on the boolean logic.
        val isDebuggable = (applicationInfo.flags and android.content.pm.ApplicationInfo.FLAG_DEBUGGABLE) != 0

        if (!isDebuggable && userOptedInToCrashReporting) {
            SentryAndroid.init(this) { options ->
                options.dsn = "https://examplePublicKey@o0.ingest.sentry.io/0" // Placeholder DSN
                options.tracesSampleRate = 1.0 // Adjust later
                // Add more configuration as needed
            }
        }
    }
}
