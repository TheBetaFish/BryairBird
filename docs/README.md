# BryairBird documentation

BryairBird is a Raven-based Linux Windows-application compatibility project. These documents describe the current research direction and planned work; they do not imply that UWP or Microsoft Store applications are already supported.

## Execution control

- [Execution plan](EXECUTION_PLAN.md) — ordered tasks, gates, evidence requirements, and milestone acceptance criteria.
- [Decision 0001](decisions/0001-canonical-milestone-roadmap.md) — canonical M0–M5 milestone sequence.
- [Decision 0002](decisions/0002-github-actions-baseline.md) — GitHub Actions is the authoritative repository build/test baseline.
- [Decision 0003](decisions/0003-reuse-pe-parser.md) — approved reuse of Raven's existing PE parser with a separate read-only inventory front end.
- [Current status](STATUS.md) — active task, open issues, decisions, and verification state.
- [Evidence logs](evidence/README.md) — format and location of reproducible task evidence.
- [Decision records](decisions/README.md) — process for documenting proposed and approved architectural choices.

## Project planning

- [UWP compatibility roadmap](uwp-roadmap.md) — architecture areas, Wine-UWP research boundaries, milestones, acceptance criteria, licensing, and security constraints.
- [M0 feasibility audit](m0-feasibility-audit.md) — source-level audit of Raven's existing APPX/MSIX reader, WinRT plumbing, activation path, package identity tests, and the still-unverified Hello Neighbor package format.
- [M0 PE inspection report](m0-inspection-report.md) — source-level inspection of Raven's existing PE parser, loader integration, tests, and extension gaps.
- [M0 read-only PE inspector specification](m0-inspector-spec.md) — approved parser-reuse architecture, CLI contract, JSON v1 behavior, limits, diagnostics, and security boundary.
- [M0 PE inspector test plan](m0-test-plan.md) — synthetic fixture matrix, malformed-input coverage, deterministic-output checks, resource limits, and read-only safety assertions.
- [Inspector JSON Schema v1](schemas/inspector-v1.schema.json) — normative machine-readable structure for inspection reports.
- [Inspector CLI source](../native/pe-inspect/) — isolated command-line argument parser and contract tests; inspection and report generation are not implemented yet.
- [M1 package-context design](m1-package-context-design.md) — proposed Raven subsystem boundaries, package graph and process identity model, activation sequence, security constraints, and synthetic-fixture acceptance criteria.
- [M1.1 process-context inspection](m1-1-process-context-inspection.md) — verified child-process launch path, current launch-state transport, package-context integration boundary, and follow-up inspection requirements.
- [M1.2 package-identity API inspection](m1-2-package-identity-api-inspection.md) — current hardcoded no-package API handlers, likely guest-context consumer, cross-process identity requirements, and regression-test plan.
- [Hello Neighbor target investigation](hello-neighbor-target.md) — target identity, package questions, evidence requirements, and compatibility checkpoints.
- [Windows EFS compatibility design](efs-compatibility-design.md) — EFS semantics, cryptographic/key-management requirements, host-filesystem boundary, security invariants, and staged research proposal.
- [Packaging](packaging.md) — existing Raven-derived runtime packaging and source-material requirements.

## UWP / WinRT architecture blueprint

- [UWP architecture blueprint index](uwp/README.md) — targeted source inventory and links to the standalone gap analysis, prefix/harvest, app-model/activation, graphics/composition, and milestone specifications.

## Documentation conventions

- Mark unknown or unverified behavior explicitly.
- Distinguish implemented behavior from planned work and experiments.
- Link claims about upstream projects to the exact upstream source or revision where possible.
- Record test environment, commands, logs, and results for compatibility claims.
- Do not commit proprietary game packages, Windows system binaries, credentials, tokens, or license files.
