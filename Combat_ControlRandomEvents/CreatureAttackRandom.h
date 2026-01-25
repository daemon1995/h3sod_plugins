#pragma once
#include <unordered_map>

struct EventHandler
{
    BOOL enabled;
    uint32_t ability;
    uint32_t chance;
    eTriggerState trigger = DEFAULT;
    void *functionPointer = nullptr;
    Patch *functionPatch = nullptr;
    eVKey hotkey = eVKey(0);
};

class CreatureAttackRandom : public IGamePatch
{
  public:
    static CreatureAttackRandom *instance;
    const H3CombatCreature *currentCombatCreature = nullptr;

    const CombatCreatureSettings *currentSettings = nullptr;

    Patch *positiveMoralePatch = nullptr;
    BOOL positiveMoralePatchActive = FALSE;
    Patch *negativeMoralePatch = nullptr;
    BOOL negativeMoralePatchActive = FALSE;

    std::unordered_map<eCreature, EventHandler> m_abilitiesMap;

    CreatureAttackRandom();
    virtual ~CreatureAttackRandom() {};

  protected:
    virtual void CreatePatches() override;

  private:
    void CreateAbilityEvent(const eCreature creature, const DWORD patchAddress, const void *functionPtr,
                            const eTriggerState trigger = DEFAULT, const eVKey hotkey = eVKey(0));

  public:
    // static void __stdcall BattleStack_AfterAttackAbility(HiHook *hook, const H3CombatCreature *attacker,
    //                                                      const H3CombatCreature *defender, const int damageDealt,
    //                                                      const int killed, const int a5);

    static int __stdcall BattleStack_AfterAttackAbilityRandom(HiHook *hook, const int min, const int max);

    static char __stdcall BattleStack_AttackMelee(HiHook *hook, const H3CombatCreature *attacker,
                                                  const H3CombatCreature *defender, const int direction);

    static void __stdcall BattleStack_Shoot(HiHook *hook, const H3CombatCreature *attacker,
                                            const H3CombatCreature *target);

    static int __stdcall CombatCreature_DamageRandom(HiHook *h, const int min, const int max);
    static int __stdcall BattleStack_DoubleDamageRandom(HiHook *h, const int min, const int max);
    static int __stdcall BattleStack_RandomToHitTargetedWall(HiHook *h, const int min, const int max);
    static int __stdcall BattleStack_RandomToSelectTargetedWall(HiHook *h, const int min, const int max);

  public:
    static CreatureAttackRandom &GetInstance();
};
