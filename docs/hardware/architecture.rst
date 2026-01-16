Architecture Overview
=====================

Simplified Model: 1A + 4B + 24C + 48D
-------------------------------------

We use a simplified notation to describe Ascend architecture:

.. code-block:: text

   1A  = 1 Host CPU
   4B  = Multiple AICPU (control processors)
   24C = 24 AICORE blocks (each with Cube unit)
   48D = 48 Vector units (2 per AICORE)

.. list-table:: Component Latencies
   :header-rows: 1
   :widths: 15 40 25 20

   * - Unit
     - Description
     - Latency from A
     - Latency from B
   * - A
     - Host CPU
     - —
     - 3μs
   * - B
     - AICPU (ARM cores)
     - 3μs
     - —
   * - C
     - AIC (Cube/Matrix unit)
     - 3μs
     - ~0μs
   * - D
     - AIV (Vector unit)
     - 3μs
     - ~0μs

.. note::

   B → C/D latency is near zero because AICPU and AICORE are tightly coupled
   on the same chip.

AICORE Architecture
-------------------

Each AICORE block contains:

.. code-block:: text

   ┌────────────────────────────────────────────────────────┐
   │                      AICORE Block                       │
   │  ┌──────────────────┐  ┌────────────────────────────┐  │
   │  │   Cube Unit      │  │      Vector Unit           │  │
   │  │   (AIC)          │  │      (AIV)                 │  │
   │  │                  │  │                            │  │
   │  │  Matrix Multiply │  │  Element-wise ops          │  │
   │  │  [M,K] × [K,N]   │  │  Activation, Normalization │  │
   │  │                  │  │                            │  │
   │  │  L0A ← Input A   │  │  UB (Unified Buffer)       │  │
   │  │  L0B ← Input B   │  │  256KB fast memory         │  │
   │  │  L0C → Output    │  │                            │  │
   │  └──────────────────┘  └────────────────────────────┘  │
   │                                                        │
   │  ┌──────────────────────────────────────────────────┐  │
   │  │              L1 Buffer (1MB)                     │  │
   │  │         Shared between Cube and Vector          │  │
   │  └──────────────────────────────────────────────────┘  │
   │                                                        │
   │  ┌──────────────────────────────────────────────────┐  │
   │  │              Scalar Unit                         │  │
   │  │         Control flow, address calculation       │  │
   │  └──────────────────────────────────────────────────┘  │
   └────────────────────────────────────────────────────────┘

Cube Unit (AIC)
^^^^^^^^^^^^^^^

- **Purpose**: Matrix multiplication (the "tensor core")
- **Operation**: C = A × B where A is [M,K], B is [K,N]
- **Throughput**: High TFLOPS for FP16/BF16 matrix ops
- **Memory**: L0A, L0B (inputs), L0C (output accumulator)

Vector Unit (AIV)
^^^^^^^^^^^^^^^^^

- **Purpose**: Element-wise operations, reductions
- **Operation**: SIMD operations on vectors
- **Throughput**: Lower than Cube, but more flexible
- **Memory**: UB (Unified Buffer) - 256KB per AICORE

AICPU Architecture
------------------

AICPU are ARM-based control processors:

.. code-block:: text

   ┌─────────────────────────────────────────┐
   │              AICPU                       │
   │  ┌─────────────────────────────────────┐ │
   │  │  ARM Cortex cores                   │ │
   │  │  - Standard C/C++ execution         │ │
   │  │  - Full instruction set             │ │
   │  │  - Can access HBM directly          │ │
   │  └─────────────────────────────────────┘ │
   │                                          │
   │  Use cases:                              │
   │  - Dynamic shape operations             │
   │  - Complex control flow                 │
   │  - Sparse operations                    │
   │  - Data preprocessing                   │
   └─────────────────────────────────────────┘

Execution Model
---------------

Kernels are launched from host and execute on device:

.. code-block:: text

   Host (A)                    Device (B + C + D)
   ────────                    ──────────────────
       │
       │  1. platform_init()
       │─────────────────────────► Initialize runtime
       │
       │  2. platform_malloc()
       │─────────────────────────► Allocate HBM
       │
       │  3. platform_memcpy_h2d()
       │─────────────────────────► DMA transfer
       │
       │  4. platform_kernel_launch()
       │─────────────────────────► Submit to queue
       │                                │
       │                                ▼
       │                          ┌──────────┐
       │                          │ AICPU or │
       │                          │ AICORE   │
       │                          │ executes │
       │                          └──────────┘
       │                                │
       │  5. platform_stream_sync()     │
       │◄───────────────────────────────┘ Wait
       │
       │  6. platform_memcpy_d2h()
       │─────────────────────────► DMA transfer
       │
      Done

Block Parallelism
-----------------

When launching a kernel, you specify the number of blocks:

.. code-block:: cpp

   // Launch on 4 AICORE blocks in parallel
   platform_kernel_launch(kernel, 4, &args, sizeof(args), stream);

Each block:

- Executes the same kernel code
- Has its own local memory (UB, L1, L0)
- Can be identified by ``block_idx`` inside the kernel
- Processes a portion of the total work

.. code-block:: text

   Work Division Example (processing 1024 elements on 4 blocks):

   Block 0: elements [0, 255]
   Block 1: elements [256, 511]
   Block 2: elements [512, 767]
   Block 3: elements [768, 1023]
