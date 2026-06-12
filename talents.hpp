#ifndef TALENTS_HPP
#define TALENTS_HPP

#include "game.hpp"
#include <string>
#include <vector>

namespace Talent {

struct Background {
    std::string id;
    const char* name;
    const char* desc;
    std::map<std::string, int> mods;  // 初始属性加成
    int moneyMult = 100;      // 金钱倍率（百分比，100=正常）
    int moodCap = 12;          // 心态上限覆盖
    int moodBonus = 0;         // 每月心态额外加成
    int incomeBonus = 0;       // 每月收入额外加成（百分比）
    int apBonus = 0;           // 每月额外 AP
    double efficiencyMult = 1.0; // 学习效率乘数
    double anxietyMult = 1.0;    // 焦虑概率乘数
};

inline const std::vector<Background> BACKGROUNDS = {
    {"rich", "家境优越",
     "起始金钱x2，文化考差心态多掉1",
     {{"culture", 1}}, 200, 12, 0, 0, 0, 1.0, 1.0},
    {"diligent", "勤奋刻苦",
     "文化课+3，每月心态+1",
     {{"culture", 3}}, 100, 12, 1, 0, 0, 1.0, 1.0},
    {"talented", "天赋型选手",
     "思维+2、代码+1，心态上限10",
     {{"thinking", 2}, {"coding", 1}}, 100, 10, 0, 0, 0, 1.0, 1.0},
    {"veteran", "竞赛老手",
     "经验+2、自选一科+3",
     {{"experience", 2}}, 100, 12, 0, 0, 0, 1.0, 1.0},
    {"scholar", "书香门第",
     "文化+4、思维+1，起始金钱减半",
     {{"culture", 4}, {"thinking", 1}}, 50, 12, 0, 0, 0, 1.0, 1.0},
    {"underdog", "草根逆袭",
     "无属性加成，起始多3 AP，月收入+20%",
     {}, 100, 12, 0, 20, 3, 1.0, 1.0},
    {"zen", "佛系心态",
     "心态上限14，焦虑概率减半，效率x0.9",
     {}, 100, 14, 0, 0, 0, 0.9, 0.5},
};

// ========== 可获取特质（从模拟赛/刷题中获得） ==========

struct TraitDef {
    std::string id;
    const char* name;
    const char* desc;
    // 特质效果通过 id 在游戏逻辑中判断
};

inline const std::vector<TraitDef> ACQUIRABLE_TRAITS = {
    {"code_master",    "代码小能手",  "代码能力 +1 永久加成"},
    {"algo_talent",    "算法达人",    "学习效率 +10%"},
    {"steady_heart",   "心态稳定",    "焦虑概率减半"},
    {"quick_learner",  "快速学习",    "知识维度 +1 随机加成"},
    {"contest_beast",  "考场杀手",    "比赛心态降幅 -1"},
    {"lucky_star",     "幸运星",      "运气 +2"},
    {"iron_will",      "钢铁意志",    "焦虑时效率不降"},
    {"deep_thinker",   "深度思考",    "思维 +1 永久加成"},
};

inline const Background* getBackground() {
    for (const auto& bg : BACKGROUNDS) {
        if (bg.id == gameState.background) return &bg;
    }
    return nullptr;
}

inline void applyBackground(const std::string& bgId) {
    for (const auto& bg : BACKGROUNDS) {
        if (bg.id == bgId) {
            gameState.background = bgId;
            // 应用属性加成
            for (const auto& [key, val] : bg.mods) {
                applyStatDelta(key, val, "背景");
            }
            // 应用金钱倍率
            gameState.money = gameState.money * bg.moneyMult / 100;
            // 应用 AP 加成
            gameState.ap += bg.apBonus;
            gameState.maxAp += bg.apBonus;
            // 设置全局乘数（供 game.hpp 使用）
            gameState.moodCap = bg.moodCap;
            gameState.efficiencyMultiplier = bg.efficiencyMult;
            gameState.anxietyMultiplier = bg.anxietyMult;
            return;
        }
    }
}

// 尝试获取特质（从模拟赛/刷题中调用）
// baseProb: 基础概率（模拟赛15%, 刷题30%）
inline bool tryAcquireTrait(double baseProb) {
    // 已有特质数
    int ownedCount = static_cast<int>(gameState.traits.size());
    // 最多拥有 4 个特质
    if (ownedCount >= 4) return false;

    // 概率随已有特质递减
    double prob = baseProb * (1.0 - ownedCount * 0.2);
    if (!Utils::randomBool(prob)) return false;

    // 找一个还没有的特质
    std::vector<int> available;
    for (int i = 0; i < static_cast<int>(ACQUIRABLE_TRAITS.size()); ++i) {
        bool has = false;
        for (const auto& t : gameState.traits) {
            if (t == ACQUIRABLE_TRAITS[i].id) { has = true; break; }
        }
        if (!has) available.push_back(i);
    }
    if (available.empty()) return false;

    int pick = available[Utils::randomInt(0, static_cast<int>(available.size()) - 1)];
    const auto& trait = ACQUIRABLE_TRAITS[pick];
    gameState.traits.push_back(trait.id);

    // 应用特质效果
    if (trait.id == "code_master") {
        applyStatDelta("coding", 1, "特质：代码小能手");
    } else if (trait.id == "lucky_star") {
        applyStatDelta("luck", 2, "特质：幸运星");
    } else if (trait.id == "deep_thinker") {
        applyStatDelta("thinking", 1, "特质：深度思考");
    } else if (trait.id == "steady_heart") {
        gameState.anxietyMultiplier *= 0.5;
    } else if (trait.id == "algo_talent") {
        gameState.efficiencyMultiplier *= 1.1;
    } else if (trait.id == "quick_learner") {
        // 随机 +1 知识维度
        int dim = Utils::randomInt(0, static_cast<int>(KNOWLEDGE_DIMS.size()) - 1);
        applyStatDelta(KNOWLEDGE_DIMS[dim], 1, "特质：快速学习");
    }
    // contest_beast 和 iron_will 在比赛/焦虑逻辑中检查

    logEvent("获得特质：" + std::string(trait.name) + " — " + trait.desc, "event");
    return true;
}

// 查询是否拥有某特质
inline bool hasTrait(const std::string& traitId) {
    for (const auto& t : gameState.traits) {
        if (t == traitId) return true;
    }
    return false;
}

} // namespace Talent

#endif // TALENTS_HPP
