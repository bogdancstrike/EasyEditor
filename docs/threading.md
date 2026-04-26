# Threading Model

> **Read this before adding a thread, queue, or mutex.** The engine has a small fixed set of named threads. Cross-thread access is via the queues described here. There is **no** "lock and hope."

---

## Threads

| Thread | Owner | Purpose | Allowed to block? |
|---|---|---|---|
| **UI** | platform (Main / Compose) | Drives gestures, dispatches commands, collects state for rendering. | **No.** Anything > 1 frame budget moves off this thread. |
| **Engine** | core | Owns the project model. Applies/reverts commands. Sole writer to project state. | No. |
| **Decode** | core (one per active source) | Drives the platform decoder. Produces frames into the frame ring buffer. | Yes (on decoder I/O). |
| **Render** | core (one) | Walks the render graph at vsync, dispatches GPU work, presents. | Yes (on GPU fences). |
| **Export** | core (spawned per export job) | Drives the offline render-encode loop. | Yes. |
| **Background** | platform-provided pool | Proxy generation, thumbnail strips, waveform extraction, autosave. | Yes. |

There is intentionally no general-purpose worker pool inside the engine. Background work goes through the platform pool (Kotlin coroutines / Swift task groups) so OS scheduling cooperates with foreground priorities.

---

## Ownership rules

1. **Project state is owned by the Engine thread.** All mutations happen there, via `CommandStack::execute`. The UI thread never mutates the project — it dispatches a command and observes the result.
2. **The render graph is constructed on the Engine thread** for a given snapshot of the project, then handed (by const reference / shared_ptr) to the Render thread for execution.
3. **GPU resources are owned by the Render thread.** Other threads enqueue requests; they never call backend ops directly.
4. **The decode ring buffer is the only shared mutable state between Decode and Render.** Implemented as a single-producer / single-consumer lock-free ring (one per stream).

---

## Queues

| From → To | Queue type | Payload |
|---|---|---|
| UI → Engine | MPSC, bounded (256), drops oldest with telemetry on overflow | `Command` instances |
| Engine → UI | SPSC, lossy: latest-wins | Project snapshots for state observation |
| Decode → Render | SPSC ring, bounded (~60 frames) | `(pts, TextureHandle)` |
| Engine → Render | SPSC, lossy: latest-wins | Render graph snapshot |
| Anywhere → Background | platform pool | `std::function<void()>` task |

Lossy queues are correct here: dropping a stale UI snapshot is preferable to backpressuring the engine.

---

## Audio is the master clock

During playback, **video resyncs to audio, never the reverse.** If the decoder falls behind, the Render thread drops frames (skips presentation for some vsyncs). The audio thread continues uninterrupted.

This is non-negotiable. Reversing it produces audible artifacts that users cannot tolerate but tolerate dropped frames easily.

---

## What you must not do

- **Do not call into the platform UI toolkit from Engine, Decode, Render, or Export.** The FFI exposes callbacks for the few cases that need it.
- **Do not block the UI thread waiting for a render.** The render result is observed via the latest-wins state queue; the UI shows the most recent frame.
- **Do not introduce a new global mutex.** If you think you need one, you almost certainly need either (a) a queue or (b) to move the data into the owning thread.
- **Do not spawn `std::thread` inside the engine.** Thread creation goes through `IThreadFactory` so the platform can name threads, set priorities, and integrate with profilers.

---

## Debugging

- All engine threads are named via `pthread_setname_np` / equivalent — visible in profilers.
- A debug build asserts ownership: `VX_ASSERT_THREAD(Engine)` panics if called off-thread.
- Set `VX_LOG_THREADS=1` to log every cross-thread queue push/pop.
