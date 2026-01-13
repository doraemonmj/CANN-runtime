runtime API
============
该部分介绍了运行时一些常用的接口。后面补充，如有必要可以做成每个api有一个详细的介绍页以及跳转

.. list-table:: Runtime 接口说明
   :widths: 30 70
   :header-rows: 1

   * - 接口
     - 描述
   * - ``rtMalloc``
     - 分配设备空间，``rtMemType_t=RT_MEMORY_HBM`` 时可在 Device 侧分配
   * - ``rtMemcpy``
     - 拷贝数据，通过参数 ``RT_MEMCPY_HOST_TO_DEVICE`` 和 ``RT_MEMCPY_DEVICE_TO_HOST`` 控制数据拷贝方向

