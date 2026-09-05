// ============================================================================
//  month_engine.hpp  —  月度引擎（v2 回合制的唯一规则归属地）
//
//  深模块：月内 sequencing（活动 → 比赛 → 考试 → 结算 → 下月）、活动成本、
//  停课、特质掉落、奖金入账、商店交易，全部收敛在这一个 interface 后面。
//  main.cpp 只剩视图：读状态、发指令、渲染 Engine 给出的 Phase 队列。
//
//  用法（UI 视角）：
//      Engine::startNewGame();                    // 开始第 1 月
//      auto r = Engine::doActivity(t, param);     // 执行活动（自动扣 AP/健康）
//      Engine::endMonthActions();                 // 「结束本月」：结算一次并入队
//      while (Engine::hasPhase()) { 渲染 currentPhase(); 完成后 phaseFinished(); }
// ============================================================================
#ifndef MONTH_ENGINE_HPP
#define MONTH_ENGINE_HPP

#include "game.hpp"
#include "story.hpp"
#include "activities.hpp"
#include "contest.hpp"
#include "culture_exam.hpp"
#include "talents.hpp"
#include "topics.hpp"
#include "ending.hpp"
#include "social.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace Engine {

// —— 月内阶段：endMonthActions 之后按序消费 ——
struct Phase {
    enum class Kind { Contest, Exam, Settlement };
    Kind kind = Kind::Settlement;
    int contestId = 0;          // kind == Contest 时有效
    bool isGaokao = false;      // kind == Exam 时有效
};

namespace state {
    inline std::vector<Phase> phases_;
    inline size_t cursor_ = 0;
    inline Calendar::MonthInfo monthInfo_;
    inline bool over_ = false;
    inline std::string overReason_;
    inline bool overByCompletion_ = false;  // true=36月走完（结局矩阵），false=健康归零
    // 当前比赛是否为活动赛（网赛/刷题/自定义），决定收尾时的特质判定路径
    inline bool contestIsActivity_ = false;
    inline Activity::Type activityKind_ = Activity::MockContest;
}

// ============================ 查询 ============================

inline const Calendar::MonthInfo& monthInfo() { return state::monthInfo_; }
inline bool isGameOver() { return state::over_; }
inline const std::string& gameOverReason() { return state::overReason_; }
inline bool hasPhase() { return state::cursor_ < state::phases_.size(); }
inline const Phase& currentPhase() { return state::phases_[state::cursor_]; }
inline bool currentContestIsActivity() { return state::contestIsActivity_; }

inline void setGameOver(std::string reason, bool byCompletion = false) {
    state::over_ = true;
    state::overReason_ = std::move(reason);
    state::overByCompletion_ = byCompletion;
}

inline bool gameEndedByCompletion() { return state::overByCompletion_; }

// —— 存档支持：内部状态的受控读取与恢复（供 save_system.hpp 使用）——
inline size_t debugCursor() { return state::cursor_; }
inline size_t debugPhaseCount() { return state::phases_.size(); }
inline const Phase& debugPhase(size_t i) { return state::phases_[i]; }
inline Activity::Type debugActivityKind() { return state::activityKind_; }
inline void appendLoadedPhase(const Phase& p) { state::phases_.push_back(p); }
inline void loadMonthInfo(const Calendar::MonthInfo& mi) { state::monthInfo_ = mi; }
// 恢复顺序约定：先 loadFromSave，再逐个 appendLoadedPhase，最后 loadMonthInfo
inline void loadFromSave(bool over, bool byCompletion, std::string reason,
                         bool contestIsActivity, Activity::Type kind, size_t cursor) {
    state::phases_.clear();
    state::cursor_ = cursor;
    state::over_ = over;
    state::overByCompletion_ = byCompletion;
    state::overReason_ = std::move(reason);
    state::contestIsActivity_ = contestIsActivity;
    state::activityKind_ = kind;
}

// ============================ 生命周期 ============================

// 回到主菜单 / 重开时清空引擎内部状态
inline void hardReset() {
    state::phases_.clear();
    state::cursor_ = 0;
    state::over_ = false;
    state::overReason_.clear();
    state::overByCompletion_ = false;
    state::contestIsActivity_ = false;
}

// 进入月机制（initGame 与背景/天赋分配应在此前完成）
inline void startNewGame() {
    hardReset();
    state::monthInfo_ = Calendar::getMonthInfo(1);
    Calendar::startMonth(1);
    Social::generateRoster();   // 随机生成 3 位机房伙伴与宿敌
    logEvent("高中生活开始了！第1年7月", "event");
}

// ============================ 活动 ============================

inline bool canDoActivity(Activity::Type t) {
    if (t == Activity::StudyCulture && gameState.isTingke) return false;  // 停课禁文化课
    return gameState.ap >= Activity::defOf(t).apCost;
}

