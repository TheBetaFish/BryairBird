# BryairBird project status

**Canonical roadmap:** M0–M5 in `docs/uwp/05-milestone-roadmap.md`  
**Current milestone:** M0 — Evidence, PE inspection tooling, and reproducible fixtures  
**Current task:** 0.0 — Documentation reconciliation complete; CI baseline partially verified, Test 4 retry pending  
**Branch:** `main`  
**Latest CI-tested source commit:** `9792be1804e520f0885cf17b248ca56f2bbe187b`  
**Latest evidence commit:** `57bf54db5446a0c9b6007270e5a91e4338ac7c63`  
**Implementation status:** No inspector implementation started.

## Decisions now in force

- Decision 0001: the M0–M5 schedule in `docs/uwp/05-milestone-roadmap.md` is canonical. The older M0–M6 schedule is retired.
- Decision 0002: use the existing `.github/workflows/ci.yml` as the automated baseline. Do not ask the user to run baseline CMake/Cargo/CTest commands locally.

## Verified repository facts

- Raven has a Rust PE parser under `native/peparse/`, exposed through `native/peparse/peparse.h` and wrapped by `native/pe/pe.cpp`.
- CI runs the Rust PE parser tests, sanitizer-backed mutation/fuzz checks, a CMake build, and six CTest shards, in addition to repository checks.
- The workflow is configured for pushes to `main`, pull requests, and `workflow_dispatch`.
- The repository requires commit subjects in `type: summary` format, enforced by `.github/scripts/check_commits.py`.

## Baseline result

- Run [44761707010](https://github.com/TheBetaFish/BryairBird/actions/runs/44761707010), commit `9792be1804e520f0885cf17b248ca56f2bbe187b`, completed with conclusion `cancelled`.
- The full Build job and five of six CTest shards passed. Repository checks, model/parser checks, provisioning, and compliance jobs passed.
- Test 4/6 was cancelled at CTest progress 59/122; visible completed results showed no test failure.
- The Report job failed because its summary step treats the cancelled shard as non-passing. This is a downstream consequence of cancellation, not a separate test assertion failure.
- A rerun of the cancelled Test 4 job was accepted. Its result is pending.
- **Baseline gate: INCOMPLETE.** Do not begin M0.1 until all six shards and the final report complete successfully.

## Open items

1. **Finish baseline:** check the Test 4 rerun and its Report job; update `docs/evidence/m0.md` with the final attempt and exact conclusions.
2. **Inspector architecture:** after the baseline gate, M0.1 must inspect the existing parser API and test harness in detail before deciding whether to extend, wrap, or replace it.
3. **Target package:** the actual Hello Neighbor Store package format and activation model remain unverified. This is a prerequisite for M4, not a reason to block M0.

## Last verification

- Roadmap and execution documents were updated and fetched back from GitHub.
- The baseline run's job conclusions and Test 4 cancellation were checked through GitHub Actions.
- No local build was run; GitHub Actions is the required baseline mechanism.
- Evidence is recorded in `docs/evidence/m0.md`; the baseline is not marked PASS.
