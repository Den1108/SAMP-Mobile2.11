// Положить в: app/src/main/cpp/samp/gui/samp_widgets/hud_ping.cpp
//
// Отдельная единица трансляции только для того, чтобы безопасно подключить
// net/netgame.h (hud.h не может - см. комментарий в hud.h про цикл инклюдов).

#include "net/netgame.h"

extern CNetGame* pNetGame;

int GetHudPing()
{
    if (!pNetGame || !pNetGame->GetPlayerPool())
        return 0;
    return pNetGame->GetPlayerPool()->GetLocalPlayerPing();
}
