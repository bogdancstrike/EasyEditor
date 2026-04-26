// vx/public_api.h
// Stable C ABI for the video-editor engine.
//
// THIS IS THE ONLY HEADER Swift/Kotlin BIND TO.
// All public engine functionality crosses this surface. See docs/ffi.md.
//
// Conventions:
//   - All functions are prefixed `vx_`.
//   - Handles are opaque structs.
//   - Functions return vx_status_t; outputs via out-pointers.
//     Exceptions are *_create functions which return the handle (NULL on failure).
//   - Strings in:  const char* (UTF-8, caller-owned).
//     Strings out: vx_string_t — caller frees via vx_string_free.
//   - No C++ exceptions ever cross this boundary. Enforced by exception_guard.h.
//   - Most calls are synchronous and cheap. Long-running work uses async APIs
//     with completion callbacks; see per-function docs and docs/threading.md.

#ifndef VX_PUBLIC_API_H
#define VX_PUBLIC_API_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#define VX_API __declspec(dllexport)
#else
#define VX_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------------
// Status / error codes
// --------------------------------------------------------------------------

typedef enum {
    VX_OK = 0,
    VX_ERR_INVALID_ARG = 1,
    VX_ERR_IO = 2,
    VX_ERR_DECODE = 3,
    VX_ERR_GPU = 4,
    VX_ERR_OOM = 5,
    VX_ERR_UNSUPPORTED = 6,
    VX_ERR_NOT_FOUND = 7,
    VX_ERR_INTERNAL = 99
} vx_status_t;

/// Human-readable description of a status code. Returned pointer is static; do not free.
VX_API const char* vx_status_message(vx_status_t status);

// --------------------------------------------------------------------------
// String type for FFI returns
// --------------------------------------------------------------------------

typedef struct {
    const char* data;   // UTF-8, NOT null-terminated. Use length.
    size_t length;
    void* _opaque;      // backing storage for vx_string_free.
} vx_string_t;

VX_API void vx_string_free(vx_string_t* s);

// --------------------------------------------------------------------------
// Engine lifecycle
// --------------------------------------------------------------------------

/// Initialize the engine. Must be called once at app startup before any other
/// vx_* call. Safe to call multiple times (subsequent calls are no-ops).
VX_API vx_status_t vx_init(void);

/// Shut down the engine. After this, no other vx_* call may be made.
VX_API void vx_shutdown(void);

/// Engine version string ("MAJOR.MINOR.PATCH+gitsha"). Static; do not free.
VX_API const char* vx_version(void);

// --------------------------------------------------------------------------
// Project
// --------------------------------------------------------------------------

typedef struct VxProject VxProject;

/// Create a new empty project with the given display name.
/// Returns NULL on allocation failure.
[[nodiscard]] VX_API VxProject* vx_project_create(const char* name);

/// Destroy a project and release its resources.
VX_API void vx_project_destroy(VxProject* project);

/// Serialize a project to JSON (UTF-8). Caller owns the returned string.
VX_API vx_status_t vx_project_serialize_json(VxProject* project, vx_string_t* out_json);

/// Load a project from a JSON string. On success, *out_project owns memory and
/// must be released with vx_project_destroy.
VX_API vx_status_t vx_project_load_json(const char* json, VxProject** out_project);

/// Add a media asset to the project.
/// path: absolute path or URI (caller owned).
/// duration_ms: duration in milliseconds.
/// width/height: native resolution.
VX_API vx_status_t vx_project_add_asset(VxProject* project, const char* path, int64_t duration_ms, int32_t width, int32_t height);

/// Render a single frame of the project sequence.
/// window: platform native window (caller owned).
/// time_ms: timeline position.
VX_API vx_status_t vx_project_render_frame(VxProject* project, void* window, int64_t time_ms);

// --------------------------------------------------------------------------
// (Phase 1+) Sequence, clip, render, export FFI surfaces will be appended here.
// Do not add new sections without updating docs/ffi.md.
// --------------------------------------------------------------------------

#ifdef __cplusplus
}  // extern "C"
#endif

#undef VX_API

#endif  // VX_PUBLIC_API_H
