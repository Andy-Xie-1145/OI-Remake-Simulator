#ifndef GAME_HPP
#define GAME_HPP

#include "types.hpp"
#include "problem_pool.hpp"
#include "events.hpp"
#include <cctype>
#include <string>
#include <vector>
#include <algorithm>
#include <set>
#include <functional>
#include <cmath>

// ========== 全局游戏状态 ==========

inline GameState gameState;

// ========== 日志函数 ==========

inline void logEvent(const std::string& message, const std::string& type = "") {
    std::string prefix;
    if (type == "event") prefix = "【事件】";
    else if (type == "think") prefix = "【思考】";
    else if (type == "code") prefix = "【代码】";
    else if (type == "check") prefix = "【对拍】";

    std::string fullMsg = prefix + message;
    gameState.gameLog.push_back(fullMsg);
}

inline double difficultyMultiplier() {
    return DIFFICULTY_SETTINGS.at(gameState.gameDifficulty).scoreMultiplier;
}

inline void pushLastAction(const std::string& action) {
    gameState.lastActions.push_back(action);
    if (gameState.lastActions.size() > 5) gameState.lastActions.erase(gameState.lastActions.begin());
}

inline void clearPendingContestNotice() {
    gameState.pendingContestNotice = PendingContestNotice{};
}

inline bool hasPendingContestNotice() {
    return gameState.pendingContestNotice.active;
}

inline PendingContestNotice consumePendingContestNotice() {
    PendingContestNotice notice = gameState.pendingContestNotice;
    clearPendingContestNotice();
    return notice;
}

// ========== 经验系统 ==========

inline void addExperience(int amount, const std::string& reason) {
    if (amount <= 0) return;

    const int before = gameState.playerStats.experience;
    gameState.playerStats.experience = std::min(20, gameState.playerStats.experience + amount);
    const int gained = gameState.playerStats.experience - before;
    if (gained > 0) {
        logEvent(reason + "，经验+" + std::to_string(gained) + "，当前经验：" + std::to_string(gameState.playerStats.experience), "event");
    }
}

inline void settleTempExperience(const std::string& reason = "经验积累转化") {
    while (gameState.playerStats.tempExperience >= 6 && gameState.playerStats.experience < 20) {
        gameState.playerStats.tempExperience -= 6;
        addExperience(1, reason);
    }
}

inline void addTempExperience(int amount, const std::string& reason) {
    if (amount <= 0) return;

    gameState.playerStats.tempExperience += amount;
    logEvent(reason + "，经验积累+" + std::to_string(amount) + "，当前经验积累：" + std::to_string(gameState.playerStats.tempExperience), "event");
    settleTempExperience("经验积累达到 6 点");
}

inline bool isTopAwardForExperience(const std::string& contestType, const std::string& award) {
    if ((contestType == "CSP-S" || contestType == "NOIP") && award == "一等奖") return true;
    if ((contestType == "WC" || contestType == "APIO" || contestType == "NOI" || contestType == "IOI") && award == "金牌") return true;
    if (contestType == "省选" && award == "省队A队") return true;
    if (contestType == "CTT" && award == "入选候选队") return true;
    if (contestType == "CTS" && award == "入选国家队") return true;
    return false;
}

// 查询玩家是否拥有某特质（特质效果判定统一走这里）
inline bool playerHasTrait(const std::string& traitId) {
    return std::find(gameState.traits.begin(), gameState.traits.end(), traitId) != gameState.traits.end();
}

// ========== 游戏初始化 ==========

inline void initGame() {
    initProblemPool();
    auto settings = DIFFICULTY_SETTINGS.at(gameState.gameDifficulty);
    gameState.playerStats.extraMoodDrop = (gameState.gameDifficulty == "expert") ? 2 : (gameState.gameDifficulty == "easy") ? 0 : 1;
    gameState.currentShopPrices = INITIAL_SHOP_PRICES.at(gameState.gameDifficulty);

    // v2 初始化
    gameState.currentMonth = 1;
    gameState.currentYear = 1;
    gameState.calendarMonth = 7;
    gameState.health = settings.initialHealth;
    gameState.money = settings.initialMoney;
    gameState.ap = 8 + settings.apBonus;
    gameState.maxAp = 8 + settings.apBonus;
    gameState.isTingke = false;
    gameState.cultureEfficiency = 1.0;
    gameState.traits.clear();
    gameState.background.clear();
    gameState.ownedItems.clear();
    gameState.settlementFacts.clear();
    gameState.examRecords.clear();
    gameState.anxietyMonths = 0;
    gameState.isAnxious = false;
    gameState.anxietyMultiplier = 1.0;
    gameState.efficiencyMultiplier = 1.0;
    gameState.moodCap = MOOD_LIMIT;

    // v0.3.0 状态重置（专题 / 生活节奏）——所有单局状态必须在此清零
    gameState.topicId = -1;           // Topics::INVALID
    gameState.topicStartMonth = 0;
    gameState.topicProgress = 0;
    gameState.carefulChecks = 0;
    gameState.masteredTopics.clear();
    gameState.isAoYe = false;
    gameState.sickNext = false;
    gameState.exerciseCountThisMonth = 0;

    // 初始化遗忘追踪
    for (const auto& dim : KNOWLEDGE_DIMS) {
        gameState.lastStudyMonth[dim] = 1;
    }
    gameState.lastStudyMonth["culture"] = 1;
}

