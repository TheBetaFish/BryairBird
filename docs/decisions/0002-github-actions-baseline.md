# Decision 0002 — GitHub Actions as the baseline verifier

**Date:** 2026-11-20  
**Status:** approved by user  
**Decision owner:** Project user  
**Scope:** Automated build and test verification

## Context

The execution plan initially asked for a local CMake/Cargo/CTest baseline and a paste-back of command output. The user explicitly requested that baseline verification use GitHub Actions instead.

The repository already has `.github/workflows/ci.yml`, which:
- triggers on pushes to `main`, pull requests, and manual `workflow_dispatch`;
- runs repository policy checks and scripts;
- tests the Rust PE parser and other Rust components;
- performs sanitizer-backed mutation/fuzz checks;
- configures and builds Raven with CMake;
- runs the CTest suite in six shards.

## Options considered

1. Ask the user to run local build/test commands.
2. Add a second workflow dedicated to the baseline.
3. Use the existing CI workflow and record its run as the authoritative repository baseline.

## Decision

Choose option 3. A documentation or implementation commit pushed to `main` triggers the existing CI workflow. Record the workflow run, commit SHA, job conclusions, and relevant failures in `docs/evidence/m0.md`.

Do not ask the user to run local CMake, Cargo, or CTest commands for the repository baseline. Do not add a duplicate workflow unless a specific coverage gap is demonstrated and documented.

## Rationale

The existing workflow already builds and tests the actual repository in its pinned CI environment. Reusing it avoids duplicate build definitions and provides consistent evidence.

## Consequences and limitations

- CI results are the authoritative baseline for the configured repository environment.
- A workflow being configured is not proof that it ran. A completed run must be inspected.
- CI does not validate the user's local GPU, graphics driver, display session, harvested components, Store entitlement, or target-game behavior.
- If GitHub Actions is disabled, unavailable, or produces no run, record the blocker. Do not claim a pass or silently substitute a local run.

## Documents changed

- `docs/EXECUTION_PLAN.md`
- `docs/uwp/05-milestone-roadmap.md`
- `docs/STATUS.md`
- `docs/evidence/m0.md`

