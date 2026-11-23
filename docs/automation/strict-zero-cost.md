# Strict zero-cost automation

## Policy

This system makes no hosted AI inference requests and requires no API key, paid model endpoint, or GitHub Models quota. The coder and reviewer run against two locally installed Ollama models on a repository-owned self-hosted Linux runner.

The existing GitHub Actions CI remains authoritative. The local worker dispatches the repository's existing CI workflow for each revision and waits for its result. It does not replace the build, six CTest shards, kernel-waits job, Rust tests, compliance scanning, or Report job with a smaller approximation.

GitHub-hosted Actions usage is still subject to GitHub's plan and usage limits. Public-repository standard runner usage is generally free under GitHub's published policy, but local compute, electricity, and internet access are not literally costless.

## Components

| Component | Responsibility | Execution |
|---|---|---|
| Automation intake | Marks issues labelled ai-task or bug as local-ready | GitHub-hosted Actions; deterministic |
| Main CI failure reporter | Opens one issue per failed push CI run on main | GitHub-hosted Actions; deterministic |
| Local coder | Makes a bounded implementation attempt using Aider and local Ollama | Self-hosted runner |
| CI gate | Runs the existing CI workflow on the worker branch | Existing GitHub-hosted CI |
| Local reviewer | Reviews the full diff and CI result using a separate Ollama model | Self-hosted runner |
| Revision loop | Feeds concrete CI/reviewer findings back to the coder; maximum three attempts | Self-hosted runner |
| Pull request | Creates a normal PR on success or a draft PR if gates remain unresolved | GitHub API via workflow token |

The local worker uses the existing .github/workflows/ci.yml through its already-supported workflow_dispatch trigger. It does not guess at a standalone CMake configure command: this repository's CI provisions pinned components, configures CMake with .github/scripts/components.sh, builds raven and raven-runtime, then runs six CTest shards plus the other required jobs.

## Runner setup

Create a dedicated self-hosted runner for this repository and give it these labels:

- self-hosted
- linux
- x64
- bryairbird-ai

Use a disposable Linux VM or a dedicated, unprivileged runner account with no personal files, browser profile, SSH keys, cloud credentials, or unrelated repository access. Do not run this agent on a workstation account that contains private data. The local coder receives no GitHub token in its process environment, but it can still read and modify files accessible to the runner account. Aider is deliberately not started with automatic approval for shell commands.

Install and start:

1. GitHub CLI (gh).
2. Python 3.
3. Rust/Cargo.
4. Ollama, listening on 127.0.0.1:11434.
5. Aider Chat, installed for the runner service account and available on PATH.

Install the local models before running the workflow. Example defaults:

- Coder: qwen2.5-coder:3b
- Reviewer: qwen2.5:3b

The workflow never pulls a model automatically. If either model is absent, it stops before editing. You can override the defaults with repository Actions variables:

- BRYAIRBIRD_CODER_MODEL
- BRYAIRBIRD_REVIEWER_MODEL

The runner service must inherit the PATH and environment needed to find Ollama, Aider, Cargo, Python, and gh. Restart the runner service after changing its environment.

## Running a task

1. Open an issue authored by the repository owner.
2. Apply ai-task for implementation work or bug for a defect.
3. Wait for the automation: local-ready label/comment.
4. In Actions, run Local AI worker (strict $0) and supply the issue number.
5. The worker creates a unique ai/issue-* branch, makes up to three revisions, dispatches the full CI workflow after each committed revision, and asks the local reviewer to inspect the complete diff and CI result.
6. It creates a normal PR only when CI succeeds and the reviewer returns a valid pass verdict. Otherwise it creates a draft PR with the unresolved state and CI links.
7. A human reviews and merges the PR. No workflow auto-merges.

The worker intentionally accepts only issues authored by the repository owner and only runs when that owner manually dispatches it. This reduces the chance that arbitrary public issue text can drive a local machine.

## Trust and permission boundaries

- Intake and CI-failure reporting are deterministic; they do not invoke a model.
- The worker workflow is manually dispatched and checks that the actor is TheBetaFish.
- The issue must be open, authored by the repository owner, and labelled ai-task or bug.
- The coder runs without GH_TOKEN, GITHUB_TOKEN, Actions credentials, or persisted checkout credentials in its child-process environment. Its Aider ignore rules are generated under RUNNER_TEMP and passed with Aider's --aiderignore option; repository assistant configuration is not committed.
- Aider is not given --yes-always. Shell-command approval is not automatically granted.
- The automation rejects changes to its own workflow, scripts, docs, and any .aiderignore file. It also rejects credential-like files and staged files larger than 5 MiB.
- GitHub credentials are used by the orchestration process to fetch the issue, push the branch, dispatch CI, and open the PR.
- The reviewer is advisory. It cannot approve, merge, or change repository settings.
- The revision limit is three. A non-green CI result or invalid reviewer response never becomes a clean result.
- No secrets, Windows binaries, proprietary packages, or game files are added by this automation.
- The local model can still make incorrect changes or produce an incorrect review. Full CI and human PR review remain mandatory.

## Known limitations

- The local runner must be online and available. GitHub cannot execute the local model when the runner is offline.
- The workflow is intentionally owner-triggered rather than automatically executing arbitrary new issues.
- Aider and Ollama must be installed and the selected models must already be present.
- Diffs above the reviewer's configured size limit fail closed and cannot receive a clean local review.
- A local AI pass is not proof of correctness. CI is the deterministic gate; human review is the merge gate.
