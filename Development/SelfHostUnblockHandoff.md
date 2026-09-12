# Self-host unblock handoff

## 2026-09-03

## Status

The compiler has a complete native backend that has never executed. ProgramGraph
lowering, the Apple arm64 encoder, the Mach-O linker, chained fixups, and ad-hoc
signing are all authored and wired. None of it runs, because the accepted seed
cannot build a candidate compiler from the current sources.

### 2026-09-04 correction: restore Compiler A for bootstrap

The accepted seed's call boundary is precise: extern calls with arguments work,
but Range-defined calls with arguments do not.

| Call shape | Result |
| --- | --- |
| internal zero-argument, `helper()` | PASS |
| extern with literal argument, `commandLineArgument(index: 0)` | PASS |
| internal with literal argument | FAIL |
| internal with variable argument | FAIL |
| internal with two arguments | FAIL |

Every failing internal case reports:

```text
first native call slice requires a literal Int callee
```

The compiler has 651 parameterized functions among 771 functions, so 84% of its
functions are behind this boundary. No bootstrap entry shape can route around
it: `generationZeroCompilerEntry` first reaches the two-argument
`compilerBCompileRoute` call, and the reachable compiler graph immediately
contains hundreds more.

This is not a regression in the seed. `Language/Bootstrap/Manifest.json`
describes it as the last verified **pre-native-fixed-point** compiler and marks
it as `candidateInput`. It was never able to compile a parameterized Range
program through its native slice.

The call-capable predecessor was Compiler A. Commit `f62600522` retired it on
2026-08-28 before the native fixed point existed. Restoring Compiler A from
`30b77a430` and running it against its era-appropriate sources still succeeds:

```text
exit=0
29,088 lines of LLVM emitted
```

Compiler A reaches today's sources and first rejects eight declaration-only
files at `enumPayloadLink / invalidEnumPayloadType`:

| File | Lines | Functions |
| --- | ---: | ---: |
| `Language/Core/Syntax/DeclarationForms.range` | 84 | 0 |
| `Language/Core/Syntax/Macro.range` | 40 | 0 |
| `Language/Core/Syntax/Member.range` | 35 | 0 |
| `Language/Core/Execution.range` | 16 | 0 |
| `Language/Core/Syntax/Construct.range` | 15 | 0 |
| `Language/Core/Macros/Promise.range` | 15 | 0 |
| `Language/Core/Package/Project.range` | 15 | 0 |
| `Language/Core/DataTypes/Vector.range` | 7 | 0 |

Those 227 lines are the first dialect boundary, not the complete dialect gap.
That distinction was tested during implementation. Replacing the declarations
with parseable stubs advances Compiler A to 420 macro-linking failures because
the rich `@many` and syntax materialization layer was never part of Compiler
A's input universe. The historical driver confirms the architecture: Compiler
A compiled a 28-file bootstrap subset into a call-capable Compiler B, and that
successor—not Compiler A—was responsible for the declaration-rich self-host
source set.

The exact predecessor subtree and Compiler A executable are now restored under
`Language/Bootstrap/CompilerA`. The bootstrap-only driver reproduces the
historical result:

```text
29,088 lines of LLVM
Generation Zero lowers a two-argument Range-defined call
```

Generation Zero still cannot consume today's source set directly. The current
sources use later surface semantics including validation-only macros and
type-position `@many`; the restored compiler reports an invalid composed project
before function reachability. Therefore the missing step is a bootstrap bridge:
extend the bootstrap-only 28-file successor source set just far enough to accept
the current canonical surface, then use that bridge to produce Generation One.
Keep canonical-source isolation, and do not extend the frozen seed or
SeedSurgery into a replacement compiler.

The first implementation pass remains useful evidence: its isolated transform
hoists 1,041 post-branch `let` and `state` declarations across 38 staged files,
and its compiler-only entry keeps the unfinished CLI out of the self-host proof.
Those mechanisms now belong to the Compiler A staging route rather than a claim
that the accepted seed can produce Generation One.

This document records the measured boundary, corrects an earlier wrong diagnosis,
and gives the ordered plan. Everything below was reproduced against the seed at
`Language/Bootstrap/range` on 2026-09-03.

