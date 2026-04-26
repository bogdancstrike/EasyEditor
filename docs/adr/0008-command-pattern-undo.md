# 0008 — Command pattern for all state mutation

- **Status:** Accepted
- **Date:** 2026-04-26
- **Tags:** `core`, `domain`, `ux`

## Context

A serious editor has rock-solid undo/redo. Users will trim, drag, color-grade, undo, redo across long sessions. If undo is even slightly inconsistent — a state change that doesn't undo cleanly, an action that bypasses the stack — users lose work and trust the app a little less every time.

The naive approach (snapshot the whole project before each change) doesn't scale: 10-minute sequences with thumbnails and waveform caches are too large to copy on every keystroke.

## Decision

Every state mutation in the engine is a `Command` with `apply(Project&)` and `revert(Project&)` methods. All mutations go through `CommandStack::execute`. There is **no** code path that mutates the project without producing a command — including UI conveniences and "internal cleanups."

```cpp
class Command {
public:
    virtual ~Command() = default;
    virtual void apply(Project&) = 0;
    virtual void revert(Project&) = 0;
    virtual std::string description() const = 0;  // for "Undo Trim" menu
};
```

Commands store the minimum delta needed to revert (e.g., a Trim command stores the previous in/out times, not a full clip copy).

The stack is bounded (default depth 200) and merges adjacent commands of the same type within a short window (e.g., dragging the playhead → one undo, not 60).

## Alternatives considered

- **Snapshot on every change.** Simple but expensive at our data sizes. Rejected.
- **Event sourcing (replay from origin).** Powerful but overkill; complicates project file format. Rejected for v1.
- **Differential GC of immutable trees (à la persistent data structures).** Elegant; significant implementation cost in C++ without a third-party library. Reconsider in v2 if commands become a maintenance burden.
- **No undo (lol).** Disqualifies the app from "professional" status.

## Consequences

- **Positive:** Undo/redo is structurally correct, not best-effort.
- **Positive:** The command log is a debugging gift — replay sessions deterministically.
- **Positive:** Future feature: collaborative editing via command broadcast becomes feasible.
- **Negative:** Every mutation needs a command class. Verbose. Mitigated by a `LambdaCommand<F, G>` helper for trivial cases — but use sparingly; named commands are better for the menu label.
- **Negative:** "Why doesn't this update?" debugging often traces to a code path that bypassed the command stack. Code review catches it; CI grep enforces it (`tools/scripts/check_no_direct_mutation.py`).

## References

- `docs/architecture.md` §14
- Gamma et al., *Design Patterns* — Command
