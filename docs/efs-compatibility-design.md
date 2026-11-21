# Windows Encrypting File System (EFS) compatibility design

**Status:** Research/design baseline; no EFS implementation is claimed.
**Scope:** Windows per-file NTFS Encrypting File System compatibility for applications running under Raven/BryairBird. This is not BitLocker, volume encryption, APPX/MSIX package encryption, or a replacement for host disk encryption.

## 1. What EFS means

Windows EFS encrypts individual files on NTFS volumes. A file encryption key (FEK) encrypts file contents. The FEK is protected for each authorized user and data recovery agent (DRA) using their public-key credentials and is stored with EFS metadata. Authorized access transparently recovers the FEK and decrypts file data. Directories are policy/metadata markers: newly created descendants inherit encryption behavior; the directory's own contents are not encrypted as one blob.

Primary Microsoft references:
- [Win32 file encryption overview](https://learn.microsoft.com/en-us/windows/win32/fileio/file-encryption)
- [Handling encrypted files and directories](https://learn.microsoft.com/en-us/windows/win32/fileio/handling-encrypted-files-and-directories)
- [MS-GPEF EFS overview](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-gpef/4d4687f9-132c-4501-9996-684b40b14735)
- [MS-EFSR protocol overview](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-efsr/4892c610-4595-4fba-a67f-a2d26b9b6dcd)
- [Backup and restore of encrypted files](https://learn.microsoft.com/en-us/windows/win32/fileio/backup-and-restore-of-encrypted-files)

## 2. Compatibility objective

Provide correct, fail-closed behavior for the Windows EFS API and filesystem contracts that real applications exercise, without pretending ordinary host files are EFS-encrypted.

The first goal is API and semantic discovery, not a new cryptographic format. Implement only after Raven's guest filesystem and identity/key-storage boundaries are inspected and a concrete test matrix exists.

## 3. Required contract areas

### 3.1 File and directory semantics

Inventory and test the Windows-facing contracts for:
- EncryptFile / DecryptFile
- FileEncryptionStatus
- EncryptionDisable
- GetFileAttributes and FILE_ATTRIBUTE_ENCRYPTED
- CreateFile with FILE_ATTRIBUTE_ENCRYPTED
- CopyFile / CopyFileEx behavior for encrypted sources and destinations
- GetVolumeInformation and FS_FILE_ENCRYPTION
- inheritance of encryption policy when creating files beneath an encrypted directory
- documented unsupported cases and precise Win32 error reporting

The API inventory is provisional until imports, traces, and Raven source inspection establish which entry points are actually needed.

### 3.2 Cryptographic and identity model

EFS is not just a boolean file attribute. The design must account for:
- per-file FEK generation and authenticated encryption semantics compatible with the Windows format;
- public-key wrapping of the FEK for each authorized user certificate;
- DRA recipients and recovery metadata;
- certificate/private-key association with the correct guest user identity;
- secure private-key protection and lifecycle;
- metadata parsing, validation, and preservation across rename, copy, backup, restore, and migration.

Do not store private keys in plaintext, derive a user's private key from a guessed password, fabricate certificates or recipients, or silently substitute a weaker encryption mode. Do not claim EFS support if data is merely marked encrypted or stored as plaintext.

### 3.3 Host filesystem boundary

EFS is an NTFS capability. Raven's Linux host filesystem is not automatically an EFS-capable NTFS volume. Before choosing a backend, inspect Raven's filesystem implementation and determine whether it can preserve Windows file attributes, streams, security metadata, and per-user identity.

Potential architectural choices to investigate—not decisions:
1. A Raven-managed encrypted-file layer with explicit metadata and a vetted cryptographic library.
2. A compatible NTFS implementation/backend if Raven already has a supported path for it.
3. Explicit unsupported status for EFS operations on filesystems that cannot provide the required semantics.

Do not assume Linux fscrypt, host permissions, or a plain xattr is equivalent to Windows EFS. Any backend must define key isolation, crash consistency, metadata integrity, backup/restore, and interoperability.

## 4. Security invariants

- Fail closed: never report successful encryption when file contents remain plaintext.
- Unauthorized identities must not receive plaintext or a usable FEK.
- Do not silently decrypt on copy, rename, export, backup, or migration.
- Preserve all authorized recipient metadata or fail with an explicit error.
- Key material must not enter logs, crash reports, Git, test artifacts, or ordinary debug output.
- No key extraction, credential bypass, recovery-agent bypass, or weakening of access checks.
- Test malformed metadata, truncated ciphertext, wrong keys, missing keys, unauthorized users, and recovery-agent cases using synthetic keys and disposable fixtures.
- Zeroize transient secret buffers where the language/runtime supports it; minimize secret lifetime and copies.
- Encryption and decryption tests must verify ciphertext-at-rest, authorized round trips, and unauthorized failure—not only API return codes.

## 5. Phased implementation proposal

This proposal does not reorder the canonical M0–M5 milestones.

### EFS-R0 — Source and API audit
- Inspect Raven filesystem abstractions, file handles, streams, attributes, security descriptors, user identity, registry/DPAPI, and crypto provider support.
- Search for existing EFS API stubs and relevant NT/Win32 entry points.
- Map each API to current implementation, missing contract, and test location.
- Document which host filesystems can preserve required metadata.
- Deliver docs/efs-implementation-plan.md; no runtime changes.

### EFS-R1 — Contract and fixture design
- Define synthetic file metadata and test certificates/keys.
- Specify recipient metadata, FEK lifecycle, user/DRA behavior, copy semantics, and error mapping from authoritative Windows contracts.
- Define crash-safe writes and atomic metadata updates.
- Security review before selecting a format or dependency.

### EFS-R2 — Minimal secure vertical slice
- Implement one narrowly scoped encrypt/decrypt/status flow only after R0/R1 approval and tests.
- Use vetted cryptographic primitives; do not invent a cipher or key-wrapping construction.
- Keep guest identity and key access explicit.
- Reject unsupported host volumes and unsupported file types accurately.

### EFS-R3 — Compatibility expansion
- Add directory inheritance, multi-recipient files, DRA behavior, copy/backup semantics, and any EFSRPC surface demonstrated to be needed.
- Add fuzzing and adversarial metadata tests.
- Validate against documented Windows behavior using legitimate test environments.

## 6. Relationship to package support

EFS is independent of APPX/MSIX package parsing and deployment:
- APPX/MSIX work concerns package containers, manifests, identity, dependencies, registration, and activation.
- EFS concerns transparent per-file encryption, recipient metadata, key management, and filesystem semantics.
- Do not conflate EFS-encrypted files with MSIX package signing, package block maps, or any Store licensing/encryption scheme.

## 7. Open questions

- Which Raven filesystem backend owns guest file operations and metadata?
- Does it support alternate data streams and durable per-file metadata?
- How are guest users, logon sessions, certificates, DPAPI, and private keys represented?
- Which EFS Win32 APIs already exist, and which are imported by the first selected test application?
- Is there an existing vetted crypto provider in Raven suitable for the required algorithms and formats?
- What backup/restore and cross-prefix behavior is expected?
- Which host filesystem guarantees can be relied upon?

No implementation decision is made until these are answered with source evidence.
