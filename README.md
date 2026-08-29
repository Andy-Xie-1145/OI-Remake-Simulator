# OI 重开模拟器 v0.4.0-beta

> 人生重开模拟器的 OI 竞赛版本 —— 36 月回合制，体验一名 OIer 的完整高中生涯

基于 Dear ImGui 的图形界面版本。原项目 [Little09qwq/oi-remake-game](https://github.com/Little09qwq/oi-remake-game)

月回合制的设计灵感来自[这篇洛谷讨论](https://www.luogu.com.cn/article/md0iry4t)（[保存站链接](https://www.luogu.me/article/md0iry4t)）。

## 功能特性

- **36 月回合制**：3 年高中生活，每月自由分配行动力（AP）
- **9 维知识体系**：DP / 数据结构 / 字符串 / 图论 / 组合计数 / 数学 / 几何 / 高级数据结构 / 构造
- **完整赛季**：CSP-S → NOIP → WC → 省选 → APIO → NOI → CTT → CTS → IOI
- **文化课考试**：期中 / 期末 / 高考独立系统，跨年文化效率加权
- **背景与特质**：7 种开局背景 + 8 种可获取特质
- **焦虑与遗忘**：心态持续低迷触发焦虑，长期不学则知识遗忘
- **金钱商店**：月收入 + 比赛奖金，价格递增
- **暑假集训**：7-8 月高强度训练选项
- **115 道题目**：覆盖入门到 NOI+ 全级别
- **4 档难度**：简单 / 普通 / 困难 / 专家
- **存档系统**（v0.3.0）：月翻页自动存档，主菜单一键继续
- **专题任务**（v0.3.0）：11 个专题，完成得永久「精通」加成
- **生活节奏**（v0.3.0）：体育锻炼、熬夜冲刺、病倒静养
- **保送与结局矩阵**（v0.3.0）：集训队锁定保送；OI 高度 × 高考档位 = 8 种结局称号 + 三年大事记
- **机房伙伴与宿敌**（v0.4.0）：3 位随机伙伴（姓名/专精/效果随机），闲聊养关系解锁挚友效果与知己倾诉；教练关系解锁特训；宿敌每场正式比赛对比播报，知耻后勇机制
- **结局后继承**：规划中——见 [功能设计评审](docs/v0.3.1-changelog.md)

> 详细更新说明见 [`docs/v0.4.0-changelog.md`](docs/v0.4.0-changelog.md)

## 获取源码

本项目使用 [Git 子模块](https://git-scm.com/book/en/v2/Git-Tools-Submodules) 管理 Dear ImGui 依赖，克隆时**必须递归初始化子模块**：

```bash
git clone --recurse-submodules https://github.com/Andy-Xie-1145/OI-Remake-Simulator.git
```

如果已经克隆但忘记加 `--recurse-submodules`，执行以下命令补全：

```bash
git submodule update --init --recursive
```

> 未初始化子模块会导致编译失败（找不到 `imgui/imgui.h`）。

## 编译方法

### 前置要求

- Windows 系统
- DirectX 11（Windows 自带）
- C++17 编译器

### 方法一：使用 build.bat（Visual Studio，推荐）

1. 双击运行 `build.bat`
2. 脚本会自动找到 Visual Studio 并编译

### 方法二：使用 build_mingw.bat（MinGW-w64）

1. 确保已安装 [MinGW-w64](https://github.com/niXman/mingw-builds-binaries/releases)（或通过 MSYS2：`pacman -S mingw-w64-x86_64-gcc`）
2. 双击运行 `build_mingw.bat`

### 方法三：使用 CMake

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## 运行

> 前往 [Releases](https://github.com/Andy-Xie-1145/OI-Remake-Simulator/releases/latest) 下载最新版本的 exe，开箱即用。

如需手动编译，编译成功后运行 `output/oi_simulator_gui.exe`。

## 项目结构

| 文件 | 说明 |
|------|------|
| `main.cpp` | 主程序入口、UI 渲染（纯视图） |
| `month_engine.hpp` | 月度引擎（回合流程、活动成本、商店交易的唯一规则归属地） |
| `save_system.hpp` | 存档系统（key-value 序列化，自动/手动存读档） |
| `topics.hpp` | 专题任务系统（进度 / 精通 / 逾期） |
| `ending.hpp` | 结局矩阵（OI 高度 × 高考档位 → 结局称号与结语） |
| `types.hpp` | 类型定义、常量、难度配置 |
| `game.hpp` | 游戏逻辑（月度结算、焦虑、遗忘、经验） |
| `story.hpp` | 月历引擎（36 月时间线、比赛/考试安排） |
| `activities.hpp` | 月度活动定义与执行 |
| `talents.hpp` | 背景系统 + 特质获取 |
| `culture_exam.hpp` | 文化课考试系统 |
| `contest.hpp` | 比赛逻辑（思考/写代码/对拍/评奖） |
| `events.hpp` | 属性变更唯一入口 + 商店 |
| `problem_pool.hpp` | 题目池 (115 题) |
| `imgui/` | Dear ImGui（Git 子模块） |

自动化测试位于 [`tests/`](tests/)（Catch2，含引擎级无头回归测试），运行 `tests/build_test_mingw.bat` 一键编译执行。领域术语见 [`CONTEXT.md`](CONTEXT.md)，架构决策见 [`docs/adr/`](docs/adr/)。

## 技术栈

- **GUI 框架**: [Dear ImGui v1.92.7](https://github.com/ocornut/imgui)
- **渲染后端**: DirectX 11
- **平台后端**: Win32
- **语言标准**: C++17

## 许可证

本项目基于 [Little09qwq/oi-remake-game](https://github.com/Little09qwq/oi-remake-game) 复刻开发，并在其基础上持续维护和扩展新内容。

原项目采用 **MIT 许可证**，版权归属 Little09qwq。

本项目采用 **Apache-2.0 许可证**。
