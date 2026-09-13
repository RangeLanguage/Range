# Deferred compiler material

This directory preserves the former project declarations, compiler test suites,
benchmark implementation, and inherited C runtime. They are reference material
for future work and are excluded from the active compiler build and editor index.
Their original relative layout is retained; scripts here are historical and may
refer to retired compiler paths.

The active compiler checks live in `Testing/Compiler` and `Testing/Tools`.
Editor navigation fixtures remain in `Testing/Editor`. Active Core consists of
`Int` and `@integer`; the other Core declarations are in `Development/DeferredCore`.

Saved benchmark results and their schema remain in `Benchmarks/Speed` because
the website imports them. They are historical measurements, not evidence for
the current C compiler.

Former generated GPUCanvas output is preserved in the ignored `Artifacts/`
directory, and benchmark build output remains ignored under `Benchmarks/Speed/.build`.