## Correction: the blocker is not compound conditionals

An earlier reading assumed the seed dies on `&&` / `||` in conditions, and
proposed rewriting compound conditionals into nested ifs. **That is wrong.**
Compound conditionals compile. Do not spend time on them.

The seed dies on a `let` binding that appears after a branch in the same body:

```swift
function start(): Int {
    let a: Int(3)
    if a == 0 { return 64 }
    let b: Int(7)
    return b
}
```

```text
compilerBProjectError	execution compass continuation effect is not materialized
```

### Reproduced discrimination

Each of these is a complete fixture body, built with
`Language/Bootstrap/Tools/bootstrap-range-compiler --emit-assembly <dir>`:

| Shape | Result |
| --- | --- |
| `if a == 0 && a == 1 { return 64 }` then `return 42` | PASS |
| nested `if a == 0 { if a == 1 { ... } }` | PASS |
| two `let` bindings, no branch | PASS |
| `let a: Int(3)` then `return a + 1` | PASS |
| `state` declared before a branch, assigned after | PASS |
| bare call statement after a branch | PASS |
| **`let` binding after a branch** | **FAIL** |

Both rewrites clear it, and both are verified:

```swift
// hoist above the branch
let a: Int(3)
let b: Int(7)
if a == 0 { return 64 }
return b

// or declare state before, assign after
let a: Int(3)
state b: Int(0)
if a == 0 { return 64 }
b: 7
return b
```

## Measurements

Counted across the 101 sources in `Language/CompilerSources.txt`
(29,350 lines, 781 functions and macros).

| Blocker | Sites | Parser records it | ProgramGraph lowers it |
| --- | --- | --- | --- |
| `let` after a branch | 1,251 in 254 functions | yes | yes |
| Field reads and method calls | 4,175 reads / 2,675 calls | yes, facet ignored | no |
| `while` loops | 507 | **no facet exists** | no |
| `@many` collection operations | 871 | yes, facet ignored | no |
| `switch` | 2 | no | no |

Top files for let-after-branch:

```text
336  Language/Compiler/Parser.range
275  Language/Compiler/Assembly.range
 75  Language/Compiler/Graph/Lowering.range
 72  Language/Compiler/SyntaxMaterialization.range
 68  Language/Core/Syntax/Query.range
 49  Language/Compiler/Backend/MachOObject.range
```

## There is no loop facet

The token `"while"` appears exactly once in the 7,263-line
`Language/Compiler/Parser.range`, at line 5148, inside the list of keywords that
disqualify a terminal expression:

```swift
if expressionFirst.kind == .identity
    && (compilerBTokenMatchesBytes(source: source, token: expressionFirst, expected: "let")
        || compilerBTokenMatchesBytes(source: source, token: expressionFirst, expected: "while")) {
    return 0 - 1
}
```

`CompilerBProcessGraph` has no loop or iteration facet. Parsing a file with a
loop still reports `valid=true`, because the loop body is skipped rather than
recorded. The compiler's own sources contain 507 loops. No amount of source
rewriting removes this: the parser must learn to represent a loop before the
compiler can ever compile itself.

## Two separate gap lists

Keep these apart. They block different milestones.

**Gap list A — blocks Generation One from existing.** The bootstrap path is
missing, not the implementation of calls in the current compiler. Compiler A
remains executable from `30b77a430` and now reproducibly builds a call-capable
Generation Zero. Generation Zero must gain the later syntax/macro surface before
it can compile today's sources. The eight declaration files and 227 lines above
are only the first Compiler A diagnostics, not a bound on this bridge.

**Gap list B — blocks the fixed point.** What the new ProgramGraph lowering
cannot lower. Generation One will contain the new lowering, so it must handle
every form the sources use before it can rebuild them.

`ProgramGraph` lowering in `Language/Compiler/Graph/Lowering.range` consumes 11
of the 26 process facets:

```text
consumes: processes expressions applications arguments locals
          assignments returns conditionals comparisons logicals executions

ignores:  projections collections collectionProductions switches switchCases
          components noValueFallbacks sourceChains sourceOccurrences
          applicationProvisions environments environmentNodes
          emittedMacroBlocks extensions functionOwners
```

