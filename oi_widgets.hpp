// ============================================================================
//  oi_widgets.hpp  —  "OI Terminal" 自定义控件库（基于 ImDrawList）
//  圆角进度环 / 难度徽章 / AP 段位 / 能力条 / 区段标题 / 强调按钮 / 卡片。
//  纯公共 API（仅依赖 imgui.h），不需要 imgui_internal.h。
//
//  用法：
//      #include "oi_theme.hpp"
//      #include "oi_widgets.hpp"
//      OIWidgets::SectionHeader("STATUS", "角色状态");
//      OIWidgets::TierBadge("省选/NOI-", OITheme::TierColor(level));
//      OIWidgets::ProgressRing("思考", cur, total, OITheme::Col::Teal);
// ============================================================================
#ifndef OI_WIDGETS_HPP
#define OI_WIDGETS_HPP

#include "imgui.h"
#include "imgui_internal.h"
#include "oi_theme.hpp"
#include <cmath>
#include <cstdio>

namespace OIWidgets {

namespace detail { inline ImU32 U32(const ImVec4& c) { return ImGui::GetColorU32(c); } }

// ---------------------------------------------------------------------------
//  区段标题：accent tick + 英文标签 + 中文标签 + 贯穿细线
//  enFont 可传入一个等宽小字号字体（可选），用于英文标签的“终端感”。
// ---------------------------------------------------------------------------
inline void SectionHeader(const char* en, const char* cjk = nullptr, ImFont* enFont = nullptr) {
    using namespace OITheme;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = ImGui::GetTextLineHeight();

    dl->AddRectFilled(ImVec2(p.x, p.y + 1), ImVec2(p.x + 3, p.y + h - 1), detail::U32(Col::Teal), 1.5f);
    ImGui::SetCursorScreenPos(ImVec2(p.x + 12, p.y));

    if (enFont) ImGui::PushFont(enFont);
    ImGui::PushStyleColor(ImGuiCol_Text, Col::Teal);
    ImGui::TextUnformatted(en);
    ImGui::PopStyleColor();
    if (enFont) ImGui::PopFont();

    if (cjk && cjk[0]) { ImGui::SameLine(0, 8); ImGui::TextUnformatted(cjk); }

    ImGui::SameLine(0, 12);
    ImVec2 rp = ImGui::GetCursorScreenPos();
    float right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    if (right > rp.x + 4)
        dl->AddLine(ImVec2(rp.x, rp.y + h * 0.5f), ImVec2(right, rp.y + h * 0.5f), detail::U32(Col::Line));
    ImGui::NewLine();
    ImGui::Spacing();
}

// ---------------------------------------------------------------------------
//  难度徽章：圆点 + 文本，半透明同色底（洛谷配色用 OITheme::TierColor）
// ---------------------------------------------------------------------------
inline void TierBadge(const char* text, const ImVec4& color) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 ts = ImGui::CalcTextSize(text);
    const float padX = 8, padY = 3, dot = 6, gap = 5;
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = padX * 2 + dot + gap + ts.x;
    float hgt = ts.y + padY * 2;

    ImVec4 bg = color; bg.w = 0.15f;
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + hgt), detail::U32(bg), 4.0f);
    float cy = p.y + hgt * 0.5f;
    dl->AddCircleFilled(ImVec2(p.x + padX + dot * 0.5f, cy), dot * 0.5f, detail::U32(color));
    dl->AddText(ImVec2(p.x + padX + dot + gap, p.y + padY), detail::U32(color), text);
    ImGui::Dummy(ImVec2(w, hgt));
}

// 纯文本标签（终端风），无圆点
inline void Tag(const char* text, const ImVec4& color = OITheme::Col::Teal) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 ts = ImGui::CalcTextSize(text);
    const float padX = 7, padY = 2;
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = padX * 2 + ts.x, hgt = ts.y + padY * 2;
    ImVec4 bg = color; bg.w = 0.14f;
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + hgt), detail::U32(bg), 4.0f);
    dl->AddText(ImVec2(p.x + padX, p.y + padY), detail::U32(color), text);
    ImGui::Dummy(ImVec2(w, hgt));
}

