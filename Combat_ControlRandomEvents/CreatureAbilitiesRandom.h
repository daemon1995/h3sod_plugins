#pragma once
#include <unordered_map>

enum eAbilityTrigger : uint8_t
{
    DEFAULT = 0,
    ALWAYS = 1,
    NEVER = 2,
};

struct EventHandler
{
    BOOL enabled;
    uint32_t ability;
    uint32_t chance;
    eAbilityTrigger trigger = DEFAULT;
    void *functionPointer = nullptr;
    Patch *functionPatch = nullptr;
    eVKey hotkey = eVKey(0);
};

class CreatureAbilitiesRandom : public IGamePatch
{
    static CreatureAbilitiesRandom *instance;
    eCreature currentCreatureId = eCreature::UNDEFINED;
    std::unordered_map<eCreature, EventHandler> m_abilitiesMap;

    CreatureAbilitiesRandom();
    virtual ~CreatureAbilitiesRandom() {};

  protected:
    virtual void CreatePatches() override;

  private:
    void CreateAbilityEvent(const eCreature creature, const DWORD patchAddress, const void *functionPtr,
                            const eAbilityTrigger trigger = DEFAULT, const eVKey hotkey = eVKey(0));

  public:
    static void __stdcall BattleStack_AfterAttackAbility(HiHook *hook, const H3CombatCreature *attacker,
                                                         const H3CombatCreature *defender, const int damageDealt,
                                                         const int killed, const int a5);

    static char __stdcall BattleStack__SetAfterHitSpellAtTarget(HiHook *hook, const H3CombatCreature *attacker,
                                                                const H3CombatCreature *defender);
    static int __stdcall BattleStack_AfterAttackAbilityRandom(HiHook *hook, const int min, const int max);

  public:
    static CreatureAbilitiesRandom &GetInstance();
};
