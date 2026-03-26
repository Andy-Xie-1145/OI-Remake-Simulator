# OI重开模拟器 v2.0

完全复刻自 [Little09qwq/oi-remake-game](https://github.com/Little09qwq/oi-remake-game)

## 编译运行

```bash
cd "/mnt/d/C++ Programs/OI_Simulator"
g++ -o output/main.exe main.cpp -std=c++17 -O2
./output/main.exe
```

## 文件结构

```
main.cpp          (20行)   - 程序入口
types.hpp         (248行)  - 类型定义、常量、商店/难度配置
problem_pool.hpp  (189行)  - 题目池(115道题目)
events.hpp        (346行)  - 事件系统(16种事件)
game.hpp          (654行)  - 比赛系统、游戏流程
output/main.exe            - 可执行文件
```

**总代码量: 1457行**

## 题目池统计

| 难度 | 题目数 |
|------|--------|
| Level 2 | 42 |
| Level 3 | 36 |
| Level 4 | 17 |
| Level 5 | 5 |
| Level 6 | 3 |
| Level 7 | 3 |
| Level 8 | 2 |
| Level 9 | 2 |
| Level 10 | 1 |
| **总计** | **115** |

## 核心数值（完全复制原版）

### 心态系统
- 心态上限: 12, 初始: 10
- 进入考场心态下降: `1 + extraMoodDrop - mental`
- 比赛后自动恢复到 `min(5 + mental, 10)`

### 计算公式
```
思考时间 = 1 + max(0, dp-map) + max(0, ds-map) + ... + adhoc
思考成功率 = 1 - max(0, thinking-map)*0.05 - (max(10-mood,0))^2 * 0.01 [30%-95%]
代码时间 = max(1, coding - quickness)  
代码成功率 = 1 - (max(10-mood,0))^2*0.01 - max(0, detail-map)*0.05 [40%-95%]
出错概率 = 0.1 + trap*0.05 - carefulness*0.03 + (max(10-mood,0))^2*0.01 [0%-80%]
```

### 属性映射
```
0-2→0-2, 3-4→3, 5-6→4, 7-8→5, 9-10→6, 11-12→7, 13-14→8, 15-17→9, 18+→10
```

## 比赛配置

| 比赛 | 时间点 | 题数 | 难度 | IOI赛制 |
|------|--------|------|------|---------|
| CSP-S | 21 | 4 | 2-6 | 否 |
| NOIP | 24 | 4 | 3-6 | 否 |
| WC | 30 | 3 | 3-9 | 否 |
| 省选D1/D2 | 27 | 3 | 4-9 | 否 |
| APIO | 30 | 3 | 5-9 | 是 |
| NOI D1/D2 | 30 | 3 | 5-10 | 否 |

## 操作说明

- `[数字][a/b/c]` - 思考/写代码/对拍或提交
- `p/n` - 上一题/下一题
- `0` - 提前离场(满分时)

> 声明：所有代码由OpenClaw代理生成，使用GLM5，可能有误。欢迎反馈各种Bug，欢迎提Issue和PR！