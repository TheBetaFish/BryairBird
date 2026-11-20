# BryairBird project status

**Canonical roadmap:** M0–M5 in `docs/uwp/05-milestone-roadmap.md`  
**Current milestone:** M0 — Evidence, PE inspection tooling, and reproducible fixtures  
**Current task:** 0.0 — Documentation reconciliation complete; GitHub Actions baseline awaiting a run  
**Branch:** `main`  
**Latest roadmap decision commit:** `0c9caf48f9ff5a6fc65b06ca3cf510f9b895e683`  
**Implementation status:** No inspector implementation started.

## Decisions now in force

- Decision 0001: the M0–M5 schedule in `docs/uwp/05-milestone-roadmap.md` is canonical. The older M0–M6 schedule is retired.
- Decision 0002: use the existing `.github/workflows/ci.yml` as the automated baseline. Do not ask the user to run baseline CMake/Cargo/CTest commands locally.

## Verified repository facts

- Raven has a Rust PE parser under `native/peparse/`, exposed through `native/peparse/peparse.h` and wrapped by `native/pe/pe.cpp`.
- CI runs the Rust PE parser tests, sanitizer-backed mutation/fuzz checks, a CMake build, and six CTest shards, in addition to repository checks.
- The workflow is configured for pushes to `main`, pull requests, and `workflow_dispatch`.
- The repository requires commit subjects in `type: summary` format, enforced by `.github/scripts/check_commits.py`.

## Open items

1. **CI baseline is blocked pending a visible run:** after pushing `0c9caf48f9ff5a6fc65b06ca3cf510f9b895e683`, the GitHub Actions runs query returned `total_count=0`; the commit status endpoint returned `state=pending`, `total_count=0`. This is not a pass or failure. Actions may not have started, may be disabled, or may not be visible to the connected integration. The existing workflow needs to produce a run before baseline can be marked PASS.
2. **Inspector architecture:** M0.1 must inspect the existing parser API and test harness in detail before deciding whether to extend, wrap, or replace it.
3. **Target package:** the actual Hello Neighbor Store package format and activation model remain unverified. This is a prerequisite for M4, not a reason to block M0.

## Last verification

- Roadmap and execution documents were updated and fetched back from GitHub.
- No local build was run. GitHub Actions is the required baseline mechanism.
- No CI result is recorded until a workflow run is visible and its jobs are complete.
