# OI 重开模拟器 v0.1.1

> 🎮 人生重开模拟器的 OI 竞赛版本 —— 体验一名 OIer 的完整竞赛生涯

基于 Dear ImGui 的图形界面版本。原项目 [Little09qwq/oi-remake-game](https://github.com/Little09qwq/oi-remake-game)

## 功能特性

- 📚 **115 道题目**：覆盖 CSP 到 NOI 全级别难度
- 🏆 **完整赛季**：CSP-S → NOIP → WC → 省选 → APIO → NOI → CTT → CTS → IOI
- 🎲 **随机事件**：训练、比赛、焦虑、遗忘、决心商店...
- 🛒 **决心商店**：用决心购买能力提升
- 🎯 **多难度模式**：简单/普通/困难/专家
- 🖼️ **图形界面**：使用 Dear ImGui

## 编译方法

### 前置要求

- Visual Studio 2019 或更高版本（需要 C++ 开发工具）
- Windows SDK
- DirectX 11（Windows 自带）

### 方法一：使用 build.bat（推荐）

1. 双击运行 `build.bat`
2. 脚本会自动找到 Visual Studio 并编译

### 方法二：使用 build_mingw.bat（MinGW-w64）

1. 确保已安装 [MinGW-w64](https://github.com/niXman/mingw-builds-binaries/releases)（或通过 MSYS2：`pacman -S mingw-w64-x86_64-gcc`）
2. 双击运行 `build_mingw.bat`

### 方法三：使用 CMake

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 运行

> 💡 **推荐直接下载**：前往 [Releases](https://github.com/Andy-Xie-1145/OI-Remake-Simulator/releases/latest) 下载最新版本的 exe，开箱即用。

如需手动编译，编译成功后运行 `output/oi_simulator_gui.exe`。

## 技术栈

- **GUI 框架**: [Dear ImGui v1.92.6](https://github.com/ocornut/imgui)
- **渲染后端**: DirectX 11
- **平台后端**: Win32

## 项目结构

| 文件 | 说明 |
|------|------|
| `main.cpp` | 主程序入口 |
| `types.hpp` | 类型定义、常量、配置 |
| `problem_pool.hpp` | 题目池 (115 题) |
| `events.hpp` | 事件系统 |
| `training_events_data.hpp` | 训练事件数据 |
| `game.hpp` | 游戏逻辑 |
| `imgui/` | ImGui 库 |
| `build.bat` | 编译脚本 |
| `CMakeLists.txt` | CMake 构建配置 |

## 许可证

本项目基于 [Little09qwq/oi-remake-game](https://github.com/Little09qwq/oi-remake-game) 复刻开发。

原项目采用 **MIT 许可证**，版权归属 Little09qwq。

本项目采用 **Apache-2.0 许可证**。

---

> ⚠️ 本项目由 AI 辅助生成，欢迎提 Issue 和 PR！
