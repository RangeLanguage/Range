# Compiler flow draft

Follow-up: [Core audit](CoreAudit.md) traces the definition/engine boundary after
the subsequent design discussion. Its assignment-template experiment supersedes
the implementation ordering below; this entry remains an orchestration sketch.

`Entry.range` expresses the proposed flow discussed with George. It is deliberately
outside the canonical source manifest. It is **not executable**: its proposed APIs
have not been implemented or type-checked. This is a concrete design to review,
not another compiler generation. Existing compiler and recovery code is unchanged.

The input is a directory, a Core location, and an output location. A compilation
owns one queryable source graph. Ordinary Range functions collect, resolve,
evaluate, validate, and emit. Project and main are graph discoveries, not mandatory
fields of the compilation container. Local query results are useful; their lifetime
does not make them a second source of semantic truth.

## What the current path actually does

| Current source | Observed behavior | Consequence for this draft |
| --- | --- | --- |
| `Language/CLI/Main.range`, `entry` | Compile/run commands return the compiler-body gate diagnostic. | Public CLI execution is still blocked; this draft does not unblock it. |
| `Language/Compiler/Main.range`, `compileProject` | Accepts an already constructed `Project`, discovers paths, creates separate stores, populates/validates them, selects a target. | Replace orchestration gradually; do not mistake the input `Project` record for a discovered macro result. |
| `Language/Compiler/SourceSet.range`, `compilerBDiscoverProjectSourcePaths` | Discovers route files first; appends Core paths only if `Project.range` exists. | Proposed Core loading is explicit and independent of that filename check. Canonicalize/deduplicate source identities and exclude nested build scopes according to build policy. |
| `Language/Compiler/Revision.range`, `compilerBPopulateProjectRevision` | Collects declarations across files, resolves macro applications, derives syntax, derives emitted macro blocks. | Useful stages exist, but per-source collection and resolution separately parse source. Retain the parsed revision and attach cross-file relationships to it. |
| `Language/Compiler/SyntaxMaterialization.range`, `compilerBMaterializeSourceSyntax` | Reads and parses a source revision, materializes syntax, then destroys the revision. | Preserve source structure and identity so later consumers do not reconstruct it. |
| `Language/Core/Macros/Syntax.range` | `syntax(template: String)` is a grammar-template macro. | A whole-compilation graph query is a proposed capability, not the current meaning of `@syntax`. Decide its spelling separately. |
| `Language/Core/Macros/Main.range` and `Language/Core/Package/Project.range` | Main is an empty Construct-targeted marker. Project contains a target-presence query/diagnostic. | Do not describe these as already returning executable plans or as having proven general macro execution. |
| `Language/Compiler/Backend/Target.range` | Identifies target/project applications by resolved declaration names and reads target selection data. | Reuse target data types; move policy toward actual evaluated relationships/results. |
| `Language/Core/MacroExecution.range` | Contains specialized freestanding macro execution and restricted condition handling. | Some execution machinery exists, but this is not the general graph-aware evaluator contract below. |
| `Language/Compiler/Graph/Lowering.range` | `graphEnsureFunction` and `graphLowerFunction` read/parse source again; call lowering recognizes macro names `print`/`write`. | Lower retained identities and evaluated results instead of reparsing or implementing macro behavior by name. |
| `Language/Compiler/Graph/Program.range` | `ProgramGraph` stores functions, blocks, operations, values, effects, and control edges. | Keep this useful derived representation. It is not the complete queryable syntax graph. |
| `Language/Compiler/Main.range`, `emitExecutable` | Encodes a program object and links runtime objects through Range code, then writes output. | Reuse backend work. Current runtime objects are an explicit remaining dependency; the draft does not remove them. |

## Every proposed operation

Existing types used by the entry draft: `URL`, `Artifact`, and `ProgramLowering`.
New types: `CompilationGraph` and `ExecutablePlan`.