// 停课/熬夜共用的 AP 重算：base = 8 + 停课2 + 熬夜2 + 难度加成 − 本月扣减；
// 已消耗的 AP 保持不变。两者互斥由各自 toggle 入口保证。
inline void recomputeApBudget() {
    const auto& settings = DIFFICULTY_SETTINGS.at(gameState.gameDifficulty);
    int baseAp = 8;
    if (gameState.isTingke) baseAp += 2;   // 停课：10 AP
    if (gameState.isAoYe) baseAp += 2;     // 熬夜：+2 AP
    baseAp += settings.apBonus;
    int spent = gameState.maxAp - gameState.ap;
    gameState.maxAp = std::max(0, baseAp - state::monthInfo_.apDeduction);
    gameState.ap = std::max(0, gameState.maxAp - spent);
}

// 停课切换（心态代价由每月 Calendar::startMonth 统一结算，切换瞬间不扣）
inline void toggleTingke() {
    if (!gameState.isTingke && gameState.isAoYe) {
        logEvent("熬夜状态下无法停课（二者互斥）", "event");
        return;
    }
    gameState.isTingke = !gameState.isTingke;
    recomputeApBudget();
    logEvent(gameState.isTingke ? "开始停课训练" : "恢复上课", "event");
}

// 熬夜切换：当月 +2 AP；代价在月末结算（健康-3、焦虑概率×1.3）。与停课互斥。
inline void toggleAoYe() {
    if (!gameState.isAoYe && gameState.isTingke) {
        logEvent("停课状态下无法熬夜（二者互斥）", "event");
        return;
    }
    gameState.isAoYe = !gameState.isAoYe;
    recomputeApBudget();
    logEvent(gameState.isAoYe ? "这个月开始熬夜冲刺……" : "恢复正常作息", "event");
}

struct ActivityOutcome {
    bool ok = false;                       // AP/健康不足等失败时为 false
    Activity::ActivityResult activity;     // 日志与（可选）待进入的比赛
};

// 执行活动：成本扣费在引擎内完成，调用方不再自行 ap-= / health-=
inline ActivityOutcome doActivity(Activity::Type type, int param = 0) {
    ActivityOutcome out;
    if (!canDoActivity(type)) {
        out.activity.logs.push_back("行动力或健康不足，无法执行该活动");
        return out;
    }
    const auto& d = Activity::defOf(type);
    gameState.ap -= d.apCost;
    applyStatDelta("health", -d.hpCost, d.name);

    out.ok = true;
    out.activity = Activity::execute(type, param);
    for (const auto& lg : out.activity.logs) logEvent(lg, "event");

    // 活动后的引擎侧记账
    if (type == Activity::Learn) Topics::onLearn(param);
    if (type == Activity::Exercise) gameState.exerciseCountThisMonth++;
    if (type == Activity::SummerCamp) Social::adjustCoach(+5);   // 参训：教练关系 +5

    if (out.activity.startContest) {
        state::contestIsActivity_ = true;
        state::activityKind_ = type;
        Contest::start(out.activity.contestId);
    }
    return out;
}

// 自定义模拟赛：模板经唯一 adapter 进入统一的 Contest::start(config)
inline bool startCustomContest(int templateIdx) {
    if (templateIdx < 0 || templateIdx >= static_cast<int>(CONTEST_TEMPLATES.size())) return false;
    const auto& tpl = CONTEST_TEMPLATES[templateIdx];
    const auto& d = Activity::defOf(Activity::CustomContest);
    if (!canDoActivity(Activity::CustomContest)) {
        logEvent("行动力或健康不足，无法开始自定义比赛", "event");
        return false;
    }
    gameState.ap -= d.apCost;
    applyStatDelta("health", -d.hpCost, d.name);

    state::contestIsActivity_ = true;
    state::activityKind_ = Activity::CustomContest;
    Contest::start(contestConfigFromTemplate(tpl));
    logEvent("开始自定义比赛：" + std::string(tpl.name), "event");
    return true;
}

// ============================ 商店 ============================

// 商店交易规则（扣钱 / 生效 / 记账 / 涨价）的实现地；返回是否成交
inline bool buyShopItem(const EventOption& opt) {
    if (opt.text == "放弃购买") return false;
    if (gameState.money < opt.cost) return false;

    gameState.money -= opt.cost;
    applySelectedOptionEffects(opt);
    gameState.purchasedItems.insert(opt.text);
    logEvent("购买：" + opt.text + "（-" + std::to_string(opt.cost) + "元）", "event");

    auto incIt = SHOP_PRICE_INCREMENTS.find(gameState.gameDifficulty);
    if (incIt != SHOP_PRICE_INCREMENTS.end()) {
        auto priceIt = incIt->second.find(opt.text);
        if (priceIt != incIt->second.end()) {
            gameState.currentShopPrices[opt.text] += priceIt->second;
        }
    }
    return true;
}

