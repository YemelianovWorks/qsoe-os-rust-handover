# Task Manager Logging Rust Default Stop Boundary

Captured: 2026-07-02 CEST.

## Scope

`tm_log` is complete for its planned Rust migration scope: Rust owns the
default typed task-manager log formatter path through `qsoe-tm-log`, while the
exported C variadic ABI and low-level LQ debug-console sink remain C-owned.

This is intentionally not a C-retirement item. The preserved C boundary is the
contract that lets existing C taskman callers keep using:

```c
void tm_log(int level, const char *fmt, ...);
```

Rust owns:

```text
rust/crates/qsoe-tm-log/src/lib.rs
rust/crates/qsoe-tm-providers/src/lib.rs
```

C remains responsible for:

```text
libtaskman/include/tm_log.h
libtaskman/src/log.c
lq/taskman/tm_log.c
```

## Selector Posture

Normal builds select the Rust formatter by default:

```text
QSOE_RUST_TM_LOG ?= 1
```

The explicit C fallback remains supported:

```text
QSOE_RUST_TM_LOG=0
```

The fallback is retained because it verifies that the Rust formatter can be
selected without replacing the exported C variadic ABI. It is a deliberate
stop-boundary, not unfinished migration work.

## Evidence Gate

The focused evidence gates are:

```sh
make tm-log-evidence
make container-tm-log-evidence
```

They verify:

- `QSOE_RUST_TM_LOG=1` selects the Rust `tm_log_emit_args` provider.
- `QSOE_RUST_TM_LOG=0` keeps the weak C fallback available.
- The exported C variadic logging ABI remains C-owned.
- The selected taskman Rust provider archive has the expected ELF posture.
- LQ taskman logging reaches the boot-smoke logging path.

Current roadmap evidence also includes the current `main` revalidation at
commit `233a473e2b4ea50f7d8b63bd6f681e80285ef452`, CI run `28613387852`,
CodeQL run `28613387902`, and Pages deployment run `28613387734`.

## Acceptance

- Rust is the default formatter implementation for task-manager logs.
- The C `tm_log(...)` variadic entry point remains the stable caller ABI.
- C continues to consume `va_list` and marshal typed log arguments.
- LQ debug-console emission remains in C.
- `QSOE_RUST_TM_LOG=0` remains a supported rollback and evidence selector.
- No C implementation removal PR is planned for this component unless the
  logging ABI itself changes.

## Stop Boundary

Do not translate the exported variadic C ABI to Rust as part of this migration
track. The remaining C code is ABI glue and platform sink ownership, not
duplicated formatter logic.

Future Rust work should only reopen this boundary if one of these changes is
true:

- taskman replaces the variadic C logging ABI with a typed ABI;
- the LQ debug-console sink gets a broader platform abstraction;
- the C fallback becomes a maintenance or security liability.

Until then, #153 should be treated as complete for its stated Rust-default
scope rather than as a pending C-retirement candidate.
