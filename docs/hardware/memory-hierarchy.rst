Memory Hierarchy
================

Ascend NPUs have a deep memory hierarchy, similar to NVIDIA GPUs but with
different names and sizes.

Overview
--------

.. code-block:: text

   ┌─────────────────────────────────────────────────────────────────┐
   │                        Memory Hierarchy                         │
   │                                                                 │
   │  ┌──────────────────────────────────────────────────────────┐  │
   │  │                    Host Memory (DDR)                      │  │
   │  │                   System RAM, ~100GB+                     │  │
   │  └──────────────────────────────────────────────────────────┘  │
   │                              │                                  │
   │                              │ PCIe (~32 GB/s)                  │
   │                              ▼                                  │
   │  ┌──────────────────────────────────────────────────────────┐  │
   │  │              HBM / Global Memory (GM)                     │  │
   │  │               32-64 GB, ~2 TB/s bandwidth                 │  │
   │  │          Visible to all cores, persistent                 │  │
   │  └──────────────────────────────────────────────────────────┘  │
   │                              │                                  │
   │                              │ (~1 TB/s)                        │
   │                              ▼                                  │
   │  ┌──────────────────────────────────────────────────────────┐  │
   │  │                    L2 Cache                               │  │
   │  │                192 MB, shared across cores                │  │
   │  │              Hardware-managed cache                       │  │
   │  └──────────────────────────────────────────────────────────┘  │
   │                              │                                  │
   │             ┌────────────────┼────────────────┐                │
   │             ▼                ▼                ▼                │
   │  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐   │
   │  │   AICORE 0     │  │   AICORE 1     │  │   AICORE N     │   │
   │  │  ┌──────────┐  │  │  ┌──────────┐  │  │  ┌──────────┐  │   │
   │  │  │ L1 (1MB) │  │  │  │ L1 (1MB) │  │  │  │ L1 (1MB) │  │   │
   │  │  └──────────┘  │  │  └──────────┘  │  │  └──────────┘  │   │
   │  │  ┌──────────┐  │  │  ┌──────────┐  │  │  ┌──────────┐  │   │
   │  │  │UB (256KB)│  │  │  │UB (256KB)│  │  │  │UB (256KB)│  │   │
   │  │  └──────────┘  │  │  └──────────┘  │  │  └──────────┘  │   │
   │  │  ┌────┬────┐   │  │  ┌────┬────┐   │  │  ┌────┬────┐   │   │
   │  │  │L0A │L0B │   │  │  │L0A │L0B │   │  │  │L0A │L0B │   │   │
   │  │  │64K │64K │   │  │  │64K │64K │   │  │  │64K │64K │   │   │
   │  │  └────┴────┘   │  │  └────┴────┘   │  │  └────┴────┘   │   │
   │  │  ┌──────────┐  │  │  ┌──────────┐  │  │  ┌──────────┐  │   │
   │  │  │L0C (256K)│  │  │  │L0C (256K)│  │  │  │L0C (256K)│  │   │
   │  │  └──────────┘  │  │  └──────────┘  │  │  └──────────┘  │   │
   │  └────────────────┘  └────────────────┘  └────────────────┘   │
   └─────────────────────────────────────────────────────────────────┘

Memory Comparison
-----------------

.. list-table::
   :header-rows: 1
   :widths: 20 15 20 45

   * - Memory
     - Size
     - Scope
     - Usage
   * - Host DDR
     - ~100+ GB
     - Host
     - Your program's data before/after NPU processing
   * - HBM (GM)
     - 32-64 GB
     - All cores
     - Input/output tensors, weights, intermediate results
   * - L2 Cache
     - 192 MB
     - All cores
     - Hardware-managed, automatic caching of GM
   * - L1 Buffer
     - 1 MB
     - Per AICORE
     - Software-managed, staging area for Cube ops
   * - UB
     - 256 KB
     - Per AICORE
     - Vector unit scratch space, fast element-wise ops
   * - L0A/L0B
     - 64 KB each
     - Per AICORE
     - Cube unit input buffers (A and B matrices)
   * - L0C
     - 256 KB
     - Per AICORE
     - Cube unit output accumulator

Data Movement Patterns
----------------------

**Pattern 1: Simple Vector Operation**

.. code-block:: text

   GM → UB → compute → UB → GM

   1. Load data from GM to UB
   2. Process in Vector unit
   3. Store result from UB to GM

**Pattern 2: Matrix Multiply**

.. code-block:: text

   GM → L1 → L0A ─┐
                  ├─► Cube ─► L0C → L1 → GM
   GM → L1 → L0B ─┘

   1. Load A matrix: GM → L1 → L0A
   2. Load B matrix: GM → L1 → L0B
   3. Compute: L0A × L0B → L0C (accumulated)
   4. Store: L0C → L1 → GM

**Pattern 3: Fused Operation (MatMul + Activation)**

.. code-block:: text

   GM → L1 → L0A ─┐
                  ├─► Cube ─► L0C → UB → Vector → UB → GM
   GM → L1 → L0B ─┘

   Cube computes matrix multiply, result goes to UB,
   Vector applies activation (ReLU, etc.), stores to GM.

Bandwidth Considerations
------------------------

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - Transfer
     - Bandwidth
     - Notes
   * - Host ↔ HBM
     - ~32 GB/s
     - PCIe bottleneck, minimize transfers
   * - HBM ↔ L2
     - ~2 TB/s
     - High bandwidth, but latency matters
   * - L2 ↔ L1
     - Higher
     - On-chip, very fast
   * - L1 ↔ L0
     - Highest
     - Register-like speed

.. tip::

   **Optimization principle**: Keep data in faster memory as long as possible.
   Fuse operations to avoid round-trips to GM.
