# 2. Prefix and Windows component harvest

**Status:** Proposed design. No harvesting scripts or runtime changes implemented.

## 2.1 Goals

- Keep Microsoft components outside Git and Raven release archives.
- Record source image, Windows build, architecture, file version, and hashes.
- Support multiple component sets without silently mixing versions.
- Separate Raven-owned compatibility DLLs from user-supplied Microsoft files.
- Make prefix setup deterministic, reversible, and auditable.
- Never harvest account tokens, passwords, Store licenses, or personal data.

## 2.2 Proposed layout

This is a proposal, not an existing Raven layout:

    ~/.local/share/bryairbird/
      components/
        win11/
          <build>-<architecture>/
            manifest.json
            hashes.sha256
            system32/
            syswow64/
            WinMetadata/
            Frameworks/
            Registry/
            StateRepository/
      prefixes/
        <prefix-name>/
          drive_c/
            Windows/System32/
            Windows/SysWOW64/
            Windows/WinMetadata/
            Program Files/
            ProgramData/
          bryairbird/
            package-store/
              packages/
              frameworks/
              resources/
              deployment.db
              package-graph/
            state/
              component-set.json
              package-registration/
            logs/

Ownership:
- components cache: immutable user-owned Windows component set.
- drive_c: Windows-compatible guest view.
- package-store: Raven package payload and canonical deployment records.
- state: generated compatibility data and component pins.
- logs: import, activation, package, and graphics traces.

Keep `RAVEN_PREFIX` and `NativePrefix` as the prefix authority. Add the component cache as an explicitly selected external store or a managed prefix subdirectory, not a competing prefix implementation.

## 2.3 Harvest pipeline

### Input

The user supplies a Windows image they are entitled to use. The pipeline should be local and read-only. It must not require account sign-in, download protected Store content, or extract installed Store apps.

### Stages

1. Inventory image edition, build, architecture, language, and selected image index.
2. Mount/read the image without modifying it.
3. Extract only an allowlisted set of Windows components and dependencies.
4. Record PE machine, version, SHA-256, imports, delay imports, exports, API-set dependencies, and WinMD metadata.
5. Collect only relevant activation/framework registration data; never import all registry hives blindly.
6. Inspect StateRepository schema/version from that specific image.
7. Validate architecture, hashes, dependency closure, and absence of personal data.
8. Publish atomically from a temporary staging directory.
9. Pin each prefix to a component-set identifier; upgrades are explicit and reversible.

### Example component manifest

    {
      "schema": 1,
      "source": {
        "kind": "windows-image",
        "build": "actual-image-build",
        "architecture": "x64",
        "edition": "actual-image-edition"
      },
      "components": [
        {
          "name": "Windows.UI.Xaml.dll",
          "relative_path": "system32/Windows.UI.Xaml.dll",
          "sha256": "computed-at-harvest",
          "machine": "AMD64",
          "file_version": "read-from-file-version-resource",
          "imports_manifest": "imports/Windows.UI.Xaml.dll.json"
        }
      ]
    }

This is a schema illustration only. Real manifests must contain measured values, never guessed versions or placeholder hashes.

## 2.4 Component groups

Start with an allowlist derived from the target's imports and runtime traces. Candidate groups:
- WinRT and metadata: `Windows.UI.Xaml.dll` and selected `WinMetadata/*.winmd`.
- Core messaging: `CoreMessaging.dll` and proven transitive dependencies.
- App model: `twinapi.appcore.dll` and selected package/application-model components.
- Graphics/composition: only the DComp, DXGI, D3D, DWrite, D2D and supporting components shown to be required.
- Framework resources: language/resource/PRI data needed by the selected app.
- Runtime dependencies: specific CRT, API-set, and system libraries in the proven closure.

This is not a claim that every listed file is required or safe to load. Every component needs a dependency and compatibility report.

## 2.5 Registry hives and registrations

Raven's `NativeRegistry` is a Raven-managed registry implementation persisted in its own hive format. It is not a native Windows registry hive parser/writer. It supports runtime class registrations and per-app registry layers.

