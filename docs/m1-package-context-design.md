# M1 design: Raven package context and activation boundaries

**Project:** BryairBird  
**Milestone:** M1 — generic package-context design  
**Status:** Design proposal; no runtime implementation or test result is claimed  
**Depends on:** [M0 feasibility audit](m0-feasibility-audit.md)

## Purpose

Define the smallest coherent set of Raven extensions needed to move from reading an APPX/MSIX archive to launching a process with a package context. This is a generic design for testable package semantics. It is not a claim that Hello Neighbor uses APPX/MSIX, nor a specification for its unknown Store package.

The design deliberately extends Raven's existing package reader, WinRT plumbing, activation lookup, and process model. It does not introduce a second package parser or a parallel COM/WinRT implementation.

## Design principles

1. **Separate parsing from deployment.** Reading a manifest must not mutate the installed-app database or make a package launchable.
2. **Keep package state explicit.** A process should receive package identity and graph state from a launch context, not from ambient global state.
3. **Make installation transactional.** A failed deployment must not leave a partially registered package.
4. **Resolve dependencies deterministically.** Framework and resource packages must be selected from declared package metadata and explicit deployment state.
5. **Preserve desktop behavior.** Ordinary Raven processes must continue to report no package identity.
6. **Test with synthetic packages.** Initial tests must use redistributable fixtures with generated identities and harmless test executables.
7. **Do not infer target requirements.** Hello Neighbor-specific activation, graphics, licensing, and service requirements remain blocked on inspection of its actual Store package.

## Proposed subsystem boundaries

| Subsystem | Existing Raven foundation | Proposed responsibility | Must not do |
|---|---|---|---|
| Package reader | `native/guest/raven-appx/` | Read package metadata and expose a validated immutable package description | Install packages, register classes globally, or launch processes |
| Deployment store | Existing app/prefix tracking in `native/nt/apps.cpp` is adjacent, but not equivalent | Track package payload location, identity, state, and deployment transaction | Treat an ordinary app layer as a UWP package graph |
| Package graph resolver | No complete implementation established by M0 | Resolve main, framework, resource, and dependency packages for a launch | Guess dependencies or silently select incompatible versions |
| Process package context | Existing package identity fixture provides a baseline | Supply identity and package graph to the process being created | Change identity for unrelated desktop processes |
| Application activation | Existing `native/guest/raven-shell/winrt.c` activation path | Resolve the declared application entry point and start it with package context | Assume a WinRT class factory alone is an app entry point |
| WinRT class activation | Existing `native/guest/raven-winrt/` and registry lookup | Resolve package-aware class registrations before any defined fallback | Load arbitrary DLL paths from untrusted package metadata |
| Runtime services | Not established by M0 | Add only the WinRT/application-model services required by measured test cases | Implement broad APIs without consumers or test evidence |

These are conceptual boundaries, not names of new source files or exported APIs. Concrete Raven interfaces should be chosen only after inspecting the repository's existing ownership, error, and lifetime conventions.

## Required data model

The first implementation should define internal data structures equivalent in meaning to the following. These are field-level requirements, not a proposed public ABI.

### Package description

- Canonical package identity: name, publisher, version, and processor architecture.
- Package type/role where declared: main, framework, resource, or other supported type.
- Manifest application declarations and their entry-point metadata.
- Declared dependencies with version and architecture constraints.
- Target device-family constraints.
- Declared capabilities and resource qualifiers.
- Package root and parsed manifest provenance.
- Validation status and structured diagnostics.

The parser should preserve source values and report unsupported or malformed constructs. It must not silently reinterpret unknown manifest elements as supported behavior.

### Deployment record

- Package identity and immutable payload location.
- Deployment state: staged, registered, failed, or removed.
- Resolved dependency references.
- Registration metadata and timestamps where Raven's conventions require them.
- Integrity/validation result.
- A transaction identifier or equivalent rollback boundary.

A package should become launchable only after the required payload and registration steps have succeeded.

### Per-process package context

- Main package identity.
- Resolved package graph.
- Application identity/entry point selected for launch.
- Package-relative resource and file lookup roots.
- Any environment or activation metadata required by the supported application model.

This context must be owned by the process/launch instance. Do not implement it as a mutable process-global singleton.

## Package graph resolution rules

