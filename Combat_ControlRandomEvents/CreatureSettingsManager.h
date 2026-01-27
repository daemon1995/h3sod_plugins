#pragma once

struct PluginText : IPluginText
{

    LPCSTR abilityTexts[eSettingsId::AMOUNT_OF_SETTINGS] = {};

  public:
    void Load() override;
};

class CreatureSettingsManager : public IGamePatch
{

    Patch *newRoundPatch = nullptr;
    Patch *endCombatPatch = nullptr;
    CombatCreatureSettings combatCreatureSettings[2][h3::limits::COMBAT_CREATURES + 1];

    BOOL combatIsStarted = false;
    BOOL tacticsPhaseRound = false;
    INT userControlPoints = 0;
    INT userControlPointsSpent = 0;
    INT userActionsUsed = 0;
    H3String actionsUsedLog;

    static CreatureSettingsManager *instance;

  private:
    CreatureSettingsManager();
    virtual ~CreatureSettingsManager() {};

  protected:
    virtual void CreatePatches() override;

  private:
    void ResetCombatSettings() noexcept;
    void ResetAllcreatureSettings() noexcept;
    void SwitchBattleStackAbilityByHotKey(H3CombatManager *mgr, H3Msg *msg);
    void ReportActionUsage(H3CombatManager *mgr, LPCSTR msg, const BOOL saveLog);

  private:
    static void __stdcall BattleMgr_StartBattle(HiHook *h, H3CombatManager *_this);
    static int __stdcall BattleMgr_ProcessAction_KeyPressed(HiHook *h, H3CombatManager *_this, H3Msg *msg);

    static void __stdcall BattleMgr_NewRound(HiHook *h, H3CombatManager *_this);
    static void __stdcall BattleMgr_SetWinner(HiHook *h, H3CombatManager *_this, const INT side);

  public:
    static CreatureSettingsManager &GetInstance();

    static const CombatCreatureSettings &GetCreatureSettings(const H3CombatCreature *creature) noexcept;
    static const CombatCreatureSettings &GetCreatureSettings(const int side, const int index) noexcept;
    static void SetCreatureSettings(const H3CombatCreature *creature, const CombatCreatureSettings &settings) noexcept;
    static void SetCreatureSettings(const int side, const int index, const CombatCreatureSettings &settings) noexcept;
    static int GetUserPoints() noexcept;
    static void SetUserPoints(const int newSize) noexcept;
    static BOOL DecreaseUserPoints(const int toDecrease) noexcept;
    static void SetAbilityForAllCreatures(const AbilityChanger &ability, const BOOL enable) noexcept;
};
