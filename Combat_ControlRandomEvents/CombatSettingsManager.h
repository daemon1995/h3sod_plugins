#pragma once

struct PluginText;

class CombatSettingsManager : public IGamePatch
{

    enum eLogTargetType
    {
        LOG_TYPE_SCREEN,
        LOG_TYPE_BATTLE_LOG,
        LOG_TYPE_BATTLE_HINT,
    };

    struct CombatUniqueInfo
    {
        H3Position position;
        H3MapItem *mapItem = nullptr;
        H3Hero *heroes[2] = {nullptr, nullptr};
        UINT gameDay = 0;

        bool operator!=(const CombatUniqueInfo &other) const noexcept
        {
            return position != other.position || mapItem != other.mapItem || heroes[0] != other.heroes[0] ||
                   heroes[1] != other.heroes[1] || gameDay != other.gameDay;
        }
    } combatUniqueInfo;

    eLogTargetType logType = LOG_TYPE_SCREEN;
    Patch *newRoundPatch = nullptr;
    Patch *endCombatPatch = nullptr;
    //  CombatStackSettings combatStackSettings[2][h3::limits::COMBAT_CREATURES + 1];

    BOOL combatIsStarted = false;
    BOOL tacticsPhaseRound = false;
    INT userMaxControlPoints = 5;
    INT userControlPoints = 0;
    INT userControlPointsSpent = 0;
    INT userActionsUsed = 0;
    BOOL isCheaterBeforeCombat = false;
    BOOL cheaterFlagSet = false;
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
    static void WriteMessageToLog(LPCSTR msg, const eLogTargetType logType = LOG_TYPE_SCREEN);
    static void ReportActionUsage(const CombatStackSettings *creatureSettings, const CombatSideSettings *side,
                                  const int settingId, const eLogTargetType logType = LOG_TYPE_SCREEN);
};
