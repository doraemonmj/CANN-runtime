Official Resources
==================

Huawei provides extensive documentation for Ascend development.
This guide complements (not replaces) the official resources.

CANN Documentation
------------------

CANN (Compute Architecture for Neural Networks) is Huawei's software stack for Ascend.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Resource
     - URL
   * - CANN Documentation Portal
     - https://www.hiascend.com/document
   * - CANN Developer Guide
     - https://www.hiascend.com/document/detail/en/canncommercial/
   * - Ascend C Programming Guide
     - https://www.hiascend.com/document/detail/en/canncommercial/700/operatordev/
   * - ACL API Reference
     - https://www.hiascend.com/document/detail/en/canncommercial/700/inferapplicationdev/

Hardware Specifications
-----------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Resource
     - URL
   * - Ascend 910 Specifications
     - https://www.hiascend.com/hardware/product
   * - Atlas Training Server
     - https://e.huawei.com/en/products/computing/ascend

Development Tools
-----------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Tool
     - Description
   * - MindStudio
     - IDE for Ascend development (profiling, debugging)
   * - ATC (Ascend Tensor Compiler)
     - Model conversion and optimization
   * - TIK
     - DSL for writing AICORE kernels
   * - Ascend C
     - C++ extension for AICORE kernels

Community Resources
-------------------

- **Gitee**: https://gitee.com/ascend (official repositories)
- **GitHub**: https://github.com/Ascend (mirrors and community)
- **Forum**: https://www.hiascend.com/forum

Version Compatibility
---------------------

This guide is written for:

- **CANN Version**: 7.0+ / 8.0+
- **Hardware**: Ascend 910A (A2), Ascend 910B/C (A3)
- **Driver**: Matching CANN version

.. note::

   API signatures may change between CANN versions. This guide uses
   high-level ACL APIs which are more stable than internal ``rt*`` APIs.
