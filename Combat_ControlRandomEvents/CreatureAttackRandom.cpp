#include "framework.h"

#include "CreatureAttackRandom.h"
char GetCombatCreatureWallAttackShots(const H3CombatCreature *creature);
struct PluginText
{
    static PluginText &GetInstance();
    LPCSTR GetCreatureAbilityCustomText(const eStackAbility settingId) const noexcept;
};

CreatureAttackRandom *CreatureAttackRandom::instance = nullptr;
CreatureAttackRandom::CreatureAttackRandom() : IGamePatch(globalPatcher->CreateInstance(instanceName))
{
    CreatePatches();
}

void __stdcall CreatureAttackRandom::BattleStack_Shoot_Prepare(HiHook *hook, const H3CombatCreature *attacker)
{
    instance->attackInitiator = attacker;
    THISCALL_1(void, hook->GetDefaultFunc(), attacker);

    if (instance->attackerActionData.isLuckTriggered)
        CombatStackSettings::GetCombatStackSettings(attacker).TriggerAbility(STACK_SETTING_DOUBLE_LUCK);

    instance->ResetAfterAttackState();
}
void __stdcall CreatureAttackRandom::BattleStack_AttackMelee_Prepare(HiHook *hook, const H3CombatCreature *attacker,
                                                                     const int direction)
{

    instance->attackInitiator = attacker;
    THISCALL_2(void, hook->GetDefaultFunc(), attacker, direction);

    if (instance->attackerActionData.isLuckTriggered)
        CombatStackSettings::GetCombatStackSettings(attacker).TriggerAbility(STACK_SETTING_DOUBLE_LUCK);

    instance->ResetAfterAttackState();
}

void __stdcall CreatureAttackRandom::BattleStack_Shoot(HiHook *hook, const H3CombatCreature *attacker,
                                                       const H3CombatCreature *defender)
{

    auto &stackAttacked = instance->stacksAttackedAtLeastOnce[attacker->side][attacker->sideIndex];
    if (stackAttacked)
        instance->attackerActionData.isSecondAttack = true;
    else
        stackAttacked = true;

    instance->currentSettings = &CombatStackSettings::GetCombatStackSettings(attacker);
    instance->currentCombatCreatureAttacker = attacker;
    instance->currentCombatCreatureDefender = defender;

    THISCALL_2(void, hook->GetDefaultFunc(), attacker, defender);

    instance->currentCombatCreatureAttacker = nullptr;
    instance->currentCombatCreatureDefender = nullptr;

    instance->currentSettings = nullptr;
    instance->attackerActionData.isSecondAttack = false;
}
char __stdcall CreatureAttackRandom::BattleStack_AttackMelee(HiHook *hook, const H3CombatCreature *attacker,
                                                             const H3CombatCreature *defender, const int direction)
{
    // store creature type before random function
    auto &stackAttacked = instance->stacksAttackedAtLeastOnce[attacker->side][attacker->sideIndex];
    if (stackAttacked)
        instance->attackerActionData.isSecondAttack = true;
    else
        stackAttacked = true;
    instance->currentSettings = &CombatStackSettings::GetCombatStackSettings(attacker);
    instance->currentCombatCreatureAttacker = attacker;
    instance->currentCombatCreatureDefender = defender;

    const char result = THISCALL_3(char, hook->GetDefaultFunc(), attacker, defender, direction);

    instance->currentCombatCreatureAttacker = nullptr;
    instance->currentCombatCreatureDefender = nullptr;

    instance->currentSettings = nullptr;
    instance->attackerActionData.isSecondAttack = false;

    return result;
}

int __stdcall CreatureAttackRandom::BattleStack_DamageRandom(HiHook *h, const int min, const int max)
{
    const auto &settings = instance->currentSettings;
    if (settings->inputDirectDamage.triggerState == TRIGGER_STATE_ENABLED)
    {
        return FASTCALL_2(int, h->GetDefaultFunc(), min, max);
    }

    const eStackAbility damageSettingId = instance->attackerActionData.isSecondAttack
                                              ? STACK_SETTING_DAMAGE_VARIATION_SECOND
                                              : STACK_SETTING_DAMAGE_VARIATION_FIRST;

    switch (settings->At(damageSettingId).triggerState)
    {
    case TRIGGER_STATE_DEFAULT:
        break;
    case TRIGGER_STATE_ALWAYS:
        settings->TriggerAbility(damageSettingId);
        return min;
    case TRIGGER_STATE_NEVER:
        settings->TriggerAbility(damageSettingId);
        return max;
    default:
        break;
    }

    // return default behavior
    return FASTCALL_2(int, h->GetDefaultFunc(), min, max);
}

