# UWP compatibility roadmap

**Project:** BryairBird  
**Runtime foundation:** Raven  
**Status:** Research and feasibility planning  
**Last reviewed:** 2026-11-19

## 1. Objective

Investigate and, if technically feasible, implement support for packaged Windows applications that depend on the Universal Windows Platform (UWP) and Windows Runtime (WinRT).

The first target application is the Microsoft Store version of **Hello Neighbor (2017)**. The broader objective is a reusable compatibility layer, not a one-game launcher.

BryairBird is a Linux-native Windows application compatibility project. It is not intended to bundle a Windows installation or require users to boot a conventional Windows virtual machine.

## 2. Current baseline

BryairBird is based on Raven. The current Raven-derived runtime provides a native Linux host runtime for Windows processes, including process and thread management, files, registry behavior, windows and input. Its desktop and graphics path uses Wayland and Vulkan, with DXVK for Direct3D 8/9/11 and vkd3d-proton for Direct3D 12.

The existing project documentation and tested-application list describe conventional desktop applications and installers. They do **not** establish UWP, WinRT, APPX/MSIX deployment, or Microsoft Store application compatibility.

This roadmap therefore treats UWP support as new work. No UWP capability should be considered implemented merely because a related Windows API or a similarly named DLL exists.

## 3. Why Wine-UWP is a reference, not a drop-in port

