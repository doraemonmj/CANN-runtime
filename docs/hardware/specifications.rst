Hardware Specifications
=======================

This page lists hardware specifications for Ascend NPU generations.

Ascend 910 Series
-----------------

.. list-table:: Ascend 910 Variants
   :header-rows: 1
   :widths: 25 25 25 25

   * - Specification
     - 910A (A2)
     - 910B (A3)
     - 910C (A3)
   * - Architecture
     - DAV_2201
     - DAV_3510
     - DAV_3510
   * - AICORE Blocks
     - 32
     - 24
     - 24
   * - AIC (Cube) per block
     - 1
     - 1
     - 1
   * - AIV (Vector) per block
     - 2
     - 2
     - 2
   * - AICPU Count
     - 8
     - 8
     - 8
   * - HBM Capacity
     - 32 GB
     - 64 GB
     - 64 GB
   * - HBM Bandwidth
     - ~1.2 TB/s
     - ~1.6 TB/s
     - ~2.0 TB/s
   * - FP16 TFLOPS
     - 320
     - 320
     - 400+
   * - TDP
     - 310W
     - 400W
     - 400W

Per-Core Memory Sizes
---------------------

.. list-table:: AICORE Memory (A3 / 910C)
   :header-rows: 1
   :widths: 30 30 40

   * - Buffer
     - Size
     - Purpose
   * - L2 Cache
     - 192 MB (shared)
     - Hardware-managed cache
   * - L1 Buffer
     - 1 MB
     - Staging for Cube operations
   * - Unified Buffer (UB)
     - 256 KB
     - Vector unit scratch space
   * - L0A Buffer
     - 64 KB
     - Cube input A
   * - L0B Buffer
     - 64 KB
     - Cube input B
   * - L0C Buffer
     - 256 KB
     - Cube output accumulator

Querying Specifications at Runtime
----------------------------------

Use the platform library to query device info:

.. code-block:: cpp

   #include "platform.h"

   PlatformDeviceInfo info;
   platform_get_device_info(0, &info);

   printf("SoC: %s\n", info.soc_version);
   printf("Architecture: %d\n", info.arch);
   printf("AICORE count: %u\n", info.aicore_cnt);
   printf("HBM size: %lu bytes\n", info.hbm_size);
   printf("UB size: %lu bytes\n", info.ub_size);

Architecture Detection
----------------------

.. code-block:: cpp

   // Determine architecture from SoC string
   if (strstr(info.soc_version, "910B") ||
       strstr(info.soc_version, "910C")) {
       // A3 architecture (DAV_3510)
   } else if (strstr(info.soc_version, "910")) {
       // A2 architecture (DAV_2201)
   }

Power and Thermal
-----------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Aspect
     - Notes
   * - TDP
     - 300-400W depending on variant
   * - Cooling
     - Requires active cooling (server fans)
   * - Operating Temp
     - 0-45°C ambient
   * - Throttling
     - Automatic frequency scaling on thermal limit
