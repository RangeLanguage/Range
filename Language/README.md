# Range language

`Language` contains two active parts:

```text
Language/
  Core/         language semantics written in Range
  Compiler/     compiler implementation written in C
```

Every `.range` file recursively under `Core/` belongs to Core; there is no
separate source manifest.

The active Core currently contains `DataTypes/Int.range` and
`Macros/Integer.range`. The remaining definitions and meaning documents are
preserved under `Development/DeferredCore` for later design work.

Core owns the public constructs, macros, and graph relationships. The compiler
provides the trusted parser, graph storage, macro execution kernel, and direct
target encoding. Its current milestone is integer representation; native output
has not yet been re-established after the compiler reset.
