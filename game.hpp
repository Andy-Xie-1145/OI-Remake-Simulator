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
inline std::vector<std::vector<int>> modificationCount;  // 修改代码次数跟踪
inline std::vector<std::vector<bool>> hasAttemptedCheck;  // 是否已尝试对拍
inline std::vector<std::vector<bool>> requiresCodeModification;  // 是否必须返工后才能再次对拍

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

inline double calculateLuckReduction() {
    if (playerStats.luck <= 0) return 0.0;
    return std::min(0.45, std::log2(playerStats.luck + 1.0) * 0.095);  // luck=10 时约减少 33%
}

inline double calculateErrorRate(const SubProblem& sp) {
    if (debugmode) return 0.0;
    double baseProb = 0.1;
    baseProb += sp.trap * 0.05;
    baseProb -= playerStats.carefulness * 0.03;
    baseProb += std::pow(std::max(10 - mood, 0), 2) * 0.01;
    baseProb *= (1.0 - calculateLuckReduction());
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
        if (sp.branch > 0) traits.push_back("分支:" + std::to_string(sp.branch));
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


inline void triggerRandomEvent(int problemIdx, int subProblemIdx) {
    if (timePoints <= 0) return;

    const auto& currentSubProblem = subProblems[problemIdx][subProblemIdx];
    double luckReduction = calculateLuckReduction();  // Luck reduces negative events

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
            probability = 0.04 * (1.0 - luckReduction);  // Luck reduces negative events
            name = "心态爆炸";
            description = "连续失败让你感到沮丧...";
            effectText = "心态值-1";
            break;
        case 1:
            condition = !lastActions.empty() && lastActions.back() == "think" &&
                thinkProgress[problemIdx][subProblemIdx] > calculateThinkTime(currentSubProblem) / 2;
            probability = 0.03;  // Positive event, not affected by luck
            name = "灵光一闪";
            description = "突然想到了一个好方法！";
            effectText = "心态值+1";
            break;
        case 2:
            condition = lastNAre(lastActions, 2, "code") &&
                codeProgress[problemIdx][subProblemIdx] > calculateCodeTime(currentSubProblem) / 2;
            probability = 0.03 * (1.0 - luckReduction);  // Luck reduces negative events
            name = "代码bug";
            description = "写着写着发现之前的代码有问题...";
            effectText = "代码进度-1";
            break;
        case 3:
            condition = lastNAre(lastActions, 2, "code");
            probability = 0.02 * (1.0 - luckReduction);  // Luck reduces negative events
            name = "键盘故障";
            description = "键盘突然有点不太灵了...";
            effectText = "心态值-1";
            break;
        case 4:
            condition = true;
            probability = 0.01 * (1.0 - luckReduction);  // Luck reduces negative events
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
    modificationCount.clear();
    hasAttemptedCheck.clear();  // 清除对拍标志
    requiresCodeModification.clear();
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
        modificationCount.push_back(std::vector<int>(prob.parts.size(), 0));  // 初始化修改计数
        hasAttemptedCheck.push_back(std::vector<bool>(prob.parts.size(), false));  // 初始化对拍标志
        requiresCodeModification.push_back(std::vector<bool>(prob.parts.size(), false));
    }
    
    // 心态下降
    int moodDrop = 1 + playerStats.extraMoodDrop;
    if (playerStats.mental > 0) moodDrop = std::max(0, moodDrop - playerStats.mental);
    mood = std::max(0, mood - moodDrop);
    
    logEvent(config.name + "比赛正式开始！", "event");
    logEvent("进入考场，心态值-" + std::to_string(moodDrop) + "，当前心态值：" + std::to_string(mood), "event");
}

// 思考部分分
inline void thinkSubProblem(int problemIdx, int subProblemIdx) {
    if (timePoints <= 0) return;
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
    if (timePoints <= 0) return;
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
    if (!isIOIContest && requiresCodeModification[problemIdx][subProblemIdx]) {
        logEvent("需要先修改代码，才能再次对拍！", "check");
        return;
    }
    if (!isIOIContest && timePoints <= 0) return;
    if (!isIOIContest) timePoints--;
    
    pushLastAction("check");

    hasAttemptedCheck[problemIdx][subProblemIdx] = true;  // 标记已尝试对拍

    logEvent((isIOIContest ? "提交" : "对拍") + std::string(" T") + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1), "check");
    
    double errorRate = errorRates[problemIdx][subProblemIdx];
    if (Utils::randomBool(errorRate)) {
        if (isIOIContest && Utils::randomBool(0.08 * (1.0 - calculateLuckReduction()))) {
            mood = std::max(0, mood - 1);
            logEvent("服务器爆炸，心态-1", "check");
        } else {
            if (!isIOIContest) {
                requiresCodeModification[problemIdx][subProblemIdx] = true;
                modificationCount[problemIdx][subProblemIdx] = 0;
                logEvent("对拍失败！需要修改代码后才能再次对拍", "check");
            } else {
                logEvent("提交失败！", "check");
            }
        }
    } else {
        const auto& sp = subProblems[problemIdx][subProblemIdx];
        isCodeComplete[problemIdx][subProblemIdx] = true;
        logEvent((isIOIContest ? "提交" : "对拍") + std::string("成功！获得 ") + std::to_string(sp.score) + " 分", "check");
        if (sp.inspire > 0) mood = std::min(MOOD_LIMIT, mood + sp.inspire);
    }
    triggerRandomEvent(problemIdx, subProblemIdx);
}

// 修改代码
inline void modifyCodeSubProblem(int problemIdx, int subProblemIdx) {
    const SubProblem& sp = subProblems[problemIdx][subProblemIdx];
    const int requiredFixes = sp.branch + 1;

    if (!requiresCodeModification[problemIdx][subProblemIdx]) {
        logEvent("当前不需要修改代码。", "code");
        return;
    }

    int timeCost = 1;

    if (timePoints < timeCost) {
        logEvent("时间点不足，无法修改代码！", "code");
        return;
    }

    timePoints -= timeCost;
    double invalidProb = 1.0 - calculateCodeSuccessRate(sp);
    if (Utils::randomBool(invalidProb)) {
        logEvent("修改代码失败！当前进度 " +
                 std::to_string(modificationCount[problemIdx][subProblemIdx]) + " / " +
                 std::to_string(requiredFixes), "code");
    } else {
        modificationCount[problemIdx][subProblemIdx]++;
        logEvent("修改代码成功！当前进度 " +
                 std::to_string(modificationCount[problemIdx][subProblemIdx]) + " / " +
                 std::to_string(requiredFixes), "code");

        if (modificationCount[problemIdx][subProblemIdx] >= requiredFixes) {
            requiresCodeModification[problemIdx][subProblemIdx] = false;
            hasAttemptedCheck[problemIdx][subProblemIdx] = false;
            errorRates[problemIdx][subProblemIdx] = calculateErrorRate(sp);
            logEvent("已完成全部修改，可再次对拍。新的出错概率为 " +
                     std::to_string(static_cast<int>(errorRates[problemIdx][subProblemIdx] * 100)) + "%", "code");
        }
    }
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

// ========== 获取训练事件类型（GUI 与共享逻辑共用） ==========
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


#endif // GAME_HPP
