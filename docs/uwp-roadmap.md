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

The source tree already contains partial APPX/MSIX package-reading code, shared WinRT ABI plumbing, and a registry-based WinRT activation-factory path. These are meaningful foundations, but they do **not** establish packaged-app deployment, per-process package identity, full UWP activation, CoreApplication/CoreWindow lifecycle, or Microsoft Store game compatibility.

See [the M0 source audit](m0-feasibility-audit.md) for exact files and explicit limitations. No UWP capability should be considered complete merely because a related Windows API or similarly named DLL exists.

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

## 6. Canonical milestones

The detailed milestone schedule is maintained in [`uwp/05-milestone-roadmap.md`](uwp/05-milestone-roadmap.md), with task-level execution and acceptance gates in [`EXECUTION_PLAN.md`](EXECUTION_PLAN.md). Those two documents are the canonical schedule; this file is the high-level architecture and target overview.

The project uses one consistent sequence:

| Milestone | Scope |
|---|---|
| **M0** | Evidence, PE inspection tooling, and reproducible fixtures |
| **M1** | Prefix setup, component harvesting, and package registration |
| **M2** | Package identity, WinRT activation, and headless packaged lifecycle |
| **M3** | Calculator / XAML rendering validation |
| **M4** | Actual Hello Neighbor Store package and game compatibility |
| **M5** | Hardening, security review, and compatibility expansion |

The previous M0–M6 sequence in this document is retired. It split activation, lifecycle, graphics, and target evaluation differently from the implementation-oriented roadmap and caused duplicate milestone meanings. Those capabilities now live under the single sequence above.

The existing [M0 feasibility audit](m0-feasibility-audit.md) is retained as preliminary source and target research. Its historical “M0” label describes the audit's original context; it is not a separate active milestone and does not override the canonical schedule.

No milestone is complete merely because its design document exists. Completion requires the acceptance criteria and evidence specified in the canonical roadmap and execution plan.

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

## 9. Initial architectural decision

Continue using Raven as the host-runtime foundation while investigating whether the UWP requirements can be implemented cleanly within its architecture.

Do not start by importing the entire Wine-UWP patch set. Follow the canonical M0–M5 sequence in `uwp/05-milestone-roadmap.md`. Target-package inspection remains an evidence-gathering prerequisite for M4; do not infer package format or activation behavior before inspecting the legitimately obtained package.

The project should remain free to revise this decision if the dependency analysis shows that the required work would effectively recreate a large portion of Wine or undermine Raven's design.
