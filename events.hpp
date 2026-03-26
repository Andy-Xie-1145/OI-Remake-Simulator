#ifndef EVENTS_HPP
#define EVENTS_HPP

#include "types.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <map>

// ========== 前向声明全局变量 ==========
extern PlayerStats playerStats;
extern std::string gameDifficulty;
extern int mood;
extern std::map<std::string, int> currentShopPrices;

// ========== 日志函数声明（无默认参数） ==========
inline void logEvent(const std::string& message, const std::string& type);

// ========== 事件选项结构 ==========
struct EventOption {
    std::string text;
    std::map<std::string, int> effects;  // 属性变化
    std::string nextEvent;                // 后续事件
    std::string description;              // 描述
    int cost = 0;                         // 商店价格
    double probability = 1.0;             // 触发概率
};

// ========== 事件配置（完全复制原版eventSystem.training） ==========
inline const std::map<std::string, std::pair<std::string, std::vector<EventOption>>> EVENT_CONFIGS = {
    {"偷学", {
        "在其他人摸鱼摆烂的时候，你却在偷偷学习。这样的学习方式也许会带来一些效果？你其实并不知道，你只是觉得在其他人休息的时候学习，会更有动力。这也是你引以为傲的一点。",
        {
            {"偷学被同学发现，被迫中断学习", {}, "", "", 0, 1.0},
            {"偷学被嘲讽：偷学照样考不过我，废物", {{"mood", -1}}, "", "", 0, 1.0},
            {"偷学了一些动态规划", {{"dp", 1}}, "", "", 0, 1.0},
            {"偷学了一些数据结构", {{"ds", 1}}, "", "", 0, 1.0},
            {"偷学了一些字符串", {{"string", 1}}, "", "", 0, 1.0},
            {"偷学了一些图论", {{"graph", 1}}, "", "", 0, 1.0},
            {"偷学了一些组合计数", {{"combinatorics", 1}}, "", "", 0, 1.0},
            {"偷学了一些文化课", {{"culture", 1}}, "", "", 0, 1.0}
        }
    }},
    {"休息", {
        "竞赛生的生活非常忙碌，适当的休息也许可以让你更好地调整心态，迎接接下来的挑战。你期待着有个好梦，便躺在了床上。",
        {
            {"在床上躺着使你感到非常舒适，你很快就睡着了，至于虚幻的梦你醒来时已经记不清了", {{"mood", 1}}, "", "", 0, 1.0},
            {"在梦里你梦到了很多：喜欢的女孩，曾经的老友，还有你那未完成的梦想", {{"determination", 500}}, "", "", 0, 1.0},
            {"渐入梦境之时，你带上了你所有的决心……", {}, "决心商店", "", 0, 1.0},
            {"你被楼下的七八岁的小孩吵的无法安然入睡，这让你感到更加烦躁", {{"mood", -1}}, "", "", 0, 1.0}
        }
    }},
    {"打隔膜", {
        "竞赛生的快乐来源之一，当然是打隔膜。你和你的朋友们一起在机房打隔膜，在享受着游戏的乐趣的同时，又要避开教练的视线——你的学长曾因为在机房打隔膜被教练抓到，然后被罚写检讨并被轰出了机房。",
        {
            {"轻轻松松带飞全场，你感受到了游戏带来的快感", {{"mood", 2}}, "", "", 0, 1.0},
            {"经过激烈的厮杀后，勉强获胜——这确实缓解了一些压力", {{"mood", 1}}, "", "", 0, 1.0},
            {"打隔膜给你带来了必胜的决心：我在OI上也一定会赢！", {{"determination", 300}}, "", "", 0, 1.0},
            {"被对面虐，心态爆炸。连开了五把却怎么都赢不了，你开始怀疑自己的实力", {{"mood", -1}}, "", "", 0, 1.0},
            {"你在打隔膜时候被抓，跟学长一样地，被罚写检讨并被轰出了机房", {{"mood", -2}}, "", "", 0, 1.0}
        }
    }},
    {"摸鱼", {
        "日常训练给你带来了巨大的压力，你决定在训练期间暂时放松一下，舒缓一下心情。总不会真的有学习机器，连摸鱼的时间都没有吧？",
        {
            {"刷了会手机，时间就过去了", {}, "", "", 0, 1.0},
            {"去床上休息一下，缓解一下压力", {}, "休息", "", 0, 1.0},
            {"朋友都在旁边：为什么不一起打隔膜？", {}, "打隔膜", "", 0, 1.0}
        }
    }},
    {"出游", {
        "竞赛生都是一些死宅，即使有空的时间也都是待在机房里。然而机房的气氛确实比较压抑，而且平时你也没有时间出去走走，那为什么不去感受一下外面的世界呢？",
        {
            {"你偶然遇上黄昏和晚霞：在另一个我不学OI的世界里，我此时会在做什么？", {}, "", "", 0, 1.0},
            {"你在湖边的咖啡厅遇到了一位学长，你回心转意决定跟他学习一会", {}, "偷学", "", 0, 1.0},
            {"原本只想开开心心地溜达，怎料天下大雨，你被淋成了落汤鸡", {{"mood", -1}}, "", "", 0, 1.0},
            {"后来你才知道：生活不只眼前的OI，还有诗和远方", {{"mood", 1}}, "", "", 0, 1.0}
        }
    }},
    {"遗忘", {
        "一些知识总会在不知不觉中遗忘，就如同历史的长河终究会把你我淹没。——希望大家一直记得我，希望大家永远忘了我。",
        {
            {"忘记动态规划", {{"dp", -1}}, "", "", 0, 1.0},
            {"忘记数据结构", {{"ds", -1}}, "", "", 0, 1.0},
            {"忘记字符串", {{"string", -1}}, "", "", 0, 1.0},
            {"忘记图论", {{"graph", -1}}, "", "", 0, 1.0},
            {"忘记组合计数", {{"combinatorics", -1}}, "", "", 0, 1.0}
        }
    }},
    {"焦虑", {
        "长期的高压生活，你总会陷入焦虑。一次次的挫折后，你开始怀疑自己是否真的适合OI，是否真的有天赋。你觉得自己不再是三年前那个充满梦想和决心的自己了。但是走到这一步，你已经没有退路了。",
        {
            {"有时候你开始思考人生的意义：我到底在追求什么？——可惜你找不到答案", {{"mood", -1}}, "", "", 0, 1.0},
            {"你开始轻微抑郁，你总觉得自己的努力没有意义，但也没有解决的办法，只能反复内耗", {{"mood", -2}}, "", "", 0, 1.0},
            {"在焦虑中，你开始选择遗忘，选择逃避，选择放弃", {}, "遗忘", "", 0, 1.0},
            {"你在一次次的焦虑中，变得更没有底气和决心", {{"determination", -500}}, "", "", 0, 1.0}
        }
    }},
    {"长期训练", {
        "你很幸运地进入到了最好的高中，这里有着最好的师资力量，最好的学习氛围，最好的竞赛氛围。你开始进行长期训练，水平很快得到了提升。",
        {
            {"综合训练", {{"dp", 1}, {"ds", 1}, {"string", 1}, {"graph", 1}, {"combinatorics", 1}}, "", "", 0, 1.0},
            {"动态规划专项训练", {{"dp", 4}}, "", "", 0, 1.0},
            {"数据结构专项训练", {{"ds", 4}}, "", "", 0, 1.0},
            {"字符串专项训练", {{"string", 4}}, "", "", 0, 1.0},
            {"图论专项训练", {{"graph", 4}}, "", "", 0, 1.0},
            {"组合计数专项训练", {{"combinatorics", 4}}, "", "", 0, 1.0},
            {"文化课训练", {{"culture", 4}}, "", "", 0, 1.0}
        }
    }},
    {"提升训练", {
        "人们只有会利用时间，才能真正地提升自己。你在碎片的时间里反复训练，水平也许会得到略微的提升——当然，你也可以选择摸鱼。",
        {
            {"动态规划专项训练", {{"dp", 1}}, "", "", 0, 1.0},
            {"数据结构专项训练", {{"ds", 1}}, "", "", 0, 1.0},
            {"字符串专项训练", {{"string", 1}}, "", "", 0, 1.0},
            {"图论专项训练", {{"graph", 1}}, "", "", 0, 1.0},
            {"组合计数专项训练", {{"combinatorics", 1}}, "", "", 0, 1.0},
            {"文化课训练", {{"culture", 1}}, "", "", 0, 1.0},
            {"训练不如摸鱼", {}, "摸鱼", "", 0, 1.0}
        }
    }},
    {"比赛训练", {
        "教练告诉你，比赛是检验你水平的最好方式。你开始参加比赛，你希望能在平时的比赛中找到自己的不足，并加以改进。这也许会给你正式的比赛带来帮助。",
        {
            {"按照教练推荐的，参加校内模拟赛", {{"coding", 1}}, "好比赛", "", 0, 1.0},
            {"你注意到一些网站上也有比赛资源，也许可以打洛谷月赛", {}, "好比赛", "", 0, 1.0},
            {"你偶然听说了Codeforces，大家都说这里的题目质量很高，你决定去试一试", {{"thinking", 1}}, "好比赛", "", 0, 1.0},
            {"你偶然听说了Atcoder，大家都说这里的题目质量很高，你决定去试一试", {{"thinking", 1}}, "好比赛", "", 0, 1.0},
            {"打比赛不如摸鱼", {}, "摸鱼", "", 0, 1.0}
        }
    }},
    {"好比赛", {
        "你对这场比赛的质量十分满意，这确实是一场出的相当不错的比赛。不过他相当大的难度，让你感到有些力不从心。",
        {
            {"比赛结束后，你发现这场比赛被Unrated了，你感到非常沮丧", {{"mood", -2}}, "", "", 0, 1.0},
            {"得到提升", {{"thinking", 2}}, "", "", 0, 1.0},
            {"得到提升", {{"coding", 2}}, "", "", 0, 1.0},
            {"得到提升", {{"ds", 2}}, "", "", 0, 1.0},
            {"得到提升", {{"dp", 2}}, "", "", 0, 1.0},
            {"得到提升", {{"string", 2}}, "", "", 0, 1.0},
            {"得到提升", {{"graph", 2}}, "", "", 0, 1.0},
            {"在做比赛的时候，你更加坚定了你的决心", {{"determination", 500}}, "", "", 0, 1.0},
            {"你觉得这不是你的正常发挥，你还能做的更好", {}, "焦虑", "", 0, 1.0}
        }
    }},
    {"正常比赛", {
        "这场比赛的质量还算中等，并没有到值得夸赞的地步，但你也许能从中获得一些启发，也可能因为打的不够好而陷入焦虑。",
        {
            {"得到提升", {{"thinking", 1}}, "", "", 0, 1.0},
            {"得到提升", {{"coding", 1}}, "", "", 0, 1.0},
            {"在做比赛的时候，你更加坚定了你的决心", {{"determination", 200}}, "", "", 0, 1.0},
            {"做完比赛你改变了看法：你觉得做这种比赛就是在浪费时间", {{"mood", -1}}, "", "", 0, 1.0},
            {"做完比赛你只觉得平平无奇，并没有带来什么实际效果", {}, "", "", 0, 1.0},
            {"你觉得这不是你的正常发挥，你还能做的更好", {}, "焦虑", "", 0, 1.0}
        }
    }},
    {"烂比赛", {
        "你意识到这是一场极其糟糕的比赛！你坚持认为这场比赛就是垃圾中的王者，不仅浪费时间还搞人心态。",
        {
            {"比赛结束后，你发现这场比赛被Unrated了，你感到非常幸运", {{"mood", 2}}, "", "", 0, 1.0},
            {"尽管如此，你还是从中得到了一些提升", {{"coding", 1}}, "", "", 0, 1.0},
            {"尽管如此，你还是从中得到了一些提升", {{"ds", 1}}, "", "", 0, 1.0},
            {"建议不会出题就不要出比赛，出题人纯纯智障", {{"mood", -1}}, "", "", 0, 1.0},
            {"虽然很垃圾但也就这样，这不会影响什么事", {}, "", "", 0, 1.0},
            {"这是不是我的问题？如果真正的比赛也是这样的，那我该怎么办？", {}, "焦虑", "", 0, 1.0},
            {"我再也不会笑了", {{"determination", -300}}, "", "", 0, 1.0},
            {"做这场比赛不如去看奶龙大战暴暴龙", {{"determination", -500}}, "", "", 0, 1.0}
        }
    }},
    {"娱乐时间", {
        "每天反复想题写题的生活一定是很压抑的，你决定利用好你的娱乐时间，做一些你觉得有意义的事情。",
        {
            {"你趁其他人娱乐，想要偷学一会", {}, "偷学", "", 0, 1.0},
            {"你困得不行了，为什么不休息一会", {}, "休息", "", 0, 1.0},
            {"年少不知摸鱼好", {}, "摸鱼", "", 0, 1.0},
            {"打隔膜是一种很好的娱乐方式", {}, "打隔膜", "", 0, 1.0},
            {"有空的时候多去看看世界，看看大自然", {}, "出游", "", 0, 1.0}
        }
    }},
    {"赛前一天", {
        "不知不觉已经到了比赛前的最后一天，你不再希望提升你的能力，你只祈祷在比赛中取得一个好成绩——那么现在做什么，才能带来好运呢？",
        {
            {"缓和心态", {{"mood_set", 7}}, "", "调整心态到恰好为7", 0, 1.0},
            {"放松一下", {{"mood", 2}}, "", "心态+2", 0, 1.0},
            {"渐入梦境", {}, "决心商店", "进入决心商店", 0, 1.0},
            {"提升训练", {}, "提升训练", "进行提升训练", 0, 1.0},
            {"休息一下", {}, "休息", "休息", 0, 1.0}
        }
    }},
    {"步入高二", {
        "时光飞逝，转眼间你已经升入高二。新的学年带来了新的挑战，你需要在OI和文化课之间找到平衡。你的决心依然坚定，但你也意识到时间变得更加宝贵。",
        {
            {"专注OI", {{"determination", 500}, {"culture", -2}}, "", "你决定继续专注于OI，为接下来的比赛做准备", 0, 1.0},
            {"均衡发展", {{"determination", 200}, {"culture", 2}, {"mood", 1}}, "", "你试图在OI和文化课之间找到平衡", 0, 1.0},
            {"感到迷茫", {{"determination", -200}, {"mood", -2}}, "焦虑", "面对繁重的学业压力，你开始质疑自己的选择", 0, 1.0},
            {"重整旗鼓", {{"determination", 300}, {"thinking", 1}, {"coding", 1}}, "", "新的学年给了你新的动力，你决定以更好的状态面对挑战", 0, 1.0}
        }
    }},
    {"决心商店", {
        "在梦境中，你到了一个神秘的商店。商店的老板告诉你，只要你愿意，你就可以用你的决心来提升你的能力。但也许你需要先慎重地考虑一下，你到底需要什么。每次购买后，商品的价格都会上涨。",
        {
            {"思维提升", {{"thinking", 2}}, "", "花费300点决心提升2点思维能力", 300, 1.0},
            {"代码提升", {{"coding", 2}}, "", "花费300点决心提升2点代码能力", 300, 1.0},
            {"细心提升", {{"carefulness", 2}}, "", "花费300点决心提升2点细心", 300, 1.0},
            {"随机提升", {{"random", 1}}, "", "花费300点决心随机提升一项算法能力", 300, 1.0},
            {"心态恢复", {{"mood", 2}}, "", "花费500点决心提升2点心态", 500, 1.0},
            {"全面提升", {{"dp", 1}, {"ds", 1}, {"string", 1}, {"graph", 1}, {"combinatorics", 1}}, "", "花费1000点决心提升所有算法能力", 1000, 1.0},
            {"速度提升", {{"quickness", 1}}, "", "花费1500点决心提升1点迅捷", 1500, 1.0},
            {"心理素质提升", {{"mental", 1}}, "", "花费1500点决心提升1点心理素质", 1500, 1.0},
            {"放弃购买", {}, "", "离开商店", 0, 1.0}
        }
    }}
};

