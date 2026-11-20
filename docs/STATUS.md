# BryairBird project status

**Canonical roadmap:** M0–M5 in `docs/uwp/05-milestone-roadmap.md`  
**Current milestone:** M0 — Evidence, PE inspection tooling, and reproducible fixtures  
**Current task:** 0.2 — Inspector specification; blocked on approval of Decision 0003  
**Branch:** `main`  
**Latest CI-tested source commit:** `9792be1804e520f0885cf17b248ca56f2bbe187b`  
**Latest documentation commit:** `0e3e047678c2c898f2c4ff5590f9b9c339433165`  
**Implementation status:** No inspector code changes.

## Decisions in force

- Decision 0001: the M0–M5 schedule in `docs/uwp/05-milestone-roadmap.md` is canonical.
- Decision 0002: use the existing `.github/workflows/ci.yml` as the automated baseline.
- Decision 0003: proposed reuse of the existing Rust PE parser with a separate read-only inventory front end; awaiting explicit user approval.

## Completed work

- **M0.0 — PASS:** GitHub Actions baseline passed on run [44761707010, attempt 2](https://github.com/TheBetaFish/BryairBird/actions/runs/44761707010). Build, all six CTest shards, parser/model checks, provisioning, compliance, and final Report succeeded. Evidence: `docs/evidence/m0.md`.
- **M0.1 — inspection complete:** `docs/m0-inspection-report.md` documents parser/loader paths and symbols, test infrastructure, tool conventions, reuse assessment, and concrete gaps. No code changes were made.

## Important parser findings

- Existing Rust PE parser is bounds-checked and already covers PE32/PE32+ headers, sections, imports, delay imports, exports, resources, and IL-only flag detection.
- Current machine support is x86/x64 plus a special PE32+ x86 IL-only case. ARM64 and unknown-machine header reporting are not supported yet.
- Generic resource traversal exists, but RT_VERSION/StringFileInfo decoding does not.
- Existing mutation tests generate synthetic inputs and run 300,000 deterministic mutations.
- The existing parser is the recommended decoding core; a second PE parser would duplicate security-sensitive logic.

## Approval gate

Review `docs/decisions/0003-reuse-pe-parser.md`. It is **proposed**, not approved. Do not implement parser changes or the inspector until the user explicitly approves it.

## Next

After approval, complete M0.2: write the versioned inspector specification, explicitly resolving ARM64/unknown-machine behavior, version-resource parsing, file hashing, directory traversal, output schema, limits, and diagnostic codes. Then write the test plan before implementation.

## Known unverified scope

- No local GPU/Wayland validation.
- No Windows component harvesting.
- No actual Hello Neighbor Store package inspection or launch validation.
- A green CI baseline does not establish UWP or game compatibility.
