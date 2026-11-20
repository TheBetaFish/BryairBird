# M0 feasibility audit: Raven and the Hello Neighbor target

**Project:** BryairBird  
**Milestone:** M0 — target and dependency inventory  
**Review date:** 2026-11-19  
**Status:** Partial — Raven source audit complete; target package inspection blocked pending package metadata

## Executive finding

Raven is not starting from zero on Windows packaging or WinRT. The current BryairBird source tree already contains:

- An APPX/MSIX package reader component (raven-appx) that parses package archives, block maps, and manifests.
- Shared WinRT object and ABI plumbing (raven-winrt).
- A registry-based RoGetActivationFactory / RoActivateInstance path in raven-shell.
- Native test fixtures for APPX package reading, WinRT activation imports, and the current no-package-identity behavior.

These are useful foundations. They are **not equivalent to UWP application deployment and execution**. The inspected code explicitly leaves important package operations unimplemented, and Raven's current process identity test expects ordinary processes to have no package identity.

The main engineering problem is therefore narrower than “add all UWP support from scratch,” but still substantial: connect package metadata and registration to process identity and application-model activation, then implement the target's actual WinRT, lifecycle, windowing, and graphics requirements.

## 1. Source-level inventory

The findings below are based on the current BryairBird source tree, not on a successful runtime test on the user's machine.

| Area | Existing source | What the code establishes | Important gap |
|---|---|---|---|
| APPX/MSIX package reading | [raven-appx](../native/guest/raven-appx/) | ZIP/ZIP64 archive handling, package footprint files, block-map reading and integrity checks, manifest parsing, package identity fields | Does not establish package deployment, registration, bundle installation, or encrypted Store package support |
| Manifest parsing | [manifest.c](../native/guest/raven-appx/manifest.c) | Reads identity, version, architecture, dependencies, target device families, capabilities, resources, prerequisites, and application declarations | The source says manifest schemas are not validated as Windows validates them; several newer reader interfaces remain E_NOTIMPL |
| Package reader API | [package.c](../native/guest/raven-appx/package.c) | Implements the APPX packaging reader interfaces for .appx / .msix package content | Source comments explicitly leave writing packages, bundle reading, encryption, editor/diagnostics classes, and several interfaces unimplemented |
| WinRT object plumbing | [raven-winrt](../native/guest/raven-winrt/) | Shared IInspectable-style object model, interface identity/refcounts, HSTRING helpers, and generated Windows.Foundation declarations | A generic ABI foundation is not a complete WinRT class library or UWP app model |
| WinRT class activation | [raven-shell/winrt.c](../native/guest/raven-shell/winrt.c) | RoGetActivationFactory reads the registered class's DllPath, loads that library, and calls DllGetActivationFactory; RoActivateInstance uses the resulting factory | This is class-factory lookup through the registry, not complete packaged-app activation, package graph setup, or CoreApplication startup |
| Process/package identity | [package_identity_client.c](../testdata/native_loader/package_identity_client.c) | The fixture explicitly expects current/process package identity queries to return APPMODEL_ERROR_NO_PACKAGE | Package identity for a Store app is not yet demonstrated |
| Prefix app tracking | [apps.cpp](../native/nt/apps.cpp) | Tracks ordinary installed programs, files, folders, registry changes, and per-app layers | This is Raven's existing installer/prefix ownership model, not APPX package deployment or the UWP package graph |
| Test wiring | [native/CMakeLists.txt](../native/CMakeLists.txt) | Defines APPX package-reader fixtures and WinRT activation-import and package-identity fixtures | CMake test definitions are not test results; no CI/CTest result was run as part of this audit |

### APPX package reader limitations recorded in source

The comments in native/guest/raven-appx/package.c explicitly identify these as not implemented:

- Writing packages and bundles.
- Reading bundles (the bundle-reader path is not implemented).
- Signature-bound block-map validation.
- Encryption.
- IAppxDigestProvider, IAppxFactory2, and editor/diagnostics classes.

The comments in native/guest/raven-appx/manifest.c also identify unsupported manifest reader interfaces for qualified resources, capabilities by class, optional package information, main/driver dependencies, host runtimes, and related metadata.

This is an important distinction: having a reader for an ordinary .msix file does not mean Raven can install a Store-delivered encrypted package, resolve its framework dependencies, register its identity, and launch its entry point.

## 2. WinRT and activation status

The current source has two different layers that should not be conflated:

1. **WinRT ABI plumbing:** shared COM-like object mechanics, IInspectable methods, strings, and some Foundation declarations.
2. **Activation:** a generic registry lookup for an already registered WinRT class, followed by loading its DLL and calling its activation-factory export.

A packaged UWP app needs more than those two layers. The runtime must establish the package context and identity, resolve package and framework dependencies, start the declared application entry point using the appropriate application model, and provide the runtime classes and services the application requests.