// ============================ 比赛 / 考试收尾 ============================

// 比赛（正式/活动）结算的唯一入口：奖金入账；正式赛额外结算教练关系与宿敌对比。
// 活动比赛（网赛/刷题/自定义）不触发宿敌与教练逻辑。
inline Contest::ContestResultView finalizeCurrentContest() {
    const bool wasActivity = state::contestIsActivity_;
    Contest::ContestResultView view = Contest::finalize();
    gameState.money += view.prizeMoney;
    if (!wasActivity) {
        if (view.hasAward) Social::adjustCoach(+10);   // 获奖：教练关系 +10
        Social::onOfficialContestFinished(view);       // 宿敌对比播报与胜负结算
    }
    state::contestIsActivity_ = false;
    return view;
}

// 活动赛（网赛/刷题/自定义）收尾：专题进度 + 按类别做特质判定。
// 网赛/自定义 15%，刷题 30%（仅在得分为正时判定）。
inline void activityContestFinished(int actualTotal) {
    Topics::onActivityContestFinished();
    const double prob =
        (state::activityKind_ == Activity::Practice) ? 0.30 : 0.15;
    if (actualTotal > 0) Talent::tryAcquireTrait(prob);
    state::contestIsActivity_ = false;
}

// 考试收尾：记录成绩（不再触发结算——结算由 endMonthActions 恰好执行一次）
inline void examFinished(int score, int maxScore, bool isGaokao) {
    recordExamScore(score, maxScore, isGaokao);
}

// ============================ 月度推进 ============================

// 阶段就位时的副作用：官方比赛立即开赛、考试立即开卷（规则归引擎所有）
inline void beginCurrentPhase() {
    if (!hasPhase()) return;
    const Phase& p = currentPhase();
    if (p.kind == Phase::Kind::Contest) {
        Contest::start(p.contestId);
    } else if (p.kind == Phase::Kind::Exam) {
        CultureExam::start(p.isGaokao);
    }
}

// 「结束本月」：结算恰好一次，然后把本月剩余阶段（比赛→考试→结算）入队
inline void endMonthActions() {
    const bool hasContest = !state::monthInfo_.contestIds.empty();
    settleMonth(hasContest);   // 每月恰好一次
    Topics::onMonthEnd();      // 专题逾期判定（结算事实追加在末尾）
    Social::onMonthEnd();      // 知耻后勇递减 + 随机伙伴事件
    if (!gameState.playerStats.isBaosong && gameState.playerStats.culture < 6)
        Social::adjustCoach(-2);   // 文化课薄弱：教练关系 -2

    if (gameState.health <= 0) {
        setGameOver("你的身体撑不住了...健康归零。");
        return;
    }

    state::phases_.clear();
    state::cursor_ = 0;
    for (int id : state::monthInfo_.contestIds) {
        state::phases_.push_back({Phase::Kind::Contest, id, false});
    }
    if (state::monthInfo_.hasExam) {
        state::phases_.push_back({Phase::Kind::Exam, 0, state::monthInfo_.isGaokao});
    }
    state::phases_.push_back({Phase::Kind::Settlement, 0, false});

    beginCurrentPhase();
}

// UI 完成当前阶段的消费后调用：推进游标；队列耗尽则翻页到下个月
inline void phaseFinished() {
    if (state::cursor_ < state::phases_.size()) {
        state::cursor_++;
    }
    if (state::cursor_ < state::phases_.size()) {
        beginCurrentPhase();
        return;
    }

    // 本月阶段消费完毕 → 推进下个月
    state::phases_.clear();
    state::cursor_ = 0;

    const int nextMonth = gameState.currentMonth + 1;
    if (nextMonth > 36) {
        setGameOver("三年高中结束了。你的 OI 之旅到此告一段落。", /*byCompletion=*/true);
        return;
    }

    const int oldYear = monthToYear(gameState.currentMonth);
    const int newYear = monthToYear(nextMonth);
    if (oldYear != newYear) {
        applyYearTransition(oldYear, newYear);
        Social::onYearTransition(newYear);   // 倾诉重置 + 高三伙伴扰动
    }

    state::monthInfo_ = Calendar::getMonthInfo(nextMonth);
    Calendar::startMonth(nextMonth);

    if (gameState.health <= 0) {
        setGameOver("你的身体撑不住了...健康归零。");
        return;
    }

    logEvent("第" + std::to_string(gameState.currentYear) + "年" +
             getCalendarMonthName(gameState.calendarMonth), "event");
}

} // namespace Engine

#endif // MONTH_ENGINE_HPP
