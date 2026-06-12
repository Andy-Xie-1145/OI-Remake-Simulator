#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <set>
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
    int math = 0;            // 数学
    int geometry = 0;        // 几何
    int data_structure = 0;  // 高级数据结构
    int adhoc = 0;           // 构造/思维题

    // 能力值 (0-20)
    int thinking = 0;        // 思维
    int coding = 0;          // 代码
    int carefulness = 0;     // 细心
    int quickness = 0;       // 迅捷
    int mental = 0;          // 心理素质
    int experience = 0;      // 经验
    int tempExperience = 0;  // 经验积累
    int culture = 0;         // 文化课
    int luck = 0;            // 运气：减少负面事件发生率 (0-20)

    // 核心属性
    int determination = 500; // 决心（保留兼容，v2 不再使用）

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
    int branch = 0;    // 分支：代码复杂度，影响修改代码的时间成本
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
    {16, {"IOI Day2", {{9,10}, {9,10}, {10,10}}, 30, true}},

    // 网赛
    {17, {"洛谷入门赛",       {{1,2}, {2,3}, {3,3}}, 15, false}},
    {18, {"洛谷月赛",         {{3,4}, {4,5}, {5,6}, {5,7}}, 21, false}},
    {19, {"AtCoder ABC",      {{2,3}, {3,4}, {4,5}, {5,6}}, 24, false}},
    {20, {"AtCoder ARC",      {{4,5}, {5,6}, {6,7}, {7,8}}, 24, false}},
    {21, {"Codeforces Div.3", {{1,2}, {2,3}, {3,4}, {4,5}}, 18, false}},
    {22, {"Codeforces Div.2", {{3,4}, {4,5}, {5,6}, {6,7}}, 21, false}},
    {23, {"Codeforces Div.1", {{6,7}, {7,8}, {8,9}}, 27, false}},
    {24, {"U Cup",            {{4,5}, {5,6}, {6,7}, {7,8}}, 24, false}},

    // 刷题练习（1题，对应洛谷难度等级）
    {28, {"入门练习",         {{1,1}}, 10, false}},
    {29, {"普及-练习",        {{2,2}}, 10, false}},
    {30, {"普及/提高-练习",    {{3,3}}, 10, false}},
    {31, {"普及+/提高练习",    {{4,5}}, 10, false}},
    {32, {"提高+/省选-练习",   {{5,6}}, 10, false}},
    {33, {"省选/NOI-练习",     {{7,8}}, 10, false}},
    {34, {"NOI+练习",         {{9,10}}, 10, false}}
};

// ========== 难度设置（完全复制原版） ==========
struct DifficultySettings {
    int talentPoints;
    int initialDetermination;
    double scoreMultiplier;
    std::string name;
    // v2 新增
    int initialMoney = 300;
    int initialHealth = 15;
    int apBonus = 0;           // 每月额外 AP
    int monthlyIncome = 100;   // 基础月收入
    double forgettingMultiplier = 1.0; // 遗忘速率倍数
    double anxietyMultiplier = 1.0;    // 焦虑概率倍数
};

inline const std::map<std::string, DifficultySettings> DIFFICULTY_SETTINGS = {
    {"easy",   {30, 3000, 0.8, "简单", 500, 18, 1, 150, 0.7, 0.7}},
    {"normal", {20, 1500, 0.9, "普通", 300, 15, 0, 100, 1.0, 1.0}},
    {"hard",   {15, 500,  1.0, "困难", 150, 13, -1, 70,  1.3, 1.3}},
    {"expert", {15, 0,    1.1, "专家", 50,  10, -1, 40,  1.5, 1.5}}
};

// ========== 商店价格（金钱驱动，v2） ==========
inline const std::map<std::string, std::map<std::string, int>> SHOP_PRICE_INCREMENTS = {
    {"easy", {
        {"思维提升", 40}, {"代码提升", 40}, {"细心提升", 40},
        {"随机提升", 30}, {"心态恢复", 50}, {"全面提升", 150},
        {"速度提升", 100}, {"心理素质提升", 150}, {"经验提升", 125},
        {"运气提升", 50}
    }},
    {"normal", {
        {"思维提升", 60}, {"代码提升", 60}, {"细心提升", 60},
        {"随机提升", 50}, {"心态恢复", 75}, {"全面提升", 225},
        {"速度提升", 150}, {"心理素质提升", 225}, {"经验提升", 200},
        {"运气提升", 75}
    }},
    {"hard", {
        {"思维提升", 90}, {"代码提升", 90}, {"细心提升", 90},
        {"随机提升", 75}, {"心态恢复", 100}, {"全面提升", 300},
        {"速度提升", 250}, {"心理素质提升", 350}, {"经验提升", 300},
        {"运气提升", 100}
    }},
    {"expert", {
        {"思维提升", 150}, {"代码提升", 150}, {"细心提升", 150},
        {"随机提升", 125}, {"心态恢复", 175}, {"全面提升", 500},
        {"速度提升", 400}, {"心理素质提升", 600}, {"经验提升", 500},
        {"运气提升", 175}
    }}
};

