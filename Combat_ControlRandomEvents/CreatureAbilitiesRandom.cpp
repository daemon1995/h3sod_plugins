#include "framework.h"

CreatureAbilitiesRandom *CreatureAbilitiesRandom::instance = nullptr;
CreatureAbilitiesRandom::CreatureAbilitiesRandom() : IGamePatch(_PI)
{
    CreatePatches();
}

int __stdcall CreatureAbilitiesRandom::BattleStack_AfterAttackAbilityRandom(HiHook *hook, const int min, const int max)
{

    const auto &map = instance->m_abilitiesMap;
    const auto &handler = map.find(instance->currentCreatureId);
    if (handler != map.end())
    {
        switch (handler->second.trigger)
        {
        case eAbilityTrigger::ALWAYS:
            return min; // always trigger ability
        case eAbilityTrigger::NEVER:
            return max; // never trigger ability
        default:
            break; // return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
        }
    }

    return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
}

char __stdcall CreatureAbilitiesRandom::BattleStack__SetAfterHitSpellAtTarget(HiHook *hook,
                                                                              const H3CombatCreature *attacker,
                                                                              const H3CombatCreature *defender)
{

    instance->currentCreatureId = eCreature(attacker->type);
    const char result = THISCALL_2(char, hook->GetDefaultFunc(), attacker, defender);
    instance->currentCreatureId = eCreature::UNDEFINED;
    return result;
}
void __stdcall CreatureAbilitiesRandom::BattleStack_AfterAttackAbility(HiHook *hook, const H3CombatCreature *attacker,
                                                                       const H3CombatCreature *defender,
                                                                       const int damageDealt, const int killed,
                                                                       const int a5)
{
    // store creature type before random function
    instance->currentCreatureId = eCreature(attacker->type);
    THISCALL_5(void, hook->GetDefaultFunc(), attacker, defender, damageDealt, killed, a5);
    instance->currentCreatureId = eCreature::UNDEFINED;
}

void CreatureAbilitiesRandom::CreateAbilityEvent(const eCreature creature, const DWORD patchAddress,
                                                 const void *functionPtr, const eAbilityTrigger trigger, eVKey hotkey)
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

void CreatureAbilitiesRandom::CreatePatches()
{
    if (!this->m_isInited)
    {
        this->m_isInited = true;

		// set after hit spell at target
        WriteHiHook(0x440220, THISCALL_, CreatureAbilitiesRandom::BattleStack__SetAfterHitSpellAtTarget);


        // blind
        CreateAbilityEvent(eCreature::UNICORN, 0x440337, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);
        CreateAbilityEvent(eCreature::WAR_UNICORN, 0x440337, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);
        
        // desease
        CreateAbilityEvent(eCreature::ZOMBIE, 0x4402D2, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);

        // curse
        CreateAbilityEvent(eCreature::BLACK_KNIGHT, 0x44042C, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);
        CreateAbilityEvent(eCreature::DREAD_KNIGHT, 0x44042C, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);
        CreateAbilityEvent(eCreature::MUMMY, 0x44042C, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);
        
        // age
        CreateAbilityEvent(eCreature::GHOST_DRAGON, 0x44025D, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);
        
        
        // stone gaze
        CreateAbilityEvent(eCreature::MEDUSA, 0x4404A0, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);
        CreateAbilityEvent(eCreature::MEDUSA_QUEEN, 0x4404A0, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);
        CreateAbilityEvent(eCreature::BASILISK, 0x4404A0, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);
        CreateAbilityEvent(eCreature::GREATER_BASILISK, 0x4404A0, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);
        
        // paralize
        CreateAbilityEvent(eCreature::SCORPICORE, 0x4405CA, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);

        // poison
        CreateAbilityEvent(eCreature::WYVERN_MONARCH, 0x440559, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);


        // set after attack abilities patches
        WriteHiHook(0x4408E0, THISCALL_, CreatureAbilitiesRandom::BattleStack_AfterAttackAbility);


		// thunderbolt
        CreateAbilityEvent(eCreature::THUNDERBIRD, 0x440EBD, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);

        // death stare
        CreateAbilityEvent(eCreature::MIGHTY_GORGON, 0x440C00, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);

        // acid breath
        CreateAbilityEvent(eCreature::RUST_DRAGON, 0x4411D7, BattleStack_AfterAttackAbilityRandom, eAbilityTrigger::ALWAYS);

        

    }
}
CreatureAbilitiesRandom &CreatureAbilitiesRandom::GetInstance()
{
    if (!instance)
    {
        instance = new CreatureAbilitiesRandom();
    }

    return *instance;
}
