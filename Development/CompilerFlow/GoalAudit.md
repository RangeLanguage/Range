# Core-to-execution audit — 2026-09-06

This is a bounded architecture audit, not the deferred macro manifesto. It
supersedes older CoreAudit.md statements where later decisions changed the model.
Evidence comes from the current checkout and focused recovery probes, not memory,
historical benchmark claims, or a successful native self-hosting run.

## Goal and present conclusion

Write the intended language and compiler in Range, then provide an external engine
capable of executing those definitions until the compiler can build itself.
The graph remains authoritative; derived machine facts retain source identities.

Core currently declares portions of that model. Handwritten recognition, recovery
semantics, and name-based lowering still supply significant meaning independently
of those declarations. The recent recovery improvements are useful scaffolding;
they do not close the Core-to-execution contract. Pause further independent control
flow, ownership, and evaluator expansion until the next slice exercises that contract.

Original September 6 inventory: CompilerSources.txt lists 101 files, including 65 Core files and 14
@syntax applications. No canonical declaration of literal, Environment, or Type
was found. Referencing a name in a signature does not define its behavior.
The September 8 updates below record later decisions and the bounded generic
syntax implementation; the original execution probes remain dated evidence.

## Status against the agreed design

Statuses are independent: something can be declared and still implemented through
hardcoded machinery. “Implemented” below names the specific proven portion.

| Area | Defined/agreed | Implemented evidence | Hardcoded or unresolved |
| --- | --- | --- | --- |
| Project collection | One sequential compilation over Core/project sources | Main.range coordinates discovery, population, validation, and output | SourceSet.range starts with project paths and appends Core only when Project.range exists; this is not the proposed explicit Core-registration-first contract. |
| Syntax | Templates annotate concrete syntax constructs | Range template matching, capture/cardinality checks, materialization records | Syntax.range has an empty body; materialization first invokes the handwritten parser and selects recognized node categories. Templates do not yet define recognition end to end. |
| Literals | Collected literal rules supply numeric/boolean meaning independently of a chosen concrete integer | Recovery lexer/parser/evaluator handle literals | No literal macro/rule collection exists. C parses numbers with strtoll into signed 64-bit nodes; Range tokenization also recognizes literals directly. Regex notation and match result remain proposals. Destination selection was subsequently agreed below; it is not implemented. |
| Members | RHS retains syntax; resolved construct reference is a requirement, value-member reference supplies a value | Complete consumed RHS spans; empty-call distinction; ordinary bare-name expression records; recovery local and preceding-member resolution | Canonical semantic connection remains incomplete. Some legacy type-shaped paths remain, including general trailing expressions and input materialization. |
| Inputs | Explicit Let or Binding, single label, direct Function/Macro inputs | Parsing and negative tests | Separate parameter records remain; Let-or-Binding capture/materialization is not complete. Binding input execution is rejected. |
| Applications | Target identity plus arguments; defaults are supplied by members | Recovery constructors and calls, bounded default/reference handling | Core argument constraints and effective-value relationships are incomplete. Scalar constructors retain special cases and do not generally follow the new Int members. |
| Integer | value, bits, signed; integer validates representability | integerValueFits executes with boundary checks | Macro body reads declaration syntax via .value, not resolved application values. Missing-member/fit diagnostics are draft code; width-dependent representation is not derived from this macro. |
| Storage/ownership | Property-level rules; ordinary value copy; bindings preserve connections; derived is reactive | Recovery scalar copying and bounded arena execution | Storage macro is empty. Aggregate COW and reactive execution are not established by recovery tests. Prior proofs/performance have not been reassessed in this audit. |
| Control flow | Definite initialization before reads; single let initialization; unused locals may remain uninitialized | Recovery structured-path analysis, including branches, returns, loops, supported exhaustive switches | These are C execution rules, not evaluated Core definitions. Canonical graph analysis and broader coverage remain separate. |
| Assignment | Identity resolution distinguishes initialization from later mutation | Recovery allows first initialization of let | Proposed Assignment.range is outside the canonical manifest and only accepts State or Binding. It does not express first initialization of Let. |
| Macro environment | Context should expose graph relationships without repeated ad hoc traversal | Existing query/emission syntax and specialized compiler processing | No canonical Environment definition or general invocation/result/effect contract; @many emission style was explicitly questioned, not reaffirmed. |
| Native output | Integer meaning is separate from physical register/memory placement | Range ProgramGraph lowering and ARM64 encoders exist; mov/ret encoder fixture executes | Lowering still maps type names to built-in layouts. General Int(bits: 24) support cannot be inferred from an instruction-encoding proof. |

