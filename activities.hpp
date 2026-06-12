#ifndef ACTIVITIES_HPP
#define ACTIVITIES_HPP

#include "game.hpp"
#include "contest.hpp"
#include <string>
#include <vector>

namespace Activity {

enum Type { Learn, MockContest, CustomContest, Practice, StudyCulture, Rest, SummerCamp };

struct ActivityDef {
    Type type;
    const char* name;
    int apCost;
    const char* desc;
};

// 网赛选项
struct MockContestOption {
    int contestId;
    const char* name;
    const char* desc;
};

inline const std::vector<MockContestOption> MOCK_CONTEST_OPTIONS = {
    {17, "洛谷入门赛",       "3题, 入门~普及, 15时间"},
    {18, "洛谷月赛",         "4题, 普及~提高, 21时间"},
    {19, "AtCoder ABC",      "4题, 基础~中等, 24时间"},
    {20, "AtCoder ARC",      "4题, 中等~较难, 24时间"},
    {21, "Codeforces Div.3", "4题, 入门~普及, 18时间"},
    {22, "Codeforces Div.2", "4题, 普及~提高, 21时间"},
    {23, "Codeforces Div.1", "3题, 提高~省选, 27时间"},
    {24, "U Cup",            "4题, 提高~省选, 24时间"},
};

// 刷题难度选项（对应洛谷等级）
struct PracticeOption {
    int contestId;
    const char* name;
    const char* tier; // 洛谷难度标签
};

inline const std::vector<PracticeOption> PRACTICE_OPTIONS = {
    {28, "入门题",       "入门"},
    {29, "普及-题",      "普及-"},
    {30, "普及/提高-题",  "普及/提高-"},
    {31, "普及+/提高题",  "普及+/提高"},
    {32, "提高+/省选-题", "提高+/省选-"},
    {33, "省选/NOI-题",   "省选/NOI-"},
    {34, "NOI+题",       "NOI/NOI+/CTSC"},
};

inline std::vector<ActivityDef> getAvailable() {
    std::vector<ActivityDef> list = {
        {Learn,         "学习知识",     2, "选择一个知识方向进行学习"},
        {MockContest,   "打网赛",       3, "参加真实网赛（完整比赛流程）"},
        {CustomContest, "自定义模拟赛", 4, "自选难度+知识点，消耗 4 AP"},
        {Practice,      "刷题练习",     2, "按难度刷一道题（完整比赛流程）"},
        {StudyCulture,  "学文化课",     2, "文化课+2, 心态+1"},
        {Rest,          "休息",         2, "健康+3, 心态+2"},
    };
    if (gameState.isTingke) {
        list.erase(std::remove_if(list.begin(), list.end(),
            [](const ActivityDef& a) { return a.type == StudyCulture; }),
            list.end());
    }
    // 暑假集训（7-8月，非高三）
    int cm = gameState.calendarMonth;
    if ((cm == 7 || cm == 8) && gameState.currentYear <= 2) {
        list.push_back({SummerCamp, "参加集训", 4, "高强度训练，知识+2，健康-2，心态-1"});
    }
    return list;
}

struct ActivityResult {
    std::vector<std::string> logs;
    bool startContest = false;
    int contestId = 0;
};

inline ActivityResult execute(Type type, int param = 0) {
    ActivityResult result;

    switch (type) {
    case Learn: {
        if (param < 0 || param >= static_cast<int>(KNOWLEDGE_DIMS.size())) {
            result.logs.push_back("无效的知识方向");
            return result;
        }
        const std::string& dim = KNOWLEDGE_DIMS[param];
        double eff = getStudyEfficiency();
        int gain = (eff >= 1.0) ? 1 : (Utils::randomDouble(0.0, 1.0) < eff ? 1 : 0);
        if (gain > 0) {
            applyStatDelta(dim, 1, "学习");
            result.logs.push_back(Utils::getStatName(dim) + " +1（效率 " +
                std::to_string(static_cast<int>(eff * 100)) + "%）");
        } else {
            result.logs.push_back("心态太低，学习效率不足");
        }
        gameState.lastStudyMonth[dim] = gameState.currentMonth;
        if (Utils::randomDouble(0.0, 1.0) < 0.3) {
            applyStatDelta("coding", 1, "学习");
            result.logs.push_back("代码能力 +1");
        }
        break;
    }

    case MockContest: {
        if (param < 0 || param >= static_cast<int>(MOCK_CONTEST_OPTIONS.size())) {
            result.logs.push_back("无效的网赛选择");
            return result;
        }
        const auto& opt = MOCK_CONTEST_OPTIONS[param];
        result.startContest = true;
        result.contestId = opt.contestId;
        result.logs.push_back(std::string("参加网赛：") + opt.name);
        break;
    }

    case CustomContest: {
        result.startContest = false;
        result.contestId = param;
        break;
    }

    case Practice: {
        if (param < 0 || param >= static_cast<int>(PRACTICE_OPTIONS.size())) {
            result.logs.push_back("无效的练习选择");
            return result;
        }
        const auto& opt = PRACTICE_OPTIONS[param];
        result.startContest = true;
        result.contestId = opt.contestId;
        result.logs.push_back(std::string("开始刷题：") + opt.name);
        break;
    }

    case StudyCulture: {
        double eff = gameState.cultureEfficiency;
        int cultureGain = static_cast<int>(2 * getStudyEfficiency() * eff);
        cultureGain = std::max(1, cultureGain);
        applyStatDelta("culture", cultureGain, "学文化课");
        result.logs.push_back("文化课 +" + std::to_string(cultureGain) +
            "（效率" + std::to_string(static_cast<int>(eff * 100)) + "%）");
        applyStatDelta("mood", 1, "学文化课");
        result.logs.push_back("心态 +1");
        gameState.lastStudyMonth["culture"] = gameState.currentMonth;
        break;
    }

    case Rest: {
        applyStatDelta("health", 3, "休息");
        applyStatDelta("mood", 2, "休息");
        result.logs.push_back("健康 +3, 心态 +2");
        if (Utils::randomDouble(0.0, 1.0) < 0.1) {
            applyStatDelta("luck", 1, "休息转运");
            result.logs.push_back("运气 +1（休息转运！）");
        }
        break;
    }

    case SummerCamp: {
        // 暑假集训：高强度训练
        double eff = getStudyEfficiency();
        // 随机选 2 个知识维度提升
        int dim1 = Utils::randomInt(0, static_cast<int>(KNOWLEDGE_DIMS.size()) - 1);
        int dim2 = Utils::randomInt(0, static_cast<int>(KNOWLEDGE_DIMS.size()) - 1);
        while (dim2 == dim1) {
            dim2 = Utils::randomInt(0, static_cast<int>(KNOWLEDGE_DIMS.size()) - 1);
        }
        const std::string& d1 = KNOWLEDGE_DIMS[dim1];
        const std::string& d2 = KNOWLEDGE_DIMS[dim2];
        int gain1 = eff >= 0.8 ? 1 : (Utils::randomBool(eff) ? 1 : 0);
        int gain2 = eff >= 0.8 ? 1 : (Utils::randomBool(eff) ? 1 : 0);
        if (gain1 > 0) {
            applyStatDelta(d1, 1, "集训");
            result.logs.push_back(Utils::getStatName(d1) + " +1");
            gameState.lastStudyMonth[d1] = gameState.currentMonth;
        }
        if (gain2 > 0) {
            applyStatDelta(d2, 1, "集训");
            result.logs.push_back(Utils::getStatName(d2) + " +1");
            gameState.lastStudyMonth[d2] = gameState.currentMonth;
        }
        // 代码和思维额外提升
        if (Utils::randomBool(0.5)) {
            applyStatDelta("coding", 1, "集训");
            result.logs.push_back("代码能力 +1");
        }
        if (Utils::randomBool(0.3)) {
            applyStatDelta("thinking", 1, "集训");
            result.logs.push_back("思维能力 +1");
        }
        // 代价
        applyStatDelta("health", -2, "集训");
        applyStatDelta("mood", -1, "集训");
        result.logs.push_back("健康 -2, 心态 -1（高强度集训）");
        // 经验积累
        addTempExperience(2, "集训经验");
        break;
    }
    }

    return result;
}

} // namespace Activity

#endif // ACTIVITIES_HPP
