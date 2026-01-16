Cube Unit Pipe Synchronization
==============================

Managing dependencies between Cube and other units.

Cube Pipeline
-------------

.. code-block:: text

   Cube execution pipeline:

   Stage 1: Load A (GM → L1 → L0A)
   Stage 2: Load B (GM → L1 → L0B)
   Stage 3: Compute (L0A × L0B → L0C)
   Stage 4: Store (L0C → L1 → GM or UB)

   These stages can overlap with proper synchronization.

Cube-Vector Synchronization
---------------------------

Common pattern: Cube computes, Vector post-processes.

.. code-block:: text

   // Matrix multiply + ReLU

   // Cube computation
   LOAD L0A, L1_A
   LOAD L0B, L1_B
   MMAD L0C, L0A, L0B
   CUBE_PIPE_BARRIER         // Wait for Cube to finish

   // Move result to UB for Vector processing
   MOVE UB_result, L0C       // L0C → L1 → UB
   MTE_PIPE_BARRIER          // Wait for data movement

   // Vector post-processing
   VRELU UB_result, UB_result
   VEC_PIPE_BARRIER          // Wait for Vector

   // Store final result
   VSTORE GM_output, UB_result
   STORE_PIPE_BARRIER

Pipeline Barriers
-----------------

Different barriers for different pipelines:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Barrier
     - Purpose
   * - ``CUBE_PIPE_BARRIER``
     - Wait for Cube operations
   * - ``VEC_PIPE_BARRIER``
     - Wait for Vector operations
   * - ``MTE_PIPE_BARRIER``
     - Wait for Memory Transfer Engine
   * - ``SCALAR_PIPE_BARRIER``
     - Wait for Scalar operations

Double-Buffered Pipeline
------------------------

Full pipeline with overlapping:

.. code-block:: text

   // Double-buffered GEMM

   // Initialize: Load first tiles
   LOAD L1_A[0], GM_A[0]
   LOAD L1_B[0], GM_B[0]
   MTE_PIPE_BARRIER

   MOVE L0A, L1_A[0]
   MOVE L0B, L1_B[0]
   MTE_PIPE_BARRIER

   for tile = 1 to N:
       // Start loading next tiles (overlaps with compute)
       LOAD L1_A[tile%2], GM_A[tile]
       LOAD L1_B[tile%2], GM_B[tile]

       // Compute current tile
       MMAD L0C, L0A, L0B, L0C
       CUBE_PIPE_BARRIER

       // Wait for next tile load
       MTE_PIPE_BARRIER

       // Move next tiles to L0
       MOVE L0A, L1_A[tile%2]
       MOVE L0B, L1_B[tile%2]
       MTE_PIPE_BARRIER

   // Final tile
   MMAD L0C, L0A, L0B, L0C
   CUBE_PIPE_BARRIER

Event-Based Synchronization
---------------------------

For fine-grained control:

.. code-block:: text

   // Set event when operation completes
   LOAD L1_A, GM_A
   SET_EVENT event_a_loaded

   LOAD L1_B, GM_B
   SET_EVENT event_b_loaded

   // Wait for specific event
   WAIT_EVENT event_a_loaded
   MOVE L0A, L1_A

   WAIT_EVENT event_b_loaded
   MOVE L0B, L1_B

   // Compute after both loaded
   MMAD L0C, L0A, L0B

Cross-Unit Dependencies
-----------------------

When Cube and Vector work on related data:

.. code-block:: text

   // Cube produces, Vector consumes

   // Cube: C = A × B
   MMAD L0C, L0A, L0B
   CUBE_PIPE_BARRIER

   // Move Cube output to UB
   MOVE UB_temp, L0C
   MTE_PIPE_BARRIER

   // Vector: Apply activation
   VRELU UB_out, UB_temp
   VEC_PIPE_BARRIER

   // Store
   VSTORE GM_out, UB_out

Performance Impact
------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Issue
     - Impact
   * - Missing barrier
     - Data corruption, wrong results
   * - Too many barriers
     - Stalls pipeline, reduces throughput
   * - Wrong barrier type
     - May not wait for intended operation

Best Practices
--------------

1. **Barrier at stage boundaries** - Between load/compute/store
2. **Match barrier to operation** - Use correct barrier type
3. **Double buffer** - Minimize stalls
4. **Profile** - Measure actual pipeline utilization
