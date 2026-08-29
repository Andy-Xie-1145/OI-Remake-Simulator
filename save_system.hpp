// ============================================================================
//  save_system.hpp  —  存档系统（G）
//
//  极简 key-value 文本序列化器，不引第三方库。存档点约定：只在「月度行动阶段」
//  生成（引擎翻页时自动触发），比赛/考试中途不存档——因此比赛临场状态与
//  文化课考试状态不入档，下次开赛时自然重建。
//
//  文件位置：exe 同目录 savegame.txt；首行 "OISAVE <版本>" 做向前兼容。
// ============================================================================
#ifndef SAVE_SYSTEM_HPP
#define SAVE_SYSTEM_HPP

#include "game.hpp"
#include "month_engine.hpp"

#include <windows.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace Save {

constexpr int kSaveVersion = 3;
constexpr const char* kMagic = "OISAVE";

// ⚠️ 防呆哨兵 ⚠️ --------------------------------------------------------------
// GameState 新增/删除字段会使 sizeof 变化，下面的 static_assert 将编译失败，
// 强制你同步以下三处（缺一不可），改完再把数字更新为新 sizeof：
//   ① Save::serialize() / Save::deserialize()
//   ② game.hpp 的 initGame() 单局状态重置
//   ③ tests/test_features.cpp 的「initGame 重置回归」字段污染清单
static_assert(sizeof(GameState) == 1008,
    "GameState layout changed! Sync 1) Save::serialize/deserialize  "
    "2) initGame() resets  3) reset regression test, then update this size.");

// ---------- 路径 ----------
inline std::string savePath() {
    char buf[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string dir(buf);
    size_t pos = dir.find_last_of("\\/");
    if (pos != std::string::npos) dir = dir.substr(0, pos + 1);
    return dir + "savegame.txt";
}

inline bool exists() {
    std::ifstream f(savePath());
    return f.good();
}

// ---------- 轻量进度探测（不全量反序列化，供「覆盖存档」确认框使用） ----------
inline bool peekProgress(int& outYear, int& outCalMonth, int& outMonth) {
    std::ifstream f(savePath());
    if (!f.good()) return false;
    std::string magic;
    int ver = 0;
    f >> magic >> ver;
    if (magic != kMagic || ver > kSaveVersion) return false;

    bool hasMonth = false, hasYear = false, hasCal = false;
    std::string line;
    while (std::getline(f, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        if (k == "g.month")      { outMonth    = std::atoi(v.c_str()); hasMonth = true; }
        else if (k == "g.year")  { outYear     = std::atoi(v.c_str()); hasYear = true; }
        else if (k == "g.calMonth") { outCalMonth = std::atoi(v.c_str()); hasCal = true; }
        if (hasMonth && hasYear && hasCal) break;   // 三个键都在文件头部
    }
    return hasMonth;
}

// ---------- 序列化 ----------
namespace detail {

inline void put(std::ostringstream& out, const std::string& k, const std::string& v) {
    out << k << "=" << v << "\n";
}
inline void put(std::ostringstream& out, const std::string& k, int v) {
    out << k << "=" << v << "\n";
}
inline void put(std::ostringstream& out, const std::string& k, bool v) {
    out << k << "=" << (v ? 1 : 0) << "\n";
}
inline void put(std::ostringstream& out, const std::string& k, double v) {
    out << k << "=" << v << "\n";
}
// 字符串值中的换行会破坏行式格式：转义为 \n 字面量
inline std::string esc(const std::string& s) {
    std::string r;
    for (char c : s) {
        if (c == '\n') r += "\\n";
        else if (c == '=') r += "\\e";
        else if (c == '\\') r += "\\\\";
        else r += c;
    }
    return r;
}
inline std::string unesc(const std::string& s) {
    std::string r;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[i + 1];
            if (n == 'n') { r += '\n'; ++i; }
            else if (n == 'e') { r += '='; ++i; }
            else if (n == '\\') { r += '\\'; ++i; }
            else r += n;
        } else r += s[i];
    }
    return r;
}

} // namespace detail

