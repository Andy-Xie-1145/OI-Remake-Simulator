#include "catch.hpp"
#include "../month_engine.hpp"

// —— 引擎级无头测试：不启动 GUI，直接驱动 MonthEngine 跑完整回合流程 ——

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

// 跳过本月剩余阶段（不打比赛、不做题），推进到下个月
void skipToNextMonth()
{
    Engine::endMonthActions();
    while (Engine::hasPhase()) {
        Engine::phaseFinished();
    }
}

} // namespace

TEST_CASE("startNewGame - 第1月为7月，无比赛考试", "[engine]")
{
    Utils::setSeed(42);
    freshGame();

    REQUIRE(gameState.currentMonth == 1);
    REQUIRE(Engine::monthInfo().calendarMonth == 7);
    REQUIRE(Engine::monthInfo().contestIds.empty());
    REQUIRE_FALSE(Engine::monthInfo().hasExam);
    REQUIRE_FALSE(Engine::hasPhase());          // 行动阶段，无排队阶段
    REQUIRE_FALSE(Engine::isGameOver());
    REQUIRE(gameState.ap == 8);                 // normal 难度 apBonus=0
}

TEST_CASE("doActivity - 扣除 AP 与健康并记录学习月份", "[engine]")
{
    Utils::setSeed(7);
    freshGame();

    const int ap0 = gameState.ap;
    const int hp0 = gameState.health;

    auto outcome = Engine::doActivity(Activity::Learn, 0 /*dp*/);
    REQUIRE(outcome.ok);
    REQUIRE(gameState.ap == ap0 - 2);           // 成本唯一真相：ACTIVITY_DEFS
    REQUIRE(gameState.health == hp0 - 1);
    REQUIRE(gameState.lastStudyMonth.count("dp") == 1);
    REQUIRE(gameState.lastStudyMonth["dp"] == gameState.currentMonth);
}

TEST_CASE("doActivity - AP 不足时拒绝执行且状态不变", "[engine]")
{
    Utils::setSeed(7);
    freshGame();
    gameState.ap = 0;

    const int hp0 = gameState.health;
    auto outcome = Engine::doActivity(Activity::Learn, 0);
    REQUIRE_FALSE(outcome.ok);
    REQUIRE(gameState.health == hp0);
    REQUIRE(gameState.ap == 0);
}

TEST_CASE("toggleTingke - 保持已消耗 AP 并提高上限", "[engine]")
{
    Utils::setSeed(7);
    freshGame();
    REQUIRE(Engine::doActivity(Activity::Rest).ok);   // 消耗 2 AP
    const int spentBefore = gameState.maxAp - gameState.ap;
    REQUIRE(spentBefore == 2);

    Engine::toggleTingke();

    REQUIRE(gameState.isTingke);
    REQUIRE(gameState.maxAp == 10);             // 停课 10 AP − 本月无扣减
    REQUIRE(gameState.ap == gameState.maxAp - spentBefore);
    // 心态代价由每月 Calendar::startMonth 统一结算，切换瞬间不变
}

TEST_CASE("endMonthActions - CSP 月入队 比赛→结算 两阶段", "[engine]")
{
    Utils::setSeed(7);
    freshGame();
    for (int m = 1; m < 4; ++m) {
        skipToNextMonth();
        REQUIRE(gameState.currentMonth == m + 1);
    }

    REQUIRE(gameState.currentMonth == 4);
    REQUIRE(Engine::monthInfo().calendarMonth == 10);

    Engine::endMonthActions();
    REQUIRE(Engine::hasPhase());
    REQUIRE(Engine::currentPhase().kind == Engine::Phase::Kind::Contest);
    REQUIRE(Engine::currentPhase().contestId == 1);   // CSP-S

    Engine::phaseFinished();
    REQUIRE(Engine::hasPhase());
    REQUIRE(Engine::currentPhase().kind == Engine::Phase::Kind::Settlement);

    Engine::phaseFinished();
    REQUIRE(gameState.currentMonth == 5);
}

