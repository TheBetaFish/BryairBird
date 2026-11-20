# BryairBird Execution Plan

Companion to the Development and Documentation Rules. The Rules say how to behave; this plan says what to do, in what order, and what counts as done.

## How to use this plan

- This file lives at `docs/EXECUTION_PLAN.md` and is listed in `docs/README.md`.
- Do tasks in order, one at a time. Do not start a task until the previous one has recorded evidence.
- **GATE** means stop, summarize, and wait for the user's explicit approval.
- **PROVISIONAL** means a hypothesis from general Windows knowledge. Confirm it with the M0 tool and real packages before building on it.
- This plan was written from a project brief and then checked against the current repository. It is not a substitute for source inspection. Cite actual paths for claims about Raven.
- The canonical milestone sequence is `docs/uwp/05-milestone-roadmap.md` and is reconciled with `docs/uwp-roadmap.md` by Decision 0001. `docs/EXECUTION_PLAN.md` controls task order and gates. If a genuine technical conflict remains, record it and stop; do not silently deviate.
- The current repository has an enforced commit-message format in `CONTRIBUTING.md` and `.github/scripts/check_commits.py`: `type: summary`. The proposed `type(task): summary` format in the original plan would fail that check. Until an approved decision changes repository policy, use `type: [task-id] summary`.
- Do not assume a particular user machine. Record the actual environment for each verification session.

## Every session

1. Read `docs/STATUS.md`, `docs/README.md`, and the documents governing the current task.
2. Check `git status` and `git log -10 --oneline`. Confirm the branch and working-tree state. Report anything unexpected before starting.
3. State the task ID, intended changes, and governing documents in one paragraph.
4. If the build or tests cannot run in the available environment, say so and use the verification handoff below.
5. Record `uname -a` and `echo $XDG_SESSION_TYPE` once per user verification session. For graphics tasks, also record `vulkaninfo --summary` and `nvidia-smi` when applicable. Do not assume an NVIDIA GPU exists.

## Conventions

### Status file

Maintain `docs/STATUS.md` under 100 lines. It records the current milestone/task, last completed task and commit, open GATEs, open decisions, and known failures.

### Evidence log

Use `docs/evidence/<milestone>.md`, with one entry per task:
- Task ID, date, and commit hash.
- Environment: distro, kernel, toolchain versions, Wayland/X11 session, and GPU/driver for graphics tasks.
- Exact commands, exit codes, and verbatim output (trimmed, never paraphrased).
- Result: PASS, FAIL, or NOT RUN, with the reason.
- Untested areas.

Never invent an environment value, command output, exit code, or test result.

### Decision records

Use `docs/decisions/NNNN-title.md` with context, options, decision, consequences, documents changed, and status (`proposed`, `approved by user`, or `superseded`).

A decision record is required for any deviation from the documentation, added scope, new dependency, or new file format. Proposed decisions are not approval.

### Commits

Follow the repository-enforced `type: summary` format from `CONTRIBUTING.md` and `.github/scripts/check_commits.py`. Include the task ID in the summary, for example `docs: [0.0] add status and evidence scaffolding`.

The commit body explains why, what was verified, and what was not. For substantive AI assistance, include:
`Assisted-by: OpenAI: GPT-6`
`AI-Scope: <specific scope>`

Docs and the code they describe land together. Never write “tests pass” unless they were run and the evidence was logged. If a task grows beyond one session, split it and record the change.

### End of every task

Report: Done, Verified, Not verified, Failures, Deviations, Next.

### Verification handoff

If a check cannot be run in the available environment, finish the documentation work, mark every affected claim NOT RUN, and give the user:
1. Exact commands.
2. What a pass looks like.
3. A paste-back template: task, commit, command, exit code, output.

The task remains **implemented, unverified** until results are returned.

## M0: Evidence, PE inspection tooling, and reproducible fixtures

### 0.0 Baseline and documentation setup

