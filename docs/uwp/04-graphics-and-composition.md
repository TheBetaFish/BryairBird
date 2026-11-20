# 4. Graphics and composition: XAML, DirectComposition, DXGI, Wayland, Vulkan

**Status:** Proposed architecture. Implementation not started.

## 4.1 Existing Raven foundation

Raven already contains:
- `native/guest/raven-composition/composition.c`: substantial DirectComposition/Windows.UI.Composition visual-tree implementation.
- `composition_interfaces.h` and `composition_tables.h`: interface definitions and vtables.
- `testdata/native_loader/composition_client.c` and `composition_swapchain_client.c`: composition fixtures.
- `native/desktop/presentation.h`: host-surface/presentation abstraction.
- `native/desktop/wayland.cpp`: Wayland toplevel, popup, subsurface, and host-surface handling.
- `native/graphics/vulkan_bridge.cpp/.h`: guest-to-host Vulkan dispatch and resource synchronization.
- DXVK/vkd3d-proton integration documented in Raven's README.

This is a real starting point. Extend and validate the current implementation against the interfaces used by harvested XAML/app-model binaries; do not create a parallel compositor.

## 4.2 Correct object model

Do not map each DirectComposition visual to a Wayland subsurface.

A DComp visual is a retained graph node. It may be transformed, clipped, animated, grouped, and backed by a surface or swap chain. A Wayland subsurface is a separate surface with window-system relationship, stacking, and input semantics. They are not equivalent.

Preferred architecture:

    UWP/XAML guest process
          |
       DXGI / D3D
          |
    Raven DComp guest ABI
          |
    retained visual tree + commit state
          |
    Raven composition renderer
          |
    GPU texture / shared resource
          |
    Vulkan / DXVK / vkd3d-proton
          |
    Raven host presentation surface
          |
        Wayland

The composition tree remains inside Raven. Wayland receives the final application surface (or a small number of genuine independent window surfaces), not every visual node.

## 4.3 Commit and rendering

The current composition source describes a retained tree committed and drawn into the target window. Preserve the separation:
- Guest calls mutate the retained tree.
- Commit publishes a consistent tree snapshot.
- Renderer consumes the snapshot.
- Host presentation displays the completed frame.
- Resource references remain alive until GPU completion.
- Resize/close releases resources only after outstanding work retires.

A future renderer may use a Vulkan composition pass, a D3D11/DXGI path through DXVK, or shared GPU resources. Choose based on Raven's resource ownership and measured interop support, not assumptions.

## 4.4 DXGI composition swap chains

XAML commonly relies on composition-capable swap-chain behavior. Validate:
- `CreateSwapChainForComposition`;
- composition surface attachment;
- flip-model buffers and frame latency;
- alpha mode and premultiplied alpha;
- resize and buffer replacement;
- color space and format selection;
- present flags and occlusion;
- fences/frame synchronization;
- shared handles and cross-device resource access.

Extend the existing composition swap-chain fixture with create-device, create-swap-chain, attach, mutate, commit, present, resize, detach, and release sequences.

## 4.5 Wayland integration

Use Raven's existing `DesktopPresentation` and `HostSurface` abstractions. Wayland owns:
- xdg-shell top-level lifecycle;
- popup/transient window behavior;
- genuine child-window/subsurface relationships;
- input routing and focus;
- configure/resize events;
- frame callbacks and presentation pacing;
- host-surface lifetime.

The guest Windows ABI must not expose Wayland protocol objects. Guest window and visual IDs remain Raven-managed handles.

A UWP app normally presents one main top-level surface. XAML popups may need independent popup surfaces depending on semantics. Compose popups into the main frame unless Windows behavior requires a genuine host popup.

## 4.6 Vulkan and Direct3D

Raven's Vulkan bridge marshals Vulkan calls and provides synchronization hooks. Raven's README documents DXVK/vkd3d-proton for Direct3D. Its D3D10.1 component delegates device creation to a configured D3D core.

