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

// ========== 全局状态（完全复制原版变量） ==========
inline PlayerStats playerStats;
inline std::string gameDifficulty = "hard";
inline int timePoints = 24;
inline int mood = 10;
inline int currentProblem = 1;
inline int totalProblems = 0;
inline std::string currentContestName = "NOIP";
inline bool debugmode = false;

// 比赛子问题统一状态
struct ContestSubProblemState
{
	int thinkProgress = 0;
	int codeProgress = 0;
	bool isCodeComplete = false;
	double errorRate = -1.0;
	int modificationCount = 0;
	bool hasAttemptedCheck = false;
	bool requiresCodeModification = false;
};

// 题目状态
inline std::vector<Problem> problems;
inline std::vector<std::vector<SubProblem>> subProblems;
inline std::vector<std::vector<ContestSubProblemState>> contestStates;

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
    if (contestStates[problemIdx][subProblemIdx].thinkProgress >= thinkTime) return 0;
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

    // 随机事件定义：条件、概率、名称、描述、效果文本、效果回调
    struct RandomEventDef
    {
        std::function<bool()> condition;
        double probability;
        std::string name;
        std::string description;
        std::string effectText;
        std::function<void()> apply;  // 触发时的效果
    };

    auto& cs = contestStates[problemIdx][subProblemIdx];
    const int thinkTime = calculateThinkTime(currentSubProblem);
    const int codeTime = calculateCodeTime(currentSubProblem);

    const RandomEventDef eventDefs[] = {
        {
            [&]() {
                return lastActions.size() >= 3 &&
                    lastActions[lastActions.size() - 1] == lastActions[lastActions.size() - 2] &&
                    lastActions[lastActions.size() - 2] == lastActions[lastActions.size() - 3];
            },
            0.04 * (1.0 - luckReduction),
            "心态爆炸", "连续失败让你感到沮丧...", "心态值-1",
            [&]() { mood = std::max(0, mood - 1); }
        },
        {
            [&]() {
                return !lastActions.empty() && lastActions.back() == "think" &&
                    cs.thinkProgress > thinkTime / 2;
            },
            0.03,
            "灵光一闪", "突然想到了一个好方法！", "心态值+1",
            [&]() { mood = std::min(MOOD_LIMIT, mood + 1); }
        },
        {
            [&]() {
                return lastNAre(lastActions, 2, "code") &&
                    cs.codeProgress > codeTime / 2;
            },
            0.03 * (1.0 - luckReduction),
            "代码bug", "写着写着发现之前的代码有问题...", "代码进度-1",
            [&]() { cs.codeProgress = std::max(0, cs.codeProgress - 1); }
        },
        {
            [&]() { return lastNAre(lastActions, 2, "code"); },
            0.02 * (1.0 - luckReduction),
            "键盘故障", "键盘突然有点不太灵了...", "心态值-1",
            [&]() { mood = std::max(0, mood - 1); }
        },
        {
            [&]() { return true; },
            0.01 * (1.0 - luckReduction),
            "监考老师巡视", "监考老师正在经过你的座位...", "心态值-1",
            [&]() { mood = std::max(0, mood - 1); }
        },
    };

    const double roll = Utils::randomDouble(0.0, 1.0);
    double accumulated = 0.0;
    for (const auto& def : eventDefs) {
        if (!def.condition()) continue;
        accumulated += def.probability;
        if (roll >= accumulated) continue;

        def.apply();

        logEvent("触发突发事件：" + def.name, "event");
        logEvent(def.description, "event");
        logEvent(def.effectText, "event");
        logEvent("当前心态值：" + std::to_string(mood), "event");
        pendingContestNotice.active = true;
        pendingContestNotice.title = "突发事件：" + def.name;
        pendingContestNotice.description = def.description;
        pendingContestNotice.effectText = def.effectText;
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
    contestStates.clear();
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
        contestStates.push_back(std::vector<ContestSubProblemState>(prob.parts.size()));
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
    auto& state = contestStates[problemIdx][subProblemIdx];
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
        state.thinkProgress++;
        logEvent("T" + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1) + " 思考成功！", "think");
        if (sp.independent == 0) {
            for (size_t i = 0; i < (size_t)subProblemIdx; i++) {
                if (subProblems[problemIdx][i].independent == 0) {
                    contestStates[problemIdx][static_cast<int>(i)].thinkProgress++;
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
    auto& state = contestStates[problemIdx][subProblemIdx];
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
        state.codeProgress++;
        logEvent("T" + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1) + " 写代码成功！", "code");
        if (state.codeProgress >= calculateCodeTime(sp)) {
            state.errorRate = calculateErrorRate(sp);
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
    const bool isIOIContest = isCurrentContestIOI();
    auto& state = contestStates[problemIdx][subProblemIdx];
    if (!isIOIContest && state.requiresCodeModification) {
        logEvent("需要先修改代码，才能再次对拍！", "check");
        return;
    }
    if (!isIOIContest && timePoints <= 0) return;
    if (!isIOIContest) timePoints--;

    pushLastAction("check");

    state.hasAttemptedCheck = true;

    logEvent((isIOIContest ? "提交" : "对拍") + std::string(" T") + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1), "check");
    
    double errorRate = state.errorRate;
    if (Utils::randomBool(errorRate)) {
        if (isIOIContest && Utils::randomBool(0.08 * (1.0 - calculateLuckReduction()))) {
            mood = std::max(0, mood - 1);
            logEvent("服务器爆炸，心态-1", "check");
        } else {
            if (!isIOIContest) {
                state.requiresCodeModification = true;
                state.modificationCount = 0;
                logEvent("对拍失败！需要修改代码后才能再次对拍", "check");
            } else {
                logEvent("提交失败！", "check");
            }
        }
    } else {
        const auto& sp = subProblems[problemIdx][subProblemIdx];
        state.isCodeComplete = true;
        logEvent((isIOIContest ? "提交" : "对拍") + std::string("成功！获得 ") + std::to_string(sp.score) + " 分", "check");
        if (sp.inspire > 0) mood = std::min(MOOD_LIMIT, mood + sp.inspire);
    }
    triggerRandomEvent(problemIdx, subProblemIdx);
}

// 修改代码
inline void modifyCodeSubProblem(int problemIdx, int subProblemIdx) {
    const SubProblem& sp = subProblems[problemIdx][subProblemIdx];
    const int requiredFixes = sp.branch + 1;
    auto& state = contestStates[problemIdx][subProblemIdx];

    if (!state.requiresCodeModification) {
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
                 std::to_string(state.modificationCount) + " / " +
                 std::to_string(requiredFixes), "code");
    } else {
        state.modificationCount++;
        logEvent("修改代码成功！当前进度 " +
                 std::to_string(state.modificationCount) + " / " +
                 std::to_string(requiredFixes), "code");

        if (state.modificationCount >= requiredFixes) {
            state.requiresCodeModification = false;
            state.hasAttemptedCheck = false;
            state.errorRate = calculateErrorRate(sp);
            logEvent("已完成全部修改，可再次对拍。新的出错概率为 " +
                     std::to_string(static_cast<int>(state.errorRate * 100)) + "%", "code");
        }
    }
}

