# FFI: C Core ↔ Native UI

The C++ engine exposes a **C ABI** (`core/include/vx/public_api.h`). Both Kotlin (via JNI) and Swift (via Swift's C interop, future iOS) bind to it.

---

## Why a C ABI

- **Stable and minimal.** Name-mangled C++ symbols change between compilers; C symbols don't.
- **Two consumers, one surface.** Kotlin and Swift bind to the same header.
- **Easy to fuzz and contract-test.** A flat function list is much easier to reason about than a virtual class hierarchy.

The cost is verbosity in the wrapper layer. Worth it.

---

## Conventions

- All public functions are prefixed `vx_`.
- Opaque handle types: `typedef struct VxFoo VxFoo;` — internals never exposed.
- Functions return `int` error codes; output values via out-parameters. The exception is `_create` functions which return the handle directly (`NULL` on failure).
- Strings: `const char*` (UTF-8) for input; `vx_string_t` (with explicit length + freer) for output to avoid implicit copies.
- Memory ownership is documented per-function. Default: caller frees outputs via the matching `_destroy` / `_free` function.
- No callbacks across threads other than the documented "render-frame-ready" callback. Cross-thread events are queued.

---

## Error model

```c
typedef enum {
    VX_OK = 0,
    VX_ERR_INVALID_ARG = 1,
    VX_ERR_IO = 2,
    VX_ERR_DECODE = 3,
    VX_ERR_GPU = 4,
    VX_ERR_OOM = 5,
    VX_ERR_UNSUPPORTED = 6,
    VX_ERR_INTERNAL = 99,
} vx_status_t;

const char* vx_status_message(vx_status_t);  // human-readable
```

Inside the C++ engine, exceptions are used freely. They are caught at the FFI boundary and converted to `vx_status_t`. **A C++ exception must never propagate out of the C ABI.** This is checked by a translation-unit-level guard in `core/src/ffi/exception_guard.h` and audited in CI.

---

## Threading

- **Most calls are synchronous and run on the calling thread.** They must be cheap (no I/O, no GPU work).
- **Heavy calls** (`vx_export_run`, `vx_proxy_generate`) are asynchronous. The caller passes a completion callback; the callback fires on a background thread and must marshal to the UI thread itself (Kotlin: `Handler(Looper.getMainLooper())`; Swift: `DispatchQueue.main.async`).
- The render-frame callback fires on the **Render thread**; consumers must not call back into the engine from inside it. Push to a queue and return.

See `docs/threading.md` for the full thread model.

---

## Adding a function

1. Add the prototype to `core/include/vx/public_api.h` with full doc comment (intent, ownership, threading).
2. Implement under `core/src/ffi/`. Wrap the body in `VX_FFI_TRY { ... } VX_FFI_CATCH;`.
3. Add a Kotlin binding in `android/app/src/main/cpp/jni_bindings.cpp` (JNI signature) and `android/app/src/main/kotlin/com/videoeditor/app/engine/NativeBridge.kt` (`external fun`).
4. Wrap with an idiomatic Kotlin API in `engine/Engine.kt` (suspend functions, Flow for streams, etc.).
5. Write a contract test in `core/tests/ffi/` and an instrumentation test in `androidTest/`.

When iOS lands, step 3 gains a Swift mirror under `ios/`.

---

## Versioning

The FFI is **internal** — both apps build against `HEAD`. No semver promises across the boundary. We can rename, reshape, and break it freely as long as the JNI/Swift wrappers update in the same commit.

The **project file format** is versioned (`schema_version`); the FFI is not.
