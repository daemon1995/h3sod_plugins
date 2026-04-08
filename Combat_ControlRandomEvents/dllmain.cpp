// dllmain.cpp : Определяет точку входа для приложения DLL.
#define _H3API_PLUGINS_
#include "framework.h"

#include "PluginText.h"

#include "CreatureAttackRandom.h"
#include "CreatureMagicRandom.h"
#include "CreatureTurnControlRandom.h"

class CombatStackSettingsDlg : public H3Dlg
{

  public:
    static void __stdcall BattleMgr_ShowMonStatDlg(HiHook *hook, H3CombatManager *mgr, H3CombatCreature *creature,
                                                   BOOL isRightClick);
};

using namespace h3;

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
LPCSTR pluginVersion = "0.9.5";
} // namespace dllText

_LHF_(GameWindowShow)
{
    // H3String string(dllText::pluginVersion);
    // H3Messagebox::Show(dllText::pluginVersion);
    return EXEC_DEFAULT;
}
_LHF_(HooksInit)
{
    _PI->WriteLoHook(0x4EEE31, GameWindowShow);

    CreatureAttackRandom::GetInstance();
    CombatSettingsManager::GetInstance();
    CreatureTurnControlRandom::GetInstance();
    CreatureMagicRandom::GetInstance();

    _PI->WriteHiHook(0x468440, THISCALL_, CombatStackSettingsDlg::BattleMgr_ShowMonStatDlg);
    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool initialized = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!initialized)
        {
            initialized = true;

            const H3Version gameVersion;
            if (gameVersion.sod() || gameVersion.version() == H3Version::GameVersion::SOD_POLISH_GOLD)
            {
                globalPatcher = GetPatcher();
                _PI = globalPatcher->CreateInstance(dllText::instanceName);
                _PI->WriteLoHook(0x4EEAF2, HooksInit);
            }
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
