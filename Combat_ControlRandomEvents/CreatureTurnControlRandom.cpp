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

    return CombatStackSettings::BattleStack_ContinuousRandom(instance->currentSettings->creature,
                                                             STACK_SETTING_POSITIVE_MORALE,
                                                             SIDE_SETTING_UNAFFECTED_BY_MORALE, hook, min, max);
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

    return CombatStackSettings::BattleStack_ContinuousRandom(instance->currentSettings->creature,
                                                             STACK_SETTING_NEGATIVE_MORALE,
                                                             SIDE_SETTING_UNAFFECTED_BY_MORALE, hook, min, max);
}

int __stdcall CreatureTurnControlRandom::AIBattleStack_NegativeMoraleRandom(HiHook *hook, const int min, const int max)
{
    if (instance->currentSettings)
    {
        const int side = instance->currentSettings->creature->side;
        switch (instance->currentSettings->At(STACK_SETTING_NEGATIVE_MORALE).triggerState)
        {
        case eTriggerState::TRIGGER_STATE_ALWAYS:
            return max; // always trigger bad morale
        case eTriggerState::TRIGGER_STATE_NEVER:
            return min; // never trigger bad morale
        default:
            if (CombatSideSettings::GetCombatSideSettings(side).At(SIDE_SETTING_UNAFFECTED_BY_MORALE).triggerState ==
                TRIGGER_STATE_ENABLED) // if morale setting is not set for stack, then check if it is set for side, and
                                       // trigger it if it is
            {
                return min; // never trigger bad morale
            }

            break; // return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
        }
    }

    return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
}

_LHF_(CreatureTurnControlRandom::BattleMgr_CheckFear)
{

    auto combatCreature = ValueAt<H3CombatCreature *>(c->ebp + 0x8);
    auto &stackSettings = CombatStackSettings::GetCombatStackSettings(combatCreature);
    Ability settings = stackSettings.At(STACK_SETTING_FEAR);
    if (settings.triggerState == TRIGGER_STATE_DISABLED)
    {
        const int side = combatCreature->side;
        auto &sideSettings = CombatSideSettings::GetCombatSideSettings(side);
        auto &sideSetting = sideSettings.At(SIDE_SETTING_UNAFFECTED_BY_FEAR);
        if (sideSetting.triggerState == TRIGGER_STATE_ENABLED)
        {
            settings = sideSetting;
            settings.triggerState = TRIGGER_STATE_NEVER; // if side is unaffected by, then stack is also
            // unaffected by, so set it to never
            sideSettings.TriggerAbility(SIDE_SETTING_UNAFFECTED_BY_FEAR);
            c->return_address = 0x04649E2;
            return NO_EXEC_DEFAULT;
        }
    }
    else
    {
        switch (settings.triggerState)
        {
        case TRIGGER_STATE_ALWAYS:
            c->return_address = 0x04649ED;
            break;
        case TRIGGER_STATE_NEVER:
            c->return_address = 0x04649E2;
            break;
        case TRIGGER_STATE_DEFAULT:
        default:
            return EXEC_DEFAULT;
        }
        stackSettings.TriggerAbility(STACK_SETTING_FEAR);
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

int __stdcall CreatureTurnControlRandom::BattleStack_FearRandom(HiHook *hook, const int min, const int max)
{

    return CombatStackSettings::BattleStack_ContinuousRandom(instance->currentSettings->creature, STACK_SETTING_FEAR,
                                                             SIDE_SETTING_UNAFFECTED_BY_FEAR, hook, min, max);
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
        WriteLoHook(0x04649D1, BattleMgr_CheckFear);
        // WriteHiHook(0x0464E73, THISCALL_, BattleMgr_CheckFear);
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
