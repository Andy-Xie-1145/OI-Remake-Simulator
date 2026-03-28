#ifndef EVENTS_HPP
#define EVENTS_HPP

#include "types.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

extern PlayerStats playerStats;
extern std::string gameDifficulty;
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

inline void clearScreen() {
#ifdef _WIN32
    if (system("cls") != 0) {}
#else
    if (system("clear") != 0) {}
#endif
}

inline void clearShopState() {
    purchasedItems.clear();
}

inline void applyStatDelta(const std::string& key, int value, const std::string& optionText) {
    if (key == "mood") {
        if (optionText == "缓和心态") {
            mood = std::max(0, std::min(MOOD_LIMIT, value));
        } else {
            mood = std::max(0, std::min(MOOD_LIMIT, mood + value));
        }
        return;
    }

    if (key == "determination") {
        playerStats.determination = std::max(0, playerStats.determination + value);
        return;
    }

    auto applyBounded = [value](int& target) {
        target = std::max(0, std::min(20, target + value));
    };

    if (key == "dp") applyBounded(playerStats.dp);
    else if (key == "ds") applyBounded(playerStats.ds);
    else if (key == "string") applyBounded(playerStats.string);
    else if (key == "graph") applyBounded(playerStats.graph);
    else if (key == "combinatorics") applyBounded(playerStats.combinatorics);
    else if (key == "thinking") applyBounded(playerStats.thinking);
    else if (key == "coding") applyBounded(playerStats.coding);
    else if (key == "carefulness") applyBounded(playerStats.carefulness);
    else if (key == "quickness") applyBounded(playerStats.quickness);
    else if (key == "mental") applyBounded(playerStats.mental);
    else if (key == "culture") applyBounded(playerStats.culture);
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

inline void displayTrainingEvent(const TrainingEvent& event, const std::vector<EventOption>& options) {
    clearScreen();
    std::cout << "\n┌────────────────────────────────────────┐\n";
    std::cout << "│\t【" << event.title << "】\t\t\t\t│\n";
    std::cout << "└────────────────────────────────────────┘\n";
    std::cout << event.description << "\n\n";

    for (size_t i = 0; i < options.size(); ++i) {
        const auto& option = options[i];
        std::cout << "  " << (i + 1) << ". " << option.text;
        if (!option.description.empty()) {
            std::cout << " (" << option.description << ")";
        }
        std::cout << "\n";
    }

    std::cout << "\n当前心态: " << mood << "/" << MOOD_LIMIT
              << "  当前决心: " << playerStats.determination << "\n";
    std::cout << "请选择: ";
}

inline void runEventChain(std::string startEvent) {
    std::string currentEvent = std::move(startEvent);

    while (!currentEvent.empty()) {
        auto eventIt = TRAINING_EVENTS.find(currentEvent);
        if (eventIt == TRAINING_EVENTS.end()) {
            logEvent("未知事件: " + currentEvent, "event");
            return;
        }

        const auto& event = eventIt->second;
        auto options = getAvailableOptions(event);
        if (options.empty()) {
            logEvent("事件没有可用选项: " + currentEvent, "event");
            return;
        }

        displayTrainingEvent(event, options);
        const int choice = Utils::readIntInRange(1, static_cast<int>(options.size()));
        const auto& selected = options[choice - 1];

        logEvent("选择了：" + selected.text, "event");
        if (!selected.description.empty()) {
            logEvent(selected.description, "event");
        }

        if (event.isShop) {
            if (selected.text == "放弃购买") {
                logEvent("离开商店", "event");
                clearShopState();
                return;
            }

            if (playerStats.determination < selected.cost) {
                logEvent("决心不足！需要" + std::to_string(selected.cost) + "点决心，当前只有" + std::to_string(playerStats.determination) + "点。", "event");
                continue;
            }

            playerStats.determination -= selected.cost;
            logEvent("消耗了" + std::to_string(selected.cost) + "点决心", "event");
            applySelectedOptionEffects(selected);
            purchasedItems.insert(selected.text);

            const auto incIt = SHOP_PRICE_INCREMENTS.find(gameDifficulty);
            if (incIt != SHOP_PRICE_INCREMENTS.end()) {
                auto priceIt = incIt->second.find(selected.text);
                if (priceIt != incIt->second.end()) {
                    currentShopPrices[selected.text] += priceIt->second;
                    logEvent("下次购买" + selected.text + "需要" + std::to_string(currentShopPrices[selected.text]) + "点决心", "event");
                }
            }

            logEvent("当前心态值：" + std::to_string(mood), "event");
            logEvent("当前决心值：" + std::to_string(playerStats.determination), "event");
            continue;
        }

        applySelectedOptionEffects(selected);
        logEvent("当前心态值：" + std::to_string(mood), "event");
        logEvent("当前决心值：" + std::to_string(playerStats.determination), "event");

        if (!selected.nextEvent.empty()) {
            logEvent("触发跳转事件：" + selected.nextEvent, "event");
            currentEvent = selected.nextEvent;
            continue;
        }

        if (!selected.nextEventProbability.empty()) {
            const std::string nextEvent = rollNextEvent(selected.nextEventProbability);
            if (!nextEvent.empty()) {
                logEvent("触发概率跳转事件：" + nextEvent, "event");
                currentEvent = nextEvent;
                continue;
            }
        }

        return;
    }
}

#endif // EVENTS_HPP