inline bool isFullScore() {
    for (int i = 0; i < totalProblems; i++) {
        int lastIdx = (int)subProblems[i].size() - 1;
        if (!contestStates[i][lastIdx].isCodeComplete) return false;
    }
    return true;
}

inline int calculateScore() {
    int total = 0;
    for (int i = 0; i < totalProblems; i++) {
        int maxScore = 0;
        for (size_t j = 0; j < subProblems[i].size(); j++) {
            if (contestStates[i][j].isCodeComplete) maxScore = std::max(maxScore, subProblems[i][j].score);
        }
        total += maxScore;
    }
    return total;
}

// 评奖
inline std::string calculateAward(int score, const std::string& contestType) {
    double mult = difficultyMultiplier();
    std::string award;

    // 分数阈值层：从高到低，首个满足即为对应奖项
    struct Threshold
    {
        int scoreBase;           // 基础分数线
        const char* awardName;   // 对应奖项名
        std::function<void()> onAward;  // 获奖时的副作用（设置 flag 等）
    };

    // 比赛评奖规则：contestType → (是否使用 tempScore, 分数阈值数组)
    // 使用 tempScore 的比赛：省选、NOI、CTT、CTS、IOI
    struct AwardRule
    {
        std::string type;       // contestType 匹配值
        bool useTempScore;      // true 则用 playerStats.tempScore 而非传入的 score
        std::vector<Threshold> thresholds;  // 分数阈值（从高到低）
        std::function<void()> preCheck;     // 评奖前的预处理（如重置 flag）
        std::function<void(const std::string&)> postCheck;  // 评奖后的额外处理
    };

    static const AwardRule rules[] = {
        {
            "CSP-S", false,
            {{270, "一等奖", nullptr}, {180, "二等奖", nullptr}, {50, "三等奖", nullptr}, {0, "没有获奖", nullptr}},
            nullptr, nullptr
        },
        {
            "NOIP", false,
            {{270, "一等奖", nullptr}, {180, "二等奖", nullptr}, {50, "三等奖", nullptr}, {0, "没有获奖", nullptr}},
            nullptr, nullptr
        },
        {
            "WC", false,
            {{220, "金牌", nullptr}, {160, "银牌", nullptr}, {100, "铜牌", nullptr}, {0, "铁牌", nullptr}},
            nullptr, nullptr
        },
        {
            "APIO", false,
            {{220, "金牌", nullptr}, {160, "银牌", nullptr}, {100, "铜牌", nullptr}, {0, "铁牌", nullptr}},
            nullptr, nullptr
        },
        {
            "省选", true,
            {{700, "省队A队", []() { playerStats.isProvincialTeamA = true; }},
             {600, "省队B队", nullptr}, {0, "没有进队", nullptr}},
            []() { playerStats.isProvincialTeamA = false; },  // 预处理：重置 A 队 flag
            [](const std::string& aw) { playerStats.isProvincialTeam = (aw.find("省队") != std::string::npos); }
        },
        {
            "NOI", true,
            {{400, "金牌", []() { playerStats.isTrainingTeam = true; }},
             {300, "银牌", nullptr}, {200, "铜牌", nullptr}, {0, "铁牌", nullptr}},
            nullptr, nullptr
        },
        {
            "CTT", true,
            {{600, "入选候选队", []() { playerStats.isCandidateTeam = true; }},
             {0, "没有入选候选队", nullptr}},
            nullptr, nullptr
        },
        {
            "CTS", true,
            {{900, "入选国家队", []() { playerStats.isNationalTeam = true; }},
             {0, "没有入选国家队", nullptr}},
            nullptr, nullptr
        },
        {
            "IOI", true,
            {{400, "金牌", []() { playerStats.isIOIgold = true; }},
             {300, "银牌", nullptr}, {200, "铜牌", nullptr}, {0, "铁牌", nullptr}},
            nullptr, nullptr
        },
    };

    for (const auto& rule : rules)
    {
        if (contestType != rule.type) continue;

        int evalScore = rule.useTempScore ? playerStats.tempScore : score;

        // 预处理
        if (rule.preCheck) rule.preCheck();

        // 从高到低匹配阈值
        for (const auto& th : rule.thresholds)
        {
            if (evalScore >= static_cast<int>(th.scoreBase * mult))
            {
                award = th.awardName;
                if (th.onAward) th.onAward();
                break;
            }
        }

        // 后处理
        if (rule.postCheck) rule.postCheck(award);

        // 记录成就
        playerStats.achievements.push_back(contestType + "：" + std::to_string(evalScore) + "分，" + award);
        break;
    }

    if (isTopAwardForExperience(contestType, award)) {
        addTempExperience(1, contestType + "最高奖项奖励");
    }

    return award;
}

