# Project Guidelines

## Purpose

This is a **readthedocs-style programming guide for Ascend NPU**, similar to the CUDA Programming Guide.

We teach developers **how to use the hardware** through concepts and examples. We do NOT implement upper-level scheduling or frameworks.

## Problem Domain

### Hardware Architecture (Simplified)

```
1A + 4B + 24C + 48D

A: Host CPU
B: Control CPU (close to accelerator)
C: Weak CPU + DSA (SIMD extensions)
D: Weak CPU + DSA (SIMD extensions)

Latencies:
  A → B: 3μs
  A → C/D: 3μs
  B → C/D: ~0μs (tightly coupled)
```

Full model: `1A + 16B`, where `1B = 4C + 24D + 48E`. We simplify to single-node for this guide.

### Software Challenge

Tasks dispatch from A, execute on C/D. The core programming questions are:

**Single Kernel:**
- How to define the calling interface?
- How to prepare and pass arguments?

**Multiple Kernels:**
- How to express the task graph (parallelism, dependencies)?
- How to allocate memory across kernels?
- How to schedule execution?

### User Model

The user (human or PyPTO framework Pass) already knows:
- Which kernels to run
- The execution order and dependencies
- How to arrange tasks (calendar scheduling)

**Our job**: Provide interfaces to describe and execute these kernels.
**NOT our job**: Decide the scheduling strategy.

## Scope

### In Scope
- Hardware concepts and abstractions
- Kernel execution model
- Memory hierarchy and management
- Synchronization primitives (events, barriers)
- Device-host communication
- Low-level programming interfaces
- Calling conventions and argument packing

### Out of Scope
- Scheduling algorithms and policies
- Framework integration (PyTorch, TensorFlow)
- High-level optimization strategies
- Application-specific logic
- Distributed computing (multi-node)

## Reference Model

The **CUDA Programming Guide** is our reference for:
- Progressive concept introduction
- Minimal, complete examples
- Hardware-focused explanations
- Building complexity incrementally

## Example Structure

Each example directory should:

1. **State the concept** - What hardware feature is demonstrated
2. **Minimal code** - Smallest code that shows the concept
3. **Explain behavior** - What happens at the hardware level
4. **Build progressively** - Each example builds on previous ones

```
basics/
  01-xxx/              # First concept
    CMakeLists.txt
    main.cpp
    README.md          # Optional - code should be self-documenting
  02-xxx/              # Second concept (builds on 01)
  ...
  06-calling-interface/  # Current: kernel calling, args, machine abstraction
```

## Writing Principles

1. **Hardware-first**: Explain hardware behavior, not software abstractions
2. **Concrete over abstract**: Demonstrate actual behavior through examples
3. **Minimal examples**: Each example teaches exactly one concept
4. **No magic**: Every line of code should be explainable in terms of hardware
5. **Progressive complexity**: Start simple, add complexity incrementally

## Code Style

- Self-documenting code over verbose comments
- Comments explain **why** (hardware behavior), not **what** (code logic)
- Minimal dependencies
- C++11 standard (matches hardware constraints)
- Clear separation: interface (headers) vs implementation (source)
