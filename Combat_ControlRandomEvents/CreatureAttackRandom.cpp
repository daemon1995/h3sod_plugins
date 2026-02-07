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
        instance->useSecondAttack = true;
    else
        stackAttacked = true;
    instance->currentSettings = &CombatStackSettings::GetCombatStackSettings(attacker);

    THISCALL_2(void, hook->GetDefaultFunc(), attacker, defender);

    instance->currentSettings = nullptr;
    instance->useSecondAttack = false;
}
const H3CombatCreature *targetCreature = nullptr;
char __stdcall CreatureAttackRandom::BattleStack_AttackMelee(HiHook *hook, const H3CombatCreature *attacker,
                                                             const H3CombatCreature *defender, const int direction)
{
    // store creature type before random function
    auto &stackAttacked = instance->stacksAttackedAtLeastOnce[attacker->side][attacker->sideIndex];
    if (stackAttacked)
        instance->useSecondAttack = true;
    else
        stackAttacked = true;
    instance->currentSettings = &CombatStackSettings::GetCombatStackSettings(attacker);

    targetCreature = defender;
    const char result = THISCALL_3(char, hook->GetDefaultFunc(), attacker, defender, direction);

    instance->currentSettings = nullptr;
    instance->useSecondAttack = false;

    return result;
}

int __stdcall CreatureAttackRandom::BattleStack_DamageRandom(HiHook *h, const int min, const int max)
{

    if (const auto &settings = instance->currentSettings)
    {
        eDamageState damageState = instance->useSecondAttack ? settings->settings.secondAttackDamage.damageState
                                                             : settings->settings.firstAttackDamage.damageState;
        switch (damageState)
        {
        case eDamageState::DAMAGE_STATE_DEFAULT:
            break;
        case eDamageState::DAMAGE_STATE_MINIMUM:
            return min;
        case eDamageState::DAMAGE_STATE_MAXIMUM:
            return max;
        case eDamageState::DAMAGE_STATE_MIN_25:
            return min + ((max - min) >> 2);
        case eDamageState::DAMAGE_STATE_MIN_50:
            return min + ((max - min) >> 1);
        case eDamageState::DAMAGE_STATE_MIN_75:
            return min + (((max - min) * 3) >> 2);
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

int __stdcall CreatureAttackRandom::BattleStack_DoubleDamageRandom(HiHook *hook, const int min, const int max)
{
    return CombatStackSettings::BattleStack_Random(hook, min, max,
                                                   instance->currentSettings->At(STACK_SETTING_DOUBLE_DAMAGE));
}
int __stdcall CreatureAttackRandom::BattleStack_LuckRandom(HiHook *hook, const int min, const int max)
{
    return CombatStackSettings::BattleStack_Random(hook, min, max,
                                                   instance->currentSettings->At(STACK_SETTING_POSITIVE_LUCK));
}

void __stdcall CreatureAttackRandom::BattleStack_AttackWall(HiHook *h, H3CombatCreature *attacker, const int wallId,
                                                            int *damages)
{
    // store creature type before random function
    instance->currentSettings = &CombatStackSettings::GetCombatStackSettings(attacker);

    if (instance->currentSettings->settings.wallAttackAim.triggerState)
    {
        instance->targetWallId = wallId; // always hit the wall
    }
    THISCALL_3(void, h->GetDefaultFunc(), attacker, wallId, damages);
    instance->currentSettings = nullptr;
}
BOOL CreatureAttackRandom::BattleStack_HasAfterAttackAbility(const H3CombatCreature *creature)
{
    return instance->m_abilitiesMap.count(eCreature(creature->type)) > 0;
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

    if (creature != eCreature::UNDEFINED, patchAddress, functionPtr)
    {
        m_abilitiesMap.insert(std::make_pair(creature, EventHandler{
                                                           true,
                                                           functionPtr,
                                                           WriteHiHook(patchAddress, FASTCALL_, functionPtr),
                                                       }));
    }
}

void CreatureAttackRandom::ResetAfterAttackState()
{
    currentSettings = nullptr;
    currentCombatCreature = nullptr;
    currentDamageAbility = nullptr;
    useSecondAttack = false;
    targetWallId = -1;
    libc::memset(stacksAttackedAtLeastOnce, 0, sizeof(stacksAttackedAtLeastOnce));
}

int BattleStack_CalaculateFinalDamage(H3CombatCreature *attacker, const H3CombatCreature *defender,
                                      const int baseDamage, const BOOL isShooting)

{
    float fireShieldDamage{};
    return THISCALL_7(int, 0x0443C60, attacker, defender, baseDamage, isShooting, false, attacker->hexesTraveled,
                      nullptr);
}

int __stdcall BattleStack_CalculateDamage(HiHook *h, H3CombatCreature *_this, signed int isTeoretic)
{

    int damageResult = THISCALL_2(int, h->GetDefaultFunc(), _this, isTeoretic);
    if (!isTeoretic)
    {
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

        const int minDamage = _this->info.damageLow;
        const int maxDamage = _this->info.damageHigh;

        _this->info.damageHigh = minDamage;
        int minPossibleDamage = THISCALL_2(int, 0x0442E80, _this, isTeoretic);
        minPossibleDamage = BattleStack_CalaculateFinalDamage(_this, targetCreature, minPossibleDamage, false);
        _this->info.damageLow = maxDamage;
        _this->info.damageHigh = maxDamage;
        int maxPossibleDamage = THISCALL_2(int, 0x0442E80, _this, isTeoretic);

        maxPossibleDamage = BattleStack_CalaculateFinalDamage(_this, targetCreature, maxPossibleDamage, false);
        _this->info.damageLow = minDamage;
        _this->info.damageHigh = maxDamage;

        int realDamage = BattleStack_CalaculateFinalDamage(_this, targetCreature, damageResult, false);
        // if (_this->type == eCreature::DREAD_KNIGHT)
        {

            // libc::sprintf(
            //     h3_TextBuffer,
            //     "%s is about deal %d damage (Min Possible: %d, Max Possible: %d)\n\nWould do like to set exact
            //     value?", P_CreatureInformation[_this->type].GetCreatureName(_this->numberAlive), realDamage,
            //     minPossibleDamage, maxPossibleDamage);
            // H3Messagebox::Choice(h3_TextBuffer);
        }
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

        /// test

        WriteHiHook(0x0441718, THISCALL_, BattleStack_CalculateDamage);
        WriteHiHook(0x0441767, THISCALL_, BattleStack_CalculateDamage);

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
        CreateAbilityEvent(eCreature::MIGHTY_GORGON, 0x440C00, BattleStack_AfterAttackAbilityRandom);

        // acid breath
        CreateAbilityEvent(eCreature::RUST_DRAGON, 0x4411D7, BattleStack_AfterAttackAbilityRandom);

        // dread knigts double damage
        WriteHiHook(0x04436D9, FASTCALL_, BattleStack_DoubleDamageRandom);

        // luck control
        WriteHiHook(0x0441557, FASTCALL_, BattleStack_LuckRandom); // melee attack
        WriteHiHook(0x043F675, FASTCALL_, BattleStack_LuckRandom); // ranged attack

        // Cyclops: Wall Damage random
        WriteHiHook(0x0445BE0, THISCALL_, BattleStack_AttackWall);
        WriteLoHook(0x0445CBB, BattleStack_MakeBallisticShot);
    }
}

CreatureAttackRandom &CreatureAttackRandom::GetInstance()
{
    if (!instance)
    {
        instance = new CreatureAttackRandom();
    }

    return *instance;
}
