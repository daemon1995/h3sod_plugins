#pragma once

struct PluginText;
class CombatSettingsManager : public IGamePatch
{

    enum eLogType
    {
        LOG_TYPE_SCREEN,
        LOG_TYPE_BATTLE_LOG,
        LOG_TYPE_BATTLE_HINT,
    };

    eLogType logType = LOG_TYPE_SCREEN;
    Patch *newRoundPatch = nullptr;
    Patch *endCombatPatch = nullptr;
    //  CombatStackSettings combatStackSettings[2][h3::limits::COMBAT_CREATURES + 1];

    BOOL combatIsStarted = false;
    BOOL tacticsPhaseRound = false;
    INT userMaxControlPoints = 1230;
    INT userControlPoints = 0;
    INT userControlPointsSpent = 0;
    INT userActionsUsed = 0;
    H3String actionsUsedLog;
    PluginText *pluginText = nullptr;
    static CombatSettingsManager *instance;

  private:
    CombatSettingsManager();
    virtual ~CombatSettingsManager() {};

  protected:
    virtual void CreatePatches() override;

  private:
    void ResetCombatSettings() noexcept;
    void SwitchBattleStackAbilityByHotKey(H3CombatManager *mgr, H3Msg *msg);
    void ReportActionUsage(H3CombatManager *mgr, LPCSTR msg, const eLogType logType);
    void SaveActionUsageToLog(H3CombatManager *mgr, const CombatStackSettings *creatureSettings);

  private:
    static void __stdcall BattleMgr_StartBattle(HiHook *h, H3CombatManager *_this);
    static int __stdcall BattleMgr_ProcessAction_KeyPressed(HiHook *h, H3CombatManager *_this, H3Msg *msg);

    static void __stdcall BattleMgr_NewRound(HiHook *h, H3CombatManager *_this);
    static void __stdcall BattleMgr_SetWinner(HiHook *h, H3CombatManager *_this, const INT side);

  public:
    static CombatSettingsManager &GetInstance();

    static int GetUserPoints() noexcept;
    static void SetUserPoints(const int newSize) noexcept;
    static BOOL DecreaseUserPoints(const int toDecrease) noexcept;
};
