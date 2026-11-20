# BryairBird documentation

BryairBird is a Raven-based Linux Windows-application compatibility project. These documents describe the current research direction and planned work; they do not imply that UWP or Microsoft Store applications are already supported.

## Project planning

- [UWP compatibility roadmap](uwp-roadmap.md) — architecture areas, Wine-UWP research boundaries, milestones, acceptance criteria, licensing, and security constraints.
- [M0 feasibility audit](m0-feasibility-audit.md) — source-level audit of Raven's existing APPX/MSIX reader, WinRT plumbing, activation path, package identity tests, and the still-unverified Hello Neighbor package format.
- [Hello Neighbor target investigation](hello-neighbor-target.md) — target identity, package questions, evidence requirements, and compatibility checkpoints.
- [Packaging](packaging.md) — existing Raven-derived runtime packaging and source-material requirements.

## Documentation conventions

- Mark unknown or unverified behavior explicitly.
- Distinguish implemented behavior from planned work and experiments.
- Link claims about upstream projects to the exact upstream source or revision where possible.
- Record test environment, commands, logs, and results for compatibility claims.
- Do not commit proprietary game packages, Windows system binaries, credentials, tokens, or license files.
