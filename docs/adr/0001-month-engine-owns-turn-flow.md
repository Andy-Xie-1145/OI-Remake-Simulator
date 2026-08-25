# 月度引擎拥有回合流程，UI 是纯视图

2026-08 架构重构时，月回合的 sequencing 规则（活动 → 比赛 → 考试 → 结算 → 下月）
原本散布在 `main.cpp` 的 GuiApp 流程方法（EndMonthAction / ContinueAfterContest /
ContinueAfterSettlement）和渲染函数里：按钮各自硬编码 AP/健康扣费、停课在 UI 重算
公式、考试月在渲染代码里二次结算（收入翻倍的真 bug）。我们决定把这些规则全部收进
`month_engine.hpp` 的 Engine 命名空间——UI 只发指令（doActivity / endMonthActions /
phaseFinished…）并按引擎给出的 Phase 队列切换屏幕。

选这个方向而非「把规则函数下沉、保留 UI 编排」的浅方案，是因为只有引擎持有完整
流程，无头测试才能不启动 DX11 就驱动 36 个月；ImGui GUI 与 Catch2 测试驱动由此
成为同一 interface 的两个 adapter。代价是新增一层指令式接口，简单改动多经过一次
跳转。

## 后果

- 每月结算恰好发生在 `endMonthActions` 一次；考试收尾只记录成绩。
- 活动的 AP/健康成本唯一真相是 `activities.hpp` 的 ACTIVITY_DEFS 表，UI 展示与
  引擎扣费读同一处。
- 特质掉落概率按活动类型区分：网赛/自定义 15%，刷题 30%。
