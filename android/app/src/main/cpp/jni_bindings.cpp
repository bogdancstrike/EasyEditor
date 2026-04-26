#include <jni.h>
#include <android/native_window_jni.h>

#include <exception>
#include <string>

#include "platform/vulkan/vulkan_backend.h"
#include "vx/public_api.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_videoeditor_app_engine_NativeBridge_nativeVersion(JNIEnv* env, jobject /* thiz */) {
    return env->NewStringUTF(vx_version());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_videoeditor_app_engine_NativeBridge_nativeVulkanSmokeTest(JNIEnv* env, jobject /* thiz */) {
    try {
        return env->NewStringUTF(vx::runVulkanSmokeTest().c_str());
    } catch (const std::exception& e) {
        const std::string message = std::string{"Vulkan unavailable: "} + e.what();
        return env->NewStringUTF(message.c_str());
    } catch (...) {
        return env->NewStringUTF("Vulkan unavailable: unknown exception");
    }
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_videoeditor_app_engine_NativeBridge_nativeCreateProject(JNIEnv* env, jobject /* thiz */, jstring name) {
    if (!name) {
        return 0;
    }
    const char* c_name = env->GetStringUTFChars(name, nullptr);
    if (!c_name) {
        return 0;
    }
    VxProject* project = vx_project_create(c_name);
    env->ReleaseStringUTFChars(name, c_name);
    return reinterpret_cast<jlong>(project);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_videoeditor_app_engine_NativeBridge_nativeAddAsset(JNIEnv* env, jobject /* thiz */, jlong handle, jstring path, jlong duration_ms, jint width, jint height) {
    if (handle == 0) {
        return static_cast<jint>(VX_ERR_INVALID_ARG);
    }
    if (!path) {
        return static_cast<jint>(VX_ERR_INVALID_ARG);
    }

    VxProject* project = reinterpret_cast<VxProject*>(handle);
    const char* c_path = env->GetStringUTFChars(path, nullptr);
    if (!c_path) {
        return static_cast<jint>(VX_ERR_OOM);
    }

    vx_status_t status = vx_project_add_asset(project, c_path, static_cast<int64_t>(duration_ms), static_cast<int32_t>(width), static_cast<int32_t>(height));
    env->ReleaseStringUTFChars(path, c_path);
    return static_cast<jint>(status);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_videoeditor_app_engine_NativeBridge_nativeRenderFrame(JNIEnv* env, jobject /* thiz */, jlong handle, jobject surface, jlong time_ms) {
    if (handle == 0) {
        return static_cast<jint>(VX_ERR_INVALID_ARG);
    }
    if (!surface) {
        return static_cast<jint>(VX_ERR_INVALID_ARG);
    }

    VxProject* project = reinterpret_cast<VxProject*>(handle);
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        return static_cast<jint>(VX_ERR_INVALID_ARG);
    }

    vx_status_t status = vx_project_render_frame(project, window, static_cast<int64_t>(time_ms));
    ANativeWindow_release(window);
    return static_cast<jint>(status);
}