// ---------------------------------------------------------------------------
//  AP 段位指示：max 个小段，前 cur 个填充 teal
// ---------------------------------------------------------------------------
inline void ApPips(int cur, int maxv, float w = 16, float h = 8, float gap = 4) {
    using namespace OITheme;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    for (int i = 0; i < maxv; ++i) {
        ImVec2 a(p.x + i * (w + gap), p.y), b(a.x + w, a.y + h);
        bool on = i < cur;
        dl->AddRectFilled(a, b, detail::U32(on ? Col::Teal : Col::Bg3), 2.0f);
        dl->AddRect(a, b, detail::U32(on ? Col::Teal : Col::Line2), 2.0f);
    }
    ImGui::Dummy(ImVec2(maxv > 0 ? maxv * (w + gap) - gap : 0, h));
}

// ---------------------------------------------------------------------------
//  能力条：名称(labelW) + 细进度条(barW) + 右对齐 mono 数值
//  value/maxv 决定填充；color 为条颜色。
// ---------------------------------------------------------------------------
inline void StatBar(const char* name, int value, int maxv, const ImVec4& color,
                    float labelW = 56, float barW = 90, ImFont* numFont = nullptr) {
    using namespace OITheme;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushStyleColor(ImGuiCol_Text, Col::TxtDim);
    ImGui::TextUnformatted(name);
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 0);

    float startX = ImGui::GetCursorPosX();
    ImGui::SetCursorPosX(startX < labelW ? labelW : startX); // 至少对齐 labelW
    ImVec2 bp = ImGui::GetCursorScreenPos();
    float bh = 6.0f, by = bp.y + (ImGui::GetTextLineHeight() - bh) * 0.5f;
    dl->AddRectFilled(ImVec2(bp.x, by), ImVec2(bp.x + barW, by + bh), detail::U32(Col::Bg1b), 3.0f);
    dl->AddRect(ImVec2(bp.x, by), ImVec2(bp.x + barW, by + bh), detail::U32(Col::Line), 3.0f);
    float frac = maxv > 0 ? (float)value / (float)maxv : 0.0f;
    if (frac > 1) frac = 1;
    if (frac > 0)
        dl->AddRectFilled(ImVec2(bp.x, by), ImVec2(bp.x + barW * frac, by + bh), detail::U32(color), 3.0f);

    ImGui::Dummy(ImVec2(barW + 8, ImGui::GetTextLineHeight()));
    ImGui::SameLine(0, 0);
    char buf[16]; snprintf(buf, sizeof buf, "%d", value);
    if (numFont) ImGui::PushFont(numFont);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(buf);
    ImGui::PopStyleColor();
    if (numFont) ImGui::PopFont();
}

// ---------------------------------------------------------------------------
//  圆角进度环：中心显示 cur，下方 "/ total" 与标签
// ---------------------------------------------------------------------------
inline void ProgressRing(const char* label, int cur, int total, const ImVec4& colorVec,
                         float radius = 30.0f, float thickness = 7.0f, ImFont* numFont = nullptr) {
    using namespace OITheme;
    ImGui::BeginGroup();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 color = detail::U32(colorVec);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float sz = (radius + thickness) * 2.0f;
    ImVec2 center(p.x + sz * 0.5f, p.y + sz * 0.5f);

    // 背景环
    dl->PathClear();
    dl->PathArcTo(center, radius, 0.0f, IM_PI * 2.0f, 64);
    dl->PathStroke(detail::U32(Col::Bg3), 0, thickness);

    // 前景弧
    float frac = total > 0 ? (float)cur / (float)total : 0.0f;
    if (frac > 1) frac = 1;
    if (frac > 0) {
        float a0 = -IM_PI * 0.5f;
        float a1 = a0 + IM_PI * 2.0f * frac;
        dl->PathClear();
        dl->PathArcTo(center, radius, a0, a1, 64);
        dl->PathStroke(color, 0, thickness);
        dl->AddCircleFilled(ImVec2(center.x + cosf(a0) * radius, center.y + sinf(a0) * radius), thickness * 0.5f, color);
        dl->AddCircleFilled(ImVec2(center.x + cosf(a1) * radius, center.y + sinf(a1) * radius), thickness * 0.5f, color);
    }

    // 中心数值
    if (numFont) ImGui::PushFont(numFont);
    char buf[16]; snprintf(buf, sizeof buf, "%d", cur);
    ImVec2 ts = ImGui::CalcTextSize(buf);
    dl->AddText(ImVec2(center.x - ts.x * 0.5f, center.y - ts.y - 1), color, buf);
    if (numFont) ImGui::PopFont();

    char buf2[16]; snprintf(buf2, sizeof buf2, "/ %d", total);
    ImVec2 ts2 = ImGui::CalcTextSize(buf2);
    dl->AddText(ImVec2(center.x - ts2.x * 0.5f, center.y + 2), detail::U32(Col::TxtFaint), buf2);

    ImGui::Dummy(ImVec2(sz, sz));

    if (label && label[0]) {
        ImVec2 ls = ImGui::CalcTextSize(label);
        float pad = (sz - ls.x) * 0.5f; if (pad < 0) pad = 0;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pad);
        ImGui::PushStyleColor(ImGuiCol_Text, Col::TxtFaint);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();
    }
    ImGui::EndGroup();
}