int __stdcall CreatureAttackRandom::BattleStack_AfterAttackAbilityRandom(HiHook *hook, const int min, const int max)
{

    auto &stackSettings = instance->currentSettings;
    constexpr auto stackSettingId = STACK_SETTING_AFTER_ATTACK_ABILITY;

    if (instance->currentCombatCreatureDefender->numberAlive <= 0)
    {
        // if defender is already dead then don't trigger after attack ability, just return default value
        return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
    }

    switch (stackSettings->At(stackSettingId).triggerState)
    {
    case eTriggerState::TRIGGER_STATE_ALWAYS:
        stackSettings->TriggerAbility(stackSettingId);
        return min; // always trigger ability
    case eTriggerState::TRIGGER_STATE_NEVER:

        stackSettings->TriggerAbility(
            stackSettingId, instance->darkKnightHandlers[stackSettings->creature->side].isDoubleDamageDisabled);
        return max; // never trigger ability
    default:
        break;
    }

    return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
}

int __stdcall CreatureAttackRandom::BattleStack_DeathStareAbility(HiHook *hook, const int min, const int max)
{
    const auto &creature = instance->currentSettings->creature;

    //    -[X] - Если в количестве до 10 - включительно то НЕ убьет Смертельным взглядом никого(Если строго больше -
    //    дефолт)

    if (creature && creature->numberAlive > 10)
        return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);

    auto &stackSettings = instance->currentSettings;
    constexpr auto stackSettingId = STACK_SETTING_AFTER_ATTACK_ABILITY;

    switch (stackSettings->At(stackSettingId).triggerState)
    {
    case eTriggerState::TRIGGER_STATE_ALWAYS:
        stackSettings->TriggerAbility(stackSettingId);
        return min; // always trigger ability
    case eTriggerState::TRIGGER_STATE_NEVER:
        stackSettings->TriggerAbility(stackSettingId);
        return max; // never trigger ability
    default:
        break;
    }

    return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
}

int __stdcall CreatureAttackRandom::BattleStack_DoubleDamageRandom(HiHook *hook, const int min, const int max)
{
    auto &attackerActionData = instance->attackerActionData;
    if (attackerActionData.isDamageEmulation)
    {
        return attackerActionData.isDoubleDamageTriggered ? min : max;
    }

    auto &stackSettings = instance->currentSettings;
    constexpr auto stackSettingId = STACK_SETTING_DOUBLE_DAMAGE;
    Ability settings = stackSettings->At(stackSettingId);

    switch (settings.triggerState)
    {
    case eTriggerState::TRIGGER_STATE_ALWAYS:
        stackSettings->TriggerAbility(stackSettingId);
        return min; // always trigger ability
    case eTriggerState::TRIGGER_STATE_NEVER:
        stackSettings->TriggerAbility(stackSettingId);
        instance->darkKnightHandlers[stackSettings->creature->side].isDoubleDamageDisabled = true;
        return max; // never trigger ability
    default:
        break;
    }

    return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
}

