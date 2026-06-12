#include "catch.hpp"
#include "../game.hpp"

TEST_CASE("difficultyMultiplier - 难度倍数", "[game]")
{
    gameState.gameDifficulty = "easy";
    REQUIRE(difficultyMultiplier() == 0.8);

    gameState.gameDifficulty = "normal";
    REQUIRE(difficultyMultiplier() == 0.9);

    gameState.gameDifficulty = "hard";
    REQUIRE(difficultyMultiplier() == 1.0);

    gameState.gameDifficulty = "expert";
    REQUIRE(difficultyMultiplier() == 1.1);
}

TEST_CASE("getMoodEfficiency - 心态效率(高心态)", "[game]")
{
    gameState.mood = 15;
    gameState.isAnxious = false;
    double eff15 = getMoodEfficiency();
    REQUIRE(eff15 == 1.5);
}

TEST_CASE("getMoodEfficiency - 心态效率(高心态=10)", "[game]")
{
    gameState.mood = 10;
    gameState.isAnxious = false;
    double eff10 = getMoodEfficiency();
    REQUIRE(eff10 == 1.5);
}

TEST_CASE("getMoodEfficiency - 心态效率(低心态=5)", "[game]")
{
    gameState.mood = 5;
    gameState.isAnxious = false;
    double eff5 = getMoodEfficiency();
    REQUIRE(eff5 == Approx(5.0 / 6.0).margin(0.01));
}

TEST_CASE("getMoodEfficiency - 心态效率(极低心态=2)", "[game]")
{
    gameState.mood = 2;
    gameState.isAnxious = false;
    double eff2 = getMoodEfficiency();
    REQUIRE(eff2 == 0.5);
}

TEST_CASE("getMoodEfficiency - 焦虑时效率降低", "[game]")
{
    gameState.mood = 15;
    gameState.isAnxious = false;
    double effNoAnx = getMoodEfficiency();

    gameState.isAnxious = true;
    double effAnx = getMoodEfficiency();
    REQUIRE(effAnx <= effNoAnx);
}

TEST_CASE("calculateMonthlyIncome - 月收入非负", "[game]")
{
    gameState.playerStats.culture = 5;
    gameState.playerStats.luck = 3;
    gameState.currentMonth = 6;
    gameState.money = 0;
    gameState.gameDifficulty = "normal";

    int income = calculateMonthlyIncome();
    REQUIRE(income >= 0);
}

TEST_CASE("getMinKnowledge - 最小知识维度非负", "[game]")
{
    gameState.playerStats.dp = 3;
    gameState.playerStats.ds = 5;
    gameState.playerStats.string = 2;
    gameState.playerStats.graph = 4;
    gameState.playerStats.combinatorics = 6;
    gameState.playerStats.math = 3;
    gameState.playerStats.geometry = 5;
    gameState.playerStats.data_structure = 4;
    gameState.playerStats.adhoc = 7;

    int minVal = getMinKnowledge();
    REQUIRE(minVal >= 0);
}