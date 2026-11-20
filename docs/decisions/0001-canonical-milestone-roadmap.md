# Decision 0001 — Canonical milestone roadmap

**Date:** 2026-11-20  
**Status:** approved by user  
**Decision owner:** Project user  
**Scope:** Project milestone naming and ordering

## Context

The project had two competing milestone schedules:

- `docs/uwp-roadmap.md` used M0 for target/package inventory, M1 for a test fixture, M2 for package identity/activation, M3 for lifecycle, M4 for graphics, M5 for target evaluation, and M6 for distribution/security.
- `docs/uwp/05-milestone-roadmap.md` used M0 for evidence/fixtures, M1 for prefix/harvesting/package registration, M2 for activation/headless lifecycle, M3 for Calculator, M4 for Hello Neighbor, and M5 for hardening.
- The execution plan separately described M0 as the PE inspector and M1 onward as prefix/app-model implementation.

This made milestone labels ambiguous and could cause tasks to be considered complete under different definitions.

## Options considered

1. Keep the older high-level M0–M6 schedule.
2. Keep the newer implementation-oriented M0–M5 schedule and reconcile all other documents to it.
3. Create separate tracks with independent milestone numbering.

## Decision

Choose option 2. The canonical project sequence is:

| Milestone | Scope |
|---|---|
| M0 | Evidence, PE inspection tooling, reproducible fixtures |
| M1 | Prefix setup, component harvesting, package registration |
| M2 | Package identity, WinRT activation, headless packaged lifecycle |
| M3 | Calculator / XAML rendering validation |
| M4 | Actual Hello Neighbor Store package and game compatibility |
| M5 | Hardening, security review, compatibility expansion |

`docs/uwp/05-milestone-roadmap.md` is the canonical milestone schedule. `docs/EXECUTION_PLAN.md` defines task-level order and gates. `docs/uwp-roadmap.md` remains the high-level architecture and target overview and must not define a competing schedule.

The old M0 feasibility audit is retained as preliminary research. Its historical milestone label is not an active milestone definition.

## Rationale

The M0–M5 sequence is preferred because it follows implementation dependencies: evidence and fixtures first, then environment/package foundations, process identity and activation, a smaller XAML validation target, the actual game, and finally hardening. It also keeps Calculator ahead of Hello Neighbor, avoiding a commercial-game-first implementation strategy.

## Consequences

- The older M0–M6 schedule is retired.
- References to milestone names in the architecture roadmap and execution plan are aligned.
- Historical audit filenames are retained to avoid breaking links; their status is explicitly clarified.
- Any future milestone changes require a new decision record and synchronized updates.

## Documents changed

- `docs/uwp-roadmap.md`
- `docs/uwp/05-milestone-roadmap.md`
- `docs/EXECUTION_PLAN.md`
- `docs/STATUS.md`
- `docs/README.md`

