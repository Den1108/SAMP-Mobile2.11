#pragma once
// Положить в: app/src/main/cpp/samp/gui/samp_widgets/hud.h
//
// Худ рисуется целиком сам (через ImGuiRenderer), без дочерних виджетов,
// поэтому порядок #include в gui.h не важен.
//
// Что показывает:
//   - стеклянная панель справа сверху: здоровье, броня (+ голод/жажда, если включить)
//   - под панелью: пилюля с деньгами и пилюля с патронами
//   - время и дата справа снизу

#include <algorithm>
#include <cmath>
#include <ctime>
#include <string>
#include "game/game.h"
#include "game/Widgets/TouchInterface.h"

extern CGame* pGame;

class Hud : public Widget
{
public:
    // Спрятать родной верхний правый блок GTA (деньги, полоски, иконка оружия).
    // Если пропадёт иконка оружия, а она вам нужна - поставьте false
    // и сдвиньте панель через kPanelRightFrac.
    static constexpr bool  kHideNativePlayerInfo = true;

    // Где стоит панель: правый край как доля ширины экрана, верх в "единицах" (1 ед = 1/1080 высоты).
    static constexpr float kPanelRightFrac = 0.875f;
    static constexpr float kPanelTop       = 18.f;

    Hud() = default;

    // Голод и жажда (0..100). Родного источника данных в клиенте нет,
    // их должен присылать сервер. Пока не вызвали - строки скрыты.
    void setNeeds(float hunger, float thirst)
    {
        m_hunger = hunger;
        m_thirst = thirst;
        m_needs  = true;
    }