// set flag that double damage has been triggered, so that damage emulation can return correct value
// if it is emulated damage then skip adding text into hintbar
_LHF_(CreatureAttackRandom::BattleStack_DoubleDamageTrigger)
{
    if (instance->attackerActionData.isDamageEmulation)
    {
        c->AL() = true; // set flag tha battle is hidden
        //        c->return_address += 5;
        return NO_EXEC_DEFAULT; // it adds 5 bytes by default;
    }
    else
    {
        instance->attackerActionData.isDoubleDamageTriggered = true;
    }
    return EXEC_DEFAULT;
}
int __stdcall CreatureAttackRandom::BattleStack_LuckRandom(HiHook *hook, const int min, const int max)
{

    // - [N+Ctrl] - (2 желания) - ваш/вражеский Искусный арбалетчик, Крестоносец, Благородный эльф и Налётчик нанесет
    // оба удара с срабатыванием удачи
    auto &currentSettings = instance->currentSettings;
    if (currentSettings->creature == instance->attackInitiator)
    {
        auto &doubleLuckSettings = currentSettings->At(STACK_SETTING_DOUBLE_LUCK);
        if (doubleLuckSettings.triggerState == TRIGGER_STATE_ENABLED)
        {
            instance->attackerActionData.isLuckTriggered = true;
            return min;
        }
    }

    return CombatStackSettings::BattleStack_ContinuousRandom(instance->currentSettings->creature,
                                                             STACK_SETTING_POSITIVE_LUCK,
                                                             SIDE_SETTING_UNAFFECTED_BY_LUCK, hook, min, max);
}

void WallDamageDlg::OnOK()
{
    selectedDamageIndex = damageValues[scrollbar->GetTick()];
}

