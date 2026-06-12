// ============================================================================
//  oi_theme.hpp  —  "OI Terminal" 重设计主题
//  精致深色 + Teal 强调色 + 洛谷难度配色。基于 Dear ImGui，不更换依赖。
//
//  用法（main.cpp）：
//      #include "oi_theme.hpp"
//      ...
//      OITheme::Apply();            // 替换原 ApplyGuiStyle()
//      ImVec4 c = OITheme::TierColor(level);   // 洛谷分级配色
//
//  注：颜色全部以 0–1 归一化。所有 16 进制注释为对应 sRGB。
// ============================================================================
#ifndef OI_THEME_HPP
#define OI_THEME_HPP

#include "imgui.h"

namespace OITheme {

// ---- 16 进制 -> ImVec4 辅助 ----------------------------------------------
constexpr ImVec4 Hex(unsigned int rgb, float a = 1.0f) {
    return ImVec4(((rgb >> 16) & 0xFF) / 255.0f,
                  ((rgb >> 8)  & 0xFF) / 255.0f,
                  ( rgb        & 0xFF) / 255.0f, a);
}

// ---- 调色板（与设计稿 skin.css 一一对应）---------------------------------
namespace Col {
    // 底色
    constexpr ImVec4 Bg0   = Hex(0x0B0E14); // 应用背景
    constexpr ImVec4 Bg1   = Hex(0x11151E); // 面板 / 卡片 / 侧栏
    constexpr ImVec4 Bg1b  = Hex(0x0E121A); // 更深的嵌入背景（日志 / 输入槽）
    constexpr ImVec4 Bg2   = Hex(0x171C28); // 抬起：按钮底 / 输入 / Tab
    constexpr ImVec4 Bg3   = Hex(0x1F2634); // hover
    constexpr ImVec4 Bg4   = Hex(0x283143); // active
    constexpr ImVec4 Line  = Hex(0x232B3A); // 边框
    constexpr ImVec4 Line2 = Hex(0x2E394C); // 强边框 / 分隔

    // 文本
    constexpr ImVec4 Txt      = Hex(0xE4E8F1);
    constexpr ImVec4 TxtDim   = Hex(0x8B95A8);
    constexpr ImVec4 TxtFaint = Hex(0x586073);

    // Teal 主强调
    constexpr ImVec4 Teal       = Hex(0x2DD4BF);
    constexpr ImVec4 TealBright = Hex(0x5BEAD8);
    constexpr ImVec4 TealDim    = Hex(0x1B7A72);
    constexpr ImVec4 TealGhost  = Hex(0x2DD4BF, 0.12f);
    constexpr ImVec4 TealGhost2 = Hex(0x2DD4BF, 0.22f);

    // 洛谷难度配色（深色下做了提亮）
    constexpr ImVec4 LgGray   = Hex(0xBFBFBF); // 暂无评定
    constexpr ImVec4 LgRed    = Hex(0xFE4C61); // 入门
    constexpr ImVec4 LgOrange = Hex(0xF39C11); // 普及-
    constexpr ImVec4 LgYellow = Hex(0xFFC116); // 普及/提高-
    constexpr ImVec4 LgGreen  = Hex(0x5EB95E); // 普及+/提高
    constexpr ImVec4 LgBlue   = Hex(0x3498DB); // 提高+/省选-
    constexpr ImVec4 LgPurple = Hex(0xB362E0); // 省选/NOI-
    constexpr ImVec4 LgBlack  = Hex(0x6B7CE0); // NOI/NOI+（亮靛替代纯黑）

