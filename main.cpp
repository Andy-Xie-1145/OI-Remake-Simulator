// OI 重开模拟器 GUI 版本
// 使用 Dear ImGui + Win32 + DirectX11

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <random>
#include <map>

// 简化版类型定义（避免依赖复杂头文件）
inline const int MOOD_LIMIT = 12;

struct PlayerStats {
    int dp = 0, ds = 0, string = 0, graph = 0, combinatorics = 0;
    int thinking = 0, coding = 0, carefulness = 0, quickness = 0, mental = 0, culture = 0;
    int determination = 500;
    int mood = 6;
};

struct ContestInfo {
    std::string name;
    int problemCount;
    int timeLimit;
};

inline const ContestInfo contests[] = {
    {"CSP-S", 4, 240},
    {"NOIP", 4, 270},
    {"WC", 3, 300},
    {"省选Day1", 3, 270},
    {"省选Day2", 3, 270},
    {"APIO", 3, 300},
    {"NOI Day1", 3, 300},
    {"NOI Day2", 3, 300},
    {"CTT Day1", 3, 300},
    {"CTT Day2", 3, 300},
    {"CTT Day3", 3, 300},
    {"CTS Day1", 3, 300},
    {"CTS Day2", 3, 300},
    {"CTS Day3", 3, 300},
    {"CTS Day4", 3, 300},
    {"IOI", 3, 300}
};

// DirectX11 全局变量
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// 游戏状态
enum class GamePhase {
    MainMenu,
    DifficultySelect,
    TalentAllocation,
    Training,
    Contest,
    Event,
    Shop,
    GameOver,
    Victory
};

struct GameState {
    GamePhase phase = GamePhase::MainMenu;
    PlayerStats player;
    int currentContestIndex = 0;
    int selectedProblem = 0;
    int difficulty = 1;
    int talentPoints = 0;
    std::string currentEvent;
    std::string eventDescription;
    std::vector<std::string> eventOptions;
    std::map<std::string, int> purchasedItems;
    std::mt19937 rng{std::random_device{}()};
    
    // 比赛状态
    std::vector<int> problemScores;
    std::vector<bool> problemSubmitted;
    int contestTimeLeft = 0;
    int currentAction = 0; // 0=思考, 1=写代码, 2=对拍
    
    // 训练事件选择
    std::vector<std::string> trainingEventOptions;
    int selectedTrainingOption = 0;
    
    void reset() {
        *this = GameState{};
        rng.seed(std::random_device{}());
    }
};

static GameState game;

// 前向声明
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// GUI 渲染函数
void RenderMainMenu();
void RenderDifficultySelect();
void RenderTalentAllocation();
void RenderTraining();
void RenderContest();
void RenderEvent();
void RenderShop();
void RenderGameOver();
void RenderVictory();

// 游戏逻辑函数
void StartNewGame();
void ProcessTraining();
void ProcessEvent();
void ProcessShop(int itemIndex);
void StartContest();
void ProcessContestTurn();
void EndContest();
int CalculateContestScore();

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        L"OI Simulator GUI", nullptr
    };
    RegisterClassExW(&wc);
    
    HWND hwnd = CreateWindowW(
        wc.lpszClassName, L"OI 重开模拟器 v0.1.0-gui-beta",
        WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720,
        nullptr, nullptr, wc.hInstance, nullptr
    );
    
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    
    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.15f, 1.00f);
    
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // 渲染游戏界面
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::Begin("OI 重开模拟器", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
            
            switch (game.phase) {
                case GamePhase::MainMenu:
                    RenderMainMenu();
                    break;
                case GamePhase::DifficultySelect:
                    RenderDifficultySelect();
                    break;
                case GamePhase::TalentAllocation:
                    RenderTalentAllocation();
                    break;
                case GamePhase::Training:
                    RenderTraining();
                    break;
                case GamePhase::Contest:
                    RenderContest();
                    break;
                case GamePhase::Event:
                    RenderEvent();
                    break;
                case GamePhase::Shop:
                    RenderShop();
                    break;
                case GamePhase::GameOver:
                    RenderGameOver();
                    break;
                case GamePhase::Victory:
                    RenderVictory();
                    break;
            }
            
            ImGui::End();
        }
        
        ImGui::Render();
        const float clear_color_with_alpha[4] = {
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w
        };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
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

