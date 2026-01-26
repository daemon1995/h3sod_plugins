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
    static constexpr LPCSTR instanceName = "SoDPlugin.CreatureAttackRandom.daemon_n";

    static CreatureAttackRandom *instance;

    const H3CombatCreature *currentCombatCreature = nullptr;
    const CombatCreatureSettings *currentSettings = nullptr;
    INT targetWallId = -1;

    std::unordered_map<eCreature, EventHandler> m_abilitiesMap;

    CreatureAttackRandom();
    virtual ~CreatureAttackRandom() {};

  protected:
    virtual void CreatePatches() override;

  private:
    void CreateAbilityEvent(const eCreature creature, const DWORD patchAddress, const void *functionPtr,
                            const eTriggerState trigger = DEFAULT, const eVKey hotkey = eVKey(0));

  public:
    static char __stdcall BattleStack_AttackMelee(HiHook *hook, const H3CombatCreature *attacker,
                                                  const H3CombatCreature *defender, const int direction);
    static void __stdcall BattleStack_Shoot(HiHook *hook, const H3CombatCreature *attacker,
                                            const H3CombatCreature *target);

    static int __stdcall BattleStack_DamageRandom(HiHook *h, const int min, const int max);
    static int __stdcall BattleStack_AfterAttackAbilityRandom(HiHook *hook, const int min, const int max);
    static int __stdcall BattleStack_DoubleDamageRandom(HiHook *h, const int min, const int max);
    static int __stdcall BattleStack_LuckRandom(HiHook *hook, const int min, const int max);

    static void __stdcall BattleStack_AttackWall(HiHook *h, H3CombatCreature *attacker, const int wallId, int *damages);
    static _LHF_(BattleStack_MakeBallisticShot);

  public:
    static CreatureAttackRandom &GetInstance();
};
