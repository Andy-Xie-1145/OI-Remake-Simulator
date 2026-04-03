#ifndef GAME_HPP
#define GAME_HPP

#include "types.hpp"
#include "problem_pool.hpp"
#include "events.hpp"
#include <cctype>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <set>
#include <limits>

// ========== 全局状态（完全复制原版变量） ==========
inline PlayerStats playerStats;
inline std::string gameDifficulty = "hard";
inline int timePoints = 24;
inline int mood = 10;
inline int currentProblem = 1;
inline int totalProblems = 0;
inline std::string currentContestName = "NOIP";
inline bool debugmode = false;

// 题目状态
inline std::vector<Problem> problems;
inline std::vector<std::vector<SubProblem>> subProblems;
inline std::vector<std::vector<int>> thinkProgress;
inline std::vector<std::vector<int>> codeProgress;
inline std::vector<std::vector<bool>> isCodeComplete;
inline std::vector<std::vector<double>> errorRates;

// 操作记录
inline std::vector<std::string> lastActions;

// 训练阶段
inline int currentPhase = 1;
inline int totalTrainingEvents = 5;

// 商店价格
inline std::map<std::string, int> currentShopPrices;

// 日志
inline std::vector<std::string> gameLog;

struct PendingContestNotice {
    bool active = false;
    std::string title;
    std::string description;
    std::string effectText;
};

inline PendingContestNotice pendingContestNotice;

// ========== 日志函数 ==========
inline void logEvent(const std::string& message, const std::string& type = "") {
    std::string prefix;
    if (type == "event") prefix = "【事件】";
    else if (type == "think") prefix = "【思考】";
    else if (type == "code") prefix = "【代码】";
    else if (type == "check") prefix = "【对拍】";
    
    std::string fullMsg = prefix + message;
    gameLog.push_back(fullMsg);
    std::cout << fullMsg << std::endl;
}

inline void waitForEnter(const std::string& prompt = "\n按回车继续...") {
    std::cout << prompt;
    if (std::cin.eof()) {
        std::cout << "\n检测到输入结束，游戏退出。\n";
        std::exit(0);
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (std::cin.eof()) {
        std::cout << "\n检测到输入结束，游戏退出。\n";
        std::exit(0);
    }
    std::cin.get();
    if (!std::cin && std::cin.eof()) {
        std::cout << "\n检测到输入结束，游戏退出。\n";
        std::exit(0);
    }
}

inline double difficultyMultiplier() {
    return DIFFICULTY_SETTINGS.at(gameDifficulty).scoreMultiplier;
}

inline const ContestConfig* getCurrentContestConfig() {
    for (const auto& [_, config] : CONTEST_CONFIGS) {
        if (config.name == currentContestName) return &config;
    }
    return nullptr;
}

inline bool isCurrentContestIOI() {
    const auto* config = getCurrentContestConfig();
    return config != nullptr && config->isIOI;
}

inline int getContestIdByName(const std::string& contestName) {
    for (const auto& [id, config] : CONTEST_CONFIGS) {
        if (config.name == contestName) return id;
    }
    return -1;
}

inline void pushLastAction(const std::string& action) {
    lastActions.push_back(action);
    if (lastActions.size() > 5) lastActions.erase(lastActions.begin());
}

inline void clearPendingContestNotice() {
    pendingContestNotice = PendingContestNotice{};
}

inline bool hasPendingContestNotice() {
    return pendingContestNotice.active;
}

inline PendingContestNotice consumePendingContestNotice() {
    PendingContestNotice notice = pendingContestNotice;
    clearPendingContestNotice();
    return notice;
}

// ========== 计算函数（完全复制原版） ==========

inline int calculateThinkTime(const SubProblem& sp) {
    if (debugmode) return 1;
    int thinkTime = 1;
    thinkTime += std::max(0, sp.dp - Utils::mapAttributeValue(playerStats.dp));
    thinkTime += std::max(0, sp.ds - Utils::mapAttributeValue(playerStats.ds));
    thinkTime += std::max(0, sp.str - Utils::mapAttributeValue(playerStats.string));
    thinkTime += std::max(0, sp.graph - Utils::mapAttributeValue(playerStats.graph));
    thinkTime += std::max(0, sp.comb - Utils::mapAttributeValue(playerStats.combinatorics));
    thinkTime += sp.adhoc;
    return thinkTime;
}

inline int calculateCodeTime(const SubProblem& sp) {
    if (debugmode) return 1;
    int codeTime = sp.coding;
    if (playerStats.quickness > 0) {
        codeTime = std::max(1, codeTime - playerStats.quickness);
    }
    return codeTime;
}

inline double calculateThinkSuccessRate(const SubProblem& sp) {
    if (debugmode) return 1.0;
    double baseProb = 1.0;
    baseProb -= std::max(0, sp.thinking - Utils::mapAttributeValue(playerStats.thinking)) * 0.05;
    baseProb -= std::pow(std::max(10 - mood, 0), 2) * 0.01;
    return std::max(0.3, std::min(0.95, baseProb));
}

inline double calculateCodeSuccessRate(const SubProblem& sp) {
    if (debugmode) return 1.0;
    double baseProb = 1.0;
    baseProb -= std::pow(std::max(10 - mood, 0), 2) * 0.01;
    baseProb -= std::max(0, sp.detail - Utils::mapAttributeValue(playerStats.coding)) * 0.05;
    return std::max(0.4, std::min(0.95, baseProb));
}

inline double calculateErrorRate(const SubProblem& sp) {
    if (debugmode) return 0.0;
    double baseProb = 0.1;
    baseProb += sp.trap * 0.05;
    baseProb -= playerStats.carefulness * 0.03;
    baseProb += std::pow(std::max(10 - mood, 0), 2) * 0.01;
    return std::max(0.0, std::min(0.8, baseProb));
}

inline std::string joinDisplayParts(const std::vector<std::string>& parts, const std::string& separator) {
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += separator;
        result += parts[i];
    }
    return result;
}

inline int getBaseBlurLevel(const Problem& problem, const SubProblem& sp) {
    (void)problem;
    return std::max(0, sp.blur);
}

inline int getEffectiveBlurLevel(const Problem& problem, const SubProblem& sp) {
    return std::max(0, getBaseBlurLevel(problem, sp) - playerStats.experience);
}

inline std::string getBlurTraitText(const Problem& problem, const SubProblem& sp) {
    const int baseBlur = getBaseBlurLevel(problem, sp);
    const int effectiveBlur = getEffectiveBlurLevel(problem, sp);
    if (sp.blur <= 0 || effectiveBlur <= 0) return "";
    return "模糊:" + std::to_string(baseBlur) + "->" + std::to_string(effectiveBlur);
}

