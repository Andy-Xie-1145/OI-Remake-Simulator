#ifndef GAME_HPP
#define GAME_HPP

#include "types.hpp"
#include "problem_pool.hpp"
#include "events.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
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

// 操作记录
inline std::vector<std::string> lastActions;

// 训练阶段
inline int currentPhase = 1;
inline int remainingEvents = 5;

// 商店价格
inline std::map<std::string, int> currentShopPrices;

// 日志
inline std::vector<std::string> gameLog;

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
    if (playerStats.culture > 0) std::cout << "│\t  文化课: " << std::setw(2) << playerStats.culture << "\t\t\t│\n";
    std::cout << "└────────────────────────────────────────┘\n";
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
        
        // 显示属性
        std::cout << "\t";
        if (sp.dp > 0) std::cout << "动态规划:" << (sp.blur && thinkProgress[idx][i] < thinkTime ? "?" : std::to_string(sp.dp)) << " ";
        if (sp.ds > 0) std::cout << "数据结构:" << (sp.blur && thinkProgress[idx][i] < thinkTime ? "?" : std::to_string(sp.ds)) << " ";
        if (sp.str > 0) std::cout << "字符串:" << (sp.blur && thinkProgress[idx][i] < thinkTime ? "?" : std::to_string(sp.str)) << " ";
        if (sp.graph > 0) std::cout << "图论:" << (sp.blur && thinkProgress[idx][i] < thinkTime ? "?" : std::to_string(sp.graph)) << " ";
        if (sp.comb > 0) std::cout << "组合计数:" << (sp.blur && thinkProgress[idx][i] < thinkTime ? "?" : std::to_string(sp.comb)) << " ";
        if (sp.thinking > 0) std::cout << "思维:" << sp.thinking << " ";
        if (sp.coding > 0) std::cout << "代码:" << sp.coding << " ";
        std::cout << "\n";
        
        double thinkRate = calculateThinkSuccessRate(sp);
        double codeRate = calculateCodeSuccessRate(sp);
        
        if (thinkProgress[idx][i] < thinkTime)
            std::cout << "\t[" << (i+1) << "a] 思考 (" << thinkProgress[idx][i] << "/" << (sp.blur && thinkProgress[idx][i] < thinkTime ? "?" : std::to_string(thinkTime)) << ", 成功率:" << (int)(thinkRate*100) << "%)\n";
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
    lastActions.push_back("think");
    if (lastActions.size() > 5) lastActions.erase(lastActions.begin());
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
    lastActions.push_back("code");
    if (lastActions.size() > 5) lastActions.erase(lastActions.begin());
}

// 对拍/提交部分分
inline void checkCodeSubProblem(int problemIdx, int subProblemIdx) {
    bool isIOIContest = false;
    for (const auto& cfg : CONTEST_CONFIGS) {
        if (cfg.second.name == currentContestName && cfg.second.isIOI) { isIOIContest = true; break; }
    }
    if (!isIOIContest && timePoints <= 0) { std::cout << "时间点不足！\n"; return; }
    if (!isIOIContest) timePoints--;
    
    lastActions.push_back("check");
    if (lastActions.size() > 5) lastActions.erase(lastActions.begin());
    
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
    double mult = DIFFICULTY_SETTINGS.at(gameDifficulty).scoreMultiplier;
    std::string award;
    
    if (contestType == "CSP-S" || contestType == "NOIP") {
        if (score >= 270 * mult) award = "一等奖";
        else if (score >= 180 * mult) award = "二等奖";
        else if (score >= 50 * mult) award = "三等奖";
        else award = "没有获奖";
    } else if (contestType == "WC" || contestType == "APIO") {
        if (score >= 220 * mult) award = "金牌";
        else if (score >= 160 * mult) award = "银牌";
        else if (score >= 100 * mult) award = "铜牌";
        else award = "铁牌";
    } else if (contestType.find("NOI") != std::string::npos) {
        if (score >= 400 * mult) { award = "金牌"; playerStats.isTrainingTeam = true; }
        else if (score >= 300 * mult) award = "银牌";
        else if (score >= 200 * mult) award = "铜牌";
        else award = "铁牌";
    } else {
        award = "完成比赛";
    }
    
    playerStats.achievements.push_back(contestType + ": " + std::to_string(score) + "分, " + award);
    return award;
}

