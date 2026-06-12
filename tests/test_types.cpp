#include "catch.hpp"
#include "../types.hpp"
#include <cstring>

TEST_CASE("monthToYear - 月份转学年", "[types]")
{
    REQUIRE(monthToYear(1) == 1);
    REQUIRE(monthToYear(9) == 1);
    REQUIRE(monthToYear(12) == 1);
    REQUIRE(monthToYear(13) == 2);
    REQUIRE(monthToYear(24) == 2);
    REQUIRE(monthToYear(25) == 3);
    REQUIRE(monthToYear(36) == 3);
}

TEST_CASE("monthToCalendarMonth - 月份转日历月", "[types]")
{
    REQUIRE(monthToCalendarMonth(1) == 7);
    REQUIRE(monthToCalendarMonth(9) == 3);
    REQUIRE(monthToCalendarMonth(12) == 6);
    REQUIRE(monthToCalendarMonth(13) == 7);
    REQUIRE(monthToCalendarMonth(14) == 8);
    REQUIRE(monthToCalendarMonth(18) == 12);
    REQUIRE(monthToCalendarMonth(19) == 1);
}

TEST_CASE("getLuoguTierName - 洛谷难度名称", "[types]")
{
    REQUIRE(std::strcmp(getLuoguTierName(1), "入门") == 0);
    REQUIRE(std::strcmp(getLuoguTierName(3), "普及/提高-") == 0);
    REQUIRE(std::strcmp(getLuoguTierName(5), "普及+/提高") == 0);
    REQUIRE(std::strcmp(getLuoguTierName(8), "省选/NOI-") == 0);
    REQUIRE(std::strcmp(getLuoguTierName(9), "NOI/NOI+/CTSC") == 0);
}

TEST_CASE("Utils::getStatName - 属性名映射", "[types]")
{
    REQUIRE(Utils::getStatName("dp") == "动态规划");
    REQUIRE(Utils::getStatName("ds") == "数据结构");
    REQUIRE(Utils::getStatName("string") == "字符串");
    REQUIRE(Utils::getStatName("graph") == "图论");
    REQUIRE(Utils::getStatName("combinatorics") == "组合计数");
    REQUIRE(Utils::getStatName("math") == "数学");
    REQUIRE(Utils::getStatName("geometry") == "几何");
    REQUIRE(Utils::getStatName("data_structure") == "高级数据结构");
    REQUIRE(Utils::getStatName("adhoc") == "构造/思维");
    REQUIRE(Utils::getStatName("unknown_key") == "unknown_key");
}

TEST_CASE("Utils::mapAttributeValue - 属性值映射", "[types]")
{
    REQUIRE(Utils::mapAttributeValue(0) == 0);
    REQUIRE(Utils::mapAttributeValue(1) == 1);
    REQUIRE(Utils::mapAttributeValue(2) == 2);
    REQUIRE(Utils::mapAttributeValue(3) == 3);
    REQUIRE(Utils::mapAttributeValue(4) == 3);
    REQUIRE(Utils::mapAttributeValue(5) == 4);
    REQUIRE(Utils::mapAttributeValue(6) == 4);
    REQUIRE(Utils::mapAttributeValue(8) == 5);
    REQUIRE(Utils::mapAttributeValue(10) == 6);
    REQUIRE(Utils::mapAttributeValue(12) == 7);
    REQUIRE(Utils::mapAttributeValue(15) == 9);
    REQUIRE(Utils::mapAttributeValue(20) == 10);
}