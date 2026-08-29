#ifndef CONTEST_HPP
#define CONTEST_HPP

#include "game.hpp"
#include "topics.hpp"
#include "social.hpp"
#include <cstdint>

namespace Contest {

// ========== 结果类型（从 main.cpp 移入） ==========

struct ContestProblemResult {
    std::string name;
    int expectedScore = 0;
    int actualScore = 0;
};

struct ContestResultView {
    std::string contestName;
    std::vector<ContestProblemResult> problems;
    int expectedTotal = 0;
    int actualTotal = 0;
    int prizeMoney = 0;   // 比赛奖金（金钱）
    std::string aggregateLabel;
    int aggregateValue = 0;
    bool hasAggregate = false;
    std::string award;
    bool hasAward = false;
};

// ========== 数据化评奖系统 ==========

enum class AwardEffect : uint8_t {
    None,
    SetProvincialTeamA,
    SetTrainingTeam,
    SetCandidateTeam,
    SetNationalTeam,
    SetIOIgold
};

enum class AwardPreCheck : uint8_t {
    None,
    ResetProvincialTeamA
};

enum class AwardPostCheck : uint8_t {
    None,
    SetProvincialTeamIfContainsTeam
};

struct Threshold {
    int scoreBase;
    const char* awardName;
    AwardEffect effect;
};

struct AwardRule {
    const char* type;
    bool useTempScore;
    AwardPreCheck preCheck;
    const Threshold* thresholds;
    int numThresholds;
    AwardPostCheck postCheck;
};

inline void applyAwardEffect(AwardEffect e) {
    switch (e) {
    case AwardEffect::SetProvincialTeamA: gameState.playerStats.isProvincialTeamA = true; break;
    case AwardEffect::SetTrainingTeam:
        gameState.playerStats.isTrainingTeam = true;
        if (!gameState.playerStats.isBaosong) {
            gameState.playerStats.isBaosong = true;   // 保送锁定（结局矩阵）
            logEvent("入选国家集训队——保送资格到手！文化课压力解除，高三 6 月将迎来庆功月。", "event");
            gameState.playerStats.achievements.push_back("保送资格");
        }
        break;
    case AwardEffect::SetCandidateTeam: gameState.playerStats.isCandidateTeam = true; break;
    case AwardEffect::SetNationalTeam: gameState.playerStats.isNationalTeam = true; break;
    case AwardEffect::SetIOIgold: gameState.playerStats.isIOIgold = true; break;
    default: break;
    }
}

inline void applyAwardPreCheck(AwardPreCheck pc) {
    if (pc == AwardPreCheck::ResetProvincialTeamA)
        gameState.playerStats.isProvincialTeamA = false;
}

inline void applyAwardPostCheck(AwardPostCheck pc, const std::string& award) {
    if (pc == AwardPostCheck::SetProvincialTeamIfContainsTeam)
        gameState.playerStats.isProvincialTeam = (award.find("省队") != std::string::npos);
}

// ========== 数据化聚合系统 ==========

enum class ScoreComponent : uint8_t {
    CurrentScore,
    PrevScore,
    NoipScore,
    PrevScore1,
    PrevScore2,
    PrevScore3,
    CttScore,
    ProvincialTeamABonus
};

enum class AggregateSaveTo : uint8_t { None, CttScore };

struct AggregateRule {
    const char* contestDay;
    const char* label;
    ScoreComponent components[5];
    int numComponents;
    AggregateSaveTo saveTo;
};

inline int getScoreComponent(ScoreComponent c, int currentScore) {
    switch (c) {
    case ScoreComponent::CurrentScore: return currentScore;
    case ScoreComponent::PrevScore: return gameState.playerStats.prevScore;
    case ScoreComponent::NoipScore: return gameState.playerStats.noipScore;
    case ScoreComponent::PrevScore1: return gameState.playerStats.prevScore1;
    case ScoreComponent::PrevScore2: return gameState.playerStats.prevScore2;
    case ScoreComponent::PrevScore3: return gameState.playerStats.prevScore3;
    case ScoreComponent::CttScore: return gameState.playerStats.cttScore;
    case ScoreComponent::ProvincialTeamABonus: return gameState.playerStats.isProvincialTeamA ? 5 : 0;
    }
    return 0;
}

// ========== 计算函数（含专题「精通」加成） ==========

// 子问题的主知识维度 → 专题 id 映射（dp/ds/str/graph/comb/adhoc）
inline int dominantKnowledgeTopic(const SubProblem& sp) {
    struct KV { int v; int topic; };
    KV kvs[] = {{sp.dp,0},{sp.ds,1},{sp.str,2},{sp.graph,3},{sp.comb,4},{sp.adhoc,8}};
    KV* best = nullptr;
    for (auto& kv : kvs) if (!best || kv.v > best->v) best = &kv;
    return (best && best->v > 0) ? best->topic : -1;
}

inline int calculateThinkTime(const SubProblem& sp) {
    if (gameState.debugmode) return 1;
    int thinkTime = 1;
    thinkTime += std::max(0, sp.dp - Utils::mapAttributeValue(gameState.playerStats.dp));
    thinkTime += std::max(0, sp.ds - Utils::mapAttributeValue(gameState.playerStats.ds));
    thinkTime += std::max(0, sp.str - Utils::mapAttributeValue(gameState.playerStats.string));
    thinkTime += std::max(0, sp.graph - Utils::mapAttributeValue(gameState.playerStats.graph));
    thinkTime += std::max(0, sp.comb - Utils::mapAttributeValue(gameState.playerStats.combinatorics));
    thinkTime += sp.adhoc;
    // 精通加成：主维度已精通 → 思考时间 -1（下限 1）
    const int domTopic = dominantKnowledgeTopic(sp);
    if (domTopic >= 0 && Topics::hasMastery(domTopic)) thinkTime = std::max(1, thinkTime - 1);
    return thinkTime;
}

inline int calculateCodeTime(const SubProblem& sp) {
    if (gameState.debugmode) return 1;
    int codeTime = sp.coding;
    int effQuick = gameState.playerStats.quickness
        + (Topics::hasMastery(9) ? 1 : 0);   // 手速特训精通：迅捷有效 +1
    if (effQuick > 0) {
        codeTime = std::max(1, codeTime - effQuick);
    }
    return codeTime;
}

inline double calculateThinkSuccessRate(const SubProblem& sp) {
    if (gameState.debugmode) return 1.0;
    double baseProb = 1.0;
    baseProb -= std::max(0, sp.thinking - Utils::mapAttributeValue(gameState.playerStats.thinking)) * 0.05;
    baseProb -= std::pow(std::max(10 - gameState.mood, 0), 2) * 0.01;
    return std::max(0.3, std::min(0.95, baseProb));
}

inline double calculateCodeSuccessRate(const SubProblem& sp) {
    if (gameState.debugmode) return 1.0;
    double baseProb = 1.0;
    baseProb -= std::pow(std::max(10 - gameState.mood, 0), 2) * 0.01;
    baseProb -= std::max(0, sp.detail - Utils::mapAttributeValue(gameState.playerStats.coding)) * 0.05;
    return std::max(0.4, std::min(0.95, baseProb));
}

inline double calculateLuckReduction() {
    if (gameState.playerStats.luck <= 0) return 0.0;
    return std::min(0.45, std::log2(gameState.playerStats.luck + 1.0) * 0.095);
}

inline double calculateErrorRate(const SubProblem& sp) {
    if (gameState.debugmode) return 0.0;
    double baseProb = 0.1;
    baseProb += sp.trap * 0.05;
    baseProb -= gameState.playerStats.carefulness * 0.03;
    baseProb += std::pow(std::max(10 - gameState.mood, 0), 2) * 0.01;
    if (Topics::hasMastery(10)) baseProb *= 0.9;  // 细心打磨精通：错误率 ×0.9
    baseProb *= (1.0 - calculateLuckReduction());
    return std::max(0.0, std::min(0.8, baseProb));
}

// ========== 显示辅助函数 ==========

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
    return std::max(0, getBaseBlurLevel(problem, sp) - gameState.playerStats.experience);
}

