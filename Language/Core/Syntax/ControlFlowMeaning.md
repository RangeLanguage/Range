# Local initialization through control flow

Agreed rule, 2026-09-06: a local may be declared before its value is supplied.
Every read requires initialization on every path reaching that read. A let may
only be initialized once on a path; a state may subsequently be assigned again.
Unused locals may remain uninitialized. This does not require an unused warning
in the current recovery engine.

```range
let user: User
if condition {
    user: User()
} else {
    return 42
}
// Only the initialized path reaches this point.
```

The recovery evaluator now checks the existing structured AST before execution:

- Sequential statements update each declaration's initialization state.
- Branches start from the same incoming state; only continuing branches join.
- Returns end a path. Initialization after a return cannot satisfy an earlier read.
- A loop may execute zero times. A possible subsequent iteration is checked to
  detect repeated initialization of an outer let. Body-local declarations are
  fresh each iteration. General loop-condition proofs are not performed.
- Switch arms join like branches. A default, both boolean cases, or all cases of
  a payloadless enum establish coverage. Other patterns conservatively retain an
  unmatched path; payload-pattern exhaustiveness is not implemented.
- Assigning a field reads its base instance. It cannot initialize an uninitialized
  base object. Assignment RHS reads are checked before the destination becomes
  initialized, including `value: value`.

Initialization state follows resolved declaration identities, including shadowed
locals. Runtime slots also distinguish uninitialized storage from an initialized
value; no zero or null default is silently supplied.

This is bounded recovery machinery, not executable Core control-flow definitions.
Core's Execution construct and retained syntax alone do not implement these
transfer/join rules. The canonical compiler still needs the corresponding graph
analysis. This pass does not introduce a general CFG abstraction, implement macro
execution, or prove aggregate ownership/COW. Existing unsupported closures and
binding inputs remain unsupported. The binder still validates unsupported syntax
in unreachable code even though initialization analysis stops at terminating paths.

Evidence: Testing/Recovery/Pass/Initialization.range and the initialization failure
fixtures execute through Testing/Tools/check-recovery-evaluator, in ordinary and
sanitized builds. They test later initialization, unused requirements, branches,
returns, loops, switch coverage, premature reads, and repeated let initialization.