`graphLowerOwner` dispatches five statement kinds only: local, execution,
conditional, assignment, return. There is no loop case, because there is no
loop facet to dispatch on.

## Plan

### 1. Restore Compiler A and build the bootstrap bridge

The Compiler A executable, runtime, source manifests, exact predecessor inputs,
and driver from `30b77a430` are restored in a bootstrap-only location. They are
not a second active compiler authority and do not enter the public CLI.

Next, evolve the bootstrap-only successor sources—not canonical language
sources—until Generation Zero accepts the current macro/type surface. Keep each
compatibility addition explicit, deterministic, and auditable. The restored
driver retains the historical staged `IntMany` to `Buffer<Int>` transport.

Canonical sources stay clean. The staging adapters and restored Compiler A are
deleted after the native fixed point replaces the bootstrap chain.

Target:

```bash
Testing/Tools/check-range-compiler-self-host
```

Success is a Generation One candidate built from today's executable compiler
sources. A boundary report or era-appropriate Compiler A output is not success.

### 2. Prove the binary backend on fixtures

Generation One carries roughly 6,500 lines of encoder and linker that have never
run. Make this pass:

```bash
Testing/Tools/check-range-binary-linker-execution
```

It currently exits early via its accepted-seed-boundary path. Expect a burst of
Mach-O and instruction-encoding bugs. Fix them against the smallest fixtures,
before touching anything else.

### 3. Close the lowering gap

This is the real work, not step 1. In this order:

- **Projections.** 4,175 sites. The parser already records the facet and the
  lowering simply ignores it. Highest payoff, no parser changes needed.
- **Loops.** 507 sites. Requires a new parser facet first, then a lowering case
  emitting blocks. The backend already has branches, block arguments, and
  control edges, so the backend side is mostly done.
- **`@many`.** 871 sites. The collections facet exists, and the runtime already
  provides RawBuffer.

Rewrite the two `switch` sites as `if` chains and skip switch support entirely.

### 4. Fixed point, then swap the seed

Generation One compiles the sources to Generation Two, Generation Two compiles
them again, and the ProgramGraph render, object, and executable must match byte
for byte. Then in one commit retire the textual arm64 renderer,
`SemanticOperationGraph`, the compass path in `Assembly.range`, the
`--emit-assembly` mode, and the step 1 staged transform.

### 5. CLI last

`Language/CLI/Main.range` currently routes `compile` and `run` through
`rangeCompilerBodyGate`, which prints `compiler-body graph lowering is not
materialized` and returns 69. Replace it with the real `buildProject` call,
retire `Language/Bootstrap/GenerationZero/RangeCLIBridge.c`, and publish the
first artifact so `install-range` has something to install. The distribution
manifest currently has an empty `artifacts` map.

## Do not do these yet

- Do not rewrite compound conditionals. They compile.
- Do not add parameterized/internal call lowering to the frozen seed or extend
  SeedSurgery into a bootstrap compiler. Compiler A already owns that capability.
- Do not touch the installer, the project registry, or RISC-V before step 2
  passes. `Language/Compiler/Graph/RiscV.range` is a deliberate stub returning
  `RISC-V ProgramGraph materialization is not implemented`.
- Do not attempt the fixed point before step 3. The sources contain 507 loops
  and Generation One cannot lower a single one of them.

## Reference commands

```bash
Testing/Tools/check-range-compiler                        # seed integrity, passes
Testing/Tools/check-compiler-a-successor                  # A -> call-capable Generation Zero, passes
Testing/Tools/check-program-graph-lowering                # frontend fixture, passes
Testing/Tools/check-range-binary-backend                  # structure only, passes
Testing/Tools/check-range-cli-bridge                      # C bridge, passes
Testing/Tools/check-range-compiler-seed-surgery           # in-memory probe, passes
Testing/Tools/check-range-binary-linker-execution         # step 2 target
Testing/Tools/check-range-compiler-self-host              # step 1 target
```

Set `RANGE_KEEP_SELF_HOST_ROOT=1` to retain the staging tree for inspection.

