// ============================================================================
//  ending.hpp  —  结局矩阵（C）
//
//  二维结局：OI 高度 × 高考档位（保送直接锁定结局）。
//  纯数据 + 纯函数，便于测试；UI 只渲染 resolve 的结果。
// ============================================================================
#ifndef ENDING_HPP
#define ENDING_HPP

#include "types.hpp"

#include <string>

namespace Ending {

enum class OiTier { Legend, NationalTeam, TrainingTeam, ProvincialTeam, None };
enum class GaokaoTier { Excellent, Good, Poor, Skipped };

struct Result {
    std::string title;
    std::string description;
};

inline OiTier oiTier() {
    const auto& ps = gameState.playerStats;
    if (ps.isIOIgold) return OiTier::Legend;
    if (ps.isNationalTeam) return OiTier::NationalTeam;
    if (ps.isTrainingTeam || ps.isBaosong) return OiTier::TrainingTeam;
    if (ps.isProvincialTeam) return OiTier::ProvincialTeam;
    return OiTier::None;
}

inline GaokaoTier gaokaoTier() {
    if (gameState.playerStats.isBaosong) return GaokaoTier::Skipped;
    // 取最近一次高考成绩
    const GameState::ExamRecord* rec = nullptr;
    for (const auto& r : gameState.examRecords)
        if (r.isGaokao) rec = &r;
    if (!rec || rec->maxScore <= 0) return GaokaoTier::Poor;
    const double pct = static_cast<double>(rec->score) / rec->maxScore;
    if (pct >= 0.9) return GaokaoTier::Excellent;
    if (pct >= 0.7) return GaokaoTier::Good;
    return GaokaoTier::Poor;
}

inline Result resolveHealthDeath() {
    return {"积劳成疾",
            "你倒在了机房里，被同学抬去了医务室。\n"
            "醒来时，教练叹着气告诉你：身体是革命的本钱，可你的本钱已经透支了。\n"
            "这一次的 OI 之旅，止步于此。"};
}

inline Result resolveCompleted36() {
    switch (oiTier()) {
    case OiTier::Legend:
        return {"世界之巅",
                "IOI 金牌挂在你胸前的那一刻，全世界都记住了你的名字。\n"
                "三年前那个不甘心的少年，这一次真的做到了不留遗憾。\n"
                "这是属于你的传奇，无可复制的传奇。"};
    case OiTier::NationalTeam:
        return {"国旗之下",
                "你穿上了国家队的队服，站上了世界赛的舞台。\n"
                "无论最终名次如何，你已经站到了无数 OIer 梦寐以求的位置。\n"
                "五星红旗因你而升起，这就是最好的答案。"};
    case OiTier::TrainingTeam:
        return {"保送上岸",
                "国家集训队的名单上有你的名字——保送资格，稳稳落袋。\n"
                "别人还在题海里挣扎的时候，你已经提前拿到了未来的入场券。\n"
                "三年苦读，一朝上岸。"};
    case OiTier::ProvincialTeam: {
        switch (gaokaoTier()) {
        case GaokaoTier::Excellent:
            return {"强基无忧",
                    "省队履历加上顶尖的高考成绩，强基计划的面试官对你赞不绝口。\n"
                    "竞赛和文化课，你两手都要，两手都硬。\n"
                    "前方的路，条条通向罗马。"};
        case GaokaoTier::Good:
            return {"硝烟散尽",
                    "省选的硝烟散去，你带着省队的光环走回了高考的考场。\n"
                    "成绩不算惊天动地，但足够对得起这三年的每一个夜晚。\n"
                    "故事到这里，刚刚好。"};
        default:
            return {"得失难言",
                    "你进了省队，却在高考场上失了手。\n"
                    "竞赛的荣誉救不回每一分，人生的取舍从来不是零和。\n"
                    "这条路走得值不值，只有你自己知道。"};
        }
    }
    case OiTier::None:
    default: {
        switch (gaokaoTier()) {
        case GaokaoTier::Excellent:
            return {"文化课之神",
                    "省队的大门没有为你打开，但你用一张近乎完美的高考成绩单完成复仇。\n"
                    "谁说学竞赛的人读不好书？你就是反例本身。\n"
                    "属于你的路，在考场之外照样宽敞。"};
        case GaokaoTier::Good:
            return {"泯然众人",
                    "三年的机房岁月最后化作一段平凡的录取通知。\n"
                    "没有省队，没有金牌，只有一段自己才知道有多燃的记忆。\n"
                    "也许泯然众人，但你从未后悔。"};
        default:
            return {"复读的十字路口",
                    "省队无缘，高考失利。你站在人生的十字路口，手里攥着成绩单。\n"
                    "有人劝你认命，你却在志愿表背面写下了两个字：再来。\n"
                    "——命运的齿轮，再次开始转动。（多周目已解锁）"};
        }
    }
    }
}

} // namespace Ending

#endif // ENDING_HPP
