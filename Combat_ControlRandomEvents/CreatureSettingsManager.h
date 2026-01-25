#pragma once
class CreatureSettingsManager : public IGamePatch
{

    CombatCreatureSettings combatCreatureSettings[2][h3::limits::COMBAT_CREATURES];

    BOOL combatIsStarted = false;
    BOOL tacticsPhaseRound = false;

    static CreatureSettingsManager *instance;

  private:
    CreatureSettingsManager();
    virtual ~CreatureSettingsManager() {};

  protected:
    virtual void CreatePatches() override;


    void ResetAllCreatureSettings() noexcept;
  public:
    static CreatureSettingsManager &GetInstance();

  private:
    static void __stdcall BattleMgr_StartBattle(HiHook *h, H3CombatManager *_this);

    static void __stdcall BattleMgr_NewRound(HiHook *h, H3CombatManager *_this);

  public:
    static const CombatCreatureSettings &GetCreatureSettings(const H3CombatCreature *creature) noexcept;
    static const CombatCreatureSettings &GetCreatureSettings(const int side, const int index) noexcept;
    static void SetCreatureSettings(const H3CombatCreature *creature, const CombatCreatureSettings &settings) noexcept;
    static void SetCreatureSettings(const int side, const int index, const CombatCreatureSettings &settings) noexcept;
};