## Note on SeedSurgery

`Testing/Support/SeedSurgery/` patches the seed in memory through
`DYLD_INSERT_LIBRARIES` and its probe passes. Applied to the full source set it
moves the boundary but does not clear it, failing instead with `could not record
execution compass relationships`. It is a diagnostic instrument, not a path to
Generation One. Prefer the staged Compiler A predecessor route in step 1.

## 2026-09-04 — Generation One staging

### What now exists

The frozen predecessor is driven entirely by a staging transform. No second
compiler is maintained, extended, or fixed: `Language/Bootstrap/CompilerA/range`
is a hash-pinned artifact, and every adaptation lives in
`Language/Bootstrap/Tools/build-compiler-a-bundle`, which is deleted together
with the CompilerA package at the fixed point.

```sh
Testing/Tools/check-compiler-a-bundle          # pins the current frontier
Language/Bootstrap/Tools/build-compiler-a-bundle <out> --entry <entry.range>
```

The bundle stages 82 canonical sources. Canonical sources are never modified by
it; adaptations are applied to an in-memory copy.

### Predecessor feature boundary, measured

Probed directly against the frozen predecessor. Everything the accepted seed
cannot do, the predecessor already does:

| Feature | Seed | Predecessor |
| --- | --- | --- |
| Internal call with arguments | fail | pass |
| `let` after a branch | fail | pass |
| `while` loops | no facet | pass |
| Member functions, `derived` | — | pass |
| Scalar `Buffer<Int>` | — | pass |
| Collection of aggregates | — | **fail** |

Collections of aggregates remain the single language-level gap.

### Adaptations the transform performs

Each one is mechanical and reversible:

- `@many` column declarations become `Buffer<Element>`, including the
  parameterized `@many(.ordered)` form.
- `Byte` maps to `Int`: `Buffer<Byte>` becomes `Buffer<Int>` and `Byte(bits: x)`
  becomes `(x)`. `Byte` is a width descriptor, so this is value-preserving.
- `firstIndex`, `append(contentsOf:)` and `slice` become shim free functions,
  since the predecessor Buffer lacks them.
- Macro declarations are stripped, and applications other than `@extern`,
  `@opaque`, `@main` and `@builtin` are dropped as validation-only.
- `print` and `diagnostic` are supplied as shims over `stringPrint` and
  `stringDiagnostic`, which are still present in `Language/Runtime`.
- `String` is substituted with the predecessor-era `@builtin(.storage)` form.
  It keeps a `bytes` member, so the 54 `.bytes` sites still resolve.
- Line comments are stripped: the predecessor scans the word `macro` inside a
  comment as a declaration.

### Ordering trap

`rewrite_collections` must run **before** `strip_applications`. Reversed, the
`@many` line is deleted together with the member declaration it annotates, and
`ProgramGraph` stages with all 46 columns silently removed. The bundle still
advances several compiler phases in that state, so this failure is easy to
mistake for progress. Verify staged output, not just the phase reached.

### Current frontier

The predecessor now parses, links macros, captures declarations, and runs
reachability-driven lowering across the real compiler call graph. It stops here:

```text
compilerError	kind=invalidFunctionDiscovery	function=fileURL	failureCode=206
```

`fileURL` builds `URL.components`, declared `@many let components: String`. That
is a collection of aggregates, the one remaining language gap. Earlier frontiers
already cleared, in order: `enumPayloadLink`, `topLevelCapture`, `macroLinking`,
and `fileManagerReadFile`.

### Aggregate collection conversions

35 sites total. 16 are in zero-function declaration files the bundle already
stubs, leaving 19 that need conversion to the column idiom the compiler uses
everywhere else:

- **Lowering diagnostics — done (2026-09-04).** `LoweringDiagnostic` became
  `LoweringDiagnosticStore`, five Int columns with the message held as
  start/count/bytes, matching `functionNameBytes` in `ProgramGraph`. 11 sites
  across Validation.range, Program.range, Lowering.range, Lifecycle.range and
  Assembly.range.
- **URL components — next.** `URL.components` is what the frontier is blocked
  on. Same byte-column treatment.
