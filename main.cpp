// OI Simulator GUI Version
// Using Dear ImGui + Win32 + DirectX11

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <random>
#include <map>

// Simplified type definitions
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
    {"Provincial Day1", 3, 270},
    {"Provincial Day2", 3, 270},
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

// DirectX11 globals
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Game state
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
    
    std::vector<int> problemScores;
    std::vector<bool> problemSubmitted;
    int contestTimeLeft = 0;
    int currentAction = 0;
    
    std::vector<std::string> trainingEventOptions;
    int selectedTrainingOption = 0;
    
    void reset() {
        *this = GameState{};
        rng.seed(std::random_device{}());
    }
};

static GameState game;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void RenderMainMenu();
void RenderDifficultySelect();
void RenderTalentAllocation();
void RenderTraining();
void RenderContest();
void RenderEvent();
void RenderShop();
void RenderGameOver();
void RenderVictory();

void StartNewGame();
void ProcessTraining();
void ProcessEvent();
void ProcessShop(int itemIndex);
void StartContest();
void ProcessContestTurn();
void EndContest();
int CalculateContestScore();

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        L"OI Simulator GUI", nullptr
    };
    RegisterClassExW(&wc);
    
    HWND hwnd = CreateWindowW(
        wc.lpszClassName, L"OI Simulator v0.1.0-gui-beta",
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
    
    // Load Chinese font (Microsoft YaHei)
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    if (font) {
        io.FontDefault = font;
    }
    
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
        
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::Begin("OI Simulator", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
            
            switch (game.phase) {
                case GamePhase::MainMenu: RenderMainMenu(); break;
                case GamePhase::DifficultySelect: RenderDifficultySelect(); break;
                case GamePhase::TalentAllocation: RenderTalentAllocation(); break;
                case GamePhase::Training: RenderTraining(); break;
                case GamePhase::Contest: RenderContest(); break;
                case GamePhase::Event: RenderEvent(); break;
                case GamePhase::Shop: RenderShop(); break;
                case GamePhase::GameOver: RenderGameOver(); break;
                case GamePhase::Victory: RenderVictory(); break;
            }
            
            ImGui::End();
        }
        
        ImGui::Render();
        const float clear_color_with_alpha[4] = {
            clear_color.x * clear_color.w, clear_color.y * clear_color.w,
            clear_color.z * clear_color.w, clear_color.w
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

void RenderMainMenu() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetCursorPosY(center.y - 100);
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 15));
    
    if (ImGui::Button("Start Game", ImVec2(200, 50))) {
        game.phase = GamePhase::DifficultySelect;
    }
    
    ImGui::SetCursorPosY(center.y - 30);
    if (ImGui::Button("How to Play", ImVec2(200, 50))) {}
    
    ImGui::SetCursorPosY(center.y + 40);
    if (ImGui::Button("Exit", ImVec2(200, 50))) {
        PostQuitMessage(0);
    }
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 50);
    ImGui::Text("OI Simulator v0.1.0-gui-beta");
    ImGui::SameLine(ImGui::GetWindowWidth() - 150);
    ImGui::Text("AI assisted");
}

