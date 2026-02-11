#include "framework.h"

#include "CreatureAttackRandom.h"

CreatureAttackRandom *CreatureAttackRandom::instance = nullptr;
CreatureAttackRandom::CreatureAttackRandom() : IGamePatch(globalPatcher->CreateInstance(instanceName))
{
    CreatePatches();
}

void __stdcall CreatureAttackRandom::BattleStack_Shoot_Prepare(HiHook *hook, const H3CombatCreature *attacker)
{
    THISCALL_1(void, hook->GetDefaultFunc(), attacker);
    instance->ResetAfterAttackState();
}
void __stdcall CreatureAttackRandom::BattleStack_AttackMelee_Prepare(HiHook *hook, const H3CombatCreature *attacker,
                                                                     const int direction)
{

    THISCALL_2(void, hook->GetDefaultFunc(), attacker, direction);
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

    THISCALL_2(void, hook->GetDefaultFunc(), attacker, defender);

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

    const char result = THISCALL_3(char, hook->GetDefaultFunc(), attacker, defender, direction);

    instance->currentSettings = nullptr;
    instance->attackerActionData.isSecondAttack = false;

    return result;
}

int __stdcall CreatureAttackRandom::BattleStack_DamageRandom(HiHook *h, const int min, const int max)
{

    if (const auto &settings = instance->currentSettings)
    {
        // if input direct damage is set to always, ignore all other settings and call original function
        if (settings->inputDirectDamage.triggerState == eTriggerState::TRIGGER_STATE_ALWAYS)
        {
            return FASTCALL_2(int, h->GetDefaultFunc(), min, max);
        }

        const eTriggerState damageState = instance->attackerActionData.isSecondAttack
                                              ? settings->secondAttackDamage.triggerState
                                              : settings->firstAttackDamage.triggerState;
        switch (damageState)
        {
        case eTriggerState::TRIGGER_STATE_DEFAULT:
            break;
        case eTriggerState::TRIGGER_STATE_ALWAYS:
            return min;
        case eTriggerState::TRIGGER_STATE_NEVER:
            return max;
        default:
            break;
        }
    }

    // return default behavior
    return FASTCALL_2(int, h->GetDefaultFunc(), min, max);
}

int __stdcall CreatureAttackRandom::BattleStack_AfterAttackAbilityRandom(HiHook *hook, const int min, const int max)
{
    return CombatStackSettings::BattleStack_Random(hook, min, max,
                                                   instance->currentSettings->At(STACK_SETTING_AFTER_ATTACK_ABILITY));
}

int __stdcall CreatureAttackRandom::BattleStack_DeathStareAbility(HiHook *hook, const int min, const int max)
{
    const auto &creature = instance->currentSettings->creature;

    //    -[X] - Если в количестве до 10 - включительно то НЕ убьет Смертельным взглядом никого(Если строго больше -
    //    дефолт)

    if (creature && creature->numberAlive > 10)
        return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);

    return CombatStackSettings::BattleStack_Random(hook, min, max,
                                                   instance->currentSettings->At(STACK_SETTING_AFTER_ATTACK_ABILITY));
}

int __stdcall CreatureAttackRandom::BattleStack_DoubleDamageRandom(HiHook *hook, const int min, const int max)
{
    const auto &attackerActionData = instance->attackerActionData;
    if (attackerActionData.isDamageEmulation)
    {
        return attackerActionData.isDoubleDamageTriggered ? min : max;
    }

    return CombatStackSettings::BattleStack_Random(hook, min, max,
                                                   instance->currentSettings->At(STACK_SETTING_DOUBLE_DAMAGE));
}

// set flag that double damage has been triggered, so that damage emulation can return correct value
// if it is emulated damage then skip adding text into hintbar
_LHF_(CreatureAttackRandom::BattleStack_DoubleDamageTrigger)
{
    if (instance->attackerActionData.isDamageEmulation)
    {
        c->AL() = true; // set flag tha battle is hidden
        //        c->return_address += 5;
        return NO_EXEC_DEFAULT;
    }
    else
    {
        instance->attackerActionData.isDoubleDamageTriggered = true;
    }
    return EXEC_DEFAULT;
}
int __stdcall CreatureAttackRandom::BattleStack_LuckRandom(HiHook *hook, const int min, const int max)
{

    const auto *luckSetting = &instance->currentSettings->At(STACK_SETTING_POSITIVE_LUCK);
    int pointsToDecrease = 1;
    if (luckSetting->triggerState == TRIGGER_STATE_DEFAULT)
    {
        const int side = instance->currentSettings->creature->side;
        luckSetting = &CombatSideSettings::GetCombatSideSettings(side).At(SIDE_SETTING_UNAFFECTED_BY_LUCK);
        pointsToDecrease = 2;
    }
    //if (luckSetting->Activate())
    {
        CombatSettingsManager::DecreaseUserPoints(pointsToDecrease);
    }

    return CombatStackSettings::BattleStack_Random(hook, min, max, *luckSetting);
}