    void draw(ImGuiRenderer* r) override
    {
        CPlayerPed* ped = pGame ? pGame->FindPlayerPed() : nullptr;
        if (!ped) return;

        if (kHideNativePlayerInfo) hideNativePlayerInfo();

        const ImVec2 disp = ImGui::GetIO().DisplaySize;
        const float  u    = disp.y / 1080.f;                 // масштаб под экран
        const float  fs   = UISettings::fontSize() * 0.5f;   // базовый размер текста

        // ---------- панель статов ----------
        const float panelW = 400.f * u;
        const float pad    = 14.f * u;
        const float rowH   = 30.f * u;
        const float gap    = 10.f * u;
        const int   rows   = m_needs ? 4 : 2;
        const float panelH = pad * 2 + rows * rowH + (rows - 1) * gap;

        const ImVec2 p0(disp.x * kPanelRightFrac - panelW, kPanelTop * u);
        const ImVec2 p1(p0.x + panelW, p0.y + panelH);

        r->drawRect(p0, p1, ImColor(0.04f, 0.05f, 0.08f, 0.55f), true, 12.f * u);
        r->drawRect(p0, p1, ImColor(1.f, 1.f, 1.f, 0.10f), false, 12.f * u, 1.5f);

        float y = p0.y + pad;
        const float x = p0.x + pad;
        const float w = panelW - pad * 2;

        // здоровье: при малом значении мигает
        const float hp = ped->GetHealth();
        float hpAlpha = 1.f;
        if (hp < 25.f) hpAlpha = 0.65f + 0.35f * std::sin((float)ImGui::GetTime() * 8.f);
        drawStat(r, ImVec2(x, y), w, rowH, "HP", hp / 100.f, hp,
                 ImColor(0.95f, 0.25f, 0.30f, hpAlpha), fs, u);
        y += rowH + gap;

        const float ar = ped->GetArmour();
        drawStat(r, ImVec2(x, y), w, rowH, "AR", ar / 100.f, ar,
                 ImColor(0.35f, 0.60f, 1.00f, 1.f), fs, u);
        y += rowH + gap;

        if (m_needs)
        {
            drawStat(r, ImVec2(x, y), w, rowH, "FD", m_hunger / 100.f, m_hunger,
                     ImColor(1.00f, 0.68f, 0.20f, 1.f), fs, u);
            y += rowH + gap;
            drawStat(r, ImVec2(x, y), w, rowH, "WT", m_thirst / 100.f, m_thirst,
                     ImColor(0.25f, 0.75f, 1.00f, 1.f), fs, u);
        }

        // ---------- деньги ----------
        const std::string money = "$ " + formatMoney((long long)pGame->GetLocalMoney());
        const float mfs = fs * 1.25f;
        const ImVec2 mts = r->calculateTextSize(money, mfs);
        const ImVec2 m0(p1.x - (mts.x + 36.f * u), p1.y + 10.f * u);
        const ImVec2 m1(p1.x, m0.y + mts.y + 16.f * u);
        r->drawRect(m0, m1, ImColor(0.04f, 0.05f, 0.08f, 0.60f), true, (m1.y - m0.y) * 0.5f);
        r->drawRect(m0, m1, ImColor(0.35f, 0.85f, 0.40f, 0.35f), false, (m1.y - m0.y) * 0.5f, 1.5f);
        r->drawText(ImVec2(m0.x + 18.f * u, m0.y + 8.f * u),
                    ImColor(0.45f, 0.95f, 0.50f, 1.f), money, true, mfs);

        // ---------- патроны ----------
        CWeapon* wp = ped->GetCurrentWeaponSlot();
        if (wp && wp->dwType != 0)
        {
            const std::string ammo = "AMMO " + std::to_string(wp->dwAmmo);
            const ImVec2 ats = r->calculateTextSize(ammo, mfs);
            const ImVec2 a0(m0.x - 10.f * u - (ats.x + 36.f * u), m0.y);
            const ImVec2 a1(m0.x - 10.f * u, m1.y);
            r->drawRect(a0, a1, ImColor(0.04f, 0.05f, 0.08f, 0.60f), true, (a1.y - a0.y) * 0.5f);
            r->drawRect(a0, a1, ImColor(1.00f, 0.75f, 0.25f, 0.35f), false, (a1.y - a0.y) * 0.5f, 1.5f);
            r->drawText(ImVec2(a0.x + 18.f * u, a0.y + 8.f * u),
                        ImColor(1.00f, 0.85f, 0.45f, 1.f), ammo, true, mfs);
        }

        // ---------- время и дата ----------
        std::time_t t = std::time(nullptr);
        std::tm tmv{};
        localtime_r(&t, &tmv);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%H:%M   %d.%m.%Y", &tmv);
        const std::string clock = buf;
        const ImVec2 cts = r->calculateTextSize(clock, fs);
        r->drawText(ImVec2(disp.x * 0.86f - cts.x, disp.y - cts.y - 10.f * u),
                    ImColor(1.f, 1.f, 1.f, 0.75f), clock, true, fs);
    }

private:
    // Одна строка статов: [чип] [полоска] [число]
    void drawStat(ImGuiRenderer* r, ImVec2 pos, float w, float h,
                  const char* tag, float v01, float value,
                  ImColor col, float fs, float u)
    {
        v01 = std::clamp(v01, 0.f, 1.f);

        // чип с подписью
        const float chipW = 44.f * u;
        r->drawRect(pos, ImVec2(pos.x + chipW, pos.y + h),
                    ImColor(col.Value.x, col.Value.y, col.Value.z, 0.90f * col.Value.w),
                    true, h * 0.5f);
        const std::string tg = tag;
        const ImVec2 tts = r->calculateTextSize(tg, fs * 0.8f);
        r->drawText(ImVec2(pos.x + (chipW - tts.x) * 0.5f, pos.y + (h - tts.y) * 0.5f),
                    ImColor(0.05f, 0.05f, 0.08f, 1.f), tg, false, fs * 0.8f);

        // число справа
        const std::string val = std::to_string((int)std::round(std::max(value, 0.f)));
        const ImVec2 vts = r->calculateTextSize(val, fs);
        const float valW = 56.f * u;
        r->drawText(ImVec2(pos.x + w - vts.x, pos.y + (h - vts.y) * 0.5f),
                    ImColor(1.f, 1.f, 1.f, 0.95f), val, true, fs);

        // полоска
        const float bx0 = pos.x + chipW + 12.f * u;
        const float bx1 = pos.x + w - valW;
        const float by0 = pos.y + h * 0.24f;
        const float by1 = pos.y + h * 0.76f;
        const float rad = (by1 - by0) * 0.5f;

        r->drawRect(ImVec2(bx0, by0), ImVec2(bx1, by1), ImColor(1.f, 1.f, 1.f, 0.10f), true, rad);
        if (v01 > 0.005f)
        {
            const float fx = bx0 + std::max((bx1 - bx0) * v01, rad * 2.f);
            r->drawRect(ImVec2(bx0, by0), ImVec2(fx, by1), col, true, rad);
            // блик сверху
            r->drawRect(ImVec2(bx0 + rad, by0 + 2.f * u), ImVec2(fx - rad, by0 + 4.f * u),
                        ImColor(1.f, 1.f, 1.f, 0.25f), true, 1.f * u);
        }
    }

    // 1915985 -> "1 915 985"
    static std::string formatMoney(long long v)
    {
        const bool neg = v < 0;
        std::string s = std::to_string(neg ? -v : v);
        for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert(i, " ");
        return neg ? "-" + s : s;
    }

    static void hideNativePlayerInfo()
    {
        if (!CTouchInterface::m_pWidgets) return;
        CWidgetGta* w = CTouchInterface::m_pWidgets[WidgetIDs::WIDGET_PLAYER_INFO];
        if (w) w->SetEnabled(false);
    }

    float m_hunger = 100.f;
    float m_thirst = 100.f;
    bool  m_needs  = false;
};
