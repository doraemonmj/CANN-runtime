Kernel Calling Interface
========================

This page covers the interface between host code and kernel code:
argument packing, alignment rules, and memory address types.

Overview
--------

The calling interface defines how the host code passes arguments to kernels. This applies to
both AICPU and AICORE kernels. For practical examples, see :file:`examples/04-aicpu-basic/`
and later examples which demonstrate real argument passing.

Argument Struct Layout
----------------------

Kernels receive arguments through a packed struct. The host and kernel
must use **identical struct definitions**.

**Simple Vector Operation:**

.. code-block:: cpp

   struct SimpleArgs {
       void* input;      // offset 0:  GM pointer [8 bytes]
       void* output;     // offset 8:  GM pointer [8 bytes]
       int32_t count;    // offset 16: scalar     [4 bytes]
       int32_t _pad;     // offset 20: padding    [4 bytes]
   };  // Total: 24 bytes

**Matrix Multiply:**

.. code-block:: cpp

   struct MatMulArgs {
       void* A;          // offset 0:  [M x K] matrix
       void* B;          // offset 8:  [K x N] matrix
       void* C;          // offset 16: [M x N] matrix
       int32_t M;        // offset 24
       int32_t N;        // offset 28
       int32_t K;        // offset 32
       int32_t _pad;     // offset 36: alignment padding
   };  // Total: 40 bytes

Alignment Rules
---------------

.. list-table::
   :header-rows: 1
   :widths: 30 20 20 30

   * - Type
     - Size
     - Alignment
     - Notes
   * - ``void*``
     - 8
     - 8
     - Device pointers
   * - ``int64_t``
     - 8
     - 8
     -
   * - ``int32_t``
     - 4
     - 4
     -
   * - ``float``
     - 4
     - 4
     -
   * - ``int16_t`` / ``half``
     - 2
     - 2
     -

**Key rules:**

1. Struct must be 8-byte aligned as a whole
2. Add explicit padding fields when needed
3. Use fixed-width types (``int32_t``, not ``int``)

Memory Address Types
--------------------

From the kernel's perspective, there are two memory spaces:

**Global Memory (GM) = HBM:**

- Large (~32-64GB), relatively slow
- Visible to all cores
- Used for input/output tensors
- **Pointers passed as arguments**

**Local Memory (UB, L1, L0):**

- Small (KB to MB), fast
- Private to each core
- Kernel allocates internally
- **NOT passed as arguments**

.. code-block:: cpp

   // Host side
   void* gm_ptr = platform_malloc(size);  // Returns GM address

   // Kernel side
   __global__ void kernel(Args* args) {
       void* gm_input = args->input;      // GM pointer from host

       // Local buffers allocated by kernel
       __local__ float ub_buf[1024];      // UB - fast, private
   }

Common Mistakes
---------------

.. list-table::
   :widths: 50 50

   * - **Wrong**
     - **Correct**
   * - Passing host pointers
     - Use ``platform_malloc`` for all pointers
   * - Forgetting padding
     - Explicit ``_pad`` fields for alignment
   * - Using ``int`` or ``long``
     - Use ``int32_t``, ``int64_t``
   * - Wrong struct size in launch
     - Always use ``sizeof(args)``

Launch Sequence
---------------

.. code-block:: cpp

   // 1. Allocate device memory
   void* input = platform_malloc(size);
   void* output = platform_malloc(size);

   // 2. Copy input data
   platform_memcpy_h2d(input, host_data, size);

   // 3. Pack arguments
   SimpleArgs args = {input, output, count, 0};

   // 4. Launch kernel
   platform_kernel_launch(kernel, blocks, &args, sizeof(args), stream);

   // 5. Synchronize
   platform_stream_sync(stream);

   // 6. Copy output back
   platform_memcpy_d2h(host_result, output, size);
