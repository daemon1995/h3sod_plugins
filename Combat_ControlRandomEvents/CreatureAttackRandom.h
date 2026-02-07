#pragma once
#include <unordered_map>

struct EventHandler
{
    BOOL enabled;
    void *functionPointer = nullptr;
    Patch *functionPatch = nullptr;
};

class CreatureAttackRandom : public IGamePatch
{
    static constexpr LPCSTR instanceName = "SoDPlugin.CreatureAttackRandom.daemon_n";

    static CreatureAttackRandom *instance;

    const H3CombatCreature *currentCombatCreature = nullptr;
    const CombatStackSettings *currentSettings = nullptr;
    const AbilityChanger *currentDamageAbility = nullptr;
    bool stacksAttackedAtLeastOnce[2][21]{{}};
    BOOL useSecondAttack = false;
    INT targetWallId = -1;

    std::unordered_map<eCreature, EventHandler> m_abilitiesMap;

    CreatureAttackRandom();
    virtual ~CreatureAttackRandom() {};

  protected:
    virtual void CreatePatches() override;

  private:
    void CreateAbilityEvent(const eCreature creature, const DWORD patchAddress, void *functionPtr);
    void ResetAfterAttackState();

  private:
    static void __stdcall BattleStack_AttackMelee_Prepare(HiHook *hook, const H3CombatCreature *attacker,
                                                          const int direction);
    static char __stdcall BattleStack_AttackMelee(HiHook *hook, const H3CombatCreature *attacker,
                                                  const H3CombatCreature *defender, const int direction);
    static void __stdcall BattleStack_Shoot_Prepare(HiHook *hook, const H3CombatCreature *attacker);
    static void __stdcall BattleStack_Shoot(HiHook *hook, const H3CombatCreature *attacker,
                                            const H3CombatCreature *target);

    static int __stdcall BattleStack_DamageRandom(HiHook *h, const int min, const int max);
    static int __stdcall BattleStack_AfterAttackAbilityRandom(HiHook *hook, const int min, const int max);
    static int __stdcall BattleStack_DoubleDamageRandom(HiHook *h, const int min, const int max);
    static int __stdcall BattleStack_LuckRandom(HiHook *hook, const int min, const int max);

    static void __stdcall BattleStack_AttackWall(HiHook *h, H3CombatCreature *attacker, const int wallId, int *damages);
    static _LHF_(BattleStack_MakeBallisticShot);

  public:
    static BOOL BattleStack_HasAfterAttackAbility(const H3CombatCreature *creature);

    static CreatureAttackRandom &GetInstance();
};