## One program, from source to ARM64

Use an explicitly selected entry for analysis, so the unresolved entry macro does
not get disguised as part of the integer proof:

```range
function main(): Int {
    return Int(value: 42)
}
```

1. **Discover sources.** Main.range:compileProject calls
   compilerBDiscoverProjectSourcePaths and creates the project revision. The
   discovery function filters .range paths; Core inclusion is currently conditional
   on a Project.range file. Do not claim this establishes the intended registration
   order or a graph-only project discovery mechanism.
2. **Recognize source.** SyntaxMaterialization.range:compilerBMaterializeSourceSyntax
   calls compilerBParseSyntaxRevisionWithResolution first. Its work list contains
   already recognized declarations, members, blocks, and selected applications.
   The parser, not the executed syntax macro, presently recognizes the function,
   return, call, label, and literal.
3. **Materialize and resolve.** Template and capture records are real. Function
   inputs, arbitrary RHS resolution, literal materialization, and a fully specified
   macro environment are still missing connections. A literal-producing macro is
   not invoked for 42.
4. **Apply Int and its defaults.** At the time of this trace, intended effective members are value=42,
   bits=64, signed=true. The literal rules must supply these meanings without
   recursively requiring a concrete Int for its own configuration. Actual recovery
   execution fails here on the bare true default; see probes below.
5. **Run integer validation.** The required-member and fit checks should inspect
   effective values and issue source-located diagnostics. Currently the standalone
   numeric helper runs, while the macro/environment integration does not.
6. **Lower the result.** Assembly.range:compilerBEncodeProjectObject selects an
   emitted @main entry, calls lowerProgram, then encodeProgram. Graph/Lowering.range
   records operations and value identities, but graphTypeNamed still maps Int to
   an eight-byte scalar type by name (Byte shares the one-byte scalar type used by
   Bool). This is not representation derived from integer's configurable members.
7. **Encode instructions.** AppleArm64Operations.range's constant path moves the
   immediate through register 9 and stores it in an eight-byte value slot. Return
   lowering records a return operand. Do not claim the existing pipeline necessarily
   emits the optimized two-instruction example. The separate encoder fixture proves
   exact bytes for mov x0, #42 and ret, not this whole source-to-program path.
8. **Produce executable output.** Main.range also handles object/runtime inputs
   and linking. No full native compilation of this example through the agreed Core
   semantics was demonstrated in this audit.

## September 6 probes and what they establish

Built the recovery interpreter from the then-current pinned sources. Ran these
functions with the September 6 Int, Byte, Bool, and String Core files:

| Return expression | Result |
| --- | --- |
| Int(42) | exit 0, result=42 |
| Int(value: 42) | exit 65, Int.range:5:17: unresolved name 'true' during binding |
| Int(value: 42, bits: 24, signed: false) | same failure |

The legacy one-unlabeled-argument scalar path bypasses ordinary member handling.
The failure on true is the first observed failure, not evidence that fixing it
alone completes integer construction. Named construction, primitive contents,
macro execution, and lowering still need the connections described above.

The parser/evaluator gates are also rerun for this audit. Their green status is
compatible with these failing probes because they prove different, narrower paths.
No compiler or Core behavior was modified to obtain these results.

## Agreed literal destination selection — 2026-09-08