- Check that every document listed in the Development and Documentation Rules exists. List missing documents and contradictions, including differences between `docs/uwp-roadmap.md` and `docs/uwp/05-milestone-roadmap.md`.
- Create `docs/STATUS.md`, `docs/evidence/`, and `docs/decisions/` if missing. Add this plan to the `docs/README.md` index.
- Add `.gitignore` entries, following repository conventions, for local-only material: harvested components, `*.wim`, `*.iso`, `*.appx`, `*.appxbundle`, `*.msix`, `*.msixbundle`, `*.eappx`, `*.emsix`, and a local `local/` directory.
- Use the existing `.github/workflows/ci.yml` as the baseline runner. It already triggers on pushes to `main` and includes the full configured CMake build, Rust parser tests, sanitizer/fuzz checks, and six CTest shards. Record the exact workflow run, commit SHA, job conclusions, and failed/skipped steps in `docs/evidence/m0.md`. Do not ask the user to run the baseline locally. Do not add a duplicate workflow.
- Commit the documentation and ignore setup using the repository's enforced commit format.
- Done when STATUS exists, the canonical milestone schedule is linked, and a GitHub Actions baseline result is recorded. If no workflow run is created or visible, record that as blocked; do not claim a pass or substitute a local run.

### 0.1 Inspection report (no code changes)

Write `docs/m0-inspection-report.md` covering:

1. Raven's PE parser/loader: paths, key functions and types, what it parses (headers, sections, imports, delay imports, resources, exports), assumptions and bounds checks, error handling, language and toolchain.
2. Test infrastructure: runner, test locations, fixture storage, how to run a single test, and CI configuration.
3. Tool conventions: where tools live, argument parsing, and output style.
4. Reuse table: capability, existing code (path and symbol), verdict (as-is, needs hardening, unsuitable), and reason.
5. Decision 0001: reuse the existing parser, wrap it, or write a standalone bounds-checked parser. If existing parsing assumes well-formed input, default to standalone unless hardening is clearly cheap and safe.

Every claim cites a real repository path. If something is not found, write “not found.” No code changes in this task.

### 0.2 Specification

Write or update `docs/m0-inspector-spec.md`. Reconcile it with `docs/m0-feasibility-audit.md` and state which statements it supersedes.

Define:

- Non-goals: never execute or load inspected files; no network; never modify inputs; no dependency resolution (M1); no signature verification; no .NET metadata parsing beyond flagging a CLR header.
- CLI, adapted to conventions discovered in 0.1:
  - `inspect <path> [--format json|text] [--output <file>] [--max-size <bytes>]`
  - `inspect-dir <dir> [--recursive] [--include '*.dll,*.exe'] [--format jsonl]`
  - Symlinks are not followed by default.
- Exit codes: 0 all inputs parsed; 1 usage error; 2 at least one input is not a valid PE (report still emitted); 3 I/O error.
- JSON schema v1 at `docs/schemas/inspector-v1.schema.json`. Top-level keys: `schema_version`, `tool`, `input` (path, size_bytes, sha256), `status` (ok, partial, error), `pe`, `imports`, `delay_imports`, `version_info`, `dependencies`, `warnings`, `errors`.
- Stable key order; import order preserved; other lists sorted; addresses/RVAs are hex strings; sizes are integers; original DLL names are preserved alongside lowercase normalized names; absent data is null, never omitted or fabricated; no run timestamps in reports.
- PE coverage: PE32 and PE32+; x86, x64, ARM64, ARM; unknown machines produce a warning while preserving readable headers. Report file/optional headers, subsystem, image base, entry RVA, image size, DLL characteristics, data directories, and sections.
- Imports: classic descriptors, named and ordinal imports, with IAT fallback when the lookup table is absent.
- Delay imports: document supported descriptor forms; unsupported forms produce warnings, never guesses.
- Version information: RT_VERSION fixed information and all StringFileInfo tables.
- Dependencies: direct and delay-load DLL names tagged as `api_set` or `named_dll`. No dependency resolution.
- Limits: streaming SHA-256; documented maximum DLLs, imports per DLL, string length, and resource depth. Exceeding a limit produces a warning and bounded/truncated output.
- Safety: treat inputs as hostile; check every RVA/size calculation for overflow; no unbounded recursion/loops; escape and flag non-UTF-8 names.
- Finalize warning/error codes, including malformed/truncated PE, unknown machine, unmapped import RVA, unsupported delay form, missing/malformed version resource, limits, and CLR-header presence.

