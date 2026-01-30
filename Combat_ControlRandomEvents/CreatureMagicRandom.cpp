#include "framework.h"

#include "CreatureMagicRandom.h"

#include <unordered_set>
BOOL CreateAvailableSpellsList(const H3CombatCreature *creature, std::vector<eSpell> &outList)
{

    outList.clear();
    outList.reserve(h3::limits::SPELLS);
    std::unordered_set<eSpell> spellsSet;
    const CreatureSpellData *spellDataArray = nullptr;
    int *faerieDragonSpells = nullptr;
    switch (creature->type)
    {

    case eCreature::MASTER_GENIE:
        for (size_t i = eSpell::QUICK_SAND; i < eSpell::STONE; i++)
        {
            if (P_Spell[i].friendlyMass)
            {
                outList.push_back(static_cast<eSpell>(i));
            }
        }

        break;
    case eCreature::ENCHANTER:
        spellDataArray = CreatureSpellData::GetEnchantersArray();
        for (size_t i = 0; i < CreatureSpellData::ENCHANTERS_ARRAY_SIZE; i++)
        {
            spellsSet.insert(spellDataArray[i].spellId);
        }
        break;
    case eCreature::FAERIE_DRAGON:
        spellDataArray = CreatureSpellData::GetFaerieDragonArray();
        for (size_t i = 0; i < CreatureSpellData::FAERIE_DRAGON_ARRAY_SIZE; i++)
        {
            if (spellDataArray[i].chanceToCast > 0)
            {
                spellsSet.insert(static_cast<eSpell>(spellDataArray[i].spellId));
            }
        }
        break;
    default:
        return FALSE;
    }

    if (outList.empty() && !spellsSet.empty())
    {
        for (const auto &spell : spellsSet)
        {
            outList.push_back(spell);
        }
    }

    outList.shrink_to_fit();

    return TRUE;
}

CreatureMagicRandom *CreatureMagicRandom::instance = nullptr;
CreatureMagicRandom::CreatureMagicRandom() : IGamePatch(globalPatcher->CreateInstance(instanceName))
{
    CreatePatches();
}

_LHF_(BattleStack_PrepareFaerieDragonSpell)
{
    if (const auto &creature = reinterpret_cast<H3CombatCreature *>(c->edi))
    {

        if (!CreatureSettingsManager::GetUserPoints())
            return EXEC_DEFAULT;

        const auto &creatureSettings = CreatureSettingsManager::GetCreatureSettings(creature);

        const eSpell setSpellByUser = creatureSettings.At(eSettingsId::SPELL_CASTING).spellToCast;
        if (setSpellByUser != eSpell::NONE)
        {
            c->eax = setSpellByUser;
            CreatureSettingsManager::DecreaseUserPoints(1);
        }
    }

    return EXEC_DEFAULT;
}

char __stdcall BattleStack_EnchanterCastsMassSpell(HiHook *h, H3CombatCreature *creature)
{

    // check if user has points
    if (CreatureSettingsManager::GetUserPoints())
    {

        CreatureSpellData *data = CreatureSpellData::GetEnchantersArray();

        const CreatureSpellData storedData = data[0];

        const auto &creatureSettings = CreatureSettingsManager::GetCreatureSettings(creature);
        const eSpell setSpellByUser = creatureSettings.At(eSettingsId::SPELL_CASTING).spellToCast;
        if (setSpellByUser != eSpell::NONE)
        {
            // emulate first spell in array being the one selected by user
            data[0].spellId = eSpell::PRAYER; // setSpellByUser;
            data[0].chanceToCast = 100;

            // call pseudo original function to check if spell can be cast
            char ret = THISCALL_1(char, 0x0447D00, creature);
            data[0] = storedData;
            // if spell can be cast, decrease user points and return success
            if (ret)
            {
                CreatureSettingsManager::DecreaseUserPoints(1);
                return true;
            }
        }
    }
    // retutrn default
    return THISCALL_1(char, h->GetDefaultFunc(), creature);
}
void CreatureMagicRandom::CreatePatches()
{

    WriteLoHook(0x04472A8, BattleStack_PrepareFaerieDragonSpell);
    WriteHiHook(0x04650F9, THISCALL_, BattleStack_EnchanterCastsMassSpell);
}

CreatureMagicRandom &CreatureMagicRandom::GetInstance()
{
    if (!instance)
    {
        instance = new CreatureMagicRandom();
    }

    return *instance;
}

SpellSelectionDlg::SpellSelectionDlg(const H3CombatCreature *creature, const BOOL isPopup, const int width,
                                     const int height)
    : H3Dlg(width, height)
{

    CreateAvailableSpellsList(creature, availableSpells);

    const size_t spellCount = availableSpells.size();
    int itemId = 1;
    for (size_t row = 0; row < 5; row++)
    {
        for (size_t column = 0; column < 5; column++)
        {
            const size_t spellIndex = row * 5 + column;
            if (spellIndex < spellCount)
            {
                const int x = 10 + column * 34;
                const int y = 10 + row * 34;
                const eSpell spell = availableSpells[spellIndex];
                H3DlgDef *spellItem = H3DlgDef::Create(x, y, itemId++, "spellint.def", spell + 1);
                AddItem(spellItem);
            }
        }
    }
    if (!isPopup)
    {
        CreateOKButton();
    }
}

BOOL SpellSelectionDlg::DialogProc(H3Msg &msg)
{
    return 0;
}
eSpell SpellSelectionDlg::ShowSettingsDlg(H3CombatCreature *creature, const H3Msg *msg)
{

    const BOOL isPopup = msg->IsRightClick();
    auto &dlg = SpellSelectionDlg(creature, isPopup);

    isPopup ? dlg.RMB_Show() : dlg.Start();

    return dlg.selectedSpell;
}
