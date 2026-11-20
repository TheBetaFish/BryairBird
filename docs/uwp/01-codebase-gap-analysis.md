# 1. Codebase gap analysis: Raven versus packaged UWP

**Status:** Source-based architecture assessment. No implementation changes are included.

## 1.1 Executive assessment

Raven is not starting from zero. The repository already contains:
- PE32/PE32+ parsing and loading through `native/pe` and `native/nt/loader.cpp`.
- A process/thread/kernel-object model and a child-runtime process architecture.
- Win32/NT services in `native/nt`.
- A persistent Raven registry with per-application registry layers.
- Activation-context and side-by-side manifest processing.
- APPX/MSIX archive, block-map, and manifest readers.
- Shared WinRT ABI/object/async/HSTRING helpers.
- Registry-based `RoGetActivationFactory`.
- CoreMessaging dispatcher queues.
- A `twinapi.appcore.dll`-style component that currently models desktop/no-package behavior.
- A DirectComposition visual-tree implementation and composition swap-chain test fixture.
- Wayland host presentation, host surfaces, and Vulkan call marshalling.
- DXVK/vkd3d-proton integration for Direct3D.

The largest gaps are package deployment and identity, package-aware process activation, CoreApplication/CoreWindow lifecycle, exact low-level contracts expected by harvested Windows binaries, and the graphics/synchronization contract joining XAML/DComp to Raven's host presentation.

Keep four layers distinct:
1. Windows compatibility ABI: Win32/NT, COM, RPC, WinRT.
2. Package/app model: registration, dependency graph, identity, activation, capabilities.
3. Windows UI/composition: CoreWindow, dispatcher, XAML, DComp, DXGI.
4. Linux host integration: Raven process model, sandbox, Wayland, Vulkan, audio/input.

## 1.2 PE and DLL loader

**Inspected:** `native/pe/loaded_image.cpp`, `native/pe/module_set.cpp`, `native/pe/module_set.h`, `native/nt/loader.cpp`, `native/main.cpp`.

Raven has a PE parser/loader, loaded-image mapping, relocation and module-set management, import resolution, delay-load support, API-set handling, and program-local library search. The PE parser is built as a Rust static library and linked into the C++ runtime.

The existence of a loader does not prove it can load the full Windows 11 component dependency closure. Harvested DLLs may require API-set contracts, forwarded exports, exact export ordinals, activation-context/package-graph DLL redirection, KnownDLL behavior, SxS policy, loader notifications, TLS/unwind/exception behavior, or system libraries not in the harvested set.

These are investigation targets, not claims that every feature is absent. Determine actual gaps from PE imports, delay imports, runtime traces, and failures for each selected binary.

**Design requirement:** create a component compatibility manifest recording Windows build, architecture, file version/hash, imports, delay imports, API-set contracts, transitive DLL closure, WinMD metadata, activation registrations, and load/test results. Avoid speculative DLL overrides.

## 1.3 Win32 and NT plumbing

Raven executes guest code on the host CPU inside a sandbox and answers Windows system/API calls through its native runtime. The child launch path is in `native/nt/child_api.cpp` and `child_process.cpp`; process-query IPC is in `native/nt/process_info_api.cpp`.

In `native/nt/system_api.cpp`, current-process package identity functions set the length output to zero and return decimal 15700 (`APPMODEL_ERROR_NO_PACKAGE`). Other-process variants do the same without querying the target's package context. `GetPackagesByPackageFamily` reports an empty inventory.

Likely pressure points from modern framework binaries include package graph APIs, app-container/token/capability queries, modern activation contexts, dispatcher/threadpool behavior, RPC service communication, memory synchronization, DPI/input, and graphics shared-resource handles. This is not an exact missing-symbol list until static and dynamic probes identify the calls.

**Workflow:** inspect PE imports → trace failed imports/activation/runtime calls → classify the failing contract → implement the smallest compatible behavior → add a fixture → rerun regressions.

## 1.4 COM and RPC

Raven has COM interface implementations and a COM marshaling fixture (`testdata/native_loader/com_marshal_client.c`). It also has a substantial `native/guest/raven-rpcrt4` implementation covering NDR, proxy, RPC binding/server, and related operations. The shared WinRT helper implements COM-like identity, QueryInterface, refcounting, IInspectable, HSTRING, async operations, boxed values, and collections.

A working IUnknown/IInspectable implementation does not guarantee a particular Windows Runtime interface ABI. Unmodified framework DLLs may depend on exact vtables/IIDs, apartments, ASTA, agile references, proxy/stub registrations, RPC endpoint behavior, and cross-process activation.

Priority investigations:
- STA/ASTA semantics for XAML UI threads;
- agile references and cross-thread marshaling;
- parameterized WinRT interface IID derivation and metadata-driven layout;
- proxy/stub discovery and activation;
- RPC authentication and endpoint naming;
- callback reentrancy, async lifetime, and shutdown order.

Use existing object helpers, but source interface layouts from authoritative WinMD/IDL metadata. Treat apartment/marshaling behavior as distinct from refcounting.

## 1.5 Registry, prefix, and activation contexts

`native/nt/prefix.cpp` creates Raven's Windows directory structure and stages Raven-owned components. `native/nt/registry.cpp` persists registry state in a Raven-managed hive format, registers runtime classes, and supports per-app registry layers. `native/nt/actctx.cpp` handles activation contexts and side-by-side manifests. `native/guest/raven-shell/winrt.c` resolves WinRT classes through the machine-wide ActivatableClassId registry key, reads DllPath, loads the library, and calls DllGetActivationFactory.

This is not a full Windows installation hive or StateRepository implementation. There is no demonstrated package-scoped class registration precedence or package graph context.