The current test fixture winrt_import_client.c requests Windows.Management.Deployment.PackageManager through RoActivateInstance. Its existence demonstrates that the project is testing a WinRT activation path; it does not by itself prove broad UWP activation or a complete package deployment API.

The current package_identity_client.c is especially informative: it asserts that the current process and queried process have no package identity. That behavior is correct for the ordinary desktop processes the fixture tests, but a packaged-app launch path will need a separate, explicit way to provide the right identity.

## 3. Hello Neighbor: publicly verified facts

The official [Xbox Store listing](https://www.xbox.com/en-us/games/store/hello-neighbor/9nddh3r9df40) currently identifies:

- Product: Hello Neighbor
- Publisher: tinyBuild
- Developer: Dynamic Pixels
- Release date: December 8, 2017
- Xbox Play Anywhere
- Playable platform listing includes PC and handheld

This confirms that the product listing is the PC-capable Store product relevant to this project. It does **not** reveal the actual Windows package format, manifest, executable architecture, or activation model.

### Package-format possibilities

Do not assume that “Microsoft Store game” automatically means classic UWP APPX/MSIX.

Microsoft documents APPX/MSIX and their bundles as app-package formats. Its GDK documentation separately describes PC-game MSIXVC packaging and explicitly distinguishes that workflow from the UWP/APPX package footprint. The target's actual downloaded package must decide which branch applies.

Useful official references:

- [Microsoft: APPX/MSIX package requirements](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/app-package-requirements)
- [Microsoft: Package identity overview](https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/package-identity-overview)
- [Microsoft GDK: Package a PC game in original MSIXVC](https://learn.microsoft.com/en-us/gaming/gdk/docs/features/common/packaging/overviews/packaging-getting-started-for-pc)

No public source located in this audit establishes whether the user's entitled Hello Neighbor PC download is APPX/MSIX, MSIXVC, or another package arrangement. The Store listing's Play Anywhere label is not sufficient evidence.

## 4. Current dependency matrix

| Requirement | Raven status from source audit | Confidence |
|---|---|---|
| Read ordinary APPX/MSIX package archive | Partial implementation exists | High |
| Parse basic package manifest and identity fields | Partial implementation exists | High |
| Read MSIX bundles | Explicitly not implemented in package reader | High |
| Read encrypted Store package content | Encryption explicitly not implemented in package reader | High |
| Install/register package into a package graph | Not demonstrated by inspected source | High |
| Provide package identity to a launched process | Current identity fixture expects no package; packaged identity path not demonstrated | High |
| Basic WinRT object/ABI plumbing | Exists | High |
| Generic WinRT activation factory lookup | Exists via registry DllPath | High |
| Full UWP application activation | Not demonstrated | High |
| CoreApplication/CoreWindow lifecycle | Not established by this audit | Medium |
| XAML/DirectComposition integration | Not established by this audit | Medium |
| Target-specific graphics path | Unknown until the actual Store build is inspected | High |
| Xbox services / Store licensing needs | Unknown for the target launch path | High |

“Not demonstrated” means the source audit did not establish the feature; it is not a claim that no partial behavior exists anywhere in the repository.

## 5. Architecture implications for BryairBird

The source audit strengthens the case for continuing with Raven as the base. We already have relevant package-reading and WinRT infrastructure, so the first implementation work should extend Raven's existing abstractions rather than create a second parallel APPX parser or generic WinRT object system.

The likely next design task is a boundary diagram and API contract for:

1. Package metadata and deployment state.
2. Per-process package identity and package graph.
3. Application activation and lifecycle.
4. WinRT class registration and activation.
5. Framework package dependencies and resource lookup.
6. Windowing, dispatch, composition, and graphics.

Wine-UWP remains a reference for behaviors and test cases. Its Wine-specific implementation should not be transplanted wholesale into Raven.

## 6. M0 exit status

**Completed from public/source evidence:**

- Confirmed the official Store listing and PC / Play Anywhere product context.
- Audited Raven's existing APPX reader, manifest parser, WinRT plumbing, activation lookup, process identity fixture, and test wiring.
- Identified explicit unsupported areas in the source.
- Established that package reading and UWP application deployment are separate capabilities.

**Still blocked:**

- Inspect the actual package obtained for the user's Store entitlement.
- Determine the exact package/container format and version.
- Read the actual manifest and identify architecture, entry point, dependencies, capabilities, and application declarations.
- Establish the target's required WinRT classes, activation model, and graphics interfaces.

**M0 is therefore partially complete, not passed.** Do not begin target-specific runtime implementation until the package metadata is available. If the package cannot be obtained for inspection, proceed only with a clearly labeled generic UWP test fixture, not assumptions about Hello Neighbor.
