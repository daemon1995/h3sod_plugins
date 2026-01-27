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

BOOL CombatCreatureSettings::IsAffected(const eSettingsId id, const H3CombatCreature *creature) const
{
    if (!creature && ownerCreature)
        creature = ownerCreature;

    if (!creature)
        return FALSE;

    const auto &info = creature->info;
    const int creatureType = creature->type;
    //    const AbilityChanger &ability = asArray[id];
    switch (id)
    {
    case POSITIVE_MORALE:
        if (P_CombatManager->specialTerrain == 2)
        {
            return false;
        }
        else
        {
            for (auto &hero : P_CombatManager->hero)
            {
                if (hero && hero->WearsArtifact(eArtifact::SPIRIT_OF_OPPRESSION))
                {
                    return false;
                }
            }
        }

    case NEGATIVE_MORALE:
        return !info.noMorale; // || creature->info.undead;
    case FEAR:
        return !(info.noMorale || creatureType == eCreature::AZURE_DRAGON); // || creature->info.undead;

    case SPELL_CASTING:
        return creatureType == eCreature::MASTER_GENIE || creatureType == eCreature::ENCHANTER ||
               creatureType == eCreature::FAERIE_DRAGON;

    case RESURRECTION:
        return creatureType == eCreature::PHOENIX;
    case RESISTANCE:
        return false;
    case POSITIVE_LUCK:
        if (P_CombatManager->specialTerrain == 2)
        {
            return false;
        }
        else
        {
            for (auto &hero : P_CombatManager->hero)
            {
                if (hero && hero->WearsArtifact(eArtifact::HOURGLASS_OF_THE_EVIL_HOUR))
                {
                    return false;
                }
            }
        }
        return true;
    case DOUBLE_DAMAGE:
        return creatureType == eCreature::DREAD_KNIGHT;
    case WALL_ATTACK_AIM:
        return info.destroyWalls;
    case AFTER_ATTACK_ABILITY:
        return CreatureAttackRandom::BattleStack_HasAfterAttackAbility(creature);
    case DAMAGE:
        return info.damageLow < info.damageHigh;
    default:
        break;
    }
    //   return

    return false;
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