### 0.3 Test plan (before implementation)

The detailed fixture matrix and expected results are maintained in `docs/m0-test-plan.md`. It covers synthetic PE32/PE32+, x86/x64/ARM/ARM64/unknown machines, imports, delay imports, RT_VERSION, SHA-256, malformed/truncated input, limits, deterministic output, directory mode, symlinks, and read-only/no-execution assertions.

Contract clarifications are resolved in section 12.1 of `docs/m0-inspector-spec.md` and section 13 of `docs/m0-test-plan.md`: overlapping sections are partial/ambiguous, zero-section headers are accepted when no section-backed data is required, input mutation is checked using one immutable buffer plus metadata rechecks, unreadable paths map to `PE_IO_ERROR`, and missing RT_VERSION is informational. Any later contradiction requires a specification update before code.

Done when the test plan is committed and indexed, every specified capability has expected fixtures/results, open contract questions are explicit, and the documentation CI result is recorded only after completion. No inspector implementation in this task.

### 0.4 Implementation

One focused commit per step. Tests for a step pass before the next begins. If the specification proves wrong, stop, write a decision record, and correct the specification first.

1. CLI skeleton, JSON writer, error type, exit codes.
2. Bounded reader, RVA mapping, DOS/NT/optional header and section parsing.
3. Streaming SHA-256.
4. Imports.
5. Delay imports.
6. Version resource.
7. Dependency block.
8. Directory mode and JSONL.
9. Mutation sweeps and schema validation.
10. Finalize the spec to match actual behavior, list limitations, and update README, roadmap, and STATUS.

### 0.5 Real-file validation (requires the user)

Write `docs/m0-validation-runbook.md`. The user runs the inspector locally against a small set of real PE files from images they are entitled to use: at least one x64 EXE, one DLL with delay imports, and one WinRT component DLL if available.

Cross-check imports with at least one independent tool (`llvm-readobj --coff-imports`, `objdump -p`, or Python `pefile`). Every discrepancy is either a bug (fix it and add a synthetic regression fixture) or a documented limitation.

Commit only summary evidence (counts and a few sample names), never the binaries or full dumps.

### 0.6 Close-out

M0 is complete only when each item has an evidence entry:

- AC-1 Builds from a clean checkout with the documented command.
- AC-2 Every automated test in the plan is PASS. NOT RUN counts as failure.
- AC-3 PE32 and PE32+ fixtures for x86, x64, and ARM64 produce correct headers and imports.
- AC-4 Malformed input never crashes or hangs; mutation sweeps respect limits.
- AC-5 Output matches the published schema and is deterministic.
- AC-6 Real-file cross-check is complete; discrepancies are resolved or documented.
- AC-7 No proprietary binaries are in the repository; record the `git ls-files` check.
- AC-8 Specification matches behavior and documents limitations.
- AC-9 STATUS, README index, and roadmap are updated with evidence links.
- AC-10 Decision log is complete; no undocumented deviations.

**GATE:** report AC-1 through AC-10 with evidence and wait for the user's sign-off. M1 does not begin before it.

## M1: Prefix setup, component harvesting, and package registration

All items are PROVISIONAL. `docs/uwp/02-prefix-and-harvest.md` governs; list any conflict.

- 1.0 Read the design and M1 inspection documents. Inspect prefix, registry, and package-store code. Write `docs/m1-implementation-plan.md` with concrete tasks and tests. GATE on divergence.
- 1.1 Create the documented prefix tree and versioned metadata. Make it idempotent; refuse to touch a non-empty directory it did not create. Test with temporary directories.
- 1.2 Harvest from a user-supplied directory first. WIM/ISO reading is a later separately documented stage. Never download components. Record provenance per file: source path, image identifier, size, SHA-256, and PE version from M0.
- 1.3 Derive a declarative component manifest (name, architecture, minimum version) from M0 reports and the codebase gap analysis, not guesses.
- 1.4 Harvest command is dry-run by default; `--apply` copies; never overwrites without an explicit flag; writes JSON report. Missing components are reported, never stubbed.
- 1.5 Inspect existing registry behavior. Add only keys traceable to a documented requirement.
- 1.6 Raven's package database remains canonical. Extraction rejects traversal, absolute paths, symlinks, reserved names, case collisions, and excessive size/compression ratios.
- 1.7 Integration tests create a prefix and harvest from a synthetic component directory built with M0 fixtures. Manual real-image validation is run by the user.
- 1.8 Close out with an acceptance checklist and GATE.

