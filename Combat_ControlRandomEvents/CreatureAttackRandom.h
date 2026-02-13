#pragma once
#include <unordered_map>
#include <unordered_set>

struct EventHandler
{
    void *functionPointer = nullptr;
    Patch *functionPatch = nullptr;
};
union PossibleDamage {
    struct
    {
        int minDamage;
        int realDamage;
        int maxDamage;
    };
    int damageValues[3]{-1, -1, -1};
};

class CreatureAttackRandom : public IGamePatch
{
    static constexpr LPCSTR instanceName = "SoDPlugin.CreatureAttackRandom.daemon_n";

    static CreatureAttackRandom *instance;

    const H3CombatCreature *attackInitiator = nullptr;
    const H3CombatCreature *currentCombatCreature = nullptr;
    CombatStackSettings *currentSettings = nullptr;
    const Ability *currentDamageAbility = nullptr;
    BOOL8 stacksAttackedAtLeastOnce[2][h3::limits::TOTAL_COMBAT_CREATURES]{{}};

    // used to store infor about real damage dealt in case of abilities that trigger after attack and can change damage
    // dealt (e.g. after attack ability, double damage, luck)
    struct
    {
        BOOL isDamageEmulation = false;
        BOOL isSecondAttack = false;
        BOOL isLuckTriggered = false;
        BOOL isDoubleDamageTriggered = false;
    } attackerActionData;

    INT targetWallId = -1;

    std::unordered_map<DWORD, void *> m_abilitiesPatchesMap;
    std::unordered_set<int> m_creaturesWithAfterAttackAbility;
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
    static int __stdcall BattleStack_CalculateDamageToMonster(HiHook *h, H3CombatCreature *_this,
                                                              H3CombatCreature *targetCreature, int baseDamage,
                                                              char shoot, char isTheoretical, int stepsTaken,
                                                              int *fireShieldDmg);
    static int __stdcall BattleStack_AfterAttackAbilityRandom(HiHook *hook, const int min, const int max);
    static int __stdcall BattleStack_DeathStareAbility(HiHook *hook, const int min, const int max);
    static int __stdcall BattleStack_DoubleDamageRandom(HiHook *h, const int min, const int max);
    static _LHF_(BattleStack_DoubleDamageTrigger);
    static int __stdcall BattleStack_LuckRandom(HiHook *hook, const int min, const int max);

    static void __stdcall BattleStack_CatapultShot(HiHook *h, H3CombatCreature *attacker, const int targetHex);
    static void __stdcall BattleStack_AttackWall(HiHook *h, H3CombatCreature *attacker, const int wallId, int *damages);
    static _LHF_(BattleStack_MakeBallisticShot);

  public:
    static BOOL BattleStack_HasAfterAttackAbility(const H3CombatCreature *creature);
    static CreatureAttackRandom &GetInstance();
};

class DamageInputDlg : H3Dlg
{
  protected:
    // virtual BOOL DialogProc(H3Msg &msg) override;
    //    virtual BOOL OnDoubleClick(INT itemID, H3Msg& msg) override;
    virtual void OnOK() override;
    //   virtual void OnCancel() override;
    virtual BOOL OnKeyPress(eVKey key, eMsgFlag flag) override;

  protected:
    const H3CombatCreature *attacker = nullptr;
    const H3CombatCreature *target = nullptr;

    const PossibleDamage &basePossibleDamage;
    const PossibleDamage &finalPossibleDamage;

    H3DlgEdit *userInputDamage = nullptr;

    int resultBaseDamage = 0;
    union DamageRow {
        struct
        {

            H3DlgText *minText;
            H3DlgText *realText;
            H3DlgText *maxText;
        };
        H3DlgItem *damageItems[3]{};
    } baseDamageRow, finalDamageRow;
    //    H3DlgText *baseDamageText = nullptr;
    //  H3DlgText *finalDamageText = nullptr;
    H3DlgDefButton *okButton = nullptr;
    H3DlgDefButton *cancelButton = nullptr;

    DamageInputDlg(const H3CombatCreature *attacker, const H3CombatCreature *target,
                   const PossibleDamage &basePossibleDamage, const PossibleDamage &finalPossibleDamage,
                   const int width = 400, const int height = 300);

  protected:
    void CreateDamageRow(const PossibleDamage &possibleDamage, DamageRow &damageRow, const int y);
    void UpdateInputDamage();

  public:
    static int ShowInputDamageDlg(const H3CombatCreature *attacker, const H3CombatCreature *target,
                                  const PossibleDamage &basePossibleDamage, const PossibleDamage &finalPossibleDamage);
};
