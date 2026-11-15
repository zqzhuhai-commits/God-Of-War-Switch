# 构建说明

1. 克隆UPSTREAM.json中的上游仓库，检出指定提交。
2. 执行`python scripts/apply-runtime.py /path/to/upstream`。脚本检查基线和补丁后应用到指定目录。
3. 使用自己的匹配启动ELF，在本地生成EE runner、注册表/头文件、IOP原生模块、VU1编译产物。汉化版与美版地址不同。生成物放在上游预期目录，不随本仓库发布。本仓库尚不提供从ISO开始的一键完整生成流程。
4. 安装devkitPro Switch工具链、libnx、deko3d、SDL2等依赖。提供PS2X_VU1_PREGENERATED_DIR（vu1_generado/vu1n_generado，后者为局部寄存器/chainTo版本）及PS2X_DK_SHADERS（包含对应着色器变体的生成CPP）。
5. 执行`scripts/build-switch.ps1`，传入上述本地路径。Windows使用PowerShell，其他平台可按脚本参数运行等价CMake命令。

构建出的ps2EntryRunner.nro在SD上命名为switch/gow/gow.nro；复制configs/720p下switch目录到SD根目录。资源由用户自备，放在switch/gow/disc-gows16941，保留play.mcd。已有前端指向sdmc:/switch/gow/gow.nro时无需随NRO更新反复安装。

固定场景/操作/频率后，对照GOW_FRAME_AUDIO_CATCHUP=0/1，记录GOW_AUDIO_STATS和GOW_PERF_DIAG=frames。PC结果不能代替Switch实机验证。
