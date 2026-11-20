# BryairBird project status

**Current milestone:** M0 — Windows Component Inspector  
**Current task:** 0.0 — Baseline and documentation setup (in progress)  
**Repository branch:** `main`  
**Inspected HEAD:** `b82da7887cf7b68af68194f24a2d9d25d77bf7ab`  
**Last completed planning work:** UWP architecture blueprint and index; see commit `7e3cb45b3b99769efc3ff22015cb74a7a18d64da`.  
**Implementation status:** No inspector implementation started.

## Current findings

- Existing UWP architecture documents are present under `docs/uwp/`.
- `docs/STATUS.md` and `docs/EXECUTION_PLAN.md` were absent at the inspected HEAD.
- Raven already has a Rust PE parser at `native/peparse/`, exposed through `native/peparse/peparse.h` and wrapped by `native/pe/pe.cpp`.
- The parser reads headers, sections, imports, delay imports, exports, resources, and the CLR header; its source includes bounds checks and a mutation test.
- The existing parser does not yet establish all proposed inspector requirements. In particular, ARM/ARM64 support and version-resource decoding need explicit inspection and tests.
- The repository's commit policy is `type: summary`, enforced by `.github/scripts/check_commits.py`. The execution-plan example `type(task): summary` conflicts with that policy.

## Open issues and GATEs

1. **M0 definition conflict:** `docs/m0-feasibility-audit.md` and `docs/uwp-roadmap.md` use M0 for target/package feasibility; `docs/uwp/05-milestone-roadmap.md` uses M0 for broad evidence/fixtures; the execution plan uses M0 for the PE inspector. Decide whether these are separate tracks or whether milestone labels must be reconciled.
2. **Milestone numbering conflict:** `docs/uwp-roadmap.md` has M1 as a test fixture, M2 as package identity/activation, M3 as lifecycle, M4 as graphics, M5 as target evaluation, and M6 as distribution. `docs/uwp/05-milestone-roadmap.md` and the execution plan use M1 for prefix/harvesting and M2 for app model/activation.
3. **Inspector scope:** the execution plan proposes a new reporting tool, while Raven already has a Rust PE parser and PE-loader wrapper. M0.1 must decide whether to wrap/extend the existing parser or justify a standalone parser. No parser decision is approved.
4. **Baseline not run:** the available GitHub inspection tools cannot execute CMake, Cargo, or CTest. The current HEAD status endpoint returned no checks, and the recent workflow-runs query returned zero runs. Local environment and working-tree status remain unknown.
5. **Hardware assumption:** do not use the desktop hardware listed in the original handoff as a current fact. Verify the user's actual machine before graphics testing.

## Open decisions

- Decision 0001: PE inspector parser strategy — pending M0.1 source report.
- M0/M1/M2 milestone naming and scope reconciliation — user approval required before M0.1 proceeds.

## Last verification

- Repository tree and selected source/docs inspected at HEAD above.
- No build or test command has been run in this environment.
- See [M0 evidence log](evidence/m0.md) for the baseline handoff.
