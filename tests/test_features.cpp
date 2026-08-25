#include "catch.hpp"
#include "../month_engine.hpp"
#include "../save_system.hpp"

// —— v0.3.0 新功能：G 存档 / C 结局矩阵 / D 专题 / E 生活节奏 ——

namespace {

void freshGame(const std::string& difficulty = "normal")
{
    gameState = GameState{};
    gameState.gameDifficulty = difficulty;
    Engine::hardReset();
    initGame();
    gameState.moodCap = MOOD_LIMIT;
    Engine::startNewGame();
}

} // namespace

// ============================ 状态重置 ============================

TEST_CASE("initGame - 重开时清理全部 v0.3.0 状态（回归：专题/熬夜/病倒残留）", "[reset]")
{
    Utils::setSeed(7);
    freshGame();

    // 模拟上一局结束时的残留状态
    Topics::accept(2);                        // 字符串专题「进行中」
    REQUIRE(Topics::active() != nullptr);
    gameState.topicProgress = 3;
    gameState.masteredTopics.insert(5);
    gameState.carefulChecks = 4;
    gameState.isAoYe = true;
    gameState.sickNext = true;
    gameState.exerciseCountThisMonth = 3;

    // 「开始新游戏」路径：不经存档，BeginSetup -> initGame
    Engine::hardReset();
    initGame();
    Engine::startNewGame();

    CHECK(Topics::active() == nullptr);       // 专题已自动清理
    CHECK(gameState.topicId == Topics::INVALID);
    CHECK(gameState.topicStartMonth == 0);
    CHECK(gameState.topicProgress == 0);
    CHECK(gameState.masteredTopics.empty());  // 精通不跨局继承
    CHECK(gameState.carefulChecks == 0);
    CHECK_FALSE(gameState.isAoYe);
    CHECK_FALSE(gameState.sickNext);
    CHECK(gameState.exerciseCountThisMonth == 0);

    // 新一局首个结算不得误判专题逾期扣心态
    Engine::endMonthActions();
    while (Engine::hasPhase()) Engine::phaseFinished();
    for (const auto& f : gameState.settlementFacts)
        CHECK(f.text.find("逾期") == std::string::npos);
}

// ============================ G · 存档系统 ============================

TEST_CASE("save - 写入后读回，关键字段一致", "[save]")
{
    Utils::setSeed(11);
    freshGame();

    gameState.money = 777;
    gameState.mood = 9;
    gameState.ap = 5;
    gameState.maxAp = 10;
    gameState.playerStats.cspScore = 233;
    gameState.playerStats.isProvincialTeam = true;
    gameState.playerStats.achievements.push_back("省队选手");
    gameState.traits.push_back("lucky_star");
    gameState.masteredTopics.insert(3);
    gameState.topicId = 1;
    gameState.topicProgress = 2;
    gameState.exerciseCountThisMonth = 2;
    gameState.currentShopPrices["思维提升"] = 123;
    gameState.lastStudyMonth["dp"] = 4;
    logEvent("测试日志一行", "event");

    REQUIRE(Save::write());

    // 破坏现场后读回
    freshGame();
    REQUIRE(gameState.money == 300);   // 已被重置

    REQUIRE(Save::read());
    CHECK(gameState.money == 777);
    CHECK(gameState.mood == 9);
    CHECK(gameState.ap == 5);
    CHECK(gameState.maxAp == 10);
    CHECK(gameState.playerStats.cspScore == 233);
    CHECK(gameState.playerStats.isProvincialTeam == true);
    CHECK(gameState.playerStats.achievements.size() == 1);
    CHECK(gameState.playerStats.achievements[0] == "省队选手");
    CHECK(gameState.traits.size() == 1);
    CHECK(gameState.traits[0] == "lucky_star");
    CHECK(gameState.masteredTopics.count(3) == 1);
    CHECK(gameState.topicId == 1);
    CHECK(gameState.topicProgress == 2);
    CHECK(gameState.exerciseCountThisMonth == 2);
    CHECK(gameState.currentShopPrices["思维提升"] == 123);
    CHECK(gameState.lastStudyMonth["dp"] == 4);

    bool foundLog = false;
    for (const auto& lg : gameState.gameLog)
        if (lg.find("测试日志一行") != std::string::npos) foundLog = true;  // logEvent 会加前缀
    CHECK(foundLog);

    // 引擎状态恢复：可继续正常推进
    REQUIRE_FALSE(Engine::isGameOver());
}