inline int getActiveBlurLevel(int problemIdx, int subProblemIdx) {
    if (problemIdx < 0 || problemIdx >= static_cast<int>(problems.size())) return 0;
    if (problemIdx >= static_cast<int>(subProblems.size())) return 0;
    if (subProblemIdx < 0 || subProblemIdx >= static_cast<int>(subProblems[problemIdx].size())) return 0;

    const auto& sp = subProblems[problemIdx][subProblemIdx];
    const int thinkTime = calculateThinkTime(sp);
    if (thinkProgress[problemIdx][subProblemIdx] >= thinkTime) return 0;
    return getEffectiveBlurLevel(problems[problemIdx], sp);
}

inline std::string getThinkTimeDisplayTotal(int problemIdx, int subProblemIdx) {
    if (problemIdx < 0 || problemIdx >= static_cast<int>(subProblems.size())) return "?";
    if (subProblemIdx < 0 || subProblemIdx >= static_cast<int>(subProblems[problemIdx].size())) return "?";

    const auto& sp = subProblems[problemIdx][subProblemIdx];
    if (getActiveBlurLevel(problemIdx, subProblemIdx) > 0) return "?";
    return std::to_string(calculateThinkTime(sp));
}

inline std::string buildSubProblemRequirementText(int problemIdx, int subProblemIdx) {
    if (problemIdx < 0 || problemIdx >= static_cast<int>(subProblems.size())) return "无显式要求";
    if (subProblemIdx < 0 || subProblemIdx >= static_cast<int>(subProblems[problemIdx].size())) return "无显式要求";

    const auto& problem = problems[problemIdx];
    const auto& sp = subProblems[problemIdx][subProblemIdx];
    const int activeBlur = getActiveBlurLevel(problemIdx, subProblemIdx);
    std::vector<std::string> requirements;
    std::vector<std::string> traits;
    const std::string blurTraitText = getBlurTraitText(problem, sp);

    if (activeBlur >= 3) {
        return blurTraitText.empty() ? "要求：模糊不清" : "要求：模糊不清\n特性：" + blurTraitText;
    }

    auto addRequirement = [&requirements](const std::string& label, int value, bool hidden) {
        if (value > 0) requirements.push_back(label + ":" + (hidden ? "?" : std::to_string(value)));
    };

    addRequirement("动态规划", sp.dp, activeBlur >= 1);
    addRequirement("数据结构", sp.ds, activeBlur >= 1);
    addRequirement("字符串", sp.str, activeBlur >= 1);
    addRequirement("图论", sp.graph, activeBlur >= 1);
    addRequirement("组合计数", sp.comb, activeBlur >= 1);
    addRequirement("思维", sp.thinking, activeBlur >= 2);
    addRequirement("代码", sp.coding, activeBlur >= 2);

    if (sp.adhoc > 0) traits.push_back("Adhoc:" + std::to_string(sp.adhoc));

    if (activeBlur <= 1) {
        if (sp.detail > 0) traits.push_back("细节:" + std::to_string(sp.detail));
        if (sp.trap > 0) traits.push_back("陷阱:" + std::to_string(sp.trap));
        if (sp.heat > 0) traits.push_back("红温:" + std::to_string(sp.heat));
        if (sp.fallback > 0) traits.push_back("回退:" + std::to_string(sp.fallback + 1));
        if (sp.inspire > 0) traits.push_back("激励:+" + std::to_string(sp.inspire));
    }

    if (!blurTraitText.empty()) traits.push_back(blurTraitText);
    if (activeBlur <= 2 && sp.independent == 0) traits.push_back("非独立");

    const std::string requirementText = requirements.empty() ? "无显式要求" : joinDisplayParts(requirements, "  ");
    if (traits.empty()) return requirementText;
    return requirementText + "\n特性：" + joinDisplayParts(traits, "  ");
}

inline void addExperience(int amount, const std::string& reason) {
    if (amount <= 0) return;

    const int before = playerStats.experience;
    playerStats.experience = std::min(20, playerStats.experience + amount);
    const int gained = playerStats.experience - before;
    if (gained > 0) {
        logEvent(reason + "，经验+" + std::to_string(gained) + "，当前经验：" + std::to_string(playerStats.experience), "event");
    }
}

inline void settleTempExperience(const std::string& reason = "经验积累转化") {
    while (playerStats.tempExperience >= 6 && playerStats.experience < 20) {
        playerStats.tempExperience -= 6;
        addExperience(1, reason);
    }
}