This is a design update to the September 6 audit. The destination-selection rules
are not implemented compiler behavior. Examples use the selected notation:
`<...>` supplies type-level members and `(...)` supplies ordinary inputs. Generic
syntax is now represented by the bounded parser pass below; specialization and
effective-value resolution remain implementation work.

An explicitly selected integer representation checks the literal directly against
its own width and signedness. That representation can come from the construction
itself or from an expected type declared on a member, function input, or function
result. Use the project's default integer type when no such constraint supplies
the type. An oversized literal does not silently select a wider type.

```range
// Project default: signed 32-bit integers.
let small: 3
// Uses the default; valid.

let tooLarge: 3000000000
// Uses the default; exceeds its signed 32-bit range.

let alsoTooLarge: Int(value: 3000000000)
// No explicit type-member override; the default still applies.

let large: Int<bits: 64>(value: 3000000000)
// Checks against signed 64 bits directly; the value fits.
```

The last case must not first construct a default 32-bit integer and then convert
it. Literal materialization must preserve the matched value until the selected
representation can validate it. Its concrete graph representation remains part
of the next materialization contract.

Backend support is a separate check: the last case also requires the compiler to
support that representation on the chosen target. The project's default width is
not a maximum supported width.

The same rule applies wherever the expected type is specified; the following
examples also use a project default of signed 32-bit integers:

```range
function accept(let value: Int<bits: 64>) {
}

accept(value: 3000000000)
// The parameter supplies the 64-bit constraint; the literal fits.

function largeValue(): Int<bits: 64> {
    return 3000000000
}
// The result supplies the 64-bit constraint; the literal fits.

construct Counter {
    let value: Int<bits: 64>
}

let counter: Counter(value: 3000000000)
// The member supplies the 64-bit constraint; the literal fits.
```

The generic members make width and signedness part of the selected integer type.
Expected-type propagation applies that type's constraints to the literal; these
positions do not each have a separate rule for applying the project default.

### Existing integer values require explicit conversion

For the current design, integer types with different width or signedness generic
arguments are distinct types. An existing value requires an explicit conversion
to cross that boundary, even when its value is known to fit or every possible
source value fits the destination. There is no implicit widening.

```range
function accept(let value: Int<bits: 64>) {
}

let count: Int<bits: 32>(value: 3)

accept(value: 3)
// Valid: the literal receives the parameter's 64-bit type directly.

accept(value: count)
// Error: count already has a 32-bit type; explicit conversion is required.

accept(value: Int<bits: 64>(value: count))
// Valid: ordinary construction explicitly converts count's value to 64 bits.
// count itself retains its original 32-bit type and value.
```

This preserves the literal rule above: supplying a literal's expected type is
different from converting an already typed value. Ordinary construction is the
agreed explicit conversion form: the destination integer receives the existing
numeric value, and its width and signedness determine whether that value fits.
The form and the checked failure behavior below are agreed but not implemented.

### Out-of-range construction and conversion

Ordinary integer construction and explicit conversion must preserve the numeric
value within the destination's representable range. If an out-of-range value is
known during compilation, report a source-located compilation error. If its fit
cannot be established until runtime, check it then and trap on failure with a
clear integer-conversion-out-of-range diagnostic. A value that fits succeeds.

```range
let small: Int<bits: 8, signed: true>(value: 3)
// Valid: 3 fits in the signed 8-bit range, -128 through 127.

let tooLarge: Int<bits: 8, signed: true>(value: 300)
// Compilation error: the out-of-range value is known during compilation.

function narrow(let value: Int<bits: 64>): Int<bits: 8, signed: true> {
    return Int<bits: 8, signed: true>(value: value)
}
// For values received at runtime: 3 succeeds; 300 traps at the conversion.
```

This settles the failure policy for ordinary integer construction and conversion.
The Core-to-graph connection and the emitted runtime check remain implementation
work. This decision does not define broader arithmetic-overflow behavior.