Reuse this stack:
- XAML/DComp calls guest-facing DXGI/D3D interfaces.
- DXVK/vkd3d-proton translates Direct3D where supported.
- Vulkan bridge handles the guest/host Vulkan boundary.
- Composition renderer consumes the resulting textures/resources.
- Wayland presents the final frame.

Do not claim all D3D11/12 composition paths are already supported by Raven's Vulkan bridge. DXVK/vkd3d-proton are separate components with their own ABI/resource models; integration with Raven's composition code must be tested.

## 4.7 Synchronization and lifetime

Define ownership for D3D devices/contexts, DXGI buffers, DComp surfaces/visuals, Vulkan images/memory, shared handles, fences/semaphores, CPU staging buffers, Wayland buffer-release callbacks, resize, and destruction.

Rules:
- Never reuse a buffer until guest and host compositor ownership have ended.
- Never destroy a GPU resource while submitted work references it.
- Avoid CPU readback on the steady-state path; retain it as fallback/test oracle.
- Keep a deterministic software-rendered reference path for correctness tests.
- Add timeout and device-loss handling.
- Validate every cross-boundary resource handle.

## 4.8 XAML bring-up sequence

Milestone 3 uses Calculator, after confirming the exact package/framework version.

1. Load the harvested XAML binary and dependencies without launching an app.
2. Resolve WinRT classes/factories used during framework startup.
3. Establish package identity and CoreApplication/CoreWindow.
4. Establish UI-thread apartment and CoreMessaging dispatcher.
5. Create a blank XAML application window.
6. Render a solid background and static text.
7. Render a button and verify pointer/keyboard input.
8. Verify layout, DPI, resize, popup, and focus.
9. Enable animation and composition surfaces.
10. Add pixel and interaction regression tests.

A window appearing once is insufficient. Verify input, resize, lifecycle, and repeatability.

## 4.9 Performance and diagnostics

Measure CPU time/frame, GPU submission/completion latency, present-to-display latency, missed frame callbacks, copies/readbacks, swap-chain stalls, memory growth during resize/open/close, and input-to-visible-response latency.

Every diagnostic should identify the layer: XAML, DComp, DXGI, D3D translation, Vulkan bridge, or Wayland. Do not set a universal 60 FPS requirement before a stable baseline exists.

## 4.10 Test matrix

Synthetic tests:
- DComp visual mutation and commit ordering;
- parent/child transforms and clipping;
- composition surface attach/detach;
- swap-chain alpha, resize, and buffer lifetime;
- frame pacing and release callbacks;
- resource/fence ownership and device loss;
- popup/transient surface behavior.

Integration:
- existing composition fixtures;
- native synthetic DComp application;
- synthetic XAML-like app using the selected Calculator interfaces;
- Calculator under lavapipe where feasible;
- hardware Vulkan path on at least one supported driver.

Regression:
- ordinary Win32 presentation unchanged;
- DXVK/vkd3d-proton games unaffected;
- Wayland and experimental X11 remain separate/tested.

## 4.11 Hello Neighbor

Hello Neighbor is a game target, not a first graphics validation target. Before treating it as a D3D12/Unreal UWP workload:
- establish actual Store package format and manifest;
- inspect actual executable graphics imports/features;
- determine whether it uses UWP, GDK, or another PC game model;
- identify the DXGI/D3D runtime path;
- validate offline launch before adding online services.

Do not infer the Store build's engine version or package model from the Steam release.

## 4.12 References

- Raven: `native/guest/raven-composition/composition.c`, `native/desktop/presentation.h`, `native/desktop/wayland.cpp`, `native/graphics/vulkan_bridge.cpp`, `native/guest/raven-d3d10/d3d10_1.c`.
- [designgears/wine-uwp](https://github.com/designgears/wine-uwp) is a useful comparison for DComp/swap-chain work, but its Wine implementation is not portable as-is.
