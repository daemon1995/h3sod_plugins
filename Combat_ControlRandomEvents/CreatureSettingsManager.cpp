#include "framework.h"

CreatureSettingsManager *CreatureSettingsManager::instance = nullptr;

CreatureSettingsManager::CreatureSettingsManager() : IGamePatch(_PI)
{
    // Initialize all creature settings to default
    CreatePatches();
    ResetCombatSettings();
}

void CreatureSettingsManager::ResetCombatSettings() noexcept
{
    libc::memset(combatCreatureSettings, 0, sizeof(combatCreatureSettings));

    combatIsStarted = false;
    tacticsPhaseRound = false;
    userControlPoints = 0;
    userControlPointsSpent = 0;
    userActionsUsed = 0;

    actionsUsedLog.Init();
}

CreatureSettingsManager &CreatureSettingsManager::GetInstance()
{
    if (!instance)
        instance = new CreatureSettingsManager();
    else
        instance = {};
    return *instance;
}

void TestInitiate(CreatureSettingsManager *instance)
{

    CombatCreatureSettings tempAttacker;
    tempAttacker.positiveMorale.triggerState = eTriggerState::ALWAYS;
    tempAttacker.afterAttackAbility.triggerState = eTriggerState::ALWAYS;
    tempAttacker.damage.triggerState = eDamageState::DAMAGE_MIN_100;
    tempAttacker.doubleDamage.triggerState = eTriggerState::ALWAYS;
    tempAttacker.positiveLuck.triggerState = eTriggerState::ALWAYS;
    CombatCreatureSettings tempDefender;
    tempDefender.negativeMorale.triggerState = eTriggerState::ALWAYS;

    for (size_t i = 0; i <= h3::limits::COMBAT_CREATURES; i++)
    {
        instance->SetCreatureSettings(0, i, tempAttacker);
        instance->SetCreatureSettings(1, i, tempDefender);
    }
}

void __stdcall CreatureSettingsManager::BattleMgr_StartBattle(HiHook *h, H3CombatManager *_this)
{

    THISCALL_1(void, h->GetDefaultFunc(), _this);
    instance->ResetCombatSettings();

    instance->combatIsStarted = false;
    instance->tacticsPhaseRound = _this->tacticsPhase;

    if (_this->tacticsPhase)
    {
        libc::sprintf(h3_TextBuffer, "Creature Settings Manager: New Round %d", _this->turn);
        H3Messagebox(h3_TextBuffer);
    }
    else
    {
        instance->combatIsStarted = true;
        TestInitiate(instance);
    }
}

// isn't executed if combat doesn't have tactics phase
void __stdcall CreatureSettingsManager::BattleMgr_NewRound(HiHook *h, H3CombatManager *_this)
{

    THISCALL_1(void, h->GetDefaultFunc(), _this);

    if (instance->combatIsStarted)
    {
        TestInitiate(instance);
        libc::sprintf(h3_TextBuffer, "Creature Settings Manager: New Round %d", _this->turn);
        _this->AddStatusMessage(h3_TextBuffer);
        //  H3Messagebox("instance->combatIsStarted");
        if (_this->turn + 1 - instance->tacticsPhaseRound == 2)
        {
        }
    }
    else if (!_this->tacticsPhase)
    {
        instance->combatIsStarted = true;
    }
}

const CombatCreatureSettings &CreatureSettingsManager::GetCreatureSettings(const H3CombatCreature *creature) noexcept
{
    return instance->combatCreatureSettings[creature->side][creature->sideIndex];
}

const CombatCreatureSettings &CreatureSettingsManager::GetCreatureSettings(const int side, const int index) noexcept
{
    return instance->combatCreatureSettings[side][index];
}
void CreatureSettingsManager::SetCreatureSettings(const H3CombatCreature *creature,
                                                  const CombatCreatureSettings &settings) noexcept
{
    instance->combatCreatureSettings[creature->side][creature->sideIndex] = settings;
}

void CreatureSettingsManager::SetCreatureSettings(const int side, const int index,
                                                  const CombatCreatureSettings &settings) noexcept
{
    instance->combatCreatureSettings[side][index] = settings;
}

int CreatureSettingsManager::GetUserPoints() noexcept
{
    return instance->userControlPoints;
}

void CreatureSettingsManager::SetUserPoints(const int newSize) noexcept
{
    instance->userControlPoints = newSize;
}

BOOL CreatureSettingsManager::DecreaseUserPoints(const int toDecrease) noexcept
{

    if (instance->userControlPoints >= toDecrease)
    {
        instance->userControlPoints -= toDecrease;
        instance->userControlPointsSpent += toDecrease;
        return true;
    }

    return false;
}

// Implementation of patch creation
void CreatureSettingsManager::CreatePatches()
{
    if (!m_isInited)
    {
        m_isInited = true;

        WriteHiHook(0x0475800, THISCALL_, BattleMgr_NewRound);
        WriteHiHook(0x0462C8A, THISCALL_, BattleMgr_StartBattle);
    }
}