Keep canonical package inventory separate from ordinary-app layers. Expose package-aware views through identity APIs, package graph APIs, activation lookup, PackageManager, and application resolution. Do not import entire Windows hives into Raven's custom registry store.

## 1.6 APPX/MSIX and deployment

**Inspected:** `native/guest/raven-appx/archive.c`, `blockmap.c`, `manifest.c`, `package.c`, `raven-deployment/deployment.c`, and APPX/deployment fixtures.

The reader handles ZIP/ZIP64 archive structure, package footprint files, block maps, content types, and manifest metadata including identity, version, architecture, dependencies, target device families, capabilities, resources, prerequisites, and application declarations.

Source comments explicitly identify unsupported bundle reading, package writing, signature-bound block-map validation, encryption, and later manifest-reader interfaces. The deployment component currently returns empty package collections and does not stage, register, update, or remove packages.

Treat package reading, package verification, deployment, registration, and activation as separate capabilities. A successful archive parse is not a successful installation or proof of Store entitlement.

## 1.7 WinRT, CoreMessaging, and twinapi

`native/guest/raven-winrt/winrt.c` and `winrt.h` provide shared WinRT object/ABI plumbing; `foundation.h` is generated from Windows metadata. `raven-coremessaging/coremessaging.c` implements DispatcherQueue/controller/timer behavior using Raven's message loop.

`raven-twinapi/twinapi.c` provides a twinapi.appcore-style component. Its source comments state that CoreApplication currently models a desktop process with no package identity and no CoreWindow. Online ID/WebAuthenticationCoreManager currently model no Microsoft account/provider; ticket requests fail rather than fabricate identity.

Gaps: packaged CoreApplication lifecycle, CoreWindow, activation event ordering, package-aware WinRT activation, framework bootstrap, package graph/resource context, and the UI-thread apartment/dispatcher integration expected by XAML.

## 1.8 DirectComposition, DXGI, Wayland, Vulkan

`native/guest/raven-composition/composition.c` is already a substantial DirectComposition/Windows.UI.Composition implementation. It models retained visual nodes, properties, commits, surfaces, and swap-chain interaction. Fixtures include `composition_client.c` and `composition_swapchain_client.c`.

`native/desktop/presentation.h` exposes host surfaces; `native/desktop/wayland.cpp` implements Wayland toplevels, popups, and subsurface attachment for owned child windows. `native/graphics/vulkan_bridge.cpp` marshals Vulkan operations. Raven's README documents DXVK and vkd3d-proton for Direct3D.

Remaining requirements include exact XAML-used DComp interfaces, composition swap-chain semantics, D3D/DXGI resource sharing, fences, frame lifetime, DPI/resize/occlusion, popup behavior, and compatibility with the actual harvested interfaces.

A DComp visual is a retained graph node; a Wayland subsurface is a separate surface with window/input/stacking semantics. They are not interchangeable. Keep the DComp tree in Raven and present the composed output through host surfaces.

## 1.9 Evidence matrix

| Subsystem | Existing evidence | Gap or unknown |
|---|---|---|
| PE loading | `native/pe/*`, `native/nt/loader.cpp` | Actual Win11 DLL closure and unresolved imports not audited |
| Win32/NT | `native/nt/*` | Exact XAML/CoreMessaging runtime call gaps not mapped |
| COM | interface implementations and marshal fixture | Full ASTA, agile, proxy/stub and cross-process behavior unproven |
| RPC | `native/guest/raven-rpcrt4/*` | Windows service endpoint/auth compatibility unknown |
| Registry | `native/nt/registry.cpp` | No full imported Windows registry/StateRepository model |
| Activation contexts | `native/nt/actctx.cpp` | Package graph and package-scoped precedence unknown |
| APPX/MSIX | reader, manifest, block-map code | Bundle/deployment/verification gaps noted in source |
| Package identity | no-package API results | Process-owned package identity not implemented |
| WinRT ABI | common object/HSTRING/async helpers | App-specific interface and lifecycle coverage incomplete |
| CoreMessaging | dispatcher queues | XAML-specific dispatcher/ASTA integration unproven |
| App model | twinapi system classes | CoreApplication currently desktop/no-CoreWindow |
| DirectComposition | substantial implementation and tests | XAML interop and GPU sync unproven |
| Wayland | host surfaces/subsurfaces | No direct mapping from DComp visual nodes to Wayland objects |
| Vulkan/D3D | Vulkan bridge, DXVK/vkd3d-proton | UWP composition-device and shared-resource paths need validation |

## 1.10 How to identify exact missing calls

For each harvested binary, begin with `Windows.UI.Xaml.dll`, `CoreMessaging.dll`, `twinapi.appcore.dll`, and their transitive dependencies:
1. Record PE architecture, version, imports, delay imports, exports, and API-set contracts.
2. Resolve each import to the Raven provider.
3. Build a minimal probe that loads the binary without launching an app.
4. Capture unresolved imports, missing registry values, activation failures, and trace output.
5. Use available symbols/metadata to identify interface and call-site contracts.
6. Add a test for each missing contract before implementing it.
7. Repeat with the next failure.

Static imports are only the first pass. XAML may resolve dependencies dynamically through GetProcAddress, WinRT activation, COM activation, and registry lookup.

## 1.11 Licensing and provenance

Microsoft DLLs remain Microsoft copyrighted binaries. Harvest locally from a Windows image the user is entitled to use. Do not commit, mirror, or include them in release artifacts. Store only metadata, hashes, extraction manifests, and test results in source control. Do not assume an evaluation image grants redistribution rights.

Wine-UWP is a technical reference, not a license grant for Microsoft's DLLs or a drop-in source for Raven.

## 1.12 References

- [Raven repository](https://github.com/TheBetaFish/BryairBird)
- [designgears/wine-uwp](https://github.com/designgears/wine-uwp)
