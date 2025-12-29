# CANN-runtime API

该项目主要梳理aicpu和aicore之间交互所使用的接口定义以及数据传递关系

## 项目结构

    .
    ├── basics
    │   ├── 01-shmem-queue
    │   ├── 02-shmem-rt-queue
    │   ├── 03-aicpu-kernel
    │   │   └── kernel
    │   ├── 04-aicore-kernel
    │   │   └── kernel
    │   ├── 05-aicore-pto-kernel
    │   │   └── kernel
    │   ├── 06-calling-interface
    │   │   └── kernel
    │   └── 07-aicpu-aicore
    │       ├── Aicorekernel
    │       └── Aicpukernel
    ├── docs
    ├── simpler
    └── src
        ├── machine
        │   └── host
        └── worker
            └── add

## 指导
[host拉起device侧kernel](basic-usage/launcher-kernel)