#include "framework.h"

#include "CreatureTurnControlRandom.h"

CreatureTurnControlRandom *CreatureTurnControlRandom::instance = nullptr;
CreatureTurnControlRandom::CreatureTurnControlRandom() : IGamePatch(globalPatcher->CreateInstance(instanceName))
{
    CreatePatches();
}

void __stdcall CreatureTurnControlRandom::BattleMgr_CheckGoodMorale(HiHook *h, const H3CombatManager *_this,
                                                                    const int side, const int index)
{

    instance->currentSettings = &CombatStackSettings::GetCombatStackSettings(side, index);
	instance->currentCreatureSide = side;
    THISCALL_3(void, h->GetDefaultFunc(), _this, side, index);
    instance->currentSettings = nullptr;
}

int __stdcall CreatureTurnControlRandom::BattleStack_PositiveMoraleRandom(HiHook *hook, const int min, const int max)
{



    const auto* luckSetting = &instance->currentSettings->At(STACK_SETTING_POSITIVE_MORALE);
    int pointsToDecrease = 1;
    if (luckSetting->triggerState == TRIGGER_STATE_DEFAULT)
    {
        const int side = instance->currentCreatureSide;
        luckSetting = &CombatSideSettings::GetCombatSideSettings(side).At(SIDE_SETTING_UNAFFECTED_BY_MORALE);
        pointsToDecrease = 2;
    }
    //if (luckSetting->Activate())
    {
        CombatSettingsManager::DecreaseUserPoints(pointsToDecrease);
    }

    return CombatStackSettings::BattleStack_Random(hook, min, max, *luckSetting);


    return CombatStackSettings::BattleStack_Random(
        hook, min, max, instance->currentSettings->At(eStackSettingsId::STACK_SETTING_POSITIVE_MORALE));
}

int __stdcall CreatureTurnControlRandom::BattleMgr_CheckBadMorale(HiHook *h, const H3CombatManager *_this,
                                                                  const int side, const int index)
{
    instance->currentSettings = &CombatStackSettings::GetCombatStackSettings(side, index);
    int result = THISCALL_3(int, h->GetDefaultFunc(), _this, side, index);
    instance->currentSettings = nullptr;
    return result;
}
int __stdcall CreatureTurnControlRandom::BattleStack_NegativeMoraleRandom(HiHook *hook, const int min, const int max)
{

    return CombatStackSettings::BattleStack_Random(
        hook, min, max, instance->currentSettings->At(eStackSettingsId::STACK_SETTING_NEGATIVE_MORALE));
}

int __stdcall CreatureTurnControlRandom::AIBattleStack_NegativeMoraleRandom(HiHook *hook, const int min, const int max)
{
    if (instance->currentSettings)
    {
        switch (instance->currentSettings->At(STACK_SETTING_NEGATIVE_MORALE).triggerState)
        {
        case eTriggerState::TRIGGER_STATE_ALWAYS:
            return max; // always trigger ability
        case eTriggerState::TRIGGER_STATE_NEVER:
            return min; // never trigger ability
        default:
            break; // return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
        }
    }

    return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
}

char __stdcall CreatureTurnControlRandom::BattleMgr_CheckFear(HiHook *h, const H3CombatManager *_this,
                                                              const H3CombatCreature *creature)
{

    instance->currentSettings = &CombatStackSettings::GetCombatStackSettings(creature);
    char result = THISCALL_2(char, h->GetDefaultFunc(), _this, creature);
    instance->currentSettings = nullptr;
    return result;
}

int __stdcall CreatureTurnControlRandom::BattleStack_FearRandom(HiHook *hook, const int min, const int max)
{
    return CombatStackSettings::BattleStack_Random(hook, min, max, instance->currentSettings->At(STACK_SETTING_FEAR));
}

void CreatureTurnControlRandom::CreatePatches()
{
    if (!this->m_isInited)
    {
        this->m_isInited = true;
        this->m_isEnabled = true;

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

CreatureTurnControlRandom &CreatureTurnControlRandom::GetInstance()
{
    if (!instance)
    {
        instance = new CreatureTurnControlRandom();
    }
    return *instance;
}
