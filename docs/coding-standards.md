# Coding Standards

> Style is enforced by tooling. This file documents the **why** and the parts tooling can't catch.

---

## Tooling

| Language | Formatter | Linter | Runs in CI |
|---|---|---|---|
| C++ | `clang-format` (root `.clang-format`) | `clang-tidy` (root `.clang-tidy`) | Yes; PR fails on diff. |
| Kotlin | `ktlint` | `detekt` | Yes. |
| GLSL/Compute | `clang-format` (Cpp style) | `glslangValidator` | Yes; build-time validation. |
| Markdown | `prettier` (line wrap off in `.prettierrc`) | — | Yes (lint only). |

Run locally before pushing: `tools/scripts/format.sh` and `tools/scripts/lint.sh`.

---

## Layering rules (enforced)

The directory structure encodes the architecture. Cross-layer includes are checked in CI.

```
core/src/
  domain/       — pure value objects, math, project model. NO platform headers, NO services.
  application/  — use-case orchestration. May depend on domain and on platform interfaces.
  platform/     — interfaces only (IGpuBackend, ICodecBackend, IFileSystemBackend, ...).
                  Concrete implementations live under platform/<backend>/ guarded by build flags.
  ffi/          — C ABI surface. Wraps application services. NO domain logic here.
  util/         — leaf utilities (logging shim, error types, IDs).
```

Forbidden include patterns (caught by `tools/scripts/check_layering.py` in CI):

- `domain/**` may not include from `application/`, `platform/`, `ffi/`.
- `application/**` may not include `ffi/` or platform-concrete headers (only platform interfaces).
- `ffi/**` is the only place allowed to use `extern "C"`.

If you find yourself fighting these, that's the signal to write an ADR proposing the change — not to add a `// NOLINT`.

---

## C++ specifics

- **C++20.** Use `std::span`, `std::optional`, `std::expected` (when targeting clang ≥ 17), `std::filesystem`.
- **No exceptions across the FFI boundary.** Inside the engine, exceptions are fine for true error states (out-of-memory, malformed file). The C ABI returns error codes.
- **No raw `new` / `delete`.** Use `std::unique_ptr` / `std::make_unique`. Smart pointers in argument lists encode ownership intent.
- **`const` by default.** Method-on-const, parameters by `const&`, member functions `const` unless they mutate.
- **`[[nodiscard]]`** on factories and on any function returning an error code.
- **No singletons.** Pass dependencies in constructors. If a "global" is unavoidable (e.g., logger), it's behind an interface and can be replaced in tests.
- **Headers are self-contained.** Each `.h` includes everything it needs and forward-declares what it can.
- **Doc comments** on all public symbols (Doxygen `///` style).

---

## Kotlin specifics

- **Coroutines + Flow** for async; never raw `Thread` / `Handler`.
- **Immutable data classes** for state; use `copy()` for updates.
- **No `lateinit var`** outside Android lifecycle binding. Prefer constructor injection.
- **No GlobalScope.** Use `viewModelScope`, `lifecycleScope`, or an injected `CoroutineScope`.
- **JNI calls** are wrapped behind `engine/NativeBridge.kt`; ViewModels never call native methods directly.

---

## Comments and documentation

Defaults:

- Don't explain *what* the code does — names should. Explain *why* when the why is non-obvious.
- Public APIs: doc comment with intent, units, ownership, threading.
- Inside functions: comment only when there's a hidden constraint (e.g., "must run after `prepareTextures` because `format` is set there").
- Never include task IDs or PR numbers in code comments — those rot.

Multi-line essays belong in `docs/`, not in headers.

---

## Tests

- Tests are first-class code. They follow the same style rules.
- File naming: `foo.cpp` ↔ `foo_test.cpp` (C++); `Foo.kt` ↔ `FooTest.kt` (Kotlin).
- One assertion per logical concept; multiple `EXPECT_*` per test only when validating one outcome from multiple angles.
- **No sleeps in tests.** Use fakes/clocks. A flake is a bug, not noise.
- Pixel-comparison tests use SSIM thresholds (see `docs/color.md`).

---

## Git hygiene

- One logical change per commit. "wip", "fixup", "address review" are squashed before merge.
- Commit message subject ≤ 72 chars; body wraps at 100. Imperative mood ("Add LUT registry", not "Added").
- Reference the relevant ADR or `docs/TODO.md` line for non-trivial changes.
- PRs touching `core/` need two reviewers. UI-only PRs need one.
