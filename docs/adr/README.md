# Architecture Decision Records (ADRs)

> An ADR is a short markdown file capturing **one architectural decision**, the **context** in which it was made, and the **consequences**. ADRs are append-only — when a decision changes, write a new ADR that supersedes the old one. Don't rewrite history.

## Why we keep them

Six months from now, no one will remember why we picked Vulkan over OpenGL ES, or why time is rational and not float, or why we went Android-first. Without ADRs, every old decision becomes a re-argued debate. With ADRs, the answer is one `grep` away.

## When to write one

- Picking a foundational technology (language, framework, library, platform).
- Choosing between two non-obvious designs.
- Reversing or amending a previous decision.
- Anything you'd want to explain to a new senior engineer in their first week.

When in doubt, write one. They're cheap.

## Format

Use `0000-template.md` as a starting point. File names are `NNNN-kebab-case-title.md`, zero-padded, monotonically increasing.

## Index

| # | Title | Status |
|---|---|---|
| [0001](0001-record-architecture-decisions.md) | Record architecture decisions | Accepted |
| [0002](0002-shared-cpp-core-with-native-ui.md) | Shared C++ core with native UI shells | Accepted |
| [0003](0003-android-first-then-ios.md) | Ship Android first, iOS in Phase 5 | Accepted |
| [0004](0004-gpu-backend-android.md) | Vulkan primary, OpenGL ES 3.2 fallback (Android) | Proposed |
| [0005](0005-rational-time.md) | Rational time, never float | Accepted |
| [0006](0006-clean-hexagonal-architecture.md) | Clean / Hexagonal architecture for the core | Accepted |
| [0007](0007-color-management.md) | Linear scene-referred working space; OCIO-ready | Accepted |
| [0008](0008-command-pattern-undo.md) | Command pattern for all state mutation | Accepted |
