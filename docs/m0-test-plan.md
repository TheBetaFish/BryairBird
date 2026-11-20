# M0.3 — PE inspector test plan

**Status:** test plan complete; implementation not authorized by this document alone  
**Date:** 2026-11-20  
**Governing specification:** `docs/m0-inspector-spec.md` (schema v1)  
**Architecture:** approved Decision 0003; reuse `native/peparse/`

## 1. Purpose and test strategy

Define deterministic synthetic fixtures and acceptance tests before implementing the read-only PE inspector. The test suite must validate the published contract, parser safety, output stability, and failure isolation without relying on proprietary Microsoft binaries or a cross-compiler.

Use a small in-test PE byte builder that writes explicit little-endian fields into bounded byte arrays. Build fixtures from known offsets and update only the fields relevant to each test. Keep fixture generation separate from production parsing. Every fixture is synthetic and reproducible from source.

The test layers are:

1. **Parser contract tests:** exercise the existing Rust parser and any narrowly scoped extensions.
2. **Inspector unit tests:** file reader, hashing, resource decoder, report model, serializer, and diagnostic mapping.
3. **CLI integration tests:** invoke the built inspector as a subprocess with a strict timeout; verify exit code, stdout/stderr, output file behavior, and directory traversal.
4. **Mutation and resource-limit tests:** deterministic malformed inputs with bounded execution.
5. **Safety tests/review:** verify the inspector remains read-only and never loads or executes inspected content.

No test may use `widen_il_only`, `LoadedImage`, relocation, process creation, dynamic library loading, or any execution-oriented parser API.

## 2. Fixture builder contract

The test-only builder must:

- Emit PE files directly into byte vectors; do not shell out to a compiler or assembler.
- Support PE32 and PE32+ optional-header layouts independently of the COFF machine field.
- Expose named field-writing helpers with explicit width and bounds checks.
- Produce a minimal valid DOS header, NT signature, COFF header, optional header, section table, and file-backed section data.
- Support explicit section and data-directory placement so tests can create valid, truncated, overlapping, and unmapped ranges.
- Generate deterministic names and byte patterns.
- Return both the byte vector and a fixture-layout record containing file offsets/RVAs used by assertions.
- Fail the test immediately if the builder itself attempts an out-of-range write.
- Keep each fixture small (normally under 16 KiB); size-limit tests may create sparse or generated files without storing large fixture blobs.

Do not commit generated PE binaries. Keep fixture definitions in test source.

## 3. Baseline fixture matrix

| ID | Fixture | Expected result |
|---|---|---|
| F-001 | Minimal PE32, I386, no imports | `ok`; PE32, x86, zero imports |
| F-002 | Minimal PE32+, AMD64, no imports | `ok`; PE32+, x64, zero imports |
| F-003 | PE32+ with ARM64 machine value | `ok` if header/layout is readable; architecture `arm64` |
| F-004 | PE32 with ARM machine value | `ok` if header/layout is readable; architecture `arm` |
| F-005 | Unknown machine with otherwise valid PE32+ headers | `ok` with `PE_UNKNOWN_MACHINE`; preserve numeric machine and report `unknown` |
| F-006 | Valid PE32 and PE32+ with different optional-header sizes | Correctly decode only fields within declared optional-header bounds |
| F-007 | Valid image with CLR directory and IL-only bit | CLR directory and IL-only flag reported; no .NET metadata claims |
| F-008 | Valid image with no CLR directory | `clr: null` |

For recognized machine/optional-header combinations that the runtime does not support, verify the specified warning and safe parsing behavior; do not infer bitness from the machine name. The ARM64 fixture must use PE32+.

## 4. Header and section cases

