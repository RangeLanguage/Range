# Range recovery seed

Bootstrap-only recovery infrastructure. **Not a compiler authority.**

The C interpreter executes Range source. It has no C, LLVM, assembly, object,
executable, linker or signing emitter. Native artifacts must be produced by the
interpreted Range compiler's existing ProgramGraph and ARM64/Mach-O backend.
The public compiler, installer, package commands and distribution launchers
never invoke this interpreter. Recovery is explicitly requested only.

## Current evidence

The C recovery parser reads all 101 canonical compiler sources without rewriting
them, with the pinned census:

```text
nodes=106955 construct=154 enum=23 function=805 macro=33 main=0 failures=0
```

A bounded evaluator now runs. `Testing/Tools/check-recovery-evaluator` checks seven
successful execution fixtures and forty-eight rejection fixtures in ordinary and
AddressSanitizer/UndefinedBehaviorSanitizer builds. It also loads **all canonical
sources unmodified** and executes the actual `appleArm64Encoding`,
`appleArm64EmitMoveImmediate`, and `appleArm64EmitWord` implementations. Range code
produces and checks the eight bytes for `mov x0, #42; ret`, then exercises the
encoder's invalid-word diagnostic and validity mutation. This runs twice in each
build. No native executable is produced by this proof.

The same gate executes the actual Range parser on focused input-member, macro
signature, member-RHS and generic fixtures, plus the Core integer-fit helper.
The generic fixture checks explicit `let` members, named and nested generic
arguments, and the difference between a specialized type reference and a call:

```range
Int<bits: 32>             // Selects a type.
Int<bits: 32>()           // Constructs using ordinary member defaults.
Int<bits: 32>(value: 3)   // Constructs with a supplied value.
```

These are structural parser proofs; neither generic construction nor conversion
executes yet. They do not establish that the Range parser handles every canonical
source. The C parser gate additionally checks 24 malformed-input probes.

**The complete compiler has not yet been executed through recovery. Generation
One, native fixed-point reproduction, and removal of the C runtime remain open.**

## Execution contract

- Primitive Int and Bool values copy by value. Int arithmetic is checked signed
  64-bit arithmetic; Byte conversion checks 0 through 255. The intrinsic numeric
  `bits:` application supplies the scalar value, as used by canonical Binary.
  This is a legacy recovery convention, not the meaning of Core's generic `bits`
  member. The intrinsic path does not evaluate Core generic defaults or project
  integer selection. Explicit specialization and ordinary generic construction
  outside that legacy path fail when required for execution.
- Constructs and enum payload aggregates have deterministic, monotonically
  assigned heap identities. Passing, returning and assigning them preserves
  identity. A local `let` cannot be reassigned; object mutation follows stored
  member policy. This is the bounded bootstrap reference model, not an
  implementation of the language's complete ownership analysis.
- `@many` collections use the existing RawBuffer runtime to hold indexes into
  interpreter values. Range integers and pointers are never truncated into the
  runtime's 32-bit index slots. Append, self-append, element, update, firstIndex,
  independent slicing, count, and destruction are supported. Slices copy the
  collection slots while retaining any aggregate element identities.
- Destruction invalidates the collection handle. Aliased access and repeated
  destruction fail with source locations. Host buffers are released explicitly;
  remaining interpreter arena allocations are reclaimed after the run.
- String literals and interpolation produce the canonical String/Byte shape.
  `append`, `substring`, `byte`, `count` and `destroy` execute their existing
  Range-authored implementations. Embedded NUL and strings longer than 4,096
  bytes are preserved.
- Free and member calls are bound to declaration identities before execution,
  using receiver type, argument labels and arity. Binding visits every branch of
  each reachable function; unresolved or ambiguous calls in untaken branches
  fail before execution. Inputs require explicit `let` or `binding`; their single
  name is also the required function argument label. Underscore parameters,
  external/local name pairs, omitted keywords, and state/derived inputs are rejected.
  Binding inputs are retained by the parser but fail explicitly if their function
  is required for execution; caller-storage binding is not implemented yet.
- `if`/`else`, `while`, early returns, nested calls, short-circuit Boolean
  expressions, and scalar/enum switches execute. Enum payload access is checked
  against the active variant at runtime.
- The current extern inventory is exactly `targetPointerBits`, `stringPrint`,
  and `stringDiagnostic`, each exercised by a fixture. Symbol aliases and ABI
  signatures are checked before execution. Other externs fail closed.
- Execution currently has a 100-million-step limit and 512-call-frame limit.
  Full-compiler performance has not been measured.

## Front-end boundary

Macro signatures, defaults, target/result type positions and bodies are parsed.
Defaults use construction syntax such as `symbol: String("")`; second-colon
defaults are rejected. A macro may omit its result while retaining its target.
Construct, function and macro generic headers require explicit `let` members.
Generic arguments require names and are stored separately from ordinary inputs
and arguments. Nested specializations are parsed; specialized union types are
not supported. This remains handwritten recognition of the declared Core forms,
not execution of their `@syntax` templates.
Environment blocks, template references, closures, switches, extensions, payload
enums and annotations remain in the AST. Union alternatives remain in the type
spelling; they are not silently reduced to their first alternative.

`@syntax` grammar templates are the only opaque grammar form: their source spans
are retained. They are not executable. Requiring one during recovery execution
fails. Decimal/version tokens retain their complete spelling and also fail if
execution requires them. Parsing a form does not imply the evaluator supports it.

The parser gate checks the exact census, structural examples, malformed-input
locations, immutable canonical inputs and isolation from public paths. The
manifest pins every interpreter/build source plus its existing Host, RawBuffer
and Metrics runtime dependencies. The evaluator gate additionally catches long
literal truncation, trailing interpolation tokens, numeric overflow, invalid
storage use, unsupported annotations and ambiguous/undefined calls.

## Open boundary

Macro evaluation, environment emission, closures, optional/union execution, the
remaining extern ABI, generic specialization and full compiler execution are not
implemented. Required
unsupported behavior fails loudly. Only intrinsic cardinality/primitive metadata
has execution meaning in this slice; other required annotations are not ignored.

The current next slice is the Core literal/materialization connection described
in `Development/CompilerFlow/GoalAudit.md`: a matched literal must reach the
selected application's effective value and generic members, then undergo Core
integer validation. Canonical generic resolution and syntax materialization
currently reject generic syntax explicitly rather than discard its relationships.
Finish that contract before independent evaluator expansion or representation
lowering. Eventual recovery compilation must enter the binary backend, never the
legacy textual assembly route. Preserve existing bootstrap mechanisms until the
native candidate and its reproduction are proven.

## Explicit development commands

```sh
Language/Recovery/Tools/build-recovery-interpreter /tmp/range-recovery
/tmp/range-recovery --run recoveryArm64Words \
  $(cat Language/CompilerSources.txt) Testing/Recovery/Canonical/Arm64Words.range
Testing/Tools/check-recovery-parser
Testing/Tools/check-recovery-evaluator
```

`--skip-manifest` is an explicit development build option. Acceptance gates never
use it. `--sanitize` builds the same implementation with ASan and UBSan.
