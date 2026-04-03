#include "game.hpp"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <d3d11.h>
#include <tchar.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

static ID3D11Device *g_pd3dDevice = nullptr;
static ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
static IDXGISwapChain *g_pSwapChain = nullptr;
static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{

    enum class GuiScreen
    {
        Home,
        Talent,
        Training,
        Notice,
        Contest,
        ContestResult,
        GameOver
    };

    struct ContestProblemResult
    {
        std::string name;
        int expectedScore = 0;
        int actualScore = 0;
    };

    struct ContestResultView
    {
        std::string contestName;
        std::vector<ContestProblemResult> problems;
        int expectedTotal = 0;
        int actualTotal = 0;
        int determinationReward = 0;
        std::string aggregateLabel;
        int aggregateValue = 0;
        bool hasAggregate = false;
        std::string award;
        bool hasAward = false;
    };

    struct TrainingView
    {
        int totalEvents = 0;
        int currentEventNumber = 0;
        std::string currentEventType;
        std::string currentEventKey;
        std::vector<EventOption> options;
        bool optionsFrozen = false;
        bool awaitingContinue = false;
    };

    struct GameOverView
    {
        std::string reason;
    };

    struct NoticeView
    {
        enum class Action
        {
            None,
            BeginTrainingPhase,
            ReturnContest,
            FinalizeContest
        };

        bool active = false;
        std::string title;
        std::string body;
        std::string detail;
        std::string buttonLabel = "继续";
        Action action = Action::None;
        int phase = 0;
        int numEvents = 0;
        std::string startLog;
    };

    void ResetSharedState()
    {
        playerStats = PlayerStats();
        gameDifficulty = "hard";
        timePoints = 24;
        mood = 10;
        currentProblem = 1;
        totalProblems = 0;
        currentContestName = "NOIP";
        debugmode = false;
        problems.clear();
        subProblems.clear();
        thinkProgress.clear();
        codeProgress.clear();
        isCodeComplete.clear();
        errorRates.clear();
        lastActions.clear();
        currentPhase = 1;
        totalTrainingEvents = 5;
        currentShopPrices.clear();
        gameLog.clear();
        clearPendingContestNotice();
        clearShopState();
    }

    std::string JoinStrings(const std::vector<std::string> &parts, const std::string &separator)
    {
        std::ostringstream builder;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            if (i > 0)
            {
                builder << separator;
            }
            builder << parts[i];
        }
        return builder.str();
    }

    std::string DifficultyLabel(const std::string &key)
    {
        const auto it = DIFFICULTY_SETTINGS.find(key);
        return it != DIFFICULTY_SETTINGS.end() ? it->second.name : key;
    }

    std::string BuildEndingSummary()
    {
        if (playerStats.isIOIgold)
            return "你成功拿到了 IOI 金牌，最终还是站在了世界 OI 之巅。";
        if (playerStats.isNationalTeam)
            return "你成为了中国国家队选手，代表中国参加了 IOI。";
        if (playerStats.isTrainingTeam)
            return "你作为国家集训队选手，已经具备了保送资格。";
        if (playerStats.isProvincialTeam)
            return "作为省队选手，你在 OI 的道路上已经取得了不错的成绩。";
        return "虽然未能进入省队，但你依然收获了宝贵的经验。";
    }

    std::string BuildRequirementText(const SubProblem &sp, int problemIdx, int subProblemIdx)
    {
        (void)sp;
        return buildSubProblemRequirementText(problemIdx, subProblemIdx);
    }

    std::string FormatEffectValue(const EventOption &option, const std::string &key, int value)
    {
        if (key == "mood" && option.text == "缓和心态")
        {
            return "心态设为" + std::to_string(value);
        }

        const std::string statName = Utils::getStatName(key);
        if (value > 0)
        {
            return statName + "+" + std::to_string(value);
        }
        if (value < 0)
        {
            return statName + std::to_string(value);
        }
        return statName + "+0";
    }

    std::string BuildOptionEffectText(const EventOption &option)
    {
        std::vector<std::string> effects;
        for (const auto &[key, value] : option.effects)
        {
            effects.push_back(FormatEffectValue(option, key, value));
        }

        if (!option.nextEvent.empty() || !option.randomChoices.empty() ||
            !option.probabilityEffects.empty() || !option.nextEventProbability.empty())
        {
            effects.push_back("?");
        }

        if (effects.empty())
        {
            return "效果：无";
        }

        return "效果：" + JoinStrings(effects, "，");
    }

    class GuiApp
    {
    public:
        GuiApp();

        void SetFontWarning(std::string warning);
        const std::string &FontWarning() const;
        void Render();

    private:
        GuiScreen screen_ = GuiScreen::Home;
        std::string selectedDifficulty_ = "hard";
        std::array<int, 5> talents_{};
        int storyCursor_ = 0;
        TrainingView training_;
        NoticeView notice_;
        ContestResultView contestResult_;
        GameOverView gameOver_;
        bool gameInitialized_ = false;
        std::string fontWarning_;

        void ResetToHome();
        int TalentBudget() const;
        int TotalAllocated() const;
        int RemainingTalent() const;
        void BeginSetup();
        void ApplyTalentAllocation();

        void BeginTrainingPhase(int phase, int numEvents, const std::string &startLog);
        void StartTrainingEvent();
        void FinishTrainingEvent();
        void SelectTrainingOption(size_t index);
        void ContinueTrainingPhase();

        void BeginContestStep(int contestId);
        void FinalizeContest();
        void HandleContestAction(int subProblemIdx, char action);
        void SetGameOver(std::string reason);
        void AdvanceStory();
        void ShowNotice(NoticeView notice);
        void DismissNotice();

        void RenderTopBar();
        void RenderHome();
        void RenderTalent();
        void RenderTraining();
        void RenderNotice();
        void RenderContest();
        void RenderContestResult();
        void RenderGameOver();
        void RenderSidebar();
        void RenderPlayerCard();
        void RenderFlagsCard();
        void RenderLogsCard();
    };

    GuiApp &App();
    ImFont *LoadChineseFont(ImGuiIO &io, std::string &loadedPath);
    void ApplyGuiStyle();

    GuiApp::GuiApp()
    {
        ResetToHome();
    }

    void GuiApp::SetFontWarning(std::string warning)
    {
        fontWarning_ = std::move(warning);
    }

    const std::string &GuiApp::FontWarning() const
    {
        return fontWarning_;
    }

    int GuiApp::TalentBudget() const
    {
        return DIFFICULTY_SETTINGS.at(selectedDifficulty_).talentPoints;
    }

    int GuiApp::TotalAllocated() const
    {
        int total = 0;
        for (int value : talents_)
            total += value;
        return total;
    }

    int GuiApp::RemainingTalent() const
    {
        return TalentBudget() - TotalAllocated();
    }

    void GuiApp::ResetToHome()
    {
        ResetSharedState();
        screen_ = GuiScreen::Home;
        selectedDifficulty_ = "hard";
        talents_.fill(0);
        storyCursor_ = 0;
        training_ = TrainingView();
        notice_ = NoticeView();
        contestResult_ = ContestResultView();
        gameOver_ = GameOverView();
        gameInitialized_ = false;
    }

    void GuiApp::BeginSetup()
    {
        gameDifficulty = selectedDifficulty_;
        initGame();
        logEvent("选择了" + DifficultyLabel(gameDifficulty) + "难度", "event");
        talents_.fill(0);
        storyCursor_ = 0;
        training_ = TrainingView();
        notice_ = NoticeView();
        contestResult_ = ContestResultView();
        gameOver_ = GameOverView();
        gameInitialized_ = true;
        screen_ = GuiScreen::Talent;
    }

    void GuiApp::ApplyTalentAllocation()
    {
        playerStats.dp = talents_[0];
        playerStats.ds = talents_[1];
        playerStats.string = talents_[2];
        playerStats.graph = talents_[3];
        playerStats.combinatorics = talents_[4];
        AdvanceStory();
    }
    void GuiApp::BeginTrainingPhase(int phase, int numEvents, const std::string &startLog)
    {
        currentPhase = phase;
        logEvent(startLog, "event");
        training_ = TrainingView();
        training_.totalEvents = numEvents;
        training_.currentEventNumber = 1;
        screen_ = GuiScreen::Training;
        StartTrainingEvent();
    }

    void GuiApp::StartTrainingEvent()
    {
        if (training_.currentEventNumber > training_.totalEvents)
        {
            training_ = TrainingView();
            AdvanceStory();
            return;
        }

        if (training_.currentEventKey.empty())
        {
            training_.currentEventType = getTrainingEventType(training_.currentEventNumber, training_.totalEvents);
            training_.currentEventKey = training_.currentEventType;
            logEvent("触发事件: " + training_.currentEventType, "event");
        }

        auto eventIt = TRAINING_EVENTS.find(training_.currentEventKey);
        if (eventIt == TRAINING_EVENTS.end())
        {
            logEvent("未知事件: " + training_.currentEventKey, "event");
            training_.awaitingContinue = true;
            training_.options.clear();
            training_.optionsFrozen = false;
            training_.currentEventKey.clear();
            return;
        }

        if (eventIt->second.isShop && training_.optionsFrozen)
        {
            if (training_.options.empty())
            {
                training_.awaitingContinue = true;
                training_.optionsFrozen = false;
                training_.currentEventKey.clear();
            }
            return;
        }

        training_.options = getAvailableOptions(eventIt->second);
        training_.optionsFrozen = eventIt->second.isShop;
        if (training_.options.empty())
        {
            logEvent("事件没有可用选项: " + training_.currentEventKey, "event");
            training_.awaitingContinue = true;
            training_.optionsFrozen = false;
            training_.currentEventKey.clear();
        }
    }

    void GuiApp::FinishTrainingEvent()
    {
        training_.awaitingContinue = true;
        training_.options.clear();
        training_.optionsFrozen = false;
        training_.currentEventKey.clear();
    }

    void GuiApp::SelectTrainingOption(size_t index)
    {
        if (training_.currentEventKey.empty())
            return;
        auto eventIt = TRAINING_EVENTS.find(training_.currentEventKey);
        if (eventIt == TRAINING_EVENTS.end() || index >= training_.options.size())
            return;

        const TrainingEvent &event = eventIt->second;
        const EventOption selected = training_.options[index];

        logEvent("选择了：" + selected.text, "event");
        if (!selected.description.empty())
        {
            logEvent(selected.description, "event");
        }

        if (event.isShop)
        {
            if (selected.text == "放弃购买")
            {
                logEvent("离开商店", "event");
                clearShopState();
                FinishTrainingEvent();
                return;
            }

            if (playerStats.determination < selected.cost)
            {
                logEvent("决心不足！需要" + std::to_string(selected.cost) + "点决心，当前只有" +
                             std::to_string(playerStats.determination) + "点。",
                         "event");
                return;
            }

            playerStats.determination -= selected.cost;
            logEvent("消耗了" + std::to_string(selected.cost) + "点决心", "event");
            applySelectedOptionEffects(selected);
            purchasedItems.insert(selected.text);
            training_.options.erase(
                std::remove_if(training_.options.begin(), training_.options.end(),
                               [&selected](const EventOption &option)
                               {
                                   return option.text == selected.text;
                               }),
                training_.options.end());

            const auto incIt = SHOP_PRICE_INCREMENTS.find(gameDifficulty);
            if (incIt != SHOP_PRICE_INCREMENTS.end())
            {
                const auto priceIt = incIt->second.find(selected.text);
                if (priceIt != incIt->second.end())
                {
                    currentShopPrices[selected.text] += priceIt->second;
                    logEvent("下次购买" + selected.text + "需要" +
                                 std::to_string(currentShopPrices[selected.text]) + "点决心",
                             "event");
                }
            }

            logEvent("当前心态值：" + std::to_string(mood), "event");
            logEvent("当前决心值：" + std::to_string(playerStats.determination), "event");
            if (training_.options.empty())
            {
                FinishTrainingEvent();
            }
            return;
        }

        applySelectedOptionEffects(selected);
        logEvent("当前心态值：" + std::to_string(mood), "event");
        logEvent("当前决心值：" + std::to_string(playerStats.determination), "event");

        if (!selected.nextEvent.empty())
        {
            logEvent("触发跳转事件：" + selected.nextEvent, "event");
            training_.currentEventKey = selected.nextEvent;
            StartTrainingEvent();
            return;
        }

        if (!selected.nextEventProbability.empty())
        {
            const std::string nextEvent = rollNextEvent(selected.nextEventProbability);
            if (!nextEvent.empty())
            {
                logEvent("触发概率跳转事件：" + nextEvent, "event");
                training_.currentEventKey = nextEvent;
                StartTrainingEvent();
                return;
            }
        }

        FinishTrainingEvent();
    }

    void GuiApp::ContinueTrainingPhase()
    {
        if (!training_.awaitingContinue)
            return;
        training_.awaitingContinue = false;
        training_.options.clear();
        training_.optionsFrozen = false;
        training_.currentEventKey.clear();
        training_.currentEventType.clear();
        ++training_.currentEventNumber;
        if (training_.currentEventNumber > training_.totalEvents)
        {
            training_ = TrainingView();
            AdvanceStory();
            return;
        }
        StartTrainingEvent();
    }

    void GuiApp::BeginContestStep(int contestId)
    {
        contestResult_ = ContestResultView();
        startContest(contestId);
        screen_ = GuiScreen::Contest;
    }

    void GuiApp::FinalizeContest()
    {
        ContestResultView result;
        result.contestName = currentContestName;

        const bool isIOIContest = isCurrentContestIOI();
        int totalExpectedScore = 0;
        int totalActualScore = 0;

        for (int i = 0; i < totalProblems; ++i)
        {
            int problemExpectedScore = 0;
            int problemActualScore = 0;

            for (size_t j = 0; j < subProblems[i].size(); ++j)
            {
                const auto &sp = subProblems[i][j];
                const bool codeCompleted = codeProgress[i][j] >= calculateCodeTime(sp);
                const bool checkCompleted = isCodeComplete[i][j];

                if (codeCompleted)
                {
                    problemExpectedScore = std::max(problemExpectedScore, sp.score);
                }

                if (checkCompleted)
                {
                    problemActualScore = std::max(problemActualScore, sp.score);
                }
                else if (codeCompleted && !isIOIContest)
                {
                    const double successRate = 1.0 - errorRates[i][j];
                    for (int k = static_cast<int>(j); k >= 0; --k)
                    {
                        if (Utils::randomBool(successRate))
                        {
                            problemActualScore = std::max(problemActualScore, subProblems[i][k].score);
                            break;
                        }
                    }
                }
            }

            totalExpectedScore += problemExpectedScore;
            totalActualScore += problemActualScore;
            result.problems.push_back({problems[i].name, problemExpectedScore, problemActualScore});
        }

        result.expectedTotal = totalExpectedScore;
        result.actualTotal = totalActualScore;

        if (currentContestName == "省选Day2")
        {
            playerStats.tempScore = totalActualScore + playerStats.prevScore + playerStats.noipScore;
            result.aggregateLabel = "省选总分";
            result.aggregateValue = playerStats.tempScore;
            result.hasAggregate = true;
        }
        else if (currentContestName == "NOI Day2")
        {
            playerStats.tempScore = totalActualScore + playerStats.prevScore + (playerStats.isProvincialTeamA ? 5 : 0);
            result.aggregateLabel = "NOI总分";
            result.aggregateValue = playerStats.tempScore;
            result.hasAggregate = true;
        }
        else if (currentContestName == "IOI Day2")
        {
            playerStats.tempScore = totalActualScore + playerStats.prevScore;
            result.aggregateLabel = "IOI总分";
            result.aggregateValue = playerStats.tempScore;
            result.hasAggregate = true;
        }
        else if (currentContestName == "CTT Day4")
        {
            playerStats.tempScore = totalActualScore + playerStats.prevScore1 + playerStats.prevScore2 + playerStats.prevScore3;
            playerStats.cttScore = playerStats.tempScore;
            result.aggregateLabel = "CTT总分";
            result.aggregateValue = playerStats.tempScore;
            result.hasAggregate = true;
        }
        else if (currentContestName == "CTS Day2")
        {
            playerStats.tempScore = totalActualScore + playerStats.prevScore + playerStats.cttScore;
            result.aggregateLabel = "CTS总分";
            result.aggregateValue = playerStats.tempScore;
            result.hasAggregate = true;
        }

        result.determinationReward = totalActualScore * 5;
        playerStats.determination += result.determinationReward;

        const int minMood = std::min(5 + playerStats.mental, 10);
        if (mood < minMood)
        {
            const int recovery = minMood - mood;
            mood = minMood;
            logEvent("比赛结束后心态自动恢复：+" + std::to_string(recovery) +
                         "，当前心态值：" + std::to_string(mood),
                     "event");
        }

        const bool isFinalDayContest =
            currentContestName != "省选Day1" &&
            currentContestName != "NOI Day1" &&
            currentContestName != "IOI Day1" &&
            currentContestName != "CTT Day1" &&
            currentContestName != "CTT Day2" &&
            currentContestName != "CTT Day3" &&
            currentContestName != "CTS Day1";

        if (isFinalDayContest)
        {
            if (currentContestName == "省选Day2")
                result.award = calculateAward(totalActualScore, "省选");
            else if (currentContestName == "NOI Day2")
                result.award = calculateAward(totalActualScore, "NOI");
            else if (currentContestName == "IOI Day2")
                result.award = calculateAward(totalActualScore, "IOI");
            else if (currentContestName == "CTT Day4")
                result.award = calculateAward(totalActualScore, "CTT");
            else if (currentContestName == "CTS Day2")
                result.award = calculateAward(totalActualScore, "CTS");
            else
                result.award = calculateAward(totalActualScore, currentContestName);

            result.hasAward = !result.award.empty();

            if (currentContestName == "CSP-S")
                playerStats.cspScore = totalActualScore;
            else if (currentContestName == "NOIP")
                playerStats.noipScore = totalActualScore;
        }
        else
        {
            playerStats.prevScore = totalActualScore;
            if (currentContestName == "CTT Day1")
                playerStats.prevScore1 = totalActualScore;
            else if (currentContestName == "CTT Day2")
                playerStats.prevScore2 = totalActualScore;
            else if (currentContestName == "CTT Day3")
                playerStats.prevScore3 = totalActualScore;
        }

        logEvent("比赛结束！实际总分: " + std::to_string(totalActualScore), "event");
        contestResult_ = std::move(result);
        screen_ = GuiScreen::ContestResult;
    }

    void GuiApp::ShowNotice(NoticeView notice)
    {
        notice_ = std::move(notice);
        notice_.active = true;
        screen_ = GuiScreen::Notice;
    }

    void GuiApp::DismissNotice()
    {
        const NoticeView::Action action = notice_.action;
        const int phase = notice_.phase;
        const int numEvents = notice_.numEvents;
        const std::string startLog = notice_.startLog;
        notice_ = NoticeView();

        switch (action)
        {
        case NoticeView::Action::BeginTrainingPhase:
            BeginTrainingPhase(phase, numEvents, startLog);
            break;
        case NoticeView::Action::ReturnContest:
            screen_ = GuiScreen::Contest;
            break;
        case NoticeView::Action::FinalizeContest:
            FinalizeContest();
            break;
        case NoticeView::Action::None:
        default:
            screen_ = GuiScreen::Training;
            break;
        }
    }

    void GuiApp::HandleContestAction(int subProblemIdx, char action)
    {
        const int problemIdx = currentProblem - 1;
        if (problemIdx < 0 || problemIdx >= static_cast<int>(subProblems.size()))
            return;
        if (subProblemIdx < 0 || subProblemIdx >= static_cast<int>(subProblems[problemIdx].size()))
            return;

        if (action == 'a')
            thinkSubProblem(problemIdx, subProblemIdx);
        else if (action == 'b')
            writeCodeSubProblem(problemIdx, subProblemIdx);
        else if (action == 'c')
            checkCodeSubProblem(problemIdx, subProblemIdx);
        else
            return;

        if (hasPendingContestNotice())
        {
            const PendingContestNotice eventNotice = consumePendingContestNotice();
            NoticeView notice;
            notice.title = eventNotice.title;
            notice.body = eventNotice.description;
            notice.detail = eventNotice.effectText;
            notice.action = isFullScore() ? NoticeView::Action::FinalizeContest : NoticeView::Action::ReturnContest;
            ShowNotice(std::move(notice));
            return;
        }

        if (isFullScore())
        {
            FinalizeContest();
        }
    }

    void GuiApp::SetGameOver(std::string reason)
    {
        gameOver_.reason = std::move(reason);
        screen_ = GuiScreen::GameOver;
    }
    void GuiApp::AdvanceStory()
    {
        while (true)
        {
            switch (storyCursor_)
            {
            case 0:
                storyCursor_ = 1;
                BeginTrainingPhase(1, 5, "第一次训练开始...");
                return;
            case 1:
                storyCursor_ = 2;
                BeginContestStep(1);
                return;
            case 2:
                storyCursor_ = 3;
                BeginTrainingPhase(3, 4, "第二次训练开始...");
                return;
            case 3:
                storyCursor_ = 4;
                if (playerStats.cspScore <= 0)
                {
                    logEvent("由于CSP-S成绩为零分，无法参加NOIP比赛", "event");
                    playerStats.noipScore = 0;
                    continue;
                }
                BeginContestStep(2);
                return;
            case 4:
                storyCursor_ = 5;
                BeginTrainingPhase(5, 4, "第三次训练开始...");
                return;
            case 5:
                storyCursor_ = 6;
                if (playerStats.cspScore >= 180 * difficultyMultiplier())
                {
                    BeginContestStep(3);
                    return;
                }
                logEvent("由于CSP-S成绩未达到二等奖及以上，无法参加WC比赛", "event");
                continue;
            case 6:
                storyCursor_ = 7;
                BeginTrainingPhase(7, 4, "第四次训练开始...");
                return;
            case 7:
                storyCursor_ = 8;
                BeginContestStep(4);
                return;
            case 8:
                storyCursor_ = 9;
                BeginTrainingPhase(9, 2, "第五次训练开始...");
                return;
            case 9:
                storyCursor_ = 10;
                BeginContestStep(5);
                return;
            case 10:
                storyCursor_ = 11;
                BeginTrainingPhase(11, 4, "第六次训练开始...");
                return;
            case 11:
                storyCursor_ = 12;
                if (playerStats.noipScore >= 180 * difficultyMultiplier())
                {
                    BeginContestStep(6);
                    return;
                }
                logEvent("由于NOIP成绩未达到二等奖及以上，无法参加APIO比赛", "event");
                continue;
            case 12:
                if (!playerStats.isProvincialTeam)
                {
                    logEvent("由于未进入省队，第一年的NOI阶段跳过", "event");
                    storyCursor_ = 16;
                    continue;
                }
                storyCursor_ = 13;
                BeginTrainingPhase(13, 4, "第七次训练开始...");
                return;
            case 13:
                storyCursor_ = 14;
                BeginContestStep(7);
                return;
            case 14:
                storyCursor_ = 15;
                BeginTrainingPhase(15, 2, "第八次训练开始...");
                return;
            case 15:
                storyCursor_ = 16;
                BeginContestStep(8);
                return;
            case 16:
                addExperience(1, "升入高二");
                storyCursor_ = 17;
                {
                    NoticeView notice;
                    notice.title = "升入高二";
                    notice.body = "经过 1 年的学习与比赛历练，你对 OI 的理解更深了一层。";
                    notice.detail = "经验+1";
                    notice.action = NoticeView::Action::BeginTrainingPhase;
                    notice.phase = 17;
                    notice.numEvents = 8;
                    notice.startLog = "第九次训练开始...";
                    ShowNotice(std::move(notice));
                }
                return;
            case 17:
                storyCursor_ = 18;
                BeginContestStep(1);
                return;
            case 18:
                storyCursor_ = 19;
                BeginTrainingPhase(19, 5, "第十次训练开始...");
                return;
            case 19:
                storyCursor_ = 20;
                if (playerStats.cspScore <= 0 && !playerStats.isTrainingTeam)
                {
                    logEvent("由于CSP-S成绩为零分，无法参加NOIP比赛", "event");
                    playerStats.noipScore = 0;
                    continue;
                }
                BeginContestStep(2);
                return;
            case 20:
                if (playerStats.isTrainingTeam)
                {
                    storyCursor_ = 21;
                    BeginTrainingPhase(21, 1, "第十一次训练开始...");
                    return;
                }
                storyCursor_ = 29;
                BeginTrainingPhase(29, 4, "第十一次训练开始...");
                return;
            case 21:
                storyCursor_ = 22;
                BeginContestStep(9);
                return;
            case 22:
                storyCursor_ = 23;
                BeginContestStep(10);
                return;
            case 23:
                storyCursor_ = 24;
                BeginContestStep(11);
                return;
            case 24:
                storyCursor_ = 25;
                BeginContestStep(12);
                return;
            case 25:
                if (playerStats.isCandidateTeam)
                {
                    storyCursor_ = 26;
                    BeginTrainingPhase(26, 4, "第十二次训练开始...");
                    return;
                }
                storyCursor_ = 29;
                BeginTrainingPhase(29, 4, "第十二次训练开始...");
                return;
            case 26:
                storyCursor_ = 27;
                BeginContestStep(13);
                return;
            case 27:
                storyCursor_ = 30;
                BeginContestStep(14);
                return;
            case 29:
                storyCursor_ = 30;
                if (playerStats.cspScore >= 180 * difficultyMultiplier())
                {
                    BeginContestStep(3);
                    return;
                }
                logEvent("由于CSP-S成绩未达到二等奖及以上，无法参加WC比赛", "event");
                continue;
            case 30:
                storyCursor_ = 31;
                BeginTrainingPhase(31, 5, "第十三次训练开始...");
                return;
            case 31:
                playerStats.isProvincialTeam = false;
                playerStats.isProvincialTeamA = false;
                storyCursor_ = 32;
                BeginContestStep(4);
                return;
            case 32:
                storyCursor_ = 33;
                BeginTrainingPhase(33, 2, "第十五次训练开始...");
                return;
            case 33:
                storyCursor_ = 34;
                BeginContestStep(5);
                return;
            case 34:
                if (!playerStats.isProvincialTeam && !playerStats.isNationalTeam)
                {
                    SetGameOver("在高二省选中未能进入省队");
                    return;
                }
                storyCursor_ = 35;
                continue;
            case 35:
                storyCursor_ = 36;
                BeginTrainingPhase(35, 4, "第十六次训练开始...");
                return;
            case 36:
                storyCursor_ = 37;
                BeginContestStep(6);
                return;
            case 37:
                storyCursor_ = 38;
                BeginTrainingPhase(38, 5, "第十七次训练开始...");
                return;
            case 38:
                playerStats.isTrainingTeam = false;
                storyCursor_ = 39;
                BeginContestStep(7);
                return;
            case 39:
                storyCursor_ = 40;
                BeginTrainingPhase(40, 2, "第十八次训练开始...");
                return;
            case 40:
                storyCursor_ = 41;
                BeginContestStep(8);
                return;
            case 41:
                if (playerStats.isNationalTeam)
                {
                    storyCursor_ = 42;
                    BeginTrainingPhase(42, 6, "第十九次训练开始...");
                    return;
                }
                if (!playerStats.isTrainingTeam)
                {
                    SetGameOver("完成NOI比赛");
                    return;
                }
                storyCursor_ = 45;
                continue;
            case 42:
                storyCursor_ = 43;
                BeginContestStep(15);
                return;
            case 43:
                storyCursor_ = 44;
                BeginContestStep(16);
                return;
            case 44:
                if (playerStats.isIOIgold || !playerStats.isTrainingTeam)
                {
                    SetGameOver("完成IOI比赛");
                    return;
                }
                storyCursor_ = 45;
                continue;
            case 45:
                storyCursor_ = 46;
                BeginTrainingPhase(45, 1, "第二十次训练开始...");
                return;
            case 46:
                playerStats.isCandidateTeam = false;
                storyCursor_ = 47;
                BeginContestStep(9);
                return;
            case 47:
                storyCursor_ = 48;
                BeginContestStep(10);
                return;
            case 48:
                storyCursor_ = 49;
                BeginContestStep(11);
                return;
            case 49:
                storyCursor_ = 50;
                BeginContestStep(12);
                return;
            case 50:
                if (!playerStats.isCandidateTeam)
                {
                    SetGameOver("未能进入候选队");
                    return;
                }
                storyCursor_ = 51;
                continue;
            case 51:
                storyCursor_ = 52;
                BeginTrainingPhase(50, 4, "第二十一次训练开始...");
                return;
            case 52:
                playerStats.isNationalTeam = false;
                storyCursor_ = 53;
                BeginContestStep(13);
                return;
            case 53:
                storyCursor_ = 54;
                BeginContestStep(14);
                return;
            case 54:
                if (!playerStats.isNationalTeam)
                {
                    SetGameOver("未能进入国家队");
                    return;
                }
                storyCursor_ = 55;
                continue;
            case 55:
                storyCursor_ = 56;
                BeginTrainingPhase(53, 6, "第二十二次训练开始...");
                return;
            case 56:
                storyCursor_ = 57;
                BeginContestStep(15);
                return;
            case 57:
                storyCursor_ = 58;
                BeginContestStep(16);
                return;
            case 58:
                SetGameOver("完成IOI比赛");
                return;
            default:
                SetGameOver("游戏流程已结束");
                return;
            }
        }
    }
    void GuiApp::Render()
    {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("OI 主窗口", nullptr,
                     ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings);

        RenderTopBar();

        const float sidebarWidth = gameInitialized_ ? 360.0f : 0.0f;
        if (sidebarWidth > 0.0f)
        {
            ImGui::BeginChild("main_content", ImVec2(-sidebarWidth - 12.0f, 0.0f), false);
        }
        else
        {
            ImGui::BeginChild("main_content", ImVec2(0.0f, 0.0f), false);
        }

        switch (screen_)
        {
        case GuiScreen::Home:
            RenderHome();
            break;
        case GuiScreen::Talent:
            RenderTalent();
            break;
        case GuiScreen::Training:
            RenderTraining();
            break;
        case GuiScreen::Notice:
            RenderNotice();
            break;
        case GuiScreen::Contest:
            RenderContest();
            break;
        case GuiScreen::ContestResult:
            RenderContestResult();
            break;
        case GuiScreen::GameOver:
            RenderGameOver();
            break;
        }

        ImGui::EndChild();

        if (gameInitialized_)
        {
            ImGui::SameLine();
            ImGui::BeginChild("sidebar", ImVec2(0.0f, 0.0f), true);
            RenderSidebar();
            ImGui::EndChild();
        }

        ImGui::End();
    }

    void GuiApp::RenderTopBar()
    {
        ImGui::TextUnformatted("OI 重开模拟器 GUI");
        ImGui::SameLine();
    ImGui::TextDisabled("基于核心数值与题库");

        if (!fontWarning_.empty())
        {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.95f, 0.78f, 0.25f, 1.0f), "%s", fontWarning_.c_str());
        }

        if (screen_ != GuiScreen::Home)
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 120.0f);
            if (ImGui::Button("返回首页", ImVec2(96.0f, 0.0f)))
            {
                ResetToHome();
            }
        }

        ImGui::Separator();
    }

    void GuiApp::RenderHome()
    {
        ImGui::Spacing();
        ImGui::TextUnformatted("开始游戏");
        ImGui::Separator();
        ImGui::TextWrapped("选择难度后开始新的一局。训练、比赛和结算都可以直接在界面中操作。");
        ImGui::Spacing();

        static const std::array<std::pair<const char *, const char *>, 4> difficultyOrder = {{{"easy", "简单：天赋点 30，初始决心 3000，分数线降低 20%"},
                                                                                              {"normal", "普通：天赋点 20，初始决心 1500，分数线降低 10%"},
                                                                                              {"hard", "困难：天赋点 15，初始决心 500，标准难度"},
                                                                                              {"expert", "专家：天赋点 15，初始决心 0，分数线提高 10%"}}};

        for (const auto &[key, description] : difficultyOrder)
        {
            const bool selected = selectedDifficulty_ == key;
            if (selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.30f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.23f, 0.50f, 0.33f, 1.0f));
            }

            if (ImGui::Button((DifficultyLabel(key) + "##difficulty").c_str(), ImVec2(120.0f, 36.0f)))
            {
                selectedDifficulty_ = key;
            }

            if (selected)
            {
                ImGui::PopStyleColor(2);
            }

            ImGui::SameLine();
            ImGui::TextWrapped("%s", description);
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::Button("开始新的一局", ImVec2(180.0f, 44.0f)))
        {
            BeginSetup();
        }
    }

    void GuiApp::RenderTalent()
    {
        ImGui::Text("当前难度：%s", DifficultyLabel(selectedDifficulty_).c_str());
        ImGui::Separator();
        ImGui::TextWrapped("分配初始算法天赋点。这里保留原版规则：你可以不把点数花完。");
        ImGui::Spacing();

        static const std::array<const char *, 5> labels = {
            "动态规划", "数据结构", "字符串", "图论", "组合计数"};

        for (size_t i = 0; i < labels.size(); ++i)
        {
            const int maxForCurrent = talents_[i] + RemainingTalent();
            ImGui::PushID(static_cast<int>(i));
            ImGui::SliderInt(labels[i], &talents_[i], 0, std::max(0, maxForCurrent));
            ImGui::PopID();
        }

        ImGui::Spacing();
        ImGui::Text("总点数：%d", TalentBudget());
        ImGui::Text("已分配：%d", TotalAllocated());
        ImGui::Text("剩余：%d", RemainingTalent());
        ImGui::Spacing();

        if (ImGui::Button("确认天赋并开始", ImVec2(180.0f, 40.0f)))
        {
            ApplyTalentAllocation();
        }
        ImGui::SameLine();
        if (ImGui::Button("返回难度选择", ImVec2(160.0f, 40.0f)))
        {
            ResetToHome();
        }
    }

    void GuiApp::RenderTraining()
    {
        ImGui::Text("训练阶段：%d / %d", training_.currentEventNumber, training_.totalEvents);
        ImGui::SameLine();
        ImGui::TextDisabled("当前大阶段：%d", currentPhase);
        ImGui::Separator();

        if (training_.awaitingContinue)
        {
            if (ImGui::Button("继续..", ImVec2(140.0f, 38.0f)))
            {
                ContinueTrainingPhase();
            }
            return;
        }

        auto eventIt = TRAINING_EVENTS.find(training_.currentEventKey);
        if (eventIt == TRAINING_EVENTS.end())
        {
            ImGui::TextWrapped("当前训练事件无法加载。可以直接继续。");
            ImGui::Spacing();
            if (ImGui::Button("继续", ImVec2(140.0f, 38.0f)))
            {
                training_.awaitingContinue = true;
                ContinueTrainingPhase();
            }
            return;
        }

        const TrainingEvent &event = eventIt->second;
        ImGui::Text("事件类型：%s", training_.currentEventType.c_str());
        ImGui::Spacing();
        ImGui::BeginChild("training_event_card", ImVec2(0.0f, 250.0f), true);
        ImGui::Text("%s", event.title.c_str());
        ImGui::Separator();
        ImGui::TextWrapped("%s", event.description.c_str());
        ImGui::Spacing();
        ImGui::Text("当前心态：%d / %d", mood, MOOD_LIMIT);
        ImGui::Text("当前决心：%d", playerStats.determination);
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::TextUnformatted("可选操作");
        ImGui::Separator();

        for (size_t i = 0; i < training_.options.size(); ++i)
        {
            const auto &option = training_.options[i];
            const std::string effectText = BuildOptionEffectText(option);
            ImGui::PushID(static_cast<int>(i));
            ImGui::BeginChild("option_card", ImVec2(0.0f, 135.0f), true);
            ImGui::TextWrapped("%zu. %s\n%s", i + 1, option.text.c_str(), effectText.c_str());
            if (!option.description.empty())
            {
                ImGui::TextWrapped("%s", option.description.c_str());
            }
            if (option.cost > 0)
            {
                ImGui::Text("花费：%d 决心", option.cost);
            }
            if (ImGui::Button("选择", ImVec2(96.0f, 30.0f)))
            {
                SelectTrainingOption(i);
            }
            ImGui::EndChild();
            ImGui::PopID();
            ImGui::Spacing();
        }
    }

    void GuiApp::RenderNotice()
    {
        if (!notice_.active)
        {
            screen_ = GuiScreen::Training;
            return;
        }

        ImGui::TextUnformatted(notice_.title.c_str());
        ImGui::Separator();

        if (!notice_.body.empty())
        {
            ImGui::TextWrapped("%s", notice_.body.c_str());
            ImGui::Spacing();
        }

        if (!notice_.detail.empty())
        {
            ImGui::TextColored(ImVec4(0.28f, 0.62f, 0.34f, 1.0f), "%s", notice_.detail.c_str());
            ImGui::Spacing();
        }

        if (ImGui::Button(notice_.buttonLabel.c_str(), ImVec2(140.0f, 40.0f)))
        {
            DismissNotice();
        }
    }

    void GuiApp::RenderContest()
    {
        ImGui::Text("%s", currentContestName.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("时间点：%d", timePoints);
        ImGui::SameLine();
        ImGui::TextDisabled("心态：%d / %d", mood, MOOD_LIMIT);
        ImGui::Separator();
        ImGui::TextWrapped("在这里切换题目并执行思考、写代码、对拍或提交等操作。");
        ImGui::Spacing();

        ImGui::BeginDisabled(timePoints != 0);
        if (ImGui::Button("结束比赛", ImVec2(140.0f, 34.0f)))
        {
            FinalizeContest();
            ImGui::EndDisabled();
            return;
        }
        ImGui::EndDisabled();
        ImGui::Spacing();

        if (totalProblems > 1)
        {
            for (int i = 1; i <= totalProblems; ++i)
            {
                if (i > 1)
                    ImGui::SameLine();
                const bool isSelected = (i == currentProblem);
                if (isSelected)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.46f, 0.28f, 1.0f));
                }
                if (ImGui::Button(("T" + std::to_string(i)).c_str(), ImVec2(64.0f, 32.0f)))
                {
                    currentProblem = i;
                }
                if (isSelected)
                {
                    ImGui::PopStyleColor();
                }
            }
            ImGui::Spacing();
        }

        const int problemIdx = currentProblem - 1;
        if (problemIdx < 0 || problemIdx >= static_cast<int>(problems.size()))
        {
            ImGui::TextUnformatted("当前没有可展示的题目。");
            return;
        }

        ImGui::Text("当前题目：T%d %s", currentProblem, problems[problemIdx].name.c_str());
        ImGui::Separator();

        for (size_t i = 0; i < subProblems[problemIdx].size(); ++i)
        {
            const auto &sp = subProblems[problemIdx][i];
            const int thinkTime = calculateThinkTime(sp);
            const int codeTime = calculateCodeTime(sp);
            const double thinkRate = calculateThinkSuccessRate(sp);
            const double codeRate = calculateCodeSuccessRate(sp);
            const bool completed = isCodeComplete[problemIdx][i];
            const bool canThink = !completed && thinkProgress[problemIdx][i] < thinkTime && timePoints > 0;
            const bool canCode = !completed && thinkProgress[problemIdx][i] >= thinkTime &&
                                 codeProgress[problemIdx][i] < codeTime && timePoints > 0;
            const bool canCheck = !completed && codeProgress[problemIdx][i] >= codeTime &&
                                  (isCurrentContestIOI() || timePoints > 0);

            ImGui::PushID(static_cast<int>(i));
            ImGui::BeginChild("subproblem_card", ImVec2(0.0f, 267.0f), true);
            ImGui::Text("部分分 %zu  |  %d 分", i + 1, sp.score);
            if (completed)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.40f, 0.85f, 0.42f, 1.0f), "已完成");
            }

            ImGui::Separator();
            ImGui::TextWrapped("%s", BuildRequirementText(sp, problemIdx, static_cast<int>(i)).c_str());
            ImGui::Spacing();
            const std::string thinkTotalText = getThinkTimeDisplayTotal(problemIdx, static_cast<int>(i));
            ImGui::Text("思考进度：%d / %s", thinkProgress[problemIdx][i], thinkTotalText.c_str());
            ImGui::Text("代码进度：%d / %d", codeProgress[problemIdx][i], codeTime);
            ImGui::Text("思考成功率：%d%%", static_cast<int>(thinkRate * 100));
            ImGui::Text("写代码成功率：%d%%", static_cast<int>(codeRate * 100));
            if (errorRates[problemIdx][i] >= 0.0)
            {
                ImGui::Text("%s出错概率：%d%%", isCurrentContestIOI() ? "提交" : "对拍",
                            static_cast<int>(errorRates[problemIdx][i] * 100));
            }
            else
            {
                ImGui::Text("%s出错概率：?", isCurrentContestIOI() ? "提交" : "对拍");
            }

            ImGui::Spacing();
            ImGui::BeginDisabled(!canThink);
            if (ImGui::Button("思考", ImVec2(100.0f, 30.0f)))
            {
                HandleContestAction(static_cast<int>(i), 'a');
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::BeginDisabled(!canCode);
            if (ImGui::Button("写代码", ImVec2(100.0f, 30.0f)))
            {
                HandleContestAction(static_cast<int>(i), 'b');
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::BeginDisabled(!canCheck);
            if (ImGui::Button(isCurrentContestIOI() ? "提交" : "对拍", ImVec2(100.0f, 30.0f)))
            {
                HandleContestAction(static_cast<int>(i), 'c');
            }
            ImGui::EndDisabled();

            ImGui::EndChild();
            ImGui::PopID();
            ImGui::Spacing();
        }
    }

    void GuiApp::RenderContestResult()
    {
        ImGui::Text("%s 比赛结果", contestResult_.contestName.c_str());
        ImGui::Separator();

        if (ImGui::BeginTable("result_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("题目");
            ImGui::TableSetupColumn("预期得分");
            ImGui::TableSetupColumn("实际得分");
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < contestResult_.problems.size(); ++i)
            {
                const auto &item = contestResult_.problems[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("T%zu %s", i + 1, item.name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d", item.expectedScore);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", item.actualScore);
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Text("预期总分：%d", contestResult_.expectedTotal);
        ImGui::Text("实际总分：%d", contestResult_.actualTotal);
        if (contestResult_.hasAggregate)
        {
            ImGui::Text("%s：%d", contestResult_.aggregateLabel.c_str(), contestResult_.aggregateValue);
        }
        ImGui::Text("决心奖励：+%d", contestResult_.determinationReward);
        if (contestResult_.hasAward)
        {
            ImGui::Text("获奖情况：%s", contestResult_.award.c_str());
        }

        ImGui::Spacing();
        if (ImGui::Button("继续后续流程", ImVec2(180.0f, 40.0f)))
        {
            AdvanceStory();
        }
    }

    void GuiApp::RenderGameOver()
    {
        ImGui::TextUnformatted("游戏结束");
        ImGui::Separator();
        ImGui::TextWrapped("%s", gameOver_.reason.c_str());
        ImGui::Spacing();
        ImGui::TextWrapped("%s", BuildEndingSummary().c_str());
        ImGui::Spacing();
        ImGui::TextUnformatted("成就记录");
        ImGui::Separator();

        if (playerStats.achievements.empty())
        {
            ImGui::TextUnformatted("没有获得任何成就");
        }
        else
        {
            for (const auto &achievement : playerStats.achievements)
            {
                ImGui::BulletText("%s", achievement.c_str());
            }
        }

        ImGui::Spacing();
        if (ImGui::Button("重新开始", ImVec2(160.0f, 40.0f)))
        {
            ResetToHome();
        }
    }

    void GuiApp::RenderSidebar()
    {
        RenderPlayerCard();
        ImGui::Spacing();
        RenderFlagsCard();
        ImGui::Spacing();
        RenderLogsCard();
    }

    void GuiApp::RenderPlayerCard()
    {
        ImGui::TextUnformatted("角色状态");
        ImGui::Separator();
        ImGui::Text("难度：%s", DifficultyLabel(gameDifficulty).c_str());
        ImGui::Text("决心：%d", playerStats.determination);
        ImGui::Text("心态：%d / %d", mood, MOOD_LIMIT);
        ImGui::Text("思维：%d", playerStats.thinking);
        ImGui::Text("代码：%d", playerStats.coding);
        ImGui::Text("细心：%d", playerStats.carefulness);
        ImGui::Text("迅捷：%d", playerStats.quickness);
        ImGui::Text("心理素质：%d", playerStats.mental);
        ImGui::Text("经验：%d", playerStats.experience);
        ImGui::Text("经验积累：%d / 6", playerStats.tempExperience);
        ImGui::Spacing();
        ImGui::TextUnformatted("知识点");
        ImGui::Text("动态规划：%d", playerStats.dp);
        ImGui::Text("数据结构：%d", playerStats.ds);
        ImGui::Text("字符串：%d", playerStats.string);
        ImGui::Text("图论：%d", playerStats.graph);
        ImGui::Text("组合计数：%d", playerStats.combinatorics);
    }

    void GuiApp::RenderFlagsCard()
    {
        ImGui::TextUnformatted("比赛与阶段");
        ImGui::Separator();
        ImGui::Text("当前阶段：%d", currentPhase);
        ImGui::Text("CSP-S：%d", playerStats.cspScore);
        ImGui::Text("NOIP：%d", playerStats.noipScore);
        ImGui::Text("阶段总分：%d", playerStats.tempScore);
        ImGui::Text("CTT总分：%d", playerStats.cttScore);
        ImGui::Spacing();
        ImGui::TextUnformatted("关键状态");
        ImGui::BulletText("省队A：%s", playerStats.isProvincialTeamA ? "是" : "否");
        ImGui::BulletText("省队：%s", playerStats.isProvincialTeam ? "是" : "否");
        ImGui::BulletText("集训队：%s", playerStats.isTrainingTeam ? "是" : "否");
        ImGui::BulletText("候选队：%s", playerStats.isCandidateTeam ? "是" : "否");
        ImGui::BulletText("国家队：%s", playerStats.isNationalTeam ? "是" : "否");
        ImGui::BulletText("IOI 金牌：%s", playerStats.isIOIgold ? "是" : "否");
    }

    void GuiApp::RenderLogsCard()
    {
        ImGui::TextUnformatted("事件日志");
        ImGui::Separator();
        ImGui::BeginChild("log_panel", ImVec2(0.0f, 0.0f), true);
        for (const auto &entry : gameLog)
        {
            ImGui::TextWrapped("%s", entry.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f)
        {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    }

    GuiApp &App()
    {
        static GuiApp app;
        return app;
    }

    ImFont *LoadChineseFont(ImGuiIO &io, std::string &loadedPath)
    {
        const std::array<const char *, 6> candidates = {
            "C:/Windows/Fonts/msyh.ttc",
            "C:/Windows/Fonts/msyh.ttf",
            "C:/Windows/Fonts/msyhbd.ttc",
            "C:/Windows/Fonts/simhei.ttf",
            "C:/Windows/Fonts/simsun.ttc",
            "C:/Windows/Fonts/Deng.ttf"};

        ImFontConfig config;
        config.OversampleH = 2;
        config.OversampleV = 2;

        for (const char *path : candidates)
        {
            if (!std::filesystem::exists(path))
                continue;
            if (ImFont *font = io.Fonts->AddFontFromFileTTF(path, 18.0f, &config, io.Fonts->GetGlyphRangesChineseFull()))
            {
                loadedPath = path;
                return font;
            }
        }

        io.Fonts->AddFontDefault();
        loadedPath.clear();
        return nullptr;
    }

    void ApplyGuiStyle()
    {
        ImGui::StyleColorsLight();
        ImGuiStyle &style = ImGui::GetStyle();
        style.WindowRounding = 10.0f;
        style.ChildRounding = 8.0f;
        style.FrameRounding = 7.0f;
        style.GrabRounding = 7.0f;
        style.TabRounding = 7.0f;

        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.95f, 0.93f, 1.0f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.98f, 0.98f, 0.96f, 1.0f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.98f, 0.98f, 0.96f, 1.0f);
        colors[ImGuiCol_Border] = ImVec4(0.77f, 0.79f, 0.73f, 1.0f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.88f, 0.90f, 0.84f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.82f, 0.86f, 0.76f, 1.0f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.76f, 0.82f, 0.68f, 1.0f);
        colors[ImGuiCol_Button] = ImVec4(0.28f, 0.47f, 0.34f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.33f, 0.54f, 0.39f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.40f, 0.28f, 1.0f);
        colors[ImGuiCol_Header] = ImVec4(0.77f, 0.84f, 0.74f, 1.0f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.80f, 0.68f, 1.0f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.61f, 0.74f, 0.59f, 1.0f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.21f, 0.38f, 0.29f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.24f, 0.42f, 0.32f, 1.0f);
        colors[ImGuiCol_Text] = ImVec4(0.12f, 0.16f, 0.14f, 1.0f);
    }

} // namespace
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        L"OI_Simulator_GUI_WindowClass", nullptr};
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowW(
        wc.lpszClassName, L"OI 重开模拟器 GUI",
        WS_OVERLAPPEDWINDOW, 100, 100, 1540, 920,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    std::string fontPath;
    if (ImFont *font = LoadChineseFont(io, fontPath))
    {
        io.FontDefault = font;
        App().SetFontWarning("已加载中文字体。");
    }
    else
    {
        App().SetFontWarning("警告：未找到可用中文字体文件，当前已退回默认字体，中文可能显示不完整。");
    }

    ApplyGuiStyle();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImVec4 clearColor = ImVec4(0.92f, 0.93f, 0.90f, 1.0f);
    MSG msg{};

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        App().Render();

        ImGui::Render();
        const float clearColorWithAlpha[4] = {
            clearColor.x * clearColor.w,
            clearColor.y * clearColor.w,
            clearColor.z * clearColor.w,
            clearColor.w};
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColorWithAlpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};

    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
                                      featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
    {
        return false;
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)
    {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

void CreateRenderTarget()
{
    ID3D11Texture2D *pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, static_cast<UINT>(LOWORD(lParam)), static_cast<UINT>(HIWORD(lParam)), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