inline void addTempExperience(int amount, const std::string& reason) {
    if (amount <= 0) return;

    playerStats.tempExperience += amount;
    logEvent(reason + "，经验积累+" + std::to_string(amount) + "，当前经验积累：" + std::to_string(playerStats.tempExperience), "event");
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

// ========== UI显示函数（使用\t对齐） ==========

inline void displayPlayerStatus() {
    std::cout << "\n┌────────────────────────────────────────┐\n";
    std::cout << "│\t玩家属性 [" << DIFFICULTY_SETTINGS.at(gameDifficulty).name << "难度]\t\t│\n";
    std::cout << "├────────────────────────────────────────┤\n";
    std::cout << "│\t决心: " << std::setw(5) << playerStats.determination << "\t\t\t│\n";
    std::cout << "│\t心态: " << std::setw(2) << mood << "/" << MOOD_LIMIT << "\t\t\t\t│\n";
    std::cout << "├────────────────────────────────────────┤\n";
    std::cout << "│\t【知识点】\t\t\t\t│\n";
    std::cout << "│\t  动态规划: " << std::setw(2) << playerStats.dp << "\t\t\t\t│\n";
    std::cout << "│\t  数据结构: " << std::setw(2) << playerStats.ds << "\t\t\t\t│\n";
    std::cout << "│\t  字符串: " << std::setw(2) << playerStats.string << "\t\t\t\t│\n";
    std::cout << "│\t  图论: " << std::setw(2) << playerStats.graph << "\t\t\t\t│\n";
    std::cout << "│\t  组合计数: " << std::setw(2) << playerStats.combinatorics << "\t\t\t│\n";
    std::cout << "├────────────────────────────────────────┤\n";
    std::cout << "│\t【能力】\t\t\t\t│\n";
    std::cout << "│\t  思维: " << std::setw(2) << playerStats.thinking << "\t\t\t\t│\n";
    std::cout << "│\t  代码: " << std::setw(2) << playerStats.coding << "\t\t\t\t│\n";
    if (playerStats.carefulness > 0) std::cout << "│\t  细心: " << std::setw(2) << playerStats.carefulness << "\t\t\t\t│\n";
    if (playerStats.quickness > 0) std::cout << "│\t  迅捷: " << std::setw(2) << playerStats.quickness << "\t\t\t\t│\n";
    if (playerStats.mental > 0) std::cout << "│\t  心理素质: " << std::setw(2) << playerStats.mental << "\t\t\t│\n";
    std::cout << "│\t  经验: " << std::setw(2) << playerStats.experience << "\t\t\t\t│\n";
    std::cout << "│\t  经验积累: " << std::setw(2) << playerStats.tempExperience << "/6\t\t\t│\n";
    if (playerStats.culture > 0) std::cout << "│\t  文化课: " << std::setw(2) << playerStats.culture << "\t\t\t│\n";
    std::cout << "└────────────────────────────────────────┘\n";
}

inline void triggerRandomEvent(int problemIdx, int subProblemIdx) {
    if (timePoints <= 0) return;

    const auto& currentSubProblem = subProblems[problemIdx][subProblemIdx];
    struct PendingEvent {
        int idx = -1;
        double probability = 0.0;
        std::string name;
        std::string description;
        std::string effectText;
    };

    auto lastNAre = [](const std::vector<std::string>& actions, int n, const std::string& value) {
        if (static_cast<int>(actions.size()) < n) return false;
        for (int i = static_cast<int>(actions.size()) - n; i < static_cast<int>(actions.size()); ++i) {
            if (actions[i] != value) return false;
        }
        return true;
    };

    std::vector<PendingEvent> pendingEvents;

    for (int idx = 0; idx < 5; ++idx) {
        bool condition = false;
        double probability = 0.0;
        std::string name;
        std::string description;
        std::string effectText;

        switch (idx) {
        case 0:
            condition = lastActions.size() >= 3 &&
                lastActions[lastActions.size() - 1] == lastActions[lastActions.size() - 2] &&
                lastActions[lastActions.size() - 2] == lastActions[lastActions.size() - 3];
            probability = 0.04;
            name = "心态爆炸";
            description = "连续失败让你感到沮丧...";
            effectText = "心态值-1";
            break;
        case 1:
            condition = !lastActions.empty() && lastActions.back() == "think" &&
                thinkProgress[problemIdx][subProblemIdx] > calculateThinkTime(currentSubProblem) / 2;
            probability = 0.03;
            name = "灵光一闪";
            description = "突然想到了一个好方法！";
            effectText = "心态值+1";
            break;
        case 2:
            condition = lastNAre(lastActions, 2, "code") &&
                codeProgress[problemIdx][subProblemIdx] > calculateCodeTime(currentSubProblem) / 2;
            probability = 0.03;
            name = "代码bug";
            description = "写着写着发现之前的代码有问题...";
            effectText = "代码进度-1";
            break;
        case 3:
            condition = lastNAre(lastActions, 2, "code");
            probability = 0.02;
            name = "键盘故障";
            description = "键盘突然有点不太灵了...";
            effectText = "心态值-1";
            break;
        case 4:
            condition = true;
            probability = 0.01;
            name = "监考老师巡视";
            description = "监考老师正在经过你的座位...";
            effectText = "心态值-1";
            break;
        }

        if (!condition) continue;
        pendingEvents.push_back({idx, probability, name, description, effectText});
    }

    if (pendingEvents.empty()) return;

    const double roll = Utils::randomDouble(0.0, 1.0);
    double accumulated = 0.0;
    for (const auto& event : pendingEvents) {
        accumulated += event.probability;
        if (roll >= accumulated) continue;

        if (event.idx == 0 || event.idx == 3 || event.idx == 4) mood = std::max(0, mood - 1);
        else if (event.idx == 1) mood = std::min(MOOD_LIMIT, mood + 1);
        else if (event.idx == 2) codeProgress[problemIdx][subProblemIdx] = std::max(0, codeProgress[problemIdx][subProblemIdx] - 1);

        logEvent("触发突发事件：" + event.name, "event");
        logEvent(event.description, "event");
        logEvent(event.effectText, "event");
        logEvent("当前心态值：" + std::to_string(mood), "event");
        pendingContestNotice.active = true;
        pendingContestNotice.title = "突发事件：" + event.name;
        pendingContestNotice.description = event.description;
        pendingContestNotice.effectText = event.effectText;
        return;
    }
}

// ========== 比赛系统（完全复制原版） ==========

inline void startContest(int contestId) {
    auto config = CONTEST_CONFIGS.at(contestId);
    currentContestName = config.name;
    timePoints = config.timePoints;
    currentProblem = 1;
    problems.clear();
    subProblems.clear();
    thinkProgress.clear();
    codeProgress.clear();
    isCodeComplete.clear();
    errorRates.clear();
    clearPendingContestNotice();
    
    totalProblems = (int)config.problemRanges.size();
    
    // 选择题目（确保不重复）
    while (true) {
        problems.clear();
        bool unique = true;
        for (int i = 0; i < totalProblems; i++) {
            Problem p = selectProblemFromRange(config.problemRanges[i].first, config.problemRanges[i].second);
            for (const auto& existing : problems) {
                if (existing.name == p.name) { unique = false; break; }
            }
            if (!unique) break;
            problems.push_back(p);
        }
        if (unique) break;
    }
    
    // 初始化状态
    for (const auto& prob : problems) {
        subProblems.push_back(prob.parts);
        thinkProgress.push_back(std::vector<int>(prob.parts.size(), 0));
        codeProgress.push_back(std::vector<int>(prob.parts.size(), 0));
        isCodeComplete.push_back(std::vector<bool>(prob.parts.size(), false));
        errorRates.push_back(std::vector<double>(prob.parts.size(), -1.0));
    }
    
    // 心态下降
    int moodDrop = 1 + playerStats.extraMoodDrop;
    if (playerStats.mental > 0) moodDrop = std::max(0, moodDrop - playerStats.mental);
    mood = std::max(0, mood - moodDrop);
    
    logEvent(config.name + "比赛正式开始！", "event");
    logEvent("进入考场，心态值-" + std::to_string(moodDrop) + "，当前心态值：" + std::to_string(mood), "event");
}

// 显示部分分状态
inline void displaySubProblems() {
    bool isIOIContest = false;
    for (const auto& cfg : CONTEST_CONFIGS) {
        if (cfg.second.name == currentContestName && cfg.second.isIOI) { isIOIContest = true; break; }
    }
    
    if (currentProblem < 1 || currentProblem > (int)subProblems.size()) return;
    int idx = currentProblem - 1;
    
    std::cout << "\n┌────────────────────────────────────────┐\n";
    std::cout << "│\t" << problems[idx].name << " 部分分\t\t\t\t│\n";
    std::cout << "└────────────────────────────────────────┘\n";
    
    for (size_t i = 0; i < subProblems[idx].size(); i++) {
        const auto& sp = subProblems[idx][i];
        int thinkTime = calculateThinkTime(sp);
        int codeTime = calculateCodeTime(sp);
        
        std::cout << "\n部分分" << (i+1) << " (" << sp.score << "分):\n";
        
        if (isCodeComplete[idx][i]) { std::cout << "\t[✓已完成]\n"; continue; }
        
        std::string requirementText = buildSubProblemRequirementText(idx, static_cast<int>(i));
        size_t newlinePos = 0;
        while ((newlinePos = requirementText.find('\n', newlinePos)) != std::string::npos) {
            requirementText.replace(newlinePos, 1, "\n\t");
            newlinePos += 2;
        }
        std::cout << "\t" << requirementText << "\n";
        
        double thinkRate = calculateThinkSuccessRate(sp);
        double codeRate = calculateCodeSuccessRate(sp);
        
        if (thinkProgress[idx][i] < thinkTime)
            std::cout << "\t[" << (i+1) << "a] 思考 (" << thinkProgress[idx][i] << "/" << getThinkTimeDisplayTotal(idx, static_cast<int>(i)) << ", 成功率:" << (int)(thinkRate*100) << "%)\n";
        if (thinkProgress[idx][i] >= thinkTime && codeProgress[idx][i] < codeTime)
            std::cout << "\t[" << (i+1) << "b] 写代码 (" << codeProgress[idx][i] << "/" << codeTime << ", 成功率:" << (int)(codeRate*100) << "%)\n";
        if (codeProgress[idx][i] >= codeTime)
            std::cout << "\t[" << (i+1) << "c] " << (isIOIContest ? "提交" : "对拍") << " (出错概率:" << (errorRates[idx][i] >= 0 ? std::to_string((int)(errorRates[idx][i]*100)) : "?") << "%)\n";
    }
}

// 思考部分分
inline void thinkSubProblem(int problemIdx, int subProblemIdx) {
    if (timePoints <= 0) { std::cout << "时间点不足！\n"; return; }
    const auto& sp = subProblems[problemIdx][subProblemIdx];
    double invalidProb = 1.0 - calculateThinkSuccessRate(sp);
    timePoints--;
    
    if (Utils::randomBool(invalidProb)) {
        logEvent("T" + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1) + " 思考无效！", "think");
        if (sp.heat > 0) {
            int moodDrop = Utils::randomInt(0, sp.heat);
            if (playerStats.mental > 0) moodDrop = std::max(0, moodDrop - playerStats.mental);
            mood = std::max(0, mood - moodDrop);
            if (moodDrop > 0) logEvent("红温效应，心态-" + std::to_string(moodDrop), "think");
        }
    } else {
        thinkProgress[problemIdx][subProblemIdx]++;
        logEvent("T" + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1) + " 思考成功！", "think");
        if (sp.independent == 0) {
            for (size_t i = 0; i < (size_t)subProblemIdx; i++) {
                if (subProblems[problemIdx][i].independent == 0) {
                    thinkProgress[problemIdx][i]++;
                    logEvent("T" + std::to_string(problemIdx+1) + " 部分分" + std::to_string(i+1) + " 非独立+1", "think");
                }
            }
        }
    }
    pushLastAction("think");
    triggerRandomEvent(problemIdx, subProblemIdx);
}