| Operation | Contract | Existing work / missing work |
| --- | --- | --- |
| `openCompilation` | Create one owner for source revisions, graph relationships, evaluation state, diagnostics, and derived allocations. | Reshape `CompilerBProjectRevision` and retained `SyntaxRevision` storage. Owner and cross-file identity API missing. |
| `collectCore` | Parse Core using a defined bootstrap grammar; register its syntax definitions in this graph. | Parser and template materialization exist. Explicit bootstrap grammar boundary and retained integration missing. |
| `collectDirectory` | Discover files in the requested scope, retain parsed bodies once per source revision, and collect declaration/application edges. | Source discovery and collection exist. Unified retention and scope policy need reshaping. |
| `prepareExecutable` | Resolve/evaluate the dependencies required to answer build queries, including generated syntax. Report unresolved dependencies, cycles, or unsupported operations. | General dependency-aware macro evaluation is missing. A scan ending is not proof of completion. |
| `requireExecutablePlan` | Query completed graph information; select build scope and target, require an entry, diagnose absence/ambiguity. Return references and evaluated configuration. | Target-selection logic exists. General graph query, completion evidence, and entry-result contract missing. No requirement that every directory contain exactly one project. |
| `lowerExecutable` | Produce validated executable operations from retained graph identities and evaluated macro results. Record diagnostics in compilation state. | Adapt `lowerProgram`; remove rereading/reparsing and macro-name shortcuts on this path. |
| `writeExecutable` | Encode, link, write, and return an `Artifact`; retain all temporary allocations under compilation ownership until cleanup. | Adapt existing binary backend and `emitExecutable`. Success still needs a real executable test. |
| `reportCompilationDiagnostics` | Render accumulated source-located diagnostics, including lowering/linking failures. | Existing diagnostic stores/renderers can be reused behind one collection interface. |
| `closeCompilation` | Release all compilation-owned resources on success and failure. Do not invalidate the returned artifact URL. | Existing destroy functions are pieces; complete ownership accounting is missing. |

`state graph` denotes compilation-owned mutable data; it is not a scheduling
primitive. Passing it to helpers must preserve graph identity and permitted
mutation. Exact binding annotations need to follow Range's chosen ownership rules;
the recovery interpreter's reference behavior is not proof of those rules.

## Proposed macro execution contract

These are design choices for implementation, not claims about today's evaluator.

1. An application resolves to a macro declaration identity. Its invocation receives
   bound arguments, its target identity, and an environment referencing this same
   compilation graph. Dispatch does not depend on the macro's textual name.
2. The body uses ordinary Range execution. Compile-time invocation can produce
   values, diagnostics, and graph additions/relationships. Runtime behavior remains
   represented in the graph for lowering; collection does not execute it.
3. A query can inspect currently known structure. A query that needs a final set
   also records a dependency on completion of that set in its scope. These are
   different requests even if they share a convenient query API.
4. Evaluation is scheduled by dependencies of the build request. A body waiting
   for a final set suspends until its producers finish. Newly emitted nodes receive
   stable identities and are collected/resolved before dependent queries complete.
   Results are associated with application identity and the graph revision they
   depend on; effects must not be duplicated by blind retries.
5. Completion means relevant producers are finished. No-progress work with unresolved
   dependencies is a diagnostic, not success. Cycles and unbounded generation need
   explicit diagnostics/limits. A macro cannot demand a completed set whose contents
   depend on the still-undecided outcome of that same query without a defined rule
   for breaking that cycle. The initial implementation should diagnose it.
6. A macro can validate duplicates when invoked, scoped to the relevant program.
   Independent build policy checks absence. Both are ordinary Range code. A macro
   requiring a complete set may defer its validation until that set closes.

This permits querying one graph across different execution times without pretending
that all facts are final at collection time. It does not require one physical table:
indexes, typed views, and derived control-flow graphs can refer to stable identities.

## First implementation slice

First retain parsed source revisions and make one lowering path consume them without
reparsing. This establishes the graph ownership needed by macro queries before we
expand execution. Preserve existing diagnostics and source identities; verify that
multiple references to the same declaration resolve to the same retained node.

Then implement one real macro invocation against that retained graph: resolve its
declaration, bind arguments/environment, execute its body, retain its result, and
consume that result. Include a body change that changes the output without changing
the macro name, and a renamed declaration with equivalent behavior. Those checks
distinguish body execution from compiler name shortcuts.

The complete vertical proof is a tiny directory producing a runnable executable
through Range compiler logic under the recovery interpreter. Check a successful
entry, missing entry, duplicate entries in the same scope, a generated entry whose
producer must finish first, and a dependency cycle. Compilation must not run the
program's runtime effects; launching the output must run them. Self-compilation
comes after that proof, not as a substitute for it.

No canonical source, recovery manifest, accepted seed, or public command is changed
by this draft. Validation: the manifest-verified recovery interpreter build parsed
`Entry.range` successfully (151 nodes, two functions, zero failures). This is only
a syntax check; it does not establish that the proposed APIs exist or execute.
