# BryairBird project status

**Canonical roadmap:** M0–M5 in `docs/uwp/05-milestone-roadmap.md`
**Current milestone:** M0 — Evidence, PE inspection tooling, and reproducible fixtures
**Current task:** 0.4 — Inspector implementation (CLI parser and deterministic JSON serializer integrated; report model next)
**Branch:** `main`
**Latest verified CI commit:** `d6ab3206e8e7e14c3a63c223196029f19320c749` (upstream Raven PR head; merged as `9769e27053763b28e90508f965d6d9dc41a76d14`)
**Latest CI run:** [45061202169 — success](https://github.com/TheBetaFish/BryairBird/actions/runs/45061202169), attempt 3
**Upstream integration:** [PR #3 merged](https://github.com/TheBetaFish/BryairBird/pull/3); build, all six test shards, Report, and Workflows checks passed on the merged PR head.
**Implementation status:** CLI argument parser, deterministic JSON serializer, CMake targets, and contract tests are integrated. No inspection engine or executable exists yet.

## Decisions in force

- Decision 0001: the M0–M5 schedule in `docs/uwp/05-milestone-roadmap.md` is canonical.
- Decision 0002: use the existing `.github/workflows/ci.yml` as the automated baseline.
- Decision 0003: **approved by user** — reuse Raven's existing Rust PE parser with a separate read-only inventory front end. Do not add a second PE decoder.

## Completed work

- **M0.0 — PASS:** baseline GitHub Actions run [44761707010, attempt 2](https://github.com/TheBetaFish/BryairBird/actions/runs/44761707010); evidence in `docs/evidence/m0.md`.
- **M0.1 — inspection complete:** `docs/m0-inspection-report.md` records parser/loader paths, tests, and reuse assessment.
- **M0.2 — specification complete:** `docs/m0-inspector-spec.md` defines read-only architecture, CLI, schema v1, limits, diagnostics, and security boundary.
- **M0.3 — test plan complete:** `docs/m0-test-plan.md` defines fixtures, malformed-input coverage, determinism, limits, and no-execution assertions.
- **M0.4 — CLI parser integrated:** `native/pe-inspect/cli.*`, CMake target, contract tests, and `docs/schemas/inspector-v1.schema.json` are committed.
- **M0.4 — JSON serializer integrated:** `native/pe-inspect/json.*` provides deterministic compact JSON serialization and contract tests. Schema conformance of full reports remains pending.
- **Upstream Raven integration — PASS:** PR #3 merged at `9769e27053763b28e90508f965d6d9dc41a76d14` after CI run [45061202169](https://github.com/TheBetaFish/BryairBird/actions/runs/45061202169) passed on head `d6ab3206e8e7e14c3a63c223196029f19320c749`. Build, all six CTest shards, and Report succeeded; Workflows checks also passed.
- **EFS design baseline:** `docs/efs-compatibility-design.md` documents Windows EFS semantics, security invariants, host filesystem questions, and staged research. No EFS implementation is claimed.

## Important parser findings

- Existing Rust PE parser is bounds-checked and covers PE32/PE32+ headers, sections, imports, delay imports, exports, resources, and IL-only flag detection.
- Current machine support is x86/x64 plus a special PE32+ x86 IL-only case. ARM64 and unknown-machine header reporting need extension.
- Generic resource traversal exists, but RT_VERSION/StringFileInfo decoding does not.
- Existing mutation tests generate synthetic inputs and run 300,000 deterministic mutations.
- The existing parser is the authoritative decoding core; a second PE parser would duplicate security-sensitive logic.

## Target and scope notes

- Hello Neighbor is paused because the current account cannot access the intended Store target. Do not infer its package format.
- Candy Crush is a candidate alternate target. Exact edition, package format, architecture, and activation model must be verified before target-specific work.
- APPX/MSIX support remains a long-term requirement. Existing package reading is partial; bundle and full deployment support are not complete.
- EFS is a separate filesystem/security track. Its research does not reorder M0–M5 or interrupt the current inspector task.

## Approval gates

- Decision 0003 is approved for the parser-reuse architecture; it does not authorize unrelated scope expansion.
- M0.3 edge-case contracts are resolved in `docs/m0-inspector-spec.md` section 12.1 and `docs/m0-test-plan.md` section 13.
- EFS implementation is not approved by the design note; source audit and security review must precede implementation.

## Next

Implement the versioned report model and schema-aligned serialization, with focused unit tests. Then connect the existing Rust parser through `native/pe/pe.h` without adding a second decoder. Keep file I/O, hashing, resource decoding, and PE parsing in separate layers.

## Known unverified scope

- No working PE inspection executable or end-to-end report generation yet.
- No local GPU/Wayland validation or Windows component harvesting.
- No actual Candy Crush package inspection or launch validation.
- No Hello Neighbor Store package inspection or launch validation.
- No EFS source audit or implementation.
- No complete APPX/MSIX bundle/deployment implementation.
- A green CI run does not establish UWP, game, or EFS runtime compatibility.
