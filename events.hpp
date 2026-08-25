#ifndef EVENTS_HPP
#define EVENTS_HPP

#include "types.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

extern GameState gameState;

inline void logEvent(const std::string& message, const std::string& type);

struct EventOption {
    std::string text;
    std::map<std::string, int> effects;
    std::vector<std::string> randomChoices;
    std::string description;
    int cost = 0;
};

inline void clearShopState() {
    gameState.purchasedItems.clear();
}

// 受 [0, 20] 边界约束的属性成员指针映射（mood/health/money 走下方特例）
inline const std::map<std::string, int PlayerStats::*> STAT_MEMBER_MAP = {
    {"dp", &PlayerStats::dp},
    {"ds", &PlayerStats::ds},
    {"string", &PlayerStats::string},
    {"graph", &PlayerStats::graph},
    {"combinatorics", &PlayerStats::combinatorics},
    {"math", &PlayerStats::math},
    {"geometry", &PlayerStats::geometry},
    {"data_structure", &PlayerStats::data_structure},
    {"adhoc", &PlayerStats::adhoc},
    {"thinking", &PlayerStats::thinking},
    {"coding", &PlayerStats::coding},
    {"carefulness", &PlayerStats::carefulness},
    {"quickness", &PlayerStats::quickness},
    {"mental", &PlayerStats::mental},
    {"experience", &PlayerStats::experience},
    {"culture", &PlayerStats::culture},
    {"luck", &PlayerStats::luck},
};

// ========== 属性变更唯一入口 ==========
// 所有数值增减必须经过这里；边界由此统一裁决：
//   mood   -> [0, moodCap]  （心态上限的唯一真相，随背景变化）
//   health -> [0, 20]
//   money  -> >= 0
//   其余   -> [0, 20]
inline void applyStatDelta(const std::string& key, int value, const std::string& optionText) {
    (void)optionText;

    if (key == "mood") {
        gameState.mood = std::max(0, std::min(gameState.moodCap, gameState.mood + value));
        return;
    }

    if (key == "health") {
        gameState.health = std::max(0, std::min(20, gameState.health + value));
        return;
    }

    if (key == "money") {
        gameState.money = std::max(0, gameState.money + value);
        return;
    }

    auto it = STAT_MEMBER_MAP.find(key);
    if (it == STAT_MEMBER_MAP.end()) {
        logEvent("警告：未知属性键 \"" + key + "\"，效果未生效", "event");
        return;
    }
    int& target = gameState.playerStats.*(it->second);
    target = std::max(0, std::min(20, target + value));
}

inline void applySelectedOptionEffects(const EventOption& option) {
    for (const auto& [key, value] : option.effects) {
        applyStatDelta(key, value, option.text);
    }

    if (!option.randomChoices.empty()) {
        const auto& stat = option.randomChoices[Utils::randomInt(0, static_cast<int>(option.randomChoices.size()) - 1)];
        applyStatDelta(stat, 1, option.text);
        logEvent("随机提升：" + Utils::getStatName(stat) + "+1", "event");
    }
}

inline std::vector<EventOption> buildShopOptions() {
    return {
        {"思维提升", {{"thinking", 1}}, {}, "花费" + std::to_string(gameState.currentShopPrices["思维提升"]) + "元提升1点思维能力", gameState.currentShopPrices["思维提升"]},
        {"代码提升", {{"coding", 1}}, {}, "花费" + std::to_string(gameState.currentShopPrices["代码提升"]) + "元提升1点代码能力", gameState.currentShopPrices["代码提升"]},
        {"细心提升", {{"carefulness", 1}}, {}, "花费" + std::to_string(gameState.currentShopPrices["细心提升"]) + "元提升1点细心", gameState.currentShopPrices["细心提升"]},
        {"随机提升", {}, {"dp", "ds", "string", "graph", "combinatorics"}, "花费" + std::to_string(gameState.currentShopPrices["随机提升"]) + "元随机提升一项算法能力", gameState.currentShopPrices["随机提升"]},
        {"心态恢复", {{"mood", 2}}, {}, "花费" + std::to_string(gameState.currentShopPrices["心态恢复"]) + "元提升2点心态", gameState.currentShopPrices["心态恢复"]},
        {"全面提升", {{"dp", 1}, {"ds", 1}, {"string", 1}, {"graph", 1}, {"combinatorics", 1}}, {}, "花费" + std::to_string(gameState.currentShopPrices["全面提升"]) + "元提升所有算法能力", gameState.currentShopPrices["全面提升"]},
        {"速度提升", {{"quickness", 1}}, {}, "花费" + std::to_string(gameState.currentShopPrices["速度提升"]) + "元提升1点迅捷", gameState.currentShopPrices["速度提升"]},
        {"心理素质提升", {{"mental", 1}}, {}, "花费" + std::to_string(gameState.currentShopPrices["心理素质提升"]) + "元提升1点心理素质", gameState.currentShopPrices["心理素质提升"]},
        {"经验提升", {{"experience", 1}}, {}, "花费" + std::to_string(gameState.currentShopPrices["经验提升"]) + "元提升1点经验", gameState.currentShopPrices["经验提升"]},
        {"运气提升", {{"luck", 2}}, {}, "花费" + std::to_string(gameState.currentShopPrices["运气提升"]) + "元提升2点运气，减少负面事件", gameState.currentShopPrices["运气提升"]},
        {"放弃购买", {}, {}, "离开商店", 0}
    };
}

#endif // EVENTS_HPP
