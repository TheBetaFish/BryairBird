# M1.1 source inspection: process launch and package context

**Project:** BryairBird  
**Milestone:** M1.1 — inspect Raven process creation and ownership  
**Status:** Source inspection complete for the native launch path and ordinary-app ownership model; no implementation or runtime test claimed  
**Review date:** 2026-11-19

## Executive finding

Raven has a well-defined process boundary that can carry packaged-app context without creating a parallel process launcher:

- Guest CreateProcess-style APIs enter `start_program` in `native/nt/child_api.cpp`.
- `start_program` resolves the requested executable through `NativePaths`, then builds a `ChildLaunch`.
- `spawn_child` in `native/nt/child_process.cpp` starts another Raven runtime instance and transports launch state through arguments and inherited file descriptors.
- `native/main.cpp` parses the child options, constructs a `NativeLaunch`, and then enters the guest image execution path.
- Existing process objects already support parent/child IPC through a handle channel and a process-request channel.

The launch contract has no package identity or package-graph field today. The current ordinary-app layer database is not a package database. The most defensible design is an explicit, validated package context attached to the guest process at startup, with process-query IPC for other-process identity queries.

**Do not add package identity to `InstalledApp` or infer it from executable paths.** Those are different concepts with different lifetimes and semantics.

## Source files inspected

| File | Verified responsibility |
|---|---|
| `native/nt/child_api.cpp` | Resolves CreateProcess-style executable names, constructs `ChildLaunch`, registers child-process APIs |
| `native/nt/child_process.h` | Defines `ChildLaunch` and `ProcessObject` |
| `native/nt/child_process.cpp` | Starts child Raven runtime, transports launch state, owns child process handles and IPC channel |
| `native/main.cpp` | Parses `--run-process` options and constructs `NativeLaunch` before guest execution |
| `native/nt/process_info_api.cpp` | Implements process-query IPC and process information APIs |
| `native/nt/apps.h` | Defines ordinary `InstalledApp` and `AppLayer` data models |
| `native/nt/apps.cpp` | Tracks installed program ownership, executable identity, filesystem changes, and registry layers |
| `native/guest/raven-shell/winrt.c` | Resolves WinRT activation classes through registry `DllPath` and `DllGetActivationFactory` |
| `native/nt/system_api.cpp` | Currently returns `APPMODEL_ERROR_NO_PACKAGE` for package identity queries (documented in M1.2) |

## 1. Guest-side process creation

In `native/nt/child_api.cpp`, `start_program` resolves the requested image:

- If an explicit application name is supplied, it is tried first.
- Otherwise, the command line's first word or candidate prefixes are considered.
- Qualified paths are resolved directly.
- Unqualified names are searched relative to the caller, current directory, Windows system folders, Windows directory, and configured search folders.
- The selected image must exist as a regular file; non-batch executables are checked for an MZ signature.
- Batch files are routed through the command processor.

It then fills the current `ChildLaunch` fields: image, command line, directory, stdio descriptors, environment, suspended-start state, gate, and inherited handles. Finally it calls `spawn_child(launch, error)`.

**Gap:** This is an executable-path resolver, not a package application resolver. It does not map a package application ID to a manifest application entry, resolve framework dependencies, or establish package identity.

## 2. Existing ChildLaunch contract

`native/nt/child_process.h` defines:

- `image`: host executable path.
- `command_line`: Windows UTF-16 command line.
- `directory`: Windows working directory.
- `standard[3]`: host descriptors for stdin, stdout, stderr.
- `environment`: UTF-16 environment block.
- `suspended` and `gate`: suspended process startup/resume.
- `inherited`: handles passed from the creating guest process.

There is no package identity, package graph, application manifest entry, package root, or resource-package list.

A parent-only package pointer would be insufficient: the child runs in a separate Raven runtime process and must receive its own validated context before guest code starts.

## 3. Host-side child process transport

`spawn_child` starts a second copy of Raven with `--run-process --execution=native`. The inspected code transports:

- environment through an anonymous memory-backed file descriptor;
- inherited handles through a serialized descriptor record;
- suspended-start state through a gate descriptor;
- process/handle IPC through a Unix socket pair;
- diagnostics, stdio, command line, working directory, sandbox options, elevation, and noninteractive state through arguments and descriptors.

The child process object retains its handle channel. No package-context descriptor or package identity option is currently passed.

**Integration point:** A versioned, bounded package-context descriptor could fit the existing descriptor-passing architecture. The descriptor should refer to a trusted registered package record, not let the guest invent an identity by supplying a string.

## 4. Child startup and NativeLaunch

In `native/main.cpp`, `--run-process` options are parsed before the native guest image is executed. The inspected startup path reconstructs command line, directory, inherited handles, environment, and parent channel, then creates a `NativeLaunch`.

This is a suitable place to receive and validate a package-context descriptor. The exact process-wide owner still requires inspection of the `NativeProcess` / `NativeEnvironment` construction and API context lifetime. The package context must be attached before the guest can call package identity APIs.

## 5. Existing process-query IPC

`native/nt/process_info_api.cpp` already provides a useful pattern for querying another process:

- A `ProcessObject` retains a child handle channel.
- The process-query path can use that channel, the parent channel, or a process-request channel to reach the target runtime.
- Requests carry a typed `kind` and bounded payload fields.
- The target runtime answers from its own `KernelContext`, rather than the caller guessing the target's state.

This pattern is relevant to `GetPackageFullName`, `GetPackageFamilyName`, and `GetApplicationUserModelId`: a query of another process must return the target process's package context, not the caller's. A dedicated request kind for package identity is a plausible extension, but the wire format and authorization rules have not been designed or implemented.

## 6. Ordinary application ownership is not package identity

`native/nt/apps.h` and `apps.cpp` implement a separate feature:

- `InstalledApp` records files, folders, registry keys, changed keys, and sometimes an executable path.
- `identity_of(prefix, executable)` first determines which ordinary installed application owns the executable, otherwise creates a path-derived identity for an executable first encountered.
- `AppLayer` redirects writes outside an app's owned files/registry state into an app-specific layer.
- `record_installation` compares prefix filesystem and registry state against an installation snapshot.

These mechanisms answer ownership and isolation questions for ordinary installed programs. They do not represent APPX/MSIX package full names, package family names, publisher identity, application IDs, dependency graphs, resource packages, or package activation metadata.

Do not reuse `InstalledApp.id` as a package family name or package full name. Do not infer package identity from a layer directory.

## 7. WinRT activation remains a separate gap

`native/guest/raven-shell/winrt.c` looks up a class under the machine-level Windows Runtime activation registry, reads its `DllPath`, loads the library, and calls `DllGetActivationFactory`.

This implements a registry-based class activation route. It does not by itself provide:

- package application entry-point selection;
- package graph registration;
- package-scoped class registration;
- framework package resolution;
- process package identity initialization.

Package-aware application activation and WinRT class activation are related but distinct operations.

## Recommended design boundary (proposal only)

1. **Deployment store:** Owns validated package records, install roots, parsed manifests, package type, and deployment state.
2. **Application resolver:** Resolves a package application ID to a manifest application declaration and executable. Keep this separate from ordinary executable search.
3. **Immutable launch context:** Represents one selected package, application entry, resolved dependency graph, resource packages, and provenance.
4. **Child transport:** Extend `ChildLaunch` with an optional package-context descriptor/reference; validate it in the child startup parser before guest entry.
5. **Guest process ownership:** Store the immutable context with the guest process's process-wide state. The API layer reads it; it does not parse manifests or scan deployment folders per call.
6. **Cross-process query:** Add a typed process-request operation so process identity APIs query the target runtime's own context.
7. **Activation and paths:** Make package-aware activation and package path resolution consume the same immutable process context.

This is a proposal, not a selected ABI. The next implementation design must inspect exact `NativeProcess` construction, `NativeEnvironment` lifetime, `KernelContext` construction, and startup ordering.

## Security and correctness requirements

- Only a trusted, validated deployment record can create package context.
- Guest environment variables and command-line strings cannot grant package identity.
- Package roots must be canonicalized and checked against registered install roots.
- Context data must be versioned, bounded, and rejected if malformed, stale, mismatched, or unsupported.
- Ordinary processes without a package context must preserve current no-package behavior.
- A packaged child must follow an explicit Windows-compatible inheritance rule; do not assume all child processes inherit package identity.
- A query of another process must use that process's context, never the caller's.
- Declared package capabilities must not be translated into unrestricted Linux host permissions.
- Do not bypass Store licensing, encryption, signature validation, or package integrity checks.

## Required tests

### Launch and transport
- Ordinary child without package metadata remains unpackaged.
- Synthetic packaged child receives only its explicitly selected package context.
- Context survives suspended launch and resume.
- Malformed, oversized, unsupported-version, stale, and mismatched descriptors fail before guest entry.
- Failed launch and process exit release all context resources.
- Existing environment, stdio, inherited-handle, elevation, sandbox, and command-line behavior remains unchanged.

### Identity and process queries
- Current-process package full name, family name, path, package ID, and application user model ID are correct.
- A process querying itself gets its own identity.
- A packaged process querying an ordinary process gets the ordinary process's no-package result.
- An ordinary process querying a packaged process gets the packaged process's identity.
- Two packaged processes with different identities do not leak identity across process boundaries.
- Invalid handles and exited processes follow documented Windows-compatible error behavior.

### Regression
- Existing `testdata/native_loader/package_identity_client.c` remains the desktop no-package regression fixture.
- Existing ordinary app-layer ownership and discard-layer behavior remain unchanged.
- Existing WinRT registry activation tests continue to pass.

None of these new package-context tests have been implemented or run.

## Next inspection task

Before changing `ChildLaunch`, trace the construction and lifetime of `NativeProcess`, `NativeEnvironment`, and `KernelContext` in the native execution path. Identify the authoritative process-owned slot for immutable package context and confirm when it becomes available relative to API registration and guest entry. Then design the typed process-request operation for cross-process identity queries.

## Verification record

- Source inspection performed against the current `main` branch on 2026-11-19.
- No source code changed.
- No compilation or CTest run performed.
- No Hello Neighbor package was available for inspection.
