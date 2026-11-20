# Hello Neighbor target investigation

**Target:** Hello Neighbor (2017), Microsoft Store edition  
**Project:** BryairBird  
**Status:** Target definition; package not yet verified

## Goal

Evaluate whether BryairBird can install, activate, display, and run the Microsoft Store edition of Hello Neighbor on Linux using the Raven-derived runtime.

This document concerns the Store edition only. Results from the Steam edition must not be treated as proof that the Store edition will work.

## Store identity

- Product ID: 9NDDH3R9DF40
- Publisher/developer branding: tinyBuild / Dynamic Pixels
- Release: 2017

The product ID identifies the Store listing; it does not identify the downloaded package format or guarantee current availability in every region.

The official [Xbox Store listing](https://www.xbox.com/en-us/games/store/hello-neighbor/9nddh3r9df40) currently lists Xbox Play Anywhere and PC/handheld play, and identifies tinyBuild as publisher, Dynamic Pixels as developer, and December 8, 2017 as the release date. These facts confirm the PC-capable listing but do not establish the Windows package format.

## Questions that must be answered from the actual package

| Question | Evidence required |
|---|---|
| Is the package APPX, MSIX, MSIX bundle, or MSIXVC? | Actual downloaded package/container and its metadata. |
| Is it UWP or a packaged desktop/GDK application? | Manifest declarations, application entry point, package capabilities, and activation behavior. |
| Which CPU architecture is included? | Package architecture and executable metadata. |
| What is the application entry point? | Package manifest. |
| Which framework packages are required? | Manifest dependency declarations and package metadata. |
| Which Windows Runtime classes are activated? | Imports, activation requests, and runtime diagnostics from a lawful test environment. |
| Which graphics interfaces are used? | Import/activation evidence and observed API calls. |
| Does it require Xbox services or a Store license at launch? | Controlled launch tests and documented errors. |
| Does the Store build differ materially from Steam? | Compare only lawful package metadata and publicly documented build information. |

Do not fill unknown cells by guessing from the Steam edition.

## Investigation procedure

1. Confirm the Store listing and entitlement using the user's own Microsoft account.
2. Obtain the package only through a supported, legitimate method.
3. Keep the package and any licensed content outside this public repository.
4. Record non-sensitive metadata: package format, identity, version, architecture, entry point, dependencies, and declared capabilities.
5. Inspect the manifest and executable metadata without modifying or redistributing the package.
6. Compare the observed requirements against the BryairBird UWP roadmap.
7. Record each unknown and the evidence needed to resolve it.

Never commit the game package, extracted game assets, proprietary DLLs, account tokens, license files, or personal Store credentials.

## Compatibility checkpoints

Use these checkpoints when reporting results:

1. **Listing verified:** the Store product is identified.
2. **Package obtained:** the user's entitled package is available locally.
3. **Package inspected:** format, manifest, identity, architecture, and dependencies are recorded.
4. **Package staged:** BryairBird can validate and stage the package in an isolated prefix.
5. **Activation reached:** the application entry point is invoked through the expected activation model.
6. **Window created:** the app creates a visible window and processes input.
7. **Menu reached:** the game reaches its main menu.
8. **Gameplay reached:** a playable session starts.
9. **Stability tested:** repeat launches, save behavior, shutdown, and basic gameplay are checked.

A checkpoint is only complete when its steps are reproducible and logs are retained. Reaching a later-looking screen once is not a substitute for recording the earlier stages.

## Current status

No Store package has been inspected as part of this document. The exact package format, manifest, entry point, architecture, dependencies, activation model, and runtime requirements remain **unverified**. Microsoft documents APPX/MSIX app packages separately from the GDK's MSIXVC PC-game packaging workflow; the Store listing alone cannot tell us which applies to this product.

The public and Raven-source portions of M0 are recorded in [the M0 feasibility audit](m0-feasibility-audit.md). The remaining blocker is inspection of the actual package metadata. Do not infer the package type from Play Anywhere, the Store listing, or the Steam build.