Therefore:
- Never copy Windows SYSTEM/SOFTWARE hives over Raven's `registry.dat`.
- Do not merge every key from Windows into a prefix.
- A class's `DllPath` is not by itself proof that activation will work.
- Keep package-scoped registrations separate from machine-wide registrations.
- Convert only selected values into a documented Raven compatibility schema.
- Record provenance for each generated class/framework registration.
- Preserve architecture-specific registry views and correct path/environment expansion.

First implement a Raven-owned registration store that generates the minimal registry projection required by existing APIs. Consider a Windows hive importer only if traces prove it necessary.

## 2.6 StateRepository

Wine-UWP's repository lists a `gen-staterepository.py` generator. This is a useful research lead, but StateRepository is an internal, version-dependent Windows database, not a stable public interchange format.

Recommended strategy:
1. Inspect the StateRepository schema and metadata from the exact source image.
2. Identify which tables/relations the selected app-model and XAML binaries actually query.
3. Record the database schema version and Windows build.
4. Generate a minimal compatibility database only when schema and invariants are understood.
5. Keep Raven's package store as the source of truth; generate StateRepository as a compatibility projection.
6. Validate generated state against source-image data and actual app traces.
7. Reject unknown schema versions rather than fabricating plausible rows.

Do not write arbitrary rows based on guesses. Do not make StateRepository authoritative until transactions, concurrency, identity consistency, and schema compatibility are demonstrated. If that cannot be done safely, implement the required app-model interfaces in Raven instead.

## 2.7 Package store and graph

Keep package payloads separate from harvested OS components:
- `packages/<package-full-name>/`: validated app packages.
- `frameworks/<package-full-name>/`: framework packages.
- `resources/<package-full-name>/`: resource packages.
- `deployment.db`: Raven-owned canonical metadata.
- `package-graph/`: dependency graph snapshots/indexes.

A package record should include package full/family names, publisher and publisher ID, version, architecture, resource ID, install root, package type, manifest digest, validation status, framework/resource dependencies, application IDs and entry points, declared capabilities, integrity status, provenance, and deployment state.

Each process receives an immutable package-graph snapshot. Updating a package must not mutate the identity or paths of an already-running process.

## 2.8 APPX/MSIX/MSIXVC

Raven's reader supports important APPX/MSIX archive operations, but not complete deployment. Source comments identify bundle reading, package writing, signature-bound block-map validation, encryption, and some manifest interfaces as incomplete.

Do not assume the Hello Neighbor Store build is plain APPX/MSIX. It may use a different PC game packaging/activation path. Identify the actual package format, manifest, dependencies, and entitlement model from legitimate package metadata before target-specific work.

Encrypted packages must be handled only through supported, authorized validation/deployment flows. No decryption or license bypass is in scope.

## 2.9 Integrity and security

- Load components only when listed in the manifest and matching their hashes.
- Keep the component cache read-only to guest processes.
- Prevent guest writes to the canonical deployment database.
- Canonicalize package roots and prevent path traversal/symlink escape.
- Keep capabilities declarative; map only explicitly supported capabilities to narrow sandbox grants.
- Log rejected packages, missing dependencies, and component mismatches.
- Pin component-set versions per prefix and support rollback.
- Never store credentials or bearer tokens in the component cache.

## 2.10 Implementation order

1. Component manifest schema and validator.
2. Metadata-only PE scanner for imports, versions, and hashes.
3. Allowlist-driven local extraction.
4. Generated registration-data format.
5. Prefix component-set pin.
6. Synthetic package deployment database and fixtures.
7. Load one harvested DLL and capture its first unresolved dependency.

## 2.11 References

- [designgears/wine-uwp](https://github.com/designgears/wine-uwp) documents local Windows image harvesting and keeping Microsoft binaries out of the repository.
- Raven: `native/nt/prefix.cpp`, `native/nt/registry.cpp`, `native/guest/raven-appx/*`, `native/guest/raven-deployment/deployment.c`.