inline std::string serialize() {
    using detail::put;
    using detail::esc;
    std::ostringstream o;
    o << kMagic << " " << kSaveVersion << "\n";
    put(o, "ver", kSaveVersion);

    // ---- PlayerStats ----
    const auto& ps = gameState.playerStats;
    put(o, "ps.dp", ps.dp);           put(o, "ps.ds", ps.ds);
    put(o, "ps.string", ps.string);   put(o, "ps.graph", ps.graph);
    put(o, "ps.comb", ps.combinatorics); put(o, "ps.math", ps.math);
    put(o, "ps.geo", ps.geometry);    put(o, "ps.hds", ps.data_structure);
    put(o, "ps.adhoc", ps.adhoc);
    put(o, "ps.thinking", ps.thinking); put(o, "ps.coding", ps.coding);
    put(o, "ps.careful", ps.carefulness); put(o, "ps.quick", ps.quickness);
    put(o, "ps.mental", ps.mental);   put(o, "ps.exp", ps.experience);
    put(o, "ps.tempExp", ps.tempExperience); put(o, "ps.culture", ps.culture);
    put(o, "ps.luck", ps.luck);
    put(o, "ps.csp", ps.cspScore);    put(o, "ps.noip", ps.noipScore);
    put(o, "ps.prev", ps.prevScore);  put(o, "ps.prev1", ps.prevScore1);
    put(o, "ps.prev2", ps.prevScore2); put(o, "ps.prev3", ps.prevScore3);
    put(o, "ps.ctt", ps.cttScore);    put(o, "ps.temp", ps.tempScore);
    put(o, "ps.noi", ps.noiScore);
    put(o, "ps.teamA", ps.isProvincialTeamA); put(o, "ps.team", ps.isProvincialTeam);
    put(o, "ps.trainTeam", ps.isTrainingTeam); put(o, "ps.cand", ps.isCandidateTeam);
    put(o, "ps.natTeam", ps.isNationalTeam);   put(o, "ps.ioiGold", ps.isIOIgold);
    put(o, "ps.extraMoodDrop", ps.extraMoodDrop);
    put(o, "ps.baosong", ps.isBaosong);
    for (size_t i = 0; i < ps.achievements.size(); ++i)
        put(o, "ps.ach." + std::to_string(i), esc(ps.achievements[i]));

    // ---- GameState 标量 ----
    put(o, "g.difficulty", gameState.gameDifficulty);
    put(o, "g.mood", gameState.mood);
    put(o, "g.health", gameState.health);
    put(o, "g.ap", gameState.ap);     put(o, "g.maxAp", gameState.maxAp);
    put(o, "g.money", gameState.money);
    put(o, "g.month", gameState.currentMonth);
    put(o, "g.year", gameState.currentYear);
    put(o, "g.calMonth", gameState.calendarMonth);
    put(o, "g.tingke", gameState.isTingke);
    put(o, "g.aoye", gameState.isAoYe);
    put(o, "g.sickNext", gameState.sickNext);
    put(o, "g.exerciseCount", gameState.exerciseCountThisMonth);
    put(o, "g.topicId", gameState.topicId);
    put(o, "g.topicStart", gameState.topicStartMonth);
    put(o, "g.topicProgress", gameState.topicProgress);
    put(o, "g.cultureEff", gameState.cultureEfficiency);
    put(o, "g.anxietyMonths", gameState.anxietyMonths);
    put(o, "g.anxious", gameState.isAnxious);
    put(o, "g.anxMult", gameState.anxietyMultiplier);
    put(o, "g.effMult", gameState.efficiencyMultiplier);
    put(o, "g.moodCap", gameState.moodCap);
    put(o, "g.background", gameState.background);
    put(o, "g.contestName", esc(gameState.currentContestName));
    put(o, "g.timePoints", gameState.timePoints);
    put(o, "g.curProblem", gameState.currentProblem);
    put(o, "g.totalProblems", gameState.totalProblems);
    put(o, "g.carefulChecks", gameState.carefulChecks);

    int i = 0;
    for (const auto& t : gameState.traits)      put(o, "g.trait." + std::to_string(i++), esc(t));
    i = 0;
    for (const auto& it : gameState.ownedItems) {
        const std::string p = "g.owned." + std::to_string(i++) + ".";
        put(o, p + "name", esc(it.first));
        put(o, p + "val", it.second);
    }
    i = 0;
    for (const auto& it : gameState.purchasedItems) put(o, "g.purchased." + std::to_string(i++), esc(it));
    for (const auto& [k, v] : gameState.currentShopPrices) put(o, "g.price." + k, v);
    for (const auto& [k, v] : gameState.lastStudyMonth)    put(o, "g.lsm." + k, v);
    i = 0;
    for (const auto& r : gameState.examRecords) {
        std::string p = "g.exam." + std::to_string(i++) + ".";
        put(o, p + "m", r.month); put(o, p + "cm", r.calendarMonth);
        put(o, p + "s", r.score); put(o, p + "max", r.maxScore);
        put(o, p + "gaokao", r.isGaokao);
    }
    // 日志只保留最近 200 条，防止存档膨胀
    size_t logStart = gameState.gameLog.size() > 200 ? gameState.gameLog.size() - 200 : 0;
    i = 0;
    for (size_t j = logStart; j < gameState.gameLog.size(); ++j)
        put(o, "g.log." + std::to_string(i++), esc(gameState.gameLog[j]));

    i = 0;
    for (int mt : gameState.masteredTopics)
        put(o, "g.mastered." + std::to_string(i++), mt);
    i = 0;
    for (const auto& c : gameState.companions) {
        const std::string p = "g.comp." + std::to_string(i++) + ".";
        put(o, p + "name", esc(c.name));
        put(o, p + "dim", c.dimIndex);
        put(o, p + "rel", c.relation);
        put(o, p + "perk", c.perkId);
        put(o, p + "gone", c.gone);
        put(o, p + "vent", c.ventUsedThisYear);
    }
    put(o, "g.coachRel", gameState.coachRelation);
    put(o, "g.rivalName", esc(gameState.rival.name));
    put(o, "g.rivalFactor", gameState.rival.baseFactor);
    put(o, "g.rivalNote", esc(gameState.rival.lastNote));
    put(o, "g.rivalryMonths", gameState.rivalryMonths);
    const auto& notice = gameState.pendingContestNotice;
    put(o, "g.notice.active", notice.active);
    put(o, "g.notice.title", esc(notice.title));
    put(o, "g.notice.desc", esc(notice.description));
    put(o, "g.notice.effect", esc(notice.effectText));

    // ---- 引擎内部状态（阶段队列 / 游标 / 月信息）----
    put(o, "eng.over", Engine::isGameOver());
    put(o, "eng.overByCompletion", Engine::gameEndedByCompletion());
    put(o, "eng.reason", esc(Engine::gameOverReason()));
    put(o, "eng.contestIsActivity", Engine::currentContestIsActivity());
    put(o, "eng.activityKind", static_cast<int>(Engine::debugActivityKind()));
    put(o, "eng.cursor", static_cast<int>(Engine::debugCursor()));
    const auto& mi = Engine::monthInfo();
    put(o, "eng.mi.year", mi.year);
    put(o, "eng.mi.calMonth", mi.calendarMonth);
    put(o, "eng.mi.hasExam", mi.hasExam);
    put(o, "eng.mi.isGaokao", mi.isGaokao);
    put(o, "eng.mi.apDeduction", mi.apDeduction);
    i = 0;
    for (int id : mi.contestIds) put(o, "eng.mi.contest." + std::to_string(i++), id);
    i = 0;
    for (size_t p = 0; p < Engine::debugPhaseCount(); ++p) {
        const auto& ph = Engine::debugPhase(p);
        std::string base = "eng.phase." + std::to_string(i++) + ".";
        put(o, base + "kind", static_cast<int>(ph.kind));
        put(o, base + "contestId", ph.contestId);
        put(o, base + "gaokao", ph.isGaokao);
    }
    return o.str();
}

