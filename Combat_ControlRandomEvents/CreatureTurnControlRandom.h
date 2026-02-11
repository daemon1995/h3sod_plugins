#pragma once
class CreatureTurnControlRandom : public IGamePatch
{
    static constexpr LPCSTR instanceName = "SoDPlugin.CreatureTurnControlRandom.daemon_n";
    static CreatureTurnControlRandom *instance;

    const CombatStackSettings *currentSettings = nullptr;
    int currentCreatureSide = -1;

  private:
    CreatureTurnControlRandom();
    virtual ~CreatureTurnControlRandom() {};

  protected:
    virtual void CreatePatches() override;

    static void __stdcall BattleMgr_CheckGoodMorale(HiHook *h, const H3CombatManager *_this, const int side,
                                                    const int index);
    static int __stdcall BattleStack_PositiveMoraleRandom(HiHook *hook, const int min, const int max);

    static int __stdcall BattleMgr_CheckBadMorale(HiHook *h, const H3CombatManager *_this, const int side,
                                                  const int index);
    static int __stdcall BattleStack_NegativeMoraleRandom(HiHook *hook, const int min, const int max);
    static int __stdcall AIBattleStack_NegativeMoraleRandom(HiHook *hook, const int min, const int max);

    static char __stdcall BattleMgr_CheckFear(HiHook *h, const H3CombatManager *_this,
                                              const H3CombatCreature *creature);
    static int __stdcall BattleStack_FearRandom(HiHook *hook, const int min, const int max);

  public:
    static CreatureTurnControlRandom &GetInstance();
};