// 写代码部分分
inline void writeCodeSubProblem(int problemIdx, int subProblemIdx) {
    if (timePoints <= 0) { std::cout << "时间点不足！\n"; return; }
    const auto& sp = subProblems[problemIdx][subProblemIdx];
    double invalidProb = 1.0 - calculateCodeSuccessRate(sp);
    timePoints--;
    
    if (Utils::randomBool(invalidProb)) {
        logEvent("T" + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1) + " 写代码无效！", "code");
        if (sp.heat > 0) {
            int moodDrop = sp.heat;
            if (playerStats.mental > 0) moodDrop = std::max(0, moodDrop - playerStats.mental);
            mood = std::max(0, mood - moodDrop);
            logEvent("红温效应，心态-" + std::to_string(moodDrop), "code");
        }
    } else {
        codeProgress[problemIdx][subProblemIdx]++;
        logEvent("T" + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1) + " 写代码成功！", "code");
        if (codeProgress[problemIdx][subProblemIdx] >= calculateCodeTime(sp)) {
            errorRates[problemIdx][subProblemIdx] = calculateErrorRate(sp);
            if (sp.inspire > 0) {
                mood = std::min(MOOD_LIMIT, mood + sp.inspire);
                logEvent("激励效果，心态+" + std::to_string(sp.inspire), "code");
            }
        }
    }
    pushLastAction("code");
    triggerRandomEvent(problemIdx, subProblemIdx);
}

// 对拍/提交部分分
inline void checkCodeSubProblem(int problemIdx, int subProblemIdx) {
    bool isIOIContest = false;
    for (const auto& cfg : CONTEST_CONFIGS) {
        if (cfg.second.name == currentContestName && cfg.second.isIOI) { isIOIContest = true; break; }
    }
    if (!isIOIContest && timePoints <= 0) { std::cout << "时间点不足！\n"; return; }
    if (!isIOIContest) timePoints--;
    
    pushLastAction("check");
    
    logEvent((isIOIContest ? "提交" : "对拍") + std::string(" T") + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1), "check");
    
    double errorRate = errorRates[problemIdx][subProblemIdx];
    if (Utils::randomBool(errorRate)) {
        if (isIOIContest && Utils::randomBool(0.08)) {
            mood = std::max(0, mood - 1);
            logEvent("服务器爆炸，心态-1", "check");
        } else {
            const auto& sp = subProblems[problemIdx][subProblemIdx];
            int fallback = sp.fallback + 1;
            codeProgress[problemIdx][subProblemIdx] = std::max(0, codeProgress[problemIdx][subProblemIdx] - fallback);
            logEvent((isIOIContest ? "提交" : "对拍") + std::string("失败！代码-") + std::to_string(fallback), "check");
        }
    } else {
        const auto& sp = subProblems[problemIdx][subProblemIdx];
        isCodeComplete[problemIdx][subProblemIdx] = true;
        logEvent((isIOIContest ? "提交" : "对拍") + std::string("成功！获得 ") + std::to_string(sp.score) + " 分", "check");
        if (sp.inspire > 0) mood = std::min(MOOD_LIMIT, mood + sp.inspire);
    }
    triggerRandomEvent(problemIdx, subProblemIdx);
}