inline std::string getBlurTraitText(const Problem& problem, const SubProblem& sp) {
    const int baseBlur = getBaseBlurLevel(problem, sp);
    const int effectiveBlur = getEffectiveBlurLevel(problem, sp);
    if (sp.blur <= 0 || effectiveBlur <= 0) return "";
    return "模糊:" + std::to_string(baseBlur) + "->" + std::to_string(effectiveBlur);
}

inline int getActiveBlurLevel(int problemIdx, int subProblemIdx) {
    if (problemIdx < 0 || problemIdx >= static_cast<int>(gameState.problems.size())) return 0;
    if (problemIdx >= static_cast<int>(gameState.subProblems.size())) return 0;
    if (subProblemIdx < 0 || subProblemIdx >= static_cast<int>(gameState.subProblems[problemIdx].size())) return 0;

    const auto& sp = gameState.subProblems[problemIdx][subProblemIdx];
    const int thinkTime = calculateThinkTime(sp);
    if (gameState.contestStates[problemIdx][subProblemIdx].thinkProgress >= thinkTime) return 0;
    return getEffectiveBlurLevel(gameState.problems[problemIdx], sp);
}

inline std::string getThinkTimeDisplayTotal(int problemIdx, int subProblemIdx) {
    if (problemIdx < 0 || problemIdx >= static_cast<int>(gameState.subProblems.size())) return "?";
    if (subProblemIdx < 0 || subProblemIdx >= static_cast<int>(gameState.subProblems[problemIdx].size())) return "?";

    const auto& sp = gameState.subProblems[problemIdx][subProblemIdx];
    if (getActiveBlurLevel(problemIdx, subProblemIdx) > 0) return "?";
    return std::to_string(calculateThinkTime(sp));
}

