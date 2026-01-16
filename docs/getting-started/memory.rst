Memory Management
=================

Device memory (HBM) must be allocated before data can be processed on the NPU.
This page covers allocation, transfers, and cleanup.

Initialization
--------------

Before using memory functions, initialize the platform:

.. code-block:: cpp

   int ret = platform_init(0);  // Initialize device 0
   if (ret != PLATFORM_SUCCESS) {
       printf("Failed to initialize platform\n");
       return 1;
   }

   // ... use memory functions ...

   platform_shutdown();  // Cleanup when done

Memory Allocation
-----------------

``platform_malloc``
^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void* platform_malloc(size_t size);

Allocate device memory (HBM).

**Parameters:**

- ``size``: Number of bytes to allocate

**Returns:**

- Device pointer on success
- ``NULL`` on failure

**Example:**

.. code-block:: cpp

   // Allocate 1MB on device
   void* dev_ptr = platform_malloc(1024 * 1024);
   if (!dev_ptr) {
       printf("Allocation failed\n");
   }

``platform_free``
^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void platform_free(void* ptr);

Free previously allocated device memory.

**Parameters:**

- ``ptr``: Device pointer from ``platform_malloc``

Data Transfers
--------------

``platform_memcpy_h2d``
^^^^^^^^^^^^^^^^^^^^^^^

Copy data from host to device (Host → Device).

.. code-block:: cpp

   int platform_memcpy_h2d(void* dst, const void* src, size_t size);

**Parameters:**

- ``dst``: Device destination pointer
- ``src``: Host source pointer
- ``size``: Number of bytes to copy

``platform_memcpy_d2h``
^^^^^^^^^^^^^^^^^^^^^^^

Copy data from device to host (Device → Host).

.. code-block:: cpp

   int platform_memcpy_d2h(void* dst, const void* src, size_t size);

**Parameters:**

- ``dst``: Host destination pointer
- ``src``: Device source pointer
- ``size``: Number of bytes to copy

Complete Example
----------------

.. code-block:: cpp

   #include <cstdio>
   #include <cstdlib>
   #include <cstring>
   #include "platform.h"

   #define N 1024

   int main() {
       // Initialize
       platform_init(0);

       // Allocate host memory
       float* host_src = (float*)malloc(N * sizeof(float));
       float* host_dst = (float*)malloc(N * sizeof(float));

       // Initialize source data
       for (int i = 0; i < N; i++) {
           host_src[i] = (float)i;
       }

       // Allocate device memory
       void* dev_buf = platform_malloc(N * sizeof(float));

       // Copy to device
       platform_memcpy_h2d(dev_buf, host_src, N * sizeof(float));

       // ... launch kernel to process data ...

       // Copy back to host
       platform_memcpy_d2h(host_dst, dev_buf, N * sizeof(float));

       // Verify (if no kernel, should match source)
       int errors = 0;
       for (int i = 0; i < N; i++) {
           if (host_src[i] != host_dst[i]) errors++;
       }
       printf("Errors: %d\n", errors);

       // Cleanup
       platform_free(dev_buf);
       free(host_src);
       free(host_dst);
       platform_shutdown();

       return 0;
   }

Memory Layout
-------------

Device pointers are **not** directly accessible from host code:

.. code-block:: cpp

   void* dev_ptr = platform_malloc(1024);

   // WRONG - will crash or give garbage
   float value = ((float*)dev_ptr)[0];

   // CORRECT - copy to host first
   float value;
   platform_memcpy_d2h(&value, dev_ptr, sizeof(float));

Alignment
---------

Device memory is automatically aligned for optimal performance:

- Default allocation uses 2MB huge pages
- Pointers are at least 512-byte aligned
- No need for manual alignment in most cases

Best Practices
--------------

1. **Minimize transfers** - H2D/D2H are slow compared to compute
2. **Batch transfers** - One large copy is faster than many small ones
3. **Reuse allocations** - Avoid malloc/free in tight loops
4. **Check allocation failures** - HBM is limited, handle OOM gracefully

Common Errors
-------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Error
     - Cause
   * - ``platform_malloc returns NULL``
     - Out of HBM memory
   * - ``memcpy returns error``
     - Platform not initialized, or invalid pointer
   * - ``Data corruption``
     - Passing host pointer as device pointer
   * - ``Crash on access``
     - Dereferencing device pointer on host