inline const std::map<std::string, std::map<std::string, int>> INITIAL_SHOP_PRICES = {
    {"easy", {
        {"思维提升", 80}, {"代码提升", 80}, {"细心提升", 80},
        {"随机提升", 60}, {"心态恢复", 100}, {"全面提升", 300},
        {"速度提升", 200}, {"心理素质提升", 300}, {"经验提升", 250},
        {"运气提升", 100}
    }},
    {"normal", {
        {"思维提升", 120}, {"代码提升", 120}, {"细心提升", 120},
        {"随机提升", 100}, {"心态恢复", 150}, {"全面提升", 450},
        {"速度提升", 300}, {"心理素质提升", 450}, {"经验提升", 400},
        {"运气提升", 150}
    }},
    {"hard", {
        {"思维提升", 180}, {"代码提升", 180}, {"细心提升", 180},
        {"随机提升", 150}, {"心态恢复", 200}, {"全面提升", 600},
        {"速度提升", 500}, {"心理素质提升", 700}, {"经验提升", 600},
        {"运气提升", 200}
    }},
    {"expert", {
        {"思维提升", 300}, {"代码提升", 300}, {"细心提升", 300},
        {"随机提升", 250}, {"心态恢复", 350}, {"全面提升", 1000},
        {"速度提升", 800}, {"心理素质提升", 1200}, {"经验提升", 1000},
        {"运气提升", 350}
    }}
};