- **Backend object model — after that.** `RelocatableObject`, `ObjectSection`,
  `ObjectSymbol`, `ObjectRelocation` across Object.range and the four Mach-O
  sources.

### Correction to the 2026-09-03 entry

The "227 declaration lines" figure was an artifact of iteratively excluding
files: dropping a type declaration breaks references from other files, so each
exclusion manufactures the next error. The real gap is the feature list above,
not a file count. Bisect by adding sources back from a clean baseline, never by
excluding them from a failing one.

## 2026-09-04 (later) — URL conversion and the String boundary

### Landed

- **`URL.components` converted.** `@many let components: String` became
  `URLComponents`, three Int columns holding start, count and bytes, with
  `urlComponentsCreate/Append/Element/Count/Destroy` beside it. The one external
  consumer, `projectRegistryResolve` in `Language/CLI/Registry.range`, reads
  through the accessors. Same shape as `LoweringDiagnosticStore`.
- **`compilerBReportProjectDiagnostics` now returns `Int`.** It was the only
  free function in the compiler returning `Void`, and the predecessor supports
  `Void` only on macros. It now returns the number of diagnostics reported
  instead of discarding both callee results, which is also more useful.
- **`String.destroy()` shimmed.** Canonical code calls it; the era construct
  exposes the destructor only as the free builtin `stringDestroy`.

### Frontier movement this session

Each fix advanced the predecessor's reachability-driven lowering:

```text
fileURL                        -> URL.components converted
compilerBProjectRevisionDestroy -> String.destroy shimmed
compilerBReportProjectDiagnostics -> Void return converted to Int
compilerBEmitProjectAssembly   <- current
```

`Testing/Tools/check-compiler-a-bundle` pins the current frontier and fails if
it moves in either direction.

### The String boundary — needs a decision

This is the next real obstacle and it is a design question, not a mechanical
one. The two Strings are incompatible in opposite directions:

- **Era String** is `@builtin(.storage)` with an opaque buffer. Literals and
  `String("")` work. Its `bytes` member is declared but vestigial, so canonical
  reads like `text.bytes.count` and `symbol.name.bytes` would compile and be
  silently wrong at runtime.
- **Current String** is a real byte column. `bytes` is genuine, but the
  predecessor rejects `String(bytes:)` construction inside `substring`, and
  without `@builtin(.storage)` it is not the literal type at all.

Marking the current String with `@builtin(.storage)` was tried and moves the
failure to `substring`. Substituting the era String is what currently reaches
the furthest frontier, and is what the bundle does today.

Three routes, in rough order of appeal:

1. **Remove canonical dependence on `String.bytes`.** 54 sites, though most are
   `Binary.bytes` and `ObjectSection.bytes` rather than String. The genuine
   String sites are far fewer: three writes (`Target.range`, `Program.range`,
   `URL.range`) and a handful of reads. This converges String on one idiom and
   is code that survives the bootstrap.
2. **Shim a byte-append primitive.** Add `stringAppendByte` over the era
   primitives and rewrite the three write sites in the transform. Blocked on
   ambiguity: `Binary`, `URLComponents` and `String` all expose `bytes`, so a
   pattern rewrite cannot tell them apart. Would need exact per-site
   replacements, which is fragile.
3. **Give the bundle a Range-level String over `Buffer<Int>`** that the
   predecessor accepts as literal storage. Cleanest in principle, but requires
   the predecessor to construct the literal type from a member initialiser,
   which is what it currently refuses.

Route 1 is the only one that leaves the codebase better afterwards. It should be
decided before more transform work, because routes 2 and 3 are discarded at the
fixed point while route 1 is not.

### Remaining aggregate collections

19 real sites total. 11 done (diagnostics), 3 done (URL). The backend object
model remains: `RelocatableObject`, `ObjectSection`, `ObjectSymbol`,
`ObjectRelocation` across `Object.range` and the four Mach-O sources.

## 2026-09-04 (Route One) — String convergence and dangling calls

### Route One landed

`String.bytes` is no longer part of the compiler's working vocabulary:

- `stringAppendByte` added to `Language/Core/DataTypes/String.range`, built only
  from `count`, `byte`, `substring` and `append`, so it behaves identically on
  the current String and the predecessor-era one.
- `binaryAppendString` and `binaryFromString` added to `Binary.range`; the nine
  `binaryAppendBytes(..., bytes: X.bytes)` String callers across MachOLinker,
  MachODynamicLink and MachOObject now go through them.
- The three genuine String write sites (`Target.range`, `Program.range`,
  `URL.range`) use `stringAppendByte`, and the two Mach-O name readers build
  their String directly instead of via a byte buffer.
- `URLComponents.bytes` renamed `componentBytes` to remove the collision with
  `String.bytes` and `Binary.bytes`.

One dependency remains by design: `binaryAsString` in `Binary.range` converts
arbitrary bytes and cannot use a printable path. It is reachable only from the
linker and code-signing path, not from the assembly path Generation One needs,
so it is deliberately left alone.

### Eight dangling call targets, 62 call sites

A whole-program undefined-reference scan found calls with no definition
anywhere in the 101 canonical sources:

```text
compilerBExecutionNoPredicate            37 sites   stale rename
compilerBProjectDeclarationRowForToken   14 sites   lost with LLVM.range
compilerBProjectFunctionReturnType        4 sites   lost with LLVM.range
compilerBProjectEntryFunctionDeclarationRow  2       lost with LLVM.range
compilerBProjectMainEntryRow              2 sites   lost with LLVM.range
compilerBExecutionCursorReady             1 site    stale rename
compilerBExecutionCursorComplete          1 site    stale rename
compilerBProjectExternSymbol              1 site    lost with LLVM.range
```

Three were stale names left behind when `compilerBExecution*` became
`semantic*`; those call sites now use the current names. Five were deleted along
with `Core/Project/LLVM.range` during the LLVM cleanup and are ported back into
`Language/Compiler/Revision.range`, adapted to `SyntaxRevision` and the
`firstIndex` idiom, together with the `compilerBProjectStoredBytesEqual` helper
they need.

This is worth dwelling on. The compiler has been calling six functions that do
not exist, in `Assembly.range`, since the LLVM cleanup. Every structural gate
stayed green throughout, because none of them compiles that file. Keep the
undefined-reference scan; it is cheap and it found a class of defect that
nothing else in the repository was looking for.

### Two transform bugs, same shape as the first

Both produced apparent phase progress on wrong input:

- The `firstIndex`, `append(contentsOf:)` and `slice` rewrites matched only the
  single-line call form. Multi-line receivers kept an unrewritten member call,
  which the predecessor reports as arena failure code 206, "call labels do not
  match a member runtime builtin".
- `stringAppendByte` lives in `String.range`, which the bundle substitutes
  wholesale, so the helper disappeared from the bundle. The substitution now
  carries the canonical free helpers across.

### Frontier movement

```text
compilerBEmitProjectAssembly       -> six ported functions, three renames
urlComponentsElement               -> helpers carried across substitution
compilerBProjectEntryFunctionDeclarationRow -> multi-line rewrites fixed
graphAppendOperation               <- current
```

### Current frontier: member assignment through a path

```text
compilerError kind=invalidFunctionDiscovery function=graphAppendOperation failureCode=2
```

Capability stage 2 is `compilerBodyArenaBuildSymbolsAndResolutions` failing. The
statement is:

```range
lowering.lastEffectOperationID: operationID
```

Assigning to a construct member through a parameter. The predecessor has no such
form. Removing that one line moves the frontier to the next such site.

There are 14 sites in four files: 10 in `Graph/Lowering.range`, 2 in
`Backend/AppleArm64Encoding.range`, 1 in `Backend/AppleArm64Program.range`, and
1 in the already-dead `construct FileManager`.

**This is not only a predecessor limitation.** `graphLowerAssignmentRow` in the
current lowering resolves an assignment target by matching a single identity
token against local bindings, and has no member-path handling at all. So these
14 sites cannot be lowered by the current compiler either. They are aspirational
code that nothing has ever compiled, which makes restructuring them a fix rather
than a concession.