inline bool isFullScore() {
    for (int i = 0; i < totalProblems; i++) {
        int lastIdx = (int)subProblems[i].size() - 1;
        if (!isCodeComplete[i][lastIdx]) return false;
    }
    return true;
}

inline int calculateScore() {
    int total = 0;
    for (int i = 0; i < totalProblems; i++) {
        int maxScore = 0;
        for (size_t j = 0; j < subProblems[i].size(); j++) {
            if (isCodeComplete[i][j]) maxScore = std::max(maxScore, subProblems[i][j].score);
        }
        total += maxScore;
    }
    return total;
}

// 评奖
inline std::string calculateAward(int score, const std::string& contestType) {
    double mult = difficultyMultiplier();
    std::string award;
    
    if (contestType == "CSP-S" || contestType == "NOIP") {
        if (score >= 270 * mult) award = "一等奖";
        else if (score >= 180 * mult) award = "二等奖";
        else if (score >= 50 * mult) award = "三等奖";
        else award = "没有获奖";
        playerStats.achievements.push_back(contestType + "：" + std::to_string(score) + "分，" + award);
    } else if (contestType == "WC" || contestType == "APIO") {
        if (score >= 220 * mult) award = "金牌";
        else if (score >= 160 * mult) award = "银牌";
        else if (score >= 100 * mult) award = "铜牌";
        else award = "铁牌";
        playerStats.achievements.push_back(contestType + "：" + std::to_string(score) + "分，" + award);
    } else if (contestType == "省选") {
        int totalScore = playerStats.tempScore;
        playerStats.isProvincialTeamA = false;
        if (totalScore >= 700 * mult) {
            award = "省队A队";
            playerStats.isProvincialTeamA = true;
        } else if (totalScore >= 600 * mult) {
            award = "省队B队";
        } else {
            award = "没有进队";
        }
        playerStats.isProvincialTeam = award.find("省队") != std::string::npos;
        playerStats.achievements.push_back("省选：" + std::to_string(totalScore) + "分，" + award);
    } else if (contestType == "NOI") {
        int totalScore = playerStats.tempScore;
        if (totalScore >= 400 * mult) { award = "金牌"; playerStats.isTrainingTeam = true; }
        else if (totalScore >= 300 * mult) award = "银牌";
        else if (totalScore >= 200 * mult) award = "铜牌";
        else award = "铁牌";
        playerStats.achievements.push_back("NOI：" + std::to_string(totalScore) + "分，" + award);
    } else if (contestType == "CTT") {
        int totalScore = playerStats.tempScore;
        award = totalScore >= 600 * mult ? "入选候选队" : "没有入选候选队";
        if (totalScore >= 600 * mult) playerStats.isCandidateTeam = true;
        playerStats.achievements.push_back("CTT：" + std::to_string(totalScore) + "分，" + award);
    } else if (contestType == "CTS") {
        int totalScore = playerStats.tempScore;
        award = totalScore >= 900 * mult ? "入选国家队" : "没有入选国家队";
        if (totalScore >= 900 * mult) playerStats.isNationalTeam = true;
        playerStats.achievements.push_back("CTS：" + std::to_string(totalScore) + "分，" + award);
    } else if (contestType == "IOI") {
        int totalScore = playerStats.tempScore;
        if (totalScore >= 400 * mult) { award = "金牌"; playerStats.isIOIgold = true; }
        else if (totalScore >= 300 * mult) award = "银牌";
        else if (totalScore >= 200 * mult) award = "铜牌";
        else award = "铁牌";
        playerStats.achievements.push_back("IOI：" + std::to_string(totalScore) + "分，" + award);
    }
    
    if (isTopAwardForExperience(contestType, award)) {
        addTempExperience(1, contestType + "最高奖项奖励");
    }

    return award;
}

// 显示比赛结果
inline void showResults() {
    const bool isIOIContest = isCurrentContestIOI();
    int totalExpectedScore = 0;
    int totalActualScore = 0;
    
    std::cout << "\n┌────────────────────────────────────────┐\n";
    std::cout << "│\t比 赛 结 果\t\t\t\t│\n";
    std::cout << "└────────────────────────────────────────┘\n";
    
    for (int i = 0; i < totalProblems; i++) {
        int problemExpectedScore = 0;
        int problemActualScore = 0;
        for (size_t j = 0; j < subProblems[i].size(); j++) {
            const auto& sp = subProblems[i][j];
            bool codeCompleted = codeProgress[i][j] >= calculateCodeTime(sp);
            bool checkCompleted = isCodeComplete[i][j];
            if (codeCompleted) problemExpectedScore = std::max(problemExpectedScore, sp.score);
            if (checkCompleted) {
                problemActualScore = std::max(problemActualScore, sp.score);
            } else if (codeCompleted && !isIOIContest) {
                double successRate = 1.0 - errorRates[i][j];
                for (int k = (int)j; k >= 0; --k) {
                    if (Utils::randomBool(successRate)) {
                        problemActualScore = std::max(problemActualScore, subProblems[i][k].score);
                        break;
                    }
                }
            }
        }
        totalExpectedScore += problemExpectedScore;
        totalActualScore += problemActualScore;
        std::cout << "T" << (i + 1) << " (" << problems[i].name << "): 预期 " << problemExpectedScore
                  << " 分, 实际 " << problemActualScore << " 分\n";
    }
    
    std::cout << "\n预期总分: " << totalExpectedScore << "\n";
    std::cout << "实际总分: " << totalActualScore << "\n";

    if (currentContestName == "省选Day2") {
        playerStats.tempScore = totalActualScore + playerStats.prevScore + playerStats.noipScore;
        std::cout << "省选总分: " << playerStats.tempScore << " (Day1 + Day2 + NOIP)\n";
    } else if (currentContestName == "NOI Day2") {
        playerStats.tempScore = totalActualScore + playerStats.prevScore + (playerStats.isProvincialTeamA ? 5 : 0);
        std::cout << "NOI总分: " << playerStats.tempScore << "\n";
    } else if (currentContestName == "IOI Day2") {
        playerStats.tempScore = totalActualScore + playerStats.prevScore;
        std::cout << "IOI总分: " << playerStats.tempScore << "\n";
    } else if (currentContestName == "CTT Day4") {
        playerStats.tempScore = totalActualScore + playerStats.prevScore1 + playerStats.prevScore2 + playerStats.prevScore3;
        playerStats.cttScore = playerStats.tempScore;
        std::cout << "CTT总分: " << playerStats.tempScore << "\n";
    } else if (currentContestName == "CTS Day2") {
        playerStats.tempScore = totalActualScore + playerStats.prevScore + playerStats.cttScore;
        std::cout << "CTS总分: " << playerStats.tempScore << "\n";
    }
    
    int detReward = totalActualScore * 5;
    playerStats.determination += detReward;
    std::cout << "决心奖励: +" << detReward << "\n";
    
    // 心态恢复
    int minMood = std::min(5 + playerStats.mental, 10);
    if (mood < minMood) {
        int recovery = minMood - mood;
        mood = minMood;
        logEvent("比赛结束后心态自动恢复：+" + std::to_string(recovery) + "，当前心态值：" + std::to_string(mood), "event");
    }
    
    bool isFinalDayContest =
        currentContestName != "省选Day1" &&
        currentContestName != "NOI Day1" &&
        currentContestName != "IOI Day1" &&
        currentContestName != "CTT Day1" &&
        currentContestName != "CTT Day2" &&
        currentContestName != "CTT Day3" &&
        currentContestName != "CTS Day1";

    if (isFinalDayContest) {
        std::string award;
        if (currentContestName == "省选Day2") award = calculateAward(totalActualScore, "省选");
        else if (currentContestName == "NOI Day2") award = calculateAward(totalActualScore, "NOI");
        else if (currentContestName == "IOI Day2") award = calculateAward(totalActualScore, "IOI");
        else if (currentContestName == "CTT Day4") award = calculateAward(totalActualScore, "CTT");
        else if (currentContestName == "CTS Day2") award = calculateAward(totalActualScore, "CTS");
        else award = calculateAward(totalActualScore, currentContestName);

        if (currentContestName == "CSP-S") playerStats.cspScore = totalActualScore;
        else if (currentContestName == "NOIP") playerStats.noipScore = totalActualScore;

        std::cout << "获奖情况: " << award << "\n";
    } else {
        playerStats.prevScore = totalActualScore;
        if (currentContestName == "CTT Day1") playerStats.prevScore1 = totalActualScore;
        else if (currentContestName == "CTT Day2") playerStats.prevScore2 = totalActualScore;
        else if (currentContestName == "CTT Day3") playerStats.prevScore3 = totalActualScore;
    }

    logEvent("比赛结束！实际总分: " + std::to_string(totalActualScore), "event");
}

