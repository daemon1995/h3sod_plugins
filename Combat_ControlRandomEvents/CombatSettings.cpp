#include "framework.h"

// Forward declarations
struct CreatureAttackRandom
{
    static BOOL CreatureAttackRandom::BattleStack_HasAfterAttackAbility(const H3CombatCreature *creature);
};
struct CreatureSpellData
{
  public:
    static BOOL CreateAvailableSpellsList(const H3CombatCreature *creature, std::vector<eSpell> &outList);
};

// Static member definitions
CombatStackSettings CombatStackSettings::combatStackSettings[2][h3::limits::COMBAT_CREATURES + 1]{};
CombatSideSettings CombatSideSettings::sideSettings[2]{};

// Method implementations
void CombatStackSettings::DecreaseDurations(const eStackSettingsId id)
{

    // for (auto &i : asArray)
    //{
    //     if (i.duration > 0)
    //     {
    //         i.duration--;
    //     }

    //    if (i.duration == 0)
    //    {
    //        i.duration = -1;
    //        i.triggerState = TRIGGER_STATE_DEFAULT;
    //    }
    //}
}

BOOL CombatStackSettings::IsAffectedBySetting(const eStackSettingsId id) const
{
    if (!creature)
        return FALSE;
    if (creature->numberAlive < 1 && creature->type == eCreature::ARROW_TOWER)
        return FALSE;

    const auto &info = creature->info;
    const int creatureType = creature->type;
    const auto &owner = creature->GetOwner();
    //    const AbilityState &ability = asArray[id];
    switch (id)
    {
    case STACK_SETTING_POSITIVE_MORALE:

        for (auto &hero : P_CombatManager->hero)
        {
            if (hero && hero->WearsArtifact(eArtifact::SPIRIT_OF_OPPRESSION))
            {
                return false;
            }
        }
        return !(P_CombatManager->specialTerrain == 2 || info.noMorale);
    case STACK_SETTING_NEGATIVE_MORALE:
        return !info.noMorale; // || creature->info.undead;
    case STACK_SETTING_FEAR:
        return !(info.noMorale || creatureType == eCreature::AZURE_DRAGON); // || creature->info.undead;
    case STACK_SETTING_SPELL_CASTING:
        return creatureType == eCreature::ENCHANTER ||
               (creatureType == eCreature::MASTER_GENIE || creatureType == eCreature::FAERIE_DRAGON) &&
                   creature->info.spellCharges > 0;

    case STACK_SETTING_RESURRECTION:
        return creatureType == eCreature::PHOENIX && creature->info.spellCharges > 0;
    case STACK_SETTING_MAGIC_RESISTANCE:
        switch (creatureType)
        {
        case eCreature::DWARF:
        case eCreature::CRYSTAL_DRAGON:
        case eCreature::BATTLE_DWARF:
            return true;
        default:
            return creature->HasUnicornsAura() || (owner && owner->GetResistancePower());
        }
        break;
    case STACK_SETTING_MAGIC_MIRROR:
        return creature->MagicMirrorEffect();
    case STACK_SETTING_POSITIVE_LUCK:
        for (auto &hero : P_CombatManager->hero)
        {
            if (hero && hero->WearsArtifact(eArtifact::HOURGLASS_OF_THE_EVIL_HOUR))
            {
                return false;
            }
        }
        return P_CombatManager->specialTerrain != 2;
    case STACK_SETTING_DOUBLE_DAMAGE:
        return creatureType == eCreature::DREAD_KNIGHT ||
               (creatureType == eCreature::BALLISTA && owner &&
                owner->secSkill[eSecondary::ARTILLERY]); // ballista's double damage
    case STACK_SETTING_WALL_ATTACK:
        return info.destroyWalls && P_CombatManager->siegeKind2 > 0;
    case STACK_SETTING_AFTER_ATTACK_ABILITY:
        return CreatureAttackRandom::BattleStack_HasAfterAttackAbility(creature);
    case STACK_SETTING_DAMAGE_VARIATION_FIRST:
        return info.damageLow < info.damageHigh;
    case STACK_SETTING_DAMAGE_VARIATION_SECOND:
        return info.doubleAttack && info.damageLow < info.damageHigh && (!info.shooter || info.numberShots > 1);
    default:
        break;
    }
    //   return

    return false;
}

AbilityState CombatStackSettings::GetNextAbilityState(const eStackSettingsId id) const
{

    const eTriggerState currentState = asArray[id].triggerState;

    AbilityState result{}; // { currentState, asArray[id].duration };
    switch (id)
    {

    case STACK_SETTING_SPELL_CASTING:
        result = GetNextSpellStateToCast();
        break;
        // bool on/off settings
    case STACK_SETTING_DAMAGE_INPUT:
        result.triggerState = currentState ? TRIGGER_STATE_DEFAULT : TRIGGER_STATE_ALWAYS;
        break;
        // settings with 3 states (default, always, never)
    default:
        if (currentState >= eTriggerState::TRIGGER_STATE_NEVER)
            result.triggerState = TRIGGER_STATE_DEFAULT;
        else
            result.triggerState = static_cast<eTriggerState>(currentState + 1);
        break;
    }

    return result;
}