int GetCombatCreatureBallisticsLevel(const H3CombatCreature *creature)
{
    if (!creature || !creature->info.destroyWalls)
        return -1;
    auto owner = creature->GetOwner();
    switch (creature->type)
    {
    case eCreature::CYCLOPS:
        return eSecSkillLevel::BASIC;
    case eCreature::CYCLOPS_KING:
        return eSecSkillLevel::ADVANCED;
    case eCreature::CATAPULT:
        if (owner)
            return static_cast<eSecSkillLevel>(owner->secSkill[eSecondary::BALLISTICS]);
    default:
        break;
    }
    return eSecSkillLevel::NONE;
}
BOOL8 HasCombatCreatureVariadicWallDamage(const H3CombatCreature *creature)
{
    const int skillLevel = GetCombatCreatureBallisticsLevel(creature);
    if (skillLevel < 0)
        return false;

    GameBallisticsInfo *gameBallisticsInfo = *reinterpret_cast<GameBallisticsInfo **>(0x0679C84);
    for (size_t i = 0; i < WALL_DAMAGES_AMOUNT; i++)
    {
        if (gameBallisticsInfo[skillLevel].dealDamageChances[i] == 100)
            return false;
    }
    return true;
}
char GetCombatCreatureWallAttackShots(const H3CombatCreature *creature)
{
    const int skillLevel = GetCombatCreatureBallisticsLevel(creature);
    if (skillLevel < 0)
        return 0;
    GameBallisticsInfo *gameBallisticsInfo = *reinterpret_cast<GameBallisticsInfo **>(0x0679C84);
    return gameBallisticsInfo[skillLevel].shots;
}
void __stdcall CreatureAttackRandom::BattleStack_CatapultShot(HiHook *h, H3CombatCreature *attacker,
                                                              const int targetHex)
{

    const int skillLevel = GetCombatCreatureBallisticsLevel(attacker);
    if (skillLevel < 0)
    {
        THISCALL_2(void, h->GetDefaultFunc(), attacker, targetHex);
        return;
    }

    auto patch = instance->allowCatapultRepeatedShotsPatch;

    // always disablle patch to prevent recursive calls, it will be enabled later if needed
    if (patch->IsApplied())
        patch->Undo();

    auto &catapultAttacksMade = instance->catapultAttacksMade;
    const BOOL isFirstAttack = catapultAttacksMade == 0;

    auto &currentSettings = CombatStackSettings::GetCombatStackSettings(attacker);

    BOOL variationDamageEnabled = false;
    BOOL variationDamageInputEnabled = false;
    const BOOL creatureHasVariadicWallDamage = HasCombatCreatureVariadicWallDamage(attacker);
    const eStackAbility damageVariationSettingId =
        isFirstAttack ? STACK_SETTING_DAMAGE_VARIATION_FIRST : STACK_SETTING_DAMAGE_VARIATION_SECOND;

    auto &damageState = currentSettings.At(damageVariationSettingId).triggerState;
    if (creatureHasVariadicWallDamage)
    {

        variationDamageInputEnabled =
            currentSettings.At(STACK_SETTING_DAMAGE_INPUT).triggerState == TRIGGER_STATE_ENABLED;

        if (variationDamageInputEnabled || damageState != TRIGGER_STATE_DISABLED)
        {
            variationDamageEnabled = true;
        }
    }

    auto &aimShotState = currentSettings.At(STACK_SETTING_WALL_ATTACK_AIM).catapultAimShotState;

    const BOOL aimShotEnabled = isFirstAttack ? aimShotState != CATAPULT_AIM_SHOT_STATE_DISABLED
                                              : aimShotState == CATAPULT_AIM_SHOT_STATE_ALL_SHOTS;

    // if neither aim shot nor damage variation is enabled then just do default shot without any changes
    if (!(aimShotEnabled || variationDamageEnabled))
    {
        THISCALL_2(void, h->GetDefaultFunc(), attacker, targetHex);
        return;
    }

    GameBallisticsInfo *gameBallisticsInfo = nullptr;
    GameBallisticsInfo storedInfo;

    //    ballisticsInfo.ballisticsSkillLevel = skillLevel;

    gameBallisticsInfo = *reinterpret_cast<GameBallisticsInfo **>(0x0679C84);
    // store original hit chances to restore them after shot
    storedInfo = gameBallisticsInfo[skillLevel];

    // if it is 1st shot we should enable patch to control next shots;
    if (aimShotState == CATAPULT_AIM_SHOT_STATE_ALL_SHOTS)
    {
        gameBallisticsInfo[skillLevel].shots = 1; // set shots to 1 to prevent multiple shots in one attack
        if (++catapultAttacksMade < storedInfo.shots)
            patch->Apply();
        else
            catapultAttacksMade = 0; // reset attacks counter for next time
    }
    else
    {
        catapultAttacksMade = 0; // reset attacks counter for next time
    }

    UINT indexOfDamageToTrigger = -1; // init as undefined value

    if (aimShotEnabled)
    {
        // set 100% chance to hit any target;
        libc::memset(&gameBallisticsInfo[skillLevel].ballisticsHitChances, 100,
                     sizeof(gameBallisticsInfo[skillLevel].ballisticsHitChances));

        if (aimShotState == CATAPULT_AIM_SHOT_STATE_ALL_SHOTS && catapultAttacksMade)
        {
            const Ability copyAbility = currentSettings.At(STACK_SETTING_WALL_ATTACK_AIM);
            currentSettings.TriggerAbility(STACK_SETTING_WALL_ATTACK_AIM);
            // restore ability state, because it will be triggered for each shot
            currentSettings.asArray[STACK_SETTING_WALL_ATTACK_AIM] = copyAbility;
        }
        else
        {
            currentSettings.TriggerAbility(STACK_SETTING_WALL_ATTACK_AIM);
        }
        indexOfDamageToTrigger = 2; // max damage by default
    }

    eStackAbility damageStackAbilityToTrigger = STACK_SETTING_NONE;
    if (variationDamageInputEnabled)
    {
        damageStackAbilityToTrigger = STACK_SETTING_DAMAGE_INPUT;
        indexOfDamageToTrigger = 1;

        WallDamageDlg wallDamageDlg = WallDamageDlg(&gameBallisticsInfo[skillLevel]);
        wallDamageDlg.Start();

        indexOfDamageToTrigger = wallDamageDlg.selectedDamageIndex;

        // change cost
        currentSettings.asArray[STACK_SETTING_DAMAGE_INPUT].cost = 1;
    }
    else if (variationDamageEnabled)
    {

        damageStackAbilityToTrigger = damageVariationSettingId;
        auto &dealDamageChances = gameBallisticsInfo[skillLevel].dealDamageChances;

        if (damageState == TRIGGER_STATE_ALWAYS) // minimum damage
        {
            for (size_t i = 0; i < WALL_DAMAGES_AMOUNT; i++)
            {
                if (dealDamageChances[i] > 0)
                {
                    indexOfDamageToTrigger = i;
                    break;
                }
            }
        }
        else if (damageState == TRIGGER_STATE_NEVER) // maximum damage
        {
            for (size_t i = WALL_DAMAGES_AMOUNT - 1; i == 0; i--)
            {
                if (dealDamageChances[i] > 0)
                {
                    indexOfDamageToTrigger = i;
                    break;
                }
            }
        }
    }

    if (indexOfDamageToTrigger >= 0 && indexOfDamageToTrigger < WALL_DAMAGES_AMOUNT)
    {
        // set needed damage chances to 100% and other chances to 0%
        libc::memset(&gameBallisticsInfo[skillLevel].dealDamageChances, 0,
                     sizeof(gameBallisticsInfo[skillLevel].dealDamageChances));

        gameBallisticsInfo[skillLevel].dealDamageChances[indexOfDamageToTrigger] = 100;

        if (damageStackAbilityToTrigger != STACK_SETTING_NONE)
        {
            const BOOL dontSpendAdditionalPoints =
                aimShotEnabled; // if aim shot is triggered then don't spend additional points for damage variation
            currentSettings.TriggerAbility(damageStackAbilityToTrigger, dontSpendAdditionalPoints);
        }
    }

    THISCALL_2(void, h->GetDefaultFunc(), attacker, targetHex);

    if (gameBallisticsInfo)
    {
        // restore original hit chances
        gameBallisticsInfo[skillLevel] = storedInfo;
    }
}