TEST_CASE("settleMonth - 每月恰好结算一次（回归：考试月不再双重结算）", "[engine]")
{
    Utils::setSeed(7);
    freshGame("normal");
    gameState.playerStats.culture = 0;      // 收入不含文化加成，便于断言
    gameState.playerStats.cspScore = 250;   // 保证第 5 月有 NOIP 资格

    // 推进到第 5 月（11 月，NOIP + 期中）
    for (int m = 1; m < 5; ++m) skipToNextMonth();
    REQUIRE(gameState.currentMonth == 5);
    REQUIRE(Engine::monthInfo().hasExam);

    const int moneyBefore = gameState.money;
    Engine::endMonthActions();              // 唯一的结算点
    const int income = gameState.money - moneyBefore;
    REQUIRE(income >= 0);

    // 结算事实恰好一条经济类记录
    int economyFacts = 0;
    for (const auto& f : gameState.settlementFacts)
        if (f.cat == SettlementFact::Cat::Economy) ++economyFacts;
    REQUIRE(economyFacts == 1);

    // 消费阶段队列（比赛/考试/结算）不得再次结算 —— 回归断言：
    // 旧实现在考试结束按钮里再调一次 settleMonth，收入会被加两次。
    while (Engine::hasPhase()) Engine::phaseFinished();
    REQUIRE(gameState.currentMonth == 6);
    REQUIRE(gameState.money == moneyBefore + income);
}

TEST_CASE("examFinished - 只记录成绩，不触发结算", "[engine]")
{
    Utils::setSeed(7);
    freshGame();
    gameState.playerStats.culture = 0;
    gameState.playerStats.cspScore = 250;

    for (int m = 1; m < 5; ++m) skipToNextMonth();
    REQUIRE(gameState.currentMonth == 5);

    Engine::endMonthActions();
    const int moneyAtSettle = gameState.money;
    REQUIRE(Engine::currentPhase().kind == Engine::Phase::Kind::Contest);
    Engine::phaseFinished();                       // 跳过 NOIP
    REQUIRE(Engine::currentPhase().kind == Engine::Phase::Kind::Exam);

    const size_t recordsBefore = gameState.examRecords.size();
    CultureExam::start(false);
    Engine::examFinished(88, 100, false);
    REQUIRE(gameState.examRecords.size() == recordsBefore + 1);
    REQUIRE(gameState.money == moneyAtSettle);     // 无第二次结算
}

TEST_CASE("settlementFacts - 类型化分类齐全", "[engine]")
{
    Utils::setSeed(7);
    freshGame();

    // 推进到第 3 月
    for (int m = 1; m <= 2; ++m) skipToNextMonth();
    REQUIRE(gameState.currentMonth == 3);

    // 制造遗忘条件：dp 上次学习在第 1 月（距今 2 个月）
    gameState.playerStats.dp = 3;
    gameState.lastStudyMonth["dp"] = 1;

    Engine::endMonthActions();                     // 第 3 月的结算

    bool hasEconomy = false;
    bool hasKnowledge = false;
    for (const auto& f : gameState.settlementFacts) {
        hasEconomy |= (f.cat == SettlementFact::Cat::Economy);
        hasKnowledge |= (f.cat == SettlementFact::Cat::Knowledge);
    }
    REQUIRE(hasEconomy);                           // 零花钱入账
    REQUIRE(hasKnowledge);                         // dp 遗忘被归类为知识

    while (Engine::hasPhase()) Engine::phaseFinished();
    REQUIRE(gameState.currentMonth == 4);          // 阶段走完才翻页
}

TEST_CASE("buyShopItem - 扣钱、生效并按难度涨价", "[engine]")
{
    Utils::setSeed(7);
    freshGame("normal");
    gameState.money = 500;
    gameState.currentShopPrices["思维提升"] = 100;

    auto options = buildShopOptions();
    const EventOption* thinkOpt = nullptr;
    for (const auto& o : options)
        if (o.text == "思维提升") { thinkOpt = &o; break; }
    REQUIRE(thinkOpt != nullptr);

    const int think0 = gameState.playerStats.thinking;
    REQUIRE(Engine::buyShopItem(*thinkOpt));
    REQUIRE(gameState.playerStats.thinking == std::min(20, think0 + 1));
    REQUIRE(gameState.money == 400);
    REQUIRE(gameState.purchasedItems.count("思维提升") == 1);
    REQUIRE(gameState.currentShopPrices["思维提升"] > 100);   // 涨价
}

TEST_CASE("soak - 固定种子随机游玩至游戏结束不崩溃", "[engine][soak]")
{
    Utils::setSeed(2026);
    freshGame("normal");

    int guard = 0;
    while (!Engine::isGameOver() && guard++ < 2000)
    {
        if (gameState.ap >= 2) {
            Engine::doActivity(static_cast<Activity::Type>(Utils::randomInt(0, 5)),
                               Utils::randomInt(0, 8));
            if (Engine::currentContestIsActivity()) {
                // 活动赛直接收尾（未真正打完也允许走流程）
                Engine::activityContestFinished(0);
            }
        } else {
            skipToNextMonth();
        }
        REQUIRE(gameState.currentMonth <= 36);
    }
    REQUIRE(Engine::isGameOver());                 // 36 月内必然结束或健康归零
}
