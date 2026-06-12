#include "game.hpp"
#include "contest.hpp"
#include "story.hpp"
#include "activities.hpp"
#include "talents.hpp"
#include "culture_exam.hpp"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "oi_theme.hpp"
#include "oi_widgets.hpp"

#include <d3d11.h>
#include <tchar.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <filesystem>
#include <functional>
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
    // —— 多字号字体句柄（同一 TTF 加载多个尺寸，构建层次）——
    ImFont *g_fontBody  = nullptr; // 正文 18
    ImFont *g_fontH1    = nullptr; // 大标题 30
    ImFont *g_fontH2    = nullptr; // 次级标题 21
    ImFont *g_fontSmall = nullptr; // 小标签 13（区段英文标签 / mono 风）

    enum class GuiScreen
    {
        Home,
        Difficulty,
        IntroStory,
        Help,
        Talent,
        MonthAction,
        MonthSettlement,
        CultureExam,
        Contest,
        ContestResult,
        GameOver
    };

    constexpr const char *kGameVersion = "v0.2.0";
    constexpr const char *kIntroStoryText =
        "我重生了？\n"
        "参加完省队选拔后，你意识到自己无缘省队了。也许从此就和 OI 无缘了。\n\n"
        "你躺在床上，闭上眼，回想起自己在 OI 赛场上挥洒汗水的场景。\n\n"
        "眼泪还是流了出来。你不甘心，你觉得你还可以做得更好。\n\n"
        "你突然惊醒，发现自己回到了高一前的暑假。\n\n"
        "之前经历的一切仿佛是一场梦，却又那么真实。\n\n"
        "你意识到，这一次，你还有 36 个月。\n\n"
        "你决定，这一次，让 OI 生涯不留遗憾。";

    constexpr float kPrimaryButtonWidth = 200.0f;
    constexpr float kPrimaryButtonHeight = 48.0f;
    constexpr float kSecondaryButtonWidth = 160.0f;
    constexpr float kSecondaryButtonHeight = 42.0f;

    ImVec4 getLuoguTierColor(int level) {
        return OITheme::TierColor(level);
    }

    // 区段标题：accent tick + 英文标签 + 中文标题 + 贯穿细线
    void RenderSection(const char *en, const char *cjk = nullptr)
    {
        OIWidgets::SectionHeader(en, cjk, g_fontSmall);
    }

    void RenderPageHeader(const char *title, const char *description = nullptr)
    {
        ImGui::Spacing();
        if (g_fontH2) ImGui::PushFont(g_fontH2);
        ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Txt);
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        if (g_fontH2) ImGui::PopFont();
        ImGui::Spacing();
        if (description != nullptr && description[0] != '\0')
        {
            ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::TxtDim);
            ImGui::TextWrapped("%s", description);
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }
        ImGui::PushStyleColor(ImGuiCol_Separator, OITheme::Col::Line);
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }


    using ContestResultView = Contest::ContestResultView;

    struct GameOverView
    {
        std::string reason;
    };

    void ResetSharedState()
    {
        gameState.playerStats = PlayerStats();
        gameState.gameDifficulty = "hard";
        gameState.timePoints = 24;
        gameState.mood = 10;
        gameState.currentProblem = 1;
        gameState.totalProblems = 0;
        gameState.currentContestName = "NOIP";
        gameState.debugmode = false;
        gameState.problems.clear();
        gameState.subProblems.clear();
        gameState.contestStates.clear();
        gameState.lastActions.clear();
        gameState.currentPhase = 1;
        gameState.totalTrainingEvents = 5;
        gameState.currentShopPrices.clear();
        gameState.gameLog.clear();
        clearPendingContestNotice();
        clearShopState();

        // v2 重置
        gameState.currentMonth = 1;
        gameState.currentYear = 1;
        gameState.calendarMonth = 7;
        gameState.ap = 8;
        gameState.maxAp = 8;
        gameState.health = 15;
        gameState.money = 0;
        gameState.isTingke = false;
        gameState.lastStudyMonth.clear();
        gameState.traits.clear();
        gameState.background.clear();
        gameState.ownedItems.clear();
        gameState.cultureEfficiency = 1.0;
        gameState.settlementLogs.clear();
        gameState.examRecords.clear();
        gameState.anxietyMonths = 0;
        gameState.isAnxious = false;
        gameState.anxietyMultiplier = 1.0;
        gameState.efficiencyMultiplier = 1.0;
        gameState.moodCap = MOOD_LIMIT;
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
        if (gameState.playerStats.isIOIgold)
            return "你成功拿到了 IOI 金牌，最终还是站在了世界 OI 之巅。";
        if (gameState.playerStats.isNationalTeam)
            return "你成为了中国国家队选手，代表中国参加了 IOI。";
        if (gameState.playerStats.isTrainingTeam)
            return "你作为国家集训队选手，已经具备了保送资格。";
        if (gameState.playerStats.isProvincialTeam)
            return "作为省队选手，你在 OI 的道路上已经取得了不错的成绩。";
        return "虽然未能进入省队，但你依然收获了宝贵的经验。";
    }

    std::string BuildRequirementText(const SubProblem &sp, int problemIdx, int subProblemIdx)
    {
        (void)sp;
        return Contest::buildSubProblemRequirementText(problemIdx, subProblemIdx);
    }

    std::string BuildOptionEffectText(const EventOption &option)
    {
        std::vector<std::string> effects;
        for (const auto &[key, value] : option.effects)
        {
            // 内联 FormatEffectValue 逻辑
            if (key == "gameState.mood" && option.text == "缓和心态")
            {
                effects.push_back("心态设为" + std::to_string(value));
            }
            else
            {
                const std::string statName = Utils::getStatName(key);
                if (value > 0)
                {
                    effects.push_back(statName + "+" + std::to_string(value));
                }
                else if (value < 0)
                {
                    effects.push_back(statName + std::to_string(value));
                }
                else
                {
                    effects.push_back(statName + "+0");
                }
            }
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
        std::string selectedBackground_;   // 选中的背景 ID
        std::array<int, 9> talents_{};     // 9 维天赋分配
        bool showTalentPage_ = false;      // true=显示天赋分配页, false=显示背景选择页
        ContestResultView contestResult_;
        GameOverView gameOver_;
        bool gameInitialized_ = false;
        std::string fontWarning_;
        GuiScreen helpReturnScreen_ = GuiScreen::Home;

        // v2 月回合制
        Calendar::MonthInfo currentMonthInfo_;
        int pendingContestIdx_ = 0;  // 当前待打的比赛在 contestIds 中的索引
        int pendingContestId_ = 0;   // 当前正在打的比赛 ID
        int activitySubMenu_ = -1;   // -1=主菜单, 0=Learn, 1=网赛, 2=刷题, 3=自定义比赛
        int customTemplateIdx_ = -1; // 自定义比赛选中的模板索引
        bool isActivityContest_ = false; // true=当前比赛是活动(模拟赛/刷题), false=正式比赛
        bool showShop_ = false;         // 商店弹窗

        void ResetToHome();
        int TalentBudget() const;
        int TotalAllocated() const;
        int RemainingTalent() const;
        void BeginSetup();
        void ApplyTalentAllocation();

        // v2 月回合流程
        void StartMonthLoop();
        void EndMonthAction();
        void ContinueAfterContest();
        void ContinueAfterSettlement();

        void BeginContestStep(int contestId);
        void FinalizeContest();
        void HandleContestAction(int subProblemIdx, char action);
        void ModifyCodeProblem(int problemIdx, int subProblemIdx);
        void SetGameOver(std::string reason);
        void CheckAndShowContestNotice(char preferredAction);
        void OpenHelp();

        void RenderTopBar();
        void RenderHome();
        void RenderDifficulty();
        void RenderIntroStory();
        void RenderHelp();
        void RenderTalent();
        void RenderMonthAction();
        void RenderMonthSettlement();
        void RenderCultureExam();
        void RenderContest();
        void RenderContestResult();
        void RenderGameOver();
        void RenderSidebar();
        void RenderHelpCard();
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
        contestResult_ = ContestResultView();
        gameOver_ = GameOverView();
        pendingContestIdx_ = 0;
        pendingContestId_ = 0;
        gameInitialized_ = false;
        helpReturnScreen_ = GuiScreen::Home;
    }

    void GuiApp::BeginSetup()
    {
        gameState.gameDifficulty = selectedDifficulty_;
        ResetSharedState();
        initGame();
        logEvent("选择了" + DifficultyLabel(gameState.gameDifficulty) + "难度", "event");
        talents_.fill(0);
        selectedBackground_.clear();
        showTalentPage_ = false;
        contestResult_ = ContestResultView();
        gameOver_ = GameOverView();
        pendingContestIdx_ = 0;
        gameInitialized_ = true;
        screen_ = GuiScreen::IntroStory;
    }

    void GuiApp::ApplyTalentAllocation()
    {
        // 9 维天赋分配
        gameState.playerStats.dp = talents_[0];
        gameState.playerStats.ds = talents_[1];
        gameState.playerStats.string = talents_[2];
        gameState.playerStats.graph = talents_[3];
        gameState.playerStats.combinatorics = talents_[4];
        gameState.playerStats.math = talents_[5];
        gameState.playerStats.geometry = talents_[6];
        gameState.playerStats.data_structure = talents_[7];
        gameState.playerStats.adhoc = talents_[8];

        // 应用背景效果
        if (!selectedBackground_.empty()) {
            Talent::applyBackground(selectedBackground_);
        }

        // 进入月回合
        StartMonthLoop();
    }

    void GuiApp::OpenHelp()
    {
        if (screen_ == GuiScreen::Help)
            return;
        helpReturnScreen_ = screen_;
        screen_ = GuiScreen::Help;
    }
    void GuiApp::StartMonthLoop()
    {
        currentMonthInfo_ = Calendar::getMonthInfo(1);
        Calendar::startMonth(1);
        pendingContestIdx_ = 0;
        screen_ = GuiScreen::MonthAction;
        logEvent("高中生活开始了！第1年7月", "event");
    }

    void GuiApp::EndMonthAction()
    {
        // 完整月度结算
        bool hasContest = !currentMonthInfo_.contestIds.empty();
        settleMonth(hasContest);

        // 检查健康
        if (Calendar::isGameOver()) {
            SetGameOver("你的身体撑不住了...健康归零。");
            return;
        }

        // 检查是否需要打比赛
        if (!currentMonthInfo_.contestIds.empty() && pendingContestIdx_ < static_cast<int>(currentMonthInfo_.contestIds.size())) {
            pendingContestId_ = currentMonthInfo_.contestIds[pendingContestIdx_];
            contestResult_ = ContestResultView();
            Contest::start(pendingContestId_);
            isActivityContest_ = false;
            screen_ = GuiScreen::Contest;
            return;
        }

        // 没有比赛 → 检查是否有考试
        if (currentMonthInfo_.hasExam) {
            CultureExam::start(currentMonthInfo_.isGaokao);
            screen_ = GuiScreen::CultureExam;
            return;
        }

        // 没有比赛也没有考试 → 直接到结算页
        screen_ = GuiScreen::MonthSettlement;
    }

    void GuiApp::ContinueAfterContest()
    {
        // 比赛结束，检查是否还有多日比赛
        pendingContestIdx_++;
        if (pendingContestIdx_ < static_cast<int>(currentMonthInfo_.contestIds.size())) {
            pendingContestId_ = currentMonthInfo_.contestIds[pendingContestIdx_];
            contestResult_ = ContestResultView();
            Contest::start(pendingContestId_);
            isActivityContest_ = false;
            screen_ = GuiScreen::Contest;
            return;
        }
        // 所有比赛打完 → 检查是否有考试
        if (currentMonthInfo_.hasExam) {
            CultureExam::start(currentMonthInfo_.isGaokao);
            screen_ = GuiScreen::CultureExam;
            return;
        }
        // 没有考试 → 结算页
        screen_ = GuiScreen::MonthSettlement;
    }

    void GuiApp::ContinueAfterSettlement()
    {
        // 推进到下个月
        int nextMonth = gameState.currentMonth + 1;
        if (nextMonth > 36) {
            SetGameOver("三年高中结束了。你的 OI 之旅到此告一段落。");
            return;
        }
        if (Calendar::isGameOver()) {
            SetGameOver("你的身体撑不住了...健康归零。");
            return;
        }

        // 学年过渡检查
        int oldYear = monthToYear(gameState.currentMonth);
        int newYear = monthToYear(nextMonth);
        if (oldYear != newYear) {
            applyYearTransition(oldYear, newYear);
        }

        currentMonthInfo_ = Calendar::getMonthInfo(nextMonth);
        Calendar::startMonth(nextMonth);
        pendingContestIdx_ = 0;

        if (Calendar::isGameOver()) {
            SetGameOver("你的身体撑不住了...健康归零。");
            return;
        }

        logEvent("第" + std::to_string(gameState.currentYear) + "年" +
                 getCalendarMonthName(gameState.calendarMonth), "event");
        screen_ = GuiScreen::MonthAction;
    }

    void GuiApp::BeginContestStep(int contestId)
    {
        contestResult_ = ContestResultView();
        Contest::start(contestId);
        screen_ = GuiScreen::Contest;
    }

    void GuiApp::FinalizeContest()
    {
        contestResult_ = Contest::finalize();
        // 奖金在此一次性结算（FinalizeContest 是产生比赛结果的唯一入口）。
        // 切勿放在 RenderContestResult() 中——那是每帧调用的渲染函数，会逐帧重复累加。
        gameState.money += contestResult_.determinationReward;
        screen_ = GuiScreen::ContestResult;
    }

    void GuiApp::CheckAndShowContestNotice(char preferredAction)
    {
        if (!hasPendingContestNotice())
            return;

        const PendingContestNotice eventNotice = consumePendingContestNotice();
        // 在月回合中，比赛通知暂用简单处理
        logEvent("【事件】" + eventNotice.title + "：" + eventNotice.description, "event");
        if (!eventNotice.effectText.empty()) {
            logEvent("效果：" + eventNotice.effectText, "event");
        }
    }

    void GuiApp::HandleContestAction(int subProblemIdx, char action)
    {
        const int problemIdx = gameState.currentProblem - 1;
        if (problemIdx < 0 || problemIdx >= static_cast<int>(gameState.subProblems.size()))
            return;
        if (subProblemIdx < 0 || subProblemIdx >= static_cast<int>(gameState.subProblems[problemIdx].size()))
            return;

        if (action == 'a')
            Contest::think(problemIdx, subProblemIdx);
        else if (action == 'b')
            Contest::code(problemIdx, subProblemIdx);
        else if (action == 'c')
            Contest::check(problemIdx, subProblemIdx);
        else
            return;

        if (hasPendingContestNotice())
        {
            CheckAndShowContestNotice('f');
            return;
        }

        if (Contest::isFullScore())
        {
            FinalizeContest();
        }
    }

    void GuiApp::ModifyCodeProblem(int problemIdx, int subProblemIdx)
    {
        if (problemIdx < 0 || problemIdx >= static_cast<int>(gameState.subProblems.size()))
            return;
        if (subProblemIdx < 0 || subProblemIdx >= static_cast<int>(gameState.subProblems[problemIdx].size()))
            return;

        Contest::modify(problemIdx, subProblemIdx);
        CheckAndShowContestNotice('r');
    }

    void GuiApp::SetGameOver(std::string reason)
    {
        gameOver_.reason = std::move(reason);
        screen_ = GuiScreen::GameOver;
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

        const float sidebarWidth = gameInitialized_ ? 312.0f : 0.0f;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
        if (sidebarWidth > 0.0f)
        {
            ImGui::BeginChild("main_content", ImVec2(-sidebarWidth - 12.0f, 0.0f), false);
        }
        else
        {
            ImGui::BeginChild("main_content", ImVec2(0.0f, 0.0f), false);
        }
        ImGui::PopStyleColor();

        switch (screen_)
        {
        case GuiScreen::Home:
            RenderHome();
            break;
        case GuiScreen::Difficulty:
            RenderDifficulty();
            break;
        case GuiScreen::IntroStory:
            RenderIntroStory();
            break;
        case GuiScreen::Help:
            RenderHelp();
            break;
        case GuiScreen::Talent:
            RenderTalent();
            break;
        case GuiScreen::MonthAction:
            RenderMonthAction();
            break;
        case GuiScreen::MonthSettlement:
            RenderMonthSettlement();
            break;
        case GuiScreen::CultureExam:
            RenderCultureExam();
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
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
            ImGui::BeginChild("sidebar", ImVec2(0.0f, 0.0f), false);
            RenderSidebar();
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::End();
    }

    void GuiApp::RenderTopBar()
    {
        ImDrawList *dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        const float chip = 24.0f;
        dl->AddRectFilled(p, ImVec2(p.x + chip, p.y + chip), ImGui::GetColorU32(OITheme::Col::TealDim), 6.0f);
        ImVec2 lt = ImGui::CalcTextSize("OI");
        dl->AddText(ImVec2(p.x + (chip - lt.x) * 0.5f, p.y + (chip - lt.y) * 0.5f), ImGui::GetColorU32(OITheme::Hex(0x04110F)), "OI");
        ImGui::Dummy(ImVec2(chip, chip));
        ImGui::SameLine(0, 10);

        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Txt);
        ImGui::TextUnformatted("OI 重开模拟器");
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 10);
        OIWidgets::Tag(kGameVersion, OITheme::Col::Teal);
        ImGui::SameLine(0, 10);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(OITheme::Col::TxtFaint, "| 36 月回合制 · 信息学竞赛生涯");

        if (!fontWarning_.empty())
        {
            ImGui::SameLine();
            ImGui::TextColored(OITheme::Col::Warn, "%s", fontWarning_.c_str());
        }

        const bool showHelp = screen_ != GuiScreen::Help;
        const bool showHome = screen_ != GuiScreen::Home;
        if (showHelp || showHome)
        {
            const float bw = 96.0f;
            const int n = static_cast<int>(showHelp) + static_cast<int>(showHome);
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - bw * n - 8.0f * (n - 1) - 8.0f);
            if (showHelp && ImGui::Button("帮助", ImVec2(bw, 0.0f)))
                OpenHelp();
            if (showHelp && showHome)
                ImGui::SameLine();
            if (showHome && ImGui::Button("返回首页", ImVec2(bw, 0.0f)))
                ResetToHome();
        }

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, OITheme::Col::Line);
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    void GuiApp::RenderHome()
    {
        const float ww = ImGui::GetWindowWidth();
        const float availH = ImGui::GetContentRegionAvail().y;
        ImGui::Dummy(ImVec2(0.0f, availH * 0.20f));

        // logo chip
        const float chip = 72.0f;
        ImGui::SetCursorPosX((ww - chip) * 0.5f);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p, ImVec2(p.x + chip, p.y + chip), ImGui::GetColorU32(OITheme::Col::TealDim), 18.0f);
        {
            ImFont *f = g_fontH1 ? g_fontH1 : ImGui::GetFont();
            ImVec2 ts = f->CalcTextSizeA(f->LegacySize, FLT_MAX, 0.0f, "OI");
            dl->AddText(f, f->LegacySize, ImVec2(p.x + (chip - ts.x) * 0.5f, p.y + (chip - ts.y) * 0.5f),
                        ImGui::GetColorU32(OITheme::Hex(0x04110F)), "OI");
        }
        ImGui::Dummy(ImVec2(chip, chip));
        ImGui::Spacing();
        ImGui::Spacing();

        if (g_fontH1) ImGui::PushFont(g_fontH1);
        const char *title = "OI 重开模拟器";
        ImGui::SetCursorPosX((ww - ImGui::CalcTextSize(title).x) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Txt);
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        if (g_fontH1) ImGui::PopFont();

        const char *subtitle = "一次重新来过的竞赛生涯，从这里开始。";
        ImGui::SetCursorPosX((ww - ImGui::CalcTextSize(subtitle).x) * 0.5f);
        ImGui::TextColored(OITheme::Col::TxtDim, "%s", subtitle);

        ImGui::Spacing();
        const char *meta = "36 月回合制    ·    9 维知识体系    ·    115 道题库";
        ImGui::SetCursorPosX((ww - ImGui::CalcTextSize(meta).x) * 0.5f);
        ImGui::TextColored(OITheme::Col::TxtFaint, "%s", meta);

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        const float btnWidth = 240.0f;
        ImGui::SetCursorPosX((ww - btnWidth) * 0.5f);
        if (OIWidgets::PrimaryButton("开始游戏  →", ImVec2(btnWidth, kPrimaryButtonHeight)))
            screen_ = GuiScreen::Difficulty;

        ImGui::Spacing();
        ImGui::Spacing();
        const float hintW = ImGui::CalcTextSize("按").x + 6 + ImGui::CalcTextSize("F1").x + 16 + 6 + ImGui::CalcTextSize("可随时查看帮助").x;
        ImGui::SetCursorPosX((ww - hintW) * 0.5f);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(OITheme::Col::TxtFaint, "按");
        ImGui::SameLine(0, 6);
        OIWidgets::Kbd("F1");
        ImGui::SameLine(0, 6);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(OITheme::Col::TxtFaint, "可随时查看帮助");
    }

    void GuiApp::RenderDifficulty()
    {
        RenderPageHeader("选择难度", "难度决定天赋点、起始金钱与分数线。选定后分配背景与天赋，开始三年高中生活。");

        struct DiffItem { const char *key; const char *name; int tp, money, income; const char *note; int tier; };
        static const DiffItem items[4] = {
            {"easy", "简单", 30, 500, 150, "分数线 −20%", 0},
            {"normal", "普通", 20, 300, 100, "分数线 −10%", 4},
            {"hard", "困难", 15, 150, 70, "标准难度", 5},
            {"expert", "专家", 15, 50, 40, "分数线 +10%", 6},
        };

        const float gap = 12.0f;
        const float cardW = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
        for (int i = 0; i < 4; ++i)
        {
            const DiffItem &d = items[i];
            const bool sel = selectedDifficulty_ == d.key;
            ImGui::PushID(i);
            if (sel) ImGui::PushStyleColor(ImGuiCol_Border, OITheme::Col::Teal);
            if (OIWidgets::BeginCard("diffcard", ImVec2(cardW, 150.0f)))
            {
                OIWidgets::TierBadge(d.name, OITheme::TierColor(d.tier));
                ImGui::Spacing();
                ImGui::Columns(3, "diffkv", false);
                ImGui::TextColored(OITheme::Col::TxtFaint, "天赋点");
                if (g_fontH2) ImGui::PushFont(g_fontH2);
                ImGui::Text("%d", d.tp);
                if (g_fontH2) ImGui::PopFont();
                ImGui::NextColumn();
                ImGui::TextColored(OITheme::Col::TxtFaint, "起始金钱");
                if (g_fontH2) ImGui::PushFont(g_fontH2);
                ImGui::Text("%d", d.money);
                if (g_fontH2) ImGui::PopFont();
                ImGui::NextColumn();
                ImGui::TextColored(OITheme::Col::TxtFaint, "月收入");
                if (g_fontH2) ImGui::PushFont(g_fontH2);
                ImGui::Text("%d", d.income);
                if (g_fontH2) ImGui::PopFont();
                ImGui::Columns(1);
                ImGui::Spacing();
                ImGui::TextColored(d.tier >= 6 ? OITheme::Col::Bad : OITheme::Col::TxtDim, "%s", d.note);
                ImGui::Spacing();
                if (sel)
                    OIWidgets::Tag("● 当前选择", OITheme::Col::Teal);
                else if (ImGui::Button("选择", ImVec2(-1, 30)))
                    selectedDifficulty_ = d.key;
            }
            OIWidgets::EndCard();
            if (sel) ImGui::PopStyleColor();
            ImGui::PopID();
            if (i % 2 == 0) ImGui::SameLine(0, gap);
        }

        ImGui::Spacing();
        if (OIWidgets::PrimaryButton("下一步：背景与天赋  →", ImVec2(kPrimaryButtonWidth + 60, kPrimaryButtonHeight)))
            BeginSetup();
    }

    void GuiApp::RenderIntroStory()
    {
        RenderPageHeader("重开", "重生回到高一前的暑假 —— 这一次，让竞赛生涯不留遗憾。");
        if (OIWidgets::BeginCard("intro_story_card", ImVec2(0.0f, 420.0f), true))
        {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Txt);
            ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
            ImGui::TextWrapped("%s", kIntroStoryText);
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }
        OIWidgets::EndCard();
        ImGui::Spacing();
        if (OIWidgets::PrimaryButton("开始高中生活  →", ImVec2(kPrimaryButtonWidth, kPrimaryButtonHeight)))
            screen_ = GuiScreen::Talent;
    }

    void GuiApp::RenderHelp()
    {
        RenderPageHeader("帮助", "这里汇总了开局、训练、比赛和关键机制的说明。右侧边栏也会根据当前界面给出速查提示。");

        static const struct { const char* title; const char* items[11]; int count; } helpSections[] = {
            {"流程概览", {"首页 -> 难度选择 -> 剧情与背景 -> 天赋分配 -> 36 月回合制。",
                           "每月有 AP（行动力），用于学习、打网赛、刷题、学文化课、休息等。",
                           "比赛月在活动结束后自动进入比赛；考试月在比赛后进行文化课考试。",
                           "月末结算：未用 AP 转健康/心态、遗忘检查、金钱收入、焦虑检查。"}, 4},
            {"月度活动", {"学习知识（2 AP）：选择 9 个知识维度之一进行学习。",
                           "打网赛（3 AP）：参加完整网赛流程，有概率获得特质。",
                           "刷题练习（2 AP）：按难度刷一道题。",
                           "自定义模拟赛（4 AP）：自选难度模板。",
                           "学文化课（2 AP）：文化课+2，心态+1。停课时不可用。",
                           "休息（2 AP）：健康+3，心态+2。",
                           "参加集训（4 AP）：7-8 月暑假限定，高强度训练。",
                           "商店：不花 AP，用金钱购买属性提升。"}, 8},
            {"比赛说明", {"标准流程通常是：思考 -> 写代码 -> 对拍/提交。",
                           "思维影响思考成功率，代码影响写代码成功率，细心影响对拍/提交稳定性。",
                           "IOI 赛制在时间为 0 时仍可继续提交；其他赛制时间为 0 后只能结束比赛。",
                           "非 IOI 比赛对拍失败后，必须先修改代码，才能再次对拍。",
                           "修改代码每次消耗 1 时间点，并按写代码成功率判定是否推进返工进度。",
                           "需要累计完成 分支+1 次成功修改，才会重新生成一版代码并恢复对拍资格。"}, 6},
            {"文化课考试", {"考试月（11月期中、1月期末、4月期中、6月期末）自动触发。",
                           "流程：阅读 -> 认真作答 或 蒙题 -> 检查（可选）-> 下一题。",
                           "作答正确率 = 0.3 + 文化*0.04 + 思维*0.02 - 难度*0.05",
                           "蒙题正确率 = 0.25 + 文化*0.03 + 运气*0.01",
                           "高考（第36月）有文化课乘数：文化>=16 得分x1.2，<10 得分x0.6。",
                           "跨年时文化课效率会根据文化属性重新计算。"}, 6},
            {"关键机制", {"心态（0-14）：影响学习效率，低于 4 连续 2 月可能触发焦虑。",
                           "健康（0-20）：归零则游戏结束。注意休息。",
                           "遗忘：连续 2 月未学习的知识维度会 -1。",
                           "停课：每月+2 AP，但心态-3，不能学文化课。",
                           "特质：从模拟赛/刷题中有概率获得，最多 4 个。",
                           "背景：开局选择，影响心态上限、学习效率、焦虑概率等。"}, 6},
            {"属性速览", {"9 维知识：DP / DS / 字符串 / 图论 / 组合计数 / 数学 / 几何 / 高级DS / 构造",
                           "思维：影响思考成功率。代码：影响写代码成功率。",
                           "细心：降低对拍翻车概率。迅捷：降低写代码耗时。",
                           "心理素质：降低心态崩盘风险。运气：降低负面事件概率。",
                           "经验：抵消模糊等级。文化课：影响考试和月收入。",
                           "所有属性上限 20。"}, 4},
            {"题目属性", {"动态规划 / 数据结构 / 字符串 / 图论 / 组合计数：这部分分主要考察的知识方向及要求强度。要求越高，而你的对应能力越不足，思考这部分分时花费的时间就越多。",
                           "思维：这部分分对理解、转化和发现关键做法的要求。数值越高，思考成功率越低。",
                           "代码：这部分分的实现工作量。数值越高，写代码所需的进度越多。",
                           "细节：这部分分在实现上的繁琐程度和出错空间。数值越高，写代码成功率越低。",
                           "陷阱：这部分分暗坑、卡点和隐藏错误的强度。数值越高，对拍或提交时翻车概率越高。",
                           "模糊：题目描述中对难度和知识点信息的隐藏程度。等级越高，你能直接看到的要求和特性越少；如果你有足够的经验，这些模糊描述就骗不了你。",
                           "分支：这部分分可能解法之间区别的复杂程度。数值越高，对拍失败后需要完成的修改次数越多。",
                           "激励：取得进展时带来的正反馈强度。数值越高，写完代码或对拍成功后恢复的心态越多。",
                           "红温：失败后的心态冲击强度。数值越高，思考失败或写代码失败时额外损失的心态越多。",
                           "Adhoc：这部分分对临场观察、构造、找性质等非模板化能力的要求。数值越高，思考耗时越长。",
                           "非独立：这部分分与前面的相关部分分存在联动。成功推进它时，可能会顺带推进前面同类的非独立部分分。"}, 11},
        };

        for (const auto& section : helpSections)
        {
            if (ImGui::CollapsingHeader(section.title, ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (int j = 0; j < section.count; ++j)
                    ImGui::BulletText("%s", section.items[j]);
            }
        }
        ImGui::Spacing();
        if (ImGui::Button("返回上一页", ImVec2(kSecondaryButtonWidth, kSecondaryButtonHeight)))
        {
            screen_ = helpReturnScreen_;
        }
    }

    void GuiApp::RenderTalent()
    {
        RenderPageHeader("背景与天赋", "背景影响心态上限、学习效率与焦虑概率，知识天赋为各维度起始值。");

        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(OITheme::Col::TxtFaint, "当前难度");
        ImGui::SameLine(0, 8);
        {
            ImVec4 dc = selectedDifficulty_ == "easy" ? OITheme::Col::LgGray
                      : selectedDifficulty_ == "normal" ? OITheme::Col::LgGreen
                      : selectedDifficulty_ == "expert" ? OITheme::Col::LgPurple
                      : OITheme::Col::LgBlue;
            OIWidgets::TierBadge(DifficultyLabel(selectedDifficulty_).c_str(), dc);
        }
        ImGui::Spacing();

        RenderSection("STEP 1", "选择背景 · 选择后不可更改");
        const float gap = 12.0f;
        const int total = static_cast<int>(Talent::BACKGROUNDS.size());
        const float cardW = (ImGui::GetContentRegionAvail().x - 2.0f * gap) / 3.0f;
        for (int i = 0; i < total; ++i)
        {
            const auto &bg = Talent::BACKGROUNDS[i];
            const bool sel = (selectedBackground_ == bg.id);
            ImGui::PushID(i);
            if (sel) ImGui::PushStyleColor(ImGuiCol_Border, OITheme::Col::Teal);
            if (OIWidgets::BeginCard("bgc", ImVec2(cardW, 140.0f)))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, sel ? OITheme::Col::Teal : OITheme::Col::Txt);
                ImGui::TextUnformatted(bg.name);
                ImGui::PopStyleColor();
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::TxtDim);
                ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
                ImGui::TextWrapped("%s", bg.desc);
                ImGui::PopTextWrapPos();
                ImGui::PopStyleColor();
                ImGui::Spacing();
                if (sel)
                    OIWidgets::Tag("● 已选择", OITheme::Col::Teal);
                else if (ImGui::Button("选择", ImVec2(-1, 28)))
                    selectedBackground_ = bg.id;
            }
            OIWidgets::EndCard();
            if (sel) ImGui::PopStyleColor();
            ImGui::PopID();
            if ((i % 3) != 2 && i != total - 1) ImGui::SameLine(0, gap);
        }

        ImGui::Spacing();
        RenderSection("STEP 2", "分配初始天赋 · 可不花完");
        if (OIWidgets::BeginCard("talentcard"))
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(OITheme::Col::TxtFaint, "剩余天赋点");
            ImGui::SameLine(0, 8);
            if (g_fontH2) ImGui::PushFont(g_fontH2);
            ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Teal);
            ImGui::Text("%d", RemainingTalent());
            ImGui::PopStyleColor();
            if (g_fontH2) ImGui::PopFont();
            ImGui::SameLine(0, 6);
            ImGui::TextColored(OITheme::Col::TxtFaint, "/ %d", TalentBudget());
            ImGui::Spacing();

            static const std::array<const char *, 9> labels = {
                "动态规划", "数据结构", "字符串", "图论", "组合计数",
                "数学", "几何", "高级数据结构", "构造 / 思维"};
            ImGui::Columns(2, "talcols", false);
            for (int i = 0; i < 9; ++i)
            {
                const int maxForCurrent = talents_[i] + RemainingTalent();
                ImGui::PushID(100 + i);
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(OITheme::Col::TxtDim, "%s", labels[i]);
                ImGui::SameLine(128);
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderInt("##talent", &talents_[i], 0, std::max(0, maxForCurrent), "%d");
                ImGui::PopID();
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }
        OIWidgets::EndCard();

        ImGui::Spacing();
        ImGui::BeginDisabled(selectedBackground_.empty());
        if (OIWidgets::PrimaryButton("确认并开始  →", ImVec2(kPrimaryButtonWidth, kPrimaryButtonHeight)))
            ApplyTalentAllocation();
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("← 返回难度选择", ImVec2(kSecondaryButtonWidth, kPrimaryButtonHeight)))
            ResetToHome();
        if (selectedBackground_.empty())
        {
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(OITheme::Col::TxtFaint, "请先选择一个背景");
        }
    }

    void GuiApp::RenderMonthAction()
    {
        // —— 月度头部卡片 ——
        double eff = getStudyEfficiency();
        if (OIWidgets::BeginCard("month_head"))
        {
            ImGui::BeginGroup();
            if (g_fontH2) ImGui::PushFont(g_fontH2);
            ImGui::Text("第%d年 %s", gameState.currentYear, getCalendarMonthName(gameState.calendarMonth));
            if (g_fontH2) ImGui::PopFont();
            ImGui::SameLine(0, 10);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(OITheme::Col::TxtFaint, "第 %d / 36 月", gameState.currentMonth);

            ImGui::Spacing();
            bool any = false;
            if (!currentMonthInfo_.contestIds.empty()) {
                std::string names;
                for (size_t k = 0; k < currentMonthInfo_.contestIds.size(); ++k) {
                    if (k) names += " ";
                    auto it = CONTEST_CONFIGS.find(currentMonthInfo_.contestIds[k]);
                    names += (it != CONTEST_CONFIGS.end()) ? it->second.name : "比赛";
                }
                OIWidgets::TierBadge(("本月比赛 · " + names).c_str(), OITheme::Col::LgOrange);
                any = true;
            }
            if (currentMonthInfo_.hasExam) {
                if (any) ImGui::SameLine();
                OIWidgets::TierBadge("本月考试", OITheme::Col::LgBlue);
                any = true;
            }
            {
                int hm = gameState.calendarMonth;
                if ((hm == 7 || hm == 8) && gameState.currentYear <= 2) {
                    if (any) ImGui::SameLine();
                    OIWidgets::TierBadge("暑假 · 可集训", OITheme::Col::LgGreen);
                    any = true;
                }
            }
            if (gameState.isAnxious) {
                if (any) ImGui::SameLine();
                OIWidgets::TierBadge("焦虑中 · 效率降低", OITheme::Col::Bad);
                any = true;
            }
            if (!any) ImGui::TextColored(OITheme::Col::TxtFaint, "本月无特殊安排");
            ImGui::EndGroup();

            const float rightW = 360.0f;
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - rightW);
            ImGui::BeginGroup();
            ImGui::TextColored(OITheme::Col::TxtFaint, "学习效率");
            int effPct = static_cast<int>(eff * 100);
            ImVec4 effc = effPct >= 100 ? OITheme::Col::Ok : (effPct >= 80 ? OITheme::Col::Warn : OITheme::Col::Bad);
            if (g_fontH2) ImGui::PushFont(g_fontH2);
            ImGui::PushStyleColor(ImGuiCol_Text, effc);
            ImGui::Text("%d%%", effPct);
            ImGui::PopStyleColor();
            if (g_fontH2) ImGui::PopFont();
            ImGui::EndGroup();

            ImGui::SameLine(0, 24);
            ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(OITheme::Col::TxtFaint, "行动力 AP");
            ImGui::SameLine(0, 8);
            ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Teal);
            ImGui::Text("%d / %d", gameState.ap, gameState.maxAp);
            ImGui::PopStyleColor();
            OIWidgets::ApPips(gameState.ap, gameState.maxAp, 14.0f, 8.0f, 3.0f);
            ImGui::EndGroup();
        }
        OIWidgets::EndCard();

        if (ImGui::Checkbox("停课  (+2 AP · 心态 −3 · 禁文化课)", &gameState.isTingke)) {
            const auto& settings = DIFFICULTY_SETTINGS.at(gameState.gameDifficulty);
            int baseAp = gameState.isTingke ? 10 : 8;
            baseAp += settings.apBonus;
            int spent = gameState.maxAp - gameState.ap;
            gameState.maxAp = baseAp - currentMonthInfo_.apDeduction;
            gameState.ap = std::max(0, gameState.maxAp - spent);
        }

        ImGui::Spacing();

        if (ImGui::BeginTabBar("activity_tabs", ImGuiTabBarFlags_FittingPolicyScroll))
        {
            if (ImGui::BeginTabItem("学习知识"))
            {
                ImGui::Spacing();
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(OITheme::Col::TxtDim, "选择学习方向 · 每次消耗");
                ImGui::SameLine(0, 6);
                OIWidgets::Tag("2 AP", OITheme::Col::Teal);
                ImGui::SameLine(0, 6);
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(OITheme::Col::TxtFaint, "健康 −1");
                ImGui::Spacing();
                ImGui::Columns(3, "learncols", false);
                for (int i = 0; i < static_cast<int>(KNOWLEDGE_DIMS.size()); ++i) {
                    const std::string& dim = KNOWLEDGE_DIMS[i];
                    int val = 0;
                    auto it = STAT_MEMBER_MAP.find(dim);
                    if (it != STAT_MEMBER_MAP.end()) val = gameState.playerStats.*(it->second);
                    bool canAfford = gameState.ap >= 2;
                    ImGui::PushID(i);
                    ImGui::BeginDisabled(!canAfford);
                    std::string label = Utils::getStatName(dim) + "    " + std::to_string(val) + "##learn";
                    if (ImGui::Button(label.c_str(), ImVec2(-1.0f, 34.0f))) {
                        gameState.ap -= 2;
                        gameState.health = std::max(0, gameState.health - 1);
                        auto result = Activity::execute(Activity::Learn, i);
                        for (const auto& log : result.logs) logEvent(log, "event");
                    }
                    ImGui::EndDisabled();
                    OIWidgets::Bar(val / 20.0f, ImVec2(ImGui::GetContentRegionAvail().x, 6.0f), OITheme::Col::Teal);
                    ImGui::PopID();
                    ImGui::Spacing();
                    ImGui::NextColumn();
                }
                ImGui::Columns(1);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("网赛"))
            {
                ImGui::Spacing();
                ImGui::Text("选择网赛 (3 AP)：");
                ImGui::Separator();
                for (int i = 0; i < static_cast<int>(Activity::MOCK_CONTEST_OPTIONS.size()); ++i) {
                    const auto& opt = Activity::MOCK_CONTEST_OPTIONS[i];
                    bool canAfford = gameState.ap >= 3;
                    ImGui::PushID(i);
                    ImGui::BeginChild(("mock_" + std::to_string(i)).c_str(), ImVec2(0.0f, 48.0f), true);
                    ImGui::Text("%s", opt.name);
                    ImGui::SameLine(200.0f);
                    ImGui::TextDisabled("%s", opt.desc);
                    ImGui::SameLine(400.0f);
                    ImGui::BeginDisabled(!canAfford);
                    if (ImGui::Button("参加##mock", ImVec2(80.0f, 28.0f))) {
                        gameState.ap -= 3;
                        gameState.health = std::max(0, gameState.health - 1);
                        auto result = Activity::execute(Activity::MockContest, i);
                        for (const auto& log : result.logs) logEvent(log, "event");
                        if (result.startContest) {
                            contestResult_ = ContestResultView();
                            Contest::start(result.contestId);
                            isActivityContest_ = true;
                            screen_ = GuiScreen::Contest;
                            ImGui::EndDisabled();
                            ImGui::EndChild();
                            ImGui::PopID();
                            ImGui::EndTabItem();
                            ImGui::EndTabBar();
                            return;
                        }
                    }
                    ImGui::EndDisabled();
                    ImGui::EndChild();
                    ImGui::PopID();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("刷题"))
            {
                ImGui::Spacing();
                ImGui::Text("选择刷题难度 (2 AP)：");
                ImGui::Separator();
                for (int i = 0; i < static_cast<int>(Activity::PRACTICE_OPTIONS.size()); ++i) {
                    const auto& opt = Activity::PRACTICE_OPTIONS[i];
                    bool canAfford = gameState.ap >= 2;
                    ImGui::PushID(i);
                    int contestId = opt.contestId;
                    auto cfgIt = CONTEST_CONFIGS.find(contestId);
                    int level = (cfgIt != CONTEST_CONFIGS.end() && !cfgIt->second.problemRanges.empty())
                        ? (cfgIt->second.problemRanges[0].first + cfgIt->second.problemRanges[0].second) / 2 : 3;
                    ImVec4 tierColor = getLuoguTierColor(level);
                    ImGui::BeginChild(("prac_" + std::to_string(i)).c_str(), ImVec2(0.0f, 40.0f), true);
                    ImGui::TextColored(tierColor, "[%s]", opt.tier);
                    ImGui::SameLine();
                    ImGui::Text("%s", opt.name);
                    ImGui::SameLine(300.0f);
                    ImGui::BeginDisabled(!canAfford);
                    if (ImGui::Button("开刷##prac", ImVec2(80.0f, 28.0f))) {
                        gameState.ap -= 2;
                        gameState.health = std::max(0, gameState.health - 1);
                        auto result = Activity::execute(Activity::Practice, i);
                        for (const auto& log : result.logs) logEvent(log, "event");
                        if (result.startContest) {
                            contestResult_ = ContestResultView();
                            Contest::start(result.contestId);
                            isActivityContest_ = true;
                            screen_ = GuiScreen::Contest;
                            ImGui::EndDisabled();
                            ImGui::EndChild();
                            ImGui::PopID();
                            ImGui::EndTabItem();
                            ImGui::EndTabBar();
                            return;
                        }
                    }
                    ImGui::EndDisabled();
                    ImGui::EndChild();
                    ImGui::PopID();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("自定义比赛"))
            {
                ImGui::Spacing();
                ImGui::Text("自定义模拟赛（消耗 4 AP，含 1 AP 额外费用）");
                ImGui::Separator();
                ImGui::Text("选择难度模板：");
                for (int i = 0; i < static_cast<int>(CONTEST_TEMPLATES.size()); ++i) {
                    const auto& tpl = CONTEST_TEMPLATES[i];
                    bool selected = (customTemplateIdx_ == i);
                    if (selected) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.42f, 0.65f, 1.0f));
                    }
                    std::string label = std::string(tpl.name) + " (" + std::to_string(tpl.numProblems) + "题, " + std::to_string(tpl.timePoints) + "时间)##tpl";
                    if (ImGui::Button(label.c_str(), ImVec2(300.0f, 32.0f))) {
                        customTemplateIdx_ = i;
                    }
                    if (selected) ImGui::PopStyleColor();
                }
                ImGui::Spacing();

                if (customTemplateIdx_ >= 0) {
                    const auto& tpl = CONTEST_TEMPLATES[customTemplateIdx_];
                    ImGui::TextColored(ImVec4(0.40f, 0.72f, 0.90f, 1.0f), "已选择：%s", tpl.name);
                    ImGui::BulletText("题目数：%d  |  时间：%d  |  赛制：%s", tpl.numProblems, tpl.timePoints, tpl.isIOI ? "IOI" : "OI");
                    ImGui::Spacing();
                    bool canAfford = gameState.ap >= 4;
                    ImGui::BeginDisabled(!canAfford);
                    if (ImGui::Button("开始自定义比赛", ImVec2(200.0f, 36.0f))) {
                        gameState.ap -= 4;
                        gameState.health = std::max(0, gameState.health - 2);
                        gameState.currentContestName = tpl.name;
                        gameState.timePoints = tpl.timePoints;
                        gameState.totalProblems = tpl.numProblems;
                        gameState.problems.clear();
                        gameState.subProblems.clear();
                        gameState.contestStates.clear();
                        for (int p = 0; p < tpl.numProblems; ++p) {
                            Problem prob;
                            prob.name = generateProblemName();
                            prob.level = (tpl.ranges[p].first + tpl.ranges[p].second) / 2;
                            prob.tag = p;
                            gameState.problems.push_back(prob);
                            auto selected = selectProblemFromRange(tpl.ranges[p].first, tpl.ranges[p].second);
                            gameState.subProblems.push_back(selected.parts);
                            std::vector<ContestSubProblemState> states(selected.parts.size());
                            gameState.contestStates.push_back(states);
                            gameState.problems.back().name = generateProblemName();
                        }
                        gameState.currentProblem = 1;
                        isActivityContest_ = true;
                        contestResult_ = ContestResultView();
                        screen_ = GuiScreen::Contest;
                        customTemplateIdx_ = -1;
                        logEvent("开始自定义比赛：" + std::string(tpl.name), "event");
                        ImGui::EndDisabled();
                        ImGui::EndTabItem();
                        ImGui::EndTabBar();
                        return;
                    }
                    ImGui::EndDisabled();
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, OITheme::Col::Line);
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        auto activities = Activity::getAvailable();
        bool hasCultureBtn = false, hasRestBtn = false, hasCampBtn = false;
        for (const auto& act : activities) {
            if (act.type == Activity::StudyCulture) hasCultureBtn = true;
            if (act.type == Activity::Rest) hasRestBtn = true;
            if (act.type == Activity::SummerCamp) hasCampBtn = true;
        }

        if (hasCultureBtn || hasRestBtn || hasCampBtn) {
            if (hasCultureBtn) {
                bool canDo = gameState.ap >= 2 && !gameState.isTingke;
                ImGui::BeginDisabled(!canDo);
                if (ImGui::Button("学文化课 · 2 AP", ImVec2(150.0f, 38.0f))) {
                    gameState.ap -= 2;
                    gameState.health = std::max(0, gameState.health - 1);
                    auto result = Activity::execute(Activity::StudyCulture);
                    for (const auto& log : result.logs) logEvent(log, "event");
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
            }
            if (hasRestBtn) {
                bool canDo = gameState.ap >= 2;
                ImGui::BeginDisabled(!canDo);
                if (ImGui::Button("休息 · 2 AP", ImVec2(130.0f, 38.0f))) {
                    gameState.ap -= 2;
                    gameState.health = std::max(0, gameState.health - 1);
                    auto result = Activity::execute(Activity::Rest);
                    for (const auto& log : result.logs) logEvent(log, "event");
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
            }
            if (hasCampBtn) {
                bool canDo = gameState.ap >= 4;
                ImGui::BeginDisabled(!canDo);
                if (ImGui::Button("集训 · 4 AP", ImVec2(130.0f, 38.0f))) {
                    gameState.ap -= 4;
                    auto result = Activity::execute(Activity::SummerCamp);
                    for (const auto& log : result.logs) logEvent(log, "event");
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
            }
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, OITheme::Col::Line);
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        if (OIWidgets::GhostButton("◆  商店", OITheme::Col::Teal, ImVec2(kSecondaryButtonWidth, kPrimaryButtonHeight))) {
            showShop_ = true;
        }
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(OITheme::Col::TxtFaint, "持有金钱 %d 元", gameState.money);

        if (gameState.ap <= 0) {
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(OITheme::Col::Warn, "· 行动力已用完");
        }

        if (showShop_) {
            ImGui::OpenPopup("商店");
            ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_FirstUseEver);
            if (ImGui::BeginPopupModal("商店", &showShop_)) {
                ImGui::Text("金钱：%d 元", gameState.money);
                ImGui::Separator();
                auto shopOptions = buildShopOptions();
                for (size_t i = 0; i < shopOptions.size(); ++i) {
                    const auto& opt = shopOptions[i];
                    ImGui::PushID(static_cast<int>(i));
                    bool canAfford = gameState.money >= opt.cost;
                    ImGui::BeginDisabled(!canAfford);
                    ImGui::TextWrapped("%s", opt.text.c_str());
                    if (!opt.description.empty()) {
                        ImGui::TextDisabled("%s", opt.description.c_str());
                    }
                    ImGui::Text("价格：%d 元", opt.cost);
                    ImGui::SameLine(300.0f);
                    if (ImGui::Button("购买", ImVec2(80.0f, 28.0f))) {
                        if (opt.text == "放弃购买") {
                            showShop_ = false;
                            ImGui::CloseCurrentPopup();
                        } else if (gameState.money >= opt.cost) {
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
                        }
                    }
                    ImGui::EndDisabled();
                    ImGui::Separator();
                    ImGui::PopID();
                }
                ImGui::EndPopup();
            }
        }

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - kPrimaryButtonWidth);
        if (OIWidgets::PrimaryButton("结束本月  →", ImVec2(kPrimaryButtonWidth, kPrimaryButtonHeight))) {
            EndMonthAction();
        }
    }

    void GuiApp::RenderMonthSettlement()
    {
        char head[96];
        snprintf(head, sizeof head, "第 %d 月结算 · 第%d年 %s", gameState.currentMonth,
                 gameState.currentYear, getCalendarMonthName(gameState.calendarMonth));
        RenderSection("SETTLEMENT", head);

        std::vector<const char*> healthLogs, knowledgeLogs, moneyLogs, otherLogs;
        for (const auto& log : gameState.settlementLogs) {
            if (log.find("健康") != std::string::npos || log.find("心态") != std::string::npos ||
                log.find("焦虑") != std::string::npos)
                healthLogs.push_back(log.c_str());
            else if (log.find("遗忘") != std::string::npos || log.find("月未学") != std::string::npos)
                knowledgeLogs.push_back(log.c_str());
            else if (log.find("零花钱") != std::string::npos || log.find("元") != std::string::npos ||
                     log.find("收入") != std::string::npos)
                moneyLogs.push_back(log.c_str());
            else
                otherLogs.push_back(log.c_str());
        }

        const float gap = 14.0f;
        const float cardW = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
        const float cardH = 158.0f;

        auto sCard = [&](const char* id, ImVec4 accent, const char* title, const char* headline,
                         const std::vector<const char*>& logs, const char* emptyText) {
            if (accent.w == 0) accent = OITheme::Col::Teal;
            if (OIWidgets::BeginCard(id, ImVec2(cardW, cardH))) {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 hp = ImGui::GetCursorScreenPos();
                dl->AddRectFilled(ImVec2(hp.x, hp.y + 1), ImVec2(hp.x + 3, hp.y + ImGui::GetTextLineHeight() - 1), ImGui::GetColorU32(accent), 1.5f);
                ImGui::SetCursorScreenPos(ImVec2(hp.x + 12, hp.y));
                ImGui::PushStyleColor(ImGuiCol_Text, accent);
                ImGui::TextUnformatted(title);
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Separator, OITheme::Col::Line);
                ImGui::Separator();
                ImGui::PopStyleColor();
                if (headline && headline[0]) {
                    ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Txt);
                    ImGui::TextUnformatted(headline);
                    ImGui::PopStyleColor();
                }
                ImGui::Spacing();
                if (logs.empty() && emptyText) {
                    ImGui::TextColored(OITheme::Col::TxtFaint, "%s", emptyText);
                } else {
                    for (const char* lg : logs) {
                        ImGui::TextColored(accent, "▍");
                        ImGui::SameLine(0, 6);
                        ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::TxtDim);
                        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
                        ImGui::TextWrapped("%s", lg);
                        ImGui::PopTextWrapPos();
                        ImGui::PopStyleColor();
                    }
                }
            }
            OIWidgets::EndCard();
        };

        char hpLine[96];
        snprintf(hpLine, sizeof hpLine, "HP %d/20  ·  心态 %d/%d%s", gameState.health, gameState.mood,
                 gameState.moodCap, gameState.isAnxious ? "  [焦虑]" : "");
        sCard("settle_health", OITheme::Col::Ok, "健康与心态", hpLine, healthLogs, nullptr);
        ImGui::SameLine(0, gap);
        sCard("settle_know", OITheme::Col::Info, "知识与遗忘", nullptr, knowledgeLogs, "本月无遗忘");

        char moneyLine[64];
        snprintf(moneyLine, sizeof moneyLine, "金钱 %d 元", gameState.money);
        sCard("settle_money", OITheme::Col::Warn, "经济", moneyLine, moneyLogs, nullptr);
        ImGui::SameLine(0, gap);

        // —— 下月预告（自定义内容）——
        if (OIWidgets::BeginCard("settle_preview", ImVec2(cardW, cardH))) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 hp = ImGui::GetCursorScreenPos();
            dl->AddRectFilled(ImVec2(hp.x, hp.y + 1), ImVec2(hp.x + 3, hp.y + ImGui::GetTextLineHeight() - 1), ImGui::GetColorU32(OITheme::Col::Teal), 1.5f);
            ImGui::SetCursorScreenPos(ImVec2(hp.x + 12, hp.y));
            ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Teal);
            ImGui::TextUnformatted("下月预告");
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Separator, OITheme::Col::Line);
            ImGui::Separator();
            ImGui::PopStyleColor();

            int nextMonth = gameState.currentMonth + 1;
            if (nextMonth <= 36) {
                int oldYear = monthToYear(gameState.currentMonth);
                int newYear = monthToYear(nextMonth);
                auto nextInfo = Calendar::getMonthInfo(nextMonth);
                ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Txt);
                ImGui::Text("第%d年 %s", nextInfo.year, getCalendarMonthName(nextInfo.calendarMonth));
                ImGui::PopStyleColor();
                if (oldYear != newYear) {
                    ImGui::SameLine(0, 8);
                    OIWidgets::Tag("升入新学年", OITheme::Col::LgYellow);
                }
                ImGui::Spacing();
                if (!nextInfo.contestIds.empty()) {
                    std::string contests;
                    for (size_t i = 0; i < nextInfo.contestIds.size(); ++i) {
                        if (i > 0) contests += " ";
                        auto it = CONTEST_CONFIGS.find(nextInfo.contestIds[i]);
                        contests += (it != CONTEST_CONFIGS.end()) ? it->second.name : "???";
                    }
                    OIWidgets::TierBadge(("比赛 · " + contests).c_str(), OITheme::Col::LgOrange);
                }
                if (nextInfo.hasExam) { ImGui::SameLine(); OIWidgets::TierBadge("考试", OITheme::Col::LgBlue); }
                if ((nextInfo.calendarMonth == 7 || nextInfo.calendarMonth == 8) && nextInfo.year <= 2) {
                    ImGui::SameLine(); OIWidgets::TierBadge("暑假集训", OITheme::Col::LgGreen);
                }
            } else {
                ImGui::TextColored(OITheme::Col::TxtFaint, "这是最后一个月");
            }
        }
        OIWidgets::EndCard();

        if (!otherLogs.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(OITheme::Col::TxtFaint, "其他");
            for (const auto* log : otherLogs) {
                ImGui::TextColored(OITheme::Col::Teal, "▍");
                ImGui::SameLine(0, 6);
                ImGui::TextColored(OITheme::Col::TxtDim, "%s", log);
            }
        }

        ImGui::Spacing();
        ImGui::Spacing();
        if (OIWidgets::PrimaryButton("继续  →", ImVec2(kPrimaryButtonWidth, kPrimaryButtonHeight))) {
            ContinueAfterSettlement();
        }
    }

    void GuiApp::RenderCultureExam()
    {
        auto& es = CultureExam::examState;
        bool isGaokao = currentMonthInfo_.isGaokao;
        const char* examTitle = isGaokao ? "高考" : "期末考试";
        int cm = gameState.calendarMonth;
        if (!isGaokao && (cm == 5 || cm == 11)) examTitle = "期中考试";

        char head[64];
        snprintf(head, sizeof head, "第%d年%d月 · %s", gameState.currentYear, gameState.calendarMonth, examTitle);
        RenderSection(isGaokao ? "GAOKAO" : "EXAM", head);

        if (CultureExam::isComplete()) {
            int score = CultureExam::finalize();
            double pct = (es.maxScore > 0) ? (double)score / es.maxScore : 0;

            if (OIWidgets::BeginCard("exam_done")) {
                OIWidgets::Tag("考试结束", OITheme::Col::LgYellow);
                ImGui::Spacing();
                ImGui::TextColored(OITheme::Col::TxtFaint, "总分");
                ImGui::SameLine(0, 10);
                if (g_fontH1) ImGui::PushFont(g_fontH1);
                ImVec4 sc = pct >= 0.8 ? OITheme::Col::Ok : (pct >= 0.6 ? OITheme::Col::Warn : OITheme::Col::Bad);
                ImGui::PushStyleColor(ImGuiCol_Text, sc);
                ImGui::Text("%d", score);
                ImGui::PopStyleColor();
                if (g_fontH1) ImGui::PopFont();
                ImGui::SameLine(0, 6);
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(OITheme::Col::TxtFaint, "/ %d", es.maxScore);
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Separator, OITheme::Col::Line);
                ImGui::Separator();
                ImGui::PopStyleColor();
                ImGui::Spacing();
                for (int i = 0; i < es.totalQ; ++i) {
                    bool ok = es.results[i] == 1;
                    ImGui::TextColored(ok ? OITheme::Col::Ok : OITheme::Col::Bad, ok ? "●" : "○");
                    ImGui::SameLine(0, 6);
                    ImGui::TextColored(OITheme::Col::TxtDim, "第%d题", i + 1);
                    ImGui::SameLine(0, 6);
                    ImGui::TextColored(ok ? OITheme::Col::Ok : OITheme::Col::Bad, ok ? "正确" : "错误");
                    if (es.guessed[i]) { ImGui::SameLine(0, 6); ImGui::TextColored(OITheme::Col::TxtFaint, "蒙题"); }
                    if (es.checked[i]) { ImGui::SameLine(0, 6); ImGui::TextColored(OITheme::Col::TxtFaint, "已检查"); }
                    if ((i % 2) == 0 && i + 1 < es.totalQ) ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.5f + 120.0f);
                }
                if (isGaokao) {
                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Separator, OITheme::Col::Line);
                    ImGui::Separator();
                    ImGui::PopStyleColor();
                    if (pct >= 0.9) ImGui::TextColored(OITheme::Col::LgYellow, "你的高考成绩优异！文化课功底扎实。");
                    else if (pct >= 0.7) ImGui::TextColored(OITheme::Col::Ok, "高考成绩良好，发挥了正常水平。");
                    else if (pct >= 0.5) ImGui::TextColored(OITheme::Col::TxtDim, "高考成绩一般，有些遗憾。");
                    else ImGui::TextColored(OITheme::Col::Bad, "高考成绩不太理想……");
                }
            }
            OIWidgets::EndCard();

            ImGui::Spacing();
            if (OIWidgets::PrimaryButton("查看月度结算  →", ImVec2(kPrimaryButtonWidth, kPrimaryButtonHeight))) {
                recordExamScore(score, es.maxScore, isGaokao);
                settleMonth(!currentMonthInfo_.contestIds.empty());
                screen_ = GuiScreen::MonthSettlement;
            }
            return;
        }

        int q = es.currentQ;

        // 题目卡片
        if (OIWidgets::BeginCard("exam_q", ImVec2(0, 220))) {
            ImGui::AlignTextToFramePadding();
            if (g_fontH2) ImGui::PushFont(g_fontH2);
            ImGui::Text("题目 %d / %d", q + 1, es.totalQ);
            if (g_fontH2) ImGui::PopFont();
            ImGui::SameLine(0, 10);
            OIWidgets::Tag(CultureExam::getPhaseName(es.phases[q]), OITheme::Col::Info);
            ImGui::PushStyleColor(ImGuiCol_Separator, OITheme::Col::Line);
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::Spacing();
            if (es.phases[q] != CultureExam::Phase::Read) {
                int year = gameState.currentYear;
                int baseDiff = (year == 1) ? 3 : (year == 2) ? 4 : 5;
                std::string qText = CultureExam::generateQuestionText(baseDiff, q);
                ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Txt);
                ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
                ImGui::TextWrapped("%s", qText.c_str());
                ImGui::PopTextWrapPos();
                ImGui::PopStyleColor();
            } else {
                ImGui::TextColored(OITheme::Col::TxtFaint, "请先阅读题目……");
            }
        }
        OIWidgets::EndCard();

        ImGui::Spacing();
        const ImVec2 bsz(128.0f, 38.0f);
        if (es.phases[q] == CultureExam::Phase::Read) {
            if (OIWidgets::PrimaryButton("阅读题目", bsz)) CultureExam::readQuestion();
        } else if (es.phases[q] == CultureExam::Phase::Answer) {
            if (OIWidgets::PrimaryButton("认真作答", bsz)) CultureExam::answerQuestion();
            ImGui::SameLine();
            if (ImGui::Button("蒙一个", bsz)) CultureExam::guessQuestion();
        } else if (es.phases[q] == CultureExam::Phase::Guess) {
            if (ImGui::Button("继续", bsz)) {}
        } else if (es.phases[q] == CultureExam::Phase::Check) {
            if (!es.checked[q]) {
                if (OIWidgets::PrimaryButton("检查答案", bsz)) CultureExam::checkQuestion();
                ImGui::SameLine();
                if (ImGui::Button("不检查了", bsz)) es.phases[q] = CultureExam::Phase::Done;
            } else {
                ImGui::TextColored(OITheme::Col::TxtDim, "已完成检查");
                es.phases[q] = CultureExam::Phase::Done;
            }
        }

        if (es.phases[q] == CultureExam::Phase::Done) {
            ImGui::Spacing();
            bool ok = es.results[q] == 1;
            ImGui::TextColored(ok ? OITheme::Col::Ok : OITheme::Col::Bad, ok ? "● 本题正确" : "○ 本题错误");
            if (es.guessed[q]) { ImGui::SameLine(0, 6); ImGui::TextColored(OITheme::Col::TxtFaint, "(蒙题)"); }
            if (es.checked[q]) { ImGui::SameLine(0, 6); ImGui::TextColored(OITheme::Col::TxtFaint, "(已检查)"); }
            ImGui::Spacing();
            if (q + 1 < es.totalQ) {
                if (OIWidgets::PrimaryButton("下一题  →", bsz)) CultureExam::nextQuestion();
            } else {
                if (OIWidgets::PrimaryButton("交卷", bsz)) CultureExam::nextQuestion();
            }
        }

        // 底部进度点
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, OITheme::Col::Line);
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(OITheme::Col::TxtFaint, "进度");
        for (int i = 0; i < es.totalQ; ++i) {
            ImGui::SameLine(0, 5);
            if (es.results[i] == 1) ImGui::TextColored(OITheme::Col::Ok, "●");
            else if (es.results[i] == 0) ImGui::TextColored(OITheme::Col::Bad, "●");
            else if (i == es.currentQ) ImGui::TextColored(OITheme::Col::Warn, "◆");
            else ImGui::TextColored(OITheme::Col::TxtFaint, "○");
        }
    }

    void GuiApp::RenderContest()
    {
        // —— 比赛头部卡片 ——
        bool endContest = false;
        if (OIWidgets::BeginCard("contest_head"))
        {
            ImGui::BeginGroup();
            if (g_fontH2) ImGui::PushFont(g_fontH2);
            ImGui::TextUnformatted(gameState.currentContestName.c_str());
            if (g_fontH2) ImGui::PopFont();
            ImGui::SameLine(0, 10);
            OIWidgets::Tag(Contest::isIOI() ? "IOI 赛制" : "OI 赛制", OITheme::Col::Info);
            ImGui::TextColored(OITheme::Col::TxtFaint, Contest::isIOI() ? "时间归零后仍可提交" : "时间归零后可结束比赛");
            ImGui::EndGroup();

            const float rightW = 380.0f;
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - rightW);

            ImGui::BeginGroup();
            ImGui::TextColored(OITheme::Col::TxtFaint, "剩余时间");
            if (g_fontH1) ImGui::PushFont(g_fontH1);
            ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Teal);
            ImGui::Text("%d", gameState.timePoints);
            ImGui::PopStyleColor();
            if (g_fontH1) ImGui::PopFont();
            ImGui::EndGroup();

            ImGui::SameLine(0, 28);
            ImGui::BeginGroup();
            ImGui::TextColored(OITheme::Col::TxtFaint, "心态");
            ImVec4 mc = gameState.mood >= MOOD_LIMIT * 2 / 3 ? OITheme::Col::Ok : (gameState.mood >= MOOD_LIMIT / 3 ? OITheme::Col::Warn : OITheme::Col::Bad);
            if (g_fontH1) ImGui::PushFont(g_fontH1);
            ImGui::PushStyleColor(ImGuiCol_Text, mc);
            ImGui::Text("%d", gameState.mood);
            ImGui::PopStyleColor();
            if (g_fontH1) ImGui::PopFont();
            ImGui::SameLine(0, 4);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(OITheme::Col::TxtFaint, "/ %d", MOOD_LIMIT);
            ImGui::EndGroup();

            ImGui::SameLine(0, 28);
            ImGui::BeginGroup();
            ImGui::Dummy(ImVec2(0, 6));
            ImGui::BeginDisabled(gameState.timePoints != 0);
            if (OIWidgets::GhostButton("结束比赛", OITheme::Col::Warn, ImVec2(120, 36)))
                endContest = true;
            ImGui::EndDisabled();
            ImGui::EndGroup();
        }
        OIWidgets::EndCard();
        if (endContest) { FinalizeContest(); return; }

        // —— 题目切换 ——
        if (gameState.totalProblems > 1)
        {
            for (int i = 1; i <= gameState.totalProblems; ++i)
            {
                if (i > 1) ImGui::SameLine();
                const bool isSelected = (i == gameState.currentProblem);
                bool done = false;
                int pidx = i - 1;
                if (pidx >= 0 && pidx < static_cast<int>(gameState.contestStates.size())) {
                    done = !gameState.contestStates[pidx].empty();
                    for (const auto& cs : gameState.contestStates[pidx]) if (!cs.isCodeComplete) { done = false; break; }
                }
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, OITheme::Col::TealGhost2);
                    ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Teal);
                    ImGui::PushStyleColor(ImGuiCol_Border, OITheme::Col::Teal);
                } else if (done) {
                    ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Ok);
                    ImGui::PushStyleColor(ImGuiCol_Button, OITheme::Col::Bg2);
                    ImGui::PushStyleColor(ImGuiCol_Border, OITheme::Col::Line);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::TxtDim);
                    ImGui::PushStyleColor(ImGuiCol_Button, OITheme::Col::Bg2);
                    ImGui::PushStyleColor(ImGuiCol_Border, OITheme::Col::Line);
                }
                if (ImGui::Button(("T" + std::to_string(i) + (done ? "  ✓" : "")).c_str(), ImVec2(72.0f, 34.0f)))
                    gameState.currentProblem = i;
                ImGui::PopStyleColor(3);
            }
            ImGui::Spacing();
        }

        const int problemIdx = gameState.currentProblem - 1;
        if (problemIdx < 0 || problemIdx >= static_cast<int>(gameState.problems.size()))
        {
            ImGui::TextUnformatted("当前没有可展示的题目。");
            return;
        }

        // —— 题目标题 ——
        if (g_fontH2) ImGui::PushFont(g_fontH2);
        ImGui::Text("T%d  %s", gameState.currentProblem, gameState.problems[problemIdx].name.c_str());
        if (g_fontH2) ImGui::PopFont();
        ImGui::SameLine(0, 10);
        OIWidgets::TierBadge(getLuoguTierName(gameState.problems[problemIdx].level), OITheme::TierColor(gameState.problems[problemIdx].level));
        ImGui::Spacing();

        for (size_t i = 0; i < gameState.subProblems[problemIdx].size(); ++i)
        {
            const auto &sp = gameState.subProblems[problemIdx][i];
            const int thinkTime = Contest::calculateThinkTime(sp);
            const int codeTime = Contest::calculateCodeTime(sp);
            const double thinkRate = Contest::calculateThinkSuccessRate(sp);
            const double codeRate = Contest::calculateCodeSuccessRate(sp);
            const auto& cs = gameState.contestStates[problemIdx][i];
            const bool completed = cs.isCodeComplete;
            const bool canThink = !completed && cs.thinkProgress < thinkTime && gameState.timePoints > 0;
            const bool canCode = !completed && cs.thinkProgress >= thinkTime &&
                                 cs.codeProgress < codeTime && gameState.timePoints > 0;
            const bool mustModifyBeforeRecheck = !completed && !Contest::isIOI() &&
                                                 cs.requiresCodeModification;
            const int requiredFixes = sp.branch + 1;
            const bool canCheck = !completed && cs.codeProgress >= codeTime &&
                                  (Contest::isIOI() || gameState.timePoints > 0) &&
                                  !mustModifyBeforeRecheck;

            ImGui::PushID(static_cast<int>(i));
            if (mustModifyBeforeRecheck)
                ImGui::PushStyleColor(ImGuiCol_Border, OITheme::Col::Warn);
            else if (completed)
                ImGui::PushStyleColor(ImGuiCol_Border, OITheme::Col::Ok);
            else
                ImGui::PushStyleColor(ImGuiCol_Border, OITheme::Col::Line);

            bool cardOpen = OIWidgets::BeginCard("subproblem_card");
            ImGui::PopStyleColor();
            if (cardOpen)
            {
                // 头部：部分分 + 分值 + 状态
                ImVec4 tc = OITheme::TierColor(gameState.problems[problemIdx].level);
                ImGui::PushStyleColor(ImGuiCol_Text, tc);
                if (g_fontH2) ImGui::PushFont(g_fontH2);
                ImGui::Text("部分分 %zu", i + 1);
                if (g_fontH2) ImGui::PopFont();
                ImGui::PopStyleColor();
                ImGui::SameLine(0, 8);
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(OITheme::Col::TxtFaint, "%d 分", sp.score);
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 96);
                if (completed) OIWidgets::Tag("● 已完成", OITheme::Col::Ok);
                else if (mustModifyBeforeRecheck) OIWidgets::Tag("● 待修改", OITheme::Col::Warn);
                else OIWidgets::Tag("○ 进行中", OITheme::Col::TxtDim);

                ImGui::PushStyleColor(ImGuiCol_Separator, OITheme::Col::Line);
                ImGui::Separator();
                ImGui::PopStyleColor();
                ImGui::Spacing();

                float inner = ImGui::GetContentRegionAvail().x;
                float ringsW = 300.0f;
                float leftW = inner - ringsW;
                if (leftW < 220.0f) leftW = inner * 0.58f;

                ImGui::Columns(2, "cbody", false);
                ImGui::SetColumnWidth(0, leftW);

                // 左列：要求 + 指标
                ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::TxtDim);
                ImGui::PushTextWrapPos(ImGui::GetColumnWidth() - 12.0f);
                ImGui::TextWrapped("%s", BuildRequirementText(sp, problemIdx, static_cast<int>(i)).c_str());
                ImGui::PopTextWrapPos();
                ImGui::PopStyleColor();
                ImGui::Spacing();

                const float metaVal = 150.0f;
                ImGui::TextColored(OITheme::Col::TxtDim, "思考成功率"); ImGui::SameLine(metaVal);
                ImGui::TextColored(OITheme::RateColor(thinkRate), "%d%%", static_cast<int>(thinkRate * 100));
                ImGui::TextColored(OITheme::Col::TxtDim, "写代码成功率"); ImGui::SameLine(metaVal);
                ImGui::TextColored(OITheme::RateColor(codeRate), "%d%%", static_cast<int>(codeRate * 100));
                ImGui::TextColored(OITheme::Col::TxtDim, "%s出错概率", Contest::isIOI() ? "提交" : "对拍"); ImGui::SameLine(metaVal);
                if (cs.errorRate >= 0.0)
                    ImGui::TextColored(OITheme::RiskColor(cs.errorRate), "%d%%", static_cast<int>(cs.errorRate * 100));
                else
                    ImGui::TextColored(OITheme::Col::TxtFaint, "?");
                if (!Contest::isIOI()) {
                    ImGui::TextColored(OITheme::Col::TxtDim, "返工分支"); ImGui::SameLine(metaVal);
                    ImGui::TextColored(OITheme::Col::TxtDim, "×%d", requiredFixes);
                }

                // 右列：进度环
                ImGui::NextColumn();
                OIWidgets::ProgressRing("思考", cs.thinkProgress, thinkTime, OITheme::Col::Teal, 30, 7, g_fontBody);
                ImGui::SameLine(0, 14);
                OIWidgets::ProgressRing("代码", cs.codeProgress, codeTime, OITheme::Col::Info, 30, 7, g_fontBody);
                if (!Contest::isIOI()) {
                    ImGui::SameLine(0, 14);
                    OIWidgets::ProgressRing("返工", cs.modificationCount, requiredFixes, OITheme::Col::Warn, 30, 7, g_fontBody);
                }
                ImGui::Columns(1);

                if (mustModifyBeforeRecheck)
                {
                    ImGui::Spacing();
                    ImGui::TextColored(OITheme::Col::Warn, "需要先修改代码，才能再次对拍");
                }

                ImGui::Spacing();
                ImVec2 bsz(108, 32);
                ImGui::BeginDisabled(!canThink);
                if ((canThink ? OIWidgets::PrimaryButton("思考", bsz) : ImGui::Button("思考", bsz)))
                    HandleContestAction(static_cast<int>(i), 'a');
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::BeginDisabled(!canCode);
                if ((canCode ? OIWidgets::PrimaryButton("写代码", bsz) : ImGui::Button("写代码", bsz)))
                    HandleContestAction(static_cast<int>(i), 'b');
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::BeginDisabled(!canCheck);
                const char* checkLabel = Contest::isIOI() ? "提交" : "对拍";
                if ((canCheck ? OIWidgets::PrimaryButton(checkLabel, bsz) : ImGui::Button(checkLabel, bsz)))
                    HandleContestAction(static_cast<int>(i), 'c');
                ImGui::EndDisabled();

                const bool canModify = !completed && cs.codeProgress >= codeTime &&
                                       cs.requiresCodeModification && !Contest::isIOI() &&
                                       gameState.timePoints > 0;
                if (canModify)
                {
                    ImGui::SameLine();
                    if (OIWidgets::GhostButton("修改代码 · 1 时间", OITheme::Col::Warn, ImVec2(150, 32)))
                        ModifyCodeProblem(problemIdx, static_cast<int>(i));
                }
            }
            OIWidgets::EndCard();
            ImGui::PopID();
            ImGui::Spacing();
        }
    }

    void GuiApp::RenderContestResult()
    {
        RenderPageHeader((contestResult_.contestName + " · 比赛结果").c_str());

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
                ImGui::Text("T%zu  %s", i + 1, item.name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(OITheme::Col::TxtDim, "%d", item.expectedScore);
                ImGui::TableSetColumnIndex(2);
                ImVec4 ac = item.actualScore >= item.expectedScore ? OITheme::Col::Ok
                          : (item.actualScore > 0 ? OITheme::Col::Warn : OITheme::Col::TxtFaint);
                ImGui::TextColored(ac, "%d", item.actualScore);
            }
            ImGui::EndTable();
        }

        ImGui::Spacing();

        // —— 汇总指标块 ——
        auto metric = [](const char *label, int value, ImVec4 color, const char *suffix) {
            if (OIWidgets::BeginCard(label, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                ImGui::TextColored(OITheme::Col::TxtFaint, "%s", label);
                if (g_fontH2) ImGui::PushFont(g_fontH2);
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::Text("%d%s", value, suffix);
                ImGui::PopStyleColor();
                if (g_fontH2) ImGui::PopFont();
            }
            OIWidgets::EndCard();
        };

        int cols = 3 + (contestResult_.hasAggregate ? 1 : 0);
        ImGui::Columns(cols, "resmetrics", false);
        metric("预期总分", contestResult_.expectedTotal, OITheme::Col::TxtDim, "");
        ImGui::NextColumn();
        metric("实际总分", contestResult_.actualTotal,
               contestResult_.actualTotal >= contestResult_.expectedTotal ? OITheme::Col::Ok : OITheme::Col::Warn, "");
        ImGui::NextColumn();
        metric("奖金", contestResult_.determinationReward, OITheme::Col::Warn, " 元");
        ImGui::NextColumn();
        if (contestResult_.hasAggregate)
        {
            metric(contestResult_.aggregateLabel.c_str(), contestResult_.aggregateValue, OITheme::Col::Teal, "");
            ImGui::NextColumn();
        }
        ImGui::Columns(1);

        if (contestResult_.hasAward)
        {
            ImGui::Spacing();
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(OITheme::Col::TxtFaint, "获奖情况");
            ImGui::SameLine(0, 8);
            OIWidgets::TierBadge(contestResult_.award.c_str(), OITheme::Col::LgYellow);
        }

        ImGui::Spacing();
        ImGui::Spacing();
        if (OIWidgets::PrimaryButton("继续后续流程  →", ImVec2(200.0f, kPrimaryButtonHeight)))
        {
            if (isActivityContest_) {
                isActivityContest_ = false;
                if (contestResult_.actualTotal > 0) {
                    Talent::tryAcquireTrait(0.15);
                }
                screen_ = GuiScreen::MonthAction;
            } else {
                ContinueAfterContest();
            }
        }
    }

    void GuiApp::RenderGameOver()
    {
        const float ww = ImGui::GetContentRegionAvail().x;
        ImGui::Spacing();
        auto centerText = [ww](const char* t){ ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ww - ImGui::CalcTextSize(t).x) * 0.5f); };

        centerText("— GAME OVER —");
        ImGui::TextColored(OITheme::Col::TxtFaint, "— GAME OVER —");
        ImGui::Spacing();
        if (g_fontH1) ImGui::PushFont(g_fontH1);
        centerText(gameOver_.reason.c_str());
        ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Txt);
        ImGui::TextUnformatted(gameOver_.reason.c_str());
        ImGui::PopStyleColor();
        if (g_fontH1) ImGui::PopFont();
        ImGui::Spacing();
        {
            std::string sum = BuildEndingSummary();
            centerText(sum.c_str());
            ImGui::TextColored(OITheme::Col::TxtDim, "%s", sum.c_str());
        }
        ImGui::Spacing();
        ImGui::Spacing();

        // —— 成就徽章（居中）——
        if (!gameState.playerStats.achievements.empty())
        {
            float total = 0.0f;
            for (const auto& a : gameState.playerStats.achievements)
                total += ImGui::CalcTextSize(a.c_str()).x + 30.0f;
            total -= 8.0f;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, (ww - total) * 0.5f));
            for (size_t i = 0; i < gameState.playerStats.achievements.size(); ++i)
            {
                const auto& a = gameState.playerStats.achievements[i];
                ImVec4 col;
                if (a.find("国家队") != std::string::npos || a.find("IOI") != std::string::npos) col = OITheme::Col::LgRed;
                else if (a.find("集训队") != std::string::npos) col = OITheme::Col::LgYellow;
                else if (a.find("省队") != std::string::npos) col = OITheme::Col::LgBlue;
                else col = OITheme::Col::Teal;
                if (i) ImGui::SameLine(0, 8);
                OIWidgets::TierBadge(a.c_str(), col);
            }
            ImGui::Spacing();
            ImGui::Spacing();
        }

        // —— 能力 + 知识 双卡 ——
        const float gap = 14.0f;
        const float cardW = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
        if (OIWidgets::BeginCard("go_ability", ImVec2(cardW, 0)))
        {
            RenderSection("STATS", "最终能力");
            const auto& ps = gameState.playerStats;
            struct AB { const char* n; int v; ImVec4 c; };
            AB abs[8] = {
                {"思维", ps.thinking, OITheme::Col::Teal}, {"代码", ps.coding, OITheme::Col::Teal},
                {"细心", ps.carefulness, OITheme::Col::TxtDim}, {"迅捷", ps.quickness, OITheme::Col::TxtDim},
                {"心理", ps.mental, OITheme::Col::TxtDim}, {"运气", ps.luck, OITheme::Col::TxtDim},
                {"经验", ps.experience, OITheme::Col::TxtDim}, {"文化课", ps.culture, OITheme::Col::Info},
            };
            ImGui::Columns(2, "goab", false);
            for (int i = 0; i < 8; ++i) {
                OIWidgets::StatBar(abs[i].n, abs[i].v, 20, abs[i].c, 56, 90, g_fontBody);
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }
        OIWidgets::EndCard();
        ImGui::SameLine(0, gap);
        if (OIWidgets::BeginCard("go_know", ImVec2(cardW, 0)))
        {
            RenderSection("KNOWLEDGE", "知识维度");
            const auto& ps = gameState.playerStats;
            struct KN { const char* n; int v; };
            KN kns[9] = {
                {"DP", ps.dp}, {"数据结构", ps.ds}, {"字符串", ps.string},
                {"图论", ps.graph}, {"组合计数", ps.combinatorics}, {"数学", ps.math},
                {"几何", ps.geometry}, {"高级DS", ps.data_structure}, {"构造", ps.adhoc},
            };
            ImGui::Columns(2, "gokn", false);
            for (int i = 0; i < 9; ++i) {
                OIWidgets::StatBar(kns[i].n, kns[i].v, 20, OITheme::Col::Teal, 64, 80, g_fontBody);
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }
        OIWidgets::EndCard();

        // —— 特质 / 考试记录 ——
        if (!gameState.traits.empty() || !gameState.examRecords.empty())
        {
            ImGui::Spacing();
            if (OIWidgets::BeginCard("go_misc"))
            {
                if (!gameState.traits.empty()) {
                    RenderSection("TRAITS", "获得的特质");
                    for (const auto& tid : gameState.traits) {
                        for (const auto& t : Talent::ACQUIRABLE_TRAITS) {
                            if (t.id == tid) {
                                ImGui::TextColored(OITheme::Col::Teal, "◆");
                                ImGui::SameLine(0, 6);
                                ImGui::Text("%s", t.name);
                                ImGui::SameLine(0, 8);
                                ImGui::TextColored(OITheme::Col::TxtDim, "%s", t.desc);
                                break;
                            }
                        }
                    }
                }
                if (!gameState.examRecords.empty()) {
                    if (!gameState.traits.empty()) ImGui::Spacing();
                    RenderSection("EXAMS", "考试记录");
                    for (const auto& rec : gameState.examRecords) {
                        const char* type = rec.isGaokao ? "高考" : "考试";
                        double pct = rec.maxScore > 0 ? (double)rec.score / rec.maxScore * 100 : 0;
                        ImGui::TextColored(OITheme::Col::TxtFaint, "M%d", rec.month);
                        ImGui::SameLine(0, 8);
                        ImGui::Text("%s", type);
                        ImGui::SameLine(0, 8);
                        ImGui::TextColored(pct >= 60 ? OITheme::Col::Ok : OITheme::Col::Warn, "%d / %d  (%.0f%%)", rec.score, rec.maxScore, pct);
                    }
                }
            }
            OIWidgets::EndCard();
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ww - kPrimaryButtonWidth) * 0.5f);
        if (OIWidgets::PrimaryButton("重新开始", ImVec2(kPrimaryButtonWidth, kPrimaryButtonHeight)))
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
        RenderHelpCard();
        ImGui::Spacing();
        RenderLogsCard();
    }

    namespace {
        void SbHeader(const char* en, const char* cjk) {
            OIWidgets::SectionHeader(en, cjk, g_fontSmall);
        }
        // 网格小指标：名称 + 数值 + 细条
        void SbMiniStat(const char* n, int v, int mx, ImVec4 c) {
            ImGui::TextColored(OITheme::Col::TxtDim, "%s", n);
            ImGui::SameLine(0, 6);
            ImGui::PushStyleColor(ImGuiCol_Text, c);
            if (g_fontBody) ImGui::PushFont(g_fontBody);
            ImGui::Text("%d", v);
            if (g_fontBody) ImGui::PopFont();
            ImGui::PopStyleColor();
            OIWidgets::Bar(mx > 0 ? (float)v / mx : 0.0f, ImVec2(ImGui::GetContentRegionAvail().x, 5.0f), c);
            ImGui::Spacing();
        }
    }

    void GuiApp::RenderHelpCard()
    {
        if (OIWidgets::BeginCard("sb_help"))
        {
            SbHeader("HINT", "帮助速查");
            if (screen_ == GuiScreen::MonthAction)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::TxtDim);
                ImGui::TextWrapped("每月用 AP 执行活动：学习、网赛、刷题、学文化课、休息。");
                ImGui::TextWrapped("停课获得更多 AP，但心态下降且不能学文化课。");
                ImGui::PopStyleColor();
            }
            else if (screen_ == GuiScreen::Contest)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::TxtDim);
                ImGui::TextWrapped("比赛按 思考 → 写代码 → 对拍/提交 推进。");
                ImGui::TextWrapped("非 IOI 对拍失败后，必须先修改代码才能再次对拍。");
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::TxtDim);
                ImGui::TextWrapped("这里会根据当前页面显示简短帮助，完整规则见帮助页。");
                ImGui::PopStyleColor();
            }
            ImGui::Spacing();
            if (ImGui::Button("打开完整帮助", ImVec2(-1, 32)))
                OpenHelp();
        }
        OIWidgets::EndCard();
    }

    void GuiApp::RenderPlayerCard()
    {
        if (OIWidgets::BeginCard("sb_player"))
        {
            SbHeader("STATUS", "角色状态");

            // 头像 + 年级 + 背景
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            const float av = 38.0f;
            dl->AddRectFilled(p, ImVec2(p.x + av, p.y + av), ImGui::GetColorU32(OITheme::Col::Bg2), 8.0f);
            dl->AddRect(p, ImVec2(p.x + av, p.y + av), ImGui::GetColorU32(OITheme::Col::TealDim), 8.0f);
            char chip[8]; snprintf(chip, sizeof chip, "G%d", gameState.currentYear);
            ImVec2 cs = ImGui::CalcTextSize(chip);
            dl->AddText(ImVec2(p.x + (av - cs.x) * 0.5f, p.y + (av - cs.y) * 0.5f), ImGui::GetColorU32(OITheme::Col::Teal), chip);
            ImGui::Dummy(ImVec2(av, av));
            ImGui::SameLine(0, 10);
            ImGui::BeginGroup();
            const char* grade = gameState.currentYear == 1 ? "高一" : (gameState.currentYear == 2 ? "高二" : "高三");
            ImGui::Text("%s · %s", grade, getCalendarMonthName(gameState.calendarMonth));
            ImGui::TextColored(OITheme::Col::TxtFaint, "第 %d / 36 月", gameState.currentMonth);
            ImGui::EndGroup();
            if (auto* bg = Talent::getBackground()) {
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(bg->name).x - 16.0f);
                OIWidgets::Tag(bg->name, OITheme::Col::Teal);
            }
            ImGui::Spacing();

            // HP / 心态 / 金钱
            ImGui::Columns(3, "sb_vitals", false);
            SbMiniStat("健康", gameState.health, 20, OITheme::Col::Ok); ImGui::NextColumn();
            SbMiniStat("心态", gameState.mood, gameState.moodCap, gameState.mood >= gameState.moodCap * 2 / 3 ? OITheme::Col::Ok : (gameState.mood >= gameState.moodCap / 3 ? OITheme::Col::Warn : OITheme::Col::Bad)); ImGui::NextColumn();
            ImGui::TextColored(OITheme::Col::TxtDim, "金钱");
            if (g_fontBody) ImGui::PushFont(g_fontBody);
            ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::Warn);
            ImGui::Text("%d", gameState.money);
            ImGui::PopStyleColor();
            if (g_fontBody) ImGui::PopFont();
            ImGui::TextColored(OITheme::Col::TxtFaint, "元");
            ImGui::Columns(1);

            if (gameState.isAnxious) { ImGui::Spacing(); OIWidgets::Tag("焦虑中 · 效率降低", OITheme::Col::Bad); }
            ImGui::Spacing();

            // 能力值
            SbHeader("ABILITY", "能力值");
            const auto& ps = gameState.playerStats;
            ImGui::Columns(2, "sb_ab", false);
            SbMiniStat("思维", ps.thinking, 20, OITheme::Col::Teal); ImGui::NextColumn();
            SbMiniStat("代码", ps.coding, 20, OITheme::Col::Teal); ImGui::NextColumn();
            SbMiniStat("细心", ps.carefulness, 20, OITheme::Col::TxtDim); ImGui::NextColumn();
            SbMiniStat("迅捷", ps.quickness, 20, OITheme::Col::TxtDim); ImGui::NextColumn();
            SbMiniStat("心理", ps.mental, 20, OITheme::Col::TxtDim); ImGui::NextColumn();
            SbMiniStat("运气", ps.luck, 20, OITheme::Col::TxtDim); ImGui::NextColumn();
            SbMiniStat("经验", ps.experience, 20, OITheme::Col::TxtDim); ImGui::NextColumn();
            SbMiniStat("文化课", ps.culture, 20, OITheme::Col::Info); ImGui::NextColumn();
            ImGui::Columns(1);
            ImGui::TextColored(OITheme::Col::TxtFaint, "临场积累");
            ImGui::SameLine(0, 6);
            ImGui::TextColored(OITheme::Col::TxtDim, "%d / 6", ps.tempExperience);
            OIWidgets::Bar(ps.tempExperience / 6.0f, ImVec2(ImGui::GetContentRegionAvail().x, 5.0f), OITheme::Col::Teal);
            ImGui::Spacing();

            // 知识维度
            SbHeader("KNOWLEDGE", "知识维度");
            ImGui::Columns(3, "sb_kn", false);
            SbMiniStat("DP", ps.dp, 20, OITheme::Col::Teal); ImGui::NextColumn();
            SbMiniStat("数据结构", ps.ds, 20, OITheme::Col::Teal); ImGui::NextColumn();
            SbMiniStat("字符串", ps.string, 20, OITheme::Col::Teal); ImGui::NextColumn();
            SbMiniStat("图论", ps.graph, 20, OITheme::Col::Teal); ImGui::NextColumn();
            SbMiniStat("组合", ps.combinatorics, 20, OITheme::Col::Teal); ImGui::NextColumn();
            SbMiniStat("数学", ps.math, 20, OITheme::Col::Teal); ImGui::NextColumn();
            SbMiniStat("几何", ps.geometry, 20, OITheme::Col::Teal); ImGui::NextColumn();
            SbMiniStat("高级DS", ps.data_structure, 20, OITheme::Col::Teal); ImGui::NextColumn();
            SbMiniStat("构造", ps.adhoc, 20, OITheme::Col::Teal); ImGui::NextColumn();
            ImGui::Columns(1);

            if (!gameState.traits.empty()) {
                ImGui::Spacing();
                SbHeader("TRAITS", "特质");
                for (const auto& tid : gameState.traits) {
                    for (const auto& t : Talent::ACQUIRABLE_TRAITS) {
                        if (t.id == tid) { OIWidgets::Tag(t.name, OITheme::Col::Teal); ImGui::SameLine(); break; }
                    }
                }
                ImGui::NewLine();
            }
        }
        OIWidgets::EndCard();
    }

    void GuiApp::RenderFlagsCard()
    {
        if (OIWidgets::BeginCard("sb_flags"))
        {
            SbHeader("SCORES", "比赛成绩");
            const auto& ps = gameState.playerStats;
            struct Sc { const char* n; int v; };
            Sc scs[5] = { {"CSP-S", ps.cspScore}, {"NOIP", ps.noipScore}, {"省选", ps.tempScore}, {"CTT", ps.cttScore}, {"NOI", ps.noiScore} };
            for (int i = 0; i < 5; ++i) {
                ImGui::TextColored(OITheme::Col::TxtDim, "%s", scs[i].n);
                ImGui::SameLine(72);
                OIWidgets::Bar(scs[i].v / 700.0f, ImVec2(ImGui::GetContentRegionAvail().x - 44.0f, 6.0f), OITheme::Col::Teal);
                ImGui::SameLine();
                if (g_fontBody) ImGui::PushFont(g_fontBody);
                ImGui::Text("%d", scs[i].v);
                if (g_fontBody) ImGui::PopFont();
            }
            ImGui::Spacing();
            SbHeader("FLAGS", "关键状态");
            auto flag = [](const char* n, bool on) {
                ImGui::TextColored(on ? OITheme::Col::Ok : OITheme::Col::TxtFaint, on ? "●" : "○");
                ImGui::SameLine(0, 6);
                ImGui::TextColored(on ? OITheme::Col::Txt : OITheme::Col::TxtFaint, "%s", n);
            };
            ImGui::Columns(2, "sb_fl", false);
            flag("省队A", ps.isProvincialTeamA); ImGui::NextColumn();
            flag("省队", ps.isProvincialTeam); ImGui::NextColumn();
            flag("集训队", ps.isTrainingTeam); ImGui::NextColumn();
            flag("候选队", ps.isCandidateTeam); ImGui::NextColumn();
            flag("国家队", ps.isNationalTeam); ImGui::NextColumn();
            flag("IOI金牌", ps.isIOIgold); ImGui::NextColumn();
            ImGui::Columns(1);
            if (gameState.isTingke) { ImGui::Spacing(); OIWidgets::Tag("已停课", OITheme::Col::Warn); }
        }
        OIWidgets::EndCard();
    }

    void GuiApp::RenderLogsCard()
    {
        if (OIWidgets::BeginCard("sb_logs_outer"))
        {
            SbHeader("LOG", "事件日志");
            ImGui::PushStyleColor(ImGuiCol_ChildBg, OITheme::Col::Bg1b);
            ImGui::BeginChild("log_panel", ImVec2(0.0f, 200.0f), false);
            for (const auto &entry : gameState.gameLog)
            {
                ImGui::TextColored(OITheme::Col::Teal, "▍");
                ImGui::SameLine(0, 6);
                ImGui::PushStyleColor(ImGuiCol_Text, OITheme::Col::TxtDim);
                ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
                ImGui::TextWrapped("%s", entry.c_str());
                ImGui::PopTextWrapPos();
                ImGui::PopStyleColor();
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f)
                ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
        OIWidgets::EndCard();
    }

    GuiApp &App()
    {
        static GuiApp app;
        return app;
    }

    ImFont *LoadChineseFont(ImGuiIO &io, std::string &loadedPath)
    {
        const std::array<const char *, 8> candidates = {
            "fonts/sarasa-mono-sc-regular.ttf", // 随 exe 打包（推荐，等宽 CJK）
            "fonts/SarasaMonoSC-Regular.ttf",
            "C:/Windows/Fonts/msyh.ttc",
            "C:/Windows/Fonts/msyh.ttf",
            "C:/Windows/Fonts/msyhbd.ttc",
            "C:/Windows/Fonts/simhei.ttf",
            "C:/Windows/Fonts/simsun.ttc",
            "C:/Windows/Fonts/Deng.ttf"};

        ImFontConfig config;
        config.OversampleH = 2;
        config.OversampleV = 2;
        const ImWchar *ranges = io.Fonts->GetGlyphRangesChineseFull();

        for (const char *path : candidates)
        {
            if (!std::filesystem::exists(path))
                continue;
            // 同一 TTF 加载 4 个字号，构建标题/正文/标签层次
            g_fontBody  = io.Fonts->AddFontFromFileTTF(path, 18.0f, &config, ranges);
            if (!g_fontBody)
                continue;
            g_fontH1    = io.Fonts->AddFontFromFileTTF(path, 30.0f, &config, ranges);
            g_fontH2    = io.Fonts->AddFontFromFileTTF(path, 21.0f, &config, ranges);
            g_fontSmall = io.Fonts->AddFontFromFileTTF(path, 13.0f, &config, ranges);
            loadedPath = path;
            return g_fontBody;
        }

        g_fontBody = io.Fonts->AddFontDefault();
        g_fontH1 = g_fontH2 = g_fontSmall = g_fontBody;
        loadedPath.clear();
        return nullptr;
    }

    void ApplyGuiStyle()
    {
        // 视觉主题统一交由 OI Terminal 重设计主题接管
        OITheme::Apply();
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

    ImVec4 clearColor = ImVec4(0.10f, 0.11f, 0.16f, 1.0f);
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
