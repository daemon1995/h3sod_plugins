#include "framework.h"

CombatSettingsManager *CombatSettingsManager::instance = nullptr;

#define RANDOM_HANDLER_DECLARATION(className)                                                                          \
    class className : public IGamePatch                                                                                \
    {                                                                                                                  \
      public:                                                                                                          \
        static className &GetInstance();                                                                               \
    };

RANDOM_HANDLER_DECLARATION(CreatureTurnControlRandom)
RANDOM_HANDLER_DECLARATION(CreatureAttackRandom)
RANDOM_HANDLER_DECLARATION(CreatureMagicRandom)

struct PluginText
{
    static PluginText &GetInstance();
    LPCSTR GetCreatureAbilitySwitchText(const H3CombatCreature *creature, const eStackAbility settingId,
                                        const Ability &changer,
                                        const eAbilityStateSwitchError errorType) const noexcept;
    LPCSTR GetSideAbilitySwitchText(const int side, const eSideAbility settingId, const Ability &changer,
                                    const eAbilityStateSwitchError errorType) const noexcept;
    LPCSTR GetAbilityTriggeredText(const CombatStackSettings *creatureSettings, const CombatSideSettings *sideSettings,
                                   const int settingId, const int pointsUsed) const noexcept;
    LPCSTR GetCreatureAbilitySwitchErrorText(const CombatStackSettings *creatureSettings, const int settingId,
                                             const eAbilityStateSwitchError errorType) const noexcept;
    LPCSTR GetSideAbilitySwitchErrorText(const CombatSideSettings *sideSettings, const int settingId,
                                         const eAbilityStateSwitchError errorType) const noexcept;

    LPCSTR GetSideAbilityCustomText(const eSideAbility settingId) const noexcept;
};
struct SpellSelectionDlg
{
    static eSpell ShowSpellSelectionDialog(H3CombatCreature *creature, const H3Msg *msg);
};

CombatSettingsManager::CombatSettingsManager() : IGamePatch(_PI)
{
    // Initialize all creature settings to default
    CreatePatches();
    ResetCombatSettings();
    pluginText = &PluginText::GetInstance();
}

void CombatSettingsManager::ResetCombatSettings() noexcept
{

    CombatStackSettings::ResetAll();
    CombatSideSettings::ResetAll();

    combatIsStarted = false;
    tacticsPhaseRound = false;
    cheaterFlagSet = false;
    userControlPoints = userMaxControlPoints;
    userControlPointsSpent = 0;
    userActionsUsed = 0;

    actionsUsedLog.Init();
}