## Generic syntax and graph relationships — 2026-09-08

Agreed and now represented: generic headers contain explicit `let` members only.
Ordinary function and macro inputs retain the existing `Let | Binding` rule.
Named generic arguments select type-level members; ordinary arguments supply
instance members or function/macro inputs. No new member category was introduced.

```range
@integer
construct Int<let bits: 64, let signed: true> {
    let value: 0
}

function echo<let width: 64>(let value: Int<bits: width>): Int<bits: width> {
    return value
}

let required: Int<bits: 32>
// A type requirement; it does not construct an instance.

let defaulted: Int<bits: 32>()
// Explicit construction, even with no ordinary arguments.

let supplied: Int<bits: 32>(value: 3)
// Generic arguments and ordinary arguments remain separate relationships.
```

Core Construct, Function and Macro declare `generics: Let` collections and their
generic syntax templates. Application retains generic arguments separately from
ordinary arguments. Int and Byte place `bits` and `signed` in generic headers;
`value` remains an instance member. Integer's draft lookup follows the generic
relationship for width and signedness. It still reads declaration syntax, not an
application's effective values.

Both the C recovery parser and the Range parser retain generic declaration
members and named arguments. The Range parser reuses the existing member and
argument facet shapes in separate collections with owner/source relationships.
Focused execution tests verify nested specializations, specialized function and
macro signatures, multiline headers, ordinary comparisons, and the distinction
between a specialized reference and an explicit empty call. Invalid member kinds,
omitted `let`, missing values, missing separators and second-colon defaults fail.

Evidence from this pass:

- The C parser reads all 101 canonical sources; its pinned census is
  `nodes=106955 construct=154 enum=23 function=805 macro=33 main=0 failures=0`.
  The parser gate passes 24 malformed-input probes and structural assertions.
- The evaluator gate passes seven ordinary execution fixtures and 48 rejection
  fixtures in both normal and ASan/UBSan builds. Its canonical Range parser
  generic fixture also passes, alongside the existing parser and ARM64 checks.
- Explicit generic execution is rejected. Generic function, signature, field and
  construction requirements have rejection fixtures, including untaken code.
  Canonical resolution and syntax materialization also reject generic syntax
  explicitly while those stages cannot preserve and resolve it.

This proves parsing and graph structure, not specialization, project-default
selection, literal materialization, conversion or native output. The recovery
interpreter's legacy Int/Byte scalar intrinsics remain fixed representations;
they do not execute the new Core generic defaults. The earlier September 6
construction failures are historical and must not be presented as current probes.
The canonical Range parser's broader qualified/chained specialization and generic
macro-application paths remain unfinished; specialized union parsing is also
outside this pass. No full-compiler or native self-hosting claim follows.

## Applications expose their member connections — 2026-09-08

Agreed: applications are part of the program's source graph. Ordinary Range code,
including a macro attached to a declaration, must be able to query an application's
target and the connections between its arguments, the declared members they
supply, and any selected defaults. Integer may use those relationships to inspect
an Int application and define behavior from its selected width, signedness and
supplied value. This is a general graph capability, not an integer-only lookup.

An argument supplies a value for a declared member, overriding its default for
that construction when one exists. The User example states the same rule as Int:

```range
construct User {
    let name: "Default"
}

User()
// name uses its declared default: "Default".

User(name: "George")
// name uses the supplied override: "George".
```

The proposed extra Application node for each named override is withdrawn. Keep
the ordinary application, its supplied arguments and their member relationships;
do not recast supplying an override as another application. If a supplied
expression is itself an application, such as readNumber(), retain that expression
normally.

```range
@integer
construct Int<let bits: 64, let signed: true> {
    let value: 0
}

let first: Int<bits: 8>(value: 42)
// bits -> generic member bits, supplied by 8
// signed -> generic member signed, supplied by its default true
// value -> instance member value, supplied by 42

let second: Int<bits: 16>()
// bits -> generic member bits, supplied by 16
// signed -> generic member signed, supplied by its default true
// value -> instance member value, supplied by its default 0
```

