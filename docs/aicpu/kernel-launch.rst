AICPU Kernel Launch
===================

This page covers how to load and launch AICPU kernels from host code.

.. seealso::

   :file:`examples/05-calling-interface/main.cpp` for detailed argument struct layout examples
   that apply to both AICORE and AICPU kernels.

Kernel Format
-------------

AICPU kernels are compiled as Linux shared libraries (``.so`` files):

.. code-block:: text

   my_kernel.cpp  →  aarch64-linux-gnu-g++  →  my_kernel.so

Requirements:

- Compiled for AArch64 (ARM 64-bit)
- Entry function with ``extern "C"`` linkage
- Position-independent code (``-fPIC``)

Writing an AICPU Kernel
-----------------------

.. code-block:: cpp

   // example_kernel.cpp

   #include <cstdint>

   extern "C" {

   // Argument structure - must match host definition
   struct ScaleArgs {
       void* input;      // Device pointer to input
       void* output;     // Device pointer to output
       int32_t count;    // Number of elements
       float scale;      // Scale factor
   };

   // Entry function - name used in launch call
   void scale_kernel(ScaleArgs* args) {
       float* in = reinterpret_cast<float*>(args->input);
       float* out = reinterpret_cast<float*>(args->output);

       for (int32_t i = 0; i < args->count; i++) {
           out[i] = in[i] * args->scale;
       }
   }

   }  // extern "C"

Compiling
---------

.. code-block:: bash

   # Cross-compile for AArch64 (from x86 host)
   aarch64-linux-gnu-g++ -shared -fPIC -O2 -o scale_kernel.so example_kernel.cpp

   # Or native compile on AArch64 (on Ascend device)
   g++ -shared -fPIC -O2 -o scale_kernel.so example_kernel.cpp

Launch API
----------

.. code-block:: cpp

   int platform_aicpu_launch(
       const void* so_data,     // .so file contents in memory
       size_t so_size,          // Size of .so file
       const char* entry,       // Entry function name
       void* args,              // Pointer to argument struct
       size_t args_size,        // sizeof(argument struct)
       PlatformStream stream    // Stream (NULL for default)
   );

Complete Example
----------------

.. code-block:: cpp

   #include <cstdio>
   #include <cstdlib>
   #include "platform.h"

   // Must match kernel definition exactly
   struct ScaleArgs {
       void* input;
       void* output;
       int32_t count;
       float scale;
   };

   // Helper to read file into memory
   void* read_file(const char* path, size_t* size) {
       FILE* f = fopen(path, "rb");
       if (!f) return nullptr;

       fseek(f, 0, SEEK_END);
       *size = ftell(f);
       fseek(f, 0, SEEK_SET);

       void* data = malloc(*size);
       fread(data, 1, *size, f);
       fclose(f);
       return data;
   }

   int main() {
       platform_init(0);

       // Prepare data
       const int N = 1024;
       float* host_in = (float*)malloc(N * sizeof(float));
       float* host_out = (float*)malloc(N * sizeof(float));
       for (int i = 0; i < N; i++) host_in[i] = (float)i;

       // Allocate device memory
       void* dev_in = platform_malloc(N * sizeof(float));
       void* dev_out = platform_malloc(N * sizeof(float));

       // Copy input to device
       platform_memcpy_h2d(dev_in, host_in, N * sizeof(float));

       // Load kernel .so
       size_t so_size;
       void* so_data = read_file("scale_kernel.so", &so_size);

       // Pack arguments
       ScaleArgs args;
       args.input = dev_in;
       args.output = dev_out;
       args.count = N;
       args.scale = 2.5f;

       // Launch kernel
       int ret = platform_aicpu_launch(
           so_data, so_size,
           "scale_kernel",         // Entry function name
           &args, sizeof(args),
           NULL                    // Default stream
       );

       if (ret != PLATFORM_SUCCESS) {
           printf("Kernel launch failed\n");
           return 1;
       }

       // Sync and get results
       platform_stream_sync(NULL);
       platform_memcpy_d2h(host_out, dev_out, N * sizeof(float));

       // Verify
       printf("Input[0]: %.1f, Output[0]: %.1f (expected %.1f)\n",
              host_in[0], host_out[0], host_in[0] * 2.5f);

       // Cleanup
       free(so_data);
       platform_free(dev_in);
       platform_free(dev_out);
       free(host_in);
       free(host_out);
       platform_shutdown();

       return 0;
   }

Argument Struct Rules
---------------------

1. **Match exactly**: Host and kernel struct must be identical
2. **Use fixed-size types**: ``int32_t`` not ``int``, etc.
3. **Align to 8 bytes**: Add padding if needed
4. **Pointers are device pointers**: From ``platform_malloc``

.. code-block:: cpp

   // CORRECT - well-defined sizes and alignment
   struct GoodArgs {
       void* ptr1;        // 8 bytes, offset 0
       void* ptr2;        // 8 bytes, offset 8
       int32_t count;     // 4 bytes, offset 16
       float value;       // 4 bytes, offset 20
   };  // Total: 24 bytes, 8-byte aligned ✓

   // WRONG - architecture-dependent sizes
   struct BadArgs {
       void* ptr;
       int count;         // 4 or 8 bytes depending on arch!
       long value;        // 4 or 8 bytes depending on arch!
   };

Error Handling
--------------

.. code-block:: cpp

   int ret = platform_aicpu_launch(...);

   switch (ret) {
       case PLATFORM_SUCCESS:
           // Kernel submitted successfully
           break;
       case PLATFORM_ERROR_INIT:
           // Platform not initialized
           break;
       case PLATFORM_ERROR_KERNEL:
           // Invalid kernel or entry point
           break;
       default:
           // Other error
           break;
   }

.. warning::

   Kernel errors may not be detected until ``platform_stream_sync()``.
   Always check sync return value.