void __stdcall CreatureAttackRandom::BattleStack_AttackWall(HiHook *h, H3CombatCreature *attacker, const int wallId,
                                                            int *damages)
{
    // store creature type before random function
    if (instance->currentSettings->wallAttackAim.triggerState)
    {
        instance->targetWallId = wallId; // always hit the wall
    }
    THISCALL_3(void, h->GetDefaultFunc(), attacker, wallId, damages);
}
_LHF_(CreatureAttackRandom::BattleStack_MakeBallisticShot)
{

    if (instance->targetWallId != -1)
    {
        c->Pop();
        c->Push(instance->targetWallId);
        instance->targetWallId = -1;
    }

    return EXEC_DEFAULT;
}

void CreatureAttackRandom::CreateAbilityEvent(const eCreature creature, const DWORD patchAddress, void *functionPtr)
{

    if (creature != eCreature::UNDEFINED)
    {
        m_creaturesWithAfterAttackAbility.insert(creature);

        if (patchAddress && functionPtr)
        {
            auto it = m_abilitiesPatchesMap.find(patchAddress);
            if (it == m_abilitiesPatchesMap.end())
            {
                //   Patch *patch = nullptr;

                WriteHiHook(patchAddress, FASTCALL_, functionPtr);
                m_abilitiesPatchesMap.insert(std::make_pair(patchAddress, functionPtr));
            }
        }
    }
}

void CreatureAttackRandom::ResetAfterAttackState()
{
    attackInitiator = nullptr;
    currentSettings = nullptr;
    currentCombatCreatureAttacker = nullptr;
    currentCombatCreatureDefender = nullptr;
    currentDamageAbility = nullptr;
    attackerActionData = {};
    targetWallId = -1;
    libc::memset(stacksAttackedAtLeastOnce, 0, sizeof(stacksAttackedAtLeastOnce));
    libc::memset(darkKnightHandlers, 0, sizeof(darkKnightHandlers));
}

static int BattleStack_CalaculateFinalDamageFromBaseDamage(const H3CombatCreature *attacker,
                                                           const H3CombatCreature *defender, const int baseDamage,
                                                           const BOOL isShooting = P_CombatManager->action ==
                                                                                   eCombatAction::SHOOT,
                                                           int *fireShieldDamage = nullptr)
{

    return THISCALL_7(int, 0x0443C60, attacker, defender, baseDamage, isShooting, false, attacker->hexesTraveled,
                      fireShieldDamage);
}

DamageInputDlg::DamageInputDlg(const H3CombatCreature *attacker, const H3CombatCreature *target,
                               const PossibleDamage &basePossibleDamage, const PossibleDamage &finalPossibleDamage,
                               const int width, const int height)
    : H3Dlg(width, height, -1, -1, false, true), attacker(attacker), target(target),
      basePossibleDamage(basePossibleDamage), finalPossibleDamage(finalPossibleDamage),
      resultBaseDamage(basePossibleDamage.realDamage)