// ========== 商店商品效果（完全复制原版） ==========
inline const std::map<std::string, std::pair<std::string, int>> SHOP_EFFECTS = {
    {"思维提升", {"thinking", 2}},
    {"代码提升", {"coding", 2}},
    {"细心提升", {"carefulness", 2}},
    {"心态恢复", {"mood", 2}},
    {"速度提升", {"quickness", 1}},
    {"心理素质提升", {"mental", 1}},
    {"经验提升", {"experience", 1}},
    {"运气提升", {"luck", 2}}
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
            {"math", "数学"}, {"geometry", "几何"}, {"data_structure", "高级数据结构"}, {"adhoc", "构造/思维"},
            {"thinking", "思维"}, {"coding", "代码"}, {"carefulness", "细心"},
            {"quickness", "迅捷"}, {"mental", "心理素质"}, {"experience", "经验"},
            {"culture", "文化课"}, {"luck", "运气"},
            {"mood", "心态"}, {"determination", "决心"},
            {"health", "健康"}, {"money", "金钱"}
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

// ========== 竞赛子问题状态 ==========

struct ContestSubProblemState {
    int thinkProgress = 0;
    int codeProgress = 0;
    bool isCodeComplete = false;
    double errorRate = -1.0;
    int modificationCount = 0;
    bool hasAttemptedCheck = false;
    bool requiresCodeModification = false;
};

// ========== 比赛通知 ==========

struct PendingContestNotice {
    bool active = false;
    std::string title;
    std::string description;
    std::string effectText;
};

// ========== 游戏全局状态 ==========

struct GameState {
    PlayerStats playerStats;
    std::string gameDifficulty = "hard";
    int timePoints = 24;
    int mood = 10;
    int currentProblem = 1;
    int totalProblems = 0;
    std::string currentContestName;
    bool debugmode = false;
    std::vector<Problem> problems;
    std::vector<std::vector<SubProblem>> subProblems;
    std::vector<std::vector<ContestSubProblemState>> contestStates;
    std::vector<std::string> lastActions;
    int currentPhase = 1;
    int totalTrainingEvents = 5;
    std::map<std::string, int> currentShopPrices;
    std::vector<std::string> gameLog;
    PendingContestNotice pendingContestNotice;
    std::set<std::string> purchasedItems;

    // v2 月回合制
    int currentMonth = 1;       // 1-36
    int currentYear = 1;        // 1-3
    int calendarMonth = 7;      // 7-12, 1-6 循环
    int ap = 8;                 // 当月剩余行动力
    int maxAp = 8;              // 行动力上限
    int health = 15;            // 0-20，归零=游戏结束
    int money = 0;              // 货币
    bool isTingke = false;      // 停课状态（10 AP, 心态-3, 禁文化课）
    std::map<std::string, int> lastStudyMonth;  // 遗忘追踪：维度→上次学习的月份
    std::vector<std::string> traits;            // 已获得特质
    std::string background;                     // 已选背景 ID
    std::map<std::string, int> ownedItems;      // 拥有物品及数量
    double cultureEfficiency = 1.0;             // 文化课效率乘数（跨年加权）

    // 月度结算临时数据
    std::vector<std::string> settlementLogs;    // 本月结算日志

    // 考试分数追踪
    struct ExamRecord {
        int month;          // 考试月份
        int calendarMonth;  // 日历月
        int score;          // 得分
        int maxScore;       // 满分
        bool isGaokao;      // 是否高考
    };
    std::vector<ExamRecord> examRecords;

    // 焦虑状态
    int anxietyMonths = 0;       // 连续低心态月数
    bool isAnxious = false;      // 当前是否焦虑
    double anxietyMultiplier = 1.0;   // 焦虑概率乘数（由背景设置）
    double efficiencyMultiplier = 1.0; // 学习效率乘数（由背景设置）
    int moodCap = MOOD_LIMIT;    // 心态上限（由背景设置）
};

// ========== v2 月历常量 ==========

// 日历月名称（calendarMonth 1-12）
inline const char* getCalendarMonthName(int cm) {
    static const char* names[] = {
        "", "1月", "2月", "3月", "4月", "5月", "6月",
        "7月", "8月", "9月", "10月", "11月", "12月"
    };
    return (cm >= 1 && cm <= 12) ? names[cm] : "???";
}

// 日历月转中文（带季节/考试标注）
inline const char* getCalendarMonthLabel(int cm) {
    static const char* labels[] = {
        "", "1月", "2月", "3月·省选", "4月·期中", "5月·APIO", "6月·期末",
        "7月·NOI", "8月", "9月", "10月·CSP", "11月·NOIP", "12月"
    };
    return (cm >= 1 && cm <= 12) ? labels[cm] : "???";
}

// 9 个知识维度的键名列表
inline const std::vector<std::string> KNOWLEDGE_DIMS = {
    "dp", "ds", "string", "graph", "combinatorics",
    "math", "geometry", "data_structure", "adhoc"
};

// month 1-36 对应的日历月（7月起）
inline int monthToCalendarMonth(int month) {
    return ((month - 1) % 12) + 7 > 12 ? ((month - 1) % 12) + 7 - 12 : ((month - 1) % 12) + 7;
}

// month 1-36 对应学年
inline int monthToYear(int month) {
    return (month - 1) / 12 + 1;
}

// ========== 洛谷难度等级 ==========

// 洛谷难度等级定义
struct LuoguTier {
    const char* name;
    int minLevel;
    int maxLevel;
};

inline const std::vector<LuoguTier> LUOGU_TIERS = {
    {"入门",          1, 1},
    {"普及-",         2, 2},
    {"普及/提高-",    3, 3},
    {"普及+/提高",    4, 5},
    {"提高+/省选-",   5, 6},
    {"省选/NOI-",     7, 8},
    {"NOI/NOI+/CTSC", 9, 10},
};

// 难度等级 -> 洛谷标签名
inline const char* getLuoguTierName(int level) {
    if (level <= 1) return "入门";
    if (level <= 2) return "普及-";
    if (level <= 3) return "普及/提高-";
    if (level <= 5) return "普及+/提高";
    if (level <= 6) return "提高+/省选-";
    if (level <= 8) return "省选/NOI-";
    return "NOI/NOI+/CTSC";
}

// ========== 题目随机名字生成 ==========

inline std::string generateProblemName() {
    static const char* prefixes[] = {
        "小", "大", "超级", "神秘", "终极", "经典", "隐藏", "传奇", "终极"
    };
    static const char* subjects[] = {
        "猴子", "数列", "树", "图", "路径", "矩阵", "字符串", "方块",
        "宝石", "迷宫", "城堡", "王国", "花园", "宝藏", "密码", "信号",
        "桥梁", "铁路", "商店", "比赛", "任务", "游戏", "排队", "分糖"
    };
    static const char* suffixes[] = {
        "", "问题", "的烦恼", "的冒险", "之谜", "大作战", "的旅程",
        "的挑战", "复兴", "变换", "计数", "排序", "构造"
    };
    std::string name = prefixes[Utils::randomInt(0, 8)];
    name += subjects[Utils::randomInt(0, 23)];
    name += suffixes[Utils::randomInt(0, 12)];
    return name;
}

// ========== 自定义比赛难度模板 ==========

struct ContestTemplate {
    const char* name;
    int numProblems;
    std::vector<std::pair<int,int>> ranges; // 难度范围
    int timePoints;
    bool isIOI;
    int apCost; // AP 消耗（基础3 + 自定义额外1 = 4）
};

inline const std::vector<ContestTemplate> CONTEST_TEMPLATES = {
    {"CSP-S 难度",   4, {{2,3}, {3,4}, {4,5}, {5,6}}, 21, false, 4},
    {"NOIP 难度",    4, {{3,4}, {3,4}, {4,6}, {5,6}}, 24, false, 4},
    {"省选 难度",    6, {{4,5}, {5,6}, {6,7}, {6,7}, {7,8}, {8,9}}, 27, false, 4},
    {"NOI 难度",     6, {{5,7}, {6,7}, {7,8}, {7,8}, {8,9}, {9,10}}, 30, false, 4},
    {"IOI 难度",     6, {{7,8}, {7,9}, {8,9}, {8,10}, {9,10}, {10,10}}, 30, true, 4},
    {"CTSC 难度",    8, {{8,9}, {8,9}, {9,10}, {9,10}, {9,10}, {9,10}, {10,10}, {10,10}}, 36, true, 4},
};

#endif // TYPES_HPP