// ========== 执行事件 ==========
inline void applyEffect(std::map<std::string, int> effects) {
    for (const auto& [key, value] : effects) {
        if (key == "mood") {
            mood = std::max(0, std::min(MOOD_LIMIT, mood + value));
        } else if (key == "mood_set") {
            mood = value;
        } else if (key == "determination") {
            playerStats.determination += value;
        } else if (key == "dp") {
            playerStats.dp = std::max(0, std::min(20, playerStats.dp + value));
        } else if (key == "ds") {
            playerStats.ds = std::max(0, std::min(20, playerStats.ds + value));
        } else if (key == "string") {
            playerStats.string = std::max(0, std::min(20, playerStats.string + value));
        } else if (key == "graph") {
            playerStats.graph = std::max(0, std::min(20, playerStats.graph + value));
        } else if (key == "combinatorics") {
            playerStats.combinatorics = std::max(0, std::min(20, playerStats.combinatorics + value));
        } else if (key == "thinking") {
            playerStats.thinking = std::max(0, std::min(20, playerStats.thinking + value));
        } else if (key == "coding") {
            playerStats.coding = std::max(0, std::min(20, playerStats.coding + value));
        } else if (key == "carefulness") {
            playerStats.carefulness = std::max(0, std::min(20, playerStats.carefulness + value));
        } else if (key == "quickness") {
            playerStats.quickness = std::max(0, std::min(20, playerStats.quickness + value));
        } else if (key == "mental") {
            playerStats.mental = std::max(0, std::min(20, playerStats.mental + value));
        } else if (key == "culture") {
            playerStats.culture = std::max(0, std::min(20, playerStats.culture + value));
        } else if (key == "random") {
            // 随机提升一项知识点
            int idx = Utils::randomInt(0, 4);
            std::string keys[] = {"dp", "ds", "string", "graph", "combinatorics"};
            if (keys[idx] == "dp") playerStats.dp = std::min(20, playerStats.dp + 1);
            else if (keys[idx] == "ds") playerStats.ds = std::min(20, playerStats.ds + 1);
            else if (keys[idx] == "string") playerStats.string = std::min(20, playerStats.string + 1);
            else if (keys[idx] == "graph") playerStats.graph = std::min(20, playerStats.graph + 1);
            else playerStats.combinatorics = std::min(20, playerStats.combinatorics + 1);
        }
    }
}

