# Core audit: definitions versus engine behavior

Historical audit: see [GoalAudit.md](GoalAudit.md) for the 2026-09-06 refresh,
later member/literal decisions, current probes, and the proposed next slice.

Scope: the current canonical files listed in `Language/CompilerSources.txt`, with
focused reads of Core syntax/macros, the Range parser/materializer/lowering, and
the C recovery parser/evaluator. This is a source audit, not an execution proof.
No Core or compiler behavior was changed.

**Core expresses part of the intended language, but does not yet completely define
how to read or execute it.** The existing engine recognizes language forms first
and applies Core templates afterward. The desired engine must let those templates
drive recognition, then consume explicitly defined behavior or primitive contracts.

## 1. Make syntax definitions control recognition

Core currently contains 14 canonical `@syntax` applications across five files. They describe
Function, Enum, EnumCase, Body, Extension, Macro,
Application, Construct, Let, State, Derived, and Binding (some have alternatives).

`Language/Core/Macros/Syntax.range:1` declares `syntax(template: String)` with an
empty body. Its applications use raw blocks and unquoted token templates. The
special handling that makes those arguments possible lives in the engine.

`Language/Compiler/SyntaxMaterialization.range:1507` discovers templates by the
name `syntax`. `compilerBMaterializeSourceSyntax` at line 1910 first invokes the
handwritten parser. `compilerBConcreteSyntaxWork` at line 1859 selects already
recognized node kinds for matching. `syntaxTemplateExactMatch` at line 936 matches
literal/capture parts within those preselected token spans. There is useful matching
and ambiguity-diagnostic code here, but it is not the complete source reader.

Required change: establish template-driven recognition from input, including typed
captures, repetition/optional captures, nested forms, token/span boundaries, and
ambiguity. Decide explicitly whether lexical rules (identifiers, strings, comments,
delimiters) are initial engine primitives or additional Core definitions. The
current lexer implements those rules directly in `Core/Syntax/Token.range:118`.

Define how raw template arguments are captured before ordinary expression parsing.
Either document syntax registration as a primitive with this contract, or implement
its body using lower-level registration operations. An empty macro is not itself a
template engine. Do not silently treat it as a complete ordinary macro program.

## 2. Supply the missing forms and relationships

| Area | Current Core evidence | Required work |
| --- | --- | --- |
| Assignment | A proposed Assignment definition now exists outside the canonical source manifest. Its destination binds to State or Binding through identity; its source constraint remains provisional. | Implement the matching/resolution rule and settle the source constraint before registering it. Keep assignment distinct from declaration initialization. |
| Inputs | Function and Macro now directly declare inputs as Let or Binding. Each input has one name; construction syntax supplies defaults. | Implement contextual matching of implicit Let and explicit Binding inputs. No separate Parameter construct is planned. |
| Macro environment | `Macro` binds `Environment`; macro bodies use `#environment.target`, queries, and emission blocks. No canonical Environment declaration was found. | Define invocation scope, target, graph/query access, emission destinations, and completion dependencies. If supplied by the engine, declare its interface explicitly. |
| Types and captures | `macro type(): Type {}` refers to Type without a canonical declaration. `@syntax` is also used as a type/category for arbitrary syntax. | Define these categories and their relation to concrete node types. Specify which are engine-provided. Do not equate an arbitrary syntax value with the whole compilation graph. |
| Applications | Application's template describes `$target($arguments)` with an Identity target. | Account for qualified/member callees, labeled arguments, attribute applications, and their declaration bindings. They need explicit forms or documented composition rules. |
| Literals/operators | No Core syntax templates for numeric/string/bool literals or arithmetic operators were found in the template inventory. | Define recognition and operator precedence/association, plus links to primitive numeric/string operations where needed. |
| Control flow | No Core templates for return, if, while, or switch in this inventory. The abandoned unified-condition constructs have been removed. | Keep if and switch. Define their surface forms and execution/control relationships, including return propagation. |
| Body queries | Body declares derived declarations/applications without bodies implementing those views. | Define the generic projection primitive or implement traversal; specify scope and node identity. |

These are gaps relative to a self-describing Core. Missing declarations may represent
intended engine-provided types; the audit does not assume they are necessarily
accidental missing files.

## 3. Separate markers, primitives, and implemented macro programs

An empty body can be valid. It must be clear what consumes its application/result.

