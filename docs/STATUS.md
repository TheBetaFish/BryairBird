# BryairBird project status

**Canonical roadmap:** M0–M5 in `docs/uwp/05-milestone-roadmap.md`  
**Current milestone:** M0 — Evidence, PE inspection tooling, and reproducible fixtures  
**Current task:** 0.0 — Baseline and documentation setup; GitHub Actions baseline pending  
**Branch:** `main`  
**Last documentation commit:** `f4470096c532e720c5c55aafac5f8712101f5e7b`  
**Implementation status:** No inspector implementation started.

## Decisions now in force

- Decision 0001: the M0–M5 schedule in `docs/uwp/05-milestone-roadmap.md` is canonical. The older M0–M6 schedule is retired.
- Decision 0002: use the existing `.github/workflows/ci.yml` as the automated baseline. Do not ask the user to run baseline CMake/Cargo/CTest commands locally.

## Verified repository facts

- Raven has a Rust PE parser under `native/peparse/`, exposed through `native/peparse/peparse.h` and wrapped by `native/pe/pe.cpp`.
- The CI workflow runs the Rust PE parser tests, sanitizer-backed mutation/fuzz checks, a CMake build, and six CTest shards, in addition to repository checks.
- The workflow is configured for pushes to `main`, pull requests, and `workflow_dispatch`.
- The repository requires commit subjects in `type: summary` format, enforced by `.github/scripts/check_commits.py`.

## Open items

1. **CI baseline:** GitHub Actions must produce a visible run for the current documentation commit. Record the run URL, commit SHA, and every job conclusion in `docs/evidence/m0.md`. No run/pass is claimed yet.
2. **Inspector architecture:** M0.1 must inspect the existing parser API and test harness in detail before deciding whether to extend, wrap, or replace it.
3. **Target package:** the actual Hello Neighbor Store package format and activation model remain unverified. This is a prerequisite for M4, not a reason to block M0.

## Last verification

- Repository source, milestone documents, and CI workflow inspected.
- No local build was run. GitHub Actions is the required baseline mechanism.
- No CI result is recorded until a workflow run is visible and its jobs are complete.