| ID | Mutation / input | Expected result |
|---|---|---|
| H-001 | Empty file | `error`, `PE_EMPTY_FILE`, `pe: null` |
| H-002 | 1–63 byte DOS-header truncations | `error`, bounded diagnostic; no crash |
| H-003 | Bad DOS signature | `error`, `PE_BAD_DOS_SIGNATURE` |
| H-004 | e_lfanew points beyond EOF or overflows | `error`, `PE_TRUNCATED_HEADER` |
| H-005 | Bad NT signature | `error`, `PE_BAD_NT_SIGNATURE` |
| H-006 | Truncated COFF or optional header | `error`, `PE_TRUNCATED_HEADER` |
| H-007 | Unsupported optional-header magic or impossible size | `error`, `PE_INVALID_OPTIONAL_HEADER` |
| H-008 | Directory count exceeds bytes available in optional header | `partial`, `PE_TRUNCATED_DIRECTORY_TABLE`; do not read beyond header |
| H-009 | Section table truncated | `partial` or `error` according to whether a valid PE identity/header was established; `PE_INVALID_SECTION_TABLE` |
| H-010 | Raw section range extends past EOF | `partial`, `PE_INVALID_SECTION_RANGE`; preserve independent header data |
| H-011 | Raw offset plus size overflows integer range | `partial`, `PE_INVALID_SECTION_RANGE` |
| H-012 | Zero sections | Must not crash; report an empty section list if the header is otherwise valid |
| H-013 | Overlapping virtual sections | Must not crash or read outside file; expected diagnostic/status requires specification clarification before implementation |
| H-014 | Overlapping raw sections | Must not crash or read outside file; expected diagnostic/status requires specification clarification before implementation |
| H-015 | RVA in headers, section raw data, zero-fill tail, gap, and outside image | Header/raw data map only when file-backed; zero-fill/gap/outside must not be treated as file bytes |

The test harness must assert arithmetic is checked before every offset-plus-length and count-times-element-size operation.

## 5. Import fixtures

Create PE32 and PE32+ variants with descriptor/thunk layouts at known offsets.

| ID | Fixture | Expected result |
|---|---|---|
| I-001 | No import directory | Empty imports and dependencies |
| I-002 | One DLL with two named imports | Preserve DLL and thunk order, names and hints |
| I-003 | One DLL with ordinal imports | Preserve ordinal values; do not invent names |
| I-004 | Multiple DLL descriptors | Preserve descriptor order |
| I-005 | PE32 thunk width | Read 32-bit thunk entries only |
| I-006 | PE32+ thunk width | Read 64-bit thunk entries only |
| I-007 | OriginalFirstThunk is zero | Use IAT fallback as specified |
| I-008 | API-set DLL name beginning `api-ms-win-` | Dependency kind `api_set`; no contract resolution |
| I-009 | API-set DLL name beginning `ext-ms-win-` | Dependency kind `api_set` |
| I-010 | Ordinary DLL name | Dependency kind `named_dll` |
| I-011 | Unmapped import-name RVA | `partial`, `PE_UNMAPPED_IMPORT_RVA` or `PE_MALFORMED_IMPORTS` as appropriate |
| I-012 | Unterminated DLL or symbol name | `partial`; bounded diagnostic; no unbounded scan |
| I-013 | Truncated descriptor array or thunk table | `partial`, `PE_MALFORMED_IMPORTS` |
| I-014 | Descriptor/symbol count at configured limit | Complete within limit |
| I-015 | Descriptor/symbol count above configured limit | Stop affected traversal; `partial`, `PE_LIMIT_EXCEEDED` |
| I-016 | Same dependency imported more than once with case variation | Preserve import records; dependency summary is unique and sorted by normalized then original name |

Validate the serialized order separately from the parser's internal data structures. Verify that names are preserved and normalization is limited to the documented lowercase dependency field.

## 6. Delay-import fixtures

| ID | Fixture | Expected result |
|---|---|---|
| D-001 | No delay-import directory | Empty delay imports |
| D-002 | RVA-based descriptor with named imports | Correct descriptor attributes, symbols, and RVA-based marker |
| D-003 | RVA-based descriptor with ordinal import | Correct ordinal and thunk width |
| D-004 | Legacy VA-based PE32 descriptor | Checked VA-to-RVA conversion and correct symbols |
| D-005 | Legacy VA-based PE32+ descriptor | Checked width and conversion; no truncation |
| D-006 | VA below image base | `partial`, `PE_MALFORMED_DELAY_IMPORTS` |
| D-007 | VA-to-RVA arithmetic overflow/out-of-range | `partial`; no pointer arithmetic wrap |
| D-008 | Unsupported/ambiguous descriptor attributes | `partial`, `PE_UNSUPPORTED_DELAY_IMPORT_FORM` |
| D-009 | Truncated descriptor or thunk table | `partial`, `PE_MALFORMED_DELAY_IMPORTS` |
| D-010 | Delay-load dependency duplicates normal import | One unique dependency summary record; both import records retained |

## 7. RT_VERSION resource fixtures

Build the resource directory tree and RT_VERSION payload as synthetic bytes. Include correctly aligned UTF-16 strings and explicit block lengths.

