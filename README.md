# OI 重开模拟器 GUI 版本

> 🎮 人生重开模拟器的 OI 竞赛版本 —— 体验一名 OIer 的完整竞赛生涯

**版本：v0.1.0-gui-beta**

基于 Dear ImGui 的图形界面版本。

## 功能特性

- 📚 **115 道题目**：覆盖 CSP 到 NOI 全级别难度
- 🏆 **完整赛季**：CSP-S → NOIP → WC → 省选 → APIO → NOI → CTT → CTS → IOI
- 🎲 **随机事件**：训练、比赛、焦虑、遗忘、决心商店...
- 🛒 **决心商店**：用决心购买能力提升
- 🎯 **多难度模式**：简单/普通/困难/专家
- 🖼️ **图形界面**：使用 Dear ImGui

## 编译方法

详见 [BUILD.md](BUILD.md)

### 快速编译 (Windows + Visual Studio)

1. 打开 "x64 Native Tools Command Prompt for VS 2022"
2. 进入项目目录
3. 运行 `compile.bat`

## 运行

编译成功后，运行 `output/oi_simulator_gui.exe`

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

## 许可证

本项目基于 [Little09qwq/oi-remake-game](https://github.com/Little09qwq/oi-remake-game) 复刻开发。

原项目采用 **MIT 许可证**，版权归属 Little09qwq。

本项目采用 **Apache-2.0 许可证**。

---

⚠️ 本项目由 AI 辅助生成，欢迎提 Issue 和 PR！
