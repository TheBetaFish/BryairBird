# M0.2 — Read-only PE inspector specification

**Status:** specification draft for implementation planning  
**Date:** 2026-11-20  
**Architecture decision:** Decision 0003, approved by user  
**Scope:** M0 evidence and PE inventory tooling

## 1. Purpose

Define a deterministic, read-only PE inventory tool for examining executable files and DLLs before package/runtime work. The tool reuses Raven's existing Rust PE parser in `native/peparse/` through its existing C ABI and C++ wrapper. It must not introduce a second PE decoder.

The inspector is an evidence tool. A successful parse establishes only that the supported structures could be read; it does not establish that a binary is safe, trusted, loadable, compatible, correctly signed, or entitled to run.

## 2. Relationship to existing documents

- This document operationalizes M0.2 in `docs/EXECUTION_PLAN.md`.
- It supersedes the PE-inspector scope, output assumptions, and proposed CLI details in `docs/m0-feasibility-audit.md` only where those details overlap. The feasibility audit remains authoritative for its broader Raven/UWP and Hello Neighbor findings, including the statement that the actual target package has not been inspected.
- `docs/m0-inspection-report.md` remains the source-inspection record for existing parser behavior and gaps.
- Decision 0003 governs the parser reuse architecture.
- This specification does not change the M0–M5 milestone schedule or authorize target-specific runtime implementation.

## 3. Non-goals and security boundary

The inspector MUST:

- Never execute, load, map for execution, or invoke inspected files.
- Never modify input files or their metadata.
- Perform no network access.
- Perform no package dependency resolution; that belongs to M1.
- Perform no Authenticode, catalog, Store, or package signature verification.
- Not parse .NET metadata beyond reporting the CLR directory and the IL-only flag when readable.
- Not claim that a parsed binary is safe or trusted.
- Treat all file contents, names, offsets, lengths, counts, and resource trees as hostile input.

The inspector MUST NOT call `widen_il_only`, loader mapping/relocation APIs, or any API that mutates or prepares an image for execution. Input buffers are immutable for the entire inspection.

## 4. Architecture

### 4.1 Layers

1. **CLI and filesystem layer:** argument parsing, path handling, directory traversal, symlink policy, file-size checks, and output destination.
2. **File digest layer:** streaming SHA-256 over the input bytes.
3. **PE decoding layer:** the existing Rust parser in `native/peparse/`, extended only for bounded read-only fields required here.
4. **Resource decoding layer:** bounded RT_VERSION decoding built on the parser's generic resource traversal.
5. **Report model and serializer:** versioned internal report model, deterministic JSON and human-readable text output.
6. **Exit policy:** aggregate status and process exit code.

The Rust parser remains authoritative for PE structure. The front end must not reimplement DOS/NT header, section, RVA, import, export, or delay-import decoding.

### 4.2 Ownership and lifetime

The existing `PEImage` uses a non-owning byte span. The front end must keep the backing file buffer alive and immutable until all parser and resource-decoder work has completed. No view, string, or resource pointer may outlive that buffer.

### 4.3 Architecture handling

Recognize and report these COFF machine values:

| Machine | Value | Reported architecture |
|---|---:|---|
| IMAGE_FILE_MACHINE_I386 | 0x014c | x86 |
| IMAGE_FILE_MACHINE_AMD64 | 0x8664 | x64 |
| IMAGE_FILE_MACHINE_ARM | 0x01c4 | arm |
| IMAGE_FILE_MACHINE_ARM64 | 0xaa64 | arm64 |

Unknown machine values are not automatically fatal if the DOS header, NT signature, COFF header, and optional header can otherwise be read safely. Preserve the numeric machine value, report architecture as `unknown`, and emit `PE_UNKNOWN_MACHINE`. Do not infer architecture from optional-header magic or filename.

PE32 and PE32+ optional-header formats must be distinguished independently of machine name. Unsupported machine/optional-header combinations are reported as warnings when safely readable; never reinterpret fields using a guessed layout.

## 5. CLI contract

The exact executable name and option parser should follow the repository's existing native-tool conventions discovered in M0.1. The command interface is:

```text
inspector inspect <path> [--format json|text] [--output <file>] [--max-size <bytes>]
inspector inspect-dir <dir> [--recursive] [--include '*.dll,*.exe'] [--format jsonl]
```

