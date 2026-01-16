Overview
========

What is Ascend NPU?
-------------------

Ascend is Huawei's AI processor architecture, designed for deep learning training
and inference. The Ascend 910 series is comparable to NVIDIA's A100/H100 in the
datacenter AI accelerator market.

.. list-table:: Ascend vs CUDA Terminology
   :header-rows: 1
   :widths: 30 30 40

   * - CUDA Concept
     - Ascend Equivalent
     - Notes
   * - GPU
     - NPU
     - Neural Processing Unit
   * - CUDA Core
     - AICORE
     - Contains Cube + Vector units
   * - Tensor Core
     - Cube Unit (AIC)
     - Matrix multiplication
   * - CUDA Thread
     - Vector Lane
     - SIMD execution
   * - Shared Memory
     - Unified Buffer (UB)
     - Per-core fast memory
   * - Global Memory
     - HBM (GM)
     - High Bandwidth Memory
   * - cudaStream
     - aclrtStream
     - Async execution queue

Programming Model
-----------------

The Ascend programming model can be simplified as:

.. code-block:: text

   ┌─────────────────────────────────────────────────────────────┐
   │                         Host (A)                            │
   │                     CPU + Host Memory                       │
   │                                                             │
   │  Your program runs here. Launches kernels to device.        │
   └─────────────────────────────────────────────────────────────┘
                              │
                              │ PCIe / Chip Link
                              ▼
   ┌─────────────────────────────────────────────────────────────┐
   │                      Device (NPU)                           │
   │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
   │  │   AICPU     │  │   AICPU     │  │   ...       │  (B)    │
   │  │  Control    │  │  Control    │  │             │         │
   │  └─────────────┘  └─────────────┘  └─────────────┘         │
   │         │                │                                  │
   │         ▼                ▼                                  │
   │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
   │  │  AICORE 0   │  │  AICORE 1   │  │   ...       │  (C+D)  │
   │  │ ┌───┬─────┐ │  │ ┌───┬─────┐ │  │             │         │
   │  │ │AIC│ AIV │ │  │ │AIC│ AIV │ │  │             │         │
   │  │ └───┴─────┘ │  │ └───┴─────┘ │  │             │         │
   │  └─────────────┘  └─────────────┘  └─────────────┘         │
   │                                                             │
   │  ┌─────────────────────────────────────────────────────┐   │
   │  │                    HBM (Global Memory)               │   │
   │  │                      32-64 GB                        │   │
   │  └─────────────────────────────────────────────────────┘   │
   └─────────────────────────────────────────────────────────────┘

Key Concepts
------------

**Two types of device code:**

1. **AICORE Kernels** (high throughput)

   - Run on Cube (matrix) and Vector units
   - Written in TIK, Ascend C, or PTO-ISA
   - Compiled to ``.o`` binary format
   - Best for: matrix multiply, convolution, element-wise ops

2. **AICPU Kernels** (flexible)

   - Run on ARM control processors
   - Written in standard C/C++
   - Compiled to ``.so`` shared library
   - Best for: dynamic shapes, control flow, complex indexing

**Execution flow:**

1. Host allocates device memory (``platform_malloc``)
2. Host copies input data to device (``platform_memcpy_h2d``)
3. Host launches kernel(s) on a stream
4. Host synchronizes (``platform_stream_sync``)
5. Host copies results back (``platform_memcpy_d2h``)

Who This Guide Is For
---------------------

- Developers who want to understand NPU hardware
- Framework developers (PyTorch, TensorFlow backend)
- Kernel optimization engineers
- Students learning parallel computing

This guide does **NOT** cover:

- Using high-level frameworks (use official Huawei docs)
- Model training workflows
- Distributed computing