void CombatSettingsManager::SwitchBattleStackAbilityByHotKey(H3CombatManager *mgr, H3Msg *msg)
{
    if (msg->IsKeyDown() && mgr->mouseCoord >= 0 && mgr->mouseCoord <= 186)
    {
        const H3CombatCreature *combatCreature = mgr->squares[mgr->mouseCoord].GetMonster();
        if (!combatCreature || combatCreature->type == eCreature::ARROW_TOWER)
            return;

        const CombatStackSettings *combatStackSettings = &CombatStackSettings::GetCombatStackSettings(combatCreature);
        const CombatSideSettings *combatSideSettings = &CombatSideSettings::GetCombatSideSettings(combatCreature);

        BOOL saveToLog = TRUE;
        BOOL affectedAllUnits = FALSE;
        eAbilityStateSwitchError errorType = ABILITY_SWITCH_NO_EFFECT;
        LPCSTR resultText = nullptr;
        eStackAbility stackSettingId = STACK_SETTING_NONE;
        eSideAbility sideSettingId = SIDE_SETTING_NONE;

        const bool shiftPressed = STDCALL_1(SHORT, PtrAt(0x63A294), VK_SHIFT) & 0x800;
        const bool ctrlPressed = STDCALL_1(SHORT, PtrAt(0x63A294), VK_CONTROL) & 0x800;

        float resultValue = 0.0f;
        std::string debugString;

        switch (msg->KeyPressed())
        {
        case eVKey::H3VK_B:
            if (shiftPressed)
            {

                resultValue = combatSideSettings->GetSideMaxResistance();
                libc::sprintf(h3_TextBuffer, "stack ptr  = %d/%d", combatCreature, combatStackSettings->creature);

                WriteMessageToLog(h3_TextBuffer, eLogTargetType::LOG_TYPE_SCREEN);
            }

            return;

        case eVKey::H3VK_G: // fear
            if (shiftPressed)
            {
                sideSettingId = SIDE_SETTING_UNAFFECTED_BY_FEAR;
                if (combatSideSettings->IsAffectedBySetting(sideSettingId))
                    errorType = ABILITY_SWITCH_SUCCESS;
            }
            else
            {
                stackSettingId = STACK_SETTING_FEAR;
                if (combatStackSettings->IsAffectedBySetting(stackSettingId))
                    errorType = ABILITY_SWITCH_SUCCESS;
            }
            break;
        case eVKey::H3VK_J: // resist

            if (shiftPressed)
            {
                sideSettingId = SIDE_SETTING_UNAFFECTED_BY_RESISTANCE;
                if (combatSideSettings->IsAffectedBySetting(sideSettingId))
                {
                    if (combatSideSettings->At(sideSettingId).triggerState == TRIGGER_STATE_DISABLED)
                    {
                        auto text = pluginText->GetSideAbilityCustomText(SIDE_SETTING_UNAFFECTED_BY_RESISTANCE);
                        if (!H3Messagebox::Choice(text))
                            return;
                    }

                    errorType = ABILITY_SWITCH_SUCCESS;
                }
            }
            else
            {
                stackSettingId = ctrlPressed ? STACK_SETTING_MAGIC_MIRROR : STACK_SETTING_MAGIC_RESISTANCE;
                if (combatStackSettings->IsAffectedBySetting(stackSettingId))
                    errorType = ABILITY_SWITCH_SUCCESS;
            }
            break;
        case eVKey::H3VK_K: // damage
            if (ctrlPressed)
            {
                stackSettingId = STACK_SETTING_DAMAGE_INPUT;
                if (combatStackSettings->IsAffectedBySetting(stackSettingId))
                    errorType = ABILITY_SWITCH_SUCCESS;
            }
            else
            {
                stackSettingId =
                    shiftPressed ? STACK_SETTING_DAMAGE_VARIATION_SECOND : STACK_SETTING_DAMAGE_VARIATION_FIRST;
                if (combatStackSettings->IsAffectedBySetting(stackSettingId))
                {
                    errorType = ABILITY_SWITCH_SUCCESS;
                }
            }
            break;
        case eVKey::H3VK_X: // after attack ability// spell casting// resurection// double damage (shift) // wall attack
                            // aim

            errorType = ABILITY_SWITCH_SUCCESS;
            if (shiftPressed)
            {

                if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_DOUBLE_DAMAGE))
                {
                    stackSettingId = STACK_SETTING_DOUBLE_DAMAGE;
                }
                else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_SPELL_CASTING))
                {
                    const eSpell selectedSpell = SpellSelectionDlg::ShowSpellSelectionDialog(
                        const_cast<H3CombatCreature *>(combatCreature), msg);
                    if (selectedSpell != eSpell::NONE)
                    {
                        combatStackSettings->SetCreatureAbilityState(combatCreature, STACK_SETTING_SPELL_CASTING,
                                                                     {TRIGGER_STATE_ALWAYS, selectedSpell, 1});
                        if (combatCreature->type == eCreature::FAERIE_DRAGON)
                        {
                            const_cast<H3CombatCreature *>(combatCreature)->faerieDragonSpell = selectedSpell;
                        }
                    }
                }
                else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_WALL_ATTACK_MINIMAL_DAMAGE))
                {
                    stackSettingId = STACK_SETTING_WALL_ATTACK_MINIMAL_DAMAGE;
                }
                else
                {
                    errorType = ABILITY_SWITCH_NO_ABILITY;
                }
            }
            else
            {
                if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_AFTER_ATTACK_ABILITY))
                {
                    stackSettingId = STACK_SETTING_AFTER_ATTACK_ABILITY;
                }
                else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_SPELL_CASTING))
                {
                    stackSettingId = STACK_SETTING_SPELL_CASTING;
                }
                else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_RESURRECTION))
                {
                    stackSettingId = STACK_SETTING_RESURRECTION;
                }
                else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_WALL_ATTACK_AIM))
                {
                    stackSettingId = STACK_SETTING_WALL_ATTACK_AIM;
                }
                else
                {
                    errorType = ABILITY_SWITCH_NO_ABILITY;
                }
            }

            break;
        case eVKey::H3VK_N: // luck (shift)
            if (ctrlPressed)
            {
                stackSettingId = STACK_SETTING_DOUBLE_LUCK;
                errorType = combatStackSettings->IsAffectedBySetting(stackSettingId) ? ABILITY_SWITCH_SUCCESS
                                                                                     : ABILITY_SWITCH_NO_EFFECT;
            }
            else if (shiftPressed)
            {
                stackSettingId = STACK_SETTING_NONE;
                sideSettingId = SIDE_SETTING_UNAFFECTED_BY_LUCK;
                if (!combatSideSettings->IsAffectedBySetting(sideSettingId))
                    errorType = ABILITY_SWITCH_NO_EFFECT;
            }
            else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_POSITIVE_LUCK) && combatCreature->luck > 0)
            {
                stackSettingId = STACK_SETTING_POSITIVE_LUCK;
                errorType = ABILITY_SWITCH_SUCCESS;
            }
            else
            {
                errorType = ABILITY_SWITCH_NO_EFFECT;
            }
            break;
        case eVKey::H3VK_M: // morale (shift)
            errorType = ABILITY_SWITCH_SUCCESS;
            stackSettingId = STACK_SETTING_POSITIVE_MORALE;

            if (shiftPressed)
            {
                stackSettingId = STACK_SETTING_NONE;
                sideSettingId = SIDE_SETTING_UNAFFECTED_BY_MORALE;
                if (!combatSideSettings->IsAffectedBySetting(sideSettingId))
                    errorType = ABILITY_SWITCH_NO_EFFECT;
            }
            else
            {
                if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_POSITIVE_MORALE) &&
                    combatCreature->morale > 0)
                {
                    stackSettingId = STACK_SETTING_POSITIVE_MORALE;
                }
                else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_NEGATIVE_MORALE) &&
                         combatCreature->morale < 0)
                {
                    stackSettingId = STACK_SETTING_NEGATIVE_MORALE;
                }
                else
                {
                    errorType = ABILITY_SWITCH_NO_EFFECT;
                }
            }

            break;
        default:
            return;
        }

        // copy current setting to modify

        if (stackSettingId == STACK_SETTING_NONE && sideSettingId == SIDE_SETTING_NONE)
        {
            return;
        }

        Ability nextAbilityState;

        if (errorType == ABILITY_SWITCH_SUCCESS)
        {
            errorType = stackSettingId != STACK_SETTING_NONE
                            ? combatStackSettings->SwitchToNextAbilityState(stackSettingId, nextAbilityState)
                            : combatSideSettings->SwitchToNextAbilityState(sideSettingId, nextAbilityState);
        }

        if (errorType != ABILITY_SWITCH_SUCCESS)
        {
            auto errorText =
                stackSettingId != STACK_SETTING_NONE
                    ? pluginText->GetCreatureAbilitySwitchErrorText(combatStackSettings, stackSettingId, errorType)
                    : pluginText->GetSideAbilitySwitchErrorText(combatSideSettings, sideSettingId, errorType);

            if (errorText)
                WriteMessageToLog(errorText, eLogTargetType::LOG_TYPE_SCREEN);
            return;
        }

        std::string resultHint;

        if (stackSettingId != STACK_SETTING_NONE)
        {
            resultHint =
                pluginText->GetCreatureAbilitySwitchText(combatCreature, stackSettingId, nextAbilityState, errorType);

            CombatStackSettings::SetCreatureAbilityState(combatCreature, stackSettingId, nextAbilityState);

            if (stackSettingId == STACK_SETTING_SPELL_CASTING && combatCreature->type == eCreature::FAERIE_DRAGON)
            {
                const_cast<H3CombatCreature *>(combatCreature)->faerieDragonSpell = nextAbilityState.spellToCast;
            }
        }
        else if (sideSettingId != SIDE_SETTING_NONE)
        {
            resultHint =
                pluginText->GetSideAbilitySwitchText(combatCreature->side, sideSettingId, nextAbilityState, errorType);
            if (!resultHint.empty())
            {
                CombatSideSettings::SetSideAbilityState(combatCreature, sideSettingId, nextAbilityState);
            }
        }
        if (!resultHint.empty())
        {
            WriteMessageToLog(resultHint.c_str(), eLogTargetType::LOG_TYPE_SCREEN);
        }
    }
}

