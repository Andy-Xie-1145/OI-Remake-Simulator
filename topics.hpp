// ============================================================================
//  topics.hpp  —  专题任务系统（D）
//
//  规则归属地：接受 / 进度判定 / 完成精通 / 逾期失败。
//  层级：位于 game/events 之上、engine/contest 之下（contest 可直接调用
//  onCheckSuccess 记录对拍进度，不产生环包含）。
//
//  平衡锚点：3 个月内 4 次匹配行动 ≈ 8 AP 投入；精通收益为该维度子问题
//  思考时间 -1（下限 1），能力专题为单项公式小加成。
// ============================================================================
#ifndef TOPICS_HPP
#define TOPICS_HPP

#include "game.hpp"

#include <set>
#include <string>
#include <vector>

namespace Topics {

constexpr int INVALID = -1;
constexpr int GOAL = 4;          // 需完成的匹配行动次数
constexpr int MONTHS_LIMIT = 3;  // 接受后的期限（月）

enum class Kind { Knowledge, Speed, Careful };

struct TopicDef {
    int id;
    Kind kind;
    int dimIndex;        // Knowledge：KNOWLEDGE_DIMS 下标；其他为 -1
    const char* name;
    const char* desc;
};

inline const std::vector<TopicDef> ALL = {
    {0,  Kind::Knowledge, 0, "DP 专题",      "专攻动态规划"},
    {1,  Kind::Knowledge, 1, "数据结构专题", "专攻数据结构"},
    {2,  Kind::Knowledge, 2, "字符串专题",   "专攻字符串"},
    {3,  Kind::Knowledge, 3, "图论专题",     "专攻图论"},
    {4,  Kind::Knowledge, 4, "组合计数专题", "专攻组合计数"},
    {5,  Kind::Knowledge, 5, "数学专题",     "专攻数学"},
    {6,  Kind::Knowledge, 6, "几何专题",     "专攻计算几何"},
    {7,  Kind::Knowledge, 7, "高级数据结构专题", "专攻高级数据结构"},
    {8,  Kind::Knowledge, 8, "构造专题",     "专攻构造/思维"},
    {9,  Kind::Speed,    -1, "手速特训",     "接受后完成 4 场网赛/刷题/模拟赛"},
    {10, Kind::Careful,  -1, "细心打磨",     "接受后累计 6 次成功对拍/提交"},
};
constexpr int CAREFUL_GOAL = 6;

inline const TopicDef* byId(int id) {
    for (const auto& t : ALL)
        if (t.id == id) return &t;
    return nullptr;
}

inline bool hasMastery(int id) {
    return gameState.masteredTopics.count(id) > 0;
}

inline const TopicDef* active() {
    return gameState.topicId != INVALID ? byId(gameState.topicId) : nullptr;
}

// 接受专题；已精通或已有进行中专题时拒绝（返回 false）
// 进度一律从 0 开始计数——只有接受之后的匹配行动才计入（不回溯历史行为）
inline bool accept(int id) {
    const TopicDef* d = byId(id);
    if (!d || active() || hasMastery(id)) return false;
    gameState.topicId = id;
    gameState.topicStartMonth = gameState.currentMonth;
    gameState.topicProgress = 0;
    logEvent(std::string("接受专题任务：") + d->name +
             "（" + std::to_string(MONTHS_LIMIT) + " 个月内完成 " +
             std::to_string(d->kind == Kind::Careful ? CAREFUL_GOAL : GOAL) +
             " 次，进度从现在起计）", "event");
    return true;
}

// 精通奖励说明（供 UI 提示与完成通知使用）
inline const char* RewardText(const TopicDef& d) {
    switch (d.kind) {
    case Kind::Knowledge: return "永久精通：比赛中该维度子问题的思考时间 -1";
    case Kind::Speed:     return "永久精通：写代码阶段有效迅捷 +1";
    case Kind::Careful:   return "永久精通：对拍/提交错误率 ×0.9";
    }
    return "";
}

inline void abandon() {
    if (gameState.topicId == INVALID) return;
    const TopicDef* d = byId(gameState.topicId);
    logEvent(std::string("放弃专题：") + (d ? d->name : "?"), "event");
    gameState.topicId = INVALID;
}

// 进度 +1 并检查完成；kind 过滤由调用方保证匹配
inline void advance(const char* reason) {
    const TopicDef* d = active();
    if (!d) return;
    gameState.topicProgress++;
    logEvent(std::string("专题进度 ") + d->name + "：" +
             std::to_string(gameState.topicProgress) + "/" +
             std::to_string(d->kind == Kind::Careful ? CAREFUL_GOAL : GOAL) +
             "（" + reason + "）", "event");

    const int goal = (d->kind == Kind::Careful) ? CAREFUL_GOAL : GOAL;
    if (gameState.topicProgress >= goal) {
        gameState.masteredTopics.insert(d->id);
        gameState.topicId = INVALID;
        logEvent(std::string("专题完成：") + d->name + "！获得永久精通加成。", "event");
    }
}

// 「学习知识」选中维度时
inline void onLearn(int dimIndex) {
    const TopicDef* d = active();
    if (d && d->kind == Kind::Knowledge && d->dimIndex == dimIndex) advance("学习匹配");
}

// 子问题在某知识维度上的要求数值（SubProblem 字段名与 KNOWLEDGE_DIMS 顺序不同构，
// 题目池仅覆盖 dp/ds/str/graph/comb/adhoc 六个维度）
inline int subProblemDimValue(const SubProblem& sp, int dimIndex) {
    switch (dimIndex) {
    case 0: return sp.dp;
    case 1: return sp.ds;
    case 2: return sp.str;
    case 3: return sp.graph;
    case 4: return sp.comb;
    case 8: return sp.adhoc;
    default: return 0;
    }
}

// 一场活动比赛（网赛/刷题/自定义）打完时
inline void onActivityContestFinished() {
    const TopicDef* d = active();
    if (!d) return;
    if (d->kind == Kind::Speed) { advance("完成一场比赛"); return; }
    if (d->kind == Kind::Knowledge) {
        // 本场比赛中主维度为专题维度的题数 ≥2 才计入
        int count = 0;
        for (const auto& parts : gameState.subProblems) {
            const SubProblem* best = nullptr;
            for (const auto& sp : parts)
                if (!best || sp.score > best->score) best = &sp;
            if (best && subProblemDimValue(*best, d->dimIndex) > 0) ++count;
        }
        if (count >= 2) advance("比赛覆盖专题维度");
    }
}

// 一次成功的对拍/提交（Contest::check 成功路径调用）
inline void onCheckSuccess() {
    const TopicDef* d = active();
    if (d && d->kind == Kind::Careful) advance("对拍成功");
}

// 月末结算调用：逾期未完成 → 失败惩罚
inline void onMonthEnd() {
    const TopicDef* d = active();
    if (!d) return;
    const int goal = (d->kind == Kind::Careful) ? CAREFUL_GOAL : GOAL;
    if (gameState.topicProgress >= goal) return;  // 理论上已完成

    if (gameState.currentMonth - gameState.topicStartMonth >= MONTHS_LIMIT) {
        applyStatDelta("mood", -2, "专题逾期");
        logEvent(std::string("专题「") + d->name + "」逾期未完成：心态 -2", "event");
        gameState.settlementFacts.push_back({SettlementFact::Cat::Other,
            std::string("专题「") + d->name + "」逾期未完成，心态 -2"});
        gameState.topicId = INVALID;
    }
}

} // namespace Topics

#endif // TOPICS_HPP