// GUI 渲染函数实现
void RenderMainMenu() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetCursorPosY(center.y - 100);
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 15));
    
    if (ImGui::Button("🎮 开始游戏", ImVec2(200, 50))) {
        game.phase = GamePhase::DifficultySelect;
    }
    
    ImGui::SetCursorPosY(center.y - 30);
    if (ImGui::Button("📖 游戏说明", ImVec2(200, 50))) {
        // 显示游戏说明
    }
    
    ImGui::SetCursorPosY(center.y + 40);
    if (ImGui::Button("🚪 退出游戏", ImVec2(200, 50))) {
        PostQuitMessage(0);
    }
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 50);
    ImGui::Text("OI 重开模拟器 v0.1.0-gui-beta");
    ImGui::SameLine(ImGui::GetWindowWidth() - 200);
    ImGui::Text("AI 辅助生成");
}

void RenderDifficultySelect() {
    ImGui::Text("选择难度");
    ImGui::Separator();
    ImGui::Spacing();
    
    const char* difficulties[] = {"简单", "普通", "困难", "专家"};
    const char* descriptions[] = {
        "适合新手，能力值上限提高",
        "标准难度，平衡体验",
        "有一定挑战性",
        "硬核难度，只有强者才能通关"
    };
    
    for (int i = 0; i < 4; i++) {
        if (ImGui::Button(difficulties[i], ImVec2(200, 40))) {
            game.difficulty = i + 1;
            StartNewGame();
            game.phase = GamePhase::TalentAllocation;
        }
        ImGui::TextWrapped("%s", descriptions[i]);
        ImGui::Spacing();
    }
    
    if (ImGui::Button("返回主菜单", ImVec2(120, 30))) {
        game.phase = GamePhase::MainMenu;
    }
}

