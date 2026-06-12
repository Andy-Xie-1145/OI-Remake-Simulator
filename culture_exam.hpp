#ifndef CULTURE_EXAM_HPP
#define CULTURE_EXAM_HPP

#include "game.hpp"
#include <string>
#include <vector>

namespace CultureExam {

enum class Phase { Read, Answer, Guess, Check, Done };

struct ExamQuestion {
    std::string text;
    int difficulty;
};

struct ExamState {
    int currentQ = 0;
    int totalQ = 5;
    std::vector<Phase> phases;
    std::vector<int> results;       // -1=未答, 0=错, 1=对
    std::vector<bool> guessed;
    std::vector<bool> checked;
    std::vector<int> questionScores; // 每题满分
    int score = 0;
    int maxScore = 0;
};

inline ExamState examState;

// 题目文本生成（模拟考试题目）
inline std::string generateQuestionText(int difficulty, int idx) {
    static const char* subjects[] = {
        "语文", "数学", "英语", "物理", "化学", "历史", "政治", "地理", "生物"
    };
    static const char* types[] = {
        "选择题", "填空题", "计算题", "简答题", "论述题", "证明题", "应用题"
    };
    static const char* topics[][3] = {
        {"文言文阅读理解", "现代文分析", "作文立意构思"},
        {"函数与导数", "数列与不等式", "解析几何"},
        {"完形填空", "阅读理解", "语法改错"},
        {"力学综合题", "电磁学分析", "光学实验"},
        {"有机化学推断", "化学平衡", "实验设计"},
        {"历史文献分析", "重大事件评述", "制度变迁"},
        {"政治理论应用", "时事分析", "经济原理"},
        {"区域地理分析", "地图判读", "自然地理"},
        {"遗传学分析", "生态系统", "分子生物学"}
    };
    int subjIdx = Utils::randomInt(0, 8);
    int topicIdx = Utils::randomInt(0, 2);
    int typeIdx = Utils::randomInt(0, 6);

    std::string text = "第" + std::to_string(idx + 1) + "题（";
    text += subjects[subjIdx];
    text += "·";
    text += topics[subjIdx][topicIdx];
    text += "）";
    text += "\n题型：" + std::string(types[typeIdx]);
    text += "  难度：" + std::string(getLuoguTierName(difficulty));

    // 根据难度生成题目描述
    if (difficulty <= 2) {
        text += "\n本题考查基础知识掌握，需要回忆课本内容并准确作答。";
    } else if (difficulty <= 4) {
        text += "\n本题需要理解概念并进行简单推理，注意审题和计算。";
    } else if (difficulty <= 6) {
        text += "\n本题涉及多个知识点的综合运用，需要较强的分析和推理能力。";
    } else {
        text += "\n本题具有较高的思维深度和计算复杂度，需要扎实的功底和清晰的逻辑。";
    }
    return text;
}

inline void start(bool isGaokao) {
    examState = ExamState();
    examState.totalQ = isGaokao ? 8 : 5;
    examState.currentQ = 0;

    int year = gameState.currentYear;
    int baseDiff = (year == 1) ? 3 : (year == 2) ? 4 : 5;

    for (int i = 0; i < examState.totalQ; ++i) {
        int diff = baseDiff + Utils::randomInt(0, 2);
        examState.phases.push_back(Phase::Read);
        examState.results.push_back(-1);
        examState.guessed.push_back(false);
        examState.checked.push_back(false);
        examState.questionScores.push_back(isGaokao ? 20 : 20);
        examState.maxScore += 20;
    }
}

inline void readQuestion() {
    if (examState.currentQ >= examState.totalQ) return;
    examState.phases[examState.currentQ] = Phase::Answer;
}

inline void answerQuestion() {
    if (examState.currentQ >= examState.totalQ) return;
    if (examState.phases[examState.currentQ] != Phase::Answer &&
        examState.phases[examState.currentQ] != Phase::Guess) return;

    int year = gameState.currentYear;
    int baseDiff = (year == 1) ? 3 : (year == 2) ? 4 : 5;
    int difficulty = baseDiff + Utils::randomInt(0, 2);

    // 正确率公式：baseRate = 0.3 + culture*0.04 + thinking*0.02 - difficulty*0.05
    double rate = 0.3
        + gameState.playerStats.culture * 0.04
        + gameState.playerStats.thinking * 0.02
        - difficulty * 0.05;
    rate = std::max(0.1, std::min(0.95, rate));

    examState.results[examState.currentQ] = Utils::randomBool(rate) ? 1 : 0;
    examState.phases[examState.currentQ] = Phase::Check;
}

inline void guessQuestion() {
    if (examState.currentQ >= examState.totalQ) return;
    if (examState.phases[examState.currentQ] != Phase::Answer) return;

    // 蒙题正确率：0.25 + culture*0.03 + luck*0.01
    double guessRate = 0.25
        + gameState.playerStats.culture * 0.03
        + gameState.playerStats.luck * 0.01;
    guessRate = std::max(0.25, std::min(0.7, guessRate));

    examState.results[examState.currentQ] = Utils::randomBool(guessRate) ? 1 : 0;
    examState.guessed[examState.currentQ] = true;
    examState.phases[examState.currentQ] = Phase::Check;
}

inline void checkQuestion() {
    if (examState.currentQ >= examState.totalQ) return;
    if (examState.phases[examState.currentQ] != Phase::Check) return;
    if (examState.checked[examState.currentQ]) return; // 只能检查一次

    examState.checked[examState.currentQ] = true;

    if (examState.results[examState.currentQ] == 0) {
        // 答错：30% + carefulness*0.02 概率翻转为对
        double fixRate = 0.3 + gameState.playerStats.carefulness * 0.02;
        if (Utils::randomBool(fixRate)) {
            examState.results[examState.currentQ] = 1;
        }
    } else {
        // 答对：5% 概率改错
        if (Utils::randomBool(0.05)) {
            examState.results[examState.currentQ] = 0;
        }
    }

    examState.phases[examState.currentQ] = Phase::Done;
}

inline void nextQuestion() {
    if (examState.currentQ < examState.totalQ) {
        examState.currentQ++;
    }
}

inline int finalize() {
    int score = 0;
    for (int i = 0; i < examState.totalQ; ++i) {
        if (examState.results[i] == 1) {
            score += examState.questionScores[i];
        }
    }
    examState.score = score;

    // 高考特殊处理：文化课乘数
    if (gameState.currentMonth == 36) {
        int culture = gameState.playerStats.culture;
        double mult = 1.0;
        if (culture >= 16) {
            mult = 1.2;
        } else if (culture < 10) {
            mult = 0.6;
        }
        score = static_cast<int>(score * mult);
        examState.score = score;
    }

    // 文化课学习效果：根据分数给少量 culture 经验
    double pct = (examState.maxScore > 0) ? (double)score / examState.maxScore : 0;
    if (pct >= 0.8) {
        applyStatDelta("culture", 1, "考试优秀");
    }

    return score;
}

inline bool isComplete() {
    return examState.currentQ >= examState.totalQ;
}

inline bool isCurrentDone() {
    if (examState.currentQ >= examState.totalQ) return true;
    return examState.phases[examState.currentQ] == Phase::Done;
}

inline const char* getPhaseName(Phase p) {
    switch (p) {
    case Phase::Read: return "阅读";
    case Phase::Answer: return "作答";
    case Phase::Guess: return "蒙题";
    case Phase::Check: return "检查";
    case Phase::Done: return "已完成";
    }
    return "???";
}

} // namespace CultureExam

#endif // CULTURE_EXAM_HPP
