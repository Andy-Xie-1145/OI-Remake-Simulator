#ifndef EVENTS_HPP
#define EVENTS_HPP

#include "types.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

extern PlayerStats playerStats;
extern int mood;
extern std::map<std::string, int> currentShopPrices;

inline void logEvent(const std::string& message, const std::string& type);

struct ProbabilityEffect {
    double probability = 0.0;
    std::map<std::string, int> effects;
};

struct EventOption {
    std::string text;
    std::map<std::string, int> effects;
    std::vector<std::string> randomChoices;
    std::string nextEvent;
    std::map<std::string, double> nextEventProbability;
    std::vector<ProbabilityEffect> probabilityEffects;
    std::string description;
    int cost = 0;
};

struct TrainingEvent {
    std::string title;
    std::string description;
    std::vector<EventOption> options;
    int optionsToShow = 1;
    bool isShop = false;
};

#include "training_events_data.hpp"

inline std::set<std::string> purchasedItems;

inline void clearShopState() {
    purchasedItems.clear();
}

// applyStatDelta 中受 bounds [0, 20] 限制的属性成员指针映射
inline const std::map<std::string, int PlayerStats::*> STAT_MEMBER_MAP = {
    {"dp", &PlayerStats::dp},
    {"ds", &PlayerStats::ds},
    {"string", &PlayerStats::string},
    {"graph", &PlayerStats::graph},
    {"combinatorics", &PlayerStats::combinatorics},
    {"thinking", &PlayerStats::thinking},
    {"coding", &PlayerStats::coding},
    {"carefulness", &PlayerStats::carefulness},
    {"quickness", &PlayerStats::quickness},
    {"mental", &PlayerStats::mental},
    {"experience", &PlayerStats::experience},
    {"culture", &PlayerStats::culture},
    {"luck", &PlayerStats::luck},
};

inline void applyStatDelta(const std::string& key, int value, const std::string& optionText) {
    // mood 特殊处理
    if (key == "mood") {
        if (optionText == "缓和心态") {
            mood = std::max(0, std::min(MOOD_LIMIT, value));
        } else {
            mood = std::max(0, std::min(MOOD_LIMIT, mood + value));
        }
        return;
    }

    // determination 特殊处理（不受 bounds 限制）
    if (key == "determination") {
        playerStats.determination = std::max(0, playerStats.determination + value);
        return;
    }

    // 其余属性通过成员指针映射查找
    auto it = STAT_MEMBER_MAP.find(key);
    if (it != STAT_MEMBER_MAP.end()) {
        int& target = playerStats.*(it->second);
        target = std::max(0, std::min(20, target + value));
    }
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
        {"思维提升", {{"thinking", 1}}, {}, "", {}, {}, "花费" + std::to_string(currentShopPrices["思维提升"]) + "点决心提升1点思维能力", currentShopPrices["思维提升"]},
        {"代码提升", {{"coding", 1}}, {}, "", {}, {}, "花费" + std::to_string(currentShopPrices["代码提升"]) + "点决心提升1点代码能力", currentShopPrices["代码提升"]},
        {"细心提升", {{"carefulness", 1}}, {}, "", {}, {}, "花费" + std::to_string(currentShopPrices["细心提升"]) + "点决心提升1点细心", currentShopPrices["细心提升"]},
        {"随机提升", {}, {"dp", "ds", "string", "graph", "combinatorics"}, "", {}, {}, "花费" + std::to_string(currentShopPrices["随机提升"]) + "点决心随机提升一项算法能力", currentShopPrices["随机提升"]},
        {"心态恢复", {{"mood", 2}}, {}, "", {}, {}, "花费" + std::to_string(currentShopPrices["心态恢复"]) + "点决心提升2点心态", currentShopPrices["心态恢复"]},
        {"全面提升", {{"dp", 1}, {"ds", 1}, {"string", 1}, {"graph", 1}, {"combinatorics", 1}}, {}, "", {}, {}, "花费" + std::to_string(currentShopPrices["全面提升"]) + "点决心提升所有算法能力", currentShopPrices["全面提升"]},
        {"速度提升", {{"quickness", 1}}, {}, "", {}, {}, "花费" + std::to_string(currentShopPrices["速度提升"]) + "点决心提升1点迅捷", currentShopPrices["速度提升"]},
        {"心理素质提升", {{"mental", 1}}, {}, "", {}, {}, "花费" + std::to_string(currentShopPrices["心理素质提升"]) + "点决心提升1点心理素质", currentShopPrices["心理素质提升"]},
        {"经验提升", {{"experience", 1}}, {}, "", {}, {}, "花费" + std::to_string(currentShopPrices["经验提升"]) + "点决心提升1点经验", currentShopPrices["经验提升"]},
        {"运气提升", {{"luck", 2}}, {}, "", {}, {}, "花费" + std::to_string(currentShopPrices["运气提升"]) + "点决心提升2点运气，减少负面事件", currentShopPrices["运气提升"]},
        {"放弃购买", {}, {}, "", {}, {}, "离开商店", 0}
    };
}

inline std::vector<EventOption> getAvailableOptions(const TrainingEvent& event) {
    std::vector<EventOption> options = event.isShop ? buildShopOptions() : event.options;

    if (event.isShop) {
        std::vector<EventOption> shopOptions;
        std::vector<EventOption> available;
        EventOption leaveOption;
        bool hasLeaveOption = false;

        for (const auto& option : options) {
            if (option.text == "放弃购买") {
                leaveOption = option;
                hasLeaveOption = true;
            } else if (purchasedItems.find(option.text) == purchasedItems.end()) {
                shopOptions.push_back(option);
            }
        }

        std::shuffle(shopOptions.begin(), shopOptions.end(), gen);
        const int shopSlots = std::max(0, event.optionsToShow - 1);
        if (static_cast<int>(shopOptions.size()) > shopSlots) {
            shopOptions.resize(shopSlots);
        }

        available = shopOptions;
        if (hasLeaveOption) {
            available.push_back(leaveOption);
        }
        return available;
    }

    std::shuffle(options.begin(), options.end(), gen);
    if (static_cast<int>(options.size()) > event.optionsToShow) {
        options.resize(event.optionsToShow);
    }
    return options;
}

inline std::string rollNextEvent(const std::map<std::string, double>& probabilities) {
    const double roll = Utils::randomDouble(0.0, 1.0);
    double accumulated = 0.0;
    for (const auto& [eventName, probability] : probabilities) {
        accumulated += probability;
        if (roll < accumulated) {
            return eventName;
        }
    }
    return "";
}

#endif // EVENTS_HPP