{

    libc::sprintf(h3_TextBuffer, PluginText::GetInstance().GetCreatureAbilityCustomText(STACK_SETTING_DAMAGE_INPUT),
                  attacker->info.GetCreatureName(attacker->numberAlive), finalPossibleDamage.realDamage,
                  basePossibleDamage.realDamage);
    constexpr int textHeight = 100;
    constexpr int textY = 16;
    userInfoText =
        CreateText(16, textY, width - 32, textHeight, h3_TextBuffer, NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1);

    constexpr int damageItemWidth = 100;
    constexpr int damageItemHeight = 19;

    constexpr int editY = textHeight + textY + 8;

    auto &inputField = userInputDamage;

    const int editX = width / 2 - damageItemWidth / 2;
    libc::sprintf(h3_TextBuffer, "%d", resultBaseDamage);
    inputField = CreateEdit(editX, editY, damageItemWidth, damageItemHeight, 11, h3_TextBuffer, NH3Dlg::Text::MEDIUM, 1,
                            5, NH3Dlg::HDassets::HD_STATUSBAR_PCX, 0, 1);
    inputField->SetFocus();
    inputField->SetAutoredraw(true);

    const int damageRowY = editY + damageItemHeight + 14;
    CreateDamageRow(basePossibleDamage, baseDamageRow, damageRowY);
    CreateDamageRow(finalPossibleDamage, finalDamageRow, damageRowY + damageItemHeight + 4);

    okButton = CreateOKButton();
    cancelButton = CreateCancelButton();
}

void DamageInputDlg::CreateDamageRow(const PossibleDamage &possibleDamage, DamageRow &damageRow, const int y)
{
    //  static_assert(sizeof(possibleDamage.damageValues) == sizeof(damageRow.damageItems));
    const size_t size = std::size(damageRow.damageItems);

    constexpr int damageItemWidth = 100;
    constexpr int damageItemHeight = 19;
    const int margin = this->widthDlg - 32 - size * (damageItemWidth + 4);

    for (size_t i = 0; i < size; i++)
    {
        libc::sprintf(h3_TextBuffer, "%d", possibleDamage.damageValues[i]);

        const int x = margin + i * (damageItemWidth + 4);

        damageRow.damageItems[i] = CreateText(x, y, damageItemWidth, damageItemHeight, h3_TextBuffer,
                                              NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1);
    }
}

void DamageInputDlg::UpdateInputDamage()
{

    const int baseDamageToDraw =
        Clamp(basePossibleDamage.minDamage, libc::atoi(userInputDamage->GetText()), basePossibleDamage.maxDamage);

    const int currentDisplayedBaseDamage = libc::atoi(baseDamageRow.realText->GetH3String().String());

    if (currentDisplayedBaseDamage == baseDamageToDraw)
    {
        return;
    }

    libc::sprintf(h3_TextBuffer, "%d", baseDamageToDraw);

    baseDamageRow.realText->SetText(h3_TextBuffer);

    const int finalDamage = BattleStack_CalaculateFinalDamageFromBaseDamage(attacker, target, baseDamageToDraw);
    libc::sprintf(h3_TextBuffer, "%d", finalDamage);
    finalDamageRow.realText->SetText(h3_TextBuffer);
    Redraw();
}

void DamageInputDlg::OnOK()
{
    const int currentDisplayedBaseDamage = libc::atoi(baseDamageRow.realText->GetH3String().String());
    resultBaseDamage = Clamp(basePossibleDamage.minDamage, currentDisplayedBaseDamage, basePossibleDamage.maxDamage);
}

BOOL DamageInputDlg::OnKeyPress(eVKey key, eMsgFlag flag)
{
    UpdateInputDamage();
    return 0;
}

int DamageInputDlg::ShowInputDamageDlg(const H3CombatCreature *attacker, const H3CombatCreature *target,
                                       const PossibleDamage &basePossibleDamage,
                                       const PossibleDamage &finalPossibleDamage)
{
    H3String storedTextBuffer = h3_TextBuffer;
    auto &dlg = DamageInputDlg(attacker, target, basePossibleDamage, finalPossibleDamage);
    dlg.Start();

    libc::sprintf(h3_TextBuffer, "%s", storedTextBuffer.String());
    return dlg.resultBaseDamage;
}

