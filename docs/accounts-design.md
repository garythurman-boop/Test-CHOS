# Native account architecture — design, not authentication

M7A/7B prioritizes boot and interrupts. **This is design only: no credential store,
password hash, login, UID enforcement or ACL implementation is present.** Private
process mappings protect address spaces, but all native apps share filesystem
authority. Saved computer names are not accounts; Logoff ends apps, not authenticated
sessions. A password field without protected storage would imply false security.

## Identity and authority

Reserve UID 0 for system services, 1–999 for future service identities, and allocate
ordinary persistent UIDs monotonically from 1000 without reuse. Separate bounded
normalized account names, display names and numeric identity. Specify a versioned,
explicitly little-endian record with bounded lengths, UID/disabled flags, hash
algorithm/version/costs, salt and output. Reject duplicate names/IDs, unknown critical
versions and invalid lengths/ranges. Use protected system paths and atomic CHFS
replacement. CRC detects corruption, not forgery; final-flush errors can leave
uncertain durability, so record updates require explicit old/new recovery policy.

Add kernel-owned process credentials and authenticated session IDs/lifecycle/
revocation. Ordinary children inherit bounded authority; only a trusted login
service may request a verified identity. Callers must never choose their own UID.
Enforce file/directory ownership/mode at every operation, including traversal,
open/read/write/create/delete/rename, configuration and stale handles. Existing
built-in role checks are not general user authorization.

Separate installation/machine administration from ordinary desktop/file authority.
Keep optical-only, target-bound typed erase checks. Define first-account/recovery
bootstrapping before exposing account creation; no implicit passwordless UID-0
desktop. Deletion disables future logins/revokes sessions without recycling UID.

## Password hashing and entropy prerequisites

Use established Argon2id rather than an original hash. Reviewed PHC reference C at
tag `20190702`, commit
[`62358ba2123abd17fccf2a108a301d4b52c01a7c`](https://github.com/P-H-C/phc-winner-argon2/tree/62358ba2123abd17fccf2a108a301d4b52c01a7c).
LICENSE offers CC0 1.0 or Apache 2.0. Before adoption, pin it, retain applicable
license/notices/attribution, explicitly select a license, review current maintenance,
adapt the freestanding allocation/thread boundary and run published vectors.
**No Argon2 implementation is currently vendored or linked into CHOS.**

Proposed policy, pending target benchmarks: Argon2id v19, unique 16-byte random
salts, 32-byte outputs, 32 MiB memory, three passes, one lane. Store bounded costs
for migration; reject excessive values before allocation. Compare fixed-length
hashes without early exit, clear secrets/workspace non-elidably, bound password
input, rate-limit retries and define lock/recovery policy.

First implement/test a real entropy source and CSPRNG with health/error handling.
Counters, MPIDR, VM IDs and fixed seeds are not adequate randomness; fail closed
rather than silently use predictable salts. Demonstrate entropy on actual target
hardware. Current user allocation permits only 64 KiB per request; a quota-aware
larger allocation path or bounded trusted service is needed before 32 MiB hashing.

## Implementation gates

1. Kernel credentials and filesystem ownership/ACL format plus legacy policy;
   cross-UID, traversal, stale-handle, rename/delete and malicious-process tests.
2. Entropy/CSPRNG and bounded service allocation; exhaustion/quota/failure tests.
3. Licensed pinned Argon2id integration; known vectors, cost bounds, salt uniqueness,
   allocation failures and secret clearing.
4. Protected account store; duplicate/corrupt/full/torn-write/cold-reboot tests;
   only then account creation UI in Setup.
5. Login service, session issuance/revocation and privilege checks; failed login/
   throttling, cross-user file/config denial, Logoff revocation and reboot tests.

Only after these gates can CHOS claim authenticated accounts or permissions.
Encryption, verified boot and protection against offline disk access/hostile
hypervisors are separate future work. See [status](status.md).