// ========== v2 心态效率 ==========
inline double getMoodEfficiency() {
    return std::max(0.5, std::min(1.5, gameState.mood / 6.0));
}

// ========== v2 月度结算 ==========

// 获取最低知识维度值
inline int getMinKnowledge() {
    int minVal = 20;
    for (const auto& dim : KNOWLEDGE_DIMS) {
        auto it = STAT_MEMBER_MAP.find(dim);
        if (it != STAT_MEMBER_MAP.end()) {
            minVal = std::min(minVal, gameState.playerStats.*(it->second));
        }
    }
    return minVal;
}

// 计算月度金钱收入
inline int calculateMonthlyIncome() {
    const auto& settings = DIFFICULTY_SETTINGS.at(gameState.gameDifficulty);
    int income = settings.monthlyIncome;
    // 文化加成
    if (gameState.playerStats.culture >= 12) income = income * 3 / 2;
    else if (gameState.playerStats.culture >= 8) income = income * 6 / 5;
    // OI 成绩加成
    if (gameState.playerStats.isNationalTeam) income += 200;
    else if (gameState.playerStats.isTrainingTeam) income += 100;
    else if (gameState.playerStats.isProvincialTeam) income += 50;
    return income;
}

// 月度结算主函数：每月恰好调用一次（由 MonthEngine 驱动）
inline void settleMonth(bool hasContest) {
    gameState.settlementFacts.clear();
    auto fact = [](SettlementFact::Cat cat, const std::string& text) {
        gameState.settlementFacts.push_back({cat, text});
    };

    // 1. 未用 AP -> 睡觉
    int unusedAp = gameState.ap;
    if (unusedAp > 0) {
        int healthGain = unusedAp;
        int moodGain = unusedAp / 2;
        gameState.health = std::min(20, gameState.health + healthGain);
        applyStatDelta("mood", moodGain, "月末睡觉");
        fact(SettlementFact::Cat::Health,
            "未用 " + std::to_string(unusedAp) +
            " AP -> 睡觉恢复（健康+" + std::to_string(healthGain) +
            ", 心态+" + std::to_string(moodGain) + "）");
    }

    // 2. 健康自然恢复
    int oldHealth = gameState.health;
    gameState.health = std::min(20, gameState.health + 2);
    if (gameState.health > oldHealth) {
        fact(SettlementFact::Cat::Health,
            "健康自然恢复 +" + std::to_string(gameState.health - oldHealth));
    }

    // 3. 月度心态变化
    int moodDelta = 0;

    // 文化影响（保送后文化课压力免除）
    if (gameState.playerStats.culture >= 12) { moodDelta += 1; fact(SettlementFact::Cat::Health, "文化课扎实：心态+1"); }
    else if (gameState.playerStats.culture < 6 && !gameState.playerStats.isBaosong) { moodDelta -= 1; fact(SettlementFact::Cat::Health, "文化课薄弱：心态-1"); }

    // OI 短板影响
    int minKnow = getMinKnowledge();
    if (minKnow >= 8) { moodDelta += 1; fact(SettlementFact::Cat::Health, "OI 各科均衡：心态+1"); }
    else if (minKnow < 4) { moodDelta -= 1; fact(SettlementFact::Cat::Health, "OI 有明显短板：心态-1"); }

    // 比赛月压力
    if (hasContest) { moodDelta -= 1; fact(SettlementFact::Cat::Health, "比赛月压力：心态-1"); }

    applyStatDelta("mood", moodDelta, "月度结算");

    // 4. 遗忘
    int forgetThreshold = 2;
    for (const auto& dim : KNOWLEDGE_DIMS) {
        auto it = gameState.lastStudyMonth.find(dim);
        if (it != gameState.lastStudyMonth.end()) {
            int monthsSince = gameState.currentMonth - it->second;
            if (monthsSince >= forgetThreshold) {
                auto statIt = STAT_MEMBER_MAP.find(dim);
                if (statIt != STAT_MEMBER_MAP.end()) {
                    int& val = gameState.playerStats.*(statIt->second);
                    if (val > 0) {
                        val = std::max(0, val - 1);
                        fact(SettlementFact::Cat::Knowledge,
                            Utils::getStatName(dim) + " 遗忘 -1（" +
                            std::to_string(monthsSince) + "个月未学）");
                    }
                }
            }
        }
    }
    // 文化遗忘（保送后不再遗忘）
    auto cultureIt = gameState.lastStudyMonth.find("culture");
    if (cultureIt != gameState.lastStudyMonth.end() && !gameState.playerStats.isBaosong) {
        int monthsSince = gameState.currentMonth - cultureIt->second;
        if (monthsSince >= forgetThreshold && gameState.playerStats.culture > 0) {
            gameState.playerStats.culture = std::max(0, gameState.playerStats.culture - 1);
            fact(SettlementFact::Cat::Knowledge,
                "文化课遗忘 -1（" + std::to_string(monthsSince) + "个月未学）");
        }
    }

    // 5. 金钱收入
    int income = calculateMonthlyIncome();
    gameState.money += income;
    fact(SettlementFact::Cat::Economy, "零花钱 +" + std::to_string(income));

    // 6. 焦虑检查（熬夜放大焦虑概率）
    const double aoYeMult = gameState.isAoYe ? 1.3 : 1.0;
    if (gameState.mood < 4) {
        gameState.anxietyMonths++;
        if (gameState.anxietyMonths >= 2 && !gameState.isAnxious) {
            double anxietyProb = 0.3 * DIFFICULTY_SETTINGS.at(gameState.gameDifficulty).anxietyMultiplier;
            anxietyProb *= gameState.anxietyMultiplier;
            anxietyProb *= aoYeMult;
            if (Utils::randomBool(anxietyProb)) {
                gameState.isAnxious = true;
                applyStatDelta("mood", -2, "焦虑发作");
                fact(SettlementFact::Cat::Health, "焦虑发作！心态 -2（连续低心态）");
            }
        }
        if (gameState.isAnxious) {
            fact(SettlementFact::Cat::Health, "焦虑状态中，效率降低");
        }
    } else {
        if (gameState.isAnxious) {
            fact(SettlementFact::Cat::Health, "心态恢复，焦虑缓解");
        }
        gameState.isAnxious = false;
        gameState.anxietyMonths = 0;
    }

    // 6.5 熬夜的健康代价
    if (gameState.isAoYe) {
        applyStatDelta("health", -3, "熬夜冲刺");
        fact(SettlementFact::Cat::Health, "熬夜冲刺：健康 -3");
    }

    // 6.6 病倒判定（健康≤4；锻炼≥2次减半；运气降低概率）
    if (!gameState.sickNext && gameState.health > 0 && gameState.health <= 4) {
        double chance = std::max(0.05, 0.25 * (1.0 - gameState.playerStats.luck * 0.01));
        if (gameState.exerciseCountThisMonth >= 2) chance *= 0.5;
        if (Utils::randomBool(chance)) {
            gameState.sickNext = true;
            fact(SettlementFact::Cat::Health, "病倒了！下月行动力 -2、心态 -1（只能静养）");
        }
    }

    // 7. 健康检查
    if (gameState.health <= 0) {
        fact(SettlementFact::Cat::Other, "健康归零！游戏结束！");
    }
}

