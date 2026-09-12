# Range compiler

The Range compiler is implemented in C. Its intended pipeline reads the language
definitions in `Language/Core`, builds their program graph, executes Core macros,
and emits target artifacts directly as bytes without LLVM.

The current implementation begins with a C lexer, parser, and bounded evaluator.
It is the only compiler implementation. There are no compiler generations or
frozen compiler seeds.

The build includes every `.c` file in `Source/`. `main.c` handles the command
line; lexer, parser, AST storage, evaluator, and a small evaluator index buffer
make up the implementation. The former host runtime and metrics are preserved
under `Development/DeferredCompiler/Language/Compiler/Source`.

The first implementation milestone is integer representation. The compiler must
evaluate `@integer` and derive the selected payload member, effective bit width,
and signed interpretation from the resulting graph. The target encoder must
consume those graph facts rather than recognize `Int`, `value`, or `bits` by name.

Build and run the focused checks with:

```sh
Language/Compiler/Tools/build-range-compiler /tmp/range-compiler
Testing/Tools/check-compiler-parser
Testing/Tools/check-compiler-evaluator
```
