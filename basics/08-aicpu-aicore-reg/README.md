# AICPU-AICore 寄存器通信示例

本示例展示了如何通过寄存器实现 AICPU 和 AICore 之间的双向握手通信。这是一个基于硬件寄存器的同步机制，用于协调两种不同处理器之间的任务执行。

## 📁 项目结构

```
08-aicpu-aicore-reg/
├── Aicpukernel/              # AICPU 端内核代码
│   ├── hello_world.cpp       # AICPU 内核实现（通信发起方）
│   ├── device_log.h/cpp      # 设备端日志工具
│   └── CMakeLists.txt        # AICPU 内核编译配置
├── Aicorekernel/             # AICore 端内核代码
│   ├── kernel.cpp            # AICore 内核实现（通信响应方）
│   └── CMakeLists.txt        # AICore 内核编译配置
├── launcher.cpp              # 主机端启动程序
├── regs.h                    # 寄存器地址获取工具
├── CMakeLists.txt            # 主编译配置
└── README.md                 # 本文档
```

## 🔄 通信流程概述

整个通信过程分为三个阶段：主机端准备、AICPU-AICore 握手、结果回收。

### 阶段一：主机端准备（launcher.cpp）

```
主机端 (Host)
    ↓
1. 初始化设备和流
    ↓
2. 获取 AICore 寄存器地址（通过 GetAicoreRegs）
    ↓
3. 准备 DeviceArgs 结构
   - aicpuSoBin: AICPU 后端 SO 文件地址
   - aicpuSoLen: SO 文件大小
   - regs: 寄存器地址数组
   - coreNum: 核心数量
   - fastPath: 同步标志（初始为 false）
    ↓
4. 拷贝 DeviceArgs 到设备内存
    ↓
5. 启动 AICPU 初始化内核
    ↓
6. 启动 AICPU 主内核
    ↓
7. 启动 AICore 内核
    ↓
8. 等待所有内核完成
```

### 阶段二：AICPU-AICore 握手通信

这是核心通信流程，展示了两个处理器如何通过寄存器进行同步：

```
AICPU 端                                AICore 端
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Step 1: 写入数据到寄存器
  ├─ 开启 Fast Path (REG_SPR_FAST_PATH_ENABLE = 0xE)
  ├─ 写入数据到 DATA_MAIN_BASE 寄存器 (值: 1234)
  └─ 内存屏障同步

Step 2: 设置同步标志
  └─ devArgs->fastPath = true  ────────→  等待 fastPath 变为 true
                                            ├─ 轮询检查 (dcci 缓存失效)
                                            └─ 超时保护 (MAX_WAIT_CYCLES)

Step 3: 等待 AICore 响应                 Step 3: 读取寄存器并准备响应
  └─ 轮询 COND 寄存器                      ├─ 读取 DATA_MAIN_BASE 寄存器值
     读取握手消息                           ├─ 获取 blockIdx 和 coreId
                                            └─ 构造握手消息:
                                               [63:48] blockIdx
                                               [47:32] coreId
                                               [31:0]  AICORE_SAY_HELLO (0x80000000)

Step 4: 验证握手消息                     Step 4: 写入握手消息
  ├─ 解析 COND 寄存器值                    └─ set_cond(handshakeMsg)
  │  • 低 32 位: 魔法值 0x80000000              写入 REG_SPR_COND 寄存器
  │  • [47:32]: coreId
  │  • [63:48]: blockIdx
  └─ 验证魔法值是否匹配

Step 5: 等待 AICore 完成                 Step 5: 清除同步标志
  └─ 轮询 fastPath 变为 false  ←────────  devArgs->fastPath = false
     (MAX_POLL_COUNT 次超时保护)              (通知 AICPU 已完成)

Step 6: 关闭 Fast Path
  └─ REG_SPR_FAST_PATH_ENABLE = 0xF
     关闭寄存器访问通道

握手完成 ✓
```

