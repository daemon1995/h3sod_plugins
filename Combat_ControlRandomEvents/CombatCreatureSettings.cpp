#include "framework.h"

CombatCreatureSettings::CombatCreatureSettings()
{
}

void CombatCreatureSettings::ResetSettings()
{
    libc::memset(this, 0, sizeof(CombatCreatureSettings));
}

void CombatCreatureSettings::DecreaseDurations()
{

    for (auto &i : asArray)
    {
        if (i.duration > 0)
        {
            i.duration--;
        }
        if (i.duration == 0)
        {
            i.triggerState = DEFAULT;
        }
    }
}

const AbilityChanger &CombatCreatureSettings::At(const eSettingsId id) const
{
    return asArray[id];
}

AbilityChanger &CombatCreatureSettings::operator[](const eSettingsId id)
{
    return asArray[id];
}

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