The connections belong to the particular application; inspecting first and second
must not overwrite the original declaration's members or each other's selections.
Generic and ordinary member relationships remain distinct. Resolution retains
member identities and the source expressions supplying them, including the origin
of defaults. It does not require every expression to have a compile-time result:

```range
Int<bits: 8>(value: readNumber())
// The graph identifies the expression supplying value. If its numeric result is
// only available at runtime, the agreed representability check happens then.
```

Current evidence: CompilerBApplicationFacetStore already has declarationSyntaxIDs;
CompilerBApplicationArgumentFacetStore records application, label, value-expression
and ordinal relationships, but no resolved member identity. Core Application
still declares generic and ordinary arguments as syntax collections. Integer's
draft body follows declaration members and therefore does not obtain a particular
application's selected values. No implementation or execution proof of the new
query contract is claimed by this decision record.

The concrete Core representation and query spelling remain to be designed using
existing identities, members and applications. Macro access to the graph is agreed;
the invocation and emission contract for applying that behavior to each relevant
application remains unfinished. Do not silently introduce an automatic callback,
change the macro's attachment target, or evaluate runtime expressions while
resolving their graph connections.

## Core model, compiler linking, and the open Application scope — 2026-09-08

Agreed responsibility boundary: Core defines the language forms and their meaning.
The compiler executes that model: it materializes syntax, resolves targets, matches
arguments to declared properties/parameters/generic members, and establishes the
queryable graph relationships. Core macros can inspect those relationships and
express behavior. Core need not contain the machinery for performing each match.
This boundary does not authorize independently hardcoded language meaning in the
compiler or the C bootstrap.

Properties and parameters belong to declarations; arguments belong to applications.
The earlier suggestion of an Application-owned members collection is withdrawn.
The graph can expose the argument/member mapping and declaration defaults without
moving members onto Application.

The general Application design itself remains under discussion. Agreement on graph
access and matching is not approval to replace every former target-specific form
with one general Application, or to implement a new matching pass yet.

Historical evidence: before f62600522 (August 28), Construct explicitly contained
Declaration and Application constructs under @graph(.declaration, .application).
Declaration held members; Application held type and arguments. Function likewise
contained separate Declaration and Application forms. The consolidation removed
the older Construct definition and introduced the standalone Application form.
Construct.Application was therefore an explicitly declared historical form, not
merely an already-defined filtered view of today's Application.

The question extends beyond construct/function/macro/enum uses to references and
member access:

```range
construct Counter {
    let count: Int
}

let counter: Counter(count: 42)
// Construct application; the count argument supplies the declared property.

let snapshot: counter.count
// Member access; its target is the declared count property on this receiver.

function echo(let count: Int): Int {
    return count
    // A bare reference to the declared input member.
}
```

Open design question: does Application broadly represent a use of a declaration,
including count and counter.count, or is it narrower, with reference/member-access
forms represented separately? If the broader meaning is chosen, the graph must
still retain the distinctions between reading a member, selecting a type,
constructing a value, and calling a function, together with the receiver where
applicable. Calling a reference an application does not by itself introduce a
runtime call or make count and count() interchangeable.

At the same historical revision, Let was a direct Let<Value> property construct
with identity/value and initializer/getter functions; it did not itself declare
nested Declaration and Application constructs. Extending the paired model to
member reads would therefore be a design choice, not simply restoring a uniform
historical arrangement. Likewise, the withdrawn extra node for a named argument
override is distinct from this question about a reference expression.

No choice between these representations is recorded yet. Continue this Core-model
discussion before changing the parser, graph schema, matching, or macro execution.

## Collection predicates and explicit type selection — 2026-09-08

Ordinary value filtering supplies a predicate over collection elements. The caller
can access a known element's properties in that predicate; filter itself need not
discover those properties through the macro environment. The earlier example
filter(named: "George") incorrectly conflated this with the user's heterogeneous
resolution lookup. The current CollectionFilter.range body and its @many -> @any
signature are drafts, not an agreed general-purpose filtering contract.