TEST_CASE("save - 魔数不符时拒绝读取", "[save]")
{
    REQUIRE_FALSE(Save::deserialize("NOTASAVE 3\nver=3\n"));
    REQUIRE_FALSE(Save::deserialize("OISAVE 999\nver=999\n"));
}

// ============================ C · 结局矩阵与保送 ============================

namespace {

// 构造一次高考成绩记录
void pushGaokao(int score, int maxScore)
{
    GameState::ExamRecord r;
    r.month = 36; r.calendarMonth = 6;
    r.score = score; r.maxScore = maxScore; r.isGaokao = true;
    gameState.examRecords.push_back(r);
}

} // namespace

TEST_CASE("ending - 结局矩阵各分支", "[ending]")
{
    freshGame();

    // 传奇：IOI 金牌
    gameState.playerStats.isIOIgold = true;
    CHECK(Ending::resolveCompleted36().title == "世界之巅");
    gameState.playerStats.isIOIgold = false;

    // 保送：无视高考档位
    gameState.playerStats.isBaosong = true;
    pushGaokao(30, 100);   // 即便高考很差
    CHECK(Ending::resolveCompleted36().title == "保送上岸");

    // 省队 + 高考卓越（≥90%）→ 强基无忧
    gameState.playerStats.isBaosong = false;
    gameState.playerStats.isProvincialTeam = true;
    gameState.examRecords.clear();
    pushGaokao(92, 100);
    CHECK(Ending::resolveCompleted36().title == "强基无忧");

    // 无省队 + 高考卓越 → 文化课之神
    gameState.playerStats.isProvincialTeam = false;
    CHECK(Ending::resolveCompleted36().title == "文化课之神");

    // 无省队 + 平平（<70%）→ 复读的十字路口
    gameState.examRecords.clear();
    pushGaokao(50, 100);
    CHECK(Ending::resolveCompleted36().title == "复读的十字路口");

    // 健康归零结局独立于矩阵
    CHECK(Ending::resolveHealthDeath().title == "积劳成疾");
}

TEST_CASE("baosong - 庆功月替换高考，文化课压力解除", "[ending]")
{
    Utils::setSeed(7);
    freshGame();
    REQUIRE_FALSE(gameState.playerStats.isBaosong);

    // 高三 6 月（第 36 月）原本是高考
    auto normalInfo = Calendar::getMonthInfo(36);
    CHECK(normalInfo.isGaokao);
    CHECK(normalInfo.hasExam);

    // 保送后：庆功月
    gameState.playerStats.isBaosong = true;
    auto freedInfo = Calendar::getMonthInfo(36);
    CHECK_FALSE(freedInfo.isGaokao);
    CHECK_FALSE(freedInfo.hasExam);
    CHECK(freedInfo.apDeduction == 0);

    // 结算不再有「文化课薄弱」惩罚（culture=0）
    gameState.playerStats.isBaosong = true;
    gameState.playerStats.culture = 0;
    gameState.currentMonth = 2;
    Calendar::startMonth(2);
    Engine::endMonthActions();
    for (const auto& f : gameState.settlementFacts)
        CHECK(f.text.find("文化课薄弱") == std::string::npos);
}

// ============================ D · 专题任务 ============================

TEST_CASE("topic - 学习匹配维度 4 次完成并获得精通", "[topics]")
{
    Utils::setSeed(7);
    freshGame();

    Topics::accept(0);   // DP 专题
    REQUIRE(Topics::active() != nullptr);

    for (int i = 0; i < 4; ++i) {
        REQUIRE(gameState.ap >= 2);
        auto out = Engine::doActivity(Activity::Learn, 0);
        REQUIRE(out.ok);
        if (!Topics::active()) break;   // 第 4 次即完成
    }
    CHECK(Topics::active() == nullptr);
    CHECK(Topics::hasMastery(0));

    // 精通加成：同一属性状态下，有/无精通对照（思考时间 -1，下限 1）
    SubProblem sp;
    sp.dp = 5; sp.adhoc = 0;
    const int withMastery = Contest::calculateThinkTime(sp);
    gameState.masteredTopics.erase(0);
    const int withoutMastery = Contest::calculateThinkTime(sp);
    CHECK(withMastery == std::max(1, withoutMastery - 1));
}