| ID | Fixture | Expected result |
|---|---|---|
| V-001 | No RT_VERSION resource | `version_info: null`, warning `PE_VERSION_RESOURCE_MISSING`; overall status remains `ok` |
| V-002 | Valid VS_VERSION_INFO fixed block | Decode fixed file/product versions and flags |
| V-003 | One StringFileInfo table | Decode every key/value pair and preserve language/code-page IDs |
| V-004 | Multiple language/code-page tables | Sort tables by language ID then code page |
| V-005 | Translation block present | Report explicit translation values |
| V-006 | Translation block absent | Do not infer or fabricate translations |
| V-007 | Block length shorter than fixed header | `partial`, `PE_VERSION_RESOURCE_MALFORMED` |
| V-008 | Child block extends past parent | `partial`, `PE_VERSION_RESOURCE_MALFORMED` |
| V-009 | Misaligned child/value boundary | `partial`; bounded diagnostic |
| V-010 | Invalid UTF-16 or missing terminator | `partial`; escaped/replacement representation must be deterministic and diagnostic |
| V-011 | Excessive block count/depth/string bytes | `partial`, `PE_LIMIT_EXCEEDED` |
| V-012 | Malformed version resource plus valid imports | Preserve valid imports and headers; version failure must not erase independent fields |

## 8. File I/O and SHA-256

| ID | Input | Expected result |
|---|---|---|
| FIO-001 | Empty file | SHA-256 equals the standard empty-input known answer; PE status is `error` |
| FIO-002 | ASCII fixture bytes `abc` | SHA-256 equals `ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad` |
| FIO-003 | Multi-chunk generated file | Digest matches an independent test-library SHA-256 implementation |
| FIO-004 | File exactly at configured maximum | Accepted for reading |
| FIO-005 | File one byte over configured maximum | `error`, `PE_TOO_LARGE`; do not hash/parse beyond policy |
| FIO-006 | Missing path | `error`, `PE_NOT_FOUND`; CLI exit 3 if I/O completion is prevented |
| FIO-007 | Path names a directory in single-file mode | `error`, `PE_IO_ERROR`; no crash |
| FIO-008 | Output path aliases input path (including normalized aliases) | Refuse with `PE_OUTPUT_CONFLICT`; input bytes and metadata unchanged |
| FIO-009 | Output destination cannot be created/written | CLI exit 3; no partial success claim |
| FIO-010 | File changes while being read | Must not produce a misleading successful digest/report; detect where supported or fail safely and document platform limitations |

Do not make permission-denied tests depend on running as an unprivileged account; those tests are environment-sensitive. Use deterministic missing-path, directory-as-file, and controlled output-failure cases in CI.

## 9. CLI, JSON schema, and deterministic behavior

| ID | Test | Expected result |
|---|---|---|
| CLI-001 | Valid `inspect` JSON invocation | Exit 0; one valid schema-v1 object |
| CLI-002 | Valid `inspect` text invocation | Exit 0; stable readable report |
| CLI-003 | Invalid command/option/value | Exit 1; concise usage diagnostic |
| CLI-004 | Malformed PE | Exit 2; error report still emitted when serialization is possible |
| CLI-005 | I/O failure preventing completion | Exit 3 |
| CLI-006 | Directory with valid PE files | JSONL contains one object per candidate in deterministic path order |
| CLI-007 | Directory with mixed valid PE and non-PE files | One report per candidate; aggregate exit 2 |
| CLI-008 | Default include filter | Only `.dll` and `.exe` candidates are inspected |
| CLI-009 | Custom include allowlist | Only matching patterns are inspected |
| CLI-010 | Symlink to file/directory | Not followed; directory mode reports `PE_SYMLINK_SKIPPED` |
| CLI-011 | Directory candidate count over limit | Stop safely; `PE_LIMIT_EXCEEDED`; exit 2 |
| CLI-012 | Identical input bytes/options/tool version run twice | Byte-for-byte identical JSON output |
| CLI-013 | Different current working directories | Output identical except for the documented display path |
| CLI-014 | Output file option | Output content equals stdout-mode report; input remains unchanged |
| CLI-015 | Non-UTF-8 filename/name where platform permits | Deterministic escaped representation and `PE_NON_UTF8_NAME` |
| CLI-016 | Diagnostics exceed configured maximum | Stop diagnostic collection at limit and emit `PE_LIMIT_EXCEEDED` without unbounded growth |

Validate every JSON report against `docs/schemas/inspector-v1.schema.json`. Add explicit assertions for the exact top-level key order, required null fields, integer-vs-hex-string types, stable diagnostic codes, and absence of timestamps/host/current-directory metadata.

A byte-for-byte determinism test must use the same input bytes, executable build/version, and options. Cross-build byte identity is not required unless the tool version is identical.

## 10. Mutation, truncation, and resource-bound tests

