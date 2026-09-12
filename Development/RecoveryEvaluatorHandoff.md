# Recovery evaluator takeover — 2026-09-04

The current task took over from `Add predecessor compatibility gate`
(`01a06ccd-f6ca-7100-8c2c-1bae1352dd68`) after the parser boundary correction.
No previous-session memory was used. The source task's agreed C-interpreter,
no-emitter recovery plan remains the direction.

## Implemented boundary

`Language/Recovery/Interpreter/evaluator.c` now binds and executes a bounded
subset of the existing AST. The recovery build tool accepts `--sanitize` and
still verifies its manifest by default. `parsedump.c` retains its census/tree
modes and adds `--run <entry> <sources...>`; result values are printed in full,
not reduced to a process-exit checksum.

The evaluator uses deterministic aggregate handles, a scoped binding environment,
checked Int64 arithmetic, Byte range checks, nominal constructs/payload enums,
Range-authored String methods, and RawBuffer-backed collection slot indexes.
Calls are bound to declarations before effects, including calls in untaken
branches. Destruction invalidates collection handles; aliases and double destroy
fail. Explicit limitations are in the recovery README and manifest.

Three existing runtime externs are bound and exercised: `targetPointerBits`,
`stringPrint`, and `stringDiagnostic`. Signature and symbol resolution occur at
binding. No dlsym fallback or silent unbound extern exists. The build pins Host,
RawBuffer and Metrics source dependencies as well as every recovery source/tool.

## Parser corrections uncovered by execution

- The fixed 4096-byte string accumulator silently truncated long literals.
  It now allocates from the full source span.
- Union type alternatives were consumed but discarded after the first. Their
  complete normalized spelling is retained; unsupported union execution rejects.
- Statement annotations could overwrite the `else` slot or disappear from
  evaluation. They now have a separate AST slot and fail when unsupported.
  A plain local `@many` annotation supplies cardinality explicitly.
- Trailing interpolation tokens are rejected rather than silently ignored.
- Oversized integer literals reject. Decimal/version token spelling remains
  retained, and attempting to execute it rejects instead of truncating to Int.
- Parser/AST allocations are released by the diagnostic driver.

The pinned canonical census remains 104856 / 158 constructs / 23 enums /
797 functions / 33 macros / 0 main / 0 failures across 101 unchanged files.

## Proofs

`Testing/Tools/check-recovery-evaluator` runs ordinary and ASan/UBSan builds:

- five successful fixtures including a generated 6000-byte string;
- twenty located rejection fixtures with exact expected diagnostic classes;
- the canonical ARM64 instruction encoder, twice per build, with all 101
  manifest sources loaded unchanged.

`Testing/Recovery/Canonical/Arm64Words.range` calls existing Range implementation
functions, verifies the exact bytes `40 05 80 d2 c0 03 5f d6`, exercises invalid
word diagnostics/validity mutation, and destroys its storage. The interpreter has
no instruction encoder: those bytes come from the canonical Range code.

This does not yet produce or run a Mach-O executable. It does not establish a
native compiler fixed point, complete macro execution or full-language typing.

## Next implementation boundary

Keep the recovery interpreter isolated. Inventory the reachable requirements of
`compileProject` and `emitExecutable` before broadening the executor. Current
loud unsupported boundaries include macros/environment emission, closures,
optional/union execution, and the rest of the host ABI. Canonical `print` is a
macro; it must be executed through its real body, not replaced with a fixture
recognizer. The next entry must request an executable through the binary backend,
not `--emit-assembly`.

The accepted native seed and Compiler A recovery mechanisms are unchanged.
Do not remove them before successor production and deterministic reproduction.
All changes remain uncommitted; this checkout already contains extensive
unrelated work that must remain intact.
