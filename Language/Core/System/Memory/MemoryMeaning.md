# Primitive storage contract

Core design draft, 2026-09-12. These declarations define the proposed interface;
the interpreter and native compiler do not implement it yet. The existing stack
allocation operations alone do not establish growable, escaping allocation.

There is no Buffer container. Address is a primitive target address. Collection
policy stays in the collection macro and the ordinary code it supplies: choosing
capacity, preserving order, bounds checks, growth, and element-count updates.

## Addresses and allocation

- Address has target-pointer size and alignment. Copying an Address does not copy
  its allocation or transfer ownership. It is distinct from an integer value.
- emptyAddress returns the distinguished no-allocation address. addressIsEmpty
  tests that value. It is the explicit initial value of an empty Array's storage;
  this does not depend on an optional type acquiring an implicit default.
- addressOffset adds a nonnegative byte offset within the same allocation,
  including its one-past-end address. Offset zero preserves an empty address;
  other offsets from an empty address are invalid. One-past-end is not readable.
  Address arithmetic must not silently wrap.
- allocateMemory requests uninitialized target memory. bytes must be nonnegative;
  alignment must be a supported positive power of two. Zero bytes returns an empty
  address. Allocation failure also returns an empty address, so a positive request
  must be checked before use. Invalid sizes/alignment are diagnosed when known
  during compilation and otherwise trap. Allocation does not initialize elements.
- releaseMemory releases an allocation's original base address after all its live
  elements have been destroyed or moved. Releasing an empty address is a no-op.
  It does not implicitly destroy values. Interior-address release, double release,
  and access after release are invalid.

Memory operations in an emitted constructor or member function run when that code
runs. Executing a macro must not allocate compiler-process memory and embed its
address into the target program. Addresses must never be serialized that way.

## Element layout and lifetime

valueStride and valueAlignment query the resolved Element specialization's target
layout. Stride is the spacing between consecutive elements, including padding;
alignment is the allocation/alignment requirement. These queries are not defined
by the host C representation or by an element type's spelling. Zero-sized elements
may have zero stride; their logical count is independent of allocated byte count.

For ordinary nonzero-sized elements, element i is at storage + i * stride. The
collection checks i against count and checks capacity * stride for overflow before
requesting memory. A one-past-end address cannot be used for a value operation.

- initializeValue creates one Element in aligned, uninitialized destination
  storage using the language's ordinary value-copy rules. It must preserve the
  source value. It is not permission to memcpy values with managed ownership.
- readValue reads an initialized Element using ordinary value-copy rules; the
  stored value remains initialized.
- moveValue transfers one initialized Element into uninitialized destination
  storage. The source becomes uninitialized. Identical source/destination is a
  no-op; otherwise the two element regions must not overlap. Moving must preserve
  the language's ownership and identity relationships, not duplicate them.
- destroyValue ends the lifetime of an initialized Element and releases its owned
  contents according to its type. The slot becomes uninitialized; its allocation
  is not released by this operation.

Repeated initialization, reading uninitialized storage, and double destruction
are invalid. Source-known violations require diagnostics. The runtime enforcement
strategy for invalid raw-memory use remains to be defined; this draft does not
claim a complete memory-safety or ownership proof.

## First Array slice

Array declares count and its local derived isEmpty. collection supplies storage
and capacity through an extension; it must not create another count slot.

```range
// Members supplied by collection:
state storage: emptyAddress()
state capacity: 0
```

Together with Array's count default, empty construction means count = 0, capacity
= 0, storage = no allocation. The supplied defaults are runtime construction
expressions; the macro only supplies their declarations. User-provided nonzero
count without initialized elements must not be accepted by a completed contract.

The collection macro's optional capacity configuration is not implemented by this
empty slice. Reserving capacity, appending, bounds-checked access, destruction,
modifier receiver binding and specialization are later execution work. The
existing emitted-modifier draft also still needs to avoid duplicating Array's
local isEmpty. Parsing these files proves none of those behaviors.

## Required execution evidence

Before claiming implementation: verify target-size Address representation; empty
initialization with no allocation and one count slot; allocation failure handling;
alignment and checked size arithmetic; typed initialization/read/move/destruction;
and generic element layout. Changing collection-authored capacity/growth policy
must change generated behavior without changing a compiler Array-name branch.
Then test count updates through its declared member and resulting isEmpty
dependencies, including two independent Array instances. No native or recovery
execution evidence is established by this document.
