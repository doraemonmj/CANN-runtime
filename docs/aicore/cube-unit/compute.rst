Cube Compute Operations
=======================

Matrix multiplication instructions for the Cube unit.

Basic Matrix Multiply
---------------------

**MMAD (Matrix Multiply-Add)**

.. code-block:: text

   MMAD L0C, L0A, L0B        // C = A × B
   MMAD L0C, L0A, L0B, L0C   // C = A × B + C (accumulate)

This is the fundamental Cube operation.

Data Layout
-----------

Matrices in L0A and L0B must be in specific layouts:

**L0A Layout (Row-major for A)**

.. code-block:: text

   A[M, K] in L0A:
   ┌─────────────────────────┐
   │ a00 a01 a02 ... a0,K-1  │  ← Row 0
   │ a10 a11 a12 ... a1,K-1  │  ← Row 1
   │ ...                     │
   │ aM-1,0 ... aM-1,K-1     │  ← Row M-1
   └─────────────────────────┘

**L0B Layout (Column-major for B)**

.. code-block:: text

   B[K, N] in L0B:
   ┌───────────────────────────────────┐
   │ b00 b10 b20 ... bK-1,0            │  ← Col 0
   │ b01 b11 b21 ... bK-1,1            │  ← Col 1
   │ ...                               │
   │ b0,N-1 b1,N-1 ... bK-1,N-1        │  ← Col N-1
   └───────────────────────────────────┘

.. note::

   B is transposed compared to standard row-major. This layout
   enables efficient dot products between A rows and B columns.

Tile Dimensions
---------------

The Cube operates on fixed tile sizes:

.. code-block:: text

   FP16:  [M=16, K=16] × [K=16, N=16] → [M=16, N=16]
   INT8:  [M=16, K=32] × [K=32, N=16] → [M=16, N=16]

For larger matrices, tile and iterate:

.. code-block:: text

   // C[128, 256] = A[128, 512] × B[512, 256]
   // Using FP16 tiles: 16×16×16

   for m_tile = 0 to 128 step 16:
       for n_tile = 0 to 256 step 16:
           clear(L0C[m_tile, n_tile])  // Initialize accumulator

           for k_tile = 0 to 512 step 16:
               load L0A = A[m_tile:m_tile+16, k_tile:k_tile+16]
               load L0B = B[k_tile:k_tile+16, n_tile:n_tile+16]
               MMAD L0C, L0A, L0B, L0C  // Accumulate

           store C[m_tile:m_tile+16, n_tile:n_tile+16] = L0C

Accumulator (L0C)
-----------------

L0C stores partial results in higher precision:

.. code-block:: text

   Input A, B: FP16
   Accumulator: FP32 (internal)
   Output: FP16 (converted on store)

This prevents overflow during long accumulations (large K).

Quantized Operations
--------------------

For INT8 matrix multiply:

.. code-block:: text

   // A: INT8 [M, K]
   // B: INT8 [K, N]
   // C: INT32 (accumulated)

   MMAD_INT8 L0C, L0A, L0B, L0C

   // Final dequantization
   C_fp16 = (C_int32 - zero_point) * scale

Convolution as MatMul
---------------------

Convolution is implemented as im2col + MatMul:

.. code-block:: text

   // Conv2D: Input[N,Ci,H,W] × Kernel[Co,Ci,Kh,Kw]

   1. im2col: Transform input patches to matrix columns
      Input[N,Ci,H,W] → A[N*Ho*Wo, Ci*Kh*Kw]

   2. Reshape kernel
      Kernel[Co,Ci,Kh,Kw] → B[Ci*Kh*Kw, Co]

   3. Matrix multiply
      C[N*Ho*Wo, Co] = A × B

   4. Reshape output
      C → Output[N,Co,Ho,Wo]

Performance Metrics
-------------------

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - Metric
     - A3 (910C)
     - Notes
   * - FP16 TFLOPS
     - ~320
     - Peak theoretical
   * - INT8 TOPS
     - ~640
     - 2× FP16
   * - Ops per Cube cycle
     - 4096
     - 16×16×16
   * - Cube frequency
     - ~1.5 GHz
     - Varies by chip

Achieving peak performance requires:

1. Keeping L0A/L0B fed (memory bandwidth)
2. Large enough tiles (reduce overhead)
3. Full K accumulation (amortize load cost)
