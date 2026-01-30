#pragma once

struct CreatureSpellData
{
    static constexpr DWORD ENCHANTERS_ARRAY_ADDRESS = 0x06608B8;
    static constexpr DWORD ENCHANTERS_ARRAY_PTR_ADDRESS = 0x0447D2B + 1;
    static constexpr size_t ENCHANTERS_ARRAY_SIZE = 8;

    static constexpr DWORD FAERIE_DRAGON_ARRAY_ADDRESS = 0x063B850;
    static constexpr DWORD FAERIE_DRAGON_ARRAY_PTR_ADDRESS = 0x0447251 + 1;
    static constexpr size_t FAERIE_DRAGON_ARRAY_SIZE = 8;

  public:
    eSpell spellId;
    int chanceToCast;

  public:
    static CreatureSpellData *GetEnchantersArray()
    {
        return *reinterpret_cast<CreatureSpellData **>(ENCHANTERS_ARRAY_PTR_ADDRESS);
    }
    static CreatureSpellData *GetFaerieDragonArray()
    {
        return *reinterpret_cast<CreatureSpellData **>(FAERIE_DRAGON_ARRAY_PTR_ADDRESS);
    }
};
class CreatureMagicRandom : IGamePatch
{
    static constexpr LPCSTR instanceName = "SoDPlugin.CreatureAttackRandom.daemon_n";
    static CreatureMagicRandom *instance;

  private:
    CreatureMagicRandom();
    virtual ~CreatureMagicRandom() {};

  protected:
    virtual void CreatePatches() override;

  private:
  public:
    static CreatureMagicRandom &GetInstance();
};

class SpellSelectionDlg : H3Dlg
{
    virtual BOOL DialogProc(H3Msg &msg) override;
    std::vector<eSpell> availableSpells;

    eSpell selectedSpell = eSpell::NONE;

    SpellSelectionDlg(const H3CombatCreature *creature, const BOOL isPopup, const int width = 400,
                      const int height = 300);

  public:
    static eSpell ShowSettingsDlg(H3CombatCreature *creature, const H3Msg *msg);
};
