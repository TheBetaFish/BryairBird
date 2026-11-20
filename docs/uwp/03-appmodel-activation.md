# 3. App model, package identity, and activation

**Status:** Proposed architecture. Implementation not started.

## 3.1 Architectural modules

The package/app-model subsystem should be a native C++ service layer in Raven, not a collection of environment variables or registry entries.

Proposed conceptual modules (names are design placeholders, not existing files):
- `PackageStore`: canonical package registration and deployment state.
- `PackageGraphResolver`: framework/resource dependency resolution.
- `ApplicationResolver`: maps app user model IDs and protocol requests to manifest applications.
- `PackageExecutionContext`: immutable process-scoped package identity and graph.
- `ActivationBroker`: creates a process and delivers activation arguments.
- `WinRTActivationRegistry`: resolves runtime classes with package-aware precedence.
- `CoreApplicationHost`: process lifecycle, CoreApplication events, and CoreWindow.
- `ProtocolRegistry`: maps URI schemes to manifest declarations.
- `StateRepositoryProjection`: optional compatibility database generated from canonical package records.

These should extend Raven's process, registry, loader, and WinRT object models, not duplicate them.

## 3.2 Immutable package execution context

Every packaged process receives an immutable context at creation. Illustrative C++ shape:

    struct PackageExecutionContext {
        PackageIdentity identity;
        PackageApplication application;
        std::filesystem::path install_root;
        std::vector<ResolvedPackage> graph;
        std::vector<ResourcePackage> resources;
        std::string component_set_id;
        PackageActivationKind activation_kind;
        std::vector<std::byte> activation_payload;
    };

This is a conceptual type, not an existing declaration or finalized ABI.

The context represents package full/family name, publisher, version, architecture, app ID, executable/entry point, install root, dependency graph, resource selection, component-set version, and activation metadata.

Only validated deployment records may create it. A guest cannot choose identity by editing its environment or passing an arbitrary package-name string.

## 3.3 Process ownership and propagation

Source inspection identified:
- `start_program` in `native/nt/child_api.cpp` constructs `ChildLaunch`.
- `spawn_child` in `native/nt/child_process.cpp` launches another Raven runtime and transports process state.
- `native/main.cpp` reconstructs child launch state before guest execution.
- `native/nt/process_info_api.cpp` already uses typed process-request IPC to query another guest runtime.

Recommended integration:
1. Add an optional package-context descriptor/reference to launch state.
2. Serialize a bounded, versioned descriptor over an inherited descriptor or equivalent trusted channel.
3. Validate it in child startup before guest entry.
4. Attach immutable context to process-owned runtime/kernel state.
5. Make identity APIs read that context.
6. Add a typed process-query operation so cross-process APIs query the target process.
7. Define child inheritance per Windows process semantics; do not assume every child inherits package identity.

Do not put authoritative identity in environment variables or ordinary `InstalledApp` layer records.

## 3.4 Package identity API surface

In `native/nt/system_api.cpp`, current-process package identity APIs currently return `APPMODEL_ERROR_NO_PACKAGE`. Other-process variants do the same without querying the target process. Preserve this behavior for ordinary processes.

Implement incrementally:
- `GetCurrentPackageFullName`
- `GetCurrentPackageFamilyName`
- `GetCurrentPackageId`
- `GetCurrentPackagePath`
- `GetCurrentApplicationUserModelId`
- `GetPackageFullName`
- `GetPackageFamilyName`
- `GetApplicationUserModelId`
- `GetPackagesByPackageFamily`
- relevant package graph APIs after their contracts are mapped.

Buffer-size rules and error codes differ by API. Follow each Windows API contract; do not force every function through one assumed signature.

## 3.5 Deployment and dependency graph

Package reading must be separate from deployment.

Lifecycle:
1. Open package and validate archive structure.
2. Validate block map and cryptographic integrity/signature according to package type.
3. Parse manifest and application declarations.
4. Resolve framework/resource dependencies.
5. Check architecture and target device family.
6. Stage payload into an immutable versioned install directory.
7. Atomically commit canonical deployment metadata.
8. Generate compatibility registry/StateRepository projections.
9. Expose the app to the application resolver.

Removal/update must be transactional and must not invalidate a package graph held by running processes.

The existing APPX reader is a starting point. The deployment component currently reports empty package collections and does not stage, register, update, or remove packages.

## 3.6 WinRT activation

