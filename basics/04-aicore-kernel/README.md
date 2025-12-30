# AICORE Add Demo

环境前置：必须设置 `ASCEND_HOME_PATH`，其中包含 `compiler/ccec_compiler/bin/bisheng` 与 CANN 头库。

构建与运行（在本目录）：
```bash
mkdir -p build && cd build
cmake ..
make -j4
./launcher 0   # 可选设备号，默认0
```

流程说明：
- `kernel/CMakeLists.txt` 使用 bisheng 编译 AIC/AIV 双路：`--cce-aicore-arch=dav-c220-cube` 与 `--cce-aicore-arch=dav-c220-vec`，链接出 `kernel/kernel.o`。
- `launcher` 运行时从 `./kernel/kernel.o` 读入二进制，调用 `rtRegisterAllKernel` 注册，`rtKernelLaunchWithHandleV2` 启动，回读校验结果。

代码要点：
- `kernel/kernel.cpp`：简单位址核，AIC 路入口 `aicore_kernel_0_mix_aic`，AIV 路入口 `aicore_kernel_0_mix_aiv`，对 `Out[blockIdx * Stride]` 写入 blockIdx。
- `launcher.cpp`：准备 HBM 输出、设定 blockDim=4、Stride=64，注册内核后执行并校验。

调整项：
- 如需更改架构或 bisheng 路径，修改 `kernel/CMakeLists.txt` 中 `--cce-aicore-arch` 或 `BISHENG_COMPILER/BISHENG_LD`。