int __stdcall CreatureAttackRandom::BattleStack_CalculateDamageToMonster(HiHook *h, H3CombatCreature *_this,
                                                                         H3CombatCreature *targetCreature,
                                                                         int baseDamage, char shoot, char isTheoretical,
                                                                         int stepsTaken, int *fireShieldDmg)
{

    auto &attackerActionData = instance->attackerActionData;
    attackerActionData.isDamageEmulation = false;
    attackerActionData.isDoubleDamageTriggered = false;

    int damageResult = THISCALL_7(int, h->GetDefaultFunc(), _this, targetCreature, baseDamage, shoot, isTheoretical,
                                  stepsTaken, fireShieldDmg);
    if (isTheoretical)
        return damageResult;

    if (P_CombatManager->autoCombat || H3AutoSolo::Get() ||
        P_CombatManager->IsHiddenBattle()) // 698A3C is some action is in proc
    {
        return damageResult;
    }

    if (_this->info.damageLow >= _this->info.damageHigh)
    {
        return damageResult;
    }
    if (_this->activeSpellDuration[eSpell::BLESS] || _this->activeSpellDuration[eSpell::CURSE])
    {
        return damageResult;
    }
    auto &settings = CombatStackSettings::GetCombatStackSettings(_this);
    auto &abilitySettings = settings.At(STACK_SETTING_DAMAGE_INPUT);
    if (abilitySettings.triggerState != TRIGGER_STATE_ENABLED)
    {
        return damageResult;
    }

    // set this flag to let other hooks know that we are in damage emulation mode and they should return values based on
    // settings instead of doing real randomization
    attackerActionData.isDamageEmulation = true;

    // patch to block hatred damage bonus text adding to the combat log during damage emulation
    auto *blockDamageLoggin = instance->_pi->WriteJmp(0x044322E, 0x0443410);

    const int minCreatureDamage = _this->info.damageLow;
    const int maxCreatureDamage = _this->info.damageHigh;

    _this->info.damageHigh = minCreatureDamage;
    const int minPossibleBaseDamage = THISCALL_2(int, 0x0442E80, _this, isTheoretical);
    const int minPossibleDamage =
        BattleStack_CalaculateFinalDamageFromBaseDamage(_this, targetCreature, minPossibleBaseDamage, shoot);

    _this->info.damageLow = maxCreatureDamage;
    _this->info.damageHigh = maxCreatureDamage;
    const int maxPossibleBaseDamage = THISCALL_2(int, 0x0442E80, _this, isTheoretical);

    const int maxPossibleDamage =
        BattleStack_CalaculateFinalDamageFromBaseDamage(_this, targetCreature, maxPossibleBaseDamage, shoot);

    _this->info.damageLow = minCreatureDamage;
    _this->info.damageHigh = maxCreatureDamage;

    PossibleDamage basePossibleDamage{minPossibleBaseDamage, baseDamage, maxPossibleBaseDamage};
    PossibleDamage finalPossibleDamage{minPossibleDamage, damageResult, maxPossibleDamage};

    const int inputBaseDamage =
        DamageInputDlg::ShowInputDamageDlg(_this, targetCreature, basePossibleDamage, finalPossibleDamage);

    if (inputBaseDamage != baseDamage)
    {
        damageResult = BattleStack_CalaculateFinalDamageFromBaseDamage(_this, targetCreature, inputBaseDamage, shoot,
                                                                       fireShieldDmg);
        if (!instance->attackerActionData.isSecondAttack)
        {
            settings.TriggerAbility(STACK_SETTING_DAMAGE_INPUT);
        }
    }

    // end of damage emulation, reset flag to let other hooks know that they should return to default behavior
    attackerActionData.isDamageEmulation = false;
    attackerActionData.isDoubleDamageTriggered = false;
    blockDamageLoggin->Destroy();
    // if (_this->type == eCreature::DREAD_KNIGHT)
    {

        // libc::sprintf(
        //     h3_TextBuffer,
        //     "%s is about deal %d damage (Min Possible: %d, Max Possible: %d)\n\nWould do like to set exact
        //     value?", P_CreatureInformation[_this->type].GetCreatureName(_this->numberAlive), realDamage,
        //     minPossibleDamage, maxPossibleDamage);
        // H3Messagebox::Choice(h3_TextBuffer);
    }
    return damageResult;
}

