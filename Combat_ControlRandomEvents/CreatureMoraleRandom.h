#pragma once
class CreatureMoraleRandom : public IGamePatch
{

    static CreatureMoraleRandom *instance;
    const CombatCreatureSettings *currentSettings = nullptr;
    static constexpr LPCSTR instanceName = "SoDPlugin.CreatureMoraleRandom.daemon_n";

  private:
    CreatureMoraleRandom();
    virtual ~CreatureMoraleRandom() {};

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
    static CreatureMoraleRandom &GetInstance();
};