void RenderDifficultySelect() {
    ImGui::Text("Select Difficulty");
    ImGui::Separator();
    ImGui::Spacing();
    
    const char* difficulties[] = {"Easy", "Normal", "Hard", "Expert"};
    const char* descriptions[] = {
        "For beginners, higher stat caps",
        "Standard difficulty, balanced experience",
        "Challenging gameplay",
        "Hardcore mode, only for the best"
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
    
    if (ImGui::Button("Back", ImVec2(120, 30))) {
        game.phase = GamePhase::MainMenu;
    }
}

void RenderTalentAllocation() {
    ImGui::Text("Talent Points Allocation");
    ImGui::Text("Remaining Points: %d", game.talentPoints);
    ImGui::Separator();
    ImGui::Spacing();
    
    struct { const char* name; int* value; const char* desc; } talents[] = {
        {"DP", &game.player.dp, "Dynamic Programming"},
        {"DS", &game.player.ds, "Data Structures"},
        {"String", &game.player.string, "String algorithms"},
        {"Graph", &game.player.graph, "Graph theory"},
        {"Combinatorics", &game.player.combinatorics, "Counting problems"}
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
        if (ImGui::Button("Start Game!", ImVec2(200, 40))) {
            game.phase = GamePhase::Training;
            ProcessTraining();
        }
    } else {
        ImGui::TextDisabled("Please allocate all talent points");
    }
}

void RenderTraining() {
    ImGui::Text("Current Status");
    ImGui::Separator();
    
    ImGui::Columns(2, "stats", false);
    ImGui::Text("DP: %d", game.player.dp);
    ImGui::NextColumn();
    ImGui::Text("DS: %d", game.player.ds);
    ImGui::NextColumn();
    ImGui::Text("String: %d", game.player.string);
    ImGui::NextColumn();
    ImGui::Text("Graph: %d", game.player.graph);
    ImGui::NextColumn();
    ImGui::Text("Combinatorics: %d", game.player.combinatorics);
    ImGui::NextColumn();
    ImGui::Text("Culture: %d", game.player.culture);
    ImGui::Columns(1);
    
    ImGui::Spacing();
    ImGui::Text("Mood: %d/%d", game.player.mood, MOOD_LIMIT);
    ImGui::Text("Determination: %d", game.player.determination);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::TextWrapped("%s", game.currentEvent.c_str());
    ImGui::TextWrapped("%s", game.eventDescription.c_str());
    
    ImGui::Spacing();
    ImGui::Text("Choose:");
    for (size_t i = 0; i < game.trainingEventOptions.size(); i++) {
        if (ImGui::Button(game.trainingEventOptions[i].c_str(), ImVec2(400, 30))) {
            game.selectedTrainingOption = i;
            ProcessEvent();
        }
    }
}

void RenderContest() {
    ImGui::Text("%s", contests[game.currentContestIndex].name.c_str());
    ImGui::Text("Time Left: %d min", game.contestTimeLeft);
    ImGui::Separator();
    
    ImGui::Text("Problems:");
    for (int i = 0; i < contests[game.currentContestIndex].problemCount; i++) {
        bool selected = (i == game.selectedProblem);
        char probName[16];
        sprintf(probName, "Problem %c", 'A' + i);
        if (ImGui::Selectable(probName, selected)) {
            game.selectedProblem = i;
        }
        
        if (i < (int)game.problemScores.size()) {
            ImGui::SameLine();
            ImGui::Text("[%d pts]", game.problemScores[i]);
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    char probName[16];
    sprintf(probName, "Problem %c", 'A' + game.selectedProblem);
    ImGui::Text("Current: %s", probName);
    ImGui::Text("Difficulty: %d", (game.currentContestIndex + 1) * 10 + game.selectedProblem * 5);
    
    ImGui::Spacing();
    
    if (ImGui::Button("Think", ImVec2(100, 30))) {
        game.currentAction = 0;
        ProcessContestTurn();
    }
    ImGui::SameLine();
    if (ImGui::Button("Code", ImVec2(100, 30))) {
        game.currentAction = 1;
        ProcessContestTurn();
    }
    ImGui::SameLine();
    if (ImGui::Button("Test", ImVec2(100, 30))) {
        game.currentAction = 2;
        ProcessContestTurn();
    }
    
    ImGui::Spacing();
    if (ImGui::Button("Leave Early", ImVec2(120, 30))) {
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
    ImGui::Text("Determination Shop");
    ImGui::Text("Your Determination: %d", game.player.determination);
    ImGui::Separator();
    
    struct { const char* name; int cost; const char* effect; } items[] = {
        {"Thinking +2", 300, "Improve thinking"},
        {"Coding +2", 300, "Improve coding"},
        {"Carefulness +2", 300, "Be more careful"},
        {"Random +1", 300, "Random skill up"},
        {"Mood +2", 500, "Restore mood"},
        {"All +1", 1000, "All skills up"},
        {"Speed +1", 1500, "Be faster"},
        {"Mental +1", 1500, "Better psychology"}
    };
    
    for (int i = 0; i < 8; i++) {
        if (ImGui::Button(items[i].name, ImVec2(150, 30))) {
            ProcessShop(i);
        }
        ImGui::SameLine();
        ImGui::Text("Cost: %d | %s", items[i].cost, items[i].effect);
    }
    
    ImGui::Spacing();
    if (ImGui::Button("Leave Shop", ImVec2(120, 30))) {
        game.phase = GamePhase::Training;
        ProcessTraining();
    }
}

void RenderGameOver() {
    ImGui::Text("Game Over");
    ImGui::Separator();
    ImGui::TextWrapped("Your OI career has ended...");
    
    ImGui::Spacing();
    if (ImGui::Button("Try Again", ImVec2(150, 40))) {
        game.reset();
        game.phase = GamePhase::MainMenu;
    }
}

void RenderVictory() {
    ImGui::Text("Congratulations!");
    ImGui::Separator();
    ImGui::Text("You have completed the entire OI season!");
    
    ImGui::Spacing();
    if (ImGui::Button("Play Again", ImVec2(150, 40))) {
        game.reset();
        game.phase = GamePhase::MainMenu;
    }
}

void StartNewGame() {
    game.player = PlayerStats{};
    game.talentPoints = 5 + game.difficulty;
    game.currentContestIndex = 0;
}

void ProcessTraining() {
    std::vector<std::string> events = {"Study", "Rest", "Practice", "Relax", "Travel", 
                                         "Long Training", "Intensive Training", "Contest Practice"};
    
    game.trainingEventOptions.clear();
    for (int i = 0; i < 3; i++) {
        int idx = game.rng() % events.size();
        game.trainingEventOptions.push_back(events[idx]);
    }
    
    game.currentEvent = "Training Time";
    game.eventDescription = "Choose what to do...";
}

void ProcessEvent() {
    int effect = game.rng() % 3 - 1;
    game.player.mood += effect;
    game.player.mood = std::max(0, std::min(MOOD_LIMIT, game.player.mood));
    
    if (game.rng() % 10 == 0) {
        game.phase = GamePhase::Contest;
        StartContest();
    } else {
        ProcessTraining();
    }
}

void ProcessShop(int itemIndex) {
    int costs[] = {300, 300, 300, 300, 500, 1000, 1500, 1500};
    
    if (game.player.determination >= costs[itemIndex]) {
        game.player.determination -= costs[itemIndex];
        
        switch (itemIndex) {
            case 0: game.player.thinking += 2; break;
            case 1: game.player.coding += 2; break;
            case 2: game.player.carefulness += 2; break;
            case 4: game.player.mood = std::min(MOOD_LIMIT, game.player.mood + 2); break;
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
    
    int score = game.rng() % 100;
    game.problemScores[game.selectedProblem] = std::max(game.problemScores[game.selectedProblem], score);
}

void EndContest() {
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
    for (int s : game.problemScores) total += s;
    return total;
}

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