- `inspect` handles one file.
- `inspect-dir` emits one JSON object per candidate file (JSON Lines); deterministic path ordering is required.
- Default output is standard output.
- `--output` writes the report to the named destination. The tool must not overwrite an input path; refuse if output resolves to the same file as any input.
- `--max-size` sets the maximum accepted input size for single-file inspection. It must be a positive integer within the implementation's representable range. The default limit is 512 MiB.
- Directory mode uses the same per-file default limit. A later explicit directory-wide limit may be added only through a separate decision.
- The include pattern is a comma-separated allowlist of filename globs; default is `*.dll,*.exe`. Matching is case-insensitive for extensions on case-insensitive platforms and must not broaden to arbitrary files.
- Symlinks are not followed by default. Symlink entries may be skipped with a warning in directory mode. No path traversal outside the requested root through symlinks is permitted.
- Directory traversal is deterministic: sort normalized relative paths using bytewise UTF-8 ordering after safe path conversion. Invalid path encodings are escaped and reported; do not silently drop entries.
- The tool must not change the current working directory.

## 6. Exit codes and aggregate status

| Exit | Meaning |
|---:|---|
| 0 | All requested inputs were successfully parsed; warnings may be present. |
| 1 | CLI usage or argument validation error. |
| 2 | At least one input is not a valid/supported PE or could not be fully parsed; reports are still emitted for inputs that can be reported. |
| 3 | Filesystem or output I/O error prevented completion. |

For a single file, `status` is:
- `ok`: required PE structures parsed and no truncation occurred; warnings may still be present.
- `partial`: basic PE identity/header is readable but one or more optional structures could not be fully decoded.
- `error`: no valid PE report can be constructed, or an I/O error prevents inspection.

For directory mode, emit one report per candidate. Exit 2 if any candidate has `partial` or `error`; exit 3 if traversal/output I/O prevents completion. Invalid non-PE files still receive a report with status `error` when a report can be serialized.

## 7. JSON schema v1

Publish the machine-readable schema at `docs/schemas/inspector-v1.schema.json`. The schema file is normative for JSON validation and is indexed in `docs/README.md`. The schema must be validated as JSON before every change; schema-v1 producers and tests must validate reports against it. Top-level keys, in this exact order, are:

1. `schema_version`
2. `tool`
3. `input`
4. `status`
5. `pe`
6. `imports`
7. `delay_imports`
8. `version_info`
9. `dependencies`
10. `warnings`
11. `errors`

### 7.1 Field requirements

- `schema_version`: string `"1"`.
- `tool`: object containing tool name and version. The version must come from the build's declared project version; do not invent a runtime git hash.
- `input`: object with display path, `size_bytes` integer, and lowercase hexadecimal `sha256`.
- `status`: one of `ok`, `partial`, `error`.
- `pe`: null if no valid PE header is established; otherwise contains:
  - `machine` numeric COFF machine value and `architecture` string.
  - `format` (`PE32` or `PE32+`).
  - `coff_header`: machine, section count, timestamp, symbol-table pointer/count, optional-header size, characteristics.
  - `optional_header`: magic, linker version, code/data sizes, entry RVA, image base, section/file alignment, OS/image/subsystem versions, image/header sizes, checksum, subsystem, DLL characteristics, stack/heap reserve/commit, loader flags, directory count.
  - `data_directories`: ordered records containing index/name, RVA, and size.
  - `sections`: file-order records containing name, virtual size/address, raw size/pointer, relocation/line-number pointers/counts, and characteristics.
  - `clr`: null if absent; otherwise directory RVA/size and readable IL-only flag.
- `imports`: ordered classic import descriptors, preserving descriptor and thunk order. Each DLL record contains original name, normalized lowercase name, and symbols. Each symbol contains either name plus hint or ordinal; include IAT RVA when available. Preserve original spelling and bytes safely.
- `delay_imports`: same DLL/symbol representation, plus descriptor attributes and whether RVA-based or legacy VA-based fields were used.
- `version_info`: null when no usable RT_VERSION resource exists; otherwise fixed-file information and every decoded StringFileInfo table, preserving language/code-page identifiers and key/value strings.
- `dependencies`: ordered unique direct and delay-load DLL names. Each record preserves original and normalized names and has kind `api_set` or `named_dll`. API-set classification is lexical only (case-insensitive `api-ms-win-` or `ext-ms-win-` prefix); it does not resolve contracts.
- `warnings` and `errors`: arrays of stable diagnostic objects with `code`, `message`, and optional `location`. Location values are structured where possible (file offset, RVA, directory index, section index, resource path, or import descriptor index). Messages are explanatory and are not stable API; codes are.

### 7.2 Serialization rules

