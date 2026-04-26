# Build System

The repo has two parallel builds that share the C++ engine:

1. **Android build** — Gradle drives CMake via `externalNativeBuild`. Output: `.apk` / `.aab`.
2. **Desktop build** — Plain CMake. Used for unit tests, fuzzers, CI on Linux runners. Output: test executables.

A future iOS build will reuse the same `core/CMakeLists.txt` via Xcode's CMake integration or a generated Xcode project.

---

## Prerequisites

| Tool | Min version | Used for |
|---|---|---|
| CMake | 3.22 | Generator |
| Ninja | 1.10 | Build backend (faster than make) |
| Clang or GCC | clang 17 / gcc 13 | C++20 |
| Java | 17 | Gradle |
| Android SDK | API 28–34 | App build |
| Android NDK | r26+ | Cross-compile core for ABIs |
| `glslangValidator` | recent | Validate shaders at build time |
| `clang-format`, `clang-tidy` | 17+ | Lint |
| `ktlint`, `detekt` | recent | Lint |

Install on Ubuntu/Debian:

```bash
sudo apt install cmake ninja-build clang-17 clang-format-17 clang-tidy-17 \
                 openjdk-17-jdk glslang-tools
```

Android SDK + NDK are easiest via Android Studio. After install, set:

```bash
export ANDROID_HOME=$HOME/Android/Sdk
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/26.x.x
```

---

## Desktop build (tests / CI)

```bash
cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DVX_BUILD_TESTS=ON \
      -DVX_ENABLE_SANITIZERS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

`compile_commands.json` lands at `build/compile_commands.json`. Symlink it to the repo root for clangd:

```bash
ln -sf build/compile_commands.json compile_commands.json
```

---

## Android build

```bash
cd android
./gradlew :app:assembleDebug          # build APK
./gradlew :app:installDebug           # install on connected device
./gradlew :app:bundleRelease          # build AAB for Play upload
./gradlew :app:connectedAndroidTest   # instrumented tests
```

The Android Gradle config invokes CMake via `externalNativeBuild`, building `core/` for the ABIs in `app/build.gradle.kts`. The resulting `libvideoeditor.so` is packaged into the APK.

### ABIs

Default debug builds: `arm64-v8a` only (matches every modern phone, fastest build).
Release builds: `arm64-v8a`, `x86_64`. We do **not** ship 32-bit ABIs (`armeabi-v7a`, `x86`) — Play Store no longer requires them and they cost build time and binary size.

---

## Shaders

GLSL compute shaders live in `core/shaders/glsl/`. At build time:

1. `glslangValidator` validates them (CI fails on warnings).
2. `glslangValidator -V` compiles to SPIR-V → `core/shaders/spirv/*.spv`.
3. For Android, SPIR-V is loaded directly by Vulkan.
4. For future iOS, `spirv-cross --msl` translates to Metal Shading Language.

Always edit the GLSL; never hand-edit MSL or SPIR-V.

---

## CI

GitHub Actions definitions live in `.github/workflows/`. Currently planned:

- `ci.yml` — on every push: format check, core build (Linux), core unit tests, Android assembleDebug.
- `release.yml` — on tag: full device matrix on Firebase Test Lab, sign + upload AAB to Play internal track.

See [`docs/architecture.md` §19](architecture.md#19-cicd-and-release-engineering).

---

## Reproducibility

- Pin every dependency version (`gradle/libs.versions.toml`, `core/third_party/CMakeLists.txt`).
- A clean build from an empty checkout must produce a working APK on Linux + macOS + Windows-WSL with documented prerequisites only.
- If a tool can't be pinned (e.g., system clang version), document the supported range in this file and check it at configure time.