### 阶段三：结果回收

```
主机端从设备内存读取 DeviceArgs
    ↓
检查 devArgs->devId (AICore 读取的寄存器值)
    ↓
验证通信是否成功
```

## 🔑 关键数据结构

### DeviceArgs (在 AICPU 和 AICore 间共享)

```cpp
struct DeviceArgs {
    uint64_t unused[12];           // 预留空间（保证结构体偏移兼容性）
    uint64_t aicpuSoBin;           // AICPU 后端 SO 文件的设备内存地址
    uint64_t aicpuSoLen;           // SO 文件大小
    volatile uint64_t regs;        // 寄存器地址数组的设备内存地址
    uint64_t devId;                // 设备 ID（AICore 会将读取的寄存器值写入此字段）
    uint64_t coreNum;              // 核心数量
    volatile bool fastPath;        // 同步标志（AICPU 和 AICore 通过它同步）
};
```

**重要说明**：
- `fastPath` 是核心同步机制：
  - AICPU 设置为 `true` 通知 AICore 开始工作
  - AICore 设置为 `false` 通知 AICPU 工作完成
- `regs` 指向一个包含所有 AICore 核心寄存器基地址的数组
- 结构体布局必须与 `libaicpu_extend_kernels.so` 的预期偏移一致

## 📝 寄存器说明

### 关键寄存器及其用途

| 寄存器名称 | 偏移地址 | 访问方向 | 用途 |
|----------|---------|---------|-----|
| `REG_SPR_FAST_PATH_ENABLE` | 0x18 | AICPU → AICore | 控制快速路径访问（0xE=开启，0xF=关闭） |
| `REG_SPR_DATA_MAIN_BASE` | 0xA0 | AICPU → AICore | AICPU 写入数据，AICore 读取数据 |
| `REG_SPR_COND` | 0x4C8 | AICore → AICPU | AICore 写入握手消息，AICPU 读取 |

### 握手消息格式（REG_SPR_COND）

```
64 位握手消息结构：
┌─────────────┬─────────────┬──────────────────────┐
│ [63:48]     │ [47:32]     │ [31:0]               │
│ Block Index │ Core ID     │ Magic (0x80000000)   │
└─────────────┴─────────────┴──────────────────────┘
```

## 🔧 编译和运行

### 1. 编译

```bash
mkdir build
cd build
cmake ..
make
```

编译产物：
- `Aicpukernel/libtilefwk_backend_server.so` - AICPU 后端服务库
- `Aicorekernel/kernel.o` - AICore 内核二进制
- `launcher` - 主机端启动程序

### 2. 运行

```bash
# 使用默认设备 ID (9)
./launcher

# 指定设备 ID（范围 0-15）
./launcher 6
```

### 3. 查看日志

设备端日志位置：
```bash
# 设置日志级别（0 = 最详细）
export ASCEND_GLOBAL_LOG_LEVEL=0

# 查看日志
cat ~/ascend/log/debug/device-<device_id>/xxx.log
```

## 🎯 核心函数说明

### AICPU 端 (hello_world.cpp)

| 函数名 | 功能 |
|-------|------|
| `WriteToAicore()` | 向 AICore 寄存器写入数据 |
| `ReadAicoreHandshake()` | 读取并验证 AICore 的握手响应 |
| `WaitForAicoreCompletion()` | 等待 AICore 完成处理（轮询 fastPath） |
| `CloseFastPath()` | 关闭寄存器快速访问通道 |
| `RegisterHandshake()` | 协调完整的握手流程 |
| `DynTileFwkBackendKernelServer()` | 动态内核主入口点 |

### AICore 端 (kernel.cpp)

| 函数名 | 功能 |
|-------|------|
| `aicore_kernel()` | AICore 内核主函数 |
| `get_data_main_base()` | 读取 DATA_MAIN_BASE 寄存器 |
| `get_coreid()` | 获取当前核心 ID |
| `get_block_idx()` | 获取块索引 |
| `set_cond()` | 设置 COND 寄存器（发送握手消息） |
| `dcci()` | 缓存失效指令（确保读取最新数据） |