| Definition | Current state | Needed contract |
| --- | --- | --- |
| syntax | Empty declaration; compiler discovers applications by name and compiles their templates. | Template registration/matching primitive, or implemented wrapper around one. |
| storage | Empty declaration with immutable/mutable/derived/binding enum values. | How storage policy attaches to a node, and how reads/writes enforce it. |
| bool / integer | Bool is empty. Integer checks bits/signed members; it does not implement arithmetic. | Primitive representation and arithmetic contracts, including overflow and conversion behavior. |
| main | Empty Construct-targeted marker. | Ordinary build code must query it or an explicit result/relationship to derive an entry. No execution implied merely by presence. |
| print | Real body invokes write twice. | Execute that body; do not substitute behavior based on the name print. |
| write / diagnostic | Construct Execution(effect: Write(...)) / Diagnostic(...) in Void-returning macro bodies. | Specify how produced values are registered as effects/diagnostics, and at which execution time. A discarded construction cannot implicitly become an effect without a rule. |
| many | Declares empty collection operations and performs count/environment work. | Identify primitive collection operations versus generated Range implementations; implement query/emission semantics. |

`Core/Execution.range` contains the Execution data structure, not a complete
evaluator. `Core/MacroExecution.range` contains specialized execution, so execution
is not wholly absent, but its restricted condition handling does not implement the
general behavior implied by the Core bodies.

## 4. Keep the recovery claim honest

The current C recovery parser has explicit rules for declarations, assignments,
calls, and control flow. It retains `@syntax` templates opaquely and does not use
them to recognize programs (`Interpreter/parser.c:153`). Its evaluator implements
scalar/storage behavior directly and does not yet execute general macro bodies.

It can remain useful as a bootstrap for executing a limited Range implementation.
Its existing tests do not prove that Core determines the grammar. For that proof,
the Range matcher/engine executed under C must actually consume Core templates;
the test must not merely exercise the C parser's fixed grammar.

## Concrete order of changes

1. Inventory the initial engine primitives and define the template-loading boundary.
   Keep this explicit and small. Core's definitions must be loadable before they can
   describe the rest of Core. The later native engine must retain the same ability.
2. Define assignment's syntax and graph relationships in Core, plus the generic
   operation that applies its storage update. Fill the capture/type prerequisites
   needed for that one form. Merely adding an annotation will not change recognition.
3. Make the matcher recognize an assignment from input using that template, without
   first calling the existing assignment parser. Return stable target/value nodes;
   resolve the target through the graph and enforce its storage policy.
4. Execute a small Range harness through the bootstrap. Feed it an isolated Core
   template and program as data. Switch only the template from colon to equals;
   update that input program accordingly. Keep engine code identical. Both must
   produce the same assignment relationship and final value. Also check the old
   spelling is rejected when only the new template is registered, immutable writes
   fail, and nested value expressions have correct boundaries.
5. Expand the same mechanism to parameters, calls, macro applications, and control
   flow. Then make recursive Core/project collection use it and return the graph
   to an ordinary sequential compiler main.

This order replaces the earlier draft's suggestion to start with lowering retention:
first prove that Core controls one real form. Retaining the original graph remains
necessary, but it cannot establish grammar authority by itself.

Validation performed: canonical declaration/template inventory plus traced source
reads. No claim that these proposed semantics already compile or run. The theoretical
equals change has not been applied to canonical Range.

Subsequent agreed cleanup: removed Semantic, Selection, and SelectionCase, along
with their predecessor staging stubs and presence-only test assertions. They had no
construction sites in the inspected Range sources. Ordinary if/switch parser and
execution behavior remains; Execution is retained for its existing effect uses.

Cleanup validation: recovery parsing passes with 104826 nodes and 155 constructs;
all 13 negative parser probes pass. Ordinary and sanitized evaluator runs each pass
five positives, 20 rejections, and canonical ARM64 checks. Predecessor bundle staging
retains its existing generationOneCompile boundary (not a successful native build).

The frozen-seed compiler gate fails during canonical syntax materialization after
this cleanup. An isolated comparison restoring only the SelectionCase definition
makes that phase pass: the seed labels FileManager.range's parameter at byte 6368
(`_ revision: FileRevision`) and the next parameter (`all suffix: String`) as
SelectionCase. The earlier description of this as the whole function was incorrect:
source offsets are bytes, not Unicode character indexes. Thus the broad colon template was masking a
recognition defect, despite there being no explicit SelectionCase construction
sites. The cleanup intentionally does not restore this false match or patch the
seed. Defining parameter syntax and its capture constraints is the next separate
review item; Core currently references Parameter without defining it.

Signature follow-up: Transformation is removed. Function directly captures inputs
and result; Macro directly captures inputs, target, and optional result. Their
existing surface punctuation is retained. Second-colon defaults are rejected;
construction defaults remain. Recovery parsing and ordinary/sanitized evaluator
checks pass, including direct execution of the Range macro signature parser.
Input-member materialization and binding-input execution remain pending.