Agreed: heterogeneous type selection accepts an actual type reference and returns
a collection statically narrowed to that type. Do not introduce an `is` keyword
for this operation, or replace the type reference with a string. The user suggested
for: or named:; for: is the proposed spelling below, not a separately settled label.

```range
construct Triangle {
    let base: Int
}

construct Circle {
    let radius: Int
}

@many
let shapes: Triangle | Circle

let triangles: shapes.filter(for: Triangle)
// Select Triangle values; the result's element type is Triangle.

let wideTriangles: triangles.filter { triangle in
    triangle.base > 10
}
// An ordinary value predicate; the result's element type remains Triangle.
```

These are design examples, not a claim that the current compiler executes these
calls. The selected type determines the static output element type even when no
values match. Which elements survive may depend on runtime values. This contract
does not require inferring type narrowing from arbitrary Boolean predicates.

Inspection found Function declaration metadata, but no complete Core contract for
typed function-valued inputs or closures. CollectionMap and CollectionEach refer to
Closure without a Core declaration. Recovery retains trailing closure syntax but
does not execute it. Core also has no macro definitions for any or one; their use
does not establish a supported cardinality or type-erasure rule.

The concrete callable/type-value signatures, type-selection implementation, and
function-versus-macro organization remain undecided. Sharing modifier declarations
and emitted implementation code is a separate concern from making an operation
available on a collection. No new compiler behavior is implemented by this record.

Typed heterogeneous filtering is now deferred. Preserve the desired shape above;
do not make its implementation or the choice between for: and named: a prerequisite
for ordinary generic collection operations.

## Shared collection operation attachment — next design discussion

The immediate question is how one generic filter function becomes available on
every @many collection. Many.range already collects @collectionModifier declarations
and uses an extension emission draft to expose them. Agreed: collectionModifier
targets Function only. Collection operations are ordinary functions; Macro is not
an additional accepted target. CollectionModifier.range now records that target.
Merely restoring generics does not establish the attachment or receiver binding.

The existing marked append, filter, each, map, slice and isEmpty macros are legacy
drafts incompatible with this target restriction. Their migration to functions is
pending the receiver contract below; changing the marker alone does not implement
or validate those operations.

Proposed direction, not yet an implemented contract: @many makes the shared marked
function available through collection member lookup. For users.filter(...), users
supplies the collection receiver and its element type supplies the function's
generic element type. Another collection uses the same function declaration with
its own receiver and element type. Decide how that receiver is declared and bound;
do not assume #environment.target is a runtime receiver inside the function.

Making a function available through the graph does not require copying its body
into every collection application. Machine-code sharing, specialization and
inlining are separate lowering decisions. The old target.Application extension
spelling is a draft, not a settled representation of this relationship.

## Later discussion and deferred implementation slice

After the collection attachment discussion, settle the intended scope of Application using declaration, argument,
reference and member-access examples above. Then inspect compiler matching against
that model and connect the literal rule's source match, materialized result and
destination constraint to the graph relationships. Define how integer reaches the
relevant application through the macro environment. Do not invent a new wrapper or
silently make the old scalar special case the new language rule.

Then implement one vertical acceptance case: Int<bits: 8, signed: false>(value: 42),
with defaults and explicit overrides using the same mechanism. Require a negative
case at 256, and verify that changing the Core-defined acceptance/validation rule
changes behavior without changing a C type-name branch. Include Bool literal
handling because signed depends on it. Keep this proof distinct from full syntax
recognition and from final ARM64 encoding.

Only after this connection is real should the slice reach representation lowering
and the existing encoder. Defer general ownership/COW, richer control-flow proofs,
regex breadth, full self-hosting, and the manifesto. The next implementation must
reduce a named dependency on hardcoded language meaning, not merely add another
independently passing recovery feature.