void CombatSettingsManager::WriteMessageToLog(LPCSTR msg, const eLogTargetType logType)
{

    auto mgr = P_CombatManager->Get();
    switch (logType)
    {
    case eLogTargetType::LOG_TYPE_SCREEN:
        if (mgr->IsHiddenBattle())
            return;
        H3ScreenChat::Get()->Show(msg);
        break;
    case eLogTargetType::LOG_TYPE_BATTLE_LOG:
        mgr->AddStatusMessage(msg);
        return;
    case eLogTargetType::LOG_TYPE_BATTLE_HINT:
        mgr->AddStatusMessage(msg, false);
        break;
    default:
        break;
    }
}

void CombatSettingsManager::ReportActionUsage(const CombatStackSettings *creatureSettings,
                                              const CombatSideSettings *sideSettings, const int settingId,
                                              const int pointsUsed, const eLogTargetType logType)
{

    LPCSTR triggerText =
        instance->pluginText->GetAbilityTriggeredText(creatureSettings, sideSettings, settingId, pointsUsed);
    if (triggerText && triggerText)
    {
        WriteMessageToLog(triggerText, logType);
    }
}

void CombatSettingsManager::SaveActionUsageToLog(H3CombatManager *mgr, const CombatStackSettings *creatureSettings)
{
}

