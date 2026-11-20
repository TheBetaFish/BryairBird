# 5. Canonical phased milestone roadmap

**Status:** Canonical project milestone sequence, approved 2026-11-20. None of these milestones are completed merely by writing this document.

This schedule is authoritative for milestone names and scope. `docs/uwp-roadmap.md` is the high-level architecture/target overview; `docs/EXECUTION_PLAN.md` defines task order, CI verification, and approval gates. Historical M0 references in `docs/m0-feasibility-audit.md` describe preliminary research, not a competing active milestone.

## Milestone 0 — Evidence and reproducible fixtures

Purpose: close uncertainty before implementation.

Tasks:
- Record the exact Raven base commit/build configuration and baseline GitHub Actions run.
- Inspect the existing Rust PE parser and its C/C++ wrapper before deciding whether to extend or wrap it.
- Build a component-import inventory tool.
- Create synthetic APPX/MSIX fixtures for identity, dependencies, app declarations, and protocol activation.
- Test malformed manifests, package path traversal, and dependency cycles.
- Define versioned component manifest and package database schemas.
- Identify the exact Calculator package/framework version and Hello Neighbor package format before target-specific assumptions.

Exit criteria:
- The existing Raven CI workflow completes its build, test, and parser checks for the baseline commit; record the exact run and job results.
- Fixtures are reproducible.
- Schemas are versioned and reviewed.
- First harvested DLL dependency report is generated locally without committing binaries.
- No licensing/package-format assumptions are hidden in implementation.

## Milestone 1 — Prefix setup, component harvest, package registration

Purpose: establish a safe Windows component set and Raven package inventory.

Tasks:
1. Implement manifest-driven local Windows-image harvest.
2. Record build, architecture, file versions, hashes, imports, and dependency closure.
3. Stage harvested files in a per-user cache outside Git.
4. Add prefix pinning to a component-set ID.
5. Implement a Raven-owned package deployment database.
6. Implement package registration and immutable install roots for synthetic fixtures.
7. Generate only the registry projection required by component traces.
8. Investigate StateRepository and create a version-specific projection only if schema/invariants are understood.
9. Preserve no-package behavior for ordinary processes.

Exit criteria:
- Fresh prefix selects a validated component set.
- Synthetic package registers and queries through Raven deployment service.
- Component hashes/provenance validate.
- Package records survive restart and update transactionally.
- No Windows binary is committed.
- No Store entitlement or authentication is fabricated.

StateRepository is not the canonical package database; Raven's package store remains authoritative.

## Milestone 2 — WinRT/COM activation and headless packaged lifecycle

Purpose: launch a synthetic packaged app without rendering.

Tasks:
- Add immutable package execution context to child launch and guest process state.
- Implement package identity APIs and output-buffer semantics.
- Add cross-process identity queries through existing process IPC patterns.
- Add package-aware WinRT activation lookup.
- Resolve package graph and framework dependencies.
- Resolve app ID/manifest application entry.
- Implement protocol activation for synthetic URI schemes.
- Extend CoreApplication lifecycle and CoreWindow enough for headless initialization.
- Validate UI-thread apartment and CoreMessaging integration.
- Add harness for harvested DLL loading and unresolved imports/API traces.

Exit criteria:
- Packaged/unpackaged processes coexist without identity leakage.
- Synthetic packaged process reports correct identity.
- Framework dependencies resolve deterministically.
- Package-scoped WinRT test class activates.
- Activation arguments are delivered correctly.
- Process exits without leaked handles/threads.
- Ordinary desktop tests pass.

## Milestone 3 — XAML application rendering: Calculator

Purpose: prove a complete XAML UI path before a game target.

Tasks:
- Load selected Windows XAML component set.
- Implement the first missing Win32/NT/COM/RPC contracts found by traces.
- Establish CoreWindow and UI-thread dispatcher.
- Complete DComp/DXGI behavior required by the selected XAML version.
- Present through Raven composition renderer and Wayland host surface.
- Validate keyboard, pointer, focus, popup, resize, DPI, and lifecycle.
- Add image and interaction regression tests.

Exit criteria:
- Calculator launches repeatedly from a registered package.
- Main UI renders without a host-side Windows VM.
- Buttons and keyboard input work.
- Resize and close/reopen work.
- No identity, activation, or graphics resource leaks.
- Stable screenshot baseline is regression-tested.
- Limitations are documented precisely.

