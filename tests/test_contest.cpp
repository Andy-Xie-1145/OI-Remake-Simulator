#include "catch.hpp"
#include "../game.hpp"
#include "../contest.hpp"

static void resetGameState()
{
    gameState = GameState{};
    gameState.debugmode = false;
    gameState.mood = 15;
    gameState.playerStats.dp = 5;
    gameState.playerStats.ds = 5;
    gameState.playerStats.string = 5;
    gameState.playerStats.graph = 5;
    gameState.playerStats.combinatorics = 5;
    gameState.playerStats.thinking = 5;
    gameState.playerStats.coding = 0;
    gameState.playerStats.quickness = 2;
    gameState.playerStats.carefulness = 3;
    gameState.playerStats.luck = 3;
    gameState.playerStats.experience = 1;
}

TEST_CASE("calculateThinkTime - 知识刚好匹配", "[contest]")
{
    resetGameState();

    SubProblem sp;
    sp.dp = 5;
    sp.ds = 5;
    sp.str = 5;
    sp.graph = 5;
    sp.comb = 5;
    sp.adhoc = 2;

    int result = Contest::calculateThinkTime(sp);
    REQUIRE(result == 8);
}

TEST_CASE("calculateThinkTime - DP知识为零", "[contest]")
{
    resetGameState();

    SubProblem sp;
    sp.dp = 5;
    sp.ds = 5;
    sp.str = 5;
    sp.graph = 5;
    sp.comb = 5;
    sp.adhoc = 0;

    gameState.playerStats.dp = 0;
    int resultZeroDp = Contest::calculateThinkTime(sp);
    REQUIRE(resultZeroDp == 10);
}

TEST_CASE("calculateCodeTime - 正常情况", "[contest]")
{
    resetGameState();

    SubProblem sp;
    sp.coding = 5;

    int result = Contest::calculateCodeTime(sp);
    REQUIRE(result == 3);
}

TEST_CASE("calculateCodeTime - 迅捷为0", "[contest]")
{
    resetGameState();

    SubProblem sp;
    sp.coding = 5;
    gameState.playerStats.quickness = 0;

    int result = Contest::calculateCodeTime(sp);
    REQUIRE(result == 5);
}

TEST_CASE("calculateThinkSuccessRate - 思考成功率上限", "[contest]")
{
    resetGameState();

    SubProblem sp;
    sp.thinking = 5;

    double result = Contest::calculateThinkSuccessRate(sp);
    REQUIRE(result == 0.95);
}

TEST_CASE("calculateCodeSuccessRate - 代码成功率", "[contest]")
{
    resetGameState();

    SubProblem sp;
    sp.detail = 5;

    double result = Contest::calculateCodeSuccessRate(sp);
    REQUIRE(result == 0.75);
}

TEST_CASE("calculateLuckReduction - 幸运减免", "[contest]")
{
    resetGameState();
    gameState.playerStats.luck = 3;

    double result = Contest::calculateLuckReduction();
    REQUIRE(result > 0.0);
    REQUIRE(result < 0.45);
}

TEST_CASE("calculateErrorRate - 有陷阱时错误率", "[contest]")
{
    resetGameState();

    SubProblem sp;
    sp.trap = 2;

    double result = Contest::calculateErrorRate(sp);
    double expected = 0.11 * (1.0 - Contest::calculateLuckReduction());
    REQUIRE(result == Approx(expected).margin(0.001));
}

TEST_CASE("calculateErrorRate - 无陷阱", "[contest]")
{
    resetGameState();

    SubProblem sp;
    sp.trap = 0;

    double result = Contest::calculateErrorRate(sp);
    REQUIRE(result >= 0.0);
    REQUIRE(result <= 0.8);
}

TEST_CASE("getBaseBlurLevel - 模糊等级直接映射", "[contest]")
{
    Problem prob;
    SubProblem sp;
    sp.blur = 3;

    int result = Contest::getBaseBlurLevel(prob, sp);
    REQUIRE(result == 3);
}

TEST_CASE("getBaseBlurLevel - 无模糊", "[contest]")
{
    Problem prob;
    SubProblem sp;
    sp.blur = 0;

    int result = Contest::getBaseBlurLevel(prob, sp);
    REQUIRE(result == 0);
}

TEST_CASE("getEffectiveBlurLevel - 经验减免模糊", "[contest]")
{
    resetGameState();
    gameState.playerStats.experience = 2;

    Problem prob;
    SubProblem sp;
    sp.blur = 3;

    int result = Contest::getEffectiveBlurLevel(prob, sp);
    REQUIRE(result == 1);
}

TEST_CASE("joinDisplayParts - 正常拼接", "[contest]")
{
    std::vector<std::string> parts = {"A", "B", "C"};
    REQUIRE(Contest::joinDisplayParts(parts, ", ") == "A, B, C");
}

TEST_CASE("joinDisplayParts - 单个", "[contest]")
{
    std::vector<std::string> single = {"hello"};
    REQUIRE(Contest::joinDisplayParts(single, ", ") == "hello");
}

TEST_CASE("joinDisplayParts - 空向量", "[contest]")
{
    std::vector<std::string> empty;
    REQUIRE(Contest::joinDisplayParts(empty, ", ") == "");
}