// 比赛主循环
inline void runContestLoop(int contestId) {
    startContest(contestId);
    
    while (timePoints > 0 && !isFullScore()) {
        std::cout << "\n┌────────────────────────────────────────┐\n";
        std::cout << "│\t" << currentContestName << "\t\t\t\t│\n";
        std::cout << "│\t时间: " << std::setw(3) << timePoints << "  题目: T" << currentProblem << "\t\t\t│\n";
        std::cout << "│\t决心: " << std::setw(5) << playerStats.determination;
        std::cout << "  心态: " << mood << "/" << MOOD_LIMIT << "\t\t│\n";
        std::cout << "└────────────────────────────────────────┘\n";
        
        displaySubProblems();
        
        std::cout << "\n操作: [数字][a/b/c] 或 [p]上一题 [n]下一题 [0]离场\n";
        std::cout << "请选择: ";
        
        std::string input = Utils::readToken();
        
        if (input == "p") currentProblem = currentProblem > 1 ? currentProblem - 1 : totalProblems;
        else if (input == "n") currentProblem = currentProblem < totalProblems ? currentProblem + 1 : 1;
        else if (input == "0" && isFullScore()) break;
        else if (input.length() >= 2 && std::isdigit(static_cast<unsigned char>(input[0]))) {
            int subIdx = input[0] - '0' - 1;
            char action = input[1];
            if (subIdx >= 0 && subIdx < (int)subProblems[currentProblem-1].size()) {
                if (action == 'a') thinkSubProblem(currentProblem - 1, subIdx);
                else if (action == 'b') writeCodeSubProblem(currentProblem - 1, subIdx);
                else if (action == 'c') checkCodeSubProblem(currentProblem - 1, subIdx);
            }
        }
    }
    
    showResults();
    waitForEnter();
}

// ========== 获取训练事件类型（完全复制原版逻辑） ==========
inline std::string getTrainingEventType(int currentEvent, int /*totalEvents*/) {
    // 根据当前阶段和事件序号决定事件类型
    if (currentPhase == 1) { 
        // 第一次训练(5次)：【长期训练】【提升训练/比赛训练】【娱乐时间】【提升训练/比赛训练】【考前一天】
        if (currentEvent == 1) return "长期训练";
        else if (currentEvent == 2 || currentEvent == 4) 
            return Utils::randomBool(0.5) ? "提升训练" : "比赛训练";
        else if (currentEvent == 3) return "娱乐时间";
        else if (currentEvent == 5) return "赛前一天";
    } else if (currentPhase == 17) { 
        // 第八次训练(8次)：【步入高二】【长期训练】【提升训练/比赛训练】【提升训练/比赛训练】【娱乐时间】【提升训练/比赛训练】【焦虑】【考前一天】
        if (currentEvent == 1) return "步入高二";
        else if (currentEvent == 2) return "长期训练";
        else if (currentEvent == 3 || currentEvent == 4 || currentEvent == 6) 
            return Utils::randomBool(0.5) ? "提升训练" : "比赛训练";
        else if (currentEvent == 5) return "娱乐时间";
        else if (currentEvent == 7) return "焦虑";
        else if (currentEvent == 8) return "赛前一天";
    } else if (currentPhase == 19 || currentPhase == 31 || currentPhase == 38) { 
        // 5次训练：【提升训练/比赛训练】【娱乐时间】【焦虑】【遗忘】【考前一天】
        if (currentEvent == 1) return Utils::randomBool(0.5) ? "提升训练" : "比赛训练";
        else if (currentEvent == 2) return "娱乐时间";
        else if (currentEvent == 3) return "焦虑";
        else if (currentEvent == 4) return "遗忘";
        else if (currentEvent == 5) return "赛前一天";
    } else if (currentPhase == 3 || currentPhase == 5 || currentPhase == 7 || 
               currentPhase == 11 || currentPhase == 13 || currentPhase == 26 || 
               currentPhase == 29 || currentPhase == 35 || currentPhase == 50) { 
        // 4次训练：【提升训练/比赛训练】【娱乐时间】【焦虑】【考前一天】
        if (currentEvent == 1) return Utils::randomBool(0.5) ? "提升训练" : "比赛训练";
        else if (currentEvent == 2) return "娱乐时间";
        else if (currentEvent == 3) return "焦虑";
        else if (currentEvent == 4) return "赛前一天";
    } else if (currentPhase == 9 || currentPhase == 15 || currentPhase == 21 || 
               currentPhase == 33 || currentPhase == 40 || currentPhase == 45) { 
        // 2次训练：【焦虑】【考前一天】
        if (currentEvent == 1) return "焦虑";
        else if (currentEvent == 2) return "赛前一天";
    } else if (currentPhase == 42 || currentPhase == 53) { 
        // 6次训练：【提升训练/比赛训练】【娱乐时间】【提升训练/比赛训练】【娱乐时间】【焦虑】【考前一天】
        if (currentEvent == 1 || currentEvent == 3) 
            return Utils::randomBool(0.5) ? "提升训练" : "比赛训练";
        else if (currentEvent == 2 || currentEvent == 4) return "娱乐时间";
        else if (currentEvent == 5) return "焦虑";
        else if (currentEvent == 6) return "赛前一天";
    }
    
    // 默认返回提升训练
    return "提升训练";
}