- For each baseline fixture, run a truncation sweep at every byte boundary for small fixtures (or a deterministic representative range for larger fixtures).
- Retain the existing Rust parser's deterministic mutation corpus and 300,000-iteration test; do not duplicate it in the CLI suite without a demonstrated coverage gap.
- Add a deterministic inspector-level mutation corpus with a fixed seed and recorded iteration count.
- Mutations must cover DOS offsets, signatures, optional-header sizes, directory counts, section offsets/sizes, import descriptor pointers, thunk terminators, resource lengths, and UTF-16 block boundaries.
- Every invocation has a hard timeout. The test must fail on timeout, signal termination, sanitizer finding, unexpected stderr panic, or invalid JSON.
- Assert that diagnostics, symbols, resource blocks, and directory candidates never exceed configured limits.
- Run sanitizer-enabled native tests where already supported by CI; do not add a new sanitizer toolchain solely for this task.
- Keep a minimized regression fixture for every discovered crash, hang, overflow, or incorrect report.

The test must assert bounded behavior, not a guessed universal memory number. Record the configured file/count limits and the CI timeout used for each test.

## 11. Read-only and no-execution assertions

The following are mandatory:

1. Hash the input before and after inspection; values must match.
2. Record file size and modification time before and after; neither may change.
3. Use a fixture whose bytes are not executable code and verify it is only read as data.
4. Review the inspector's process-spawn and dynamic-loading dependencies; it must not call process creation, `dlopen`/equivalent, loader mapping, or execution APIs.
5. Verify no network library or network request path is introduced.
6. Verify `widen_il_only`, `LoadedImage`, relocation, and execution-oriented APIs are not referenced by inspector code.
7. Ensure the output path cannot overwrite any input, including directory-mode candidates.
8. Include a source-level allowlist/review of filesystem operations; opening inputs read-only is the only permitted input operation.

These assertions complement code review; they do not claim to prove the absence of every possible side effect in all operating systems.

## 12. CI integration and test organization

Follow existing repository organization and test registration. Prefer adding tests alongside the existing PE parser tests and a dedicated inspector test target/script only when the implementation introduces a distinct executable.

- Rust parser tests remain under `native/peparse/`.
- C++ wrapper contracts remain in the existing native PE test target.
- Inspector-specific unit/CLI tests must have stable names and run under the repository's existing CMake/CTest workflow.
- Schema validation may use an already-installed CI dependency; if a new dependency is required, document it and obtain approval before adding it.
- No new network downloads or proprietary fixture assets.
- Test output must identify the fixture ID and expected/actual diagnostic code on failure.

## 13. Specification questions to resolve before implementation

The following cases are not fully specified by the current M0.2 contract and must not be silently guessed:

1. **Overlapping sections:** specify whether overlaps are warnings, partial errors, or accepted descriptive metadata. Define whether virtual and raw overlaps are treated differently.
2. **Zero-section images:** confirm whether a structurally valid PE with zero sections is accepted as `ok` or treated as partial/invalid.
3. **Input changing during inspection:** choose a portable consistency policy (e.g. metadata recheck, stable open-handle snapshot where available, or documented best-effort).
4. **Unreadable paths:** define behavior where the OS does not reliably expose permission failures (especially under elevated CI users).
5. **Missing version resource:** the specification currently says warning only; confirm whether this is correct for ordinary DLL/EXE inputs (it is expected to be common and should not fail inspection).

These are narrow contract clarifications, not permission to expand the inspector. Record any changed behavior in the specification before implementation.

## 14. M0.3 acceptance criteria

M0.3 is complete when:

- [ ] This test plan is committed and linked from `docs/README.md`.
- [ ] `docs/STATUS.md` records M0.3 complete and M0.4 as next.
- [ ] Every required M0.2 capability has at least one fixture and an expected result.
- [ ] PE32/PE32+, x86/x64/ARM/ARM64/unknown machine handling is covered.
- [ ] Imports, delay imports, RT_VERSION, dependency classification, and SHA-256 have positive and negative cases.
- [ ] Malformed/truncated inputs, count/depth limits, deterministic mutations, and timeouts are covered.
- [ ] Schema validation, deterministic output, directory aggregation, symlink behavior, and output/input alias protection are covered.
- [ ] No Microsoft binaries, proprietary packages, or compiler-generated fixtures are committed.
- [ ] Open contract questions are explicitly listed and resolved or carried as a pre-implementation gate.
- [ ] The documentation CI run is recorded only after it completes.

M0.3 defines tests; it does not implement the inspector. M0.4 begins only after the open contract questions in section 13 are resolved and the test plan is committed.