// 显示比赛结果
inline void showResults() {
    int score = calculateScore();
    
    std::cout << "\n┌────────────────────────────────────────┐\n";
    std::cout << "│\t比 赛 结 果\t\t\t\t│\n";
    std::cout << "├────────────────────────────────────────┤\n";
    
    for (int i = 0; i < totalProblems; i++) {
        int probScore = 0;
        for (size_t j = 0; j < subProblems[i].size(); j++) {
            if (isCodeComplete[i][j]) probScore = std::max(probScore, subProblems[i][j].score);
        }
        std::cout << "│\tT" << (i+1) << " (" << problems[i].name << "): " << std::setw(3) << probScore << " 分\t\t│\n";
    }
    
    std::cout << "├────────────────────────────────────────┤\n";
    std::cout << "│\t总分: " << std::setw(3) << score << " 分\t\t\t\t│\n";
    
    int detReward = score * 5;
    playerStats.determination += detReward;
    std::cout << "│\t决心奖励: +" << detReward << "\t\t\t\t│\n";
    
    std::string award = calculateAward(score, currentContestName);
    std::cout << "│\t获奖: " << award << "\t\t\t\t│\n";
    std::cout << "└────────────────────────────────────────┘\n";
    
    // 心态恢复
    int minMood = std::min(5 + playerStats.mental, 10);
    if (mood < minMood) {
        int recovery = minMood - mood;
        mood = minMood;
        logEvent("比赛结束后心态自动恢复：+" + std::to_string(recovery), "event");
    }
    
    // 保存成绩
    if (currentContestName == "CSP-S") playerStats.cspScore = score;
    else if (currentContestName == "NOIP") playerStats.noipScore = score;
    else if (currentContestName == "省选Day1") playerStats.prevScore = score;
    else if (currentContestName == "NOI Day1") playerStats.prevScore = score;
    
    logEvent("比赛结束！总分: " + std::to_string(score), "event");
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
        
        std::string input;
        std::cin >> input;
        
        if (input == "p") currentProblem = currentProblem > 1 ? currentProblem - 1 : totalProblems;
        else if (input == "n") currentProblem = currentProblem < totalProblems ? currentProblem + 1 : 1;
        else if (input == "0" && isFullScore()) break;
        else if (input.length() >= 2) {
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
}

// ========== 训练阶段 ==========
inline void runTrainingPhase(int numEvents) {
    for (int i = 0; i < numEvents; i++) {
        std::cout << "\n┌────────────────────────────────────────┐\n";
        std::cout << "│\t训练阶段 (" << (i+1) << "/" << numEvents << ")\t\t\t│\n";
        std::cout << "└────────────────────────────────────────┘\n";
        
        displayPlayerStatus();
        
        // 根据阶段选择事件类型
        std::string eventType;
        if (currentPhase == 1) {
            if (i == 0) eventType = "长期训练";
            else if (i == numEvents - 1) eventType = "赛前一天";
            else eventType = "提升训练";
        } else if (currentPhase == 17) {  // 步入高二
            if (i == 0) eventType = "步入高二";
            else eventType = "提升训练";
        } else {
            if (i == numEvents - 1) eventType = "赛前一天";
            else eventType = "提升训练";
        }
        
        runEventChain(eventType);
        
        std::cout << "\n按回车继续...";
        std::cin.ignore();
        std::cin.get();
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
    gameLog.clear();
    currentShopPrices = INITIAL_SHOP_PRICES.at(gameDifficulty);
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
    
    int choice;
    std::cin >> choice;
    
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
    
    std::cout << "动态规划 (剩余 " << remaining << " 点): "; std::cin >> dp; remaining -= dp;
    std::cout << "数据结构 (剩余 " << remaining << " 点): "; std::cin >> ds; remaining -= ds;
    std::cout << "字符串 (剩余 " << remaining << " 点): "; std::cin >> str; remaining -= str;
    std::cout << "图论 (剩余 " << remaining << " 点): "; std::cin >> graph; remaining -= graph;
    std::cout << "组合计数 (剩余 " << remaining << " 点): "; std::cin >> comb;
    
    playerStats.dp = dp;
    playerStats.ds = ds;
    playerStats.string = str;
    playerStats.graph = graph;
    playerStats.combinatorics = comb;
    
    displayPlayerStatus();
}

inline void showGameOver() {
    std::cout << "\n┌────────────────────────────────────────┐\n";
    std::cout << "│\t\t游 戏 结 束\t\t\t│\n";
    std::cout << "└────────────────────────────────────────┘\n\n";
    
    std::cout << "【你的成就】\n";
    for (const auto& ach : playerStats.achievements) {
        std::cout << "  - " << ach << "\n";
    }
    
    displayPlayerStatus();
    std::cout << "\n感谢游玩 OI重开模拟器！\n";
}

// ========== 主游戏流程 ==========
inline void runGame() {
    showTitle();
    selectDifficulty();
    allocateTalent();
    
    std::cout << "\n按回车开始游戏...";
    std::cin.ignore();
    std::cin.get();
    
    // === 第一阶段：高一 ===
    // 训练 + CSP-S
    logEvent("训练阶段开始！", "event");
    runTrainingPhase(5);
    logEvent("CSP-S比赛即将开始...", "event");
    runContestLoop(1);
    
    std::cout << "\n按回车继续...";
    std::cin.get();
    
    // 训练 + NOIP
    runTrainingPhase(4);
    logEvent("NOIP比赛即将开始...", "event");
    runContestLoop(2);
    
    std::cout << "\n按回车继续...";
    std::cin.get();
    
    // WC（如果成绩够）
    double mult = DIFFICULTY_SETTINGS.at(gameDifficulty).scoreMultiplier;
    if (playerStats.cspScore >= 180 * mult) {
        runTrainingPhase(4);
        logEvent("WC比赛即将开始...", "event");
        runContestLoop(3);
        std::cout << "\n按回车继续...";
        std::cin.get();
    } else {
        logEvent("CSP-S成绩未达二等奖，无法参加WC", "event");
    }
    
    // 省选Day1
    runTrainingPhase(4);
    logEvent("省选Day1比赛即将开始...", "event");
    runContestLoop(4);
    
    // 省选Day2
    runTrainingPhase(2);
    logEvent("省选Day2比赛即将开始...", "event");
    runContestLoop(5);
    
    // 检查是否进省队
    int provScore = playerStats.prevScore + playerStats.noipScore;
    if (provScore >= 600 * mult) {
        playerStats.isProvincialTeam = true;
        logEvent("恭喜进入省队！", "event");
        
        std::cout << "\n按回车继续...";
        std::cin.get();
        
        // APIO
        if (playerStats.noipScore >= 180 * mult) {
            runTrainingPhase(4);
            logEvent("APIO比赛即将开始...", "event");
            runContestLoop(6);
            std::cout << "\n按回车继续...";
            std::cin.get();
        }
        
        // NOI Day1
        runTrainingPhase(4);
        logEvent("NOI Day1比赛即将开始...", "event");
        runContestLoop(7);
        
        // NOI Day2
        runTrainingPhase(2);
        logEvent("NOI Day2比赛即将开始...", "event");
        runContestLoop(8);
        
        if (playerStats.isTrainingTeam) {
            logEvent("恭喜进入国家集训队！", "event");
        }
    } else {
        logEvent("未能进入省队，OI生涯结束", "event");
    }
    
    showGameOver();
}

#endif // GAME_HPP
