# 移除 v0.1.5 线性剧情的训练事件机制

v0.2.0 转向月回合制后，旧的线性剧情机器（TRAINING_EVENTS 数据表、
getTrainingEventType 的 phase 序列机、getAvailableOptions / rollNextEvent、
GameState.currentPhase）已经没有任何调用者，但代码保留了两年。2026-08 架构清理时
全部物理删除，**不打算恢复**：如果未来要加随机小事件，应作为月度引擎的活动/阶段
扩展重新设计，而不是复活 phase 序列机。

同时删除：决心（determination，v2 已被金钱取代）、题目随机名生成器
generateProblemName、从未生效的「考场杀手 / 钢铁意志」空壳定义（二者已分别接入
Contest::start 与 getStudyEfficiency 真正生效）。