inline std::string buildSubProblemRequirementText(int problemIdx, int subProblemIdx) {
    if (problemIdx < 0 || problemIdx >= static_cast<int>(gameState.subProblems.size())) return "无显式要求";
    if (subProblemIdx < 0 || subProblemIdx >= static_cast<int>(gameState.subProblems[problemIdx].size())) return "无显式要求";

    const auto& problem = gameState.problems[problemIdx];
    const auto& sp = gameState.subProblems[problemIdx][subProblemIdx];
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

// ========== 配置辅助 ==========

inline int getContestIdByName(const std::string& name) {
    for (const auto& [id, config] : CONTEST_CONFIGS) {
        if (config.name == name) return id;
    }
    return -1;
}

inline const ContestConfig* getCurrentConfig() {
    auto it = CONTEST_CONFIGS.find(getContestIdByName(gameState.currentContestName));
    return (it != CONTEST_CONFIGS.end()) ? &it->second : nullptr;
}

inline bool isIOI() {
    auto* cfg = getCurrentConfig();
    return cfg && cfg->isIOI;
}

// ========== 随机事件 ==========

inline void triggerRandomEvent(int problemIdx, int subProblemIdx) {
    if (gameState.timePoints <= 0) return;

    const auto& currentSubProblem = gameState.subProblems[problemIdx][subProblemIdx];
    double luckReduction = calculateLuckReduction();

    auto lastNAre = [](const std::vector<std::string>& actions, int n, const std::string& value) {
        if (static_cast<int>(actions.size()) < n) return false;
        for (int i = static_cast<int>(actions.size()) - n; i < static_cast<int>(actions.size()); ++i) {
            if (actions[i] != value) return false;
        }
        return true;
    };

    struct RandomEventDef {
        std::function<bool()> condition;
        double probability;
        std::string name;
        std::string description;
        std::string effectText;
        std::function<void()> apply;
    };

    auto& cs = gameState.contestStates[problemIdx][subProblemIdx];
    const int thinkTime = calculateThinkTime(currentSubProblem);
    const int codeTime = calculateCodeTime(currentSubProblem);

    const RandomEventDef eventDefs[] = {
        {
            [&]() {
                return gameState.lastActions.size() >= 3 &&
                    gameState.lastActions[gameState.lastActions.size() - 1] == gameState.lastActions[gameState.lastActions.size() - 2] &&
                    gameState.lastActions[gameState.lastActions.size() - 2] == gameState.lastActions[gameState.lastActions.size() - 3];
            },
            0.04 * (1.0 - luckReduction),
            "心态爆炸", "连续失败让你感到沮丧...", "心态值-1",
            [&]() { applyStatDelta("mood", -1, "心态爆炸"); }
        },
        {
            [&]() {
                return !gameState.lastActions.empty() && gameState.lastActions.back() == "think" &&
                    cs.thinkProgress > thinkTime / 2;
            },
            0.03 + (Social::hasFriendPerk(SocialPerk::Inspiration) ? 0.01 : 0.0),
            "灵光一闪", "突然想到了一个好方法！", "心态值+1",
            [&]() { applyStatDelta("mood", 1, "灵光一闪"); }
        },
        {
            [&]() {
                return lastNAre(gameState.lastActions, 2, "code") &&
                    cs.codeProgress > codeTime / 2;
            },
            0.03 * (1.0 - luckReduction) * (Social::hasFriendPerk(SocialPerk::CodeReview) ? 0.7 : 1.0),
            "代码bug", "写着写着发现之前的代码有问题...", "代码进度-1",
            [&]() { cs.codeProgress = std::max(0, cs.codeProgress - 1); }
        },
        {
            [&]() { return lastNAre(gameState.lastActions, 2, "code"); },
            0.02 * (1.0 - luckReduction),
            "键盘故障", "键盘突然有点不太灵了...", "心态值-1",
            [&]() { applyStatDelta("mood", -1, "键盘故障"); }
        },
        {
            [&]() { return true; },
            0.01 * (1.0 - luckReduction),
            "监考老师巡视", "监考老师正在经过你的座位...", "心态值-1",
            [&]() { applyStatDelta("mood", -1, "监考老师巡视"); }
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
        logEvent("当前心态值：" + std::to_string(gameState.mood), "event");
        gameState.pendingContestNotice.active = true;
        gameState.pendingContestNotice.title = "突发事件：" + def.name;
        gameState.pendingContestNotice.description = def.description;
        gameState.pendingContestNotice.effectText = def.effectText;
        return;
    }
}

// ========== 核心比赛函数 ==========

// 唯一入口：按配置开始一场比赛
inline void start(const ContestConfig& config) {
    gameState.currentContestName = config.name;
    gameState.timePoints = config.timePoints;
    gameState.currentProblem = 1;
    gameState.problems.clear();
    gameState.subProblems.clear();
    gameState.contestStates.clear();
    clearPendingContestNotice();

    gameState.totalProblems = (int)config.problemRanges.size();

    while (true) {
        gameState.problems.clear();
        bool unique = true;
        for (int i = 0; i < gameState.totalProblems; i++) {
            Problem p = selectProblemFromRange(config.problemRanges[i].first, config.problemRanges[i].second);
            for (const auto& existing : gameState.problems) {
                if (existing.name == p.name) { unique = false; break; }
            }
            if (!unique) break;
            gameState.problems.push_back(p);
        }
        if (unique) break;
    }

    for (const auto& prob : gameState.problems) {
        gameState.subProblems.push_back(prob.parts);
        gameState.contestStates.push_back(std::vector<ContestSubProblemState>(prob.parts.size()));
    }

    int moodDrop = 1 + gameState.playerStats.extraMoodDrop;
    if (gameState.playerStats.mental > 0) moodDrop = std::max(0, moodDrop - gameState.playerStats.mental);
    // 特质「考场杀手」：比赛入场心态降幅 -1
    if (playerHasTrait("contest_beast")) moodDrop = std::max(0, moodDrop - 1);
    applyStatDelta("mood", -moodDrop, "入场紧张");

    logEvent(config.name + "比赛正式开始！", "event");
    logEvent("进入考场，心态值-" + std::to_string(moodDrop) + "，当前心态值：" + std::to_string(gameState.mood), "event");
}

inline void start(int contestId) {
    start(CONTEST_CONFIGS.at(contestId));
}

inline void think(int problemIdx, int subProblemIdx) {
    if (gameState.timePoints <= 0) return;
    const auto& sp = gameState.subProblems[problemIdx][subProblemIdx];
    auto& state = gameState.contestStates[problemIdx][subProblemIdx];
    double invalidProb = 1.0 - calculateThinkSuccessRate(sp);
    gameState.timePoints--;

    if (Utils::randomBool(invalidProb)) {
        logEvent("T" + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1) + " 思考无效！", "think");
        if (sp.heat > 0) {
            int moodDrop = Utils::randomInt(0, sp.heat);
            if (gameState.playerStats.mental > 0) moodDrop = std::max(0, moodDrop - gameState.playerStats.mental);
            applyStatDelta("mood", -moodDrop, "红温效应");
            if (moodDrop > 0) logEvent("红温效应，心态-" + std::to_string(moodDrop), "think");
        }
    } else {
        state.thinkProgress++;
        logEvent("T" + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1) + " 思考成功！", "think");
        if (sp.independent == 0) {
            for (size_t i = 0; i < (size_t)subProblemIdx; i++) {
                if (gameState.subProblems[problemIdx][i].independent == 0) {
                    gameState.contestStates[problemIdx][static_cast<int>(i)].thinkProgress++;
                    logEvent("T" + std::to_string(problemIdx+1) + " 部分分" + std::to_string(i+1) + " 非独立+1", "think");
                }
            }
        }
    }
    pushLastAction("think");
    triggerRandomEvent(problemIdx, subProblemIdx);
}

