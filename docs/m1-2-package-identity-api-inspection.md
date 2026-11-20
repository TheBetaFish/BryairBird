# M1.2 source inspection: package identity API implementation

**Project:** BryairBird  
**Milestone:** M1.2 — trace package identity API behavior  
**Status:** Source inspection complete for the inspected API handlers; implementation and tests pending  
**Review date:** 2026-11-19

## Executive finding

Raven's current package identity behavior is implemented as explicit no-package stubs in `native/nt/system_api.cpp`.

The current-process package identity functions write a zero length when a length pointer is supplied and return decimal 15700, the Windows `APPMODEL_ERROR_NO_PACKAGE` result. The process-query variants do the same for every process handle without examining which process is being queried. `GetPackagesByPackageFamily` reports zero packages.

This confirms that package identity is not merely missing from the APPX reader: the guest-facing Windows API layer deliberately models a desktop-only environment. A packaged-process implementation must replace these unconditional handlers with context-aware behavior.

## Verified handler behavior

In `register_system_api(ApiRegistry &registry, KernelContext &context)`, the inspected code registers:

### Current-process APIs

- `GetCurrentPackageFullName`
- `GetCurrentPackageFamilyName`
- `GetCurrentPackageId`
- `GetCurrentPackagePath`
- `GetCurrentApplicationUserModelId`

Each handler sets the supplied length output to zero when non-null and returns 15700.

### Other-process APIs

- `GetPackageFullName`
- `GetPackageFamilyName`
- `GetApplicationUserModelId`

Each handler sets the length output to zero when non-null and returns 15700. The inspected implementation does not resolve the process handle or inspect a target process's identity.

### Family enumeration

`GetPackagesByPackageFamily` validates required pointers, writes zero package counts, and returns success. This is consistent with Raven's current assumption that no app packages are installed.

## Ownership implications

The inspected `KernelContext` is a process-wide service object for the guest API handlers. It already carries references to:

- `NativeProcess`
- `NativeEnvironment`
- `NativeMemory`
- `NativeObjects`
- `NativeThreads`
- `NativePaths`
- `ModuleSet`
- The entry PE module and launch metadata

This makes the per-process kernel/API context a plausible consumer of an immutable package-context object. It is not yet proven to be the correct owner of deployment records or package graph storage.

The `NativeEnvironment` constructor receives a reference to `NativeProcess` and initializes process/thread environment state. No package metadata is visible in the inspected constructor signature or the inspected `KernelContext` declaration.

## Recommended behavior model

The next implementation should distinguish three cases:

1. **Ordinary process:** no package context; preserve the existing 15700 / zero-length behavior.
2. **Current packaged process:** answer current-process identity queries from that process's immutable package context.
3. **Query of another process:** resolve the target process's own context, not the caller's. If the target is ordinary or no longer queryable, return the appropriate documented Windows-compatible result.

Do not make the API handler infer package identity from the executable path, current directory, registry state, or a global “active package.” Those sources are ambiguous when multiple package versions or processes exist.

## Proposed layering

- **Deployment layer:** owns installed/staged package records and dependency resolution.
- **Launch layer:** selects a package application and constructs an immutable per-launch context.
- **Guest process layer:** owns the context for the lifetime of the guest process.
- **API layer:** reads the current process context or resolves a target process context and formats Windows API outputs.

The API handlers should not parse manifests, scan package directories, or perform dependency resolution during each API call.

## Required test expansion

Keep `testdata/native_loader/package_identity_client.c` as the ordinary-process regression test. Add separate synthetic fixtures for:

- Current package full name and family name for a packaged process.
- Current package path and package ID output structures.
- Current application user model ID.
- A process querying itself through a process handle.
- A packaged caller querying an ordinary process.
- An ordinary caller querying a packaged process.
- Two packaged processes with different identities.
- Null, undersized, and invalid output buffers with Windows-compatible error behavior.
- A process that has exited or whose process handle is invalid.

The current fixture covers only the no-package case. These additional tests have not been implemented or run.

## Remaining source inspection

Before implementing the handlers, inspect:

1. How `NativeProcess` and `KernelContext` are constructed in the main execution path.
2. How `ProcessObject` relates to the guest process hosted by a child Raven runtime.
3. Whether the parent can query child package metadata without introducing a new unsafe host-global lookup.
4. The precise Windows output-buffer and error semantics for each supported API.

The last point requires checking the Windows API contracts and should be recorded per function rather than assuming every API shares one signature.

## Status

M1.2 confirms the package identity API gap and the current hardcoded no-package behavior. It identifies the guest API context as a likely consumer of package identity, but does not yet choose the authoritative owner for cross-process identity queries.

No source code was changed. No build or CTest run was performed as part of this inspection.