- Preserve the exact top-level key order above.
- Preserve import descriptor and thunk order as found in the file.
- Preserve section order as found in the file.
- Sort dependency records by normalized name, then original name.
- Sort version string tables by language ID then code page; preserve string order within a table as encountered.
- Sort warning/error diagnostics by input location, then code; diagnostics without a location sort after located diagnostics.
- Encode RVAs, file offsets, image bases, and other address-like values as lowercase `0x`-prefixed hexadecimal strings. Numeric sizes/counts remain JSON integers.
- Use JSON null for absent optional structures; do not omit required fields and do not fabricate data.
- Strings must be valid UTF-8 JSON. Invalid source bytes must be represented with a deterministic escaped form and accompanied by `PE_NON_UTF8_NAME` or the relevant string diagnostic.
- Do not include timestamps for inspection time, host identity, current directory, or other nondeterministic environment data.
- JSON output must be byte-for-byte deterministic for identical input bytes, tool version, and options.

## 8. PE field coverage

### 8.1 Headers and sections

Read DOS header, NT signature, COFF header, and PE32/PE32+ optional header with checked bounds. Report all fields listed in section 7.1 when present and valid.

Data-directory count is bounded by both the optional-header size and the implementation maximum. Do not read entries beyond either bound. A declared directory count larger than the available entries produces `PE_TRUNCATED_DIRECTORY_TABLE`.

Section table bounds and every raw-data range must be checked for integer overflow and file containment. Virtual ranges are descriptive; do not map or execute sections.

### 8.2 Imports

Support normal import descriptors, named imports, ordinal imports, PE32 and PE32+ thunk widths, multiple DLLs, and IAT fallback when OriginalFirstThunk is zero. Preserve import order. An unmapped or truncated import name/table produces a partial report and a stable warning/error, never a guessed name.

### 8.3 Delay imports

Support the descriptor encodings already handled by Raven's parser: RVA-based descriptors and legacy VA-based descriptors for PE32/PE32+. Validate every VA-to-RVA conversion against the image base and bounds. Unsupported or ambiguous descriptor forms produce `PE_UNSUPPORTED_DELAY_IMPORT_FORM`; do not guess.

### 8.4 Version resources

Decode RT_VERSION resources using bounded traversal of the existing generic resource tree. Support VS_VERSION_INFO fixed file information and all StringFileInfo language/code-page tables. Validate block lengths, alignment, value lengths, child boundaries, and UTF-16 strings. A malformed block must not invalidate otherwise readable PE headers/imports; mark status `partial` and emit a version-resource diagnostic. Translation tables may be reported if present, but must not be inferred when absent.

### 8.5 Dependencies

List only direct import and delay-load DLL names. Classify API-set contract names lexically as specified above. No search path resolution, KnownDLL lookup, API-set schema lookup, framework package mapping, or filesystem probing is part of M0.

## 9. Resource and work limits

Limits are mandatory and must be documented in CLI help and report metadata or diagnostics:

| Limit | Default |
|---|---:|
| Maximum input file size | 512 MiB |
| Maximum directory candidate files | 20,000 |
| Maximum import descriptors per image | 4,096 |
| Maximum imported symbols per image | 100,000 |
| Maximum decoded string bytes | 4,096 |
| Maximum resource-tree depth | 8 |
| Maximum RT_VERSION block count | 65,536 |
| Maximum diagnostics per file | 1,024 |

When a limit is reached, stop the affected traversal safely, emit `PE_LIMIT_EXCEEDED`, and mark the report `partial`. Never silently truncate. SHA-256 must be streamed in bounded chunks; do not require a second full-file copy for hashing.

The implementation may choose a lower configurable limit only if documented. Raising a default requires a decision record and updated tests.

## 10. Stable diagnostic codes

The initial v1 code set is:

