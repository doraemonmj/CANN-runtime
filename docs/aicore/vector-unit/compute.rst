Vector Compute Operations
=========================

Element-wise operations using PTO-ISA vector instructions.

Instruction Categories
----------------------

.. list-table::
   :header-rows: 1
   :widths: 25 35 40

   * - Category
     - Operations
     - Example
   * - Arithmetic
     - add, sub, mul, div
     - ``VADD dst, src1, src2``
   * - Math
     - sqrt, rsqrt, exp, log
     - ``VEXP dst, src``
   * - Activation
     - relu, sigmoid, tanh
     - ``VRELU dst, src``
   * - Comparison
     - max, min, gt, lt
     - ``VMAX dst, src1, src2``
   * - Reduction
     - sum, max, min
     - ``VREDUCE_SUM dst, src``
   * - Type Convert
     - fp16↔fp32, int8↔fp16
     - ``VCONV_F16_F32 dst, src``
   * - Memory
     - load, store, copy
     - ``VLOAD dst, addr``

Basic Vector Operations
-----------------------

**Vector Add (VADD)**

.. code-block:: text

   VADD  ub_dst[0:N], ub_src1[0:N], ub_src2[0:N]

   // C equivalent:
   for (int i = 0; i < N; i++) {
       dst[i] = src1[i] + src2[i];
   }

**Vector Multiply (VMUL)**

.. code-block:: text

   VMUL  ub_dst[0:N], ub_src1[0:N], ub_src2[0:N]

**Vector Scalar (VMULS)**

.. code-block:: text

   VMULS ub_dst[0:N], ub_src[0:N], scalar_value

   // Multiply all elements by scalar
   for (int i = 0; i < N; i++) {
       dst[i] = src[i] * scalar_value;
   }

Activation Functions
--------------------

**ReLU**

.. code-block:: text

   VRELU ub_dst[0:N], ub_src[0:N]

   // max(0, x)
   for (int i = 0; i < N; i++) {
       dst[i] = src[i] > 0 ? src[i] : 0;
   }

**Sigmoid**

.. code-block:: text

   VSIGMOID ub_dst[0:N], ub_src[0:N]

   // 1 / (1 + exp(-x))

**GELU**

.. code-block:: text

   VGELU ub_dst[0:N], ub_src[0:N]

   // 0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3)))

Reduction Operations
--------------------

**Sum Reduction**

.. code-block:: text

   VREDUCE_SUM ub_dst[0], ub_src[0:N]

   // Sum all elements
   dst[0] = 0;
   for (int i = 0; i < N; i++) {
       dst[0] += src[i];
   }

**Max Reduction**

.. code-block:: text

   VREDUCE_MAX ub_dst[0], ub_src[0:N]

   // Find maximum
   dst[0] = src[0];
   for (int i = 1; i < N; i++) {
       dst[0] = max(dst[0], src[i]);
   }

Type Conversion
---------------

**FP16 to FP32**

.. code-block:: text

   VCONV_F16_F32 ub_dst[0:N], ub_src[0:N]

   // Widen FP16 to FP32
   // Note: dst needs 2x space

**FP32 to FP16**

.. code-block:: text

   VCONV_F32_F16 ub_dst[0:N], ub_src[0:N]

   // Narrow FP32 to FP16

Data Movement
-------------

**Load from GM to UB**

.. code-block:: text

   VLOAD ub_dst[0:N], gm_addr, N

**Store from UB to GM**

.. code-block:: text

   VSTORE gm_addr, ub_src[0:N], N

**Copy within UB**

.. code-block:: text

   VCOPY ub_dst[0:N], ub_src[0:N]

Fused Operations
----------------

Some operations can be fused for efficiency:

**Multiply-Add (MAD)**

.. code-block:: text

   VMAD ub_dst[0:N], ub_a[0:N], ub_b[0:N], ub_c[0:N]

   // dst = a * b + c

**Add-ReLU**

.. code-block:: text

   VADD_RELU ub_dst[0:N], ub_src1[0:N], ub_src2[0:N]

   // dst = relu(src1 + src2)

Softmax Example
---------------

Complete softmax implementation:

.. code-block:: text

   // Input: ub_x[0:N]
   // Output: ub_y[0:N]
   // Temp: ub_tmp[0:N]

   // 1. Find max for numerical stability
   VREDUCE_MAX ub_max[0], ub_x[0:N]

   // 2. Subtract max and exp
   VSUBS ub_tmp[0:N], ub_x[0:N], ub_max[0]
   VEXP ub_tmp[0:N], ub_tmp[0:N]

   // 3. Sum of exp
   VREDUCE_SUM ub_sum[0], ub_tmp[0:N]

   // 4. Reciprocal
   VRECIP ub_sum[0], ub_sum[0]

   // 5. Normalize
   VMULS ub_y[0:N], ub_tmp[0:N], ub_sum[0]

Performance Tips
----------------

1. **Maximize vector width** - Process 256 elements (FP16) per instruction
2. **Fuse operations** - Use combined instructions when available
3. **Minimize data movement** - Keep data in UB for multiple operations
4. **Use appropriate precision** - FP16 has 2x throughput of FP32

Example Code
------------

Full working example: :file:`examples/14-vector-simd/`

Host code demonstrating vector operations:

.. literalinclude:: ../../../examples/14-vector-simd/main.cpp
   :language: cpp
   :caption: 14-vector-simd/main.cpp
   :lines: 1-80

AICORE vector kernel (PTO-ISA):

.. literalinclude:: ../../../examples/14-vector-simd/kernel.pto
   :language: text
   :caption: 14-vector-simd/kernel.pto
