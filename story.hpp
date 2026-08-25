#ifndef STORY_HPP
#define STORY_HPP

#include "types.hpp"
#include <vector>
#include <string>

// ========== v2 月历引擎 ==========

namespace Calendar {

struct MonthInfo {
    int year = 1;
    int calendarMonth = 7;
    std::vector<int> contestIds;  // CONTEST_CONFIGS keys
    bool hasExam = false;
    bool isGaokao = false;
    int apDeduction = 0;
};

inline MonthInfo getMonthInfo(int month) {
    const int cm = monthToCalendarMonth(month);
    const int yr = monthToYear(month);
    MonthInfo info;
    info.year = yr;
    info.calendarMonth = cm;

    // 比赛资格检查
    const auto& ps = gameState.playerStats;
    const double mult = difficultyMultiplier();
    const bool noipGood = ps.noipScore >= 180 * mult;

    // 每月模式
    switch (cm) {
    case 7: // NOI / ISIJ
        if (ps.isProvincialTeam) {
            info.contestIds = {7, 8}; // NOI Day1, Day2
        }
        break;
    case 10: // CSP-S
        info.contestIds = {1};
        info.apDeduction = 5;
        break;
    case 11: // NOIP + 期中
        if (ps.cspScore > 0) {
            info.contestIds = {2};
            info.apDeduction = 5;
        }
        info.hasExam = true;
        break;
    case 1: // WC + 期末
        if (noipGood) {
            info.contestIds = {3};
            info.apDeduction = 5;
        }
        info.hasExam = true;
        break;
    case 3: // 省选
        if (noipGood) {
            info.contestIds = {4, 5}; // 省选 Day1, Day2
            info.apDeduction = 5;
        }
        break;
    case 4: // 期中
        info.hasExam = true;
        info.apDeduction = 5;
        break;
    case 5: // APIO
        if (noipGood) {
            info.contestIds = {6};
            info.apDeduction = 5;
        }
        break;
    case 6: // 期末 / 高考
        info.hasExam = true;
        info.apDeduction = 5;
        if (yr == 3) info.isGaokao = true;
        break;
    }

    // 保送锁定：高三 6 月高考替换为「庆功月」（无考试、不扣 AP）
    if (info.isGaokao && gameState.playerStats.isBaosong) {
        info.hasExam = false;
        info.isGaokao = false;
        info.apDeduction = 0;
    }

    // 资格赛没过 → 恢复 AP（不扣）
    if (info.contestIds.empty() && info.apDeduction > 0 && !info.hasExam) {
        info.apDeduction = 0;
    }

    return info;
}

inline void startMonth(int month) {
    const auto settings = DIFFICULTY_SETTINGS.at(gameState.gameDifficulty);
    const int cm = monthToCalendarMonth(month);
    gameState.currentMonth = month;
    gameState.currentYear = monthToYear(month);
    gameState.calendarMonth = cm;

    // 基础 AP：8 + 停课2 + 熬夜2 + 难度加成（与 Engine::recomputeApBudget 同式）
    int baseAp = 8;
    if (gameState.isTingke) baseAp += 2;
    if (gameState.isAoYe) baseAp += 2;
    baseAp += settings.apBonus;

    // 停课扣心态
    if (gameState.isTingke) {
        applyStatDelta("mood", -3, "停课");
    }

    // 病倒静养：本月行动力 -2、心态 -1（一次性消费，不可清除）
    if (gameState.sickNext) {
        baseAp -= 2;
        applyStatDelta("mood", -1, "病后静养");
        logEvent("大病初愈，这个月状态不佳（AP-2，心态-1）", "event");
        gameState.sickNext = false;
    }
    gameState.exerciseCountThisMonth = 0;  // 新月锻炼计数重置

    // 比赛/考试月扣 AP
    MonthInfo info = getMonthInfo(month);
    baseAp -= info.apDeduction;

    gameState.ap = std::max(0, baseAp);
    gameState.maxAp = baseAp; // 记录原始值用于结算
    gameState.settlementFacts.clear();
}

// endMonth 在 game.hpp 中实现（需要遗忘/金钱等逻辑）

} // namespace Calendar

#endif // STORY_HPP
