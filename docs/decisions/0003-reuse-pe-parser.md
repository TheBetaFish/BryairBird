# Decision 0003 — Reuse Raven's existing PE parser

**Status:** proposed  
**Date:** 2026-11-20  
**Scope:** M0 inspector architecture

## Context

M0.1 inspected Raven's existing PE reader in `native/peparse/`, its C ABI and C++ wrapper, loader/module/resource consumers, tests, and CI integration. The parser already performs checked PE32/PE32+ header, section, RVA, import, delay-import, export, and resource parsing. It has synthetic fixtures and a deterministic 300,000-iteration mutation test.

The planned inspector additionally needs ARM64 and unknown-machine header reporting, RT_VERSION decoding, streaming file hashing, directory traversal, deterministic JSON/text output, and stable diagnostic codes. Those capabilities are not all present today.

## Options

1. **Reuse and extend the existing parser; build a separate inventory front end.**
   - Keep PE decoding in Rust.
   - Extend machine-header handling and version-resource support only as required by the approved spec.
   - Expose new read-only fields through the existing C ABI and C++ wrapper.
   - Keep file I/O, hashing, directory traversal, output serialization, and CLI policy outside the parser.
2. **Write a standalone second PE parser.**
   - Duplicates security-sensitive parsing and risks disagreements with Raven's loader.
3. **Use the runtime loader/debug CLI as the entire inspector.**
   - Existing inspection switches are useful for regression comparisons but do not provide the planned stable schema, directory mode, or non-mutating report contract.

## Decision

**Recommended option: 1 — reuse and extend the existing parser, with a separate read-only inventory front end.**

The existing parser is the authoritative source for PE structure. Do not duplicate PE decoding. Do not call `widen_il_only` or any image-loading/mutation API from the inspector. Keep the input immutable.

## Consequences

- Existing parser behavior and tests become the baseline contract.
- ARM64 and unknown-machine handling must be specified explicitly; the current parser rejects them.
- RT_VERSION decoding is a new bounded resource consumer.
- Hashing, file access, directory traversal, JSON schema, warning/error codes, and CLI exit policy are separate layers.
- Add regression fixtures for every parser extension and preserve the existing loader behavior.
- No Windows DLLs, package payloads, or other proprietary binaries are needed in source control.
- The alternative standalone parser is rejected unless future evidence proves the existing parser cannot safely support the required read-only inspection contract.

## Documents affected

- `docs/m0-inspection-report.md`
- `docs/m0-inspector-spec.md` (to be written in M0.2 after decision approval)
- `docs/m0-test-plan.md` (to be written in M0.3)
- `docs/EXECUTION_PLAN.md`
- `docs/STATUS.md`

## Approval

This recommendation is **proposed**, not approved. Do not implement parser changes or the inspector until the user explicitly approves this decision.