| Code | Meaning |
|---|---|
| `PE_NOT_FOUND` | Input path does not exist. |
| `PE_IO_ERROR` | Read or filesystem operation failed. |
| `PE_TOO_LARGE` | Input exceeds configured maximum size. |
| `PE_EMPTY_FILE` | Input is empty. |
| `PE_BAD_DOS_SIGNATURE` | DOS signature is absent/invalid. |
| `PE_BAD_NT_SIGNATURE` | NT signature is absent/invalid. |
| `PE_TRUNCATED_HEADER` | Required header bytes are missing. |
| `PE_TRUNCATED_DIRECTORY_TABLE` | Data-directory table is shorter than declared. |
| `PE_INVALID_OPTIONAL_HEADER` | Optional-header magic/size is invalid or unsupported. |
| `PE_UNKNOWN_MACHINE` | COFF machine value is not recognized. |
| `PE_MACHINE_FORMAT_MISMATCH` | Recognized machine and optional-header format combination is unsupported. |
| `PE_INVALID_SECTION_TABLE` | Section table is malformed or out of bounds. |
| `PE_INVALID_SECTION_RANGE` | Section raw range is outside the file or overflows. |
| `PE_UNMAPPED_IMPORT_RVA` | Import RVA cannot be safely mapped to file bytes. |
| `PE_MALFORMED_IMPORTS` | Import descriptors or thunks are malformed. |
| `PE_UNSUPPORTED_DELAY_IMPORT_FORM` | Delay-import descriptor form is unsupported/ambiguous. |
| `PE_MALFORMED_DELAY_IMPORTS` | Delay-import descriptors or thunks are malformed. |
| `PE_VERSION_RESOURCE_MISSING` | No RT_VERSION resource was found. |
| `PE_VERSION_RESOURCE_MALFORMED` | RT_VERSION data is malformed. |
| `PE_NON_UTF8_NAME` | A source name is not valid UTF-8 and was escaped. |
| `PE_LIMIT_EXCEEDED` | A configured count/depth/string/diagnostic limit was reached. |
| `PE_CLR_HEADER_PRESENT` | A CLR header is present; metadata is not parsed. |
| `PE_SYMLINK_SKIPPED` | Directory entry was a symlink and was not followed. |
| `PE_OUTPUT_CONFLICT` | Output path aliases an input path. |

Codes are stable within schema v1. New codes may be added without changing existing meanings. A breaking change to field shape or semantics requires schema version 2.

## 11. Failure isolation and partial reports

- Header failure before a valid PE identity is established yields `pe: null`, `status: error`, and a diagnostic.
- A valid PE header with malformed imports, delay imports, sections, or resources retains all independently decoded fields and yields `partial`.
- A missing version resource is informational and does not by itself make a report partial; emit `PE_VERSION_RESOURCE_MISSING` as a warning.
- I/O failure before the file can be fully read yields an error report when enough input metadata is available; process exit code is 3 if completion is prevented.
- Do not stop parsing independent directories merely because one directory is malformed, unless shared bounds state makes further reads unsafe.

## 12.1 Pre-implementation contract clarifications

The M0.3 test plan identified edge cases not fully decided in the initial draft. The following policies are normative for schema v1:

1. **Overlapping sections:** if raw-file or virtual-address ranges overlap, emit `PE_INVALID_SECTION_TABLE`, retain independently readable metadata, and mark the report `partial`. Do not choose one section arbitrarily for RVA mapping. Any RVA whose mapping is ambiguous is treated as unmapped.
2. **Zero-section images:** a structurally valid PE header with zero sections is reportable as `ok` when no requested directory/resource decoding requires section-backed bytes. Report an empty section list. A nonzero directory that cannot be mapped remains a partial-report condition.
3. **Input mutation during inspection:** open the input once for read-only access, capture file identity/size/modification metadata before reading, read the bounded contents into the immutable parser buffer, and recheck metadata after reading. If detectable metadata changes, emit `PE_IO_ERROR` and do not report a successful inspection. Compute SHA-256 incrementally over that same immutable buffer so the digest and parser input are identical. This is best-effort detection; platforms/filesystems that permit content changes without observable metadata changes cannot be guaranteed to provide a snapshot.
4. **Unreadable paths:** map permission and other open/read failures to `PE_IO_ERROR`. Do not depend on a permission-specific test succeeding under elevated CI identities. If the file cannot be read completely, process exit is 3.
5. **Missing RT_VERSION:** retain `PE_VERSION_RESOURCE_MISSING` as informational warning only. Absence does not make an otherwise valid ordinary EXE/DLL partial.

## 12. Acceptance criteria for M0.2

M0.2 is complete when:

1. This specification is committed and indexed from the documentation index.
2. The schema v1 key set and field types are unambiguous enough to write a JSON Schema.
3. Machine handling, version-resource behavior, limits, deterministic ordering, symlink behavior, and exit codes are specified.
4. The implementation boundary reuses `native/peparse/` and does not introduce a second PE decoder.
5. All unsupported behavior is surfaced as warnings/errors rather than guessed.
6. `docs/STATUS.md` records M0.2 complete and M0.3 as the next task.
7. CI result is recorded only after the corresponding GitHub Actions run completes.

M0.2 does not implement the inspector. M0.3 must define fixtures and expected results before code changes begin.
