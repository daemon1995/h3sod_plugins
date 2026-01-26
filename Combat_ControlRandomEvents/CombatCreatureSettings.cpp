#include "framework.h"

int CombatCreatureSettings::BattleStack_Random(HiHook *hook, const int min, const int max,
                                               const AbilityChanger &abilityChanger)
{
    switch (abilityChanger.triggerState)
    {
    case eTriggerState::ALWAYS:
        return min; // always trigger ability
    case eTriggerState::NEVER:
        return max; // never trigger ability
    default:
        return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
    }
}
