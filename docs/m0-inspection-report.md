# M0.1 — PE parser and test infrastructure inspection

**Date:** 2026-11-20  
**Task:** 0.1 — Inspection report (documentation only)  
**Repository ref inspected:** `main` at `ecffafeaf1abc070548e310a2a15e58e304548dd`  
**Baseline:** [CI run 44761707010, attempt 2](https://github.com/TheBetaFish/BryairBird/actions/runs/44761707010) — PASS  
**Code changes:** None.

## 1. PE parser and loader

### Implementation and public API

| Layer | Path | Important symbols / role |
|---|---|---|
| Rust parser | `native/peparse/src/lib.rs` | `Image::parse`, `Image::file_span`, `Image::text_at`, `Image::imports`, `Image::delay_imports`, `Image::exports`, `Image::resources`, `Image::il_only`, `read_resources`, `widen_il_only` |
| Rust/C ABI | `native/peparse/src/ffi.rs` | `raven_pe_parse`, `raven_pe_file_span`, `raven_pe_text_at`, `raven_pe_imports`, `raven_pe_delay_imports`, `raven_pe_exports`, `raven_pe_resources`, `raven_pe_widen_il_only` |
| C ABI contract | `native/peparse/peparse.h` | `raven_pe_headers`, `raven_pe_section`, directory/import/delay/export/resource structs and callback types |
| C++ wrapper | `native/pe/pe.h`, `native/pe/pe.cpp` | `raven::PEImage`, `PEImport`, `PEDelayImport`, `PEExport`; translates parser refusal strings to `std::runtime_error` and copies callback data into C++ containers |
| Image mapping | `native/pe/loaded_image.h`, `native/pe/loaded_image.cpp` | `raven::LoadedImage`; maps image sections and applies relocations |
| Module binding | `native/pe/module_set.h`, `native/pe/module_set.cpp` | `raven::ModuleSet`; binds imports and resolves exports/forwarders |
| Resources | `native/pe/resources.h`, `native/pe/resources.cpp` | `PEResource`, `read_resources`; adapts Rust resource records |
| Build integration | `native/CMakeLists.txt`, `native/peparse/Cargo.toml` | Cargo builds the Rust static library; CMake builds `raven_pe` and links it into Raven |

The parser is Rust 2021, exposed through a C ABI and consumed by C++20. `native/peparse/Cargo.toml` declares a private, unpublished staticlib/rlib with no third-party Rust dependencies listed. CMake requires Cargo and builds `libraven_peparse.a` in the build tree.

### Existing coverage

- DOS and PE signatures; COFF and PE32/PE32+ optional headers; image dimensions, alignments, directories, and section table.
- File-backed RVA mapping and bounded, NUL-terminated strings.
- Classic imports, named and ordinal thunks, and lookup-table fallback to IAT.
- Delay-load descriptors, named/ordinal thunks, RVA-based and PE32 VA-based descriptor forms.
- Exports, ordinal/name tables, and forwarded exports.
- Resource directory traversal with named/ID keys, language, data ranges, and caller-supplied record-size accounting.
- CLR data-directory presence is used for IL-only detection; the parser does not decode .NET metadata.
- Separate image loader covers section mapping, relocations, and image permissions.

### Validation and error handling

The Rust parser uses checked range helpers (`range`, `read_le`, `subslice`) and returns static error strings rather than reading beyond the backing slice. It rejects invalid alignments, image dimensions, section counts, out-of-range/overlapping sections, malformed import/export/delay tables, and invalid resource trees. Import/export/resource enumeration uses callbacks that can stop traversal. The C++ callback adapter captures exceptions so they do not unwind through Rust frames.

The parser has explicit limits in several table walkers (for example, delay descriptors/imports, exports, and resource traversal). The existing limits and their exact semantics must be carried into the inspector specification rather than assumed to match the proposed inspector limits.

### Important scope gaps and assumptions

- **Machine support is narrower than the inspector target.** `Image::parse` currently accepts x86 (`0x14c`) and x64 (`0x8664`), plus a special PE32+ x86 IL-only case. ARM64 and unknown machine types are refused as inconsistent/unsupported. The planned inspector requires ARM64 support and readable headers for unknown machines; this needs a deliberate parser/API change or a clearly scoped compatibility layer.
- **PE32+ x86 is a special .NET any-CPU rewrite path**, not general architecture support. `widen_il_only` mutates bytes; an inspection tool must never call it on its input.
- Version-resource extraction is **not found** in the parser API or `raven::PEImage`; only generic resource traversal exists. RT_VERSION parsing and StringFileInfo decoding would be new functionality.
- CLR support is limited to checking the CLR directory's IL-only flag. No metadata tables, assembly references, or runtime requirements are decoded.
- The parser accepts a byte slice already resident in memory. File opening, streaming hashing, directory traversal, JSON serialization, and CLI exit policy are outside this parser.
- The parser reports errors by static text, not a structured warning/error code taxonomy. The inspector must define its own stable external diagnostics.
- `PEImage` holds a non-owning span; the source bytes must outlive the object and every view derived from it.
- The parser validates the PE image and its readable tables, not Authenticode trust, package signatures, or dependency availability.

## 2. Test infrastructure

| Area | Path | Findings |
|---|---|---|
| Rust parser tests | `native/peparse/tests/mutations.rs` | Builds a synthetic PE32+ image in Rust; tests imports, exports, delay imports, resources, IL-only flag, stop callbacks, malformed inputs, and 300,000 deterministic mutation iterations with truncation reads |
| C++ PE contracts | `native/pe/pe_test.cpp` | Builds PE32 and PE32+ fixtures directly in test code; checks headers, imports, IAT fallback, delay imports, invalid inputs, image mapping and relocations |
| Module graph tests | `native/pe/module_set_test.cpp` | Synthetic x86/x64 modules, imports, exports, forwarders, relocation and binding behavior |
| CMake registration | `native/CMakeLists.txt` | `pe_test` registered as CTest `pe_contracts`; module tests as `module_set_contracts`; each has a 10-second timeout |
| CI parser validation | `.github/workflows/ci.yml` | Models job runs `cargo test --locked --release` in `native/peparse`; full build and six CTest shards run after component provisioning |
| Fixture provisioning | `scripts/provision-fixtures.py`, `testdata/native_loader/` | Build-time generated/downloaded test inputs live under `RAVEN_TESTDATA_DIR` in the build tree; parser unit fixtures are generated in test source, not stored as binary files |
| PE regression comparator | `scripts/native-pe-regression.py` | Uses argparse, invokes Raven and LLVM tools, compares imports/headers/exports/loaded image behavior, and emits JSON results |

### Single-test commands

From a configured build tree:
- `ctest --test-dir <build-dir> -R '^pe_contracts$' --output-on-failure`
- `ctest --test-dir <build-dir> -R '^module_set_contracts$' --output-on-failure`

Rust parser:
- From `native/peparse/`: `cargo test --locked --release`
- One Rust test: `cargo test --locked --release survives_random_changes` (or another exact test name).

The CI workflow's `Models` job runs the Rust parser test command. CTest's `pe_contracts` and `module_set_contracts` are included in the six sharded test jobs.

## 3. Tool conventions

- Existing Python tools live in `scripts/`; Python utilities generally use the standard library and `argparse` where a CLI is needed.
- `scripts/native-pe-regression.py` is the closest PE-specific precedent: argparse, subprocess invocation with timeouts and checked exit status, explicit output validation, SHA-256 evidence, and JSON result objects.
- Existing native runtime inspection commands are exposed by the Raven executable (the regression script uses `--inspect-imports`, `--inspect-exports`, and `--dump-image`). They are runtime/debug interfaces, not a complete standalone read-only inventory CLI.
- C++ tests use small synthetic byte vectors and local assertion helpers; Rust tests construct byte vectors with explicit little-endian writes.
- CMake owns native test registration and timeouts. Cargo owns Rust unit tests. CI compiles Python scripts with `py_compile` and parses shell scripts with `bash -n`.
- No repository-wide JSON CLI schema or shared command framework was found in the inspected paths. Do not invent one as an existing convention.

## 4. Reuse assessment

| Capability | Existing implementation | Verdict | Reason |
|---|---|---|---|
| DOS/COFF/optional headers | `native/peparse/src/lib.rs::Image::parse` | Reuse; needs extension | Strong checked parser, but ARM64/unknown-machine policy does not meet inspector requirements |
| PE32/PE32+ sections and RVA mapping | `Image::parse`, `Image::file_span`, `Image::text_at` | Reuse as-is for currently supported architectures | Existing bounds checks and tests; preserve semantics |
| Classic imports | `Image::imports`, `raven::PEImage::imports` | Reuse; add targeted tests | Named/ordinal imports and IAT fallback already exist |
| Delay imports | `Image::delay_imports` | Reuse; needs coverage review | RVA and PE32 VA descriptor forms exist; PE32+ VA form is not covered by current C++ tests |
| Exports | `Image::exports`, `PEImage::exports` | Reuse as-is | Includes ordinals, names, holes, forwarders, validation |
| Generic resources | `Image::resources`, `read_resources` | Reuse as-is for traversal | Bounded resource tree walk exists; version-resource semantics do not |
| Version information | No RT_VERSION parser found | New parser atop generic resources | Must parse VERSIONINFO resource structure and string tables safely |
| CLR metadata | `Image::il_only` | Reuse only for IL-only flag | Does not parse metadata; inspector only needs to flag CLR header presence |
| Streaming SHA-256 | No reusable file-hashing helper identified in inspected PE paths | New small file-I/O layer | Hash input without loading it all into memory |
| Directory walk, symlink policy, CLI | No PE inventory CLI found | New Python or C++ front end | Keep file I/O, limits, serialization and exit policy outside parser |
| JSON schema/output | No shared inspector schema found | New documented output layer | Must be deterministic and versioned |
| Hostile-input tests | `native/peparse/tests/mutations.rs`, `native/pe/pe_test.cpp` | Extend existing tests | Existing synthetic fixtures and deterministic mutation harness are a strong foundation |

## 5. Parser reuse decision

**Recommendation:** reuse the existing Rust parser as the sole PE decoding implementation, expose missing read-only metadata through its existing C ABI/C++ wrapper, and build the inventory CLI/reporting layer around it. Do not write a second standalone PE parser.

Rationale:
1. Raven already has a bounds-checked parser with deterministic mutation coverage and C/C++ integration.
2. A second parser would duplicate a security-sensitive format implementation and create inconsistent interpretations.
3. Existing loader, module, resource, and regression tests provide reusable fixtures and contracts.
4. The gaps are identifiable extensions (machine-type policy, RT_VERSION decoding, streaming/file front end, structured diagnostics), not evidence that the parser is fundamentally unsuitable.

This is a proposed architectural decision, not implementation authorization. See Decision 0003. No code was changed in M0.1.

## 6. Sources inspected

- `native/peparse/peparse.h`
- `native/peparse/src/lib.rs`
- `native/peparse/src/ffi.rs`
- `native/peparse/tests/mutations.rs`
- `native/pe/pe.h`
- `native/pe/pe.cpp`
- `native/pe/loaded_image.h`, `native/pe/loaded_image.cpp`
- `native/pe/module_set.h`, `native/pe/module_set.cpp`
- `native/pe/resources.h`, `native/pe/resources.cpp`
- `native/pe/pe_test.cpp`
- `native/pe/module_set_test.cpp`
- `native/CMakeLists.txt`
- `native/peparse/Cargo.toml`
- `scripts/native-pe-regression.py`
- `scripts/provision-fixtures.py`
- `.github/workflows/ci.yml`
- `docs/EXECUTION_PLAN.md`
- `docs/uwp/05-milestone-roadmap.md`

No claim is made that the entire repository was exhaustively reviewed.