    // 语义
    constexpr ImVec4 Ok   = Hex(0x5EB95E);
    constexpr ImVec4 Warn = Hex(0xFFC116);
    constexpr ImVec4 Bad  = Hex(0xFE4C61);
    constexpr ImVec4 Info = Hex(0x3498DB);
}

// ---- 洛谷分级 -> 颜色 ------------------------------------------------------
// 与原 getLuoguTierColor 保持相同的 level 区间，仅替换为规范洛谷配色。
inline ImVec4 TierColor(int level) {
    if (level <= 0) return Col::LgGray;
    if (level <= 1) return Col::LgRed;     // 入门
    if (level <= 2) return Col::LgOrange;  // 普及-
    if (level <= 3) return Col::LgYellow;  // 普及/提高-
    if (level <= 4) return Col::LgGreen;   // 普及+/提高
    if (level <= 5) return Col::LgBlue;    // 提高+/省选-
    if (level <= 6) return Col::LgPurple;  // 省选/NOI-
    return Col::LgBlack;                    // NOI/NOI+
}

// 成功率 / 安全度 -> 语义色（高=绿，中=黄，低=红）
inline ImVec4 RateColor(double rate, double good = 0.7, double mid = 0.4) {
    if (rate >= good) return Col::Ok;
    if (rate >= mid)  return Col::Warn;
    return Col::Bad;
}
// 风险度（越低越好）-> 语义色
inline ImVec4 RiskColor(double risk, double good = 0.2, double mid = 0.5) {
    if (risk <= good) return Col::Ok;
    if (risk <= mid)  return Col::Warn;
    return Col::Bad;
}

// ---- 应用主题 --------------------------------------------------------------
inline void Apply() {
    ImGuiStyle& s = ImGui::GetStyle();

    // 几何：克制的圆角 + 宽松留白，营造“精致”而非“圆滚”
    s.WindowRounding    = 10.0f;
    s.ChildRounding     = 8.0f;
    s.FrameRounding     = 6.0f;
    s.PopupRounding     = 8.0f;
    s.GrabRounding      = 6.0f;
    s.TabRounding       = 7.0f;
    s.ScrollbarRounding = 8.0f;

    s.WindowPadding   = ImVec2(18, 16);
    s.FramePadding    = ImVec2(12, 8);
    s.CellPadding     = ImVec2(8, 6);
    s.ItemSpacing     = ImVec2(10, 10);
    s.ItemInnerSpacing= ImVec2(8, 6);
    s.IndentSpacing   = 18.0f;
    s.ScrollbarSize   = 12.0f;
    s.GrabMinSize     = 12.0f;

    s.WindowBorderSize = 1.0f;
    s.ChildBorderSize  = 1.0f;
    s.FrameBorderSize  = 1.0f;   // 细边框是“终端/代码感”的关键
    s.TabBorderSize    = 0.0f;
    s.SeparatorTextBorderSize = 1.0f;

    s.WindowTitleAlign = ImVec2(0.0f, 0.5f);

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                 = Col::Txt;
    c[ImGuiCol_TextDisabled]         = Col::TxtFaint;
    c[ImGuiCol_WindowBg]             = Col::Bg0;
    c[ImGuiCol_ChildBg]              = Col::Bg1;
    c[ImGuiCol_PopupBg]              = OITheme::Hex(0x11151E, 0.98f);
    c[ImGuiCol_Border]               = Col::Line;
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]              = Col::Bg2;
    c[ImGuiCol_FrameBgHovered]       = Col::Bg3;
    c[ImGuiCol_FrameBgActive]        = Col::Bg4;
    c[ImGuiCol_TitleBg]              = Col::Bg0;
    c[ImGuiCol_TitleBgActive]        = Col::Bg0;
    c[ImGuiCol_TitleBgCollapsed]     = Col::Bg0;
    c[ImGuiCol_MenuBarBg]            = Col::Bg1;
    c[ImGuiCol_ScrollbarBg]          = Col::Bg1b;
    c[ImGuiCol_ScrollbarGrab]        = Col::Line2;
    c[ImGuiCol_ScrollbarGrabHovered] = OITheme::Hex(0x3A4356);
    c[ImGuiCol_ScrollbarGrabActive]  = OITheme::Hex(0x46506A);
    c[ImGuiCol_CheckMark]            = Col::Teal;
    c[ImGuiCol_SliderGrab]           = Col::Teal;
    c[ImGuiCol_SliderGrabActive]     = Col::TealBright;
    // 默认按钮：低调的“次级”样式；主操作请用 OIWidgets::PrimaryButton
    c[ImGuiCol_Button]               = Col::Bg2;
    c[ImGuiCol_ButtonHovered]        = Col::Bg3;
    c[ImGuiCol_ButtonActive]         = Col::Bg4;
    c[ImGuiCol_Header]               = Col::TealGhost;
    c[ImGuiCol_HeaderHovered]        = Col::TealGhost2;
    c[ImGuiCol_HeaderActive]         = Col::TealGhost2;
    c[ImGuiCol_Separator]            = Col::Line;
    c[ImGuiCol_SeparatorHovered]     = Col::TealDim;
    c[ImGuiCol_SeparatorActive]      = Col::Teal;
    c[ImGuiCol_ResizeGrip]           = Col::Line2;
    c[ImGuiCol_ResizeGripHovered]    = Col::TealDim;
    c[ImGuiCol_ResizeGripActive]     = Col::Teal;
    c[ImGuiCol_Tab]                  = Col::Bg1;
    c[ImGuiCol_TabHovered]           = Col::Bg3;
    c[ImGuiCol_TabActive]            = Col::Bg2;
    c[ImGuiCol_TabUnfocused]         = Col::Bg1;
    c[ImGuiCol_TabUnfocusedActive]   = Col::Bg2;
    c[ImGuiCol_TableHeaderBg]        = Col::Bg2;
    c[ImGuiCol_TableBorderStrong]    = Col::Line2;
    c[ImGuiCol_TableBorderLight]     = Col::Line;
    c[ImGuiCol_TableRowBg]           = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TableRowBgAlt]        = OITheme::Hex(0x171C28, 0.5f);
    c[ImGuiCol_PlotLines]            = Col::Teal;
    c[ImGuiCol_PlotLinesHovered]     = Col::TealBright;
    c[ImGuiCol_PlotHistogram]        = Col::Teal;
    c[ImGuiCol_PlotHistogramHovered] = Col::TealBright;
    c[ImGuiCol_TextSelectedBg]       = Col::TealGhost2;
    c[ImGuiCol_DragDropTarget]       = Col::Teal;
    c[ImGuiCol_NavHighlight]         = Col::Teal;
    c[ImGuiCol_NavWindowingHighlight]= ImVec4(0.9f, 0.95f, 0.95f, 0.3f);
    c[ImGuiCol_NavWindowingDimBg]    = OITheme::Hex(0x0B0E14, 0.6f);
    c[ImGuiCol_ModalWindowDimBg]     = OITheme::Hex(0x05070B, 0.7f);

#ifdef ImGuiCol_TabSelected            // ImGui 新版本别名（v1.91+）
    c[ImGuiCol_TabSelected]          = Col::Bg2;
    c[ImGuiCol_TabSelectedOverline]  = Col::Teal;
    c[ImGuiCol_TabDimmed]            = Col::Bg1;
    c[ImGuiCol_TabDimmedSelected]    = Col::Bg2;
#endif
}

} // namespace OITheme

#endif // OI_THEME_HPP