struct GameBallisticsInfo
{
    struct BallisticsHitChances
    {
        char chanceToHitKeep;
        char chanceToHitTower;
        char chanceToHitGate;
        char chanceToHitWall;
    } ballisticsHitChances;
    char shots;
    char changeToDeal0Damage;
    char changeToDeal1Damage;
    char changeToDeal2Damage;
};

struct BallisticsInfo
{
    int ballisticsSkillLevel = eSecSkillLevel::NONE;
    int shotsAmount = 0;
} ballisticsInfo;
void __stdcall CreatureAttackRandom::BattleStack_CatapultShot(HiHook *h, H3CombatCreature *attacker,
                                                              const int targetHex)
{

    const int creatureType = attacker->type;
    int skillLevel = 0;
    if (creatureType == eCreature::CATAPULT)
    {
        skillLevel = P_CombatManager->hero[0]->secSkill[eSecondary::BALLISTICS];
    }
    else
    {
        skillLevel = creatureType == eCreature::CYCLOPS_KING + 1;
    }

    ballisticsInfo.ballisticsSkillLevel = skillLevel;

    GameBallisticsInfo *gameballisticsInfo = *reinterpret_cast<GameBallisticsInfo **>(0x0679C84);

    // store original hit chances to restore them after shot
    GameBallisticsInfo storedInfo = gameballisticsInfo[skillLevel];

    // set 100% chance to hit any target;
    libc::memset(&gameballisticsInfo[skillLevel].ballisticsHitChances, 100,
                 sizeof(gameballisticsInfo[skillLevel].ballisticsHitChances));
    gameballisticsInfo[skillLevel].changeToDeal0Damage = 0;
    gameballisticsInfo[skillLevel].changeToDeal1Damage = 0;
    gameballisticsInfo[skillLevel].changeToDeal2Damage = 100;

    gameballisticsInfo[skillLevel].shots = 1;

    // info.ballisticsHitChances.chanceToHitKeep = 100;

    // store creature type before random function
    instance->currentSettings = &CombatStackSettings::GetCombatStackSettings(attacker);
    THISCALL_2(void, h->GetDefaultFunc(), attacker, targetHex);
    attacker->info.morale = 1;
    attacker->info.siegeWeapon = false;
    //  P_CombatManager->activeStack = nullptr;
//    P_CombatManager->action = eCombatAction::CANCEL;
 //   P_CombatManager->actionUndergoing = false;

   // THISCALL_3(void, 0x0464F10, P_CombatManager->Get(), attacker->side, attacker->sideIndex);
    instance->currentSettings = nullptr;

    // restore original hit chances
    gameballisticsInfo[skillLevel] = storedInfo;

    ballisticsInfo = {};
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
    currentSettings = nullptr;
    currentCombatCreature = nullptr;
    currentDamageAbility = nullptr;
    attackerActionData = {};
    targetWallId = -1;
    libc::memset(stacksAttackedAtLeastOnce, 0, sizeof(stacksAttackedAtLeastOnce));
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

// BOOL DamageInputDlg::DialogProc(H3Msg &msg)
//{
//     return 0;
// }

DamageInputDlg::DamageInputDlg(const H3CombatCreature *attacker, const H3CombatCreature *target,
                               const PossibleDamage &basePossibleDamage, const PossibleDamage &finalPossibleDamage,
                               const int width, const int height)
    : H3Dlg(width, height, -1, -1, true, true), attacker(attacker), target(target),
      basePossibleDamage(basePossibleDamage), finalPossibleDamage(finalPossibleDamage),
      resultBaseDamage(basePossibleDamage.realDamage)

{

    constexpr int margin = 16;

    constexpr int damageItemWidth = 100;
    constexpr int damageItemHeight = 20;

    constexpr int editHeight = 20;
    // const int editWidth = width - margin * 2;
    const int editY = height - editHeight - 44;
    auto &inputField = userInputDamage;

    libc::sprintf(h3_TextBuffer, "%d", resultBaseDamage);
    inputField = CreateEdit(margin, editY, damageItemWidth, damageItemHeight, 13, h3_TextBuffer, NH3Dlg::Text::MEDIUM,
                            1, 5, NH3Dlg::HDassets::HD_STATUSBAR_PCX, 0, 1);
    inputField->SetFocus();
    inputField->SetAutoredraw(true);

    const int damageRowY = margin + editHeight + 4;
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
    constexpr int damageItemHeight = 20;
    constexpr int margin = 16;

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
        WriteHiHook(0x04419D0, THISCALL_, BattleStack_AttackMelee_Prepare);

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
        //     WriteHiHook(0x0445BE0, THISCALL_, BattleStack_AttackWall);
        //       WriteLoHook(0x0445CBB, BattleStack_MakeBallisticShot);
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
