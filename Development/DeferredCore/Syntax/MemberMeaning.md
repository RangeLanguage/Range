# Member right-hand meaning

Agreed design, 2026-09-06. This records the bounded member discussion, not the
deferred macro manifesto or a claim that the compiler implements these rules.

`Let.value: @syntax` retains a relationship to the original right-hand syntax.
Resolution determines what that syntax supplies. Keep syntax identities and their
relationships; do not replace them with detached evaluated values or reparse text
downstream to recover meaning.

| Example | Agreed meaning |
| --- | --- |
| `let user: User` | A reference to a construct declares a compatible-value requirement; it supplies no instance. |
| `let user: User()` | A construct application supplies an instance; in an input position it supplies a default. |
| `let user: fetchUser()` | A function application supplies its result. |
| `let user: original` | When `original` resolves to an existing value member, its value is copied under the property-level rules. |
| `let count: 255` | The collected literal rule supplies numeric meaning; the literal supplies the default. The specific number does not define the accepted family or choose machine width. |
| `let count: previous + 1` | The operator expression supplies its result; retain its syntax and resolved operation relationships. |

A bare identifier's spelling is insufficient to choose between a requirement and
a supplied value: its resolved declaration matters. Parentheses must remain
observable even for a zero-argument application.

Value-copy semantics do not mandate an immediate physical copy. Copy-on-write is
an implementation option. A later permitted mutation must preserve the copied
value according to each property's rules; binding properties retain their
connections rather than becoming independent state. This is not a claim that the
recovery evaluator already implements copy-on-write or full ownership semantics.

This pass introduces no Value/Expression wrapper, no new macro, and no new
restriction on which syntax forms exist. Evaluation timing, literal rule execution,
and application default resolution still need implementation work.

## Source trace and gaps

- `Language/Compiler/Parser.range`, member-facet parsing: an identity after the
  colon is recorded as a type token. Only string/integer tokens take the direct
  value path. Parenthesized initializers can become expression facets, but an
  empty `User()` now receives an initializer expression despite its empty argument
  span. Ordinary members additionally retain `rhsSyntaxIDs`, pointing to the
  complete consumed RHS token span, including bare `User` and `original`.
  Bare references now also enter the ordinary expression graph. They are
  deliberately not classified as initializers until resolution.
  Special derived-body and collection-production paths still use their existing
  dedicated records and leave this new relationship unset.
- The same direct-value test excludes bare boolean names. Thus the new Core
  `signed: true`/`false` declarations parsing successfully in the recovery frontend
  does not prove canonical literal materialization works.
- `Language/Compiler/Source/parser.c`, `atTypePosition`: every name token
  enters type parsing. Local/member initialization therefore treats `original`
  and `fetchUser()` as type-shaped forms before resolving them. Locals and members
  now also retain exact consumed RHS byte spans and an explicit application flag,
  preserving empty `User()` separately from `User`. Plain let/state bare RHS
  names additionally have a name-reference node. Decorated type forms and function
  applications in this position still have legacy representation paths.
- `Language/Compiler/Source/evaluator.c`: bare RHS names now use ordinary
  lexical/member/declaration lookup during binding and retain the resolved
  declaration identity. A value reference supplies its value; a construct
  reference remains a requirement. A local requirement may be initialized later;
  reads require definite initialization, and let initialization must be unique.
  An unused local may remain uninitialized. See ControlFlowMeaning.md.
  Constructor member defaults can reference preceding members. General forward
  or cyclic member dependencies are not implemented in this bounded evaluator.
  Scalar copies are independent. Objects still retain arena references, so this
  does not establish aggregate copy-on-write or full ownership behavior.

Preservation follow-up: compiler member tests cover bare construct/member
references, empty construct/function calls, nested arguments, and an integer
literal. Structural tests assert exact source spans and
empty-call distinction for both members and locals. These are syntax proofs only.
General trailing operator expressions in the legacy typed-member parsing path,
input-member materialization, literal resolution, and value-copy behavior remain
separate work. IdentityReferences.range additionally executes local/member value
references, name shadowing, explicit construction, required constructor inputs,
and scalar snapshot behavior. Negative fixtures reject unknown names and reads
before initialization. Canonical Range parsing now retains bare member
names as ordinary expression nodes; full canonical semantic resolution remains
to be connected and is not established by the recovery evaluator tests.