TEST_CASE("topic - 逾期未完成为失败（关闭专题并留下事实）", "[topics]")
{
    Utils::setSeed(7);
    freshGame();
    Topics::accept(3);                       // 图论专题
    gameState.topicStartMonth = gameState.currentMonth - 3;   // 立即逾期

    const int moodBefore = gameState.mood;
    Engine::endMonthActions();

    CHECK(gameState.topicId == Topics::INVALID);
    CHECK(gameState.mood < moodBefore);      // 心态净下降（含逾期 -2）
    bool hasFailFact = false;
    for (const auto& f : gameState.settlementFacts)
        if (f.cat == SettlementFact::Cat::Other) hasFailFact = true;
    CHECK(hasFailFact);
}

TEST_CASE("topic - 细心专题凭历史对拍直接完成", "[topics]")
{
    Utils::setSeed(7);
    freshGame();
    gameState.carefulChecks = 10;
    Topics::accept(10);
    CHECK(Topics::active() == nullptr);
    CHECK(Topics::hasMastery(10));
}

// ============================ E · 生活节奏 ============================

TEST_CASE("exercise - 1AP 换 2 点健康并累计次数", "[exercise]")
{
    Utils::setSeed(7);
    freshGame();
    const int ap0 = gameState.ap;
    const int hp0 = gameState.health;

    auto out = Engine::doActivity(Activity::Exercise);
    REQUIRE(out.ok);
    CHECK(gameState.ap == ap0 - 1);
    CHECK(gameState.health == hp0 + 2);
    CHECK(gameState.exerciseCountThisMonth == 1);
}

TEST_CASE("aoye - 熬夜加 AP、与停课互斥、结算扣健康", "[exercise]")
{
    Utils::setSeed(7);
    freshGame();

    // 正常开启：maxAp 8 -> 10
    Engine::toggleAoYe();
    CHECK(gameState.isAoYe);
    CHECK(gameState.maxAp == 10);

    // 停课被拒绝（互斥）
    Engine::toggleTingke();
    CHECK_FALSE(gameState.isTingke);

    // 结算：健康净变化 = 自然回复+2 与睡眠回复，再 -3 熬夜代价
    gameState.ap = 0;   // 不让未用 AP 参与睡觉回复，便于精确断言
    const int hpBefore = gameState.health;
    const int expectAfter = std::max(0, std::min(20, hpBefore + 2) - 3);
    Engine::endMonthActions();
    CHECK(gameState.health == expectAfter);
    bool hasAoyeFact = false;
    for (const auto& f : gameState.settlementFacts)
        if (f.text.find("熬夜冲刺") != std::string::npos) hasAoyeFact = true;
    CHECK(hasAoyeFact);

    // 关闭熬夜后停课可用
    Engine::toggleAoYe();
    CHECK_FALSE(gameState.isAoYe);
    Engine::toggleTingke();
    CHECK(gameState.isTingke);
}

TEST_CASE("illness - 低健康时有概率病倒且下月生效", "[exercise]")
{
    Utils::setSeed(99);
    freshGame();
    gameState.ap = 0;                 // 排除睡觉回复干扰
    gameState.health = 2;             // 结算自然回复 +2 后恰好 ≤4
    gameState.playerStats.luck = 0;   // 概率取上限 25%
    gameState.exerciseCountThisMonth = 0;

    int sickCount = 0;
    for (int i = 0; i < 60; ++i) {
        // 每轮都把现场拉回「结算后健康恰好 ≤4」的临界状态
        gameState.health = 2;
        gameState.ap = 0;
        gameState.sickNext = false;
        Engine::endMonthActions();    // 结算：自然回复+2 → 健康4，随后病倒判定
        if (gameState.sickNext) ++sickCount;
        if (Engine::isGameOver()) break;
        gameState.currentMonth -= 1;  // 回退月份以便重复采样
    }
    CHECK(sickCount > 0);
    CHECK(sickCount < 60);

    // 病倒生效：新月开始时消耗（AP 预算 -2、心态 -1）
    gameState.sickNext = true;
    const int moodBefore = gameState.mood;
    Calendar::startMonth(gameState.currentMonth + 1);
    CHECK(gameState.sickNext == false);   // 已消费
    CHECK(gameState.mood == std::max(0, moodBefore - 1));
}
