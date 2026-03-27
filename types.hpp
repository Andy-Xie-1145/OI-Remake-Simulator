#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <random>
#include <map>

// ========== 核心常量（完全复制原版） ==========
inline const int MOOD_LIMIT = 12;  // 心态上限

// ========== 玩家属性结构（完全复制原版playerStats） ==========
struct PlayerStats {
    // 知识点 (0-20)
    int dp = 0;              // 动态规划
    int ds = 0;              // 数据结构
    int string = 0;          // 字符串
    int graph = 0;           // 图论
    int combinatorics = 0;   // 组合计数
    
    // 能力值 (0-20)
    int thinking = 0;        // 思维
    int coding = 0;          // 代码
    int carefulness = 0;     // 细心
    int quickness = 0;       // 迅捷
    int mental = 0;          // 心理素质
    int culture = 0;         // 文化课
    
    // 核心属性
    int determination = 500; // 决心
    
    // 比赛成绩
    int cspScore = 0;
    int noipScore = 0;
    int prevScore = 0;
    int prevScore1 = 0;
    int prevScore2 = 0;
    int prevScore3 = 0;
    int cttScore = 0;
    int tempScore = 0;
    int noiScore = 0;
    
    // 状态标记
    bool isProvincialTeamA = false;
    bool isProvincialTeam = false;
    bool isTrainingTeam = false;
    bool isCandidateTeam = false;
    bool isNationalTeam = false;
    bool isIOIgold = false;
    
    // 其他
    int extraMoodDrop = 0;
    std::vector<std::string> achievements;
};

// ========== 部分分结构（完全复制原版SubProblem） ==========
struct SubProblem {
    int dp = 0;
    int ds = 0;
    int str = 0;
    int graph = 0;
    int comb = 0;
    int adhoc = 0;
    int thinking = 0;
    int coding = 0;
    int detail = 0;
    int trap = 0;
    int independent = 0;
    int heat = 0;
    int blur = 0;      // 模糊：思考进度未满时显示?
    int fallback = 0;
    int inspire = 0;
    int score = 0;
};

// ========== 题目结构 ==========
struct Problem {
    std::string name;
    int level;
    std::vector<SubProblem> parts;
    int tag = 0;
};

// ========== 比赛配置（完全复制原版contestConfigs） ==========
struct ContestConfig {
    std::string name;
    std::vector<std::pair<int, int>> problemRanges;  // {minLevel, maxLevel}
    int timePoints;
    bool isIOI = false;
};

inline const std::map<int, ContestConfig> CONTEST_CONFIGS = {
    {1, {"CSP-S", {{2,3}, {3,4}, {4,5}, {5,6}}, 21, false}},
    {2, {"NOIP", {{3,4}, {3,4}, {4,6}, {5,6}}, 24, false}},
    {3, {"WC", {{3,4}, {6,8}, {8,9}}, 30, false}},
    {4, {"省选Day1", {{4,5}, {6,7}, {8,8}}, 27, false}},
    {5, {"省选Day2", {{5,6}, {6,7}, {8,9}}, 27, false}},
    {6, {"APIO", {{5,8}, {6,9}, {8,9}}, 30, true}},
    {7, {"NOI Day1", {{5,7}, {7,8}, {8,9}}, 30, false}},
    {8, {"NOI Day2", {{7,7}, {8,9}, {9,10}}, 30, false}},
    {9, {"CTT Day1", {{8,9}, {9,10}, {9,10}}, 30, true}},
    {10, {"CTT Day2", {{8,9}, {9,10}, {9,10}}, 30, true}},
    {11, {"CTT Day3", {{8,9}, {9,10}, {9,10}}, 30, true}},
    {12, {"CTT Day4", {{9,9}, {9,10}, {10,10}}, 30, true}},
    {13, {"CTS Day1", {{8,9}, {9,10}, {9,10}}, 30, true}},
    {14, {"CTS Day2", {{9,10}, {9,10}, {9,10}}, 30, true}},
    {15, {"IOI Day1", {{7,9}, {8,9}, {9,10}}, 30, true}},
    {16, {"IOI Day2", {{9,10}, {9,10}, {10,10}}, 30, true}}
};

// ========== 难度设置（完全复制原版） ==========
struct DifficultySettings {
    int talentPoints;
    int initialDetermination;
    double scoreMultiplier;
    std::string name;
};

inline const std::map<std::string, DifficultySettings> DIFFICULTY_SETTINGS = {
    {"easy", {30, 3000, 0.8, "简单"}},
    {"normal", {20, 1500, 0.9, "普通"}},
    {"hard", {15, 500, 1.0, "困难"}},
    {"expert", {15, 0, 1.1, "专家"}}
};

