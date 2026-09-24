#pragma once
// Положить в: app/src/main/cpp/samp/gui/samp_widgets/hud.h
// Виджет вызывается из UI (gui.h / gui.cpp), см. инструкцию в ответе.

#include <algorithm>
#include <string>
#include "game/game.h"

extern CGame* pGame;

class Hud : public Widget
{
public:
    Hud()
    {
        const float fs = UISettings::fontSize() / 2;

        // Полоски здоровья и брони (empty, filled, рамка)
        m_health = new ProgressBar(ImColor(0.25f, 0.00f, 0.00f, 0.65f),
                                   ImColor(0.90f, 0.10f, 0.10f, 1.00f), true);
        m_armour = new ProgressBar(ImColor(0.20f, 0.20f, 0.20f, 0.65f),
                                   ImColor(0.85f, 0.85f, 0.85f, 1.00f), true);
        addChild(m_health);
        addChild(m_armour);

        // Подписи
        m_money = new Label("$0", ImColor(0.35f, 0.85f, 0.35f), true, fs);
        m_ammo  = new Label(" ",  ImColor(1.00f, 1.00f, 1.00f), true, fs);
        addChild(m_money);
        addChild(m_ammo);
    }

    void performLayout() override
    {
        const float barW = sx(260.f);
        const float barH = sy(22.f);
        const float gap  = sy(8.f);

        m_health->setFixedSize(ImVec2(barW, barH));
        m_health->setPosition(ImVec2(0.f, 0.f));

        m_armour->setFixedSize(ImVec2(barW, barH));
        m_armour->setPosition(ImVec2(0.f, barH + gap));

        m_money->setPosition(ImVec2(0.f, (barH + gap) * 2));
        m_ammo->setPosition(ImVec2(barW * 0.55f, (barH + gap) * 2));

        Widget::performLayout();
    }

    void draw(ImGuiRenderer* renderer) override
    {
        update();
        Widget::draw(renderer); // рисует всех детей (полоски и подписи)
    }

private:
    void update()
    {
        if (!pGame) return;

        CPlayerPed* ped = pGame->FindPlayerPed();
        if (!ped) return;

        // здоровье / броня (0..100)
        m_health->setValue(std::clamp(ped->GetHealth() / 100.0f, 0.0f, 1.0f));
        m_armour->setValue(std::clamp(ped->GetArmour() / 100.0f, 0.0f, 1.0f));

        // деньги (обновляем текст только при изменении)
        std::string money = "$" + std::to_string(pGame->GetLocalMoney());
        if (money != m_money->text()) m_money->setText(money);

        // патроны текущего оружия
        std::string ammo;
        CWeapon* w = ped->GetCurrentWeaponSlot();
        if (w && w->dwType != 0)
            ammo = std::to_string(w->dwAmmo);
        else
            ammo = " ";
        if (ammo != m_ammo->text()) m_ammo->setText(ammo);
    }

    static float sx(float x) { return x * ImGui::GetIO().DisplaySize.x / 1920.f; }
    static float sy(float y) { return y * ImGui::GetIO().DisplaySize.y / 1080.f; }

    ProgressBar* m_health;
    ProgressBar* m_armour;
    Label*       m_money;
    Label*       m_ammo;
};