The resolver should be deterministic and reject ambiguous graphs. At minimum it needs to:

1. Match dependencies by package identity and declared version constraints.
2. Reject architecture-incompatible dependencies.
3. Distinguish framework packages from the main application package.
4. Preserve resource-package qualifiers rather than treating all resources as interchangeable.
5. Detect missing dependencies and dependency cycles.
6. Produce a stable resolved graph that can be logged and reproduced.
7. Return explicit unsupported-format or unsupported-manifest errors when the current implementation cannot safely interpret a declaration.

The initial fixture set should cover a single main package, one framework dependency, one resource package, a missing dependency, an incompatible architecture, and an invalid version constraint. These are synthetic test cases, not claims about the target game.

## Process identity and compatibility

The existing no-package fixture is a regression requirement: ordinary processes must continue to return the existing no-package result.

A separate packaged-process fixture should verify that:

- The package identity APIs return the expected identity only inside the launched test process.
- A second ordinary process remains un-packaged.
- The process package graph contains exactly the dependencies resolved for that launch.
- Process teardown releases package-context references.
- Failed activation does not leak package identity into a later process.

The precise Windows API surface and error mapping should follow the existing Raven compatibility conventions. Do not invent new externally visible Windows behavior solely to simplify internal implementation.

## Activation sequence

The proposed high-level launch sequence is:

1. Locate an already staged package by canonical identity.
2. Load its parsed package description.
3. Resolve and validate the package graph.
4. Select an application declaration from the manifest.
5. Construct an immutable launch context containing package identity and graph.
6. Create the process with that context attached before application code can query package identity.
7. Initialize package-aware activation and resource lookup.
8. Invoke the declared entry point using the activation model actually supported by the package.
9. Tear down process-owned context on exit.

This sequence is intentionally abstract. The exact entry-point contract cannot be chosen until a real package manifest and Raven's process-creation lifecycle are examined together.

## Security and trust boundaries

- Treat package manifests, archive paths, dependency declarations, and class-registration metadata as untrusted input.
- Canonicalize paths and prevent archive path traversal.
- Do not load a DLL merely because an untrusted manifest names a path; require it to resolve within an approved package or runtime location under Raven's policy.
- Keep package payloads isolated from unrelated app prefixes.
- Do not bypass Store licensing, encryption, signature checks, or entitlement requirements.
- Never store account tokens or license material in the repository or test fixtures.
- Log package identity and failure reasons without logging credentials or private license data.

## M1 implementation order and acceptance criteria

| Step | Work | Acceptance evidence |
|---|---|---|
| M1.1 | Inspect existing Raven package-reader and process-creation interfaces in detail | Source references and a concrete internal interface proposal; no duplicate parser |
| M1.2 | Define immutable package-description and diagnostic model | Unit tests for valid, malformed, and unsupported synthetic manifests |
| M1.3 | Define deployment-state storage and transaction boundary | Tests prove failed staging/registration leaves no launchable partial record |
| M1.4 | Implement deterministic dependency graph resolution | Fixture tests for framework/resource dependencies and all listed failure cases |
| M1.5 | Attach package context to a child process | Packaged identity fixture passes while existing no-package fixture remains unchanged |
| M1.6 | Connect package-aware activation to the existing activation layer | A harmless synthetic packaged test app activates; ordinary desktop activation remains unchanged |

M1 is not complete until the relevant build and test commands have actually run in CI or a documented local environment. Merely adding test definitions does not count as passing them.

## Explicitly deferred

The following must not be implemented based only on this design:

- Hello Neighbor-specific entry-point or engine assumptions.
- Store acquisition, entitlement, or licensing workarounds.
- Encrypted package decryption or signature bypass.
- Broad CoreApplication, CoreWindow, XAML, DirectComposition, or Xbox services implementation without a package-driven requirement.
- MSIXVC support unless inspection establishes that the target or a separately selected test target requires it.
- Any claim of game compatibility before a real target package has been inspected and launched.

## M1 exit conditions

M1 design is ready for implementation only when the repository's actual process creation and ownership conventions have been inspected and the proposed data flow can be mapped to existing Raven abstractions without duplicating them.

M1 implementation passes only when synthetic package fixtures, package-graph tests, packaged-process identity tests, and the ordinary no-package regression test run successfully. Hello Neighbor remains a separate target-validation milestone.
