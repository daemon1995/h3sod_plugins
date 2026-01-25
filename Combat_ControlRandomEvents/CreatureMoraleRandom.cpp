#include "framework.h"

CreatureMoraleRandom *CreatureMoraleRandom::instance = nullptr;
CreatureMoraleRandom::CreatureMoraleRandom() : IGamePatch(_PI)
{
    CreatePatches();
}

void __stdcall CreatureMoraleRandom::BattleMgr_CheckGoodMorale(HiHook *h, H3CombatManager *_this, const int side,
                                                               const int index)
{

    const auto &creatureSettings = CreatureSettingsManager::GetCreatureSettings(side, index);

    switch (creatureSettings.positiveMorale)
    {
    case eTriggerState::DEFAULT:

        if (instance->positiveMoralePatchActive)
        {
            instance->positiveMoralePatch->Undo();
            instance->positiveMoralePatchActive = FALSE;
        }

        break;
        // Always good morale
    case eTriggerState::ALWAYS:
        if (!instance->positiveMoralePatchActive)
        {
            instance->positiveMoralePatch->Apply();
            instance->positiveMoralePatchActive = TRUE;
        }
        break;

        // Never good morale
    case eTriggerState::NEVER:
        return;

    default:
        if (instance->positiveMoralePatchActive)
        {
            instance->positiveMoralePatch->Undo();
            instance->positiveMoralePatchActive = FALSE;
        }
        break;
    }
    THISCALL_3(void, h->GetDefaultFunc(), _this, side, index);
}
int __stdcall CreatureMoraleRandom::BattleMgr_CheckBadMorale(HiHook *h, H3CombatManager *_this, const int side,
                                                             const int index)
{
    const auto &creatureSettings = CreatureSettingsManager::GetCreatureSettings(side, index);

    switch (creatureSettings.negativeMorale)
    {
    case eTriggerState::DEFAULT:

        if (instance->negativeMoralePatchActive)
        {
            instance->negativeMoralePatch->Undo();
            instance->negativeMoralePatchActive = FALSE;
        }

        break;
        // Always bad morale
    case eTriggerState::ALWAYS:
        if (!instance->negativeMoralePatchActive)
        {
            instance->negativeMoralePatch->Apply();
            instance->negativeMoralePatchActive = TRUE;
        }
        break;

        // Never bad morale
    case eTriggerState::NEVER:
        return 0;

    default:
        if (instance->negativeMoralePatchActive)
        {
            instance->negativeMoralePatch->Undo();
            instance->negativeMoralePatchActive = FALSE;
        }
        break;
    }

    // defalt behavior
    return THISCALL_3(int, h->GetDefaultFunc(), _this, side, index);
}
void CreatureMoraleRandom::CreatePatches()
{
    if (!this->m_isInited)
    {
        this->m_isInited = true;
        positiveMoralePatch = _PI->CreateHexPatch(0x04645A9, "EB 15 909090");
        negativeMoralePatch = _PI->CreateHexPatch(0x046479D, "EB 3F 909090");
        WriteHiHook(0x0464500, THISCALL_, BattleMgr_CheckGoodMorale);
        WriteHiHook(0x0464720, THISCALL_, BattleMgr_CheckBadMorale);
    }
}

CreatureMoraleRandom &CreatureMoraleRandom::GetInstance()
{
    if (!instance)
    {
        instance = new CreatureMoraleRandom();
    }
    return *instance;
}