## M2: Package identity, WinRT activation, and headless packaged lifecycle

`docs/uwp/03-appmodel-activation.md` and the M1 process/package-identity inspection documents govern.

- 2.0 Read the documents, inspect source, and write `docs/m2-implementation-plan.md`. GATE on divergence.
- 2.1 Parse AppxManifest.xml into a typed model: Identity, Properties, Dependencies, Resources, Applications, Capabilities, and Extensions (including activatable classes). Preserve and report unknown elements.
- 2.2 Derive PackageFullName, PackageFamilyName, and publisher ID using Microsoft's documented algorithm; verify against known pairs from legitimate real packages. Never hardcode.
- 2.3 Implement register, deregister, and list with atomic Raven package DB updates.
- 2.4 Resolve framework packages, architecture, and minimum versions deterministically. Missing dependencies are explicit failures.
- 2.5 Select package-identity APIs from actual target import scans. Return real values for registered packages and documented no-package errors otherwise.
- 2.6 Implement package-aware WinRT activation for the minimal classes demonstrated by targets.
- 2.7 Determine lifecycle requirements from target inspection; implement only the activation, suspend, resume, and termination behavior exercised.
- 2.8 Add manifest, identity, resolution, synthetic-package, and negative tests.
- 2.9 Close out and GATE.

## M3: Calculator / XAML rendering validation

- 3.0 **Scope GATE:** inspect the actual Calculator package and dependency surface. Verify whether it is a XAML application. Record a decision on Calculator, a smaller window-only test app, or both. The user decides.
- 3.1 The user supplies a package from a legitimate source. Record its format. If encrypted or license-bound, stop and document findings. Do not write decryption, key-extraction, or license-bypass code.
- 3.2 Read `docs/uwp/04-graphics-and-composition.md`; list required surfaces from evidence. Log explicit Vulkan device selection; never assume device index 0.
- 3.3 Record checkpoints: activation reaches entry, window creation, first frame, input event, suspend/resume, clean exit. Ask the user for screenshot/recording at window and first-frame checkpoints.
- 3.4 Add scripted launch harness with log capture, timeouts, and exit-code checks.
- 3.5 Close out and GATE.

## M4: Hello Neighbor Store package and game compatibility

- 4.0 Obtain the package legitimately. Record actual format (loose files, APPX/MSIX, bundle, encrypted, or another format). Run M0 inspector on every PE. Write `docs/hello-neighbor-inventory.md` covering binaries, framework dependencies, Win32/WinRT imports, graphics, input, audio, networking, Xbox services, and licensing/anti-tamper indicators.
- 4.1 Map each requirement to Raven status (done, partial, missing) with evidence.
- 4.2 **Licensing GATE:** if Store license/entitlement checks exist, document calls and legitimate paths. No spoofing, bypass, or decryption.
- 4.3 Fix loop: capture logs, identify the first failure, make the smallest evidence-backed change, add a test where possible, and repeat.
- 4.4 Document supported behavior, known issues, and exact reproduction steps; GATE.

## M5: Hardening, security review, and compatibility expansion

- Fuzz PE, manifest XML, and package ZIP/OPC parsers; retain harnesses.
- Review path traversal, archive bombs, package permissions, and no-execution guarantees.
- Provide one documented regression command for M0-M4 and CI where available.
- Record reproducible startup/frame-time baselines.
- Maintain a compatibility matrix by distro, GPU, driver, and session type.
- Audit documentation: every claim links to evidence or is marked unverified.
- Maintain a release checklist.

## Standing rules

- A passing build is not a finished milestone. Only recorded evidence is.
- If scope is uncertain, it is out of scope until a decision record is approved.
- If any plan conflicts with existing architecture documentation, stop and ask.
