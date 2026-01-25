#include "framework.h"

CreatureAttackRandom *CreatureAttackRandom::instance = nullptr;
CreatureAttackRandom::CreatureAttackRandom() : IGamePatch(_PI)
{
    CreatePatches();
}

int __stdcall CreatureAttackRandom::CombatCreature_DamageRandom(HiHook *h, const int min, const int max)
{

    if (instance->currentSettings)
    {
        switch (instance->currentSettings->damage)
        {
        case eDamageState::DAMAGE_DEFAULT:
            break;
        case eDamageState::DAMAGE_MINIMAL:
            return min;
        case eDamageState::DAMAGE_MAXIMAL:
            return max;
        case eDamageState::DAMAGE_MIN_25:
            return min + ((max - min) >> 2);
        case eDamageState::DAMAGE_MIN_50:
            return min + ((max - min) >> 1);
        case eDamageState::DAMAGE_MIN_75:
            return min + (((max - min) * 3) >> 2);
        default:
            break;
        }
    }

    // return default behavior
    return FASTCALL_2(int, h->GetDefaultFunc(), min, max);
}



int __stdcall CreatureAttackRandom::BattleStack_AfterAttackAbilityRandom(HiHook *hook, const int min, const int max)
{

    if (instance->currentSettings)
    {
        switch (instance->currentSettings->afterAttackAbility)
        {
        case eTriggerState::ALWAYS:
            return min; // always trigger ability
        case eTriggerState::NEVER:
            return max; // never trigger ability
        default:
            break; // return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
        }
    }


    return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
}

void __stdcall CreatureAttackRandom::BattleStack_Shoot(HiHook *hook, const H3CombatCreature *attacker,
                                                       const H3CombatCreature *defender)
{
    instance->currentSettings = &CreatureSettingsManager::GetCreatureSettings(attacker);
    THISCALL_2(void, hook->GetDefaultFunc(), attacker, defender);
    instance->currentSettings = nullptr;
}
char __stdcall CreatureAttackRandom::BattleStack_AttackMelee(HiHook *hook, const H3CombatCreature *attacker,
                                                             const H3CombatCreature *defender, const int direction)
{
    // store creature type before random function
    instance->currentSettings = &CreatureSettingsManager::GetCreatureSettings(attacker);
    const char result = THISCALL_3(char, hook->GetDefaultFunc(), attacker, defender, direction);
    instance->currentSettings = nullptr;
    return result;
}

void CreatureAttackRandom::CreateAbilityEvent(const eCreature creature, const DWORD patchAddress,
                                              const void *functionPtr, const eTriggerState trigger, eVKey hotkey)
{

    if (creature != eCreature::UNDEFINED, patchAddress, functionPtr)
    {
        m_abilitiesMap.insert(
            std::make_pair(creature,
                           EventHandler{
                               true,
                               1,
                               20,
                               trigger,
                               _pi->WriteHiHook(patchAddress, FASTCALL_, BattleStack_AfterAttackAbilityRandom),

                           }

                           ));
    }
}

void CreatureAttackRandom::CreatePatches()
{
    if (!this->m_isInited)
    {
        this->m_isInited = true;

        // used to init currentCreature and currentSettings before and after attack functions
        WriteHiHook(0x43F620, THISCALL_, BattleStack_Shoot);
        WriteHiHook(0x441330, THISCALL_, BattleStack_AttackMelee);

        // physical damage control
        // the game has two places where it calculates damage for physical attacks
        WriteHiHook(0x443024, FASTCALL_, CombatCreature_DamageRandom);
        WriteHiHook(0x442FE9, FASTCALL_, CombatCreature_DamageRandom);

        // set after attack abilities patches
        // blind
        CreateAbilityEvent(eCreature::UNICORN, 0x440337, BattleStack_AfterAttackAbilityRandom);
        CreateAbilityEvent(eCreature::WAR_UNICORN, 0x440337, BattleStack_AfterAttackAbilityRandom);

        // desease
        CreateAbilityEvent(eCreature::ZOMBIE, 0x4402D2, BattleStack_AfterAttackAbilityRandom);

        // curse
        CreateAbilityEvent(eCreature::BLACK_KNIGHT, 0x44042C, BattleStack_AfterAttackAbilityRandom);
        CreateAbilityEvent(eCreature::DREAD_KNIGHT, 0x44042C, BattleStack_AfterAttackAbilityRandom);
        CreateAbilityEvent(eCreature::MUMMY, 0x44042C, BattleStack_AfterAttackAbilityRandom);

        // age
        CreateAbilityEvent(eCreature::GHOST_DRAGON, 0x44025D, BattleStack_AfterAttackAbilityRandom,
                           eTriggerState::ALWAYS);

        // stone gaze
        CreateAbilityEvent(eCreature::MEDUSA, 0x4404A0, BattleStack_AfterAttackAbilityRandom);
        CreateAbilityEvent(eCreature::MEDUSA_QUEEN, 0x4404A0, BattleStack_AfterAttackAbilityRandom);
        CreateAbilityEvent(eCreature::BASILISK, 0x4404A0, BattleStack_AfterAttackAbilityRandom);
        CreateAbilityEvent(eCreature::GREATER_BASILISK, 0x4404A0, BattleStack_AfterAttackAbilityRandom);

        // paralize
        CreateAbilityEvent(eCreature::SCORPICORE, 0x4405CA, BattleStack_AfterAttackAbilityRandom);

        // poison
        CreateAbilityEvent(eCreature::WYVERN_MONARCH, 0x440559, BattleStack_AfterAttackAbilityRandom);

        // set after hit spells patches
        // thunderbolt
        CreateAbilityEvent(eCreature::THUNDERBIRD, 0x440EBD, BattleStack_AfterAttackAbilityRandom);
        // death stare
        CreateAbilityEvent(eCreature::MIGHTY_GORGON, 0x440C00, BattleStack_AfterAttackAbilityRandom);

        // acid breath
        CreateAbilityEvent(eCreature::RUST_DRAGON, 0x4411D7, BattleStack_AfterAttackAbilityRandom);
    }
}

CreatureAttackRandom &CreatureAttackRandom::GetInstance()
{
    if (!instance)
    {
        instance = new CreatureAttackRandom();
    }

    return *instance;
}