inline void code(int problemIdx, int subProblemIdx) {
    if (gameState.timePoints <= 0) return;
    const auto& sp = gameState.subProblems[problemIdx][subProblemIdx];
    auto& state = gameState.contestStates[problemIdx][subProblemIdx];
    double invalidProb = 1.0 - calculateCodeSuccessRate(sp);
    gameState.timePoints--;

    if (Utils::randomBool(invalidProb)) {
        logEvent("T" + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1) + " 写代码无效！", "code");
        if (sp.heat > 0) {
            int moodDrop = sp.heat;
            if (gameState.playerStats.mental > 0) moodDrop = std::max(0, moodDrop - gameState.playerStats.mental);
            applyStatDelta("mood", -moodDrop, "红温效应");
            logEvent("红温效应，心态-" + std::to_string(moodDrop), "code");
        }
    } else {
        state.codeProgress++;
        logEvent("T" + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1) + " 写代码成功！", "code");
        if (state.codeProgress >= calculateCodeTime(sp)) {
            state.errorRate = calculateErrorRate(sp);
            if (sp.inspire > 0) {
                applyStatDelta("mood", sp.inspire, "激励效果");
                logEvent("激励效果，心态+" + std::to_string(sp.inspire), "code");
            }
        }
    }
    pushLastAction("code");
    triggerRandomEvent(problemIdx, subProblemIdx);
}

