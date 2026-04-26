# 0009 — Vendor nlohmann::json for project serialization

- **Status:** Accepted
- **Date:** 2026-04-26
- **Tags:** `core`, `serialization`, `json`

## Context

Phase 0 implemented a bare-bones, in-tree JSON parser to prove the round-trip of the project model without introducing third-party dependencies early on. This parser lacks support for arrays and deep object graphs.
As we move to Phase 1 (MVP Editor), the project model will grow to include `media_pool`, `sequences`, `tracks`, and `clips` (all of which are arrays). Expanding the custom parser to handle this safely and robustly would distract from building the video engine. 

## Decision

We will vendor `nlohmann::json` into `core/third_party/json/` (or via Conan/vcpkg if we adopt a package manager later). 

## Alternatives considered

- **Expand custom parser:** Slower to develop, prone to edge-case bugs, distracts from core value.
- **FlatBuffers / Cap'n Proto:** Great for caching, but the project file needs to be human-readable and easily mergable in Git. JSON is preferred for the EDL (Edit Decision List) format.
- **yyjson / simdjson:** Faster, but `nlohmann::json` has a more ergonomic API for C++17/20, which optimizes for developer velocity. The project file parsing is not in the hot path (unlike frame rendering).

## Consequences

- Binary size will increase slightly.
- We get robust, well-tested JSON parsing with excellent C++ integration.
- We can easily map our structs using `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE`.
