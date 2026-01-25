#pragma once
class CreatureMoraleRandom : public IGamePatch
{

    Patch *positiveMoralePatch = nullptr;
    BOOL positiveMoralePatchActive = FALSE;
    Patch *negativeMoralePatch = nullptr;
    BOOL negativeMoralePatchActive = FALSE;
    static CreatureMoraleRandom *instance;

  private:
    CreatureMoraleRandom();
    virtual ~CreatureMoraleRandom() {};

  protected:
    virtual void CreatePatches() override;

    static void __stdcall BattleMgr_CheckGoodMorale(HiHook *h, H3CombatManager *_this, const int side, const int index);
    static int __stdcall BattleMgr_CheckBadMorale(HiHook *h, H3CombatManager *_this, const int side, const int index);

  public:
    static CreatureMoraleRandom &GetInstance();
};
