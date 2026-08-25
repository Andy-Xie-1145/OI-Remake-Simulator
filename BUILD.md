# OI 重开模拟器 GUI 版本

## 编译说明

### 方法一：使用 Visual Studio (推荐)

1. 打开 "x64 Native Tools Command Prompt for VS"
2. 进入项目目录
3. 运行 `compile.bat`

### 方法二：使用 CMake

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 依赖

- Visual Studio 2019 或更高版本
- Windows SDK
- DirectX 11 (Windows 自带)

## 运行

编译成功后，可执行文件在 `output/oi_simulator_gui.exe`

## 项目结构

```
OI_Simulator_GUI/
├── main.cpp           # 主程序（UI 视图）
├── month_engine.hpp   # 月度引擎（回合流程规则）
├── types.hpp          # 类型定义
├── problem_pool.hpp   # 题目池
├── events.hpp         # 属性变更入口 + 商店
├── game.hpp           # 游戏逻辑
├── tests/             # Catch2 测试（tests/build_test_mingw.bat）
├── imgui/             # ImGui 库
└── CMakeLists.txt     # CMake 配置
```