## ⚠️ 重要注意事项

### 1. 寄存器状态持久性
**问题**：写入寄存器的值会一直保留，即使程序结束也不会清除。

**影响**：下次运行时可能读取到上次的残留数据。

**解决方案**：
- 每次通信开始前重置关键寄存器
- 使用魔法值验证数据有效性
- 通信结束后显式清除寄存器

### 2. 同步机制
- `fastPath` 标志必须使用 `volatile` 修饰
- 轮询时需要使用缓存失效指令 `dcci()`
- 使用内存屏障 `__sync_synchronize()` 确保顺序性

### 3. 超时保护
- AICPU 等待 AICore：`MAX_POLL_COUNT = 100,000` 次轮询
- AICore 等待 AICPU：`MAX_WAIT_CYCLES = 10,000,000,000` 次轮询

### 4. 结构体布局
`DeviceArgs` 的内存布局必须严格匹配，因为偏移量是硬编码在 `libaicpu_extend_kernels.so` 中的。

## 📊 执行时序图

```
时间轴  Host              AICPU                    AICore
  │      │                  │                        │
  ├──────┤                  │                        │
  │ Init │                  │                        │
  ├──────┤                  │                        │
  │      ├─ Launch Init ───→│                        │
  │      │                  ├─ InitLogSwitch()       │
  │      │                  └─ Return                │
  │      │                  │                        │
  │      ├─ Launch AICPU ──→│                        │
  │      │                  ├─ WriteToAicore()       │
  │      │                  ├─ fastPath = true ─────→│
  │      │                  │                        ├─ Wait fastPath
  │      ├─ Launch AICore ─────────────────────────→│
  │      │                  │                        ├─ Read register
  │      │                  │                        ├─ set_cond()
  │      │                  ├─ ReadAicoreHandshake() │
  │      │                  ├─ Verify magic value    │
  │      │                  ├─ Wait fastPath ────────┤
  │      │                  │                        ├─ fastPath = false
  │      │                  ├─ CloseFastPath()       │
  │      │                  └─ Return                └─ Return
  │      │                  │                        │
  ├──────┤                  │                        │
  │ Sync │                  │                        │
  ├──────┤                  │                        │
  │ Read │                  │                        │
  │Result│                  │                        │
  └──────┘                  │                        │
```

## 🔍 调试技巧

### 1. 启用详细日志
```bash
export ASCEND_GLOBAL_LOG_LEVEL=0
```

### 2. 检查握手消息
在 AICPU 端，`ReadAicoreHandshake()` 会打印：
```
Core[X] COND register: magic=0x80000000, coreId=Y, blockIdx=Z
```

### 3. 验证寄存器写入
查看日志中的：
```
[AICPU->AICORE] Wrote 0x4d2 to offset 0xa0
```

### 4. 检查超时
如果出现超时，检查：
- `fastPath` 标志是否正确设置
- 缓存是否正确失效
- 两个内核是否都成功启动

## 📚 扩展阅读

- **Fast Path 机制**：允许 AICPU 直接访问 AICore 的特殊寄存器
- **COND 寄存器**：AICore 专用的条件寄存器，用于向 AICPU 发送信号
- **DATA_MAIN_BASE**：AICore 的数据主基址寄存器，可被 AICPU 写入

## 🎓 学习要点

1. ✅ 理解双向握手协议的设计
2. ✅ 掌握寄存器级别的进程间通信
3. ✅ 熟悉 volatile 和内存屏障的使用
4. ✅ 了解异构处理器协同工作机制
5. ✅ 掌握设备端日志调试方法

---

**版本信息**：基于 CANN 框架的寄存器通信示例
**最后更新**：2026-01