// ========== v2 学年过渡 ==========

inline void applyYearTransition(int oldYear, int newYear) {
    if (oldYear == newYear) return;

    // 文化课效率跨年加权
    int culture = gameState.playerStats.culture;
    if (culture >= 16) {
        gameState.cultureEfficiency = 1.3;
    } else if (culture >= 12) {
        gameState.cultureEfficiency = 1.1;
    } else if (culture >= 8) {
        gameState.cultureEfficiency = 1.0;
    } else if (culture >= 4) {
        gameState.cultureEfficiency = 0.8;
    } else {
        gameState.cultureEfficiency = 0.6;
    }

    // 高二停课选项解锁
    if (newYear == 2) {
        logEvent("步入高二，可以选择停课训练", "event");
    }

    // 高三文化课效率恢复（如果停课）
    if (newYear == 3) {
        logEvent("步入高三，时间紧迫", "event");
    }

    // 重置焦虑
    gameState.anxietyMonths = 0;
}

// 保存考试分数
inline void recordExamScore(int score, int maxScore, bool isGaokao) {
    GameState::ExamRecord rec;
    rec.month = gameState.currentMonth;
    rec.calendarMonth = gameState.calendarMonth;
    rec.score = score;
    rec.maxScore = maxScore;
    rec.isGaokao = isGaokao;
    gameState.examRecords.push_back(rec);
}

// 获取学习效率（含焦虑影响；钢铁意志特质免疫焦虑惩罚）
inline double getStudyEfficiency() {
    double eff = getMoodEfficiency();
    eff *= gameState.efficiencyMultiplier;
    if (gameState.isAnxious && !playerHasTrait("iron_will")) eff *= 0.7;
    return std::max(0.3, std::min(1.5, eff));
}

#endif // GAME_HPP
