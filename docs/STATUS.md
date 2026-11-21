# BryairBird project status

**Canonical roadmap:** M0–M5 in `docs/uwp/05-milestone-roadmap.md`  
**Current milestone:** M0 — Evidence, PE inspection tooling, and reproducible fixtures  
**Current task:** 0.4 — Inspector implementation (step 1: CLI/report skeleton)  
**Branch:** `main`  
**Latest CI-tested source commit:** `9792be1804e520f0885cf17b248ca56f2bbe187b` (baseline; inspector CLI changes pending)  
**Latest documentation commit:** `fa1dd5a87f58653ec1fbd254a86d5350d6691687`  
**Implementation status:** Initial CLI argument parser and contract tests added; no file inspection, hashing, PE decoding, or report generation yet.

## Decisions in force

- Decision 0001: the M0–M5 schedule in `docs/uwp/05-milestone-roadmap.md` is canonical.
- Decision 0002: use the existing `.github/workflows/ci.yml` as the automated baseline.
- Decision 0003: **approved by user** — reuse Raven's existing Rust PE parser with a separate read-only inventory front end. Do not add a second PE decoder.

## Completed work

- **M0.0 — PASS:** GitHub Actions baseline passed on run [44761707010, attempt 2](https://github.com/TheBetaFish/BryairBird/actions/runs/44761707010). Build, all six CTest shards, parser/model checks, provisioning, compliance, and final Report succeeded. Evidence: `docs/evidence/m0.md`.
- **M0.1 — inspection complete:** `docs/m0-inspection-report.md` documents parser/loader paths and symbols, test infrastructure, tool conventions, reuse assessment, and concrete gaps. No code changes were made.
- **M0.2 — specification complete:** `docs/m0-inspector-spec.md` defines the read-only architecture, CLI, JSON schema v1 contract, PE coverage, version-resource behavior, limits, deterministic output, diagnostics, and failure handling.
- **M0.3 — test plan complete:** `docs/m0-test-plan.md` defines generated fixture cases, expected outcomes, malformed-input/mutation coverage, CLI/schema/determinism checks, resource limits, and no-execution assertions. `docs/README.md` and `docs/EXECUTION_PLAN.md` link the plan.
- **M0.4 — initial CLI layer added:** `native/pe-inspect/cli.*` parses the specified single-file and directory command forms, validates options/duplicates/numeric limits, and has unit contracts in `cli_test.cpp`. `docs/schemas/inspector-v1.schema.json` now defines the report shape. No inspection engine or CLI executable exists yet.

## Important parser findings

- Existing Rust PE parser is bounds-checked and already covers PE32/PE32+ headers, sections, imports, delay imports, exports, resources, and IL-only flag detection.
- Current machine support is x86/x64 plus a special PE32+ x86 IL-only case. ARM64 and unknown-machine header reporting need extension.
- Generic resource traversal exists, but RT_VERSION/StringFileInfo decoding does not.
- Existing mutation tests generate synthetic inputs and run 300,000 deterministic mutations.
- The existing parser is the authoritative decoding core; a second PE parser would duplicate security-sensitive logic.

## Approval gates

- Decision 0003 is approved. This authorizes the parser-reuse architecture, not unreviewed scope expansion.
- M0.3 test plan is committed. M0.3 edge-case contracts are resolved in `docs/m0-inspector-spec.md` section 12.1 and `docs/m0-test-plan.md` section 13. M0.4 may proceed in small tested steps under those contracts.

## Next

Verify CI run [44956822826](https://github.com/TheBetaFish/BryairBird/actions/runs/44956822826) for the CLI parser/CMake integration. Fix any build or test failures. After a passing run, add the report model and deterministic JSON serialization; keep file I/O, hashing, and PE decoding separate.

## Known unverified scope

- Documentation-only CI run [44809904558](https://github.com/TheBetaFish/BryairBird/actions/runs/44809904558) passed. The later CLI/CMake run [44956822826](https://github.com/TheBetaFish/BryairBird/actions/runs/44956822826) was pending at last check.
- No local GPU/Wayland validation.
- No Windows component harvesting.
- No actual Hello Neighbor Store package inspection or launch validation.
- A green CI baseline does not establish UWP or game compatibility.