// ========== 训练阶段 ==========
inline void runTrainingPhase(int numEvents) {
    totalTrainingEvents = numEvents;
    for (int i = 0; i < numEvents; i++) {
        int currentEvent = i + 1;
        
        // 清屏
        clearScreen();
        
        std::cout << "\n┌────────────────────────────────────────┐\n";
        std::cout << "│\t训练阶段 (" << currentEvent << "/" << numEvents << ")\t\t\t│\n";
        std::cout << "└────────────────────────────────────────┘\n";
        
        // 根据阶段和事件序号选择事件类型（完全复制原版逻辑）
        std::string eventType = getTrainingEventType(currentEvent, numEvents);
        
        logEvent("触发事件: " + eventType, "event");
        
        displayPlayerStatus();
        
        runEventChain(eventType);
        
        waitForEnter();
    }
}

// ========== 游戏初始化和主流程 ==========

inline void initGame() {
    initProblemPool();
    playerStats = PlayerStats();
    auto settings = DIFFICULTY_SETTINGS.at(gameDifficulty);
    playerStats.determination = settings.initialDetermination;
    playerStats.extraMoodDrop = (gameDifficulty == "expert") ? 2 : (gameDifficulty == "easy") ? 0 : 1;
    mood = 10;
    currentPhase = 1;
    currentProblem = 1;
    totalProblems = 0;
    currentContestName = "NOIP";
    gameLog.clear();
    currentShopPrices = INITIAL_SHOP_PRICES.at(gameDifficulty);
    clearShopState();
}

inline void showTitle() {
    std::cout << "\n┌────────────────────────────────────────────────┐\n";
    std::cout << "│\t\t\t\t\t\t\t│\n";
    std::cout << "│\t\tOI 重 开 模 拟 器 v2.0\t\t\t│\n";
    std::cout << "│\t\t\t\t\t\t\t│\n";
    std::cout << "│\t    重开你的人生，成为一名OIer\t\t\t│\n";
    std::cout << "│\t\t\t\t\t\t\t│\n";
    std::cout << "└────────────────────────────────────────────────┘\n\n";
}

inline void selectDifficulty() {
    std::cout << "请选择游戏难度：\n";
    std::cout << "  1. 简单 - 天赋点30，决心3000，分数线降低20%\n";
    std::cout << "  2. 普通 - 天赋点20，决心1500，分数线降低10%\n";
    std::cout << "  3. 困难 - 天赋点15，决心500，标准难度\n";
    std::cout << "  4. 专家 - 天赋点15，决心0，分数线提高10%\n";
    std::cout << "请输入(1-4): ";
    
    int choice = Utils::readIntInRange(1, 4);
    
    switch(choice) {
        case 1: gameDifficulty = "easy"; break;
        case 2: gameDifficulty = "normal"; break;
        case 3: gameDifficulty = "hard"; break;
        case 4: gameDifficulty = "expert"; break;
        default: gameDifficulty = "hard";
    }
    
    initGame();
    logEvent("选择了" + DIFFICULTY_SETTINGS.at(gameDifficulty).name + "难度", "event");
}

inline void allocateTalent() {
    auto settings = DIFFICULTY_SETTINGS.at(gameDifficulty);
    int total = settings.talentPoints;
    int remaining = total;
    
    std::cout << "\n分配你的初始天赋点！共 " << total << " 点。\n\n";
    
    int dp=0, ds=0, str=0, graph=0, comb=0;
    
    std::cout << "动态规划 (剩余 " << remaining << " 点): "; dp = Utils::readIntInRange(0, remaining); remaining -= dp;
    std::cout << "数据结构 (剩余 " << remaining << " 点): "; ds = Utils::readIntInRange(0, remaining); remaining -= ds;
    std::cout << "字符串 (剩余 " << remaining << " 点): "; str = Utils::readIntInRange(0, remaining); remaining -= str;
    std::cout << "图论 (剩余 " << remaining << " 点): "; graph = Utils::readIntInRange(0, remaining); remaining -= graph;
    std::cout << "组合计数 (剩余 " << remaining << " 点): "; comb = Utils::readIntInRange(0, remaining);
    
    playerStats.dp = dp;
    playerStats.ds = ds;
    playerStats.string = str;
    playerStats.graph = graph;
    playerStats.combinatorics = comb;
    
    displayPlayerStatus();
}

inline void showGameOver(const std::string& reason) {
    std::cout << "\n┌────────────────────────────────────────┐\n";
    std::cout << "│\t\t游 戏 结 束\t\t\t│\n";
    std::cout << "└────────────────────────────────────────┘\n\n";
    std::cout << reason << "\n\n";
    
    std::cout << "【你的成就】\n";
    if (playerStats.achievements.empty()) {
        std::cout << "  - 没有获得任何成就\n";
    } else {
        for (const auto& ach : playerStats.achievements) {
            std::cout << "  - " << ach << "\n";
        }
    }
    
    std::cout << "\n";
    if (playerStats.isIOIgold) std::cout << "你成功拿到了 IOI 金牌，最终还是站在了世界 OI 之巅。\n";
    else if (playerStats.isNationalTeam) std::cout << "你成为了中国国家队选手，代表中国参加了 IOI。\n";
    else if (playerStats.isTrainingTeam) std::cout << "你作为国家集训队选手，已经具备了保送资格。\n";
    else if (playerStats.isProvincialTeam) std::cout << "作为省队选手，你在 OI 的道路上已经取得了不错的成绩。\n";
    else std::cout << "虽然未能进入省队，但你依然收获了宝贵的经验。\n";
}

