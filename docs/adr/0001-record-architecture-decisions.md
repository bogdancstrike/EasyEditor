# 0001 — Record architecture decisions

- **Status:** Accepted
- **Date:** 2026-04-26
- **Tags:** `process`

## Context

We're building a substantial cross-platform application with a shared C++ engine. Many foundational decisions (language, GPU API, color science, timeline data model, FFI shape) will affect work years from now. Without a written record, every old choice becomes a re-argued debate, and new contributors will silently re-do work because they can't tell what's been considered.

## Decision

We will keep ADRs in `docs/adr/` using the format described in [`docs/adr/README.md`](./README.md). Every significant architectural decision gets one. ADRs are append-only — superseded ADRs stay; new ADRs reference what they replace.

## Alternatives considered

- **No formal record (rely on git log + commit messages).** Insufficient. Commits describe code changes, not the reasoning behind a design. The "why" rots.
- **A single `decisions.md` file.** Loses the per-decision granularity and makes superseding messy. Hard to link to one decision.
- **A wiki / Notion / external tool.** Splits docs from code. ADRs need to be in the repo so they version with the code that implements them.

## Consequences

- Every PR introducing a foundational change is expected to include or update an ADR.
- New contributors read `docs/adr/` as part of onboarding.
- We accept the small overhead of writing ~200-word documents.

## References

- Michael Nygard, ["Documenting Architecture Decisions"](https://cognitect.com/blog/2011/11/15/documenting-architecture-decisions)