// ========== 获取训练事件类型（GUI 与共享逻辑共用） ==========
inline std::string getTrainingEventType(int currentEvent, int /*totalEvents*/) {
    // 表驱动：phase → 事件序列模板，每个元素为固定字符串或 "提升/比赛" 随机选择
    struct EventEntry
    {
        std::string fixed;  // 非空则直接返回
        bool isRandom;      // 为 true 时从 {"提升训练", "比赛训练"} 随机
    };

    using EventSeq = std::vector<EventEntry>;
    static const std::vector<int> phases_5events = {19, 31, 38};
    static const std::vector<int> phases_4events = {3, 5, 7, 11, 13, 26, 29, 35, 50};
    static const std::vector<int> phases_2events = {9, 15, 21, 33, 40, 45};
    static const std::vector<int> phases_6events = {42, 53};

    static const EventSeq seq_phase1 = {
        {"长期训练", false}, {"", true}, {"娱乐时间", false}, {"", true}, {"赛前一天", false}};
    static const EventSeq seq_phase17 = {
        {"步入高二", false}, {"长期训练", false}, {"", true}, {"", true},
        {"娱乐时间", false}, {"", true}, {"焦虑", false}, {"赛前一天", false}};
    static const EventSeq seq_5events = {
        {"", true}, {"娱乐时间", false}, {"焦虑", false}, {"遗忘", false}, {"赛前一天", false}};
    static const EventSeq seq_4events = {
        {"", true}, {"娱乐时间", false}, {"焦虑", false}, {"赛前一天", false}};
    static const EventSeq seq_2events = {
        {"焦虑", false}, {"赛前一天", false}};
    static const EventSeq seq_6events = {
        {"", true}, {"娱乐时间", false}, {"", true}, {"娱乐时间", false}, {"焦虑", false}, {"赛前一天", false}};

    const EventSeq* seq = nullptr;
    if (currentPhase == 1) seq = &seq_phase1;
    else if (currentPhase == 17) seq = &seq_phase17;
    else if (std::find(phases_5events.begin(), phases_5events.end(), currentPhase) != phases_5events.end()) seq = &seq_5events;
    else if (std::find(phases_4events.begin(), phases_4events.end(), currentPhase) != phases_4events.end()) seq = &seq_4events;
    else if (std::find(phases_2events.begin(), phases_2events.end(), currentPhase) != phases_2events.end()) seq = &seq_2events;
    else if (std::find(phases_6events.begin(), phases_6events.end(), currentPhase) != phases_6events.end()) seq = &seq_6events;

    if (seq != nullptr && currentEvent >= 1 && currentEvent <= static_cast<int>(seq->size()))
    {
        const auto& entry = (*seq)[currentEvent - 1];
        if (entry.isRandom)
        {
            return Utils::randomBool(0.5) ? "提升训练" : "比赛训练";
        }
        return entry.fixed;
    }

    return "提升训练";
}

// ========== 游戏初始化和主流程 ==========

inline void initGame() {
    // 差异初始化：题库初始化 + 难度相关设定
    initProblemPool();
    auto settings = DIFFICULTY_SETTINGS.at(gameDifficulty);
    playerStats.determination = settings.initialDetermination;
    playerStats.extraMoodDrop = (gameDifficulty == "expert") ? 2 : (gameDifficulty == "easy") ? 0 : 1;
    currentShopPrices = INITIAL_SHOP_PRICES.at(gameDifficulty);
}


#endif // GAME_HPP