CombatSettingsManager &CombatSettingsManager::GetInstance()
{
    if (!instance)
        instance = new CombatSettingsManager();
    else
        instance = {};
    return *instance;
}

static void TestInitiate(CombatSettingsManager *instance)
{
    return;
    CombatStackSettings tempAttacker;
    tempAttacker.positiveMorale.triggerState = TRIGGER_STATE_ALWAYS;
    tempAttacker.afterAttackAbility.triggerState = TRIGGER_STATE_ALWAYS;
    tempAttacker.firstAttackDamage.triggerState = TRIGGER_STATE_ALWAYS;
    tempAttacker.secondAttackDamage.triggerState = TRIGGER_STATE_ALWAYS;
    tempAttacker.doubleDamage.triggerState = TRIGGER_STATE_ALWAYS;
    tempAttacker.positiveLuck.triggerState = TRIGGER_STATE_ALWAYS;
    tempAttacker.wallAttackAim.triggerState = TRIGGER_STATE_ALWAYS;
    const Ability tempAttackerAbility = {
        TRIGGER_STATE_ALWAYS,
    };
    CombatStackSettings tempDefender;
    tempDefender.negativeMorale.triggerState = TRIGGER_STATE_ALWAYS;

    for (size_t i = 0; i <= h3::limits::COMBAT_CREATURES; i++)
    {
        const auto &attackerStack = &P_CombatManager->stacks[0][i];
        CombatStackSettings::SetCreatureAbilityState(attackerStack, STACK_SETTING_POSITIVE_MORALE, tempAttackerAbility);
        CombatStackSettings::SetCreatureAbilityState(attackerStack, STACK_SETTING_AFTER_ATTACK_ABILITY,
                                                     tempAttackerAbility);

        const auto &defenderStack = &P_CombatManager->stacks[1][i];
        //  instance->SetCreatureAbilityState(defenderStack, STACK_SETTING_NEGATIVE_MORALE,
        //  tempAttackerAbility);
        CombatStackSettings::SetCreatureAbilityState(defenderStack, STACK_SETTING_RESURRECTION, tempAttackerAbility);

        //  instance->SetCombatStackSettings(0, i, tempAttacker);
        //  instance->SetCombatStackSettings(1, i, tempDefender);
    }
}

void __stdcall CombatSettingsManager::BattleMgr_StartBattle(HiHook *h, H3CombatManager *_this)
{

    THISCALL_1(void, h->GetDefaultFunc(), _this);

    // check if combat has human player

    IGamePatch *appliedPatches[] = {
        &CreatureTurnControlRandom::GetInstance(),
        &CreatureAttackRandom::GetInstance(),
        &CreatureMagicRandom::GetInstance(),
    };

    auto &newRoundPatch = instance->newRoundPatch;
    auto &endCombatPatch = instance->endCombatPatch;

    const BOOL combatHasHuman = _this->isHuman[0] || _this->isHuman[1];

    // if not a human player, disable all patches
    if (!combatHasHuman)
    {
        for (auto &i : appliedPatches)
        {
            if (i->IsEnabled())
                i->SetEnabled(false);
        }

        if (newRoundPatch->IsApplied())
            newRoundPatch->Undo();

        if (endCombatPatch->IsApplied())
            endCombatPatch->Undo();
    }
    // else, enable all patches and apply new round patch
    else
    {
        for (auto &i : appliedPatches)
        {
            if (!i->IsEnabled())
                i->SetEnabled(true);
        }

        if (!newRoundPatch->IsApplied())
            newRoundPatch->Apply();
        if (!endCombatPatch->IsApplied())
            endCombatPatch->Apply();
    }

    // if player is not a cheater
    //  if (!P_Game->isCheater)

    CombatUniqueInfo combatUniqueInfo = {
        _this->position, _this->mapitem, {_this->hero[0], _this->hero[1]}, P_Game->date.CurrentDay()};
    if (instance->combatUniqueInfo != combatUniqueInfo)
    {
        instance->isCheaterBeforeCombat = P_Game->isCheater;
        instance->combatUniqueInfo = combatUniqueInfo;
    }
    else
    {
        P_Game->isCheater = instance->isCheaterBeforeCombat;
    }

    instance->ResetCombatSettings();

    instance->tacticsPhaseRound = _this->tacticsPhase;

    if (_this->tacticsPhase)
    {
        //   libc::sprintf(h3_TextBuffer, "Creature Settings Manager: New Round %d", _this->turn);
        // H3Messagebox(h3_TextBuffer);
    }
    else
    {
        instance->combatIsStarted = true;
    }
}

