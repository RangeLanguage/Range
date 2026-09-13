# Range compiler

The Range compiler is implemented in C. Its intended pipeline reads the language
definitions in `Language/Core`, builds their program graph, executes Core macros,
and emits target artifacts directly as bytes without LLVM.

The current implementation begins with a C lexer, parser, and bounded evaluator.
It is the only compiler implementation. There are no compiler generations or
frozen compiler seeds.

The build includes every `.c` file in `Source/`: `lexer.c` scans tokens,
`parser.c` builds the graph, `model.c` owns graph allocation and structural
helpers, `graph.c` formats readable graph output, and `compiler.c` loads sources
and runs the bounded evaluator.
Shared graph structs live in `model.h`; lexer and parser interfaces retain
their own headers. The former host runtime and metrics are preserved
under `Development/DeferredCompiler/Language/Compiler/Source`.

The first implementation milestone is integer representation. The compiler must
evaluate `@integer` and derive the selected payload member, effective bit width,
and signed interpretation from the resulting graph. The target encoder must
consume those graph facts rather than recognize `Int`, `value`, or `bits` by name.

Build and run the focused checks with:

```sh
Language/Compiler/Tools/build-range-compiler /tmp/range-compiler
/tmp/range-compiler Language/Core
/tmp/range-compiler --tree Language/Core
/tmp/range-compiler --emit-graph Language/.range/Build Language/Core
Testing/Tools/check-compiler-parser
Testing/Tools/check-compiler-graph
Testing/Tools/check-compiler-evaluator
```

`--emit-graph` creates the output directory and writes one readable graph per
source (`Int.txt`, `Integer.txt`). `graph.c` and `graph.h` own this Range-like
display format. Source templates in `Language/Compiler/Types/*.range` are loaded
on each graph emission and emitted as `Macro.txt`, `Construct.txt`, and `Member.txt`.
Their names are reserved for the definitions. The `@type` templates
use unquoted names, such as `name: Macro`, and group their field declarations
inside `fields: { ... }`. Graph names and references likewise
use bare identifiers; string literals inside embedded Range code keep their
original quotes. The templates
declare bare fields without value-type constraints. `@many` means one or more
values; `?` allows zero, so `@many members?` means zero or more members.
These templates authorize field lookup and enforce graph shape: unknown fields,
missing required fields, and excess values on single fields are rejected before
emission. C adapters supply stored values; they do not grant access to undeclared
fields. Semantic Core validation remains separate. The templates currently cover
Macro, Construct, and Member nodes. Other expression nodes retain their inspection
format. Use `--graph-types <directory>` before `--emit-graph` to load another set;
the default source directory is recorded by the build tool, independent of cwd.
The display shows named nodes, macros, generics, members, and expression values,
without source spans or parser bookkeeping. This is an inspection view, not
an invocation of Core macros: `@integer(64)` and `@bool(true)` denote primitive
graph literals. The view is not
executable Range code or a graph interchange format. `--tree` retains the
detailed parser dump. Macro views currently show only top-level local values
whose expressions explicitly reference a `#` context field, plus the retained Range
code inside explicit `#environment { ... }` blocks. Each block gets an
`environment` section; absent blocks produce no section. These sections show
that deferred blocks exist, not that their conditions ran or their code was
expanded. Other macro statements and top-level helper functions are omitted
from this focused view. Source text is retained on units for this display.
Duplicate source basenames are rejected to
avoid overwriting another source's graph. Output is emitted only after all
sources parse and their macro declaration queries resolve successfully.

Graph emission resolves parameterless macros applied to constructs. `#name` and
`#members` refer to the macro; `#target` refers to the annotated construct.
Queries support `.name`, `.members`, `.generics`, `.filter(named:)`, `.first`,
and member `.value`, provided the receiver's template declares the field.
`#environment` now returns the list of explicit deferred environment blocks.
The old `#environment.target.Declaration` alias is no longer supported:
`#target` directly identifies the target construct. `#generics` is invalid on
a Macro, while `#target.generics` is allowed by Construct's template.
An empty selection or absent optional field resolves to `none`. Unknown fields,
unresolved or ambiguous macros, and cyclic member evaluation fail with source
locations before output is written.

Each annotation owns its application context and resolved member bindings.
`Int.txt` shows these under `macros.resolved`; `Integer.txt` retains its defining
expressions. `#members` returns the original member nodes, including unevaluated
members, and printing those nodes reproduces their declarations. This is graph
inspection, not implementation of `for` syntax or runtime printing. Resolution
does not overwrite expressions or share evaluated results between targets.
Validation statements and explicit environment expansion blocks stay deferred;
instance specialization, macro parameters, and general macro execution remain
unimplemented.

Inputs may be files or directories. A directory includes every `.range` file
recursively, including hidden subdirectories. Sources are sorted by canonical
path and overlapping inputs are parsed once. Recursive traversal skips directory
symlinks; explicitly supplied directory symlinks and source-file symlinks work.
No sources, inaccessible inputs, and malformed sources fail with diagnostics.

All sources are parsed into the existing arena-owned syntax graph before
`--run` binds and evaluates its entry. Names remain available for later resolution;
the parser does not require declarations to precede their uses. This is the
structural graph foundation with declaration-query resolution, not yet generic
specialization or full macro execution.
