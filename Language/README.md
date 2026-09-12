# Range Compiler

`Language` owns the language's Core, compiler, bootstrap, and temporary
runtime. Its `Compiler` directory is the repository's only compiler
implementation.

The root `Project.range` therefore declares `RangeLanguage`. `RangeCompiler`
names remain component identities for the compiler executable and its runtime,
not the identity of the enclosing project.

## Layout

```text
Language/
  Core/                Range's bootstrap core library and macro surface
  Compiler/            compiler entry and implementation sources
  Runtime/             temporary native runtime boundary
  Bootstrap/           accepted hash-pinned compiler seed
    GenerationZero/    temporary native CLI and graph bridge
  CompilerSources.txt  canonical self-host source manifest
  Project.range        compiler project declaration
```

There is intentionally no intermediate `Sources` directory. Core is a sibling
of Compiler because projects consume Core; it is not owned by the compiler
implementation.

The checked-in bootstrap at `Bootstrap/range` is a hash-pinned macOS arm64
seed. It still emits deterministic Apple arm64 assembly and depends dynamically
only on libSystem; it is not the public successor backend. It is replaced only
after a candidate compiler reproduces byte-identical ProgramGraph, object code,
executable bytes, and focused fixture output.

The compiler pipeline is:

```text
Range graph
  -> ProgramGraph
  -> Apple arm64 machine code
  -> relocatable object model
  -> Mach-O executable and ad-hoc signature
```

The public compiler does not invoke `clang`, `as`, or `ld`. Clang is restricted
to producing the hash-pinned temporary runtime objects during bootstrap and
distribution creation. The Range linker now owns multi-object relocation,
import stubs, the GOT, chained binds/rebases, exports, and ad-hoc signing. The C
host and RawBuffer runtime remain temporary until dynamic graph-native many
storage, file/process effects, and cleanup are native.

## Commands

```sh
range version
range register <project-name> [project-root]
range projects
range run <project-path-or-name> [-- args...]
```

Until the native CLI artifact passes its reproducibility gate, the equivalent
bootstrap launcher is `Language/Bootstrap/Tools/range`. Compiler proofs live
under `Testing/Tools` and are not public CLI commands.

The current self-host boundary is the continuation after the compiler entry's
first compound conditional. Removing the former compiler does not itself prove
self-hosting.