Calculator is a UI framework validation target, not proof of universal UWP compatibility.

## Milestone 4 — Hello Neighbor package and game compatibility

Purpose: validate the actual Store build, not assumptions from another release channel.

Preconditions:
- Inspect package through a legitimate user entitlement and supported export path.
- Identify whether it is APPX/MSIX, MSIXVC, or another PC game package.
- Confirm manifest, architecture, dependencies, executable, graphics API, and activation model.
- Confirm whether online services are required for local/offline launch.

Tasks:
- Implement only the deployment path required by the actual package.
- Resolve required framework and app-model dependencies.
- Validate actual graphics API and feature requirements.
- Validate DXVK/vkd3d-proton and composition/presentation.
- Test controller and keyboard/mouse input.
- Test local save persistence and filesystem paths.
- Add online identity only through the actual supported authentication flow.
- Verify cloud saves only with real account authorization and service behavior.

Exit criteria:
- Actual Store package installs through supported package path.
- Game launches without a Windows VM.
- Rendering, input, audio, and local saves work reliably.
- Required authentication uses real identity/tokens; no fabricated entitlement.
- Online/cloud behavior is claimed only after direct tests.
- Performance and incompatibilities are measured/documented.

Do not treat Hello Neighbor as a confirmed UWP workload until its Store package and activation model are verified. The Xbox Store listing alone does not prove package format.

## Milestone 5 — Hardening and compatibility expansion

Tasks:
- Expand package formats/bundles based on real needs.
- Improve StateRepository compatibility only where traces justify it.
- Add package update, rollback, and removal transactions.
- Add suspend/resume and background activation where required.
- Expand XAML, DComp, and DXGI coverage.
- Test multiple Windows component builds and architectures.
- Add crash diagnostics, performance telemetry, and sandbox regressions.
- Document app-specific compatibility and known limitations.

Exit criteria:
- Every supported feature has regression fixtures.
- Component updates are reversible.
- Package graph and process identity remain consistent across updates.
- Security boundaries are tested.
- Compatibility claims list apps, versions, component sets, host GPU, and driver configurations.

## Cross-milestone engineering rules

- Every implementation change names its source contract and regression test.
- Do not add speculative APIs solely because a harvested DLL imports them; confirm ABI and semantics.
- Keep Microsoft binaries, Windows package payloads, credentials, and tokens out of Git.
- Keep package identity immutable per process.
- Keep Raven's canonical package database separate from StateRepository projections.
- Preserve unpackaged Win32 behavior.
- Fail closed for unsupported signatures, encryption, capabilities, and licensing.
- Reuse Raven's native loader, process model, registry, WinRT helpers, composition engine, and presentation backend.
- Do not copy Wine code without license and architecture review.

## Current status snapshot

| Area | Status |
|---|---|
| Raven source audit | Targeted source audit complete; full repo not exhaustively reviewed |
| APPX/MSIX reader | Existing partial reader |
| Package deployment | Not implemented; current shim reports empty inventory |
| Package identity | Not implemented; current APIs report no package |
| Package-aware activation | Not implemented |
| Packaged CoreApplication/CoreWindow | Not implemented; current shim is desktop/no-CoreWindow |
| CoreMessaging dispatcher | Existing; XAML compatibility unproven |
| DirectComposition | Existing substantial implementation/tests; XAML compatibility unproven |
| Wayland/Vulkan | Existing infrastructure; UWP integration unproven |
| Windows component harvest | Not implemented |
| StateRepository projection | Not implemented |
| Calculator | Not tested |
| Hello Neighbor Store build | Package format/activation model not verified |

## Reference

[designgears/wine-uwp](https://github.com/designgears/wine-uwp) documents a project-specific sequence of local Windows component harvesting, app-model hosting, DComp/swap-chain work, and selected shims. Raven must establish its own test results.

## CI verification policy

Use the repository's existing [CI workflow](../.github/workflows/ci.yml) as the authoritative automated baseline. It already runs on pushes to `main`, pull requests, and manual `workflow_dispatch`. It includes commit/file/license/script checks, Rust PE-reader tests, sanitizer-backed mutation/fuzz checks, a CMake build, and six CTest shards. Do not create a duplicate baseline workflow unless a documented gap is demonstrated.

A successful CI run validates the repository's configured build/test environment. It does not validate the user's local GPU, graphics driver, Wayland session, harvested Windows components, or Hello Neighbor runtime behavior; those require separate evidence at the relevant milestones.
