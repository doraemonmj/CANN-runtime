# Claude Code Project Context

**This is an Ascend NPU programming guide** (reference: CUDA Programming Guide).

We teach hardware concepts through examples. We do NOT implement scheduling or frameworks.

## Problem Domain

### Hardware Model

Simplified Ascend architecture: `1A + 4B + 24C + 48D`

| Unit | Description | Latency from A |
|------|-------------|----------------|
| A | CPU (host) | - |
| B | CPU (control) | 3μs |
| C | Weak CPU + DSA (SIMD) | 3μs |
| D | Weak CPU + DSA (SIMD) | 3μs |

Note: B → C/D latency is ~0μs (tightly coupled).

### Software Model

Tasks are dispatched from A, executed on C/D. Many kernels run on C/D.

**Core questions this project teaches:**

1. **Single kernel**: How to define calling interface? How to prepare arguments?
2. **Multiple kernels**: How to express the graph (parallelism, dependencies)? How to allocate memory? How to schedule?

### User Model

The user (human or PyPTO framework) knows:
- Which kernels to run
- How to arrange them (calendar scheduling)
- Dependencies between kernels

**We provide**: Interfaces to describe and execute these kernels.

**We do NOT provide**: The scheduling logic itself.

## Project Scope

| In Scope | Out of Scope |
|----------|--------------|
| Hardware concepts | Scheduling algorithms |
| Kernel interfaces | Framework integration |
| Memory management | Production optimizations |
| Synchronization primitives | Application logic |
| Execution model | Distributed computing |

## Guidelines

1. **Hardware-first**: Explain what hardware does, not software abstractions
2. **Minimal examples**: Each example teaches exactly one concept
3. **Progressive**: Examples build on previous ones
4. **No magic**: Every line explainable in hardware terms

## Directory Structure

```
basics/
  01-xxx/    # First concept
  02-xxx/    # Builds on 01
  ...
  06-calling-interface/  # Kernel calling, args packing, machine abstraction
```
