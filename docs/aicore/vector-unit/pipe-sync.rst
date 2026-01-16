Vector Unit Pipe Synchronization
================================

Managing pipeline dependencies in the Vector unit.

Pipeline Overview
-----------------

The Vector unit has multiple pipeline stages:

.. code-block:: text

   Time →
   ────────────────────────────────────────────────────────
   Load Pipe:    [LD1] [LD2] [LD3] [LD4]
   Compute Pipe:       [C1 ] [C2 ] [C3 ] [C4 ]
   Store Pipe:              [ST1] [ST2] [ST3] [ST4]

   Without sync: pipes run independently (overlap = good)
   With sync:    ensure data is ready before next stage

Why Synchronization?
--------------------

.. code-block:: text

   // Problem: compute before data is loaded

   VLOAD ub_x[0:N], gm_addr      // Takes ~100 cycles
   VMUL ub_y[0:N], ub_x[0:N], 2  // Starts immediately!
                                  // But ub_x not ready yet!

   // Solution: sync after load

   VLOAD ub_x[0:N], gm_addr
   PIPE_BARRIER                   // Wait for load to complete
   VMUL ub_y[0:N], ub_x[0:N], 2  // Now ub_x is valid

Synchronization Primitives
--------------------------

**PIPE_BARRIER**

Wait for all pending operations on a pipe:

.. code-block:: text

   VLOAD ub_a, gm_addr_a
   VLOAD ub_b, gm_addr_b
   PIPE_BARRIER           // Wait for both loads
   VADD ub_c, ub_a, ub_b  // Safe to use ub_a, ub_b

**SET_FLAG / WAIT_FLAG**

Fine-grained synchronization with flags:

.. code-block:: text

   VLOAD ub_a, gm_addr_a
   SET_FLAG flag_a        // Signal when ub_a is ready

   VLOAD ub_b, gm_addr_b
   SET_FLAG flag_b        // Signal when ub_b is ready

   // Can do other work here...

   WAIT_FLAG flag_a       // Wait for ub_a only
   // Use ub_a

   WAIT_FLAG flag_b       // Wait for ub_b
   // Use ub_a and ub_b

Common Patterns
---------------

**Pattern 1: Load-Compute-Store**

.. code-block:: text

   for each tile:
       VLOAD ub_in, gm_in + offset
       PIPE_BARRIER              // Wait for load

       VMUL ub_out, ub_in, 2.0
       // More compute...
       PIPE_BARRIER              // Wait for compute

       VSTORE gm_out + offset, ub_out
       PIPE_BARRIER              // Wait for store (if reusing buffer)

**Pattern 2: Double Buffering**

.. code-block:: text

   // Tile 0: Load to A
   VLOAD ub_a, gm_in + 0
   SET_FLAG load_a

   for tile = 1 to N-1:
       // Load next tile to B
       VLOAD ub_b, gm_in + tile * TILE_SIZE
       SET_FLAG load_b

       // Wait for A and compute
       WAIT_FLAG load_a
       VMUL ub_a, ub_a, 2.0
       PIPE_BARRIER

       // Store A
       VSTORE gm_out + (tile-1) * TILE_SIZE, ub_a
       SET_FLAG store_a

       // Swap A and B
       WAIT_FLAG store_a
       WAIT_FLAG load_b
       swap(ub_a, ub_b)
       swap(load_a, load_b)

   // Final tile
   WAIT_FLAG load_a
   VMUL ub_a, ub_a, 2.0
   PIPE_BARRIER
   VSTORE gm_out + (N-1) * TILE_SIZE, ub_a

**Pattern 3: Reduction**

.. code-block:: text

   // Sum all elements
   VLOAD ub_data, gm_in, N
   PIPE_BARRIER

   VREDUCE_SUM ub_sum, ub_data
   PIPE_BARRIER            // Wait for reduction

   VSTORE gm_out, ub_sum, 1
   PIPE_BARRIER            // Ensure store completes

Performance Considerations
--------------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Issue
     - Impact
   * - Too many barriers
     - Serializes execution, loses pipeline benefit
   * - Too few barriers
     - Data hazards, incorrect results
   * - Barrier placement
     - At tile boundaries, not per-instruction

Rules of Thumb
--------------

1. **Barrier after loads** - Before using loaded data
2. **Barrier after compute** - Before storing results
3. **Double buffer** - Overlap load/compute/store
4. **Use flags** - For fine-grained control
5. **Profile** - Find actual bottlenecks

Debugging Sync Issues
---------------------

Symptoms of missing synchronization:

- Random incorrect results
- Results change between runs
- Works with small data, fails with large
- Works with 1 block, fails with many

To debug:

1. Add barriers after every operation (will be slow but correct)
2. Remove barriers one at a time
3. Find minimum barriers needed