// ========== 运行事件 ==========
inline std::string runEvent(const std::string& eventName) {
    auto it = EVENT_CONFIGS.find(eventName);
    if (it == EVENT_CONFIGS.end()) {
        logEvent("未知事件: " + eventName, "event");
        return "";
    }
    
    const auto& [description, options] = it->second;
    
    std::cout << "\n┌────────────────────────────────────────┐\n";
    std::cout << "│\t【" << eventName << "】\t\t\t\t│\n";
    std::cout << "└────────────────────────────────────────┘\n";
    std::cout << description << "\n\n";
    
    // 显示选项
    int optionsToShow = std::min(5, (int)options.size());
    std::vector<int> availableIndices;
    
    for (int i = 0; i < (int)options.size() && (int)availableIndices.size() < optionsToShow; i++) {
        availableIndices.push_back(i);
    }
    
    for (int i = 0; i < (int)availableIndices.size(); i++) {
        int idx = availableIndices[i];
        std::cout << "  " << (i + 1) << ". " << options[idx].text;
        if (!options[idx].description.empty()) {
            std::cout << " (" << options[idx].description << ")";
        }
        std::cout << "\n";
    }
    
    std::cout << "请选择: ";
    int choice;
    std::cin >> choice;
    
    if (choice < 1 || choice > (int)availableIndices.size()) {
        logEvent("无效选择", "event");
        return "";
    }
    
    const auto& selected = options[availableIndices[choice - 1]];
    
    // 商店特殊处理
    if (eventName == "决心商店") {
        if (selected.text == "放弃购买") {
            logEvent("离开商店", "event");
            return "";
        }
        
        int cost = currentShopPrices[selected.text];
        if (playerStats.determination < cost) {
            logEvent("决心不足！需要 " + std::to_string(cost) + " 决心", "event");
            return "决心商店";  // 继续商店
        }
        
        playerStats.determination -= cost;
        applyEffect(selected.effects);
        
        // 更新价格
        currentShopPrices[selected.text] += SHOP_PRICE_INCREMENTS.at(gameDifficulty).at(selected.text);
        
        logEvent("购买成功: " + selected.text + "，花费 " + std::to_string(cost) + " 决心", "event");
        logEvent("当前决心: " + std::to_string(playerStats.determination), "event");
        
        // 继续商店
        return "决心商店";
    }
    
    // 应用效果
    applyEffect(selected.effects);
    
    // 记录日志
    logEvent(selected.text, "event");
    
    return selected.nextEvent;
}

// ========== 执行事件链 ==========
inline void runEventChain(std::string startEvent) {
    std::string currentEvent = startEvent;
    while (!currentEvent.empty()) {
        currentEvent = runEvent(currentEvent);
    }
}

#endif // EVENTS_HPP
