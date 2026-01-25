#include "framework.h"

CreatureSettingsManager *CreatureSettingsManager::instance = nullptr;

CreatureSettingsManager::CreatureSettingsManager() : IGamePatch(_PI)
{
    // Initialize all creature settings to default
    CreatePatches();
    ResetAllCreatureSettings();
}

void CreatureSettingsManager::ResetAllCreatureSettings() noexcept
{
    libc::memset(combatCreatureSettings, 0, sizeof(combatCreatureSettings));
}

CreatureSettingsManager &CreatureSettingsManager::GetInstance()
{
    if (!instance)
        instance = new CreatureSettingsManager();
    else
        instance = {};
    return *instance;
}

void __stdcall CreatureSettingsManager::BattleMgr_StartBattle(HiHook *h, H3CombatManager *_this)
{

    THISCALL_1(void, h->GetDefaultFunc(), _this);
    instance->combatIsStarted = false;
	instance->ResetAllCreatureSettings();
    instance->tacticsPhaseRound = _this->tacticsPhase;

    if (_this->tacticsPhase)
    {
        libc::sprintf(h3_TextBuffer, "Creature Settings Manager: New Round %d", _this->turn);
        //  H3Messagebox(h3_TextBuffer);
    }
}

void __stdcall CreatureSettingsManager::BattleMgr_NewRound(HiHook *h, H3CombatManager *_this)
{

    THISCALL_1(void, h->GetDefaultFunc(), _this);
    // if (!instance->combatIsStarted && _this->tacticsPhase)
    //{
    //     instance->combatIsStarted = true;
    // }
    if (instance->combatIsStarted)
    {
        libc::sprintf(h3_TextBuffer, "Creature Settings Manager: New Round %d", _this->turn);
        _this->AddStatusMessage(h3_TextBuffer);

        if (_this->turn + 1 - instance->tacticsPhaseRound == 2)
        {
            for (size_t i = 0; i < h3::limits::COMBAT_CREATURES; i++)
            {
                instance->combatCreatureSettings[0][i].positiveMorale = eTriggerState::ALWAYS;
                instance->combatCreatureSettings[0][i].afterAttackAbility = eTriggerState::ALWAYS;
                instance->combatCreatureSettings[0][i].damage = eDamageState::DAMAGE_MIN_50;
            }
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
