Ascend NPU Programming Guide
=============================

A practical guide to programming Huawei Ascend NPUs, focusing on hardware concepts
and low-level interfaces. This guide teaches the same concepts as the CUDA Programming
Guide, but for Ascend hardware.

.. note::

   This guide focuses on **understanding hardware**, not on using frameworks.
   We teach how kernels work, not how to use PyTorch or TensorFlow.

Quick Start
-----------

.. code-block:: cpp

   #include "platform.h"

   int main() {
       // Initialize device
       platform_init(0);

       // Allocate device memory
       void* dev_ptr = platform_malloc(1024);

       // Copy data to device
       platform_memcpy_h2d(dev_ptr, host_data, 1024);

       // Launch kernel...

       // Cleanup
       platform_free(dev_ptr);
       platform_shutdown();
   }

Contents
--------

.. toctree::
   :maxdepth: 2
   :caption: Introduction

   introduction/overview
   introduction/official-resources

.. toctree::
   :maxdepth: 2
   :caption: Hardware Architecture

   hardware/architecture
   hardware/memory-hierarchy
   hardware/specifications

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   getting-started/device-query
   getting-started/memory
   getting-started/streams

.. toctree::
   :maxdepth: 2
   :caption: AICPU Programming

   aicpu/overview
   aicpu/kernel-launch
   aicpu/logging
   aicpu/atomic
   aicpu/queue

.. toctree::
   :maxdepth: 2
   :caption: AICORE Programming

   aicore/overview
   aicore/kernel-launch
   aicore/calling-interface
   aicore/vector-unit/index
   aicore/cube-unit/index
   aicore/atomic
   aicore/pmu

.. toctree::
   :maxdepth: 2
   :caption: Synchronization

   synchronization/overview
   synchronization/register
   synchronization/atomic
   synchronization/queue

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
