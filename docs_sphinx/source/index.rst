.. CANN runtime documentation master file, created by
   sphinx-quickstart on Tue Jan 13 10:42:04 2026.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

CANN runtime 开发指南
==========================
本文档旨在为开发者提供 CANN Runtime 库的全面使用指南。内容聚焦于 Host 与 Device 之间的高效交互机制，
详细说明了设备初始化、内存分配与数据传输、执行流管理、事件同步以及自定义内核启动等核心 API 的使用方法。

同时，通过一系列简明实用的代码示例，直观展示从主机端提交任务到昇腾 AI 芯片执行的完整流程，帮助开发者快速掌握底层运行时编程模型。

Github: https://github.com/doraemonmj/CANN-runtime

.. toctree::
   :maxdepth: 2
   :caption: 引言

   user_guide/guide
   user_guide/API


.. toctree::
   :maxdepth: 2
   :caption: 编程指南

   basics/launch/index