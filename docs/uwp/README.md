# BryairBird UWP / WinRT architecture blueprint

This directory specifies how to extend Raven with native packaged-app, UWP, WinRT, XAML, and DirectComposition support.

**Design principle:** preserve Raven's native Linux runtime model. Extend its process, package, ABI, activation, graphics, and presentation subsystems. Do not transplant Wine internals or treat Windows binaries as a substitute for the OS services they expect.

## Repository files inspected

This is a targeted source audit of the current `main` branch, not an exhaustive review of all 1,795 tracked files.

### Runtime, loader, and process model
- `README.md`
- `CMakeLists.txt`
- `native/CMakeLists.txt`
- `native/main.cpp`
- `native/pe/loaded_image.cpp`
- `native/pe/module_set.cpp`
- `native/pe/module_set.h`
- `native/nt/loader.cpp`
- `native/nt/loader.h`
- `native/nt/actctx.cpp`
- `native/nt/child_api.cpp`
- `native/nt/child_process.cpp`
- `native/nt/child_process.h`
- `native/nt/process_info_api.cpp`
- `native/nt/system_api.cpp`

### Prefix, registry, package, and WinRT
- `native/nt/prefix.cpp`
- `native/nt/registry.cpp`
- `native/nt/registry.h`
- `native/guest/raven-appx/appx.h`
- `native/guest/raven-appx/archive.c`
- `native/guest/raven-appx/blockmap.c`
- `native/guest/raven-appx/manifest.c`
- `native/guest/raven-appx/package.c`
- `native/guest/raven-deployment/deployment.c`
- `native/guest/raven-winrt/winrt.c`
- `native/guest/raven-winrt/winrt.h`
- `native/guest/raven-winrt/foundation.h`
- `native/guest/raven-shell/winrt.c`
- `native/guest/raven-coremessaging/coremessaging.c`
- `native/guest/raven-twinapi/twinapi.c`
- `native/guest/raven-rpcrt4/rpc_base.c`

### Composition, graphics, and tests
- `native/guest/raven-composition/composition.c`
- `native/guest/raven-composition/composition_interfaces.h`
- `native/desktop/presentation.h`
- `native/desktop/wayland.cpp`
- `native/graphics/vulkan_bridge.cpp`
- `native/graphics/vulkan_bridge.h`
- `native/guest/raven-d3d10/d3d10_1.c`
- `testdata/native_loader/appx_client.c`
- `testdata/native_loader/appx_fixtures.py`
- `testdata/native_loader/com_marshal_client.c`
- `testdata/native_loader/composition_client.c`
- `testdata/native_loader/composition_swapchain_client.c`
- `testdata/native_loader/coremessaging_client.c`
- `testdata/native_loader/deployment_client.c`
- `testdata/native_loader/package_identity_client.c`
- `testdata/native_loader/protocol_client.c`
- `testdata/native_loader/twinapi_client.c`
- `testdata/native_loader/winrt_import_client.c`

## Specification files

1. [Codebase gap analysis](01-codebase-gap-analysis.md)
2. [Prefix and Windows component harvest](02-prefix-and-harvest.md)
3. [App model and activation](03-appmodel-activation.md)
4. [Graphics and composition](04-graphics-and-composition.md)
5. [Milestone roadmap](05-milestone-roadmap.md)
6. [Microsoft UWP / WinRT platform contracts](06-microsoft-platform-contracts.md)

## Current source-level findings

- Raven already has PE loading, registry and activation-context infrastructure, APPX/MSIX reading, WinRT ABI helpers, CoreMessaging dispatcher queues, a twinapi.appcore-style shim, DirectComposition objects, Wayland presentation, and Vulkan bridging.
- The deployment component explicitly reports an empty package inventory and does not deploy, stage, or remove packages.
- Package identity APIs currently return `APPMODEL_ERROR_NO_PACKAGE`.
- The current CoreApplication shim models a desktop process without package identity or CoreWindow.
- DirectComposition is already a substantial implementation; it is not a blank subsystem.
- No evidence in this audit establishes that Windows 11 XAML binaries can already load or run successfully under Raven.
- No Windows binaries, Store packages, credentials, tokens, or licensing data belong in this repository.

## Guardrails

- No Wine dependency or Wine internal ABI.
- No checked-in Microsoft DLLs or WinMD binaries.
- No fabricated package identity, Store entitlement, Xbox identity, or authentication tokens.
- No bypass of package signatures, encryption, Store licensing, or account authentication.
- No one-to-one mapping of DComp visuals to Wayland subsurfaces. Keep the retained composition graph in Raven and present its output through host surfaces.
- Compatibility is established by per-app import, activation, API, and rendering tests.

## External reference

[designgears/wine-uwp](https://github.com/designgears/wine-uwp) documents the pattern of harvesting Windows components locally, implementing missing app-model/composition behavior, and keeping Microsoft binaries out of source control. Its implementation is Wine-specific; it is a research reference, not a drop-in base.