inline void check(int problemIdx, int subProblemIdx) {
    const bool isIOIContest = isIOI();
    auto& state = gameState.contestStates[problemIdx][subProblemIdx];
    if (!isIOIContest && state.requiresCodeModification) {
        logEvent("需要先修改代码，才能再次对拍！", "check");
        return;
    }
    if (!isIOIContest && gameState.timePoints <= 0) return;
    if (!isIOIContest) gameState.timePoints--;

    pushLastAction("check");

    state.hasAttemptedCheck = true;

    logEvent((isIOIContest ? "提交" : "对拍") + std::string(" T") + std::to_string(problemIdx+1) + " 部分分" + std::to_string(subProblemIdx+1), "check");

    double errorRate = state.errorRate;
    if (Utils::randomBool(errorRate)) {
        if (isIOIContest && Utils::randomBool(0.08 * (1.0 - calculateLuckReduction()))) {
            applyStatDelta("mood", -1, "服务器爆炸");
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
        const auto& sp = gameState.subProblems[problemIdx][subProblemIdx];
        state.isCodeComplete = true;
        logEvent((isIOIContest ? "提交" : "对拍") + std::string("成功！获得 ") + std::to_string(sp.score) + " 分", "check");
        gameState.carefulChecks++;   // 累计成功对拍/提交（细心专题进度）
        Topics::onCheckSuccess();
        if (sp.inspire > 0) applyStatDelta("mood", sp.inspire, "提交成功激励");
    }
    triggerRandomEvent(problemIdx, subProblemIdx);
}

inline void modify(int problemIdx, int subProblemIdx) {
    const SubProblem& sp = gameState.subProblems[problemIdx][subProblemIdx];
    // 错题本精通（伙伴挚友效果）：修改所需次数 -1，下限 1
    const int requiredFixes = std::max(1, sp.branch + 1 -
        (Social::hasFriendPerk(SocialPerk::Notebook) ? 1 : 0));
    auto& state = gameState.contestStates[problemIdx][subProblemIdx];

    if (!state.requiresCodeModification) {
        logEvent("当前不需要修改代码。", "code");
        return;
    }

    int timeCost = 1;

    if (gameState.timePoints < timeCost) {
        logEvent("时间点不足，无法修改代码！", "code");
        return;
    }

    gameState.timePoints -= timeCost;
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

// ========== 评分与评奖 ==========

inline bool isFullScore() {
    for (int i = 0; i < gameState.totalProblems; i++) {
        int lastIdx = (int)gameState.subProblems[i].size() - 1;
        if (!gameState.contestStates[i][lastIdx].isCodeComplete) return false;
    }
    return true;
}

inline int calculateScore() {
    int total = 0;
    for (int i = 0; i < gameState.totalProblems; i++) {
        int maxScore = 0;
        for (size_t j = 0; j < gameState.subProblems[i].size(); j++) {
            if (gameState.contestStates[i][j].isCodeComplete)
                maxScore = std::max(maxScore, gameState.subProblems[i][j].score);
        }
        total += maxScore;
    }
    return total;
}

inline std::string calculateAward(int score, const std::string& contestType) {
    static const Threshold cspThresholds[] = {
        {270, "一等奖", AwardEffect::None}, {180, "二等奖", AwardEffect::None},
        {50, "三等奖", AwardEffect::None}, {0, "没有获奖", AwardEffect::None}
    };
    static const Threshold noipThresholds[] = {
        {270, "一等奖", AwardEffect::None}, {180, "二等奖", AwardEffect::None},
        {50, "三等奖", AwardEffect::None}, {0, "没有获奖", AwardEffect::None}
    };
    static const Threshold wcThresholds[] = {
        {220, "金牌", AwardEffect::None}, {160, "银牌", AwardEffect::None},
        {100, "铜牌", AwardEffect::None}, {0, "铁牌", AwardEffect::None}
    };
    static const Threshold apioThresholds[] = {
        {220, "金牌", AwardEffect::None}, {160, "银牌", AwardEffect::None},
        {100, "铜牌", AwardEffect::None}, {0, "铁牌", AwardEffect::None}
    };
    static const Threshold provThresholds[] = {
        {700, "省队A队", AwardEffect::SetProvincialTeamA},
        {600, "省队B队", AwardEffect::None}, {0, "没有进队", AwardEffect::None}
    };
    static const Threshold noiThresholds[] = {
        {400, "金牌", AwardEffect::SetTrainingTeam},
        {300, "银牌", AwardEffect::None}, {200, "铜牌", AwardEffect::None},
        {0, "铁牌", AwardEffect::None}
    };
    static const Threshold cttThresholds[] = {
        {600, "入选候选队", AwardEffect::SetCandidateTeam},
        {0, "没有入选候选队", AwardEffect::None}
    };
    static const Threshold ctsThresholds[] = {
        {900, "入选国家队", AwardEffect::SetNationalTeam},
        {0, "没有入选国家队", AwardEffect::None}
    };
    static const Threshold ioiThresholds[] = {
        {400, "金牌", AwardEffect::SetIOIgold},
        {300, "银牌", AwardEffect::None}, {200, "铜牌", AwardEffect::None},
        {0, "铁牌", AwardEffect::None}
    };

    static const AwardRule rules[] = {
        {"CSP-S", false, AwardPreCheck::None, cspThresholds, 4, AwardPostCheck::None},
        {"NOIP",  false, AwardPreCheck::None, noipThresholds, 4, AwardPostCheck::None},
        {"WC",    false, AwardPreCheck::None, wcThresholds, 4, AwardPostCheck::None},
        {"APIO",  false, AwardPreCheck::None, apioThresholds, 4, AwardPostCheck::None},
        {"省选",  true,  AwardPreCheck::ResetProvincialTeamA, provThresholds, 3, AwardPostCheck::SetProvincialTeamIfContainsTeam},
        {"NOI",   true,  AwardPreCheck::None, noiThresholds, 4, AwardPostCheck::None},
        {"CTT",   true,  AwardPreCheck::None, cttThresholds, 2, AwardPostCheck::None},
        {"CTS",   true,  AwardPreCheck::None, ctsThresholds, 2, AwardPostCheck::None},
        {"IOI",   true,  AwardPreCheck::None, ioiThresholds, 4, AwardPostCheck::None},
    };

    const double mult = difficultyMultiplier();
    std::string award;

    for (const auto& rule : rules) {
        if (contestType != rule.type) continue;

        int evalScore = rule.useTempScore ? gameState.playerStats.tempScore : score;

        applyAwardPreCheck(rule.preCheck);

        for (int i = 0; i < rule.numThresholds; ++i) {
            if (evalScore >= static_cast<int>(rule.thresholds[i].scoreBase * mult)) {
                award = rule.thresholds[i].awardName;
                applyAwardEffect(rule.thresholds[i].effect);
                break;
            }
        }

        applyAwardPostCheck(rule.postCheck, award);

        gameState.playerStats.achievements.push_back(contestType + "：" + std::to_string(evalScore) + "分，" + award);
        break;
    }

    if (isTopAwardForExperience(contestType, award)) {
        addTempExperience(1, contestType + "最高奖项奖励");
    }

    return award;
}

// ========== 比赛结束（从 main.cpp 提取） ==========

inline ContestResultView finalize() {
    ContestResultView result;
    result.contestName = gameState.currentContestName;

    const bool isIOIContest = isIOI();
    int totalExpectedScore = 0;
    int totalActualScore = 0;

    for (int i = 0; i < gameState.totalProblems; ++i) {
        int problemExpectedScore = 0;
        int problemActualScore = 0;

        for (size_t j = 0; j < gameState.subProblems[i].size(); ++j) {
            const auto& sp = gameState.subProblems[i][j];
            const auto& cs = gameState.contestStates[i][j];
            const bool codeCompleted = cs.codeProgress >= calculateCodeTime(sp);
            const bool checkCompleted = cs.isCodeComplete;

            if (codeCompleted) {
                problemExpectedScore = std::max(problemExpectedScore, sp.score);
            }

            if (checkCompleted) {
                problemActualScore = std::max(problemActualScore, sp.score);
            } else if (codeCompleted && !isIOIContest) {
                const double successRate = 1.0 - cs.errorRate;
                for (int k = static_cast<int>(j); k >= 0; --k) {
                    if (Utils::randomBool(successRate)) {
                        problemActualScore = std::max(problemActualScore, gameState.subProblems[i][k].score);
                        break;
                    }
                }
            }
        }

        totalExpectedScore += problemExpectedScore;
        totalActualScore += problemActualScore;
        result.problems.push_back({gameState.problems[i].name, problemExpectedScore, problemActualScore});
    }

    result.expectedTotal = totalExpectedScore;
    result.actualTotal = totalActualScore;

    // 数据化多日比赛聚合
    static const AggregateRule aggregateRules[] = {
        {"省选Day2", "省选总分",
         {ScoreComponent::CurrentScore, ScoreComponent::PrevScore, ScoreComponent::NoipScore}, 3,
         AggregateSaveTo::None},
        {"NOI Day2", "NOI总分",
         {ScoreComponent::CurrentScore, ScoreComponent::PrevScore, ScoreComponent::ProvincialTeamABonus}, 3,
         AggregateSaveTo::None},
        {"IOI Day2", "IOI总分",
         {ScoreComponent::CurrentScore, ScoreComponent::PrevScore}, 2,
         AggregateSaveTo::None},
        {"CTT Day4", "CTT总分",
         {ScoreComponent::CurrentScore, ScoreComponent::PrevScore1, ScoreComponent::PrevScore2, ScoreComponent::PrevScore3}, 4,
         AggregateSaveTo::CttScore},
        {"CTS Day2", "CTS总分",
         {ScoreComponent::CurrentScore, ScoreComponent::PrevScore, ScoreComponent::CttScore}, 3,
         AggregateSaveTo::None},
    };

    for (const auto& rule : aggregateRules) {
        if (gameState.currentContestName != rule.contestDay) continue;
        int aggregate = 0;
        for (int i = 0; i < rule.numComponents; ++i)
            aggregate += getScoreComponent(rule.components[i], totalActualScore);
        gameState.playerStats.tempScore = aggregate;
        if (rule.saveTo == AggregateSaveTo::CttScore)
            gameState.playerStats.cttScore = aggregate;
        result.aggregateLabel = rule.label;
        result.aggregateValue = aggregate;
        result.hasAggregate = true;
        break;
    }

    // 比赛奖金（金钱，按实际得分发放）
    result.prizeMoney = totalActualScore * 5;

    const int minMood = std::min(5 + gameState.playerStats.mental, 10);
    if (gameState.mood < minMood) {
        const int recovery = minMood - gameState.mood;
        gameState.mood = std::min(gameState.moodCap, minMood);
        logEvent("比赛结束后心态自动恢复：+" + std::to_string(recovery) +
                     "，当前心态值：" + std::to_string(gameState.mood), "event");
    }

    const bool isFinalDayContest =
        gameState.currentContestName != "省选Day1" &&
        gameState.currentContestName != "NOI Day1" &&
        gameState.currentContestName != "IOI Day1" &&
        gameState.currentContestName != "CTT Day1" &&
        gameState.currentContestName != "CTT Day2" &&
        gameState.currentContestName != "CTT Day3" &&
        gameState.currentContestName != "CTS Day1";

    if (isFinalDayContest) {
        static const std::pair<const char*, const char*> awardTypeMap[] = {
            {"省选Day2", "省选"}, {"NOI Day2", "NOI"}, {"IOI Day2", "IOI"},
            {"CTT Day4", "CTT"}, {"CTS Day2", "CTS"},
        };

        std::string awardType = gameState.currentContestName;
        for (const auto& [day, type] : awardTypeMap) {
            if (gameState.currentContestName == day) { awardType = type; break; }
        }

        result.award = calculateAward(totalActualScore, awardType);
        result.hasAward = !result.award.empty();

        if (gameState.currentContestName == "CSP-S") gameState.playerStats.cspScore = totalActualScore;
        else if (gameState.currentContestName == "NOIP") gameState.playerStats.noipScore = totalActualScore;
    } else {
        gameState.playerStats.prevScore = totalActualScore;
        if (gameState.currentContestName == "CTT Day1") gameState.playerStats.prevScore1 = totalActualScore;
        else if (gameState.currentContestName == "CTT Day2") gameState.playerStats.prevScore2 = totalActualScore;
        else if (gameState.currentContestName == "CTT Day3") gameState.playerStats.prevScore3 = totalActualScore;
    }

    logEvent("比赛结束！实际总分: " + std::to_string(totalActualScore), "event");
    return result;
}

} // namespace Contest

#endif // CONTEST_HPP