void RenderTalentAllocation() {
    ImGui::Text("天赋点分配");
    ImGui::Text("剩余天赋点: %d", game.talentPoints);
    ImGui::Separator();
    ImGui::Spacing();
    
    struct { const char* name; int* value; const char* desc; } talents[] = {
        {"动态规划", &game.player.dp, "影响DP题目的能力"},
        {"数据结构", &game.player.ds, "影响DS题目的能力"},
        {"字符串", &game.player.string, "影响字符串题目的能力"},
        {"图论", &game.player.graph, "影响图论题目的能力"},
        {"组合计数", &game.player.combinatorics, "影响组合计数题目的能力"}
    };
    
    for (auto& t : talents) {
        ImGui::Text("%s (%d)", t.name, *t.value);
        ImGui::SameLine(200);
        
        ImGui::PushID(t.name);
        if (ImGui::Button("-", ImVec2(30, 0)) && *t.value > 0) {
            (*t.value)--;
            game.talentPoints++;
        }
        ImGui::SameLine();
        if (ImGui::Button("+", ImVec2(30, 0)) && game.talentPoints > 0) {
            (*t.value)++;
            game.talentPoints--;
        }
        ImGui::PopID();
        
        ImGui::SameLine();
        ImGui::TextDisabled("%s", t.desc);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    if (game.talentPoints == 0) {
        if (ImGui::Button("开始游戏!", ImVec2(200, 40))) {
            game.phase = GamePhase::Training;
            ProcessTraining();
        }
    } else {
        ImGui::TextDisabled("请分配完所有天赋点");
    }
}

void RenderTraining() {
    ImGui::Text("当前状态");
    ImGui::Separator();
    
    // 显示属性
    ImGui::Columns(2, "stats", false);
    ImGui::Text("动态规划: %d", game.player.dp);
    ImGui::NextColumn();
    ImGui::Text("数据结构: %d", game.player.ds);
    ImGui::NextColumn();
    ImGui::Text("字符串: %d", game.player.string);
    ImGui::NextColumn();
    ImGui::Text("图论: %d", game.player.graph);
    ImGui::NextColumn();
    ImGui::Text("组合计数: %d", game.player.combinatorics);
    ImGui::NextColumn();
    ImGui::Text("文化课: %d", game.player.culture);
    ImGui::Columns(1);
    
    ImGui::Spacing();
    ImGui::Text("心态: %d/%d", game.player.determination, MOOD_LIMIT);
    ImGui::Text("决心: %d", game.player.determination);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // 显示事件
    ImGui::TextWrapped("%s", game.currentEvent.c_str());
    ImGui::TextWrapped("%s", game.eventDescription.c_str());
    
    ImGui::Spacing();
    ImGui::Text("请选择:");
    for (size_t i = 0; i < game.trainingEventOptions.size(); i++) {
        if (ImGui::Button(game.trainingEventOptions[i].c_str(), ImVec2(400, 30))) {
            game.selectedTrainingOption = i;
            ProcessEvent();
        }
    }
}

void RenderContest() {
    ImGui::Text("%s", contests[game.currentContestIndex].name.c_str());
    ImGui::Text("剩余时间: %d 分钟", game.contestTimeLeft);
    ImGui::Separator();
    
    // 显示题目列表
    ImGui::Text("题目列表:");
    for (int i = 0; i < contests[game.currentContestIndex].problemCount; i++) {
        bool selected = (i == game.selectedProblem);
        char probName[16];
        sprintf(probName, "题目 %c", 'A' + i);
        if (ImGui::Selectable(probName, selected)) {
            game.selectedProblem = i;
        }
        
        // 显示得分
        if (i < (int)game.problemScores.size()) {
            ImGui::SameLine();
            ImGui::Text("[%d分]", game.problemScores[i]);
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // 显示当前题目信息
    char probName[16];
    sprintf(probName, "题目 %c", 'A' + game.selectedProblem);
    ImGui::Text("题目: %s", probName);
    ImGui::Text("难度: %d", (game.currentContestIndex + 1) * 10 + game.selectedProblem * 5);
    
    ImGui::Spacing();
    
    // 操作按钮
    if (ImGui::Button("思考 (消耗时间)", ImVec2(150, 30))) {
        game.currentAction = 0;
        ProcessContestTurn();
    }
    ImGui::SameLine();
    if (ImGui::Button("写代码", ImVec2(100, 30))) {
        game.currentAction = 1;
        ProcessContestTurn();
    }
    ImGui::SameLine();
    if (ImGui::Button("对拍", ImVec2(100, 30))) {
        game.currentAction = 2;
        ProcessContestTurn();
    }
    
    ImGui::Spacing();
    if (ImGui::Button("提前离场", ImVec2(120, 30))) {
        EndContest();
    }
}

void RenderEvent() {
    ImGui::TextWrapped("%s", game.currentEvent.c_str());
    ImGui::Spacing();
    ImGui::TextWrapped("%s", game.eventDescription.c_str());
    ImGui::Spacing();
    ImGui::Separator();
    
    for (size_t i = 0; i < game.eventOptions.size(); i++) {
        if (ImGui::Button(game.eventOptions[i].c_str(), ImVec2(400, 30))) {
            ProcessEvent();
        }
    }
}

void RenderShop() {
    ImGui::Text("决心商店");
    ImGui::Text("你的决心: %d", game.player.determination);
    ImGui::Separator();
    
    struct { const char* name; int cost; const char* effect; } items[] = {
        {"思维提升", 300, "思维+2"},
        {"代码提升", 300, "代码+2"},
        {"细心提升", 300, "细心+2"},
        {"随机提升", 300, "随机算法+1"},
        {"心态恢复", 500, "心态+2"},
        {"全面提升", 1000, "所有算法+1"},
        {"速度提升", 1500, "迅捷+1"},
        {"心理素质提升", 1500, "心理素质+1"}
    };
    
    for (int i = 0; i < 8; i++) {
        if (ImGui::Button(items[i].name, ImVec2(200, 30))) {
            ProcessShop(i);
        }
        ImGui::SameLine();
        ImGui::Text("花费: %d | %s", items[i].cost, items[i].effect);
    }
    
    ImGui::Spacing();
    if (ImGui::Button("离开商店", ImVec2(120, 30))) {
        game.phase = GamePhase::Training;
        ProcessTraining();
    }
}

void RenderGameOver() {
    ImGui::Text("游戏结束");
    ImGui::Separator();
    ImGui::TextWrapped("你的 OI 生涯已经结束...");
    
    ImGui::Spacing();
    if (ImGui::Button("重新开始", ImVec2(150, 40))) {
        game.reset();
        game.phase = GamePhase::MainMenu;
    }
}

void RenderVictory() {
    ImGui::Text("🎉 恭喜通关!");
    ImGui::Separator();
    ImGui::Text("你已经完成了整个 OI 赛季!");
    
    ImGui::Spacing();
    if (ImGui::Button("再来一次", ImVec2(150, 40))) {
        game.reset();
        game.phase = GamePhase::MainMenu;
    }
}

// 游戏逻辑函数实现
void StartNewGame() {
    game.player = PlayerStats{};
    game.talentPoints = 5 + game.difficulty;
    game.currentContestIndex = 0;
}

void ProcessTraining() {
    // 简化版训练逻辑
    std::vector<std::string> events = {"偷学", "休息", "打隔膜", "摸鱼", "出游", 
                                         "长期训练", "提升训练", "比赛训练"};
    
    game.trainingEventOptions.clear();
    for (int i = 0; i < 3; i++) {
        int idx = game.rng() % events.size();
        game.trainingEventOptions.push_back(events[idx]);
    }
    
    game.currentEvent = "训练时间";
    game.eventDescription = "选择你要做的事情...";
}

void ProcessEvent() {
    // 处理事件结果
    int effect = game.rng() % 3 - 1; // -1, 0, 1
    game.player.determination += effect * 100;
    game.player.determination = std::max(0, std::min(MOOD_LIMIT, game.player.determination));
    
    // 进入下一阶段
    if (game.rng() % 10 == 0) {
        game.phase = GamePhase::Contest;
        StartContest();
    } else {
        ProcessTraining();
    }
}

void ProcessShop(int itemIndex) {
    // 简化版商店逻辑
    int costs[] = {300, 300, 300, 300, 500, 1000, 1500, 1500};
    
    if (game.player.determination >= costs[itemIndex]) {
        game.player.determination -= costs[itemIndex];
        
        switch (itemIndex) {
            case 0: game.player.thinking += 2; break;
            case 1: game.player.coding += 2; break;
            case 2: game.player.carefulness += 2; break;
            case 4: game.player.determination = std::min(MOOD_LIMIT, game.player.determination + 2); break;
            case 5:
                game.player.dp++; game.player.ds++; game.player.string++;
                game.player.graph++; game.player.combinatorics++;
                break;
            case 6: game.player.quickness++; break;
            case 7: game.player.mental++; break;
        }
    }
}

void StartContest() {
    game.contestTimeLeft = contests[game.currentContestIndex].timeLimit;
    game.problemScores.clear();
    game.problemSubmitted.clear();
    
    int probCount = contests[game.currentContestIndex].problemCount;
    for (int i = 0; i < probCount; i++) {
        game.problemScores.push_back(0);
        game.problemSubmitted.push_back(false);
    }
}

void ProcessContestTurn() {
    game.contestTimeLeft -= 15;
    
    if (game.contestTimeLeft <= 0) {
        EndContest();
        return;
    }
    
    // 简化版比赛逻辑
    int score = game.rng() % 100;
    game.problemScores[game.selectedProblem] = std::max(game.problemScores[game.selectedProblem], score);
}

void EndContest() {
    int totalScore = CalculateContestScore();
    
    game.currentContestIndex++;
    
    if (game.currentContestIndex >= 16) {
        game.phase = GamePhase::Victory;
    } else {
        game.phase = GamePhase::Training;
        ProcessTraining();
    }
}

int CalculateContestScore() {
    int total = 0;
    for (int s : game.problemScores) {
        total += s;
    }
    return total;
}

// DirectX11 初始化
bool CreateDeviceD3D(HWND hWnd) {
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
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 
        createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, 
        &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK) {
        return false;
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    if (msg == WM_SIZE) {
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), 
                DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    }
    if (msg == WM_SYSCOMMAND) {
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
    }
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
