# M1.1 source inspection: process launch and package context

**Project:** BryairBird  
**Milestone:** M1.1 — inspect Raven process creation and ownership  
**Status:** Source inspection complete for the launch path; no implementation or runtime test claimed  
**Review date:** 2026-11-19

## Finding

Raven's native process model starts a child by launching another Raven runtime instance. The parent constructs a `ChildLaunch`, and `spawn_child` serializes launch state through command-line options, inherited file descriptors, and an environment block.

This is a credible integration boundary for future package context: the child must receive its package identity and resolved package graph before guest application code begins. However, the existing launch contract has no package-context member or package-context descriptor. Adding package identity only to a parent-side data structure would therefore be insufficient; the child startup path must receive and own the context too.

This inspection narrows the design. It does not yet establish the best ownership location inside `NativeProcess`, `KernelContext`, or the guest API registration layer. Those structures need a more focused follow-up inspection before coding.

## Inspected source

| File | Observed behavior | Relevance |
|---|---|---|
| `native/nt/child_process.h` | Declares `ChildLaunch` with image, command line, directory, standard descriptors, environment, suspended-start state, gate, and inherited handles | Current parent-to-child launch contract; no package context field exists |
| `native/nt/child_process.cpp` | `spawn_child` starts another Raven runtime with `--run-process --execution=native`; passes environment and inherited descriptors; tracks the resulting process object | Actual host process creation and launch-state transport |
| `native/nt/child_api.cpp` | Builds a `ChildLaunch` from CreateProcess-style arguments and calls `spawn_child` | Main guest-facing child creation path inspected |
| `native/main.cpp` | Parses child options including `--command-line`, `--directory`, `--environment-fd`, `--inherit-fd`, `--handle-channel-fd`, and suspended-process state | Child startup currently has no explicit package-context option |
| `native/nt/process_api.cpp` | Implements process-related guest APIs and uses process objects for current/child process operations | Process API layer; not yet proven to own package identity state |
| `native/nt/process_info_api.cpp` | Implements process information and snapshots, including process listings | Not a package deployment or identity store |
| `testdata/native_loader/package_identity_client.c` | Verifies that ordinary prefix programs and their queried process handles return `APPMODEL_ERROR_NO_PACKAGE` | Existing desktop no-package regression fixture |
| `native/guest/raven-shell/winrt.c` | Resolves WinRT activation classes through registry `DllPath`, loads the library, and calls `DllGetActivationFactory` | Existing activation lookup; not a package-aware launch mechanism |

## Launch-state transport today

The inspected `ChildLaunch` structure currently carries:

- Executable image path.
- Guest command line and working directory.
- Standard input/output/error descriptors.
- Environment block.
- Suspended-start flag and resume gate.
- Inherited handles.

The `spawn_child` implementation creates the child runtime command line, passes descriptors into the child, and writes the environment block to a memory-backed file descriptor. The child startup parser reads the corresponding options.

No package identity, package graph, application declaration, or package resource roots are represented in this launch structure or in the inspected startup options.

## Consequences for the M1 design

### 1. Package context must cross the process boundary

A package context created only in the parent runtime cannot satisfy package identity queries made by the guest application in the child. The launch protocol needs a child-readable representation of the validated context.

A descriptor-backed, versioned record is one possible transport mechanism because the current launch protocol already passes descriptors and reads structured data from them. This is a design candidate, not a selected ABI or implementation decision.

### 2. Context must be attached before guest code runs

The package identity must be available before the guest executable can call package identity APIs. Attaching it after the process begins executing creates a race and can expose incorrect no-package results during initialization.

The exact startup hook must be identified by inspecting where Raven constructs its guest process and initializes its API environment.

### 3. Preserve the existing desktop default

The existing package identity fixture is a required regression test. Ordinary child processes launched without package metadata must continue to report no package identity.

Package context should therefore be optional and explicit. Absence of the context must preserve current behavior.

### 4. Do not overload the ordinary app-layer database

The existing `native/nt/apps.cpp` code tracks ordinary installed programs and per-app prefix layers. The source audit did not establish it as a package deployment database. Package deployment records and package graph state should remain conceptually distinct unless a later source inspection finds an appropriate existing abstraction.

### 5. Do not treat WinRT class activation as application activation

The current `RoGetActivationFactory` path loads a registered class library. A package-aware application launch still needs to select the manifest-declared application entry point and initialize the process package context. Those are separate operations.

## Updated implementation investigation order

Before writing code, inspect these specific boundaries:

1. **Guest process construction:** Find the type that owns the guest image, process environment, and API dispatch state. Determine its lifetime relative to the host child process.
2. **Startup initialization:** Find the exact point between parsing child launch options and entering guest application code.
3. **API registration:** Find where package identity APIs are currently provided and how their no-package result is generated.
4. **Process query semantics:** Determine how APIs that query another process obtain its package identity, and how that identity is associated with the process object.
5. **Descriptor protocol conventions:** Reuse existing serialization, validation, and descriptor ownership patterns rather than inventing an unrelated IPC format.

Only after these points are understood should a package-context transport structure be added.

## Acceptance tests to preserve and add

- Existing ordinary process identity fixture remains unchanged and passes.
- Ordinary child process launched without package metadata still reports no package.
- A synthetic packaged child receives only its own supplied package identity.
- A parent process without package context remains un-packaged when it launches a synthetic packaged child.
- A child with malformed or unsupported context fails closed before guest application code runs.
- Process exit and failed launch release all package-context resources.
- Querying another process reports that process's context, not the caller's context.

These tests define intended behavior. They have not yet been implemented or run.

## Current status

M1.1 confirms the host process launch boundary and the need for explicit child-side package-context transport. It does not yet settle the guest-process ownership location or the Windows API implementation path. The next source inspection should trace guest process construction, startup, and API registration in detail.

No source code was changed. No build or CTest run was performed as part of this inspection.
