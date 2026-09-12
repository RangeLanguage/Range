# Single-name parameters

Agreed rule: each input requires explicit `let` or `binding`, and one name serves
both as the local identity and the argument label. Underscore/no-label parameters and separate external/local names are removed
from the current language implementation. Parameter macros may extend this later;
no extension mechanism is being designed in this change.

```range
function get(let revision: FileRevision, let suffix: String): FileQuery {
    // The body uses revision and suffix.
}

// A corresponding call uses get(revision: currentRevision, suffix: ".range").
```

The example body above is illustrative, not an executable implementation.

Changes include the Core FileManager method, recovery fixtures, RangeView function
and constructor-member declarations and their affected calls, and older test
fixtures containing these parameter forms. Frozen predecessor sources are preserved.
Primitive positional construction (such as Int(42)) is a different surface and is
not removed by this parameter change.

The Range parser records the explicit keyword, identity, type, owner, and order; the
rendered label is derived from identity. Both function and macro parsers reject name
pairs and the underscore name. The recovery parser enforces the same declaration
rule, and recovery function calls require the parameter's name as their label.

Validation includes executing the actual Range-written function parameter parser
through the recovery interpreter, checking retained names for the accepted form
and parser rejection status for the removed forms. Separate recovery fixtures
reject function/macro aliases, underscore parameters, and an unlabeled function call.

Subsequent decision: input members reuse Let or Binding. Omission of `let` is deferred
until templates/macros can support that shorthand. No separate Parameter construct
is planned. Function now contains inputs
and result directly; Macro contains inputs, target, and optional result. The
Transformation wrapper has been removed. This representation does not yet implement
input-member materialization or binding-input execution. Both parsers retain the
explicit input keyword; the recovery evaluator fails loudly on a reachable binding
input rather than silently passing a copy.

Defaults now use ordinary construction (for example `symbol: String("")`). A second
colon for a default is rejected. The Range parser retains the constructor's argument
span as its existing default record; this is not yet full application-graph default
evaluation. Tests execute the actual Range macro signature parser with and without
a result and verify the retained target and default independently.

The previously exposed frozen-seed template failure remains an open item. The next
step is materialization of explicitly declared input members using existing Core
templates, without introducing an omission rule or a broad catch-all colon template.

Explicit-input migration validation: all active Range declarations were audited,
excluding preserved predecessor sources and intentional rejection fixtures. The
recovery parser and ordinary/sanitized evaluator gates pass, including canonical
parameter and macro-signature parser execution. Thirty-four rejection cases cover
the current execution boundary and removed syntax. The editor grammar tests pass;
predecessor bundle staging retains its existing generationOneCompile boundary.
The frozen-seed compiler gate still fails in canonical syntax materialization;
these checks do not constitute a full compiler build.
