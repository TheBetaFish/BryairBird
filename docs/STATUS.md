# BryairBird project status

**Canonical roadmap:** M0–M5 in `docs/uwp/05-milestone-roadmap.md`  
**Current milestone:** M0 — Evidence, PE inspection tooling, and reproducible fixtures  
**Current task:** 0.3 — Inspector test plan  
**Branch:** `main`  
**Latest CI-tested source commit:** `9792be1804e520f0885cf17b248ca56f2bbe187b`  
**Latest documentation commit:** `76eefe572e4cadd9c47dabf81a13f78a9149d619`  
**Implementation status:** No inspector code changes.

## Decisions in force

- Decision 0001: the M0–M5 schedule in `docs/uwp/05-milestone-roadmap.md` is canonical.
- Decision 0002: use the existing `.github/workflows/ci.yml` as the automated baseline.
- Decision 0003: **approved by user** — reuse Raven's existing Rust PE parser with a separate read-only inventory front end. Do not add a second PE decoder.

## Completed work

- **M0.0 — PASS:** GitHub Actions baseline passed on run [44761707010, attempt 2](https://github.com/TheBetaFish/BryairBird/actions/runs/44761707010). Build, all six CTest shards, parser/model checks, provisioning, compliance, and final Report succeeded. Evidence: `docs/evidence/m0.md`.
- **M0.1 — inspection complete:** `docs/m0-inspection-report.md` documents parser/loader paths and symbols, test infrastructure, tool conventions, reuse assessment, and concrete gaps. No code changes were made.
- **M0.2 — specification complete:** `docs/m0-inspector-spec.md` defines the read-only architecture, CLI, JSON schema v1 contract, PE coverage, version-resource behavior, limits, deterministic output, diagnostics, and failure handling.

## Important parser findings

- Existing Rust PE parser is bounds-checked and already covers PE32/PE32+ headers, sections, imports, delay imports, exports, resources, and IL-only flag detection.
- Current machine support is x86/x64 plus a special PE32+ x86 IL-only case. ARM64 and unknown-machine header reporting need extension.
- Generic resource traversal exists, but RT_VERSION/StringFileInfo decoding does not.
- Existing mutation tests generate synthetic inputs and run 300,000 deterministic mutations.
- The existing parser is the authoritative decoding core; a second PE parser would duplicate security-sensitive logic.

## Approval gates

- Decision 0003 is approved. This authorizes the parser-reuse architecture, not unreviewed scope expansion.
- M0.3 must define generated fixtures and expected outcomes before implementation.
- M0.4 implementation remains subject to the specification and test plan.

## Next

Complete M0.3: write the synthetic-fixture test plan, expected outcomes, malformed-input/mutation coverage, deterministic-output checks, and safety assertions. Do not implement the inspector until the test plan is committed.

## Known unverified scope

- Latest documentation CI run has not yet been confirmed.
- No local GPU/Wayland validation.
- No Windows component harvesting.
- No actual Hello Neighbor Store package inspection or launch validation.
- A green CI baseline does not establish UWP or game compatibility.