inline bool maybeRunContest(const std::string& contestName, const std::string& skipReason, bool condition) {
    if (!condition) {
        logEvent(skipReason + "，无法参加" + contestName + "比赛", "event");
        return false;
    }

    int contestId = getContestIdByName(contestName);
    if (contestId < 0) {
        logEvent("未找到比赛配置：" + contestName, "event");
        return false;
    }

    runContestLoop(contestId);
    return true;
}

// ========== 主游戏流程 ==========
inline void runGame() {
    showTitle();
    selectDifficulty();
    allocateTalent();
    waitForEnter("\n按回车开始游戏...");

    currentPhase = 1;
    logEvent("第一次训练开始...", "event");
    runTrainingPhase(5);
    runContestLoop(1);

    currentPhase = 3;
    logEvent("第二次训练开始...", "event");
    runTrainingPhase(4);
    if (playerStats.cspScore <= 0) {
        logEvent("由于CSP-S成绩为零分，无法参加NOIP比赛", "event");
        playerStats.noipScore = 0;
    } else {
        runContestLoop(2);
    }

    currentPhase = 5;
    logEvent("第三次训练开始...", "event");
    runTrainingPhase(4);
    maybeRunContest("WC", "由于CSP-S成绩未达到二等奖及以上", playerStats.cspScore >= 180 * difficultyMultiplier());

    currentPhase = 7;
    logEvent("第四次训练开始...", "event");
    runTrainingPhase(4);
    runContestLoop(4);

    currentPhase = 9;
    logEvent("第五次训练开始...", "event");
    runTrainingPhase(2);
    runContestLoop(5);

    currentPhase = 11;
    logEvent("第六次训练开始...", "event");
    runTrainingPhase(4);
    maybeRunContest("APIO", "由于NOIP成绩未达到二等奖及以上", playerStats.noipScore >= 180 * difficultyMultiplier());

    if (playerStats.isProvincialTeam) {
        currentPhase = 13;
        logEvent("第七次训练开始...", "event");
        runTrainingPhase(4);
        runContestLoop(7);

        currentPhase = 15;
        logEvent("第八次训练开始...", "event");
        runTrainingPhase(2);
        runContestLoop(8);
    } else {
        logEvent("由于未进入省队，第一年的NOI阶段跳过", "event");
    }

    logEvent("经过 1 年的学习与比赛历练，你对 OI 的理解更深了一层。", "event");
    addExperience(1, "升入高二");
    currentPhase = 17;
    logEvent("第九次训练开始...", "event");
    runTrainingPhase(8);
    runContestLoop(1);

    currentPhase = 19;
    logEvent("第十次训练开始...", "event");
    runTrainingPhase(5);
    if (playerStats.cspScore <= 0 && !playerStats.isTrainingTeam) {
        logEvent("由于CSP-S成绩为零分，无法参加NOIP比赛", "event");
        playerStats.noipScore = 0;
    } else {
        runContestLoop(2);
    }

    if (playerStats.isTrainingTeam) {
        currentPhase = 21;
        logEvent("第十一次训练开始...", "event");
        runTrainingPhase(1);
        runContestLoop(9);
        runContestLoop(10);
        runContestLoop(11);
        runContestLoop(12);

        if (playerStats.isCandidateTeam) {
            currentPhase = 26;
            logEvent("第十二次训练开始...", "event");
            runTrainingPhase(4);
            runContestLoop(13);
            runContestLoop(14);
        } else {
            currentPhase = 29;
            logEvent("第十二次训练开始...", "event");
            runTrainingPhase(4);
            maybeRunContest("WC", "由于CSP-S成绩未达到二等奖及以上", playerStats.cspScore >= 180 * difficultyMultiplier());
        }
    } else {
        currentPhase = 29;
        logEvent("第十一次训练开始...", "event");
        runTrainingPhase(4);
        maybeRunContest("WC", "由于CSP-S成绩未达到二等奖及以上", playerStats.cspScore >= 180 * difficultyMultiplier());
    }

    currentPhase = 31;
    logEvent("第十三次训练开始...", "event");
    runTrainingPhase(5);
    playerStats.isProvincialTeam = false;
    playerStats.isProvincialTeamA = false;
    runContestLoop(4);

    currentPhase = 33;
    logEvent("第十五次训练开始...", "event");
    runTrainingPhase(2);
    runContestLoop(5);

    if (!playerStats.isProvincialTeam && !playerStats.isNationalTeam) {
        showGameOver("在高二省选中未能进入省队");
        return;
    }

    currentPhase = 35;
    logEvent("第十六次训练开始...", "event");
    runTrainingPhase(4);
    runContestLoop(6);

    currentPhase = 38;
    logEvent("第十七次训练开始...", "event");
    runTrainingPhase(5);
    playerStats.isTrainingTeam = false;
    runContestLoop(7);

    currentPhase = 40;
    logEvent("第十八次训练开始...", "event");
    runTrainingPhase(2);
    runContestLoop(8);

    if (playerStats.isNationalTeam) {
        currentPhase = 42;
        logEvent("第十九次训练开始...", "event");
        runTrainingPhase(6);
        runContestLoop(15);
        runContestLoop(16);
        if (playerStats.isIOIgold || !playerStats.isTrainingTeam) {
            showGameOver("完成IOI比赛");
            return;
        }
    } else if (!playerStats.isTrainingTeam) {
        showGameOver("完成NOI比赛");
        return;
    }

    currentPhase = 45;
    logEvent("第二十次训练开始...", "event");
    runTrainingPhase(1);
    playerStats.isCandidateTeam = false;
    runContestLoop(9);
    runContestLoop(10);
    runContestLoop(11);
    runContestLoop(12);

    if (!playerStats.isCandidateTeam) {
        showGameOver("未能进入候选队");
        return;
    }

    currentPhase = 50;
    logEvent("第二十一次训练开始...", "event");
    runTrainingPhase(4);
    playerStats.isNationalTeam = false;
    runContestLoop(13);
    runContestLoop(14);

    if (!playerStats.isNationalTeam) {
        showGameOver("未能进入国家队");
        return;
    }

    currentPhase = 53;
    logEvent("第二十二次训练开始...", "event");
    runTrainingPhase(6);
    runContestLoop(15);
    runContestLoop(16);
    showGameOver("完成IOI比赛");
}

#endif // GAME_HPP