// ---------- 反序列化 ----------
inline bool deserialize(const std::string& text) {
    std::istringstream in(text);
    std::string magic;
    int ver = 0;
    in >> magic >> ver;
    if (magic != kMagic || ver > kSaveVersion) return false;

    std::map<std::string, std::string> kv;
    std::string line;
    while (std::getline(in, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        kv[line.substr(0, eq)] = line.substr(eq + 1);
    }
    auto getI = [&](const std::string& k, int def = 0) -> int {
        auto it = kv.find(k);
        return it != kv.end() ? std::atoi(it->second.c_str()) : def;
    };
    auto getB = [&](const std::string& k, bool def = false) -> bool {
        auto it = kv.find(k);
        return it != kv.end() ? it->second == "1" : def;
    };
    auto getD = [&](const std::string& k, double def = 1.0) -> double {
        auto it = kv.find(k);
        return it != kv.end() ? std::atof(it->second.c_str()) : def;
    };
    auto getS = [&](const std::string& k, const std::string& def = "") -> std::string {
        auto it = kv.find(k);
        return it != kv.end() ? detail::unesc(it->second) : def;
    };

    auto& ps = gameState.playerStats;
    ps.dp = getI("ps.dp");          ps.ds = getI("ps.ds");
    ps.string = getI("ps.string");  ps.graph = getI("ps.graph");
    ps.combinatorics = getI("ps.comb"); ps.math = getI("ps.math");
    ps.geometry = getI("ps.geo");   ps.data_structure = getI("ps.hds");
    ps.adhoc = getI("ps.adhoc");
    ps.thinking = getI("ps.thinking"); ps.coding = getI("ps.coding");
    ps.carefulness = getI("ps.careful"); ps.quickness = getI("ps.quick");
    ps.mental = getI("ps.mental");  ps.experience = getI("ps.exp");
    ps.tempExperience = getI("ps.tempExp"); ps.culture = getI("ps.culture");
    ps.luck = getI("ps.luck");
    ps.cspScore = getI("ps.csp");   ps.noipScore = getI("ps.noip");
    ps.prevScore = getI("ps.prev"); ps.prevScore1 = getI("ps.prev1");
    ps.prevScore2 = getI("ps.prev2"); ps.prevScore3 = getI("ps.prev3");
    ps.cttScore = getI("ps.ctt");   ps.tempScore = getI("ps.temp");
    ps.noiScore = getI("ps.noi");
    ps.isProvincialTeamA = getB("ps.teamA"); ps.isProvincialTeam = getB("ps.team");
    ps.isTrainingTeam = getB("ps.trainTeam"); ps.isCandidateTeam = getB("ps.cand");
    ps.isNationalTeam = getB("ps.natTeam");   ps.isIOIgold = getB("ps.ioiGold");
    ps.extraMoodDrop = getI("ps.extraMoodDrop");
    ps.isBaosong = getB("ps.baosong");
    ps.achievements.clear();
    for (int j = 0;; ++j) {
        auto it = kv.find("ps.ach." + std::to_string(j));
        if (it == kv.end()) break;
        ps.achievements.push_back(detail::unesc(it->second));
    }

    gameState.gameDifficulty = getS("g.difficulty", "normal");
    gameState.mood = getI("g.mood", 10);
    gameState.health = getI("g.health", 15);
    gameState.ap = getI("g.ap");    gameState.maxAp = getI("g.maxAp");
    gameState.money = getI("g.money");
    gameState.currentMonth = getI("g.month", 1);
    gameState.currentYear = getI("g.year", 1);
    gameState.calendarMonth = getI("g.calMonth", 7);
    gameState.isTingke = getB("g.tingke");
    gameState.isAoYe = getB("g.aoye");
    gameState.sickNext = getB("g.sickNext");
    gameState.exerciseCountThisMonth = getI("g.exerciseCount");
    gameState.topicId = getI("g.topicId", -1);
    gameState.topicStartMonth = getI("g.topicStart");
    gameState.topicProgress = getI("g.topicProgress");
    gameState.cultureEfficiency = getD("g.cultureEff");
    gameState.anxietyMonths = getI("g.anxietyMonths");
    gameState.isAnxious = getB("g.anxious");
    gameState.anxietyMultiplier = getD("g.anxMult");
    gameState.efficiencyMultiplier = getD("g.effMult");
    gameState.moodCap = getI("g.moodCap", MOOD_LIMIT);
    gameState.background = getS("g.background");
    gameState.currentContestName = getS("g.contestName");
    gameState.timePoints = getI("g.timePoints");
    gameState.currentProblem = getI("g.curProblem", 1);
    gameState.totalProblems = getI("g.totalProblems");
    gameState.carefulChecks = getI("g.carefulChecks");

    gameState.traits.clear();       gameState.ownedItems.clear();
    gameState.purchasedItems.clear();
    gameState.currentShopPrices.clear(); gameState.lastStudyMonth.clear();
    gameState.masteredTopics.clear();
    for (const auto& [k, v] : kv) {
        if (k.rfind("g.trait.", 0) == 0)    gameState.traits.push_back(detail::unesc(v));
        else if (k.rfind("g.owned.", 0) == 0 && k.rfind(".name") == k.size() - 5)
            gameState.ownedItems[detail::unesc(v)] = getI(k.substr(0, k.size() - 5) + ".val");
        else if (k.rfind("g.purchased.", 0) == 0) gameState.purchasedItems.insert(detail::unesc(v));
        else if (k.rfind("g.price.", 0) == 0)    gameState.currentShopPrices[k.substr(8)] = std::atoi(v.c_str());
        else if (k.rfind("g.lsm.", 0) == 0)      gameState.lastStudyMonth[k.substr(6)] = std::atoi(v.c_str());
        else if (k.rfind("g.mastered.", 0) == 0) gameState.masteredTopics.insert(std::atoi(v.c_str()));
    }
    // 伙伴（定长 3 位，按索引恢复）
    gameState.companions.assign(3, CompanionNpc{});
    for (const auto& [k, v] : kv) {
        if (k.rfind("g.comp.", 0) != 0) continue;
        const size_t dot = k.find('.', 7);
        if (dot == std::string::npos) continue;
        const int idx = std::atoi(k.substr(7, dot - 7).c_str());
        const std::string field = k.substr(dot + 1);
        if (idx < 0 || idx >= (int)gameState.companions.size()) continue;
        auto& c = gameState.companions[idx];
        if (field == "name") c.name = detail::unesc(v);
        else if (field == "dim") c.dimIndex = std::atoi(v.c_str());
        else if (field == "rel") c.relation = std::atoi(v.c_str());
        else if (field == "perk") c.perkId = std::atoi(v.c_str());
        else if (field == "gone") c.gone = (v == "1");
        else if (field == "vent") c.ventUsedThisYear = (v == "1");
    }
    gameState.coachRelation = getI("g.coachRel", 20);
    gameState.rival.name = getS("g.rivalName");
    gameState.rival.baseFactor = getD("g.rivalFactor", 1.0);
    gameState.rival.lastNote = getS("g.rivalNote");
    gameState.rivalryMonths = getI("g.rivalryMonths");
    gameState.examRecords.clear();
    for (int j = 0;; ++j) {
        std::string p = "g.exam." + std::to_string(j) + ".";
        if (kv.find(p + "m") == kv.end()) break;
        GameState::ExamRecord rec;
        rec.month = getI(p + "m"); rec.calendarMonth = getI(p + "cm");
        rec.score = getI(p + "s"); rec.maxScore = getI(p + "max");
        rec.isGaokao = getB(p + "gaokao");
        gameState.examRecords.push_back(rec);
    }
    gameState.gameLog.clear();
    for (int j = 0;; ++j) {
        auto it = kv.find("g.log." + std::to_string(j));
        if (it == kv.end()) break;
        gameState.gameLog.push_back(detail::unesc(it->second));
    }

    gameState.pendingContestNotice = PendingContestNotice{};
    gameState.pendingContestNotice.active = getB("g.notice.active");
    gameState.pendingContestNotice.title = getS("g.notice.title");
    gameState.pendingContestNotice.description = getS("g.notice.desc");
    gameState.pendingContestNotice.effectText = getS("g.notice.effect");

    Engine::loadFromSave(
        getB("eng.over"), getB("eng.overByCompletion"),
        getS("eng.reason"),
        getB("eng.contestIsActivity"),
        static_cast<Activity::Type>(getI("eng.activityKind")),
        static_cast<size_t>(getI("eng.cursor")));

    Calendar::MonthInfo mi;
    mi.year = getI("eng.mi.year", 1);
    mi.calendarMonth = getI("eng.mi.calMonth", 7);
    mi.hasExam = getB("eng.mi.hasExam");
    mi.isGaokao = getB("eng.mi.isGaokao");
    mi.apDeduction = getI("eng.mi.apDeduction");
    for (int j = 0;; ++j) {
        auto it = kv.find("eng.mi.contest." + std::to_string(j));
        if (it == kv.end()) break;
        mi.contestIds.push_back(std::atoi(it->second.c_str()));
    }
    for (int j = 0;; ++j) {
        std::string p = "eng.phase." + std::to_string(j) + ".";
        if (kv.find(p + "kind") == kv.end()) break;
        Engine::Phase ph;
        ph.kind = static_cast<Engine::Phase::Kind>(getI(p + "kind"));
        ph.contestId = getI(p + "contestId");
        ph.isGaokao = getB(p + "gaokao");
        Engine::appendLoadedPhase(ph);
    }
    Engine::loadMonthInfo(mi);

    // 比赛临场数据不入档：清空，下次 Contest::start 自然重建
    gameState.problems.clear();
    gameState.subProblems.clear();
    gameState.contestStates.clear();
    return true;
}

// ---------- 读入校验：拒绝非法状态（防呆④） ----------
// deserialize 宽松填充默认值，这里把关键不变量兜住；失败即视为存档损坏。
inline bool validateLoaded() {
    if (gameState.currentMonth < 1 || gameState.currentMonth > 36) return false;
    if (gameState.currentYear != monthToYear(gameState.currentMonth)) return false;
    if (gameState.calendarMonth != monthToCalendarMonth(gameState.currentMonth)) return false;
    if (gameState.health < 0 || gameState.health > 20) return false;
    if (gameState.mood < 0) return false;
    if (gameState.moodCap < 8 || gameState.moodCap > 24) return false;
    if (gameState.ap < 0 || gameState.maxAp < gameState.ap) return false;
    if (gameState.money < 0) return false;
    if (!DIFFICULTY_SETTINGS.count(gameState.gameDifficulty)) return false;
    return true;
}

inline bool write() {
    // 防呆③：只在月度行动阶段存档。阶段队列非空 = 比赛/考试进行中，状态不完整。
    if (Engine::hasPhase()) {
        logEvent("警告：当前处于比赛/考试流程中，已跳过自动存档", "event");
        return false;
    }

    // 防呆⑤：原子写——先写临时文件并校验魔数，再替换正式档，中途崩溃不会毁掉旧档
    const std::string tmpPath = savePath() + ".tmp";
    {
        std::ofstream f(tmpPath, std::ios::trunc);
        if (!f.good()) return false;
        f << serialize();
        f.flush();
        if (!f.good()) { f.close(); std::remove(tmpPath.c_str()); return false; }
    }
    {
        std::ifstream chk(tmpPath);
        std::string magic; int ver = 0;
        chk >> magic >> ver;
        if (magic != kMagic || ver > kSaveVersion) {
            chk.close(); std::remove(tmpPath.c_str()); return false;
        }
    }
    std::remove(savePath().c_str());           // Windows rename 不覆盖已存在目标
    if (std::rename(tmpPath.c_str(), savePath().c_str()) != 0) return false;
    return true;
}

inline bool read() {
    std::ifstream f(savePath());
    if (!f.good()) return false;
    std::ostringstream ss;
    ss << f.rdbuf();

    // 防呆④：先在备份上尝试，校验失败则原样恢复现场，绝不让玩家状态变成半残
    GameState backup = gameState;
    const size_t cursorBackup = Engine::debugCursor();
    (void)cursorBackup;
    if (!deserialize(ss.str()) || !validateLoaded()) {
        gameState = backup;
        // 引擎阶段队列同样恢复：清空即可回到「行动阶段」语义（备份时必为空）
        Engine::hardReset();
        return false;
    }
    return true;
}

inline void erase() {
    std::remove(savePath().c_str());
}

} // namespace Save

#endif // SAVE_SYSTEM_HPP
