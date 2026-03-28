# OI 重开模拟器 v0.0.3-beta

> 🎮 人生重开模拟器的 OI 竞赛版本 —— 体验一名 OIer 的完整竞赛生涯

C++ 复刻版，原项目 [Little09qwq/oi-remake-game](https://github.com/Little09qwq/oi-remake-game)

## 快速开始

```bash
g++ -o output/main main.cpp -std=c++17 -O2
./output/main
```

## 游戏特色

- 📚 **115 道题目**：覆盖 CSP 到 NOI 全级别难度
- 🏆 **完整赛季**：CSP-S → NOIP → WC → 省选 → APIO → NOI → CTT → CTS → IOI
- 🎲 **随机事件**：训练、比赛、焦虑、遗忘、决心商店...
- 🛒 **决心商店**：用"决心"购买能力提升
- 🎯 **多难度模式**：简单/普通/困难/专家

## 代码结构

| 文件 | 说明 |
|------|------|
| `main.cpp` | 程序入口 |
| `types.hpp` | 类型定义、常量、配置 |
| `problem_pool.hpp` | 题目池 (115 题) |
| `events.hpp` | 事件系统 |
| `training_events_data.hpp` | 训练事件数据 |
| `game.hpp` | 比赛系统、游戏流程 |

## 核心机制

### 属性
- **知识点**：动态规划、数据结构、字符串、图论、组合计数
- **能力值**：思维、代码、细心、迅捷、心理素质
- **心态**：影响操作成功率

### 比赛流程
1. 分配初始天赋点
2. 参加训练提升能力
3. 在比赛中合理分配时间
4. 思考 → 写代码 → 对拍/提交
5. 获得成绩和奖项

## 操作

- `数字[a/b/c]` - 选择题目并执行操作（思考/写代码/对拍）
- `p/n` - 上一题/下一题
- `0` - 提前离场（满分时）

## 许可证

本项目基于 [Little09qwq/oi-remake-game](https://github.com/Little09qwq/oi-remake-game) 复刻开发。

原项目采用 **MIT 许可证**，版权归属 Little09qwq。

本项目采用 **Apache-2.0 许可证**。

---

> ⚠️ 声明：所有代码由 OpenClaw 代理生成，使用 GLM-5，可能有误。欢迎反馈各种 Bug，欢迎提 Issue 和 PR！