// ---------------------------------------------------------------------------
//  线性进度条（带圆角内嵌底，比原生更精致）
// ---------------------------------------------------------------------------
inline void Bar(float frac, const ImVec2& size, const ImVec4& color, const char* overlay = nullptr) {
    using namespace OITheme;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float r = size.y * 0.5f;
    if (frac < 0) frac = 0; if (frac > 1) frac = 1;
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), detail::U32(Col::Bg1b), r);
    dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), detail::U32(Col::Line), r);
    if (frac > 0) {
        float w = size.x * frac; if (w < size.y) w = size.y;
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + size.y), detail::U32(color), r);
    }
    if (overlay && overlay[0]) {
        ImVec2 ts = ImGui::CalcTextSize(overlay);
        dl->AddText(ImVec2(p.x + (size.x - ts.x) * 0.5f, p.y + (size.y - ts.y) * 0.5f),
                    detail::U32(Col::Txt), overlay);
    }
    ImGui::Dummy(size);
}

// ---------------------------------------------------------------------------
//  强调按钮（teal 实心，深色文字）+ 语义按钮（幽灵底）
// ---------------------------------------------------------------------------
inline bool PrimaryButton(const char* label, const ImVec2& size = ImVec2(0, 0)) {
    using namespace OITheme;
    ImGui::PushStyleColor(ImGuiCol_Button, Col::Teal);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Col::TealBright);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Col::TealDim);
    ImGui::PushStyleColor(ImGuiCol_Text, Hex(0x04110F));
    bool r = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    return r;
}

// 语义幽灵按钮（warn / danger 等）：传入主色，底为该色 14% 透明
inline bool GhostButton(const char* label, const ImVec4& color, const ImVec2& size = ImVec2(0, 0)) {
    ImVec4 bg = color; bg.w = 0.14f;
    ImVec4 bgH = color; bgH.w = 0.24f;
    ImVec4 bgA = color; bgA.w = 0.30f;
    ImGui::PushStyleColor(ImGuiCol_Button, bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bgH);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, bgA);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::PushStyleColor(ImGuiCol_Border, bgH);
    bool r = ImGui::Button(label, size);
    ImGui::PopStyleColor(5);
    return r;
}

// ---------------------------------------------------------------------------
//  卡片：带边框 + 内边距 + 自适应高度的子窗口
//      if (OIWidgets::BeginCard("id")) { ... } OIWidgets::EndCard();
//  size.y == 0 时自动按内容高度收缩（需 ImGui v1.90+ 的 AutoResizeY）。
// ---------------------------------------------------------------------------
inline bool BeginCard(const char* id, const ImVec2& size = ImVec2(0, 0), bool inset = false) {
    using namespace OITheme;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, inset ? Col::Bg1b : Col::Bg1);
    ImGui::PushStyleColor(ImGuiCol_Border, Col::Line);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 14));
    ImGuiChildFlags flags = ImGuiChildFlags_Borders;
    if (size.y == 0.0f) flags |= ImGuiChildFlags_AutoResizeY;
    bool r = ImGui::BeginChild(id, size, flags, ImGuiWindowFlags_NoSavedSettings);
    return r;
}
inline void EndCard() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

// ---------------------------------------------------------------------------
//  键位提示 [F1] 风格小标
// ---------------------------------------------------------------------------
inline void Kbd(const char* text) {
    using namespace OITheme;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 ts = ImGui::CalcTextSize(text);
    const float padX = 6, padY = 1;
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = padX * 2 + ts.x, hgt = ts.y + padY * 2;
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + hgt), detail::U32(Col::Bg2), 4.0f);
    dl->AddRect(p, ImVec2(p.x + w, p.y + hgt), detail::U32(Col::Line2), 4.0f);
    dl->AddText(ImVec2(p.x + padX, p.y + padY), detail::U32(Col::TxtDim), text);
    ImGui::Dummy(ImVec2(w, hgt));
}

} // namespace OIWidgets

#endif // OI_WIDGETS_HPP
