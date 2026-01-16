Device Query
============

The first step in NPU programming is discovering available devices
and their capabilities.

Example Code
------------

.. code-block:: cpp

   #include <cstdio>
   #include "platform.h"

   int main() {
       // Get device count
       uint32_t device_count = 0;
       int ret = platform_get_device_count(&device_count);
       if (ret != PLATFORM_SUCCESS) {
           printf("Failed to get device count\n");
           return 1;
       }
       printf("Found %u NPU device(s)\n", device_count);

       // Query each device
       for (uint32_t i = 0; i < device_count; i++) {
           PlatformDeviceInfo info;
           platform_get_device_info(i, &info);

           printf("Device %u:\n", i);
           printf("  SoC: %s\n", info.soc_version);
           printf("  AICORE count: %u\n", info.aicore_cnt);
           printf("  HBM: %lu GB\n", info.hbm_size / (1024*1024*1024));
       }

       return 0;
   }

API Reference
-------------

``platform_get_device_count``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   int platform_get_device_count(uint32_t* count);

Get the number of available NPU devices.

**Parameters:**

- ``count``: Output parameter for device count

**Returns:**

- ``PLATFORM_SUCCESS`` on success
- ``PLATFORM_ERROR`` on failure

``platform_get_device_info``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   int platform_get_device_info(int device_id, PlatformDeviceInfo* info);

Query detailed information about a device.

**Parameters:**

- ``device_id``: Device index (0-based)
- ``info``: Output structure for device information

**Returns:**

- ``PLATFORM_SUCCESS`` on success
- ``PLATFORM_ERROR`` on failure

``PlatformDeviceInfo`` Structure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   typedef struct {
       char soc_version[64];   // SoC name, e.g., "Ascend910C"
       PlatformArch arch;      // Architecture enum
       uint32_t aicore_cnt;    // Number of AICORE blocks
       uint32_t aic_cnt;       // Cube units
       uint32_t aiv_cnt;       // Vector units
       uint32_t aicpu_cnt;     // AICPU count
       uint64_t hbm_size;      // Total HBM in bytes
       uint64_t hbm_free;      // Available HBM (after init)
       uint64_t l2_size;       // L2 cache size
       uint64_t ub_size;       // UB per core
       uint64_t l0a_size;      // L0A buffer size
       uint64_t l0b_size;      // L0B buffer size
       uint64_t l0c_size;      // L0C buffer size
       uint64_t l1_size;       // L1 buffer size
   } PlatformDeviceInfo;

Sample Output
-------------

.. code-block:: text

   === Ascend NPU Device Query ===

   Found 16 NPU device(s)

   --- Device 0 ---
     SoC Version         : Ascend910_9392
     Architecture        : DAV_2201 (A2)

     Core Configuration:
       AICORE blocks    : 24
       AIC (Cube) cores : 24
       AIV (Vector) cores: 48
       AICPU count      : 8

     Memory Hierarchy:
     HBM Total           : 32.00 GB
     HBM Free            : 30.50 GB
     L2 Cache            : 192.00 MB
     L1 Buffer           : 1.00 MB
     Unified Buffer (UB) : 256.00 KB
     L0A Buffer          : 64.00 KB
     L0B Buffer          : 64.00 KB
     L0C Buffer          : 256.00 KB

.. note::

   HBM free size is only accurate after calling ``platform_init()``.
   Before initialization, it may show 0.

Best Practices
--------------

1. **Always check device count** before assuming devices exist
2. **Handle multi-device systems** - servers often have 8-16 NPUs
3. **Check architecture** if your kernel has different code paths
4. **Log device info** at startup for debugging