[designgears/wine-uwp](https://github.com/designgears/wine-uwp) is a valuable implementation reference because it has already pursued packaged-app activation, package identity, WinRT integration, CoreWindow, composition, and Store package workflows.

Its implementation is built inside Wine, however. Wine and Raven have different internal runtime designs and different boundaries between guest Windows code and the Linux host. UWP work may depend on Wine-specific facilities such as its PE loader, NTDLL boundary, object and handle model, synchronization, COM implementation, windowing internals, and process services.

Consequently:

- Do not assume Wine-UWP patches can be applied directly to Raven.
- Do not copy code until the exact source, revision, file-level license, copyright notices, and compatibility with BryairBird's licensing have been reviewed.
- Prefer published Windows API documentation and specifications for defining behavior.
- Treat Wine-UWP as an implementation and test reference. Reimplement or adapt only the parts that fit Raven's architecture and licensing.
- Record every imported or adapted component in the project's provenance and license documentation before it is merged.

The first deliverable is a dependency and API mapping, not a large code port.

## 4. Required compatibility areas

UWP support is a stack of related capabilities. A working implementation will likely need all of the following areas, although the exact requirements depend on the target package.

| Area | Required investigation |
|---|---|
| Package format and deployment | Identify APPX, MSIX, bundle, or MSIXVC/GDK packaging; parse manifests; stage package contents; validate dependencies and architecture. |
| Package identity | Provide the package identity and package graph information expected by packaged applications. |
| Activation | Implement the relevant Windows application model activation path, including process startup and activation arguments. |
| WinRT ABI and COM | Support the ABI, activation factories, interface negotiation, reference counting, and threading behavior used by the app. |
| Runtime services | Identify required WinRT classes and services; implement only documented and observed requirements. |
| Application lifecycle | Investigate CoreApplication, views, suspension/resume, termination, and event dispatch. |
| Windowing | Determine whether the app requires CoreWindow, a dispatcher, input integration, or other UWP-specific window behavior. |
| Graphics | Investigate the app's D3D/DXGI path, swap-chain creation, composition, and presentation requirements. Reuse Raven's existing graphics architecture where possible. |
| Storage and resources | Investigate package-relative assets, resource qualifiers, application data, settings, and file access semantics. |
| Security model | Map package capabilities and AppContainer expectations onto Raven's existing sandbox. Do not silently equate the two security models. |
| Store and licensing | Separate package acquisition, entitlement, licensing, and runtime activation. A successful download does not prove that the app can launch. |

This is a research checklist, not a claim that every UWP application uses every item.

## 5. Proposed architecture

Keep the UWP-specific behavior behind explicit subsystems so that the existing desktop runtime remains independently testable.

- **Package subsystem:** package inspection, manifest parsing, staging, dependency registration, and package identity metadata.
- **Activation subsystem:** packaged application registration, activation requests, and process lifecycle integration.
- **WinRT subsystem:** ABI and COM-facing runtime classes, activation factories, and threading behavior.
- **Application model subsystem:** lifecycle, dispatch, view, and window integration.
- **Graphics bridge:** mapping the target application's documented graphics path onto Raven's existing Vulkan/DXVK/vkd3d-proton infrastructure where possible.
- **Security bridge:** mapping package capabilities to explicit Raven sandbox grants and denials.
- **Diagnostics:** structured logs for package identity, activation, missing runtime classes, interface requests, window creation, graphics initialization, and denied capabilities.

Avoid implementing a broad, speculative UWP API surface. Start with a small, testable subset and expand only when a test application demonstrates the need.

## 6. Milestones and acceptance criteria

### M0 — Establish the target and inventory dependencies

- Obtain the target package through a legitimate account and supported method.
- Record the package file type, manifest identity, architecture, dependencies, entry point, and declared capabilities.
- Determine whether the package is classic APPX/MSIX or uses the GDK/MSIXVC path.
- Identify the first activation and graphics APIs it imports or requests.
- Compare those requirements with Raven's existing implementations and Wine-UWP's documented work.

**Exit criteria:** a reviewed API/dependency matrix with unknowns clearly marked. No runtime code changes are required for this milestone.

### M1 — Define a minimal UWP test fixture

- Choose or create a small, legally redistributable test application/package.
- Make package inspection and manifest validation independently testable.
- Add tests for architecture, identity, dependency declarations, and invalid package handling.

**Exit criteria:** the test fixture can be inspected deterministically without launching a Windows application.

### M2 — Package identity and activation prototype

- Define the package identity data model.
- Design the mapping from packaged-app activation to Raven process creation.
- Test activation with a minimal fixture before attempting a commercial game.

**Exit criteria:** the fixture reaches a known activation milestone and reports structured diagnostics. Do not call this full UWP support.

### M3 — WinRT and application lifecycle

- Implement the smallest required WinRT activation and ABI surface.
- Test interface negotiation, reference lifetimes, threading, dispatch, and lifecycle transitions.
- Keep missing APIs explicit and diagnosable.

**Exit criteria:** a fixture can complete a documented lifecycle sequence without relying on undocumented behavior.

### M4 — Windowing and graphics

- Establish the required CoreWindow or other window model.
- Integrate event dispatch and input with Raven's desktop backend.
- Test swap-chain creation, resize, presentation, and shutdown.
- Validate on both software Vulkan and a supported hardware Vulkan driver.

**Exit criteria:** a test application can create and present a window reliably. A successful window alone is not evidence of game compatibility.

### M5 — Target application evaluation

- Install/register the target package using the supported package path.
- Attempt activation and capture structured logs at each boundary.
- Resolve only demonstrated blockers, adding regression tests for each fix.

**Exit criteria:** document the highest reproducible milestone reached: package inspection, registration, activation, window creation, menu, or gameplay.

### M6 — Distribution and security review

- Review package and license obligations for every component.
- Keep proprietary Windows binaries, Windows images, and game packages out of the public repository.
- Review sandbox behavior and document the limits of any compatibility exceptions.
- Produce reproducible build and test instructions.

**Exit criteria:** a release can be rebuilt from documented, legally distributable source and inputs, with known limitations stated.

## 7. Hello Neighbor target: facts to verify

The target is the **Microsoft Store version of Hello Neighbor (2017)**, not the Steam build.

Known identifier to use when researching the Store listing: 9NDDH3R9DF40.

Before designing around a package format, inspect the actual package manifest and files obtained through the user's legitimate Store entitlement. Do not infer APPX/MSIX versus MSIXVC/GDK from the Store listing alone.

The game has been associated with Unreal Engine 4 in PC release materials, but the exact Store build, engine version, graphics requirements, architecture, and runtime dependencies must be verified against the target package. Do not assume that the Steam build and Store build have identical packaging or startup behavior.

Store entitlement and package acquisition are separate from execution compatibility. A successful authentication or download is not proof that Raven can install or run the game.

## 8. Technical and project constraints

- **No Windows installation requirement:** development and runtime should remain Linux-hosted. Locally obtained Windows components, if any are legally necessary for research or testing, must not be committed or redistributed.
- **No proprietary implementation copying:** use public API documentation and appropriately licensed source. Do not use leaked Windows source, decompiled proprietary components, or disassembly as implementation material.
- **License provenance:** record upstream URL, revision, affected files, license, copyright notices, and modifications for every reused component.
- **Sandbox integrity:** compatibility must not silently grant Windows applications unrestricted access to the host filesystem, devices, or credentials.
- **Incremental implementation:** each milestone must have a minimal fixture, a reproducible test, and logs that identify the failing boundary.
- **No compatibility claims without evidence:** distinguish implemented, partially implemented, experimentally tested, and untested behavior.

## 9. Initial decision

Continue using Raven as the host-runtime foundation while investigating whether the UWP requirements can be implemented cleanly within its architecture.

Do not start by importing the entire Wine-UWP patch set. First complete M0: inspect the target package requirements, map Wine-UWP's relevant subsystems, and compare both against Raven's existing process, COM, windowing, graphics, and sandbox architecture.

The project should remain free to revise this decision if the dependency analysis shows that the required work would effectively recreate a large portion of Wine or undermine Raven's design.