void CreatureAttackRandom::CreatePatches()
{
    if (!this->m_isInited)
    {
        this->m_isInited = true;

        // used as a pre-hook to count shots and melee attacks
        // both are "CALL_" type
        WriteHiHook(0x04458D8, THISCALL_, BattleStack_Shoot_Prepare);
        WriteHiHook(0x0445999, THISCALL_, BattleStack_AttackMelee_Prepare);

        // used to init currentCreature and currentSettings before and after attack functions
        // both are "SPLICE_" type
        WriteHiHook(0x43F620, THISCALL_, BattleStack_Shoot);
        WriteHiHook(0x441330, THISCALL_, BattleStack_AttackMelee);
        // physical damage control
        // the game has two places where it calculates damage for physical attacks
        WriteHiHook(0x443024, FASTCALL_, BattleStack_DamageRandom);
        WriteHiHook(0x442FE9, FASTCALL_, BattleStack_DamageRandom);

        //   if (0)
        {
            // real damage calculation based based on "BattleStack_DamageRandom" results
            // used to show input dialog with possible damage range and let player choose exact damage value

            // RANGE ATTACKS
            // Magog shoot
            WriteHiHook(0x043F94E, THISCALL_, BattleStack_CalculateDamageToMonster);
            // Lich's death cloud
            WriteHiHook(0x043FD32, THISCALL_, BattleStack_CalculateDamageToMonster);
            // other shooting attacks
            WriteHiHook(0x043FA54, THISCALL_, BattleStack_CalculateDamageToMonster);

            // MELEE ATTACKS
            // Strike All around
            WriteHiHook(0x04400D5, THISCALL_, BattleStack_CalculateDamageToMonster);

            // basic melee attacks
            WriteHiHook(0x044172E, THISCALL_, BattleStack_CalculateDamageToMonster);
            // Dragon breath
            WriteHiHook(0x044177F, THISCALL_, BattleStack_CalculateDamageToMonster);
        }

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
        CreateAbilityEvent(eCreature::GHOST_DRAGON, 0x44025D, BattleStack_AfterAttackAbilityRandom);

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
        CreateAbilityEvent(eCreature::MIGHTY_GORGON, 0x440C00, BattleStack_DeathStareAbility);

        // acid breath
        CreateAbilityEvent(eCreature::RUST_DRAGON, 0x4411D7, BattleStack_AfterAttackAbilityRandom);
        //     return;

        // double damage
        WriteHiHook(0x04436D9, FASTCALL_, BattleStack_DoubleDamageRandom); // dread knights
        WriteHiHook(0x04435FD, FASTCALL_, BattleStack_DoubleDamageRandom); // ballista
        WriteLoHook(0x04436F5, BattleStack_DoubleDamageTrigger);           // dread knights double damage trigger
        WriteLoHook(0x0443620, BattleStack_DoubleDamageTrigger);           // ballista double damage trigger

        // luck control
        WriteHiHook(0x0441557, FASTCALL_, BattleStack_LuckRandom); // melee attack
        WriteHiHook(0x043F675, FASTCALL_, BattleStack_LuckRandom); // ranged attack

        // Cyclops// Catapult: Wall Damage random

        WriteHiHook(0x047942D, THISCALL_, BattleStack_CatapultShot);
        allowCatapultRepeatedShotsPatch = _pi->WriteJmp(0x0479432, 0x0479444);
        allowCatapultRepeatedShotsPatch->Undo(); // allow to trigger catapult shot multiple times in a row
        // WriteHiHook(0x0445BE0, THISCALL_, BattleStack_AttackWall);
        // WriteLoHook(0x0445CBB, BattleStack_MakeBallisticShot);
    }
}

BOOL CreatureAttackRandom::BattleStack_HasAfterAttackAbility(const H3CombatCreature *creature)
{
    return instance->m_creaturesWithAfterAttackAbility.count(creature->type);
}

CreatureAttackRandom &CreatureAttackRandom::GetInstance()
{
    if (!instance)
    {
        instance = new CreatureAttackRandom();
    }

    return *instance;
}