The natural form is the one `ProgramGraph` already uses: hold the mutable
scalars in an indexed column and go through `update(element:index:)`, or in the
encoder's case append to the existing diagnostic String rather than replacing
it. That decision is the next step.

## 2026-09-04 (cont.) — member assignment cleared, ownership phase reached

### Landed

- **Member assignment removed.** `FunctionGraphLoweringState`'s three mutable
  scalars became single-element `@many` columns with named accessors
  (`loweringCurrentBlockID`, `loweringSetCurrentBlockID`, and so on): 10 writes
  and 13 reads routed through them. `AppleArm64Encoding.valid` became a column
  behind `appleArm64Valid` / `appleArm64Invalidate`, and its `diagnostic` now
  appends rather than being replaced. The only member assignment left in the
  compiler is inside the already-dead `construct FileManager`.
- **The bootstrap driver moved out of canonical sources.**
  `Language/Bootstrap/CompilerA/GenerationOneEntry.range` now creates, uses and
  destroys the project revision inside one body. The predecessor cannot prove
  ownership for a function returning an owned aggregate, which is what
  `compileProject` does when it returns a `Revision` wrapping the compiler
  revision. Keeping that shape confined to the bootstrap entry leaves
  `compileProject` untouched for the CLI path, and the entry is deleted at the
  fixed point.

### A new phase, not yet characterized

The predecessor now clears declaration capture, macro linking, and
reachability-driven function discovery across the whole compiler call graph. It
fails in the ownership phase:

```text
compilerError kind=invalidFunctionReachability stage=6014
  function=generationOneCompile ownedPathStage=14
```

Stage 6014 is `6000 + memoryBuildStatus`, so owned-path stage 14.

What is ruled out, each tested directly:

- Not `URLComponents`. An era-shaped `CompilerBProjectDeclarationStore` and a
  `ProgramGraph` fail identically.
- Not nesting. It fails whether the store is created in the entry, one call
  deeper, or directly under `@main`.
- Not `state` versus `let` on column members. Staging them as `let`, matching
  the era declaration form exactly, changes nothing.
- Not the `bufferSlice` shim returning an owned `Buffer`.
- Not Strings. Creating and destroying a String in the entry passes.

So: creating and destroying any buffer-owning construct anywhere reachable from
the entry is rejected, while the era sources doing the same thing still compile
cleanly (`check-compiler-a-successor`, 29,088 lines). The difference between the
two bundles has not been found.

This is the first obstacle of the session that is not a defect in the current
sources. The previous ones were all real bugs: dangling calls, a lost function
set, member assignment that neither compiler supports. This one is a predecessor
analysis that accepts the era's sources and rejects today's, for a reason not
yet isolated.

### Suggested next probes

1. Bisect the two bundles against each other rather than probing forward. Build
   the era bundle, then swap in today's sources one subsystem at a time until
   ownership breaks. That locates the difference instead of guessing at it.
2. Check whether the ownership phase is even required for the assembly path.
   `failureStage == 4` gates the owned-return branch; if the phase can be
   satisfied by a narrower entry that touches fewer owned constructs, Generation
   One may be reachable without solving the general case.
3. Read `compilerBodyArenaBuildMemoryFacts` in the predecessor sources for what
   returns positive 14. The optional-coalescing rule at `CompilerBodyOwnership`
   line 3599 returns `0 - 14`, which is a different path and not this one.

## 2026-09-04 (recovery seed) — front end complete

### Direction change

The bootstrap is now a **recovery interpreter written in C** under
`Language/Recovery/`, not a transpiler and not Python. It executes canonical
Range compiler sources and emits nothing: no C, no LLVM, no assembly, no object
code, no executable format. Every emitted byte remains the responsibility of the
Range compiler itself, through Range-authored ProgramGraph lowering, ARM64
encoding, Mach-O generation, linking and signing.

It is recovery infrastructure, not a second compiler authority. It is never
invoked by the public `range` command or by ordinary project builds.

### Stage one landed: lexer, parser, AST

Real front end, no regex source rewriting:

```text
Language/Recovery/Interpreter/lexer.c      re-entrant over explicit spans
Language/Recovery/Interpreter/ast.c        arena-backed nodes, source locations
Language/Recovery/Interpreter/parser.c     recursive descent, precedence climbing
Language/Recovery/Manifest.json            hash-pinned, 7 sources, 44,153 bytes
Language/Recovery/Tools/build-recovery-interpreter
Testing/Tools/check-recovery-parser
```

All 101 canonical sources parse, unmodified:

```text
nodes=104145 construct=158 enum=23 function=797 macro=33 failures=0
```

The gate verifies the sources are byte-identical before and after parsing, that
the public CLI does not reference the recovery seed, and that malformed input
fails with a file:line:column location rather than being skipped. Five probes
cover bad characters, bad escapes, unterminated constructs and unterminated
templates.

### Language forms the front end handles

Interpolated strings with nested parentheses and quotes, `#environment` and `$`
template references, union type positions, optional types, external/internal
parameter label pairs, macro signatures with `->` result types, inferred member
types (`let author: "George"`), and payload enum cases.

### Recorded exception

One, and it is in the manifest rather than in code comments alone:

- **`@syntax` templates.** These declare surface grammar, not expressions. The
  parser retains the template span verbatim and never interprets or rewrites it.
  If the evaluator ever needs one at run time, that is a loud failure, not a
  silent skip.

Nothing else is stripped. Macro declarations, attributes and annotations are all
retained in the AST.

### Front-end proof boundary corrected

The first pass overclaimed. Corrections, all verified:

- **Macros are parsed in full.** `parseMacro` previously skipped signatures by
  paren depth and retained bodies as raw spans, so "these forms are parsed" was
  false. Parameters, both default forms, target and result types, and bodies as
  real blocks are now in the AST. Node count rose 104,145 to 104,856.
- **Forms this exposed**, each now parsed rather than skipped: `#environment`
  emission blocks, trailing closures, `switch` with relational patterns,
  `extension` declarations, `@any` collection literals, standalone macro
  application statements, inferred type positions on locals as well as members,
  macro parameter defaults in both syntaxes, and function declarations inside
  macro bodies.
- **A bodyless function must be `@extern` or `@builtin`.** Anything else is a
  parse failure. This came out of a probe that the parser was wrongly accepting.
- **`@syntax` is the only opaque form**, recorded in the manifest with its
  justification and the evaluator contract that requiring one must fail loudly.
- **`lexdump.c` removed.** The gate now asserts that every file in the recovery
  directory is hashed by the manifest, so no unlisted source can sit inside the
  trust boundary.
- **The gate asserts the pinned census**, field by field against
  `Manifest.json`, not merely `failures=0`.
- **Structural assertions added** for eleven difficult forms, so a permissive
  parser cannot silently omit them. A `--tree` mode dumps the AST for this.
- **Probes went from four to thirteen**, adding well-tokenized syntax failures:
  malformed parameters in two positions, invalid declaration and member
  structure, trailing and stray tokens, a missing block, a bad macro signature
  and an incomplete switch arm. Every failure must carry `file:line:column`.
- **Public-path isolation widened** past `Language/CLI` to `install-range`,
  `Language/Bootstrap/Tools`, `Language/Core/Package`, `Distribution` and
  `Language/Compiler`.
- **The CI syntax check was broken.** `bash -n Language/Bootstrap/Tools/*`
  passed a directory to `bash -n` whenever a `__pycache__` existed, which my own
  debugging had created. It now enumerates files and tests each shebang.
  `xargs -r` is not portable to the `macos-15` runner, so the check loops
  instead. The stray `__pycache__` is deleted.

All workflow steps were run in order, exactly as CI runs them, after these
corrections.

### Next: the vertical slice

Per the plan, before the full interpreter: primitive value semantics, reference
identity for constructs, buffer-owning constructs through functions,
caller-visible mutation, nested aggregate construction and returns, strings and
interpolation, `@many` create/element/append/update/slice/destroy, payload
enums, member and free-function calls with labeled arguments, control flow with
early returns, and extern calls through the runtime boundary. Aggregate identity
uses deterministic interpreter heap handles; no ownership analysis is
reproduced, but logical destruction and invalid use must be explicit and
deterministic.