// ========== 商店价格增长配置（完全复制原版shopPriceIncrements） ====
inline const std::map<std::string, std::map<std::string, int>> SHOP_PRICE_INCREMENTS = {
    {"easy", {
        {"思维提升", 200}, {"代码提升", 200}, {"细心提升", 200},
        {"随机提升", 0}, {"心态恢复", 0}, {"全面提升", 200},
        {"速度提升", 1000}, {"心理素质提升", 2000}
    }},
    {"normal", {
        {"思维提升", 300}, {"代码提升", 300}, {"细心提升", 300},
        {"随机提升", 200}, {"心态恢复", 200}, {"全面提升", 300},
        {"速度提升", 2000}, {"心理素质提升", 3000}
    }},
    {"hard", {
        {"思维提升", 500}, {"代码提升", 500}, {"细心提升", 500},
        {"随机提升", 200}, {"心态恢复", 200}, {"全面提升", 1000},
        {"速度提升", 3000}, {"心理素质提升", 5000}
    }},
    {"expert", {
        {"思维提升", 1000}, {"代码提升", 1000}, {"细心提升", 1000},
        {"随机提升", 500}, {"心态恢复", 500}, {"全面提升", 2000},
        {"速度提升", 5000}, {"心理素质提升", 7000}
    }}
};

// ========== 商店初始价格（完全复制原版getInitialShopPrices） ==========
inline const std::map<std::string, std::map<std::string, int>> INITIAL_SHOP_PRICES = {
    {"easy", {
        {"思维提升", 200}, {"代码提升", 200}, {"细心提升", 200},
        {"随机提升", 200}, {"心态恢复", 300}, {"全面提升", 800},
        {"速度提升", 1000}, {"心理素质提升", 1000}
    }},
    {"normal", {
        {"思维提升", 300}, {"代码提升", 300}, {"细心提升", 300},
        {"随机提升", 300}, {"心态恢复", 300}, {"全面提升", 1000},
        {"速度提升", 1000}, {"心理素质提升", 1000}
    }},
    {"hard", {
        {"思维提升", 300}, {"代码提升", 300}, {"细心提升", 300},
        {"随机提升", 300}, {"心态恢复", 500}, {"全面提升", 1000},
        {"速度提升", 1500}, {"心理素质提升", 1500}
    }},
    {"expert", {
        {"思维提升", 500}, {"代码提升", 500}, {"细心提升", 500},
        {"随机提升", 500}, {"心态恢复", 500}, {"全面提升", 1500},
        {"速度提升", 2500}, {"心理素质提升", 2500}
    }}
};

// ========== 商店商品效果（完全复制原版） ==========
inline const std::map<std::string, std::pair<std::string, int>> SHOP_EFFECTS = {
    {"思维提升", {"thinking", 2}},
    {"代码提升", {"coding", 2}},
    {"细心提升", {"carefulness", 2}},
    {"心态恢复", {"mood", 2}},
    {"速度提升", {"quickness", 1}},
    {"心理素质提升", {"mental", 1}}
};

// ========== 随机事件配置（完全复制原版randomEvents） ==========
struct RandomEvent {
    std::string name;
    std::string description;
    std::string effect;  // 效果描述
    double probability;
};

inline const std::vector<RandomEvent> RANDOM_EVENTS = {
    {"心态爆炸", "连续失败让你感到沮丧...", "心态值-1", 0.04},
    {"灵光一闪", "突然想到了一个好方法！", "心态值+1", 0.03},
    {"代码bug", "写着写着发现之前的代码有问题...", "代码进度-1", 0.03},
    {"键盘故障", "键盘突然有点不太灵了...", "心态值-1", 0.02},
    {"监考老师巡视", "监考老师正在经过你的座位...", "心态值-1", 0.01}
};

// ========== 随机数生成器 ==========
inline std::random_device rd;
inline std::mt19937 gen(rd());

// ========== 工具函数 ==========
namespace Utils {
    inline int randomInt(int min, int max) {
        std::uniform_int_distribution<int> dis(min, max);
        return dis(gen);
    }
    
    inline double randomDouble(double min, double max) {
        std::uniform_real_distribution<double> dis(min, max);
        return dis(gen);
    }
    
    inline bool randomBool(double probability) {
        std::bernoulli_distribution dis(probability);
        return dis(gen);
    }
    
    inline std::string getStatName(const std::string& key) {
        static const std::map<std::string, std::string> names = {
            {"dp", "动态规划"}, {"ds", "数据结构"}, {"string", "字符串"},
            {"graph", "图论"}, {"combinatorics", "组合计数"},
            {"thinking", "思维"}, {"coding", "代码"}, {"carefulness", "细心"},
            {"quickness", "迅捷"}, {"mental", "心理素质"}, {"culture", "文化课"},
            {"mood", "心态"}, {"determination", "决心"}
        };
        auto it = names.find(key);
        return it != names.end() ? it->second : key;
    }
    
    // 属性映射函数（完全复制原版mapAttributeValue）
    inline int mapAttributeValue(int value) {
        if (value <= 2) return value;
        if (value <= 4) return 3;
        if (value <= 6) return 4;
        if (value <= 8) return 5;
        if (value <= 10) return 6;
        if (value <= 12) return 7;
        if (value <= 14) return 8;
        if (value <= 17) return 9;
        return 10;
    }
}

#endif // TYPES_HPP