AbilityState CombatStackSettings::GetNextSpellStateToCast() const noexcept
{

    if (IsAffectedBySetting(STACK_SETTING_SPELL_CASTING))
    {
        std::vector<eSpell> availableSpells;
        if (CreatureSpellData::CreateAvailableSpellsList(creature, availableSpells))
        {

            if (spellCasting.spellToCast == eSpell::NONE)
            {
                return {TRIGGER_STATE_ALWAYS, availableSpells.front()};
            }

            if (spellCasting.spellToCast == availableSpells.back())
            {
                return {TRIGGER_STATE_DEFAULT, eSpell::NONE};
            }
            const size_t length = availableSpells.size();
            for (size_t i = 0; i < length; i++)
            {
                if (this->spellCasting.spellToCast == availableSpells[i])
                {
                    return {TRIGGER_STATE_ALWAYS, availableSpells[i + 1]};
                }
            }
        }
    }

    return {TRIGGER_STATE_DEFAULT, eSpell::NONE};
}

int CombatStackSettings::BattleStack_Random(HiHook *hook, const int min, const int max,
                                            const AbilityState &abilityChanger)
{
    // if ability isn't duration based, decrease points per use

    // if (const int userPints = CombatSettingsManager::GetUserPoints())
    {
        switch (abilityChanger.triggerState)
        {
        case eTriggerState::TRIGGER_STATE_ALWAYS:
            CombatSettingsManager::DecreaseUserPoints(1);
            return min; // always trigger ability
        case eTriggerState::TRIGGER_STATE_NEVER:
            CombatSettingsManager::DecreaseUserPoints(1);
            return max; // never trigger ability
        default:
            break;
        }
    }

    return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
}

void CombatStackSettings::ResetAll()
{
    for (int side = 0; side < 2; side++)
    {
        for (int index = 0; index < h3::limits::COMBAT_CREATURES + 1; index++)
        {
            combatStackSettings[side][index].Reset();
            if (P_CombatManager)
            {
                combatStackSettings[side][index].creature = &P_CombatManager->stacks[side][index];
            }
        }
    }
}
BOOL CombatSideSettings::IsAffectedBySetting(const eSideSettingsId id) const
{
    float maxSideResistance = 0.f;
    switch (id)
    {
    case SIDE_SETTING_UNAFFECTED_BY_MORALE:
        for (auto &stack : P_CombatManager->stacks[side])
        {
            if (CombatStackSettings::GetCombatStackSettings(&stack).IsAffectedBySetting(
                    STACK_SETTING_POSITIVE_MORALE) ||
                CombatStackSettings::GetCombatStackSettings(&stack).IsAffectedBySetting(STACK_SETTING_NEGATIVE_MORALE))
            {
                return true;
            }
        }
        break;
    case SIDE_SETTING_UNAFFECTED_BY_LUCK:
        for (auto &stack : P_CombatManager->stacks[side])
        {
            if (CombatStackSettings::GetCombatStackSettings(&stack).IsAffectedBySetting(STACK_SETTING_POSITIVE_LUCK))
            {
                return true;
            }
        }
        break;
    case SIDE_SETTING_UNAFFECTED_BY_FEAR:
        for (auto &stack : P_CombatManager->stacks[side])
        {
            if (CombatStackSettings::GetCombatStackSettings(&stack).IsAffectedBySetting(STACK_SETTING_FEAR))
            {
                return true;
            }
        }
        break;
    case SIDE_SETTING_UNAFFECTED_BY_RESISTANCE:
        maxSideResistance = GetSideMaxResistance();
        return maxSideResistance > 0.f && maxSideResistance < 1.f;
    default:
        break;
    }
    return false;
}
float CombatSideSettings::GetSideMaxResistance() const
{

    auto &stacks = P_CombatManager->stacks[side];

    float result = 1.f;

    const auto &hero = P_CombatManager->hero[side];

    float heroResistance = hero ? hero->GetResistancePower() : 0.f;

    for (auto &stack : stacks)
    {

        if (stack.numberAlive < 1 || stack.activeSpellDuration[eSpell::HYPNOTIZE])
            continue;

        float spellSuccess = 1.f; // = stack.type.magicResistance;
        switch (stack.type)
        {
        case eCreature::DWARF:
        case eCreature::CRYSTAL_DRAGON:
            spellSuccess = 0.8f;
            break;
        case eCreature::BATTLE_DWARF:
            spellSuccess = 0.6f;
            break;
        default:
            break;
        }

        if (hero)
        {
            spellSuccess = spellSuccess - (1.f - heroResistance);
        }
        if (stack.HasUnicornsAura())
        {
            spellSuccess *= 0.8f;
        }
        if (result > spellSuccess)
        {
            result = spellSuccess;
        }
    }
    return 1.f - result;
}
void CombatSideSettings::ResetAll()
{
    for (int side = 0; side < 2; side++)
    {
        sideSettings[side] = {};
        sideSettings[side].side = side;
    }
}
