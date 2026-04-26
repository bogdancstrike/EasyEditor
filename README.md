# Video Editor App

A professional mobile video editor with LUT-based color grading, multi-track timeline editing, and customizable export. Architected as a shared C++ engine consumed by native UI shells. **Android first**, iOS planned.

## Repository layout

```
.
├── core/              Shared C++ engine (domain, render graph, color, codec orchestration)
├── android/           Android app (Kotlin + Jetpack Compose, JNI to core)
├── ios/               iOS app (placeholder — Phase 5+)
├── docs/              Architecture, ADRs, threading, color, coding standards, TODO
├── test_assets/       Sample LUTs and test media (Git LFS)
└── tools/             Build scripts, shader compilation, dev utilities
```

## Quick start (Android)

Prerequisites: Android Studio Hedgehog+, NDK r26+, CMake 3.22+, Ninja, Java 17.

```bash
cd android
./gradlew :app:assembleDebug
./gradlew :app:installDebug
```

The Gradle build invokes CMake to compile `core/` for the configured ABIs and packages the resulting `.so` files into the APK.

## Documentation

- [docs/architecture.md](docs/architecture.md) — full architecture spec
- [docs/TODO.md](docs/TODO.md) — current milestones and tasks
- [docs/build.md](docs/build.md) — build system details
- [docs/threading.md](docs/threading.md) — threading model
- [docs/color.md](docs/color.md) — color pipeline
- [docs/coding-standards.md](docs/coding-standards.md) — style and conventions
- [docs/adr/](docs/adr/) — Architecture Decision Records

## Architectural principles (enforced)

1. **Engine-first.** All editor logic lives in `core/`. Platform code is "just UI."
2. **Clean / Hexagonal.** Domain has zero framework dependencies. Application services orchestrate. Platform abstractions sit at the edge.
3. **Non-destructive.** The project is an EDL referencing source media. Source files are never mutated.
4. **Color-correct by default.** Linear scene-referred working space, RGBA16F intermediates.
5. **Command pattern everywhere.** Every state mutation goes through the command stack. No bypasses.
6. **Rational time.** Never float. NTSC framerates demand it.
7. **No singletons in the engine.** Pass context explicitly.
8. **ADRs for every significant decision.** Future-you will thank you.

These are not aspirational — they are enforced by directory structure (e.g. `core/src/domain/` has no platform headers in scope), CI lint rules, and code review.