Raven's `native/guest/raven-shell/winrt.c` currently looks up a class in the machine-wide ActivatableClassId registry key, reads `DllPath`, loads the library, and calls `DllGetActivationFactory`.

Extend it in layers:
1. Add package-graph-aware class registration lookup.
2. Define app-local, package, framework-package, and machine registration precedence from Windows behavior and tests.
3. Resolve paths against registered package roots.
4. Preserve machine registration fallback for desktop compatibility.
5. Tie module lifetime to runtime/class objects.
6. Trace missing classes and record the registration source consulted.

Not every WinRT class is provided by a DLL activation factory; some are supplied by Raven shims, COM servers, or system services.

## 3.7 CoreApplication and CoreWindow

The current `raven-twinapi/twinapi.c` source explicitly models CoreApplication as a desktop process with no package identity and no CoreWindow. This is a direct app-model gap.

CoreApplicationHost should:
- establish package-aware application lifetime;
- create and own a guest-visible CoreWindow abstraction;
- deliver activation events and arguments in the correct order;
- integrate dispatcher and event loop;
- coordinate visibility, suspend/resume, and exit;
- connect CoreWindow input/size/DPI events to Raven's window model;
- initialize the required UI-thread COM apartment and dispatcher.

CoreWindow is a guest Windows API object, not a Wayland surface. Raven translates its behavior into the existing window/presentation abstractions.

Start with the smallest CoreWindow path needed by a simple XAML app. Defer background tasks, multi-window, toast, and shell integration until a test app proves they are required.

## 3.8 Protocol activation

Use manifest-declared URI schemes:
- parse protocol declarations;
- index schemes by package/application ID;
- resolve a URI to an app;
- create the process with its package context;
- deliver activation arguments through the appropriate activation interface;
- preserve package identity and graph.

Reject unregistered/ambiguous schemes unless the app explicitly asks the user to choose. Registration must not grant host filesystem or network access.

## 3.9 StateRepository compatibility

Treat StateRepository as a compatibility projection, not Raven's authoritative package database. Raven's package store owns identity, dependencies, deployment state, and install roots. A version-specific generator may emit the minimal tables/rows required by the harvested components.

Before implementation, identify the Windows build/schema, trace actual queries, understand transaction expectations, and test read/write consistency. If that cannot be established, implement required app-model service interfaces natively rather than fabricating an incomplete database.

## 3.10 Store, TokenBroker, and account shims

Shims may emulate service availability or provide a controlled unsupported path. They must never pretend a user is authenticated or a package is licensed.

Rules:
- No account provider: report no provider or a documented unsupported state.
- Real sign-in: use the official authentication protocol and secure token handling.
- Tokens: protected per-user secret storage; never source control or logs.
- Store licensing: reflect actual package entitlement and verification.
- Missing license: fail closed.
- Xbox identity/cloud-save APIs: no fabricated IDs, tickets, or success results.
- Treat offline launch as possible only if the actual package/runtime behavior demonstrates it.

## 3.11 CoreMessaging and threading

`raven-coremessaging/coremessaging.c` implements DispatcherQueue/controller/timer objects using Raven's window-message loop. It is a useful base, but does not prove XAML UI-thread compatibility.

Verify:
- STA/ASTA initialization;
- dispatcher ownership and shutdown;
- reentrant dispatch and nested pumping;
- callback/event-token lifetime;
- threadpool and async completion ordering;
- CoreWindow integration;
- safe shutdown with callbacks or composition frames in flight.

## 3.12 Test plan

Synthetic tests first:
- package identity and dependency graph;
- two packages with distinct identities;
- framework resolution;
- package-scoped activation registration;
- protocol activation;
- CoreApplication event ordering;
- CoreWindow create/show/resize/close;
- dispatcher and async reentrancy;
- ordinary desktop remains no-package;
- packaged process querying another process.

Then load a harvested component and capture the first unresolved contract.

## 3.13 References

- Raven: `native/nt/child_api.cpp`, `child_process.cpp`, `process_info_api.cpp`, `system_api.cpp`, `native/guest/raven-shell/winrt.c`, `raven-winrt/*`, `raven-twinapi/twinapi.c`, `raven-coremessaging/coremessaging.c`, `raven-deployment/deployment.c`.
- [designgears/wine-uwp](https://github.com/designgears/wine-uwp) documents a Wine-specific package host and selected shims. Reproduce required semantics using Raven's own runtime architecture.
