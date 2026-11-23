# 6. Microsoft UWP / WinRT platform contracts

**Status:** Research synthesis of public Microsoft documentation, checked 2026-11-22. This document records platform contracts relevant to Raven/BryairBird; it is not an implementation claim or a promise that every UWP API is in scope.

## 6.1 Platform status and compatibility target

Microsoft describes UWP as supported but not under active development and recommends the Windows App SDK for new Windows applications. BryairBird's goal is therefore compatibility with existing packaged UWP applications and the WinRT contracts they actually use. Do not treat every current Windows App SDK feature as a UWP requirement, and do not assume desktop-packaged Win32 apps have UWP's AppContainer behavior.

Primary references:
- [What's a UWP app?](https://learn.microsoft.com/en-us/windows/uwp/get-started/universal-application-platform-guide)
- [WinRT API reference](https://learn.microsoft.com/en-us/uwp/)

## 6.2 Package manifest, identity, and dependency graph

The package manifest is the deployment contract consumed by Windows. Its root Package document describes identity, properties, resources, dependencies, capabilities, extensions, applications, and COM extensions. Package dependencies are explicit; framework/resource package relationships and target device families affect whether a package can be deployed and run.

Identity is not just a display name. Microsoft's package identity model includes name, publisher, version, architecture, and resource ID; Package Family Name is derived from name and publisher ID and is commonly used as the stable family-scoped identity. Package data and security are commonly scoped to the package family. Do not synthesize identity from an arbitrary package name or conflate Package Full Name with Package Family Name.

Implementation implications:
- Keep canonical package records and dependency resolution in Raven's package store, not registry strings or environment variables.
- Validate manifest identity, architecture, resource qualifiers, target device family, framework dependencies, and minimum versions before registration.
- Preserve the distinction between package full name, family name, application user model ID, publisher identity, and package graph.
- Missing or incompatible required dependencies must fail deployment/activation explicitly; do not silently omit them.
- Treat manifest parsing, signature/block-map verification, deployment, registration, and activation as separate stages.

References:
- [Package manifest root element](https://learn.microsoft.com/en-us/uwp/schemas/appxpackage/uapmanifestschema/element-package)
- [Package manifest overview](https://learn.microsoft.com/en-us/uwp/schemas/appxpackage/appx-package-manifest)
- [Package dependencies](https://learn.microsoft.com/en-us/uwp/schemas/appxpackage/uapmanifestschema/element-f-dependencies)
- [Package identity overview](https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/package-identity-overview)

## 6.3 AppContainer and capability semantics

UWP apps have package identity and run in an AppContainer. Their declared capabilities govern access to protected resources, devices, and APIs. A package identity by itself does not mean every packaged app runs in AppContainer: packaged desktop apps commonly run at medium integrity/full trust unless configured otherwise.

Capabilities are not a generic allow-all switch. They include general-use, device, privacy-sensitive, restricted, and custom categories. Users can change privacy access at runtime, so a declared privacy-sensitive capability does not guarantee access is currently granted. Some restricted capabilities require Store approval for Store submission, while sideloading behavior differs.

Implementation implications:
- Model package identity, AppContainer token state, integrity, capability SIDs/grants, and host sandbox permissions as related but distinct concepts.
- Map each capability to a narrowly scoped Raven sandbox policy; never translate a capability into unrestricted host access.
- A denied or revoked privacy permission must produce the documented failure path, not silent access.
- Keep ordinary desktop/full-trust behavior distinct from UWP AppContainer behavior.
- Do not grant restricted capabilities merely because they appear in a manifest; record unsupported/denied policy explicitly.

References:
- [UWP app capability declarations](https://learn.microsoft.com/en-us/windows/uwp/packaging/app-capability-declarations)
- [Current capability declaration reference](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/app-capability-declarations)

## 6.4 Activation and application lifecycle

UWP activation is broader than process creation. Applications may be activated by launch, protocol URI, file type, and other manifest extensions. Activation arguments and the activation kind are delivered to the app through the corresponding WinRT activation contract. The app model distinguishes execution states such as NotRunning, Running, Suspended, Terminated, and ClosedByUser.

UWP processes are normally suspended after leaving the foreground. A suspended app may later resume with in-memory state intact, or the OS may terminate it to reclaim memory. Apps must persist important state during suspension because termination does not provide a final notification; on a later launch, the app restores state using the activation information. Resuming also requires refreshing stale data and reacquiring released exclusive resources.

Implementation implications:
- Resolve the manifest's application and extension declarations before creating a packaged process.
- Preserve activation kind and bounded activation payload in trusted process launch state; do not rely on environment variables.
- Deliver activation events in the correct order and keep package identity attached to the process.
- Represent suspension/resume/termination as distinct lifecycle transitions; do not equate window minimize with process termination.
- Use explicit deferrals for asynchronous work in lifecycle handlers and complete them exactly once.
- Keep UWP lifecycle behavior separate from Windows App SDK desktop lifecycle; desktop apps do not receive UWP's automatic Process Lifecycle Management suspension model.

References:
- [Launching, resuming, and background tasks](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/)
- [UWP app lifecycle](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/app-lifecycle)
- [Handle app suspend](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/suspend-an-app)
- [Handle app resume](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/resume-an-app)
- [ApplicationExecutionState](https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.activation.applicationexecutionstate)

## 6.5 Background tasks and app services

Background tasks are triggered by system/application events and may run while the foreground app is suspended. Registration can include conditions; Windows validates registration and may cancel execution when policy or resource requirements are no longer met. Trigger types have resource/time constraints, and user settings can limit background activity. An implementation must support cancellation and progress/completion notification rather than assuming background work runs indefinitely.

App services expose a service endpoint to another app, typically exchanging a ValueSet of primitive values (with richer values serialized). They can run in-process or out-of-process. This is an IPC contract, not simply a function call; lifetime, activation, connection closure, and serialization matter.

Implementation implications:
- Defer broad background task support until a selected app proves it is required; begin with registration/trigger/condition/cancellation contracts and deterministic fixtures.
- Never promise unbounded background execution.
- Model app-service connection lifetime and provider activation, validate ValueSet payloads, and handle disconnects and provider termination.
- Keep app services distinct from RPC, COM marshaling, and arbitrary host IPC.

References:
- [Background task conditions](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/set-conditions-for-running-a-background-task)
- [In-process background tasks](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/create-and-register-an-inproc-background-task)
- [Timer background tasks](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/run-a-background-task-on-a-timer-)
- [App services and extensions](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/app-services)
- [UWP interprocess communication](https://learn.microsoft.com/en-us/windows/uwp/communication/interprocess-communication)

## 6.6 Storage and file access

UWP storage APIs include Windows.Storage, Windows.Storage.Streams, and Windows.Storage.Pickers. Apps have package-private Local, LocalCache, Roaming, and Temporary data locations. Access outside those locations is constrained: a picker can grant access to a user-selected item, and FutureAccessList can preserve access beyond the immediate activation/session. Declared capabilities can grant specific library or device access, but broad filesystem access is restricted and user/privacy policy still applies.

Implementation implications:
- Resolve package data folders from package identity, not a shared global directory.
- Keep LocalState, LocalCache, RoamingState, and TemporaryFolder semantics distinct; do not assume all are backed up or synchronized identically.
- Persist picker grants using a per-package FutureAccessList model and enforce the grant on subsequent opens.
- Treat manifest capabilities, picker grants, and Raven's explicit host path grants as separate authorization sources.
- Do not map a file picker selection to permanent unrestricted access to its parent directory.

References:
- [Files, folders, and libraries](https://learn.microsoft.com/en-us/windows/uwp/files/)
- [File access permissions](https://learn.microsoft.com/en-us/windows/uwp/files/file-access-permissions)
- [UWP file I/O learning track](https://learn.microsoft.com/en-us/windows/uwp/get-started/fileio-learning-track)

## 6.7 WinRT ABI, async, and threading

WinRT is COM-based. WinRT interfaces implicitly require IInspectable, which in turn requires IUnknown. Runtime classes implement interfaces rather than exposing methods directly on the class identity; activation factories, static interfaces, and parameterized interface instances are separate ABI concepts. Interface IID derivation and metadata-defined vtable layouts must come from authoritative metadata, not guessed C++ declarations.

Many WinRT APIs are asynchronous. C++/WinRT represents common async operations with coroutines and co_await; async completion, cancellation, progress, and deferrals have observable lifetime and ordering semantics. WinRT also uses thread-pool work items for background work, but this does not replace UI-thread dispatcher/apartment requirements.

Implementation implications:
- Preserve exact ABI signatures, HRESULT behavior, output initialization, vtable order, IIDs, and reference-counting semantics.
- Treat apartment initialization, ASTA/UI-thread affinity, agile references, and cross-apartment marshaling as distinct from basic COM refcounting.
- Ensure async operation objects remain alive through completion/cancellation and invoke completion handlers according to the contract.
- Do not block the UI dispatcher waiting for work that may complete on that dispatcher.
- Generate interface layouts from metadata and add ABI contract tests for every newly implemented interface.

References:
- [WinRT type system](https://learn.microsoft.com/en-us/uwp/winrt-cref/winrt-type-system)
- [Asynchronous programming in UWP](https://learn.microsoft.com/en-us/windows/uwp/threading-async/asynchronous-programming-universal-windows-platform-apps)
- [Threading and async programming](https://learn.microsoft.com/en-us/windows/uwp/threading-async/)

## 6.8 Prioritized BryairBird work derived from this research

The documentation does not justify implementing the entire UWP surface at once. The repository audit already records that package identity APIs currently return no-package, deployment reports an empty inventory, CoreApplication is modeled as a desktop process without package identity/CoreWindow, and package-aware activation is not established. Prioritize those concrete gaps:

1. Implement process-owned package identity with correct Windows buffer sizing and cross-process query behavior.
2. Validate and expose manifest dependency and target-device-family data for package registration.
3. Add a narrowly scoped packaged activation path that carries activation kind/arguments into the process and uses package-aware class resolution.
4. Add a testable CoreApplication lifecycle foundation, preserving no-package desktop behavior.
5. Defer broad capability enforcement, file picker persistence, app services, and background tasks until a target app trace identifies the needed subset; document their security/lifetime contracts before implementation.

For every task, require a synthetic fixture, explicit unsupported/error behavior, no fabricated identity/entitlement, and regression tests for ordinary desktop processes.

## 6.9 Source freshness and interpretation

Microsoft Learn pages are living documentation and may describe APIs across multiple Windows versions. The applicable API contract, minimum OS version, device family, manifest namespace, and behavior must be checked against the actual package's target and the selected Windows component build. This synthesis is a starting index, not a substitute for those version-specific checks.
