// ============================================================================
//  social.hpp  —  机房伙伴 / 教练 / 宿敌（v0.4.0）
//
//  层级：位于 game/events 之上、engine/contest 之下（与 topics.hpp 同构，
//  可被 contest.hpp 叶子包含，不产生环）。
//
//  设计要点（v0.4.0 评审定稿）：
//  · 固定 3 位伙伴，开局随机生成（姓名库随机、专精随机、挚友效果随机互异）
//  · 高三开学每人 20% 概率退环境；若全部触发则随机强留一人
//  · 渐进揭示：专精始终可见；挚友效果 ≥40 揭示；知己（≥85）解锁「倾诉」
//  · 教练：获奖 +10、参训 +5、文化薄弱月 -2；≥40 停课心态惩罚减 1，≥70 特训
//  · 宿敌：每场正式比赛按玩家预期分×强度系数模拟得分并播报对比；
//    输 → 心态-2 且获得 2 个月「知耻后勇」（效率×1.1）；NOI 决胜压过 → 成就
// ============================================================================
#ifndef SOCIAL_HPP
#define SOCIAL_HPP

#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <vector>

namespace Social {

constexpr int REL_FRIEND = 40;     // 挚友阈值（效果揭示 + 生效）
constexpr int REL_CONFIDANT = 85;  // 知己阈值（解锁「倾诉」）
constexpr double GONE_CHANCE_Y3 = 0.20;

// ---------- 姓名库 ----------
inline const char* SURNAMES[] = {
    "陈", "林", "苏", "周", "沈", "顾", "江", "赵", "许", "韩",
    "秦", "陆", "宋", "叶", "程", "钟", "孟", "方", "杜", "贺",
};
inline const char* GIVEN[] = {
    "默", "小满", "远航", "知远", "亦晨", "书航", "雨桐", "思齐", "望舒", "砚秋",
    "云帆", "星野", "清越", "景行", "若愚", "明澈", "疏影", "既白", "南絮", "北辰",
    "听澜", "折柳", "照野", "拾光", "未名", "青崖", "拂衣", "观棋", "枕流", "问天",
};

// 挚友效果元数据
struct PerkInfo {
    SocialPerk id;
    const char* name;
};
inline const PerkInfo PERKS[] = {
    {SocialPerk::CodeReview,  "代码复查"},
    {SocialPerk::Inspiration, "灵感碰撞"},
    {SocialPerk::Notebook,    "错题本"},
    {SocialPerk::StudyBuddy,  "补习互助"},
    {SocialPerk::MoodAnchor,  "心态锚"},
};
inline const char* perkName(int perkId) {
    for (const auto& p : PERKS)
        if ((int)p.id == perkId) return p.name;
    return "???";
}
inline std::string perkDesc(int perkId) {
    switch ((SocialPerk)perkId) {
    case SocialPerk::CodeReview:  return "比赛中「代码bug」事件概率 ×0.7";
    case SocialPerk::Inspiration: return "「灵光一闪」事件概率 +1%";
    case SocialPerk::Notebook:    return "对拍失败后修改代码所需次数 -1";
    case SocialPerk::StudyBuddy:  return "学文化课时文化课收益额外 +1";
    case SocialPerk::MoodAnchor:  return "焦虑发作概率 ×0.85";
    }
    return "";
}

inline std::string randomName() {
    return std::string(SURNAMES[Utils::randomInt(0, 19)]) +
           GIVEN[Utils::randomInt(0, 29)];
}

// 是否存在在世且达到挚友的伙伴持有该效果
inline bool hasFriendPerk(SocialPerk p) {
    for (const auto& c : gameState.companions)
        if (!c.gone && c.relation >= REL_FRIEND && c.perkId == (int)p) return true;
    return false;
}

// ---------- 开局生成 ----------
inline void generateRoster() {
    gameState.companions.clear();

    // 专精维度：0..8 不重复抽取
    std::vector<int> dims = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    std::shuffle(dims.begin(), dims.end(), gen);
    // 挚友效果：0..4 不重复抽取
    std::vector<int> perks = {0, 1, 2, 3, 4};
    std::shuffle(perks.begin(), perks.end(), gen);

    auto usedNames = std::set<std::string>();
    auto rollUniqueName = [&]() {
        std::string n;
        do { n = randomName(); } while (usedNames.count(n));
        usedNames.insert(n);
        return n;
    };

    for (int i = 0; i < 3; ++i) {
        CompanionNpc c;
        c.name = rollUniqueName();
        c.dimIndex = dims[i];
        c.perkId = perks[i];
        c.relation = Utils::randomInt(10, 25);
        gameState.companions.push_back(c);
        logEvent("认识了机房同学：" + c.name +
                 "（擅长" + Utils::getStatName(KNOWLEDGE_DIMS[dims[i]]) + "）", "event");
    }

    gameState.rival = RivalState{};
    do { gameState.rival.name = rollUniqueName(); } while (false);
    gameState.rival.baseFactor = Utils::randomDouble(0.90, 1.15);
}

// ---------- 关系 ----------
inline int tierOf(int relation) {
    if (relation >= REL_CONFIDANT) return 2;  // 知己
    if (relation >= REL_FRIEND) return 1;     // 挚友
    return 0;
}
inline const char* tierName(int t) {
    return t == 2 ? "知己" : (t == 1 ? "挚友" : "同学");
}

// 「机房闲聊」：关系成长 + 小增益 + 知己倾诉；返回日志
inline std::vector<std::string> chat(int companionIndex) {
    std::vector<std::string> logs;
    if (companionIndex < 0 || companionIndex >= (int)gameState.companions.size())
        return logs;
    auto& c = gameState.companions[companionIndex];
    if (c.gone) { logs.push_back(c.name + " 已经不在机房了……"); return logs; }

    const int tierBefore = tierOf(c.relation);
    int gain = Utils::randomInt(4, 7);
    c.relation = std::min(100, c.relation + gain);
    logs.push_back("和 " + c.name + " 聊了一会（关系 +" + std::to_string(gain) + "）");

    const int tierAfter = tierOf(c.relation);
    if (tierAfter > tierBefore)
        logs.push_back(std::string("你们的关系升级为「") + tierName(tierAfter) + "」！");

    // 30% 小增益
    if (Utils::randomBool(0.30)) {
        int kind = Utils::randomInt(0, 2);
        if (kind == 0) {
            applyStatDelta(KNOWLEDGE_DIMS[c.dimIndex], 1, "伙伴互助");
            logs.push_back(c.name + " 分享了" + Utils::getStatName(KNOWLEDGE_DIMS[c.dimIndex]) +
                           "的心得（+1）");
        } else if (kind == 1) {
            applyStatDelta("mood", 1, "伙伴谈心");
            logs.push_back("聊完心情好了不少（心态 +1）");
        } else {
            logs.push_back("聊得很投机，但没聊出什么干货");
        }
    }

    // 知己「倾诉」：焦虑状态下自动触发，每学年一次
    if (gameState.isAnxious && c.relation >= REL_CONFIDANT && !c.ventUsedThisYear) {
        gameState.isAnxious = false;
        gameState.anxietyMonths = 0;
        c.ventUsedThisYear = true;
        logs.push_back("你向 " + c.name + " 倾诉了最近的压力……焦虑消散了。（本学年倾诉已使用）");
    }
    return logs;
}

// ---------- 教练 ----------
inline void adjustCoach(int delta) {
    int before = gameState.coachRelation;
    gameState.coachRelation = std::max(0, std::min(100, gameState.coachRelation + delta));
    if (before < 40 && gameState.coachRelation >= 40)
        logEvent("教练开始信任你了：停课的心态惩罚减轻。", "event");
    if (before < 70 && gameState.coachRelation >= 70)
        logEvent("教练关系深厚：解锁「教练特训」，集训效果增强。", "event");
}
inline int tingkeMoodCost() {
    return gameState.coachRelation >= 40 ? 2 : 3;
}
inline bool coachEliteTraining() { return gameState.coachRelation >= 70; }

// ---------- 宿敌 ----------
// 正式比赛结束后调用：模拟宿敌得分、播报对比、结算胜负。
// 模板化以解除对 Contest::ContestResultView 的编译期依赖（鸭子类型：
// 需要成员 contestName / expectedTotal / actualTotal）。
template <typename ViewT>
inline void onOfficialContestFinished(const ViewT& view) {
    const double factor = std::max(0.80, std::min(1.25,
        gameState.rival.baseFactor + Utils::randomDouble(-0.05, 0.05)));
    const int rivalScore = (int)std::lround(view.expectedTotal * factor);
    const bool win = view.actualTotal > rivalScore;

    std::string note = "vs " + gameState.rival.name + " " +
                       std::to_string(rivalScore) + " 分，你 " +
                       std::to_string(view.actualTotal) + " 分";
    if (std::string(view.contestName).find("NOI") != std::string::npos &&
        std::string(view.contestName).find("Day1") == std::string::npos)
        note += win ? " —— 一生之敌，就此终结！" : " —— 仍旧差一口气";

    if (win) {
        applyStatDelta("mood", 1, "赢下宿敌");
        logEvent("赢了 " + note + "！心态 +1", "event");
    } else if (view.actualTotal < rivalScore) {
        applyStatDelta("mood", -2, "输给宿敌");
        gameState.rivalryMonths = 2;   // 知耻后勇：2 个月效率 ×1.1
        logEvent("输了 " + note + "。心态 -2，但你憋着一股劲（知耻后勇：2 个月内学习效率 ×1.1）", "event");
    } else {
        logEvent("打平了 " + note, "event");
    }

    // NOI 决胜压过宿敌 → 隐藏成就
    const auto& ach = gameState.playerStats.achievements;
    const bool already = std::find(ach.begin(), ach.end(), "一生之敌") != ach.end();
    if (win && std::string(view.contestName) == "NOI Day2" && !already) {
        gameState.playerStats.achievements.push_back("一生之敌");
        logEvent("隐藏成就达成：一生之敌！", "event");
    }

    // 记录最近对比（持久化，供宿敌卡片展示）
    gameState.rival.lastNote = note;
}

// 学年过渡：倾诉次数重置；高三扰动（保底留一人）
inline void onYearTransition(int newYear) {
    for (auto& c : gameState.companions) c.ventUsedThisYear = false;

    if (newYear != 3 || gameState.companions.empty()) return;

    std::vector<int> leaving;
    for (int i = 0; i < (int)gameState.companions.size(); ++i)
        if (!gameState.companions[i].gone &&
            Utils::randomBool(GONE_CHANCE_Y3))
            leaving.push_back(i);

    // 保底：不能所有人都走光
    if (leaving.size() == gameState.companions.size() && !leaving.empty())
        leaving.erase(leaving.begin() + Utils::randomInt(0, (int)leaving.size() - 1));

    for (int i : leaving) {
        gameState.companions[i].gone = true;
        logEvent(gameState.companions[i].name + " 高三转去了文化班，机房里少了一个身影……", "event");
    }
}

// 月末：知耻后勇递减 + 15% 随机伙伴事件（结算事实·其他）
inline void onMonthEnd() {
    if (gameState.rivalryMonths > 0) {
        gameState.rivalryMonths--;
        if (gameState.rivalryMonths == 0)
            logEvent("知耻后勇的劲头过去了。", "event");
    }

    std::vector<int> alive;
    for (int i = 0; i < (int)gameState.companions.size(); ++i)
        if (!gameState.companions[i].gone) alive.push_back(i);
    if (alive.empty() || !Utils::randomBool(0.15)) return;

    int idx = alive[Utils::randomInt(0, (int)alive.size() - 1)];
    auto& c = gameState.companions[idx];
    switch (Utils::randomInt(0, 3)) {
    case 0: {  // 借笔记
        applyStatDelta(KNOWLEDGE_DIMS[c.dimIndex], 1, "伙伴借笔记");
        c.relation = std::min(100, c.relation + 3);
        gameState.settlementFacts.push_back({SettlementFact::Cat::Other,
            c.name + " 借给你一沓笔记：" + Utils::getStatName(KNOWLEDGE_DIMS[c.dimIndex]) + " +1"});
        break;
    }
    case 1: {  // 生日会
        applyStatDelta("mood", 1, "生日会");
        c.relation = std::min(100, c.relation + 3);
        gameState.settlementFacts.push_back({SettlementFact::Cat::Other,
            "参加了 " + c.name + " 的生日会：心态 +1"});
        break;
    }
    case 2: {  // 小摩擦
        applyStatDelta("mood", -1, "伙伴摩擦");
        c.relation = std::max(0, c.relation - 5);
        gameState.settlementFacts.push_back({SettlementFact::Cat::Other,
            "和 " + c.name + " 闹了点小摩擦：心态 -1，关系 -5"});
        break;
    }
    default: { // 联机放松
        applyStatDelta("mood", 1, "联机放松");
        c.relation = std::min(100, c.relation + 2);
        gameState.settlementFacts.push_back({SettlementFact::Cat::Other,
            "和 " + c.name + " 联机放松了一晚：心态 +1"});
        break;
    }
    }
}

} // namespace Social

#endif // SOCIAL_HPP
