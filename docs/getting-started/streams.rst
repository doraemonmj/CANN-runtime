Streams
=======

Streams are command queues that enable asynchronous execution on the NPU.
Operations submitted to a stream execute in order, but different streams
can execute concurrently.

Example Code
------------

Full source: :file:`examples/03-stream/main.cpp`

.. literalinclude:: ../../examples/03-stream/main.cpp
   :language: cpp
   :caption: Stream Operations Example (03-stream/main.cpp)

Concepts
--------

.. code-block:: text

   ┌────────────────────────────────────────────────────────────┐
   │                     Stream Model                           │
   │                                                            │
   │  Stream A:  [memcpy] → [kernel1] → [kernel2] → [memcpy]   │
   │                                                            │
   │  Stream B:  [memcpy] → [kernel3] ─────────────→ [memcpy]   │
   │                                                            │
   │  Operations within a stream: sequential                    │
   │  Operations across streams: potentially parallel           │
   └────────────────────────────────────────────────────────────┘

Default Stream
--------------

A default stream is created during ``platform_init()``. Pass ``NULL`` to
use it:

.. code-block:: cpp

   platform_init(0);

   // These use the default stream
   platform_memcpy_h2d(dev_dst, host_src, size);  // implicit default stream
   platform_kernel_launch(kernel, blocks, args, size, NULL);  // explicit NULL
   platform_stream_sync(NULL);  // sync default stream

Custom Streams
--------------

Create custom streams for concurrent execution:

.. code-block:: cpp

   PlatformStream stream1 = platform_stream_create();
   PlatformStream stream2 = platform_stream_create();

   // Launch work on different streams (can run in parallel)
   platform_kernel_launch(kernel_a, 4, args_a, sizeof(args_a), stream1);
   platform_kernel_launch(kernel_b, 4, args_b, sizeof(args_b), stream2);

   // Wait for both
   platform_stream_sync(stream1);
   platform_stream_sync(stream2);

   // Cleanup
   platform_stream_destroy(stream1);
   platform_stream_destroy(stream2);

API Reference
-------------

``platform_stream_create``
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   PlatformStream platform_stream_create(void);

Create a new stream.

**Returns:**

- Stream handle on success
- ``NULL`` on failure

``platform_stream_destroy``
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void platform_stream_destroy(PlatformStream stream);

Destroy a stream and free its resources.

``platform_stream_sync``
^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   int platform_stream_sync(PlatformStream stream);

Wait for all operations on the stream to complete.

**Parameters:**

- ``stream``: Stream to synchronize (``NULL`` for default)

**Returns:**

- ``PLATFORM_SUCCESS`` when all operations complete
- ``PLATFORM_ERROR_STREAM`` on failure

Synchronization Patterns
------------------------

**Pattern 1: Simple Sequential**

.. code-block:: cpp

   // All operations on default stream, sequential
   platform_memcpy_h2d(dev_input, host_input, size);
   platform_kernel_launch(kernel, blocks, &args, sizeof(args), NULL);
   platform_stream_sync(NULL);  // Wait for kernel
   platform_memcpy_d2h(host_output, dev_output, size);

**Pattern 2: Overlapping Compute and Transfer**

.. code-block:: cpp

   PlatformStream compute_stream = platform_stream_create();
   PlatformStream transfer_stream = platform_stream_create();

   // Overlap: compute on batch N while transferring batch N+1
   for (int batch = 0; batch < num_batches; batch++) {
       // Transfer next batch (if not last)
       if (batch + 1 < num_batches) {
           platform_memcpy_h2d_async(dev_next, host_next, size, transfer_stream);
       }

       // Compute current batch
       platform_kernel_launch(kernel, blocks, &args, sizeof(args), compute_stream);

       // Sync before next iteration
       platform_stream_sync(compute_stream);
       platform_stream_sync(transfer_stream);

       // Swap buffers
       swap(dev_current, dev_next);
   }

**Pattern 3: Multiple Independent Kernels**

.. code-block:: cpp

   // Launch independent kernels on separate streams
   platform_kernel_launch(kernel_conv, 8, &conv_args, sizeof(conv_args), stream1);
   platform_kernel_launch(kernel_bn, 4, &bn_args, sizeof(bn_args), stream2);
   platform_kernel_launch(kernel_relu, 4, &relu_args, sizeof(relu_args), stream3);

   // Wait for all
   platform_stream_sync(stream1);
   platform_stream_sync(stream2);
   platform_stream_sync(stream3);

When to Use Multiple Streams
----------------------------

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Use Multiple Streams
     - Use Single Stream
   * - Independent operations
     - Sequential dependencies
   * - Overlap compute and transfer
     - Simple kernel sequences
   * - Multiple small kernels
     - One large kernel
   * - Pipeline processing
     - Batch processing

Best Practices
--------------

1. **Start simple** - Use default stream until you need more
2. **Profile first** - Don't add streams without measuring benefit
3. **Limit stream count** - 2-4 streams usually sufficient
4. **Always sync** - Ensure completion before reading results
5. **Destroy streams** - Avoid resource leaks
