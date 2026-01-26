#include "framework.h"

CreatureMoraleRandom *CreatureMoraleRandom::instance = nullptr;
CreatureMoraleRandom::CreatureMoraleRandom() : IGamePatch(globalPatcher->CreateInstance(instanceName))
{
    CreatePatches();
}

void __stdcall CreatureMoraleRandom::BattleMgr_CheckGoodMorale(HiHook *h, const H3CombatManager *_this, const int side,
                                                               const int index)
{

    instance->currentSettings = &CreatureSettingsManager::GetCreatureSettings(side, index);

    THISCALL_3(void, h->GetDefaultFunc(), _this, side, index);
    instance->currentSettings = nullptr;
}

int __stdcall CreatureMoraleRandom::BattleStack_PositiveMoraleRandom(HiHook *hook, const int min, const int max)
{
    return CombatCreatureSettings::BattleStack_Random(hook, min, max, instance->currentSettings->positiveMorale);
}

int __stdcall CreatureMoraleRandom::BattleMgr_CheckBadMorale(HiHook *h, const H3CombatManager *_this, const int side,
                                                             const int index)
{
    instance->currentSettings = &CreatureSettingsManager::GetCreatureSettings(side, index);
    int result = THISCALL_3(int, h->GetDefaultFunc(), _this, side, index);
    instance->currentSettings = nullptr;
    return result;
}
int __stdcall CreatureMoraleRandom::BattleStack_NegativeMoraleRandom(HiHook *hook, const int min, const int max)
{

    return CombatCreatureSettings::BattleStack_Random(hook, min, max, instance->currentSettings->negativeMorale);
}

int __stdcall CreatureMoraleRandom::AIBattleStack_NegativeMoraleRandom(HiHook *hook, const int min, const int max)
{
    if (instance->currentSettings)
    {
        switch (instance->currentSettings->negativeMorale.triggerState)
        {
        case eTriggerState::ALWAYS:
            return max; // always trigger ability
        case eTriggerState::NEVER:
            return min; // never trigger ability
        default:
            break; // return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
        }
    }

    return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
}

char __stdcall CreatureMoraleRandom::BattleMgr_CheckFear(HiHook *h, const H3CombatManager *_this,
                                                         const H3CombatCreature *creature)
{

    instance->currentSettings = &CreatureSettingsManager::GetCreatureSettings(creature);
    char result = THISCALL_2(char, h->GetDefaultFunc(), _this, creature);
    instance->currentSettings = nullptr;
    return result;
}

int __stdcall CreatureMoraleRandom::BattleStack_FearRandom(HiHook *hook, const int min, const int max)
{
    return CombatCreatureSettings::BattleStack_Random(hook, min, max, instance->currentSettings->fear);
}

void CreatureMoraleRandom::CreatePatches()
{
    if (!this->m_isInited)
    {
        this->m_isInited = true;

        // positive morale code
        WriteHiHook(0x0464500, THISCALL_, BattleMgr_CheckGoodMorale);
        WriteHiHook(0x04645B3, FASTCALL_, BattleStack_PositiveMoraleRandom);

        // negative morale code
        WriteHiHook(0x0464E63, THISCALL_, BattleMgr_CheckBadMorale);

        WriteHiHook(0x04647A7, FASTCALL_, BattleStack_NegativeMoraleRandom);
        // ATTENTION: AI has additional negative morale check that gives 25% chance to evade it;
        // return result for it must be reversed
        WriteHiHook(0x04647D0, FASTCALL_, AIBattleStack_NegativeMoraleRandom);

        // fear check
        WriteHiHook(0x0464E73, THISCALL_, BattleMgr_CheckFear);
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