int __stdcall CombatSettingsManager::BattleMgr_ProcessAction_KeyPressed(HiHook *h, H3CombatManager *_this, H3Msg *msg)
{
    int result = THISCALL_2(int, h->GetDefaultFunc(), _this, msg);

    if (_this->autoCombat || H3AutoSolo::Get() || _this->IsHiddenBattle() ||
        IntAt(0x698A3C)) // 698A3C is some action is in proc
    {
        return result;
    }

    instance->SwitchBattleStackAbilityByHotKey(_this, msg);

    return result;
}

// isn't executed if combat doesn't have tactics phase
void __stdcall CombatSettingsManager::BattleMgr_NewRound(HiHook *h, H3CombatManager *_this)
{

    THISCALL_1(void, h->GetDefaultFunc(), _this);

    if (instance->combatIsStarted)
    {

        //   TestInitiate(instance);
        // libc::sprintf(h3_TextBuffer, "Creature Settings Manager: New Round %d", _this->turn);
        // _this->AddStatusMessage(h3_TextBuffer);
        //  H3Messagebox("instance->combatIsStarted");
        if (_this->turn + 1 - instance->tacticsPhaseRound == 2)
        {
        }
        if (_this->tacticsPhase)
        {
            return;
        }
        CombatStackSettings::HandleNewCombatRound();
        CombatSideSettings::HandleNewCombatRound();
    }
    else if (!_this->tacticsPhase)
    {
        instance->combatIsStarted = true;
    }
}

void __stdcall CombatSettingsManager::BattleMgr_SetWinner(HiHook *h, H3CombatManager *_this, const INT side)
{
    THISCALL_2(void, h->GetDefaultFunc(), _this, side);
    if (instance->cheaterFlagSet)
    {
        P_Game->isCheater = true;
    }

    instance->ResetCombatSettings();
}

void CombatSettingsManager::SetUserPoints(const int newSize) noexcept
{
    instance->userControlPoints = newSize;
}

BOOL CombatSettingsManager::DecreaseUserPoints(const int toDecrease) noexcept
{
    if (!toDecrease)
        return false;

    //  if (instance->userControlPoints > 0 && instance->userControlPoints >= toDecrease)
    {
        instance->userControlPoints -= toDecrease;
        instance->userControlPointsSpent += toDecrease;
        //    return true;
    }
    if (!instance->isCheaterBeforeCombat && !instance->cheaterFlagSet && instance->userControlPoints < 0)
    {
        P_Game->isCheater = true;
        instance->cheaterFlagSet = true;

        LPCSTR cheatMsg = P_GeneralText->GetText(262);
        WriteMessageToLog("CHEATER", LOG_TYPE_SCREEN);
    }
    return false;
}

// Implementation of patch creation
void CombatSettingsManager::CreatePatches()
{
    if (!m_isInited)
    {

        m_isInited = true;
        m_isEnabled = true;
        WriteHiHook(0x0462C8A, THISCALL_, BattleMgr_StartBattle);
        WriteHiHook(0x04746B0, THISCALL_, BattleMgr_ProcessAction_KeyPressed);

        newRoundPatch = WriteHiHook(0x0475800, THISCALL_, BattleMgr_NewRound);
        endCombatPatch = WriteHiHook(0x0475CFD, THISCALL_, BattleMgr_SetWinner);
    }
}
