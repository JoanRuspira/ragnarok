// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "status.hpp"

#include <functional>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <yaml-cpp/yaml.h>

#include "../common/cbasetypes.hpp"
#include "../common/ers.hpp"
#include "../common/malloc.hpp"
#include "../common/nullpo.hpp"
#include "../common/random.hpp"
#include "../common/showmsg.hpp"
#include "../common/strlib.hpp"
#include "../common/timer.hpp"
#include "../common/utilities.hpp"
#include "../common/utils.hpp"

#include "battle.hpp"
#include "clif.hpp"
#include "elemental.hpp"
#include "guild.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "path.hpp"
#include "pc.hpp"
#include "pc_groups.hpp"
#include "pet.hpp"
#include "script.hpp"

using namespace rathena;

// Regen related flags.
enum e_regen {
	RGN_NONE = 0x00,
	RGN_HP   = 0x01,
	RGN_SP   = 0x02,
	RGN_SHP  = 0x04,
	RGN_SSP  = 0x08,
};

// Bonus values and upgrade chances for refining equipment
static struct {
	int chance[REFINE_CHANCE_TYPE_MAX][MAX_REFINE]; /// Success chance
	int bonus[MAX_REFINE]; /// Cumulative fixed bonus damage
	int randombonus_max[MAX_REFINE]; /// Cumulative maximum random bonus damage
	struct refine_cost cost[REFINE_COST_MAX];
} refine_info[REFINE_TYPE_MAX];

static struct eri *sc_data_ers; /// For sc_data entries
static struct status_data dummy_status;

short current_equip_item_index; /// Contains inventory index of an equipped item. To pass it into the EQUP_SCRIPT [Lupus]
unsigned int current_equip_combo_pos; /// For combo items we need to save the position of all involved items here
int current_equip_card_id; /// To prevent card-stacking (from jA) [Skotlex]
// We need it for new cards 15 Feb 2005, to check if the combo cards are insrerted into the CURRENT weapon only to avoid cards exploits
short current_equip_opt_index; /// Contains random option index of an equipped item. [Secret]

unsigned int SCDisabled[SC_MAX]; ///< List of disabled SC on map zones. [Cydh]

sc_type SkillStatusChangeTable[MAX_SKILL];
int StatusIconChangeTable[SC_MAX];
unsigned int StatusChangeFlagTable[SC_MAX];
int StatusSkillChangeTable[SC_MAX];
int StatusRelevantBLTypes[EFST_MAX];
unsigned int StatusChangeStateTable[SC_MAX];
unsigned int StatusDisplayType[SC_MAX];

static unsigned short status_calc_str(struct block_list *,struct status_change *,int);
static unsigned short status_calc_agi(struct block_list *,struct status_change *,int);
static unsigned short status_calc_vit(struct block_list *,struct status_change *,int);
static unsigned short status_calc_int(struct block_list *,struct status_change *,int);
static unsigned short status_calc_dex(struct block_list *,struct status_change *,int);
static unsigned short status_calc_luk(struct block_list *,struct status_change *,int);
static unsigned short status_calc_batk(struct block_list *,struct status_change *,int);
static unsigned short status_calc_watk(struct block_list *,struct status_change *,int);
static unsigned short status_calc_matk(struct block_list *,struct status_change *,int);
static signed short status_calc_hit(struct block_list *,struct status_change *,int);
static signed short status_calc_critical(struct block_list *,struct status_change *,int);
static signed short status_calc_flee(struct block_list *,struct status_change *,int);
static signed short status_calc_flee2(struct block_list *,struct status_change *,int);
static defType status_calc_def(struct block_list *bl, struct status_change *sc, int);
static signed short status_calc_def2(struct block_list *,struct status_change *,int);
static defType status_calc_mdef(struct block_list *bl, struct status_change *sc, int);
static signed short status_calc_mdef2(struct block_list *,struct status_change *,int);
static unsigned short status_calc_speed(struct block_list *,struct status_change *,int);
static short status_calc_aspd_rate(struct block_list *,struct status_change *,int);
static unsigned short status_calc_dmotion(struct block_list *bl, struct status_change *sc, int dmotion);
#ifdef RENEWAL_ASPD
static short status_calc_aspd(struct block_list *bl, struct status_change *sc, bool fixed);
#endif
static short status_calc_fix_aspd(struct block_list *bl, struct status_change *sc, int);
static unsigned int status_calc_maxhp(struct block_list *bl, uint64 maxhp);
static unsigned int status_calc_maxsp(struct block_list *bl, uint64 maxsp);
static unsigned char status_calc_element(struct block_list *bl, struct status_change *sc, int element);
static unsigned char status_calc_element_lv(struct block_list *bl, struct status_change *sc, int lv);
static enum e_mode status_calc_mode(struct block_list *bl, struct status_change *sc, enum e_mode mode);
#ifdef RENEWAL
static unsigned short status_calc_ematk(struct block_list *,struct status_change *,int);
#endif
static int status_get_hpbonus(struct block_list *bl, enum e_status_bonus type);
static int status_get_spbonus(struct block_list *bl, enum e_status_bonus type);
static unsigned int status_calc_maxhpsp_pc(struct map_session_data* sd, unsigned int stat, bool isHP);
static int status_get_sc_interval(enum sc_type type);

static bool status_change_isDisabledOnMap_(sc_type type, bool mapIsVS, bool mapIsPVP, bool mapIsGVG, bool mapIsBG, unsigned int mapZone, bool mapIsTE);
#define status_change_isDisabledOnMap(type, m) ( status_change_isDisabledOnMap_((type), mapdata_flag_vs2((m)), m->flag[MF_PVP] != 0, mapdata_flag_gvg2_no_te((m)), m->flag[MF_BATTLEGROUND] != 0, (m->zone << 3) != 0, mapdata_flag_gvg2_te((m))) )

const std::string SizeFixDatabase::getDefaultLocation() {
	return std::string(db_path) + "/size_fix.yml";
}

/**
 * Reads and parses an entry from size_fix.
 * @param node: YAML node containing the entry.
 * @return count of successfully parsed rows
 */
uint64 SizeFixDatabase::parseBodyNode(const YAML::Node &node) {
	std::string weapon_name;

	if (!this->asString(node, "Weapon", weapon_name))
		return 0;

	std::string weapon_name_constant = "W_" + weapon_name;
	int64 constant;

	if (!script_get_constant(weapon_name_constant.c_str(), &constant)) {
		this->invalidWarning(node["Weapon"], "Size Fix unknown weapon %s, skipping.\n", weapon_name.c_str());
		return 0;
	}

	if (constant < W_FIST || constant > W_2HSTAFF) {
		this->invalidWarning(node["Weapon"], "Size Fix weapon %s is an invalid weapon, skipping.\n", weapon_name.c_str());
		return 0;
	}

	int weapon_id = static_cast<int>(constant);
	std::shared_ptr<s_sizefix_db> size = this->find(weapon_id);
	bool exists = size != nullptr;

	if (!exists)
		size = std::make_shared<s_sizefix_db>();

	if (this->nodeExists(node, "Small")) {
		uint16 small;

		if (!this->asUInt16(node, "Small", small))
			return 0;

		if (small > 100) {
			this->invalidWarning(node["Small"], "Small Size Fix %d for %s is out of bounds, defaulting to 100.\n", small, weapon_name.c_str());
			small = 100;
		}

		size->small = small;
	} else {
		if (!exists)
			size->small = 100;
	}

	if (this->nodeExists(node, "Medium")) {
		uint16 medium;

		if (!this->asUInt16(node, "Medium", medium))
			return 0;

		if (medium > 100) {
			this->invalidWarning(node["Medium"], "Medium Size Fix %d for %s is out of bounds, defaulting to 100.\n", medium, weapon_name.c_str());
			medium = 100;
		}

		size->medium = medium;
	} else {
		if (!exists)
			size->medium = 100;
	}

	if (this->nodeExists(node, "Large")) {
		uint16 large;

		if (!this->asUInt16(node, "Large", large))
			return 0;

		if (large > 100) {
			this->invalidWarning(node["Large"], "Large Size Fix %d for %s is out of bounds, defaulting to 100.\n", large, weapon_name.c_str());
			large = 100;
		}

		size->large = large;
	} else {
		if (!exists)
			size->large = 100;
	}

	if (!exists)
		this->put(weapon_id, size);

	return 1;
}

SizeFixDatabase size_fix_db;

/**
 * Returns the status change associated with a skill.
 * @param skill The skill to look up
 * @return The status registered for this skill
 */
sc_type status_skill2sc(int skill)
{
	int idx = skill_get_index(skill);
	if( idx == 0 ) {
		ShowError("status_skill2sc: Unsupported skill id %d\n", skill);
		return STATUS_NONE;
	}
	return SkillStatusChangeTable[idx];
}

/**
 * Returns the FIRST skill (in order of definition in initChangeTables) to use a given status change.
 * Utilized for various duration lookups. Use with caution!
 * @param sc The status to look up
 * @return A skill associated with the status
 */
int status_sc2skill(sc_type sc)
{
	if( sc < 0 || sc >= SC_MAX ) {
		ShowError("status_sc2skill: Unsupported status change id %d\n", sc);
		return 0;
	}

	return StatusSkillChangeTable[sc];
}

/**
 * Returns the status calculation flag associated with a given status change.
 * @param sc The status to look up
 * @return The scb_flag registered for this status (see enum scb_flag)
 */
unsigned int status_sc2scb_flag(sc_type sc)
{
	if( sc < 0 || sc >= SC_MAX ) {
		ShowError("status_sc2scb_flag: Unsupported status change id %d\n", sc);
		return SCB_NONE;
	}

	return StatusChangeFlagTable[sc];
}

/**
 * Returns the bl types which require a status change packet to be sent for a given client status identifier.
 * @param type The client-side status identifier to look up (see enum efst_types)
 * @return The bl types relevant to the type (see enum bl_type)
 */
int status_type2relevant_bl_types(int type)
{
	if( type < EFST_BLANK || type >= EFST_MAX ) {
		ShowError("status_type2relevant_bl_types: Unsupported type %d\n", type);
		return EFST_BLANK;
	}

	return StatusRelevantBLTypes[type];
}

#define add_sc(skill,sc) set_sc(skill,sc,EFST_BLANK,SCB_NONE)
// Indicates that the status displays a visual effect for the affected unit, and should be sent to the client for all supported units
#define set_sc_with_vfx(skill, sc, icon, flag) set_sc((skill), (sc), (icon), (flag)); if((icon) < EFST_MAX) StatusRelevantBLTypes[(icon)] |= BL_SCEFFECT

static void set_sc(uint16 skill_id, sc_type sc, int icon, unsigned int flag)
{
	uint16 idx = skill_get_index(skill_id);
	if( idx == 0 ) {
		ShowError("set_sc: Unsupported skill id %d (SC: %d. Icon: %d)\n", skill_id, sc, icon);
		return;
	}
	if( sc <= STATUS_NONE || sc >= SC_MAX ) {
		ShowError("set_sc: Unsupported status change id %d (Skill: %d. Icon: %d)\n", sc, skill_id, icon);
		return;
	}

	if( StatusSkillChangeTable[sc] == 0 )
		StatusSkillChangeTable[sc] = skill_id;
	if( StatusIconChangeTable[sc] == EFST_BLANK )
		StatusIconChangeTable[sc] = icon;
	StatusChangeFlagTable[sc] |= flag;

	if( SkillStatusChangeTable[idx] == STATUS_NONE )
		SkillStatusChangeTable[idx] = sc;
}

static void set_sc_with_vfx_noskill(sc_type sc, int icon, unsigned flag) {
	if (sc > STATUS_NONE && sc < SC_MAX) {
		if (StatusIconChangeTable[sc] == EFST_BLANK)
			StatusIconChangeTable[sc] = icon;
		StatusChangeFlagTable[sc] |= flag;
	}
	if (icon > EFST_BLANK && icon < EFST_MAX)
		StatusRelevantBLTypes[icon] |= BL_SCEFFECT;
}

void initChangeTables(void)
{
	int i;

	for (i = 0; i < SC_MAX; i++)
		StatusIconChangeTable[i] = EFST_BLANK;

	for (i = 0; i < MAX_SKILL; i++)
		SkillStatusChangeTable[i] = STATUS_NONE;

	for (i = 0; i < EFST_MAX; i++)
		StatusRelevantBLTypes[i] = BL_PC;

	memset(StatusSkillChangeTable, 0, sizeof(StatusSkillChangeTable));
	memset(StatusChangeFlagTable, 0, sizeof(StatusChangeFlagTable));
	memset(StatusChangeStateTable, 0, sizeof(StatusChangeStateTable));
	memset(StatusDisplayType, 0, sizeof(StatusDisplayType));
	memset(SCDisabled, 0, sizeof(SCDisabled));

	/* First we define the skill for common ailments. These are used in skill_additional_effect through sc cards. [Skotlex] */


	/* The main status definitions */
	add_sc( SK_SM_BASH			, STATUS_STUN		);
	set_sc( SK_SM_PROVOKE		, STATUS_PROVOKE		, EFST_PROVOKE		, SCB_DEF|SCB_DEF2|SCB_BATK|SCB_WATK );
	set_sc( SK_AL_SIGNUMCRUCIS		, STATUS_CRUCIS_PLAYER	, EFST_CRUCIS_PLAYER	, SCB_MATK|SCB_MDEF	);
	set_sc( SK_SM_MAGNUM		, STATUS_WATK_ELEMENT	, EFST_WATK_ELEMENT	, SCB_NONE	);
	set_sc( SK_MC_COINFLIP		, STATUS_COINFLIP_DEX	, EFST_COIN_DEX	, SCB_DEX	);
	set_sc( SK_MC_COINFLIP		, STATUS_COINFLIP_LUK	, EFST_COIN_LUK	, SCB_LUK	);
	set_sc( SK_SO_ASTRALSTRIKE		, STATUS_ASTRALSTRIKE_DEBUFF	, EFST_ASTRALSTRIKE_DEBUFF	, SCB_NONE	);
	set_sc( SK_BS_ENDURE		, STATUS_ENDURE		, EFST_ENDURE		, SCB_MDEF|SCB_DSPD|SCB_DEF );
	add_sc( SK_MG_FROSTDIVER		, STATUS_FREEZE		);
	add_sc( SK_MG_STONECURSE		, STATUS_STONECURSE		);
	add_sc( SK_AL_RUWACH		, STATUS_RUWACH		);
	set_sc( SK_AL_INCREASEAGI		, STATUS_INCREASEAGI	, EFST_INC_AGI, SCB_AGI|SCB_SPEED|SCB_ASPD );
	set_sc( SK_MO_DECAGI		, STATUS_DECREASEAGI	, EFST_DEC_AGI, SCB_AGI|SCB_SPEED );

	set_sc( SK_AL_ANGELUS		, STATUS_ANGELUS		, EFST_ANGELUS		, SCB_DEF2|SCB_MAXHP );

	set_sc( SK_AL_BLESSING		, STATUS_BLESSING		, EFST_BLESSING		, SCB_STR|SCB_INT|SCB_DEX);

	set_sc( SK_AM_PETROLOGY		, STATUS_PETROLOGY		, EFST_LANDBOOST		, SCB_NONE);
	set_sc( SK_AM_PYROTECHNIA		, STATUS_PYROTECHNIA		, EFST_FIREBOOST		, SCB_NONE);
	set_sc( SK_CR_HARMONIZE		, STATUS_HARMONIZE		, EFST_NORMALBOOST		, SCB_NONE);

	set_sc( SK_SA_FIREINSIGNIA		, STATUS_FIREINSIGNIA		, EFST_FIREINSIGNIA		, SCB_NONE);
	set_sc( SK_SA_WATERINSIGNIA		, STATUS_WATERINSIGNIA		, EFST_WATERINSIGNIA		, SCB_NONE);
	set_sc( SK_SA_EARTHINSIGNIA		, STATUS_EARTHINSIGNIA		, EFST_EARTHINSIGNIA		, SCB_NONE);
	set_sc( SK_SA_WINDINSIGNIA		, STATUS_WINDINSIGNIA		, EFST_WINDINSIGNIA		, SCB_NONE);
	
	
	set_sc( SK_AC_IMPROVECONCENTRATION	, STATUS_IMPROVECONCENTRATION	, EFST_CONCENTRATION, SCB_AGI|SCB_DEX );
	set_sc( SK_RA_SPIRITANIMAL	, STATUS_SPIRITANIMAL	, EFST_SPIRITANIMAL, SCB_INT|SCB_MATK );
	set_sc( SK_TF_HIDING		, STATUS_HIDING		, EFST_HIDING		, SCB_SPEED );
	add_sc( SK_TF_POISONSLASH		, STATUS_POISON		);
	set_sc( SK_KN_COUNTERATTACK		, STATUS_AUTOCOUNTER	, EFST_AUTOCOUNTER	, SCB_NONE );
	set_sc( SK_PR_IMPOSITIO		, STATUS_IMPOSITIOMANUS		, EFST_IMPOSITIO	, SCB_WATK);
	set_sc(SK_MS_ARMORREINFORCEMENT, STATUS_ARMORREINFORCEMENT, EFST_REINFORCEMENT, SCB_DEF);

	set_sc( SK_PR_SUFFRAGIUM		, STATUS_SUFFRAGIUM		, EFST_SUFFRAGIUM		, SCB_NONE );
	set_sc( SK_PR_ASPERSIO		, STATUS_ASPERSIO		, EFST_ASPERSIO		, SCB_ATK_ELE );
	set_sc( SK_BI_BENEDICTIO		, STATUS_BENEDICTIO		, EFST_BENEDICTIO		, SCB_DEF_ELE|SCB_WATK|SCB_MATK|SCB_AGI|SCB_VIT|SCB_LUK);
	set_sc( SK_PR_KYRIE		, STATUS_KYRIE		, EFST_KYRIE		, SCB_NONE );
	set_sc( SK_PR_MAGNIFICAT		, STATUS_MAGNIFICAT		, EFST_MAGNIFICAT		, SCB_REGEN );
	add_sc( SK_SA_SILENCE		, STATUS_SILENCE		);
	set_sc( SK_MG_QUAGMIRE		, STATUS_QUAGMIRE		, EFST_QUAGMIRE		, SCB_AGI|SCB_DEX|SCB_ASPD|SCB_SPEED );
	set_sc( SK_BS_WEAPONRYEXPERTISE		, STATUS_WEAPONRYEXPERTISE		, EFST_ADRENALINE		, SCB_ASPD|SCB_HIT );
	set_sc( SK_KN_ADRENALINERUSH		, STATUS_ADRENALINERUSH		, EFST_KNADRENALINE		, SCB_ASPD|SCB_HIT );
		

	set_sc( SK_BS_WEAPONSHARPENING	, STATUS_WEAPONSHARPENING	, EFST_WEAPONPERFECT, SCB_CRI|SCB_WATK );
	set_sc( SK_BS_POWERTHRUST		, STATUS_POWERTHRUST		, EFST_OVERTHRUST		, SCB_NONE );
	set_sc( SK_BS_MAXIMIZEPOWER		, STATUS_MAXIMIZEPOWER	, EFST_MAXIMIZE, SCB_REGEN );
	set_sc( SK_AS_CLOAKING		, STATUS_CLOAKING		, EFST_CLOAKING		, SCB_SPEED );
	set_sc( SK_AS_ENCHANTPOISON	, STATUS_ENCHANTPOISON		, EFST_ENCHANTPOISON, SCB_ATK_ELE );
	set_sc( SK_TF_POISONREACT		, STATUS_POISONREACT	, EFST_POISONREACT	, SCB_NONE );
	set_sc( SK_AS_VENOMSPLASHER		, STATUS_VENOMSPLASHER	, EFST_SPLASHER	, SCB_NONE );
	set_sc( SK_NV_TRICKDEAD		, STATUS_TRICKDEAD		, EFST_TRICKDEAD		, SCB_REGEN );
	set_sc( SK_SM_AUTOBERSERK		, STATUS_ANGER	, EFST_AUTOBERSERK	, SCB_NONE );
	set_sc( SK_SM_SPEARSTANCE			, STATUS_SPEARSTANCE		, EFST_SPEARSTANCE, SCB_STR|SCB_VIT );
	set_sc( SK_AM_HATCHHOMUNCULUS			, STATUS_BIOETHICS		, EFST_BIOETHICS, SCB_NONE );
	set_sc( SK_SA_SUMMONELEMENTAL			, STATUS_ELEMENTALCONTROL		, EFST_ELEMENTALCONTROL, SCB_NONE );
	
	set_sc( SK_MC_LOUD			, STATUS_LOUDEXCLAMATION		, EFST_SHOUT, SCB_STR );
	set_sc( SK_MC_UPROAR			, STATUS_CRAZYUPROAR		, EFST_UPROAR, SCB_WATK );
	set_sc( SK_SM_SPIRITGROWTH			, STATUS_SPIRITGROWTH		, EFST_SPIRITGROWTH, SCB_STR|SCB_INT );
	set_sc( SK_BS_AXEQUICKEN			, STATUS_AXEQUICKEN		, EFST_AXEQUICKEN, SCB_ASPD|SCB_WATK );
	set_sc( SK_RG_DAGGERQUICKEN			, STATUS_DAGGERQUICKEN		, EFST_DAGGERQUICKEN, SCB_ASPD|SCB_WATK );
	set_sc( SK_KN_OHQUICKEN			, STATUS_ONEHANDQUICKEN		, EFST_OHQUICKEN, SCB_ASPD|SCB_WATK );
	set_sc( SK_EX_DANCINGBLADES			, STATUS_DANCINGBLADES		, EFST_DANCINGBLADES, SCB_WATK );
	set_sc( SK_CR_OHQUICKEN			, STATUS_ONEHANDQUICKEN		, EFST_OHQUICKEN, SCB_ASPD|SCB_WATK );
	set_sc( SK_KN_SQUICKEN			, STATUS_SPEARQUICKEN		, EFST_SQUICKEN, SCB_ASPD|SCB_WATK );
	set_sc( SK_KN_THQUICKEN			, STATUS_TWOHANDQUICKEN		, EFST_THQUICKEN, SCB_ASPD|SCB_WATK );
	set_sc( SK_AL_MACEQUICKEN			, STATUS_MACEQUICKEN		, EFST_MACEQUICKEN, SCB_ASPD|SCB_WATK );
	set_sc( SK_MO_KNUCKLEQUICKEN			, STATUS_KNUCKLEQUICKEN		, EFST_KNUCKLEQUICKEN, SCB_ASPD|SCB_WATK );
	set_sc( SK_HT_BOWQUICKEN			, STATUS_BOWQUICKEN		, EFST_BOWQUICKEN, SCB_ASPD|SCB_WATK );
	set_sc( SK_SA_BOOKQUICKEN			, STATUS_BOOKQUICKEN		, EFST_QUICKSTUDY, SCB_ASPD|SCB_WATK );
	set_sc( SK_HT_CYCLONICCHARGE			, STATUS_CYCLONICCHARGE		, EFST_CYCLONICCHARGE, SCB_FLEE|SCB_WATK );
	set_sc( SK_MG_ENERGYCOAT		, STATUS_ENERGYCOAT		, EFST_ENERGYCOAT		, SCB_MATK );
	add_sc( SK_RG_BACKSTAB		, STATUS_STUN		);

	set_sc( SK_ST_STRIPWEAPON		, STATUS_STRIPWEAPON	, EFST_NOEQUIPWEAPON, SCB_WATK );
	set_sc( SK_ST_STRIPSHIELD		, STATUS_STRIPSHIELD	, EFST_NOEQUIPSHIELD, SCB_DEF );
	set_sc( SK_ST_STRIPARMOR		, STATUS_STRIPARMOR		, EFST_NOEQUIPARMOR, SCB_NONE );
	set_sc( SK_ST_STRIPHELM		, STATUS_STRIPHELM		, EFST_NOEQUIPHELM, SCB_NONE );
	set_sc( SK_CR_CP_WEAPON		, STATUS_CP_WEAPON		, EFST_PROTECTWEAPON, SCB_NONE );
	set_sc( SK_CR_CP_SHIELD		, STATUS_CP_SHIELD		, EFST_PROTECTSHIELD, SCB_NONE );
	set_sc( SK_CR_CP_ARMOR		, STATUS_CP_ARMOR		, EFST_PROTECTARMOR, SCB_NONE );
	set_sc( SK_CR_CP_HELM		, STATUS_CP_HELM		, EFST_PROTECTHELM, SCB_NONE );
	set_sc( SK_CR_AUTOGUARD		, STATUS_AUTOGUARD		, EFST_AUTOGUARD		, SCB_NONE );
	add_sc( SK_CR_SMITE		, STATUS_STUN		);
	set_sc( SK_CR_REFLECTSHIELD	, STATUS_REFLECTSHIELD	, EFST_REFLECTSHIELD	, SCB_NONE );
	add_sc( SK_CR_HOLYCROSS		, STATUS_BLIND		);
	set_sc( SK_CR_SWORNPROTECTOR		, STATUS_SWORNPROTECTOR	, EFST_DEVOTION	, SCB_NONE);
	set_sc( SK_PA_DEFENDINGAURA		, STATUS_DEFENDINGAURA		, EFST_DEFENDER		, SCB_SPEED|SCB_ASPD );
	set_sc( SK_SH_MENTALSTRENGTH		, STATUS_MENTALSTRENGTH		, EFST_STEELBODY		, SCB_DEF|SCB_MDEF|SCB_ASPD|SCB_SPEED );
	add_sc( SK_SH_GRAPPLE		, STATUS_GRAPPLE_WAIT	);
	set_sc( SK_MO_TRIPLEARMCANNON		, STATUS_COMBO1	, EFST_COMBO1	, SCB_NONE );
	set_sc( SK_MO_TRIPLEARMCANNON		, STATUS_COMBO2	, EFST_COMBO2	, SCB_NONE );
	set_sc( SK_MO_TRIPLEARMCANNON		, STATUS_COMBO3	, EFST_COMBO3	, SCB_NONE );
	set_sc( SK_MO_TRIPLEARMCANNON		, STATUS_COMBO4	, EFST_COMBO4	, SCB_NONE );
	set_sc( SK_MO_TRIPLEARMCANNON		, STATUS_COMBO5	, EFST_COMBO5	, SCB_NONE );
	set_sc( SK_MO_TRIPLEARMCANNON		, STATUS_COMBO6	, EFST_COMBO6	, SCB_NONE );
	set_sc( SK_MO_TRIPLEARMCANNON		, STATUS_COMBO7	, EFST_COMBO7	, SCB_NONE );
	set_sc( SK_MO_TRIPLEARMCANNON		, STATUS_COMBO8	, EFST_COMBO8	, SCB_NONE );
	set_sc( SK_MO_TRIPLEARMCANNON		, STATUS_COMBO9	, EFST_COMBO9	, SCB_NONE );
	set_sc( SK_MO_TRIPLEARMCANNON		, STATUS_COMBO10	, EFST_COMBO10	, SCB_NONE );
	
	set_sc( SK_SH_ULTRAINSTINCT	, STATUS_ULTRAINSTINCT	, EFST_EXPLOSIONSPIRITS	, SCB_STR|SCB_AGI|SCB_VIT|SCB_WATK );
	set_sc( SK_KN_FURY	, STATUS_FURY	, EFST_FURY	, SCB_CRI|SCB_WATK );
	set_sc( SK_BI_LAUDATEDOMINIUM	, STATUS_LAUDATEDOMINIUM	, EFST_SK_BI_LAUDATEDOMINIUM	,  SCB_MATK );
	set_sc( SK_SA_AUTOSPELL		, STATUS_AUTOSPELL		, EFST_AUTOSPELL		, SCB_NONE );
	
	set_sc( SK_BA_ACOUSTICRYTHM		, STATUS_ACCOUSTICRYTHM		, EFST_RICHMANKIM	, SCB_STR|SCB_VIT|SCB_MAXHP|SCB_MAXSP );
	set_sc( SK_CL_BATTLEDRUMS	, STATUS_BATTLEDRUMS		, EFST_DRUMBATTLEFIELD	, SCB_WATK|SCB_DEF|SCB_MATK|SCB_MDEF );

	set_sc( SK_BA_MOONLIGHTSONATA		, STATUS_MOONLIGHTSONATA	, EFST_INTOABYSS	, SCB_NONE );
	set_sc( SK_CL_PAGANPARTY		, STATUS_PAGANPARTY		, EFST_SIEGFRIED	, SCB_NONE );
	set_sc( SK_BA_IMPRESSIVERIFF	, STATUS_IMPRESSIVERIFF		, EFST_ASSASSINCROSS		, SCB_AGI|SCB_LUK|SCB_ASPD|SCB_CRI );
	set_sc( SK_BA_MAGICSTRINGS		, STATUS_MAGICSTRINGS	, EFST_POEMBRAGI	, SCB_INT|SCB_DEX	);
	set_sc( SK_CM_AURABLADE		, STATUS_AURABLADE		, EFST_AURABLADE		, SCB_NONE );
	set_sc( SK_CM_PARRY		, STATUS_PARRY		, EFST_PARRYING		, SCB_NONE );
	set_sc( SK_CM_SPEAR_DYNAMO	, STATUS_SPEARDYNAMO	, EFST_LKCONCENTRATION	, SCB_HIT );
	set_sc( SK_PA_FORTIFY	, STATUS_FORTIFY	, EFST_FORTIFY	, SCB_DEF|SCB_MDEF );
	set_sc( SK_CM_FRENZY		, STATUS_FRENZY		, EFST_BERSERK		, SCB_SPEED|SCB_ASPD );
	set_sc( SK_BI_ASSUMPTIO		, STATUS_ASSUMPTIO		,EFST_ASSUMPTIO		, SCB_NONE );

	set_sc( SK_SO_MYSTICALAMPLIFICATION		, STATUS_MYSTICALAMPLIFICATION		, EFST_MAGICPOWER		, SCB_MATK );
	set_sc( SK_KN_RECKONING		, STATUS_RECKONING		, EFST_SACRIFICE, SCB_NONE		);
	set_sc( SK_HT_LIVINGTORNADO		, STATUS_LIVINGTORNADO		, EFST_HURRICANEFURY, SCB_NONE		);
	set_sc( SK_RA_ZEPHYRSNIPING		, STATUS_ZEPHYRSNIPING		, EFST_ZEPHYR_SNIPING, SCB_NONE		);
	set_sc( SK_PA_GOSPEL		, STATUS_GOSPEL		, EFST_GOSPEL		, SCB_SPEED|SCB_ASPD );
	//add_sc( SK_PA_GOSPEL		, SC_SCRESIST		);
	set_sc( SK_EX_ENCHANTDEADLYPOISON			, STATUS_ENCHANTDEADLYPOISON		, EFST_EDP		, SCB_NONE );
	//set_sc( SK_RA_FALCONEYES		, SC_TRUESIGHT		, EFST_TRUESIGHT		, SCB_STR|SCB_AGI|SCB_VIT|SCB_INT|SCB_DEX|SCB_LUK|SCB_CRI|SCB_HIT );
	set_sc( SK_HT_WINDRACER		, STATUS_WINDRACER		, EFST_WINDWALK		, SCB_AGI|SCB_SPEED );
	set_sc( SK_MS_MELTDOWN		, STATUS_MELTDOWN		, EFST_MELTDOWN		, SCB_WATK );
	set_sc( SK_BS_CARTBOOST		, STATUS_CARTBOOST		, EFST_CARTBOOST		, SCB_SPEED|SCB_WATK );
	set_sc( SK_AS_STEALTH		, STATUS_CHASEWALK		, EFSK_AS_STEALTH		, SCB_SPEED );
	set_sc( SK_ST_KILLERINSTIINCT		, STATUS_KILLERINSTINCT	, EFST_SWORDREJECT, SCB_WATK|SCB_CRI );
	set_sc( SK_CL_MARIONETTECONTROL		, STATUS_MARIONETTE		, EFST_MARIONETTE_MASTER, SCB_STR|SCB_AGI|SCB_VIT|SCB_INT|SCB_DEX|SCB_LUK );
	set_sc( SK_CL_MARIONETTECONTROL		, STATUS_MARIONETTE2	, EFST_MARIONETTE, SCB_STR|SCB_AGI|SCB_VIT|SCB_INT|SCB_DEX|SCB_LUK );
	add_sc( SK_SM_TRAUMATIC_STRIKE		, STATUS_BLEEDING		);
	set_sc( SK_SA_MINDGAMES		, STATUS_MINDGAMES	, EFST_MINDBREAKER	, SCB_MATK|SCB_MDEF2 );
	set_sc( SK_WZ_FORESIGHT		, STATUS_FORESIGHT	, EFST_MEMORIZE	, SCB_NONE );
	//set_sc( SK_WZ_ILLUSIONARYBLADES			, SC_SMA		, EFST_SMA_READY, SCB_NONE );
	set_sc( SK_PF_MAGICSQUARED	, STATUS_MAGICSQUARED		, EFST_DOUBLECASTING, SCB_NONE );

	set_sc( SK_MS_MAXPOWERTHRUST	, STATUS_MAXPOWERTHRUST	, EFST_OVERTHRUSTMAX, SCB_NONE );


	set_sc( SK_RG_CLOSECONFINE		, STATUS_CLOSECONFINE2	, EFST_RG_CCONFINE_S, SCB_NONE );
	set_sc( SK_RG_CLOSECONFINE		, STATUS_CLOSECONFINE	, EFST_RG_CCONFINE_M, SCB_FLEE );
	add_sc( SK_MO_FALINGFIST		, STATUS_STUN		);


	
	/* Rune Knight */
	set_sc( SK_KN_ENCHANTBLADE		, STATUS_ENCHANTBLADE	, EFST_ENCHANTBLADE		, SCB_NONE );
	set_sc( SK_KN_STONESKIN	, STATUS_STONESKIN	, EFST_STONEHARDSKIN		, SCB_DEF|SCB_MDEF );
	set_sc( SK_MO_BODYRELOCATION	, STATUS_DEFENSIVESTANCE	, EFST_DEFENSIVESTANCE		, SCB_DEF|SCB_MDEF );
	set_sc( SK_KN_VIGOR	, STATUS_VIGOR	, EFST_VITALITYACTIVATION		, SCB_VIT );
	
	/* GC Guillotine Cross */
	set_sc_with_vfx( SK_AS_VENOMIMPRESS, STATUS_VENOMIMPRESS	, EFST_VENOMIMPRESS	, SCB_NONE );
	set_sc( SK_EX_HALLUCINATIONWALK	, STATUS_HALLUCINATIONWALK	, EFST_HALLUCINATIONWALK	, SCB_FLEE );
	set_sc( SK_AS_ROLLINGCUTTER	, STATUS_ROLLINGCUTTER	, EFST_ROLLINGCUTTER	, SCB_NONE );
	set_sc_with_vfx( SK_EX_DARKCLAW	, STATUS_DARKCLAW		, EFST_DARKCROW		, SCB_NONE );

	/* Arch Bishop */
	//set_sc( SK_BI_EPICLESIS		, SC_EPICLESIS		, EFST_EPICLESIS, SCB_NONE);
	set_sc( SK_AL_RENEW		, STATUS_RENEW		, EFST_RENOVATIO		, SCB_REGEN );
	set_sc( SK_BI_EXPIATIO		, STATUS_EXPIATIO		, EFST_EXPIATIO		, SCB_NONE );
	set_sc( SK_BI_INLUCEMBATALIA		, STATUS_INLUCEMBATALIA		, EFST_ULTOR		, SCB_WATK|SCB_MATK|SCB_ASPD );
	set_sc( SK_PR_DUPLELUX		, STATUS_DUPLELUX		, EFST_DUPLELIGHT		, SCB_NONE );
	set_sc( SK_BI_OFFERTORIUM		, STATUS_OFFERTORIUM	, EFST_OFFERTORIUM	, SCB_NONE );
	add_sc( SK_PR_LEXAETERNA		, STATUS_LEXAETERNA );

	/* Warlock */
	add_sc( SK_SA_WHITEIMPRISON	, STATUS_MAGICCELL	);
	set_sc( SK_SO_HINDSIGHT	, STATUS_HINDSIGHT	, EFST_RECOGNIZEDSPELL	, SCB_MATK);
	set_sc( SK_SO_VOIDMAGEPRODIGY	, STATUS_VOIDMAGEPRODIGY, EFST_TELEKINESIS_INTENSE, SCB_MATK );
	set_sc( SK_CR_GEOGRAFIELD_ATK	, STATUS_GEOGRAFIELD, EFST_GEOGRAFIELD, SCB_NONE );

	/* Ranger */
	set_sc( SK_RA_TYHPOONFLOW		, STATUS_TYPHOONFLOW		, EFST_FEARBREEZE		, SCB_NONE );
	set_sc( SK_HT_CAMOUFLAGE		, STATUS_CAMOUFLAGE		, EFST_CAMOUFLAGE		, SCB_SPEED );
	set_sc( SK_ST_UNLIMIT		, STATUS_UNLIMIT		, EFST_UNLIMIT		, SCB_NONE);

	/* Mechanic */
	set_sc( SK_MS_NEUTRALBARRIER	, STATUS_NEUTRALBARRIER	, EFST_NEUTRALBARRIER	, SCB_DEF|SCB_MDEF );

	/* Royal Guard */
	set_sc( SK_CR_VANGUARDFORCE		, STATUS_VANGUARDFORCE		, EFST_PRESTIGE		, SCB_DEF );
	set_sc( SK_CM_MILLENIUMSHIELDS		, STATUS_MILLENIUMSHIELDS	, EFST_KINGS_GRACE	, SCB_NONE );

	/* Shadow Chaser */
	set_sc( SK_ST_REPRODUCE		, STATUS_REPRODUCE		, EFST_REPRODUCE		, SCB_NONE );
	set_sc( SK_RG_PLAGIARISM		, STATUS_PLAGIARISM		, EFST_PLAGIARISM		, SCB_NONE );
	set_sc( SK_ST_AUTOSHADOWSPELL	, STATUS_AUTOSHADOWSPELL	, EFST_AUTOSHADOWSPELL	, SCB_NONE );
	set_sc( SK_ST_STALK		, STATUS_STALK	, EFST_SHADOWFORM		, SCB_NONE );
	set_sc( SK_EX_INVISIBILITY		, STATUS_INVISIBILITY	, EFST_INVISIBILITY	, SCB_NONE );
	set_sc( SK_RG_STRIPACCESSORY	, STATUS_STRIPACCESSORY	, EFST_STRIPACCESSARY	, SCB_DEX|SCB_INT|SCB_LUK|SCB_STR|SCB_AGI|SCB_VIT );

	/* Sura */

	/* Wanderer / Minstrel */
	set_sc( SK_BA_WINDMILLPOEM		, STATUS_WINDMILLPOEM		, EFST_RUSH_WINDMILL, SCB_WATK|SCB_DEF );
	set_sc( SK_BA_ECHONOCTURNAE			, STATUS_ECHONOCTURNAE			, EFST_ECHOSONG			, SCB_MATK|SCB_MDEF  );
	set_sc_with_vfx( SK_CL_DEEPSLEEPLULLABY	, STATUS_SLEEP			, EFST_SLEEP, SCB_NONE );
	set_sc( SK_PF_STASIS	, STATUS_STASIS			, EFST_STASIS, SCB_NONE );

	/* Sorcerer */
	set_sc( SK_PF_SPELLFIST		, STATUS_SPELLFIST		, EFST_SPELLFIST		, SCB_NONE );
	set_sc( SK_AS_POISONOUSCLOUD   , STATUS_POISON         , EFST_CLOUD_KILL, SCB_NONE );

	/* Genetic */
	set_sc_with_vfx( SK_RA_ULLREAGLETOTEM		, STATUS_ULLREAGLETOTEM	, EFST_ULLR	, SCB_NONE );
	

	// New Mounts
	set_sc_with_vfx_noskill( STATUS_ALL_RIDING	, EFST_ALL_RIDING	, SCB_SPEED );


	/* Status that don't have a skill associated */
	StatusIconChangeTable[STATUS_WEIGHT50] = EFST_WEIGHTOVER50;
	StatusIconChangeTable[STATUS_WEIGHT90] = EFST_WEIGHTOVER90;
	StatusIconChangeTable[STATUS_ASPDPOTION0] = EFST_ATTHASTE_POTION1;
	StatusIconChangeTable[STATUS_ASPDPOTION1] = EFST_ATTHASTE_POTION2;
	StatusIconChangeTable[STATUS_ASPDPOTION2] = EFST_ATTHASTE_POTION3;
	StatusIconChangeTable[STATUS_ASPDPOTION3] = EFST_ATTHASTE_INFINITY;
	StatusIconChangeTable[STATUS_CHASEWALK2] = EFSK_AS_STEALTH2;
	StatusIconChangeTable[STATUS_STRFOOD] = EFST_FOOD_STR;
	StatusIconChangeTable[STATUS_AGIFOOD] = EFST_FOOD_AGI;
	StatusIconChangeTable[STATUS_VITFOOD] = EFST_FOOD_VIT;
	StatusIconChangeTable[STATUS_INTFOOD] = EFST_FOOD_INT;
	StatusIconChangeTable[STATUS_DEXFOOD] = EFST_FOOD_DEX;
	StatusIconChangeTable[STATUS_LUKFOOD] = EFST_FOOD_LUK;
	


	/* Warlock Spheres */
	StatusIconChangeTable[STATUS_SPHERE_1] = EFST_SUMMON1;
	StatusIconChangeTable[STATUS_SPHERE_2] = EFST_SUMMON2;
	StatusIconChangeTable[STATUS_SPHERE_3] = EFST_SUMMON3;
	StatusIconChangeTable[STATUS_SPHERE_4] = EFST_SUMMON4;
	StatusIconChangeTable[STATUS_SPHERE_5] = EFST_SUMMON5;

	/* Warlock Preserved spells */
	StatusIconChangeTable[STATUS_SPELLBOOK1] = EFST_SPELLBOOK1;
	StatusIconChangeTable[STATUS_SPELLBOOK2] = EFST_SPELLBOOK2;
	StatusIconChangeTable[STATUS_SPELLBOOK3] = EFST_SPELLBOOK3;
	StatusIconChangeTable[STATUS_SPELLBOOK4] = EFST_SPELLBOOK4;
	StatusIconChangeTable[STATUS_SPELLBOOK5] = EFST_SPELLBOOK5;
	StatusIconChangeTable[STATUS_SPELLBOOK6] = EFST_SPELLBOOK6;
	StatusIconChangeTable[STATUS_MAXSPELLBOOK] = EFST_SPELLBOOK7;
	StatusIconChangeTable[STATUS_STORE_SPELLBOOK] = EFST_STORE_SPELLBOOK;
	//StatusIconChangeTable[STATUS_NEUTRALBARRIER_MASTER] = EFST_NEUTRALBARRIER_MASTER;
	
	StatusIconChangeTable[STATUS_CONFUSION] = EFST_CONFUSION;
	StatusIconChangeTable[STATUS_PARALYSIS] = EFST_PARALYSE;
	StatusIconChangeTable[STATUS_VENOMIMPRESS] = EFST_VENOMIMPRESS;
	

	StatusIconChangeTable[STATUS_ALL_RIDING] = EFST_ALL_RIDING;
	StatusIconChangeTable[STATUS_PUSHCART] = EFST_ON_PUSH_CART;
	


	// Clan System
	StatusIconChangeTable[STATUS_CLAN_INFO] = EFST_CLAN_INFO;
	StatusIconChangeTable[STATUS_SWORDCLAN] = EFST_SWORDCLAN;
	StatusIconChangeTable[STATUS_ARCWANDCLAN] = EFST_ARCWANDCLAN;
	StatusIconChangeTable[STATUS_GOLDENMACECLAN] = EFST_GOLDENMACECLAN;
	StatusIconChangeTable[STATUS_CROSSBOWCLAN] = EFST_CROSSBOWCLAN;
	StatusIconChangeTable[STATUS_JUMPINGCLAN] = EFST_JUMPINGCLAN;


	/* Other SC which are not necessarily associated to skills */
	StatusChangeFlagTable[STATUS_ASPDPOTION0] |= SCB_ASPD;
	StatusChangeFlagTable[STATUS_ASPDPOTION1] |= SCB_ASPD;
	StatusChangeFlagTable[STATUS_ASPDPOTION2] |= SCB_ASPD;
	StatusChangeFlagTable[STATUS_ASPDPOTION3] |= SCB_ASPD;
	

	
	StatusChangeFlagTable[STATUS_STRFOOD] |= SCB_STR;
	StatusChangeFlagTable[STATUS_AGIFOOD] |= SCB_AGI;
	StatusChangeFlagTable[STATUS_VITFOOD] |= SCB_VIT;
	StatusChangeFlagTable[STATUS_INTFOOD] |= SCB_INT;
	StatusChangeFlagTable[STATUS_DEXFOOD] |= SCB_DEX;
	StatusChangeFlagTable[STATUS_LUKFOOD] |= SCB_LUK;
	
	StatusChangeFlagTable[STATUS_CHASEWALK2] |= SCB_STR|SCB_DEX;
	

	StatusChangeFlagTable[STATUS_ALL_RIDING] |= SCB_SPEED;
	StatusChangeFlagTable[STATUS_PUSHCART] |= SCB_SPEED;
	

	

	// Clan System
	StatusChangeFlagTable[STATUS_CLAN_INFO] |= SCB_NONE;
	StatusChangeFlagTable[STATUS_SWORDCLAN] |= SCB_STR|SCB_VIT|SCB_MAXHP|SCB_MAXSP;
	StatusChangeFlagTable[STATUS_ARCWANDCLAN] |= SCB_INT|SCB_DEX|SCB_MAXHP|SCB_MAXSP;
	StatusChangeFlagTable[STATUS_GOLDENMACECLAN] |= SCB_LUK|SCB_INT|SCB_MAXHP|SCB_MAXSP;
	StatusChangeFlagTable[STATUS_CROSSBOWCLAN] |= SCB_DEX|SCB_AGI|SCB_MAXHP|SCB_MAXSP;
	StatusChangeFlagTable[STATUS_JUMPINGCLAN] |= SCB_STR|SCB_AGI|SCB_VIT|SCB_INT|SCB_DEX|SCB_LUK;

	
	// renewal EDP increases your weapon atk
	StatusChangeFlagTable[STATUS_ENCHANTDEADLYPOISON] |= SCB_WATK;



	/* StatusDisplayType Table [Ind] */
	StatusDisplayType[STATUS_ALL_RIDING]	  = BL_PC;
	StatusDisplayType[STATUS_PUSHCART]		  = BL_PC;
	StatusDisplayType[STATUS_SPHERE_1]		  = BL_PC;
	StatusDisplayType[STATUS_SPHERE_2]		  = BL_PC;
	StatusDisplayType[STATUS_SPHERE_3]		  = BL_PC;
	StatusDisplayType[STATUS_SPHERE_4]		  = BL_PC;
	StatusDisplayType[STATUS_SPHERE_5]		  = BL_PC;
	StatusDisplayType[STATUS_CAMOUFLAGE]	  = BL_PC;
	StatusDisplayType[STATUS_DUPLELUX]	  = BL_PC;
	StatusDisplayType[STATUS_VENOMIMPRESS]	  = BL_PC;
	StatusDisplayType[STATUS_HALLUCINATIONWALK]	  = BL_PC;
	StatusDisplayType[STATUS_ROLLINGCUTTER]	  = BL_PC;
	StatusDisplayType[STATUS_STALK]	  = BL_PC;
	StatusDisplayType[STATUS_DARKCLAW]		  = BL_PC;
	StatusDisplayType[STATUS_OFFERTORIUM]	  = BL_PC;
	StatusDisplayType[STATUS_VOIDMAGEPRODIGY] = BL_PC;
	StatusDisplayType[STATUS_GEOGRAFIELD] = BL_PC;
	StatusDisplayType[STATUS_UNLIMIT]		  = BL_PC;
	StatusDisplayType[STATUS_ULLREAGLETOTEM]     = BL_PC;
	


	// Clans
	StatusDisplayType[STATUS_CLAN_INFO] = BL_PC|BL_NPC;


	/* StatusChangeState (SCS_) NOMOVE */
	StatusChangeStateTable[STATUS_AUTOCOUNTER]			|= SCS_NOMOVE;
	StatusChangeStateTable[STATUS_TRICKDEAD]			|= SCS_NOMOVE;
	StatusChangeStateTable[STATUS_GRAPPLE]			|= SCS_NOMOVE;
	StatusChangeStateTable[STATUS_GRAPPLE_WAIT]		|= SCS_NOMOVE;
	StatusChangeStateTable[STATUS_GOSPEL]				|= SCS_NOMOVE|SCS_NOMOVECOND;
	StatusChangeStateTable[STATUS_CLOSECONFINE]			|= SCS_NOMOVE;
	StatusChangeStateTable[STATUS_CLOSECONFINE2]		|= SCS_NOMOVE;
	StatusChangeStateTable[STATUS_MAGICCELL]		|= SCS_NOMOVE;
	StatusChangeStateTable[STATUS_STASIS]			    |= SCS_NOMOVE;
	StatusChangeStateTable[STATUS_CAMOUFLAGE]			|= SCS_NOMOVE|SCS_NOMOVECOND;
	StatusChangeStateTable[STATUS_PARALYSIS]			|= SCS_NOMOVE;
	StatusChangeStateTable[STATUS_MILLENIUMSHIELDS]			|= SCS_NOMOVE;
	

	/* StatusChangeState (SCS_) NOPICKUPITEMS */
	// StatusChangeStateTable[STATUS_HIDING]				|= SCS_NOPICKITEM;
	// StatusChangeStateTable[STATUS_CLOAKING]				|= SCS_NOPICKITEM;
	StatusChangeStateTable[STATUS_TRICKDEAD]			|= SCS_NOPICKITEM;
	StatusChangeStateTable[STATUS_GRAPPLE]			|= SCS_NOPICKITEM;
	StatusChangeStateTable[STATUS_NOCHAT]				|= SCS_NOPICKITEM|SCS_NOPICKITEMCOND;

	/* StatusChangeState (SCS_) NODROPITEMS */
	StatusChangeStateTable[STATUS_AUTOCOUNTER]			|= SCS_NODROPITEM;
	StatusChangeStateTable[STATUS_GRAPPLE]			|= SCS_NODROPITEM;
	StatusChangeStateTable[STATUS_NOCHAT]				|= SCS_NODROPITEM|SCS_NODROPITEMCOND;

	/* StatusChangeState (SCS_) NOCAST (skills) */
	StatusChangeStateTable[STATUS_SILENCE]				|= SCS_NOCAST;
	StatusChangeStateTable[STATUS_FRENZY]				|= SCS_NOCAST;
	StatusChangeStateTable[STATUS_MAGICCELL]		|= SCS_NOCAST;
	StatusChangeStateTable[STATUS_STASIS]    			|= SCS_NOCAST;
	StatusChangeStateTable[STATUS_MILLENIUMSHIELDS]			|= SCS_NOCAST;

	/* StatusChangeState (SCS_) NOCHAT (skills) */
	StatusChangeStateTable[STATUS_FRENZY]				|= SCS_NOCHAT;
	StatusChangeStateTable[STATUS_NOCHAT]				|= SCS_NOCHAT|SCS_NOCHATCOND;
}

static void initDummyData(void)
{
	memset(&dummy_status, 0, sizeof(dummy_status));
	dummy_status.hp =
	dummy_status.max_hp =
	dummy_status.max_sp =
	dummy_status.str =
	dummy_status.agi =
	dummy_status.vit =
	dummy_status.int_ =
	dummy_status.dex =
	dummy_status.luk =
	dummy_status.hit = 1;
	dummy_status.speed = 2000;
	dummy_status.adelay = 4000;
	dummy_status.amotion = 2000;
	dummy_status.dmotion = 2000;
	dummy_status.ele_lv = 1; // Min elemental level.
	dummy_status.mode = MD_CANMOVE;
}

/**
 * For copying a status_data structure from b to a, without overwriting current Hp and Sp
 * @param a: Status data structure to copy from
 * @param b: Status data structure to copy to
 */
static inline void status_cpy(struct status_data* a, const struct status_data* b)
{
	memcpy((void*)&a->max_hp, (const void*)&b->max_hp, sizeof(struct status_data)-(sizeof(a->hp)+sizeof(a->sp)));
}

/**
 * Sets HP to a given value
 * Will always succeed (overrides heal impediment statuses) but can't kill an object
 * @param bl: Object whose HP will be set [PC|MOB|HOM|MER|ELEM|NPC]
 * @param hp: What the HP is to be set as
 * @param flag: Used in case final value is higher than current
 *		Use 2 to display healing effect
 * @return heal or zapped HP if valid
 */
int status_set_hp(struct block_list *bl, unsigned int hp, int flag)
{
	struct status_data *status;
	if (hp < 1)
		return 0;
	status = status_get_status_data(bl);
	if (status == &dummy_status)
		return 0;

	if (hp > status->max_hp)
		hp = status->max_hp;
	if (hp == status->hp)
		return 0;
	if (hp > status->hp)
		return status_heal(bl, hp - status->hp, 0, 1|flag);
	return status_zap(bl, status->hp - hp, 0);
}

/**
 * Sets Max HP to a given value
 * @param bl: Object whose Max HP will be set [PC|MOB|HOM|MER|ELEM|NPC]
 * @param maxhp: What the Max HP is to be set as
 * @param flag: Used in case final value is higher than current
 *		Use 2 to display healing effect
 * @return heal or zapped HP if valid
 */
int status_set_maxhp(struct block_list *bl, unsigned int maxhp, int flag)
{
	struct status_data *status;
	int64 heal;

	if (maxhp < 1)
		return 0;
	status = status_get_status_data(bl);
	if (status == &dummy_status)
		return 0;

	if (maxhp == status->max_hp)
		return 0;

	heal = maxhp - status->max_hp;
	status->max_hp = maxhp;

	if (heal > 0)
		status_heal(bl, heal, 0, 1|flag);
	else
		status_zap(bl, -heal, 0);

	return maxhp;
}

/**
 * Sets SP to a given value
 * @param bl: Object whose SP will be set [PC|HOM|MER|ELEM]
 * @param sp: What the SP is to be set as
 * @param flag: Used in case final value is higher than current
 *		Use 2 to display healing effect		
 * @return heal or zapped SP if valid
 */
int status_set_sp(struct block_list *bl, unsigned int sp, int flag)
{
	struct status_data *status;

	status = status_get_status_data(bl);
	if (status == &dummy_status)
		return 0;

	if (sp > status->max_sp)
		sp = status->max_sp;
	if (sp == status->sp)
		return 0;
	if (sp > status->sp)
		return status_heal(bl, 0, sp - status->sp, 1|flag);
	return status_zap(bl, 0, status->sp - sp);
}

/**
 * Sets Max SP to a given value
 * @param bl: Object whose Max SP will be set [PC|HOM|MER|ELEM]
 * @param maxsp: What the Max SP is to be set as
 * @param flag: Used in case final value is higher than current
 *		Use 2 to display healing effect
 * @return heal or zapped HP if valid
 */
int status_set_maxsp(struct block_list *bl, unsigned int maxsp, int flag)
{
	struct status_data *status;
	if (maxsp < 1)
		return 0;
	status = status_get_status_data(bl);
	if (status == &dummy_status)
		return 0;

	if (maxsp == status->max_sp)
		return 0;
	if (maxsp > status->max_sp)
		status_heal(bl, maxsp - status->max_sp, 0, 1|flag);
	else
		status_zap(bl, status->max_sp - maxsp, 0);

	status->max_sp = maxsp;
	return maxsp;
}

/**
 * Takes HP/SP from an Object
 * @param bl: Object who will have HP/SP taken [PC|MOB|HOM|MER|ELEM]
 * @param hp: How much HP to charge
 * @param sp: How much SP to charge	
 * @return hp+sp through status_damage()
 * Note: HP/SP are integer values, not percentages. Values should be
 *	 calculated either within function call or before
 */
int64 status_charge(struct block_list* bl, int64 hp, int64 sp)
{
	if(!(bl->type&BL_CONSUME))
		return (int)hp+sp; // Assume all was charged so there are no 'not enough' fails.
	return status_damage(NULL, bl, hp, sp, 0, 3, 0);
}

/**
 * Inflicts damage on the target with the according walkdelay.
 * @param src: Source object giving damage [PC|MOB|PET|HOM|MER|ELEM]
 * @param target: Target of the damage
 * @param dhp: How much damage to HP
 * @param dsp: How much damage to SP
 * @param walkdelay: Amount of time before object can walk again
 * @param flag: Damage flag decides various options
 *		flag&1: Passive damage - Does not trigger cancelling status changes
 *		flag&2: Fail if there is not enough to subtract
 *		flag&4: Mob does not give EXP/Loot if killed
 *		flag&8: Used to damage SP of a dead character
 * @return hp+sp
 * Note: HP/SP are integer values, not percentages. Values should be
 *	 calculated either within function call or before
 */
int status_damage(struct block_list *src,struct block_list *target,int64 dhp, int64 dsp, t_tick walkdelay, int flag, uint16 skill_id)
{
	struct status_data *status;
	struct status_change *sc;
	int hp = (int)cap_value(dhp,INT_MIN,INT_MAX);
	int sp = (int)cap_value(dsp,INT_MIN,INT_MAX);

	nullpo_ret(target);

	if(sp && !(target->type&BL_CONSUME))
		sp = 0; // Not a valid SP target.

	if (hp < 0) { // Assume absorbed damage.
		status_heal(target, -hp, 0, 1);
		hp = 0;
	}

	if (sp < 0) {
		status_heal(target, 0, -sp, 1);
		sp = 0;
	}

	if (target->type == BL_SKILL) {
		if (!src || src->type&battle_config.can_damage_skill)
			return (int)skill_unit_ondamaged((struct skill_unit *)target, hp);
		return 0;
	}

	status = status_get_status_data(target);
	if(!status || status == &dummy_status )
		return 0;

	if ((unsigned int)hp >= status->hp) {
		if (flag&2) return 0;
		hp = status->hp;
	}

	if ((unsigned int)sp > status->sp) {
		if (flag&2) return 0;
		sp = status->sp;
	}

	if (!hp && !sp)
		return 0;

	if( !status->hp )
		flag |= 8;

	sc = status_get_sc(target);

	if( hp && !(flag&1) ) {
		if( sc ) {
			struct status_change_entry *sce;
			if (sc->data[STATUS_STONECURSE] && sc->opt1 == OPT1_STONE)
				status_change_end(target, STATUS_STONECURSE, INVALID_TIMER);
			status_change_end(target, STATUS_FREEZE, INVALID_TIMER);
			status_change_end(target, STATUS_SLEEP, INVALID_TIMER);
			status_change_end(target, STATUS_TRICKDEAD, INVALID_TIMER);
			status_change_end(target, STATUS_HIDING, INVALID_TIMER);
			status_change_end(target, STATUS_CLOAKING, INVALID_TIMER);
			status_change_end(target, STATUS_INVISIBILITY, INVALID_TIMER);
			status_change_end(target, STATUS_CHASEWALK, INVALID_TIMER);
			status_change_end(target, STATUS_CAMOUFLAGE, INVALID_TIMER);
			if ((sce=sc->data[STATUS_ENDURE]) && !sce->val4) {
				/** [Skotlex]
				* Endure count is only reduced by non-players on non-gvg maps.
				* val4 signals infinite endure.
				**/
				if (src && src->type != BL_PC && !map_flag_gvg2(target->m) && !map_getmapflag(target->m, MF_BATTLEGROUND) && --(sce->val2) <= 0)
					status_change_end(target, STATUS_ENDURE, INVALID_TIMER);
			}

			
		}

		if (target->type == BL_PC)
			pc_bonus_script_clear(BL_CAST(BL_PC,target),BSF_REM_ON_DAMAGED);
		unit_skillcastcancel(target, 2);
	}

	status->hp-= hp;
	status->sp-= sp;

	if (sc && hp && status->hp) {
		if (sc->data[STATUS_ANGER] &&
			(!sc->data[STATUS_PROVOKE] || !sc->data[STATUS_PROVOKE]->val2) &&
			status->hp < status->max_hp/3)
			sc_start4(src,target,STATUS_PROVOKE,100,20,1,0,0,0);
	}

	switch (target->type) {
		case BL_PC:  pc_damage((TBL_PC*)target,src,hp,sp); break;
		case BL_MOB: mob_damage((TBL_MOB*)target, src, hp); break;
		case BL_ELEM: elementSK_AL_HEAL((TBL_ELEM*)target,hp,sp); break;
	}

	if( src && target->type == BL_PC && ((TBL_PC*)target)->disguise ) { // Stop walking when attacked in disguise to prevent walk-delay bug
		unit_stop_walking( target, 1 );
	}

	if( status->hp || (flag&8) ) { // Still lives or has been dead before this damage.
		if (walkdelay)
			unit_set_walkdelay(target, gettick(), walkdelay, 0);
		return (int)(hp+sp);
	}

	status->hp = 0;
	/** [Skotlex]
	* NOTE: These dead functions should return:
	* 0: Death cancelled, auto-revived.
	* Non-zero: Standard death. Clear status, cancel move/attack, etc
	* &2: Remove object from map.
	* &4: Delete object from memory. (One time spawn mobs)
	**/
	switch (target->type) {
		case BL_PC:  flag = pc_dead((TBL_PC*)target,src); break;
		case BL_MOB: flag = mob_dead((TBL_MOB*)target, src, flag&4?3:0); break;
		case BL_ELEM: flag = elemental_dead((TBL_ELEM*)target); break;
		default:	// Unhandled case, do nothing to object.
			flag = 0;
			break;
	}

	if(!flag) // Death cancelled.
		return (int)(hp+sp);

	// Normal death
	if (battle_config.clear_unit_ondeath &&
		battle_config.clear_unit_ondeath&target->type)
		skill_clear_unitgroup(target);

	if(target->type&BL_REGEN) { // Reset regen ticks.
		struct regen_data *regen = status_get_regen_data(target);
		if (regen) {
			memset(&regen->tick, 0, sizeof(regen->tick));
			if (regen->sregen)
				memset(&regen->sregen->tick, 0, sizeof(regen->sregen->tick));
			if (regen->ssregen)
				memset(&regen->ssregen->tick, 0, sizeof(regen->ssregen->tick));
		}
	}

	

	status_change_clear(target,0);

	if(flag&4) // Delete from memory. (also invokes map removal code)
		unit_free(target,CLR_DEAD);
	else if(flag&2) // remove from map
		unit_remove_map(target,CLR_DEAD);
	else { // Some death states that would normally be handled by unit_remove_map
		unit_stop_attack(target);
		unit_stop_walking(target,1);
		unit_skillcastcancel(target,0);
		clif_clearunit_area(target,CLR_DEAD);
		skill_unit_move(target,gettick(),4);
		skill_cleartimerskill(target);
	}

	// Always run NPC scripts for players last
	//FIXME those ain't always run if a player die if he was resurrect meanwhile
	//cf SC_REBIRTH, SC_KAIZEL, pc_dead...
	if(target->type == BL_PC) {
		TBL_PC *sd = BL_CAST(BL_PC,target);
		

		npc_script_event(sd,NPCE_DIE);
	}

	return (int)(hp+sp);
}

/**
 * Heals an object
 * @param bl: Object to heal [PC|MOB|HOM|MER|ELEM]
 * @param hhp: How much HP to heal
 * @param hsp: How much SP to heal
 * @param flag:	Whether it's Forced(&1), gives HP/SP(&2) heal effect,
 *      or gives HP(&4) heal effect with 0 heal
 *		Forced healing overrides heal impedement statuses (Berserk)
 * @return hp+sp
 */
int status_heal(struct block_list *bl,int64 hhp,int64 hsp, int flag)
{
	struct status_data *status;
	struct status_change *sc;
	int hp = (int)cap_value(hhp,INT_MIN,INT_MAX);
	int sp = (int)cap_value(hsp,INT_MIN,INT_MAX);
	status = status_get_status_data(bl);

	if (status == &dummy_status || !status->hp)
		return 0;

	sc = status_get_sc(bl);
	if (sc && !sc->count)
		sc = NULL;

	if (hp < 0) {
		if (hp == INT_MIN) // -INT_MIN == INT_MIN in some architectures!
			hp++;
		status_damage(NULL, bl, -hp, 0, 0, 1, 0);
		hp = 0;
	}

	if(hp) {
		if( sc && (sc->data[STATUS_FRENZY]) ) {
			if( flag&1 )
				flag &= ~2;
			else
				hp = 0;
		}

		if((unsigned int)hp > status->max_hp - status->hp)
			hp = status->max_hp - status->hp;
	}

	if(sp < 0) {
		if (sp == INT_MIN)
			sp++;
		status_damage(NULL, bl, 0, -sp, 0, 1, 0);
		sp = 0;
	}

	if(sp) {
		if((unsigned int)sp > status->max_sp - status->sp)
			sp = status->max_sp - status->sp;
	}

	if(!sp && !hp && !(flag&4))
		return 0;

	status->hp += hp;
	status->sp += sp;

	if(hp && sc &&
		sc->data[STATUS_ANGER] &&
		sc->data[STATUS_PROVOKE] &&
		sc->data[STATUS_PROVOKE]->val2==1 &&
		status->hp>=status->max_hp/3
	)	// End auto berserk.
		status_change_end(bl, STATUS_PROVOKE, INVALID_TIMER);

	// Send HP update to client

	switch(bl->type) {
		case BL_PC:  pc_heal((TBL_PC*)bl,hp,sp,flag); break;
		case BL_MOB: mob_heal((TBL_MOB*)bl,hp); break;
		case BL_ELEM: elementSK_AL_HEAL((TBL_ELEM*)bl,hp,sp); break;
	}

	return (int)hp+sp;
}

/**
 * Applies percentage based damage to a unit.
 * If a mob is killed this way and there is no src, no EXP/Drops will be awarded.
 * @param src: Object initiating HP/SP modification [PC|MOB|PET|HOM|MER|ELEM]
 * @param target: Object to modify HP/SP
 * @param hp_rate: Percentage of HP to modify. If > 0:percent is of current HP, if < 0:percent is of max HP
 * @param sp_rate: Percentage of SP to modify. If > 0:percent is of current SP, if < 0:percent is of max SP
 * @param flag: \n
 *		0: Heal target \n 
 *		1: Use status_damage \n 
 *		2: Use status_damage and make sure target must not die from subtraction
 * @return hp+sp through status_heal()
 */
int status_percent_change(struct block_list *src, struct block_list *target, int8 hp_rate, int8 sp_rate, uint8 flag)
{
	struct status_data *status;
	unsigned int hp = 0, sp = 0;

	status = status_get_status_data(target);


	// It's safe now [MarkZD]
	if (hp_rate > 99)
		hp = status->hp;
	else if (hp_rate > 0)
		hp = apply_rate(status->hp, hp_rate);
	else if (hp_rate < -99)
		hp = status->max_hp;
	else if (hp_rate < 0)
		hp = (apply_rate(status->max_hp, -hp_rate));
	if (hp_rate && !hp)
		hp = 1;

	if (flag == 2 && hp >= status->hp)
		hp = status->hp-1; // Must not kill target.

	if (sp_rate > 99)
		sp = status->sp;
	else if (sp_rate > 0)
		sp = apply_rate(status->sp, sp_rate);
	else if (sp_rate < -99)
		sp = status->max_sp;
	else if (sp_rate < 0)
		sp = (apply_rate(status->max_sp, -sp_rate));
	if (sp_rate && !sp)
		sp = 1;

	// Ugly check in case damage dealt is too much for the received args of
	// status_heal / status_damage. [Skotlex]
	if (hp > INT_MAX) {
		hp -= INT_MAX;
		if (flag)
			status_damage(src, target, INT_MAX, 0, 0, (!src||src==target?5:1), 0);
		else
			status_heal(target, INT_MAX, 0, 0);
	}
	if (sp > INT_MAX) {
		sp -= INT_MAX;
		if (flag)
			status_damage(src, target, 0, INT_MAX, 0, (!src||src==target?5:1), 0);
		else
			status_heal(target, 0, INT_MAX, 0);
	}
	if (flag)
		return status_damage(src, target, hp, sp, 0, (!src||src==target?5:1), 0);
	return status_heal(target, hp, sp, 0);
}

/**
 * Revives a unit
 * @param bl: Object to revive [PC|MOB|HOM]
 * @param per_hp: Percentage of HP to revive with
 * @param per_sp: Percentage of SP to revive with
 * @return Successful (1) or Invalid target (0)
 */
int status_revive(struct block_list *bl, unsigned char per_hp, unsigned char per_sp)
{
	struct status_data *status;
	unsigned int hp, sp;
	if (!status_isdead(bl)) return 0;

	status = status_get_status_data(bl);
	if (status == &dummy_status)
		return 0; // Invalid target.

	hp = (int64)status->max_hp * per_hp/100;
	sp = (int64)status->max_sp * per_sp/100;

	if(hp > status->max_hp - status->hp)
		hp = status->max_hp - status->hp;
	else if (per_hp && !hp)
		hp = 1;

	if(sp > status->max_sp - status->sp)
		sp = status->max_sp - status->sp;
	else if (per_sp && !sp)
		sp = 1;

	status->hp += hp;
	status->sp += sp;

	if (bl->prev) // Animation only if character is already on a map.
		clif_resurrection(bl, 1);
	switch (bl->type) {
		case BL_PC:  pc_revive((TBL_PC*)bl, hp, sp); break;
		case BL_MOB: mob_revive((TBL_MOB*)bl, hp); break;
	}
	return 1;
}

/**
 * Checks whether the src can use the skill on the target,
 * taking into account status/option of both source/target
 * @param src:	Object using skill on target [PC|MOB|PET|HOM|MER|ELEM]
		src MAY be NULL to indicate we shouldn't check it, this is a ground-based skill attack
 * @param target: Object being targeted by src [PC|MOB|HOM|MER|ELEM]
		 target MAY be NULL, which checks if src can cast skill_id on the ground
 * @param skill_id: Skill ID being used on target
 * @param flag:	0 - Trying to use skill on target
 *		1 - Cast bar is done
 *		2 - Skill already pulled off, check is due to ground-based skills or splash-damage ones
 * @return src can use skill (1) or cannot use skill (0)
 * @author [Skotlex]
 */
bool status_check_skilluse(struct block_list *src, struct block_list *target, uint16 skill_id, int flag) {
	struct status_data *status;
	struct status_change *sc = NULL, *tsc;
	int hide_flag;

	status = src ? status_get_status_data(src) : &dummy_status;

	if (src && src->type != BL_PC && status_isdead(src))
		return false;

	if (!skill_id) { // Normal attack checks.
		// This mode is only needed for melee attacking.
		if (!status_has_mode(status,MD_CANATTACK))
			return false;
		// Dead state is not checked for skills as some skills can be used
		// on dead characters, said checks are left to skill.cpp [Skotlex]
		if (target && status_isdead(target))
			return false;
	}

	switch( skill_id ) {
		default:
			break;
	}

	if ( src )
		sc = status_get_sc(src);

	if( sc && sc->count ) {
		if (sc->data[STATUS_ALL_RIDING])
			return false; //You can't use skills while in the new mounts (The client doesn't let you, this is to make cheat-safe)

		

		if (sc->opt1 && sc->opt1 != OPT1_BURNING) { // Stuned/Frozen/etc
			if (flag != 1) // Can't cast, casted stuff can't damage.
				return false;
			if (skill_get_casttype(skill_id) == CAST_DAMAGE)
				return false; // Damage spells stop casting.
		}

		if (
			(sc->data[STATUS_TRICKDEAD] && skill_id != SK_NV_TRICKDEAD)
			|| (sc->data[STATUS_AUTOCOUNTER] && !flag && skill_id)
			// || (sc->data[STATUS_GOSPEL] && sc->data[STATUS_GOSPEL]->val4 == BCT_SELF && skill_id != SK_PA_GOSPEL
		)
			return false;

		

		if (sc->data[STATUS_GRAPPLE]) {
			switch (sc->data[STATUS_GRAPPLE]->val1) {
				case 5: if (skill_id == SK_SH_ASURASTRIKE) break;
				case 4: if (skill_id == SK_MO_PALMSTRIKE) break;
				case 3: if (skill_id == SK_MO_OCCULTIMPACT) break;
				case 2: if (skill_id == SK_MO_TRIPLEARMCANNON) break;
				case 1: if (skill_id == SK_MO_BODYRELOCATION) break;
				default: return false;
			}
		}

		

		if (skill_id && // Do not block item-casted skills.
			(src->type != BL_PC || ((TBL_PC*)src)->skillitem != skill_id)
		) {	// Skills blocked through status changes...
			if (!flag && ( // Blocked only from using the skill (stuff like autospell may still go through
				sc->cant.cast ||
#ifndef RENEWAL
				(sc->data[SC_BASILICA] && (sc->data[SC_BASILICA]->val4 != src->id || skill_id != HP_BASILICA)) || // Only Basilica caster that can cast, and only Basilica to cancel it
#endif
				// (sc->data[STATUS_MARIONETTE] && skill_id != SK_CL_MARIONETTECONTROL) || // Only skill you can use is marionette again to cancel it
				(sc->data[STATUS_MARIONETTE2] && skill_id == SK_CL_MARIONETTECONTROL) || // Cannot use marionette if you are being buffed by another
				
				(sc->data[STATUS_STASIS] && skill_block_check(src, STATUS_STASIS, skill_id)) 
				

			))
				return false;

			// Skill blocking.
			if (
				
				(sc->data[STATUS_NOCHAT] && sc->data[STATUS_NOCHAT]->val1&MANNER_NOSKILL)
			)
				return false;
		}

		if (sc->option) {
			if ((sc->option&OPTION_HIDE) && src->type == BL_PC && (skill_id == 0 || !skill_get_inf2(skill_id, INF2_ALLOWWHENHIDDEN))) {
				// Non players can use all skills while hidden.
				return false;
			}
			if (sc->option&OPTION_CHASEWALK && skill_id != SK_AS_STEALTH)
				return false;
		}
	}

	if (target == NULL || target == src) // No further checking needed.
		return true;

	tsc = status_get_sc(target);

	if (tsc && tsc->count) {
		/**
		* Attacks in invincible are capped to 1 damage and handled in battle.cpp.
		* Allow spell break and eske for sealed shrine GDB when in INVINCIBLE state.
		**/
		
		if(!skill_id && tsc->data[STATUS_TRICKDEAD])
			return false;
		if((skill_id == SK_WZ_STORMGUST)
			&& tsc->data[STATUS_FREEZE])
			return false;
		if(skill_id == SK_PR_LEXAETERNA && (tsc->data[STATUS_FREEZE] || (tsc->data[STATUS_STONECURSE] && tsc->opt1 == OPT1_STONE)))
			return false;
		
	}

	// If targetting, cloak+hide protect you, otherwise only hiding does.
	hide_flag = flag?OPTION_HIDE:(OPTION_HIDE|OPTION_CLOAK|OPTION_CHASEWALK);

 	// Skill that can hit hidden target
	if( skill_get_inf2(skill_id, INF2_TARGETHIDDEN) )
		hide_flag &= ~OPTION_HIDE;

	switch( target->type ) {
		case BL_PC: {
				struct map_session_data *tsd = (TBL_PC*)target;
				bool is_boss = (src && status_get_class_(src) == CLASS_BOSS);
				bool is_detect = status_has_mode(status,MD_DETECTOR);

				if (pc_isinvisible(tsd))
					return false;
				if (tsc) {
					if ((tsc->option&hide_flag) && !is_boss && (tsd->special_state.perfect_hiding || !is_detect))
						return false;

					if ((tsc->data[STATUS_CAMOUFLAGE]) && !(is_boss || is_detect) && (!skill_id || (!flag && src)))
						return false; // Insect, demon, and boss can detect
				}
			}
			break;
		case BL_ITEM: // Allow targetting of items to pick'em up (or in the case of mobs, to loot them).
			// !TODO: Would be nice if this could be used to judge whether the player can or not pick up the item it targets. [Skotlex]
			if (status_has_mode(status,MD_LOOTER))
				return true;
			return false;
		
		default:
			// Check for chase-walk/hiding/cloaking opponents.
			if( tsc ) {
				if( tsc->option&hide_flag && !status_has_mode(status,MD_DETECTOR))
					return false;
			}
	}
	return true;
}

/**
 * Checks whether the src can see the target
 * @param src:	Object using skill on target [PC|MOB|PET|HOM|MER|ELEM]
 * @param target: Object being targeted by src [PC|MOB|HOM|MER|ELEM]
 * @return src can see (1) or target is invisible (0)
 * @author [Skotlex]
 */
int status_check_visibility(struct block_list *src, struct block_list *target)
{
	int view_range;
	struct status_data* status = status_get_status_data(src);
	struct status_change* tsc = status_get_sc(target);
	switch (src->type) {
		case BL_MOB:
			view_range = ((TBL_MOB*)src)->min_chase;
			break;
		case BL_PET:
			view_range = ((TBL_PET*)src)->db->range2;
			break;
		default:
			view_range = AREA_SIZE;
	}

	if (src->m != target->m || !check_distance_bl(src, target, view_range))
		return 0;

	if ( src->type == BL_NPC) // NPCs don't care for the rest
		return 1;

	if (tsc) {
		bool is_boss = (status_get_class_(src) == CLASS_BOSS);
		bool is_detector = status_has_mode(status,MD_DETECTOR);

		switch (target->type) {	// Check for chase-walk/hiding/cloaking opponents.
			case BL_PC: {
					struct map_session_data *tsd = (TBL_PC*)target;

					if (((tsc->option&(OPTION_HIDE|OPTION_CLOAK|OPTION_CHASEWALK)) || tsc->data[STATUS_CAMOUFLAGE]) && !is_boss && (tsd->special_state.perfect_hiding || !is_detector))
						return 0;
					
				}
				break;
			default:
				if (((tsc->option&(OPTION_HIDE|OPTION_CLOAK|OPTION_CHASEWALK)) || tsc->data[STATUS_CAMOUFLAGE]) && !is_boss && !is_detector)
					return 0;
		}
	}

	return 1;
}

/**
 * Base ASPD value taken from the job tables
 * @param sd: Player object
 * @param status: Player status
 * @return base amotion after single/dual weapon and shield adjustments [RENEWAL]
 *	  base amotion after single/dual weapon and stats adjustments [PRE-RENEWAL]
 */
int status_base_amotion_pc(struct map_session_data* sd, struct status_data* status)
{
	int amotion;
	int classidx = pc_class2idx(sd->status.class_);

	int16 skill_lv, val = 0;
	float temp_aspd = 0;

	amotion = job_info[classidx].aspd_base[sd->weapontype1]; // Single weapon
	if (sd->status.shield)
		amotion += job_info[classidx].aspd_base[MAX_WEAPON_TYPE];
	else if (sd->weapontype2 != W_FIST && sd->equip_index[EQI_HAND_R] != sd->equip_index[EQI_HAND_L])
		amotion += job_info[classidx].aspd_base[sd->weapontype2] / 4; // Dual-wield

	switch(sd->status.weapon) {
		case W_BOW:
		case W_MUSICAL:
		case W_WHIP:
		case W_REVOLVER:
		case W_RIFLE:
		case W_GATLING:
		case W_SHOTGUN:
		case W_GRENADE:
			temp_aspd = status->dex * status->dex / 7.0f + status->agi * status->agi * 0.5f;
			break;
		default:
			temp_aspd = status->dex * status->dex / 5.0f + status->agi * status->agi * 0.5f;
			break;
	}
	temp_aspd = (float)(sqrt(temp_aspd) * 0.25f) + 0xc4;
	
	
	
	if (pc_isriding(sd))
		val -= 50 - 10 * 5; //pc_checkskill(sd, KN_CAVALIERMASTERY)
	amotion = ((int)(temp_aspd + ((float)(status_calc_aspd(&sd->bl, &sd->sc, true) + val) * status->agi / 200)) - min(amotion, 200));

 	return amotion;
}

/**
 * Base attack value calculated for units
 * @param bl: Object to get attack for [BL_CHAR|BL_NPC]
 * @param status: Object status
 * @return base attack
 */
unsigned short status_base_atk(const struct block_list *bl, const struct status_data *status, int level)
{
	int flag = 0, str, dex, dstr;

#ifdef RENEWAL
	if (!(bl->type&battle_config.enable_baseatk_renewal))
		return 0;
#else
	if (!(bl->type&battle_config.enable_baseatk))
		return 0;
#endif

	if (bl->type == BL_PC)
	switch(((TBL_PC*)bl)->status.weapon) {
		case W_BOW:
		case W_MUSICAL:
		case W_WHIP:
		case W_REVOLVER:
		case W_RIFLE:
		case W_GATLING:
		case W_SHOTGUN:
		case W_GRENADE:
			flag = 1;
	}
	if (flag) {
#ifdef RENEWAL
		dstr =
#endif
		str = status->dex;
		dex = status->str;
	} else {
#ifdef RENEWAL
		dstr =
#endif
		str = status->str;
		dex = status->dex;
	}

	/** [Skotlex]
	* Normally only players have base-atk, but homunc have a different batk
	* equation, hinting that perhaps non-players should use this for batk.
	**/
	switch (bl->type) {
		
		case BL_PC:
#ifdef RENEWAL
			str = (dstr * 10 + dex * 10 / 5 + status->luk * 10 / 3 + level * 10 / 4) / 10;
#else
			dstr = str / 10;
			str += dstr*dstr;
			str += dex / 5 + status->luk / 5;
#endif
			break;
		default:// Others
#ifdef RENEWAL
			str = dstr + level;
#else
			dstr = str / 10;
			str += dstr*dstr;
			str += dex / 5 + status->luk / 5;
#endif
			break;
	}

	return cap_value(str, 0, USHRT_MAX);
}

#ifdef RENEWAL
/**
 * Weapon attack value calculated for Players
 * @param wa: Weapon attack
 * @param status: Player status
 * @return weapon attack
 */
unsigned int status_weapon_atk(weapon_atk &wa)
{
	return wa.atk + wa.atk2;
}
#endif

#ifndef RENEWAL
unsigned short status_base_matk_min(const struct status_data* status) { return status->int_ + (status->int_ / 7) * (status->int_ / 7); }
unsigned short status_base_matk_max(const struct status_data* status) { return status->int_ + (status->int_ / 5) * (status->int_ / 5); }
#else
/*
* Calculates minimum attack variance 80% from db's ATK1 for non BL_PC
* status->batk (base attack) will be added in battle_calc_base_damage
*/
unsigned short status_base_atk_min(struct block_list *bl, const struct status_data* status, int level)
{
	switch (bl->type) {
		case BL_PET:
		case BL_MOB:
		case BL_MER:
		case BL_ELEM:
			return status->rhw.atk * 80 / 100;
		default:
			return status->rhw.atk;
	}
}

/*
* Calculates maximum attack variance 120% from db's ATK1 for non BL_PC
* status->batk (base attack) will be added in battle_calc_base_damage
*/
unsigned short status_base_atk_max(struct block_list *bl, const struct status_data* status, int level)
{
	switch (bl->type) {
		case BL_PET:
		case BL_MOB:
		case BL_MER:
		case BL_ELEM:
			return status->rhw.atk * 120 / 100;
		default:
			return status->rhw.atk2;
	}
}

/*
* Calculates minimum magic attack
*/
unsigned short status_base_matk_min(struct block_list *bl, const struct status_data* status, int level)
{
	switch (bl->type) {
		case BL_PET:
		case BL_MOB:
		case BL_MER:
		case BL_ELEM:
			return status->int_ + level + status->rhw.matk * 70 / 100;
		case BL_PC:
		default:
			return status->int_ + (status->int_ / 2) + (status->dex / 5) + (status->luk / 3) + (level / 4);
	}
}

/*
* Calculates maximum magic attack
*/
unsigned short status_base_matk_max(struct block_list *bl, const struct status_data* status, int level)
{
	switch (bl->type) {
		case BL_PET:
		case BL_MOB:
		case BL_MER:
		case BL_ELEM:
			return status->int_ + level + status->rhw.matk * 130 / 100;
		case BL_PC:
		default:
			return status->rhw.matk;
			// return status->int_ + (status->int_ / 2) + (status->dex / 5) + (status->luk / 3) + (level / 4);
	}
}
#endif

/**
 * Fills in the misc data that can be calculated from the other status info (except for level)
 * @param bl: Object to calculate status on [PC|MOB|PET|HOM|MERC|ELEM]
 * @param status: Player status
 */
void status_calc_misc(struct block_list *bl, struct status_data *status, int level)
{
	int stat;

	// Non players get the value set, players need to stack with previous bonuses.
	if( bl->type != BL_PC )
		status->batk =
		status->hit = status->flee =
		status->def2 = status->mdef2 =
		status->cri = status->flee2 = 0;

#ifdef RENEWAL // Renewal formulas
	
		// Hit
		stat = status->hit;
		stat += level + status->dex + (bl->type == BL_PC ? status->luk / 3 + 175 : 150); //base level + ( every 1 dex = +1 hit ) + (every 3 luk = +1 hit) + 175
		status->hit = cap_value(stat, 1, SHRT_MAX);
		// Flee
		stat = status->flee;
		stat += level + status->agi + (bl->type == BL_MER ? 0 : bl->type == BL_PC ? status->luk / 5 : 0) + 100; //base level + ( every 1 agi = +1 flee ) + (every 5 luk = +1 flee) + 100
		status->flee = cap_value(stat, 1, SHRT_MAX);
		// Def2
		if (bl->type == BL_MER)
			stat = (int)(status->vit + ((float)level / 10) + ((float)status->vit / 5));
		else {
			stat = status->def2;
			stat += (int)(((float)level + status->vit) / 2 + (bl->type == BL_PC ? ((float)status->agi / 5) : 0)); //base level + (every 2 vit = +1 def) + (every 5 agi = +1 def)
		}
		status->def2 = cap_value(stat, 0, SHRT_MAX);
		// Mdef2
		if (bl->type == BL_MER)
			stat = (int)(((float)level / 10) + ((float)status->int_ / 5));
		else {
			stat = status->mdef2;
			stat += (int)(bl->type == BL_PC ? (status->int_ + ((float)level / 4) + ((float)(status->dex + status->vit) / 5)) : ((float)(status->int_ + level) / 4)); //(every 4 base level = +1 mdef) + (every 1 int = +1 mdef) + (every 5 dex = +1 mdef) + (every 5 vit = +1 mdef)
		}
		status->mdef2 = cap_value(stat, 0, SHRT_MAX);
	

	// ATK
	if (bl->type != BL_PC) {
		status->rhw.atk2 = status_base_atk_max(bl, status, level);
		status->rhw.atk = status_base_atk_min(bl, status, level);
	}

	// MAtk
	status->matk_min = status_base_matk_min(bl, status, level);
	status->matk_max = status_base_matk_max(bl, status, level);
#else
	// Matk
	status->matk_min = status_base_matk_min(status);
	status->matk_max = status_base_matk_max(status);
	// Hit
	stat = status->hit;
	stat += level + status->dex;
	status->hit = cap_value(stat, 1, SHRT_MAX);
	// Flee
	stat = status->flee;
	stat += level + status->agi;
	status->flee = cap_value(stat, 1, SHRT_MAX);
	// Def2
	stat = status->def2;
	stat += status->vit;
	status->def2 = cap_value(stat, 0, SHRT_MAX);
	// Mdef2
	stat = status->mdef2;
	stat += status->int_ + (status->vit>>1);
	status->mdef2 = cap_value(stat, 0, SHRT_MAX);
#endif

	//Critical
	if( bl->type&battle_config.enable_critical ) {
		stat = status->cri;
		stat += 10 + (status->luk*10/3); // (every 1 luk = +0.3 critical)
		status->cri = cap_value(stat, 1, SHRT_MAX);
	} else
		status->cri = 0;

	if (bl->type&battle_config.enable_perfect_flee) {
		stat = status->flee2;
		stat += status->luk + 10; // (every 10 luk = +1 perfect flee)
		status->flee2 = cap_value(stat, 0, SHRT_MAX);
	} else
		status->flee2 = 0;

	if (status->batk) {
		int temp = status->batk + status_base_atk(bl, status, level);
		status->batk = cap_value(temp, 0, USHRT_MAX);
	} else
		status->batk = status_base_atk(bl, status, level);

	if (status->cri) {
		switch (bl->type) {
			case BL_MOB:
				if(battle_config.mob_critical_rate != 100)
					status->cri = cap_value(status->cri*battle_config.mob_critical_rate/100,1,SHRT_MAX);
				if(!status->cri && battle_config.mob_critical_rate)
					status->cri = 10;
				break;
			case BL_PC:
				// Players don't have a critical adjustment setting as of yet.
				break;
			default:
				if(battle_config.critical_rate != 100)
					status->cri = cap_value(status->cri*battle_config.critical_rate/100,1,SHRT_MAX);
				if (!status->cri && battle_config.critical_rate)
					status->cri = 10;
		}
	}

	if(bl->type&BL_REGEN)
		status_calc_regen(bl, status, status_get_regen_data(bl));
}

/**
 * Calculates the initial status for the given mob
 * @param md: Mob object
 * @param opt: Whether or not it is the first calculation
		This will only be false when a mob levels up (Regular and WoE Guardians)
 * @return 1 for calculated special statuses or 0 for none
 * @author [Skotlex]
 */
int status_calc_mob_(struct mob_data* md, enum e_status_calc_opt opt)
{
	struct status_data *status;
	struct block_list *mbl = NULL;
	int flag=0;

	if (opt&SCO_FIRST) { // Set basic level on respawn.
		if (md->level > 0 && md->level <= MAX_LEVEL && md->level != md->db->lv)
			;
		else
			md->level = md->db->lv;
	}

	// Check if we need custom base-status
	if (battle_config.mobs_level_up && md->level > md->db->lv)
		flag|=1;

	if (md->special_state.size)
		flag|=2;

	if (md->guardian_data && md->guardian_data->guardup_lv)
		flag|=4;
	if (md->mob_id == MOBID_EMPERIUM)
		flag|=4;

	if (battle_config.slaves_inherit_speed && md->master_id)
		flag|=8;

	if (md->master_id && md->special_state.ai>AI_ATTACK)
		flag|=16;

	if (md->master_id && battle_config.slaves_inherit_mode)
		flag |= 32;

	if (!flag) { // No special status required.
		if (md->base_status) {
			aFree(md->base_status);
			md->base_status = NULL;
		}
		if (opt&SCO_FIRST)
			memcpy(&md->status, &md->db->status, sizeof(struct status_data));
		return 0;
	}
	if (!md->base_status)
		md->base_status = (struct status_data*)aCalloc(1, sizeof(struct status_data));

	status = md->base_status;
	memcpy(status, &md->db->status, sizeof(struct status_data));

	if (flag&(8|16))
		mbl = map_id2bl(md->master_id);

	if (flag&8 && mbl) {
		struct status_data *mstatus = status_get_base_status(mbl);

		if (mstatus &&
			battle_config.slaves_inherit_speed&(status_has_mode(mstatus,MD_CANMOVE)?1:2))
			status->speed = mstatus->speed;
		if( status->speed < 2 ) // Minimum for the unit to function properly
			status->speed = 2;
	}

	if (flag&32)
		status_calc_slave_mode(md, map_id2md(md->master_id));

	if (flag&1) { // Increase from mobs leveling up [Valaris]
		int diff = md->level - md->db->lv;

		status->str += diff;
		status->agi += diff;
		status->vit += diff;
		status->int_ += diff;
		status->dex += diff;
		status->luk += diff;
		status->max_hp += diff * status->vit;
		status->max_sp += diff * status->int_;
		status->hp = status->max_hp;
		status->sp = status->max_sp;
		status->speed -= cap_value(diff, 0, status->speed - 10);
	}

	if (flag&2 && battle_config.mob_size_influence) { // Change for sized monsters [Valaris]
		if (md->special_state.size == SZ_MEDIUM) {
			status->max_hp >>= 1;
			status->max_sp >>= 1;
			if (!status->max_hp) status->max_hp = 1;
			if (!status->max_sp) status->max_sp = 1;
			status->hp = status->max_hp;
			status->sp = status->max_sp;
			status->str >>= 1;
			status->agi >>= 1;
			status->vit >>= 1;
			status->int_ >>= 1;
			status->dex >>= 1;
			status->luk >>= 1;
			if (!status->str) status->str = 1;
			if (!status->agi) status->agi = 1;
			if (!status->vit) status->vit = 1;
			if (!status->int_) status->int_ = 1;
			if (!status->dex) status->dex = 1;
			if (!status->luk) status->luk = 1;
		} else if (md->special_state.size == SZ_BIG) {
			status->max_hp <<= 1;
			status->max_sp <<= 1;
			status->hp = status->max_hp;
			status->sp = status->max_sp;
			status->str <<= 1;
			status->agi <<= 1;
			status->vit <<= 1;
			status->int_ <<= 1;
			status->dex <<= 1;
			status->luk <<= 1;
		}
	}

	status_calc_misc(&md->bl, status, md->level);

	if(flag&4) { // Strengthen Guardians - custom value +10% / lv
		struct guild_castle *gc;
		struct map_data *mapdata = map_getmapdata(md->bl.m);

		gc=guild_mapname2gc(mapdata->name);
		if (!gc)
			ShowError("status_calc_mob: No castle set at map %s\n", mapdata->name);
		else if(gc->castle_id < 24 || md->mob_id == MOBID_EMPERIUM) {
#ifdef RENEWAL
			status->max_hp += 50 * (gc->defense / 5);
#else
			status->max_hp += 1000 * gc->defense;
#endif
			status->hp = status->max_hp;
			status->def += (gc->defense+2)/3;
			status->mdef += (gc->defense+2)/3;
		}
		if(md->mob_id != MOBID_EMPERIUM) {
			status->max_hp += 1000 * gc->defense;
			status->hp = status->max_hp;
			status->batk += 2 * md->guardian_data->guardup_lv + 8;
			status->rhw.atk += 2 * md->guardian_data->guardup_lv + 8;
			status->rhw.atk2 += 2 * md->guardian_data->guardup_lv + 8;
			status->aspd_rate -= 2 * md->guardian_data->guardup_lv + 3;
		}
	}

	if (flag&16 && mbl) { // Max HP setting from Summon Flora/marine Sphere
		struct unit_data *ud = unit_bl2ud(mbl);
		// Remove special AI when this is used by regular mobs.
		if (mbl->type == BL_MOB && !((TBL_MOB*)mbl)->special_state.ai)
			md->special_state.ai = AI_NONE;
		if (ud) { 
			// Different levels of HP according to skill level
			if(!ud->skill_id) // !FIXME: We lost the unit data for magic decoy in somewhere before this
				ud->skill_id = ((TBL_PC*)mbl)->menuskill_id;
			switch(ud->skill_id) {
				case SK_HT_MYSTICTOTEM:
				case SK_HT_ELEMENTALTOTEM:
				{
					struct status_data *mstatus = status_get_status_data(mbl);
					if(!mstatus)
						break;
					
					struct map_session_data *msd;
					if ((msd = map_id2sd(md->master_id)) != NULL) {
						status->max_hp = msd->status.max_hp;
						status->max_sp = 9999;
						status->str = msd->status.str;
						status->agi = msd->status.agi;
						status->vit = msd->status.vit;
						status->int_ = msd->status.int_;
						status->dex = msd->status.dex;
						status->luk = msd->status.luk;
						md->masterteleport_timer = INVALID_TIMER;
						md->master = msd;
						if (!msd->td){
							msd->td = md;
						} else {
							msd->td2 = md;
						}
					}
					// status->max_hp = (1000 * ((TBL_PC*)mbl)->menuskill_val) + (mstatus->sp * 4) + (status_get_lv(mbl) * 12);
					// status->matk_min = status->matk_max = 250 + 50*((TBL_PC*)mbl)->menuskill_val;
					status->matk_max = mstatus->matk_max;
					status->matk_min = mstatus->matk_min;
					status->batk = mstatus->watk;
					break;
				}
			}
			status->hp = status->max_hp;
		}
	}

	if (opt&SCO_FIRST) // Initial battle status
		memcpy(&md->status, status, sizeof(struct status_data));

	return 1;
}

/**
 * Calculates the stats of the given pet
 * @param pd: Pet object
 * @param opt: Whether or not it is the first calculation
		This will only be false when a pet levels up
 * @return 1
 * @author [Skotlex]
 */
void status_calc_pet_(struct pet_data *pd, enum e_status_calc_opt opt)
{
	nullpo_retv(pd);

	if (opt&SCO_FIRST) {
		memcpy(&pd->status, &pd->db->status, sizeof(struct status_data));
		pd->status.mode = MD_CANMOVE; // Pets discard all modes, except walking
		pd->status.class_ = CLASS_NORMAL;
		pd->status.speed = pd->get_pet_walk_speed();

		if(battle_config.pet_attack_support || battle_config.pet_damage_support) {
			// Attack support requires the pet to be able to attack
			pd->status.mode = static_cast<e_mode>(pd->status.mode|MD_CANATTACK);
		}
	}

	if (battle_config.pet_lv_rate && pd->master) {
		struct map_session_data *sd = pd->master;
		int lv;

		lv =sd->status.base_level*battle_config.pet_lv_rate/100;
		if (lv < 0)
			lv = 1;
		if (lv != pd->pet.level || opt&SCO_FIRST) {
			struct status_data *bstat = &pd->db->status, *status = &pd->status;

			pd->pet.level = lv;
			if (!(opt&SCO_FIRST)) // Lv Up animation
				clif_misceffect(&pd->bl, 0);
			status->rhw.atk = (bstat->rhw.atk*lv)/pd->db->lv;
			status->rhw.atk2 = (bstat->rhw.atk2*lv)/pd->db->lv;
			status->str = (bstat->str*lv)/pd->db->lv;
			status->agi = (bstat->agi*lv)/pd->db->lv;
			status->vit = (bstat->vit*lv)/pd->db->lv;
			status->int_ = (bstat->int_*lv)/pd->db->lv;
			status->dex = (bstat->dex*lv)/pd->db->lv;
			status->luk = (bstat->luk*lv)/pd->db->lv;

			status->rhw.atk = cap_value(status->rhw.atk, 1, battle_config.pet_max_atk1);
			status->rhw.atk2 = cap_value(status->rhw.atk2, 2, battle_config.pet_max_atk2);
			status->str = cap_value(status->str,1,battle_config.pet_max_stats);
			status->agi = cap_value(status->agi,1,battle_config.pet_max_stats);
			status->vit = cap_value(status->vit,1,battle_config.pet_max_stats);
			status->int_= cap_value(status->int_,1,battle_config.pet_max_stats);
			status->dex = cap_value(status->dex,1,battle_config.pet_max_stats);
			status->luk = cap_value(status->luk,1,battle_config.pet_max_stats);

			status_calc_misc(&pd->bl, &pd->status, lv);

			if (!(opt&SCO_FIRST)) // Not done the first time because the pet is not visible yet
				clif_send_petstatus(sd);
		}
	} else if (opt&SCO_FIRST) {
		status_calc_misc(&pd->bl, &pd->status, pd->db->lv);
		if (!battle_config.pet_lv_rate && pd->pet.level != pd->db->lv)
			pd->pet.level = pd->db->lv;
	}

	// Support rate modifier (1000 = 100%)
	pd->rate_fix = min(1000 * (pd->pet.intimate - battle_config.pet_support_min_friendly) / (1000 - battle_config.pet_support_min_friendly) + 500, USHRT_MAX);
	pd->rate_fix = min(apply_rate(pd->rate_fix, battle_config.pet_support_rate), USHRT_MAX);
}

/**
 * Get HP bonus modifiers
 * @param bl: block_list that will be checked
 * @param type: type of e_status_bonus (STATUS_BONUS_FIX or STATUS_BONUS_RATE)
 * @return bonus: total bonus for HP
 * @author [Cydh]
 */
static int status_get_hpbonus(struct block_list *bl, enum e_status_bonus type) {
	int bonus = 0;

	if (type == STATUS_BONUS_FIX) {
		struct status_change *sc = status_get_sc(bl);

		//Only for BL_PC
		if (bl->type == BL_PC) {
			struct map_session_data *sd = map_id2sd(bl->id);
			uint16 skill_lv;

			bonus += sd->bonus.hp;
			if ((skill_lv = pc_checkskill(sd,SK_SM_INNER_STRENGTH)) > 0)
				bonus += skill_lv * 400;
#ifndef HP_SP_TABLES
			if ((sd->class_&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE && sd->status.base_level >= 99)
				bonus += 2000; // Supernovice lvl99 hp bonus.
			if ((sd->class_&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE && sd->status.base_level >= 150)
				bonus += 2000; // Supernovice lvl150 hp bonus.
#endif
		}

		//Bonus by SC
		if (sc) {
			if( sc->data[STATUS_ACCOUSTICRYTHM] )
				bonus += sc->data[STATUS_ACCOUSTICRYTHM]->val3;
			if(sc->data[STATUS_SWORDCLAN])
				bonus += 30;
			if(sc->data[STATUS_ARCWANDCLAN])
				bonus += 30;
			if(sc->data[STATUS_GOLDENMACECLAN])
				bonus += 30;
			if(sc->data[STATUS_CROSSBOWCLAN])
				bonus += 30;
		}
	} else if (type == STATUS_BONUS_RATE) {
		struct status_change *sc = status_get_sc(bl);


		// Max rate reduce is -100%
		bonus = cap_value(bonus,-100,INT_MAX);
	}

	return min(bonus,INT_MAX);
}

/**
* HP bonus rate from equipment
*/
static int status_get_hpbonus_equip(TBL_PC *sd) {
	int bonus = 0;

	bonus += sd->hprate;

	return bonus -= 100; //Default hprate is 100, so it should be add 0%
}

/**
* HP bonus rate from usable items
*/
static int status_get_hpbonus_item(block_list *bl) {
	int bonus = 0;

	struct status_change *sc = status_get_sc(bl);



	// Max rate reduce is -100%
	return cap_value(bonus, -100, INT_MAX);
}

/**
 * Get SP bonus modifiers
 * @param bl: block_list that will be checked
 * @param type: type of e_status_bonus (STATUS_BONUS_FIX or STATUS_BONUS_RATE)
 * @return bonus: total bonus for SP
 * @author [Cydh]
 */
static int status_get_spbonus(struct block_list *bl, enum e_status_bonus type) {
	int bonus = 0;

	if (type == STATUS_BONUS_FIX) {
		struct status_change *sc = status_get_sc(bl);

		//Only for BL_PC
		if (bl->type == BL_PC) {
			struct map_session_data *sd = map_id2sd(bl->id);
			uint16 skill_lv;

			bonus += sd->bonus.sp;
		}

		//Bonus by SC
		if (sc) {
			if( sc->data[STATUS_ACCOUSTICRYTHM] )
				bonus += sc->data[STATUS_ACCOUSTICRYTHM]->val3;
			if(sc->data[STATUS_SWORDCLAN])
				bonus += 10;
			if(sc->data[STATUS_ARCWANDCLAN])
				bonus += 10;
			if(sc->data[STATUS_GOLDENMACECLAN])
				bonus += 10;
			if(sc->data[STATUS_CROSSBOWCLAN])
				bonus += 10;

		}
	} else if (type == STATUS_BONUS_RATE) {
		struct status_change *sc = status_get_sc(bl);

		//Only for BL_PC
		if (bl->type == BL_PC) {
			struct map_session_data *sd = map_id2sd(bl->id);
			uint8 i;

			if((i = (pc_checkskill(sd,SK_SM_MEDITATE)*2)) > 0)
				bonus += i;
			

			if ((i = (pc_checkskill(sd, ( SK_BA_SHOWMANSHIP))*2)) > 0)
				bonus += i;


		}

	
		// Max rate reduce is -100%
		bonus = cap_value(bonus,-100,INT_MAX);
	}

	return min(bonus,INT_MAX);
}

/**
* SP bonus rate from equipment
*/
static int status_get_spbonus_equip(TBL_PC *sd) {
	int bonus = 0;

	bonus += sd->sprate;

	return bonus -= 100; //Default sprate is 100, so it should be add 0%
}

/**
* SP bonus rate from usable items
*/
static int status_get_spbonus_item(block_list *bl) {
	int bonus = 0;

	struct status_change *sc = status_get_sc(bl);


	// Max rate reduce is -100%
	return cap_value(bonus, -100, INT_MAX);
}

/**
 * Get final MaxHP or MaxSP for player. References: http://irowiki.org/wiki/Max_HP and http://irowiki.org/wiki/Max_SP
 * The calculation needs base_level, base_status/battle_status (vit or int), additive modifier, and multiplicative modifier
 * @param sd Player
 * @param stat Vit/Int of player as param modifier
 * @param isHP true - calculates Max HP, false - calculated Max SP
 * @return max The max value of HP or SP
 */
static unsigned int status_calc_maxhpsp_pc(struct map_session_data* sd, unsigned int stat, bool isHP) {
	double dmax = 0;
	uint16 idx, level, job_id;

	nullpo_ret(sd);

	job_id = pc_mapid2jobid(sd->class_,sd->status.sex);
	idx = pc_class2idx(job_id);
	level = umax(sd->status.base_level,1);

	if (isHP) { //Calculates MaxHP
		double equip_bonus = 0, item_bonus = 0;
		dmax = job_info[idx].base_hp[level-1] * (1 + (umax(stat,1) * 0.01)) * ((sd->class_&JOBL_UPPER)?1.25:(pc_is_taekwon_ranker(sd))?3:1);
		dmax += status_get_hpbonus(&sd->bl,STATUS_BONUS_FIX);
		equip_bonus = (dmax * status_get_hpbonus_equip(sd) / 100);
		item_bonus = (dmax * status_get_hpbonus_item(&sd->bl) / 100);
		dmax += equip_bonus + item_bonus;
		dmax += (int64)(dmax * status_get_hpbonus(&sd->bl,STATUS_BONUS_RATE) / 100); //Aegis accuracy
	}
	else { //Calculates MaxSP
		double equip_bonus = 0, item_bonus = 0;
		dmax = job_info[idx].base_sp[level-1] * (1 + (umax(stat,1) * 0.01)) * ((sd->class_&JOBL_UPPER)?1.25:(pc_is_taekwon_ranker(sd))?3:1);
		dmax += status_get_spbonus(&sd->bl,STATUS_BONUS_FIX);
		equip_bonus = (dmax * status_get_spbonus_equip(sd) / 100);
		item_bonus = (dmax * status_get_spbonus_item(&sd->bl) / 100);
		dmax += equip_bonus + item_bonus;
		dmax += (int64)(dmax * status_get_spbonus(&sd->bl,STATUS_BONUS_RATE) / 100); //Aegis accuracy
	}

	//Make sure it's not negative before casting to unsigned int
	if(dmax < 1) dmax = 1;

	return cap_value((unsigned int)dmax,1,UINT_MAX);
}

/**
 * Calculates player's weight
 * @param sd: Player object
 * @param flag: Calculation type
 *   CALCWT_ITEM - Item weight
 *   CALCWT_MAXBONUS - Skill/Status/Configuration max weight bonus
 * @return false - failed, true - success
 */
bool status_calc_weight(struct map_session_data *sd, enum e_status_calc_weight_opt flag)
{
	int b_weight, b_max_weight, skill, i;
	struct status_change *sc;

	nullpo_retr(false, sd);

	sc = &sd->sc;
	b_max_weight = sd->max_weight; // Store max weight for later comparison
	b_weight = sd->weight; // Store current weight for later comparison
	sd->max_weight = job_info[pc_class2idx(sd->status.class_)].max_weight_base + sd->status.str * 300; // Recalculate max weight

	if (flag&CALCWT_ITEM) {
		sd->weight = 0; // Reset current weight

		for(i = 0; i < MAX_INVENTORY; i++) {
			if (!sd->inventory.u.items_inventory[i].nameid || sd->inventory_data[i] == NULL)
				continue;
			sd->weight += sd->inventory_data[i]->weight * sd->inventory.u.items_inventory[i].amount;
		}
	}

	if (flag&CALCWT_MAXBONUS) {
		// Skill/Status bonus weight increases
		sd->max_weight += sd->add_max_weight; // From bAddMaxWeight
	
		// if (pc_isriding(sd) && pc_checkskill(sd, KN_RIDING) > 0)
		// 	sd->max_weight += 10000;
		// else if (pc_isridingdragon(sd))
		// 	sd->max_weight += 5000 + 2000 * pc_checkskill(sd, RK_DRAGONTRAINING);
		
		if (pc_ismadogear(sd))
			sd->max_weight += 15000;
	}

	// Update the client if the new weight calculations don't match
	if (b_weight != sd->weight)
		clif_updatestatus(sd, SP_WEIGHT);
	if (b_max_weight != sd->max_weight) {
		clif_updatestatus(sd, SP_MAXWEIGHT);
		pc_updateweightstatus(sd);
	}

	return true;
}

/**
 * Calculates player's cart weight
 * @param sd: Player object
 * @param flag: Calculation type
 *   CALCWT_ITEM - Cart item weight
 *   CALCWT_MAXBONUS - Skill/Status/Configuration max weight bonus
 *   CALCWT_CARTSTATE - Whether to check for cart state
 * @return false - failed, true - success
 */
bool status_calc_cart_weight(struct map_session_data *sd, enum e_status_calc_weight_opt flag)
{
	int b_cart_weight_max, i;

	nullpo_retr(false, sd);

	if (!pc_iscarton(sd) && !(flag&CALCWT_CARTSTATE))
		return false;

	b_cart_weight_max = sd->cart_weight_max; // Store cart max weight for later comparison
	sd->cart_weight_max = battle_config.max_cart_weight; // Recalculate max weight

	if (flag&CALCWT_ITEM) {
		sd->cart_weight = 0; // Reset current cart weight
		sd->cart_num = 0; // Reset current cart item count

		for(i = 0; i < MAX_CART; i++) {
			if (!sd->cart.u.items_cart[i].nameid)
				continue;
			sd->cart_weight += itemdb_weight(sd->cart.u.items_cart[i].nameid) * sd->cart.u.items_cart[i].amount; // Recalculate current cart weight
			sd->cart_num++; // Recalculate current cart item count
		}
	}

	
	// Update the client if the new weight calculations don't match
	if (b_cart_weight_max != sd->cart_weight_max)
		clif_updatestatus(sd, SP_CARTINFO);

	return true;
}

/**
 * Calculates player data from scratch without counting SC adjustments
 * Should be invoked whenever players raise stats, learn passive skills or change equipment
 * @param sd: Player object
 * @param opt: Whether it is first calc (login) or not
 * @return (-1) for too many recursive calls, (1) recursive call, (0) success
 */
int status_calc_pc_sub(struct map_session_data* sd, enum e_status_calc_opt opt)
{
	static int calculating = 0; ///< Check for recursive call preemption. [Skotlex]
	struct status_data *base_status; ///< Pointer to the player's base status
	const struct status_change *sc = &sd->sc;
	struct s_skill b_skill[MAX_SKILL]; ///< Previous skill tree
	int i, skill, refinedef = 0;
	short index = -1;

	if (++calculating > 10) // Too many recursive calls!
		return -1;

	// Remember player-specific values that are currently being shown to the client (for refresh purposes)
	memcpy(b_skill, &sd->status.skill, sizeof(b_skill));

	pc_calc_skilltree(sd);	// SkillTree calculation

	if (opt&SCO_FIRST) {
		// Load Hp/SP from char-received data.
		sd->battle_status.hp = sd->status.hp;
		sd->battle_status.sp = sd->status.sp;
		sd->regen.sregen = &sd->sregen;
		sd->regen.ssregen = &sd->ssregen;
	}

	base_status = &sd->base_status;
	// These are not zeroed. [zzo]
	sd->hprate = 100;
	sd->sprate = 100;
	sd->castrate = 100;
	sd->delayrate = 100;
	sd->dsprate = 100;
	sd->hprecov_rate = 100;
	sd->sprecov_rate = 100;
	sd->matk_rate = 100;
	sd->critical_rate = sd->hit_rate = sd->flee_rate = sd->flee2_rate = 100;
	sd->def_rate = sd->def2_rate = sd->mdef_rate = sd->mdef2_rate = 100;
	sd->regen.state.block = 0;
	sd->add_max_weight = 0;

	sd->indexed_bonus = {};

	memset (&sd->right_weapon.overrefine, 0, sizeof(sd->right_weapon) - sizeof(sd->right_weapon.atkmods));
	memset (&sd->left_weapon.overrefine, 0, sizeof(sd->left_weapon) - sizeof(sd->left_weapon.atkmods));

	if (sd->special_state.intravision)
		clif_status_load(&sd->bl, EFST_CLAIRVOYANCE, 0);

	if (sd->special_state.no_walk_delay)
		clif_status_load(&sd->bl, EFST_ENDURE, 0);

	memset(&sd->special_state,0,sizeof(sd->special_state));

	if (pc_isvip(sd)) // Magic Stone requirement avoidance for VIP.
		sd->special_state.no_gemstone = battle_config.vip_gemstone;

	if (!sd->state.permanent_speed) {
		memset(&base_status->max_hp, 0, sizeof(struct status_data)-(sizeof(base_status->hp)+sizeof(base_status->sp)));
		base_status->speed = DEFAULT_WALK_SPEED;
	} else {
		int pSpeed = base_status->speed;

		memset(&base_status->max_hp, 0, sizeof(struct status_data)-(sizeof(base_status->hp)+sizeof(base_status->sp)));
		base_status->speed = pSpeed;
	}

	// !FIXME: Most of these stuff should be calculated once, but how do I fix the memset above to do that? [Skotlex]
	// Give them all modes except these (useful for clones)
	base_status->mode = static_cast<e_mode>(MD_MASK&~(MD_STATUSIMMUNE|MD_IGNOREMELEE|MD_IGNOREMAGIC|MD_IGNORERANGED|MD_IGNOREMISC|MD_DETECTOR|MD_ANGRY|MD_TARGETWEAK));

	base_status->size = (sd->class_&JOBL_BABY) ? SZ_SMALL : (((sd->class_&MAPID_BASEMASK) == MAPID_SUMMONER) ? battle_config.summoner_size : SZ_MEDIUM);
	if (battle_config.character_size && pc_isriding(sd)) { // [Lupus]
		if (sd->class_&JOBL_BABY) {
			if (battle_config.character_size&SZ_BIG)
				base_status->size++;
		} else
		if(battle_config.character_size&SZ_MEDIUM)
			base_status->size++;
	}
	base_status->aspd_rate = 1000;
	base_status->ele_lv = 1;
	base_status->race = ((sd->class_&MAPID_BASEMASK) == MAPID_SUMMONER) ? battle_config.summoner_race : RC_PLAYER_HUMAN;
	base_status->class_ = CLASS_NORMAL;

	sd->autospell.clear();
	sd->autospell2.clear();
	sd->autospell3.clear();
	sd->addeff.clear();
	sd->addeff_atked.clear();
	sd->addeff_onskill.clear();
	sd->skillatk.clear();
	sd->skillusesprate.clear();
	sd->skillusesp.clear();
	sd->skillheal.clear();
	sd->skillheal2.clear();
	sd->skillblown.clear();
	sd->skillcastrate.clear();
	sd->skillfixcastrate.clear();
	sd->subskill.clear();
	sd->skillcooldown.clear();
	sd->skillfixcast.clear();
	sd->skillvarcast.clear();
	sd->add_def.clear();
	sd->add_mdef.clear();
	sd->add_mdmg.clear();
	sd->reseff.clear();
	sd->itemgrouphealrate.clear();
	sd->add_drop.clear();
	sd->itemhealrate.clear();
	sd->subele2.clear();
	sd->subrace3.clear();
	sd->skilldelay.clear();
	sd->sp_vanish.clear();
	sd->hp_vanish.clear();

	// Zero up structures...
	memset(&sd->hp_loss, 0, sizeof(sd->hp_loss)
		+ sizeof(sd->sp_loss)
		+ sizeof(sd->hp_regen)
		+ sizeof(sd->sp_regen)
		+ sizeof(sd->percent_hp_regen)
		+ sizeof(sd->percent_sp_regen)
		+ sizeof(sd->def_set_race)
		+ sizeof(sd->mdef_set_race)
		+ sizeof(sd->norecover_state_race)
		+ sizeof(sd->hp_vanish_race)
		+ sizeof(sd->sp_vanish_race)
	);

	memset(&sd->bonus, 0, sizeof(sd->bonus));

	// Autobonus
	pc_delautobonus(sd, sd->autobonus, true);
	pc_delautobonus(sd, sd->autobonus2, true);
	pc_delautobonus(sd, sd->autobonus3, true);

	// Parse equipment
	for (i = 0; i < EQI_MAX; i++) {
		current_equip_item_index = index = sd->equip_index[i]; // We pass INDEX to current_equip_item_index - for EQUIP_SCRIPT (new cards solution) [Lupus]
		current_equip_combo_pos = 0;
		if (index < 0)
			continue;
		if (i == EQI_AMMO)
			continue;
		if (pc_is_same_equip_index((enum equip_index)i, sd->equip_index, index))
			continue;
		if (!sd->inventory_data[index])
			continue;

		base_status->def += sd->inventory_data[index]->def;

		// Items may be equipped, their effects however are nullified.
		if (opt&SCO_FIRST && sd->inventory_data[index]->equip_script && (pc_has_permission(sd,PC_PERM_USE_ALL_EQUIPMENT)
			|| !itemdb_isNoEquip(sd->inventory_data[index],sd->bl.m))) { // Execute equip-script on login
			run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
			if (!calculating)
				return 1;
		}

		// Sanitize the refine level in case someone decreased the value inbetween
		if (sd->inventory.u.items_inventory[index].refine > MAX_REFINE)
			sd->inventory.u.items_inventory[index].refine = MAX_REFINE;

		if (sd->inventory_data[index]->type == IT_WEAPON) {
			int r = sd->inventory.u.items_inventory[index].refine, wlv = sd->inventory_data[index]->wlv;
			struct weapon_data *wd;
			struct weapon_atk *wa;

			if(wlv >= REFINE_TYPE_MAX)
				wlv = REFINE_TYPE_MAX - 1;
			if(i == EQI_HAND_L && sd->inventory.u.items_inventory[index].equip == EQP_HAND_L) {
				wd = &sd->left_weapon; // Left-hand weapon
				wa = &base_status->lhw;
			} else {
				wd = &sd->right_weapon;
				wa = &base_status->rhw;
			}
			wa->atk += sd->inventory_data[index]->atk;
			if(r)
				wa->atk2 += refine_info[wlv].bonus[r-1] / 100;
#ifdef RENEWAL
			if (sd->bonus.weapon_atk_rate)
				wa->atk = wa->atk * sd->bonus.weapon_atk_rate / 100;
			wa->matk += sd->inventory_data[index]->matk;
			wa->wlv = wlv;
			if(r && sd->weapontype1 != W_BOW) // Renewal magic attack refine bonus
				wa->matk += refine_info[wlv].bonus[r-1] / 100;
#endif
			if(r) // Overrefine bonus.
				wd->overrefine = refine_info[wlv].randombonus_max[r-1] / 100;
			wa->range += sd->inventory_data[index]->range;
			if(sd->inventory_data[index]->script && (pc_has_permission(sd,PC_PERM_USE_ALL_EQUIPMENT) || !itemdb_isNoEquip(sd->inventory_data[index],sd->bl.m))) {
				if (wd == &sd->left_weapon) {
					sd->state.lr_flag = 1;
					run_script(sd->inventory_data[index]->script,0,sd->bl.id,0);
					sd->state.lr_flag = 0;
				} else
					run_script(sd->inventory_data[index]->script,0,sd->bl.id,0);
				if (!calculating) // Abort, run_script retriggered this. [Skotlex]
					return 1;
			}
#ifdef RENEWAL
			if (sd->bonus.weapon_matk_rate)
				wa->matk += wa->matk * sd->bonus.weapon_matk_rate / 100;
#endif
			if(sd->inventory.u.items_inventory[index].card[0] == CARD0_FORGE) { // Forged weapon
				wd->star += ((sd->inventory.u.items_inventory[index].card[1]>>8)*20); //each star cru, gives +100 damage
				if(wd->star >= 300) wd->star = 500; // 3 Star Crumbs now give +500 dmg
				if(pc_famerank(MakeDWord(sd->inventory.u.items_inventory[index].card[2],sd->inventory.u.items_inventory[index].card[3]) ,MAPID_BLACKSMITH))
					wd->star += 10;
				if (!wa->ele) // Do not overwrite element from previous bonuses.
					wa->ele = (sd->inventory.u.items_inventory[index].card[1]&0x0f);
			}
		} else if(sd->inventory_data[index]->type == IT_ARMOR) {
			int r;

			if ( (r = sd->inventory.u.items_inventory[index].refine) )
				refinedef += refine_info[REFINE_TYPE_ARMOR].bonus[r-1];
			if(sd->inventory_data[index]->script && (pc_has_permission(sd,PC_PERM_USE_ALL_EQUIPMENT) || !itemdb_isNoEquip(sd->inventory_data[index],sd->bl.m))) {
				if( i == EQI_HAND_L ) // Shield
					sd->state.lr_flag = 3;
				run_script(sd->inventory_data[index]->script,0,sd->bl.id,0);
				if( i == EQI_HAND_L ) // Shield
					sd->state.lr_flag = 0;
				if (!calculating) // Abort, run_script retriggered this. [Skotlex]
					return 1;
			}
		} else if( sd->inventory_data[index]->type == IT_SHADOWGEAR ) { // Shadow System
			if (sd->inventory_data[index]->script && (pc_has_permission(sd,PC_PERM_USE_ALL_EQUIPMENT) || !itemdb_isNoEquip(sd->inventory_data[index],sd->bl.m))) {
				run_script(sd->inventory_data[index]->script,0,sd->bl.id,0);
				if( !calculating )
					return 1;
			}
		}
	}

	if(sd->equip_index[EQI_AMMO] >= 0) {
		index = sd->equip_index[EQI_AMMO];
		if(sd->inventory_data[index]) { // Arrows
			sd->bonus.arrow_atk += sd->inventory_data[index]->atk;
			sd->state.lr_flag = 2;
			if( !itemdb_group_item_exists(IG_THROWABLE, sd->inventory_data[index]->nameid) ) // Don't run scripts on throwable items
				run_script(sd->inventory_data[index]->script,0,sd->bl.id,0);
			sd->state.lr_flag = 0;
			if (!calculating) // Abort, run_script retriggered status_calc_pc. [Skotlex]
				return 1;
		}
	}

	// Process and check item combos
	if (!sd->combos.empty()) {
		for (const auto &combo : sd->combos) {
			s_item_combo *item_combo;

			current_equip_item_index = -1;
			current_equip_combo_pos = combo->pos;

			if (combo->bonus == nullptr || !(item_combo = itemdb_combo_exists(combo->id)))
				continue;

			bool no_run = false;
			size_t j = 0;

			// Check combo items
			while (j < item_combo->nameid.size()) {
				item_data *id = itemdb_exists(item_combo->nameid[j]);

				// Don't run the script if at least one of combo's pair has restriction
				if (id && !pc_has_permission(sd, PC_PERM_USE_ALL_EQUIPMENT) && itemdb_isNoEquip(id, sd->bl.m)) {
					no_run = true;
					break;
				}

				j++;
			}

			if (no_run)
				continue;

			run_script(combo->bonus, 0, sd->bl.id, 0);

			if (!calculating) // Abort, run_script retriggered this
				return 1;
		}
	}

	// Store equipment script bonuses
	memcpy(sd->indexed_bonus.param_equip,sd->indexed_bonus.param_bonus,sizeof(sd->indexed_bonus.param_equip));
	memset(sd->indexed_bonus.param_bonus, 0, sizeof(sd->indexed_bonus.param_bonus));

	base_status->def += (refinedef+50)/100;

	// Parse Cards
	for (i = 0; i < EQI_MAX; i++) {
		current_equip_item_index = index = sd->equip_index[i]; // We pass INDEX to current_equip_item_index - for EQUIP_SCRIPT (new cards solution) [Lupus]
		current_equip_combo_pos = 0;
		if (index < 0)
			continue;
		if (i == EQI_AMMO)
			continue;
		if (pc_is_same_equip_index((enum equip_index)i, sd->equip_index, index))
			continue;

		if (sd->inventory_data[index]) {
			int j;
			struct item_data *data;

			// Card script execution.
			if (itemdb_isspecial(sd->inventory.u.items_inventory[index].card[0]))
				continue;
			for (j = 0; j < MAX_SLOTS; j++) { // Uses MAX_SLOTS to support Soul Bound system [Inkfish]
				int c = sd->inventory.u.items_inventory[index].card[j];
				current_equip_card_id= c;
				if(!c)
					continue;
				data = itemdb_exists(c);
				if(!data)
					continue;
				if (opt&SCO_FIRST && data->equip_script && (pc_has_permission(sd,PC_PERM_USE_ALL_EQUIPMENT) || !itemdb_isNoEquip(data,sd->bl.m))) {// Execute equip-script on login
					run_script(data->equip_script,0,sd->bl.id,0);
					if (!calculating)
						return 1;
				}
				if(!data->script)
					continue;
				if(!pc_has_permission(sd,PC_PERM_USE_ALL_EQUIPMENT) && itemdb_isNoEquip(data,sd->bl.m)) // Card restriction checks.
					continue;
				if(i == EQI_HAND_L && sd->inventory.u.items_inventory[index].equip == EQP_HAND_L) { // Left hand status.
					sd->state.lr_flag = 1;
					run_script(data->script,0,sd->bl.id,0);
					sd->state.lr_flag = 0;
				} else
					run_script(data->script,0,sd->bl.id,0);
				if (!calculating) // Abort, run_script his function. [Skotlex]
					return 1;
			}
		}
	}
	current_equip_card_id = 0; // Clear stored card ID [Secret]

	// Parse random options
	for (i = 0; i < EQI_MAX; i++) {
		current_equip_item_index = index = sd->equip_index[i];
		current_equip_combo_pos = 0;
		current_equip_opt_index = -1;

		if (index < 0)
			continue;
		if (i == EQI_AMMO)
			continue;
		if (pc_is_same_equip_index((enum equip_index)i, sd->equip_index, index))
			continue;
		
		if (sd->inventory_data[index]) {
			for (uint8 j = 0; j < MAX_ITEM_RDM_OPT; j++) {
				short opt_id = sd->inventory.u.items_inventory[index].option[j].id;

				if (!opt_id)
					continue;
				current_equip_opt_index = j;

				std::shared_ptr<s_random_opt_data> data = random_option_db.find(opt_id);

				if (!data || !data->script)
					continue;
				if (!pc_has_permission(sd, PC_PERM_USE_ALL_EQUIPMENT) && itemdb_isNoEquip(sd->inventory_data[index], sd->bl.m))
					continue;
				if (i == EQI_HAND_L && sd->inventory.u.items_inventory[index].equip == EQP_HAND_L) { // Left hand status.
					sd->state.lr_flag = 1;
					run_script(data->script, 0, sd->bl.id, 0);
					sd->state.lr_flag = 0;
				}
				else
					run_script(data->script, 0, sd->bl.id, 0);
				if (!calculating)
					return 1;
			}
		}
		current_equip_opt_index = -1;
	}

	

	pc_bonus_script(sd);

	if( sd->pd ) { // Pet Bonus
		struct pet_data *pd = sd->pd;
		std::shared_ptr<s_pet_db> pet_db_ptr = pd->get_pet_db();

		if (pet_db_ptr != nullptr && pet_db_ptr->pet_bonus_script)
			run_script(pet_db_ptr->pet_bonus_script,0,sd->bl.id,0);
		if (pet_db_ptr != nullptr && pd->pet.intimate > 0 && (!battle_config.pet_equip_required || pd->pet.equip > 0) && pd->state.skillbonus == 1 && pd->bonus)
			pc_bonus(sd,pd->bonus->type, pd->bonus->val);
	}

	// param_bonus now holds card bonuses.
	if(base_status->rhw.range < 1) base_status->rhw.range = 1;
	if(base_status->lhw.range < 1) base_status->lhw.range = 1;
	if(base_status->rhw.range < base_status->lhw.range)
		base_status->rhw.range = base_status->lhw.range;

	sd->bonus.double_rate += sd->bonus.double_add_rate;
	sd->bonus.perfect_hit += sd->bonus.perfect_hit_add;
	sd->bonus.splash_range += sd->bonus.splash_add_range;

	// Damage modifiers from weapon type
	std::shared_ptr<s_sizefix_db> right_weapon = size_fix_db.find(sd->weapontype1);
	std::shared_ptr<s_sizefix_db> left_weapon = size_fix_db.find(sd->weapontype2);

	sd->right_weapon.atkmods[SZ_SMALL] = right_weapon->small;
	sd->right_weapon.atkmods[SZ_MEDIUM] = right_weapon->medium;
	sd->right_weapon.atkmods[SZ_BIG] = right_weapon->large;
	sd->left_weapon.atkmods[SZ_SMALL] = left_weapon->small;
	sd->left_weapon.atkmods[SZ_MEDIUM] = left_weapon->medium;
	sd->left_weapon.atkmods[SZ_BIG] = left_weapon->large;

	if((pc_isriding(sd) || pc_isridingdragon(sd)) &&
		(sd->status.weapon==W_1HSPEAR || sd->status.weapon==W_2HSPEAR))
	{	// When Riding with spear, damage modifier to mid-class becomes
		// same as versus large size.
		sd->right_weapon.atkmods[SZ_MEDIUM] = sd->right_weapon.atkmods[SZ_BIG];
		sd->left_weapon.atkmods[SZ_MEDIUM] = sd->left_weapon.atkmods[SZ_BIG];
	}

// ----- STATS CALCULATION -----

	// Job bonuses
	index = pc_class2idx(sd->status.class_);
	for(i=0;i<(int)sd->status.job_level && i<MAX_LEVEL;i++) {
		if(!job_info[index].job_bonus[i])
			continue;
		switch(job_info[index].job_bonus[i]) {
			case 1: base_status->str++; break;
			case 2: base_status->agi++; break;
			case 3: base_status->vit++; break;
			case 4: base_status->int_++; break;
			case 5: base_status->dex++; break;
			case 6: base_status->luk++; break;
		}
	}

	// If a Super Novice has never died and is at least joblv 70, he gets all stats +10
	if(((sd->class_&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE && (sd->status.job_level >= 70  || sd->class_&JOBL_THIRD)) && sd->die_counter == 0) {
		base_status->str += 10;
		base_status->agi += 10;
		base_status->vit += 10;
		base_status->int_+= 10;
		base_status->dex += 10;
		base_status->luk += 10;
	}

	// Absolute modifiers from passive skills

	if((skill=pc_checkskill(sd,SK_AC_OWL))>0)
		base_status->int_ += skill * 2;
	if((skill=pc_checkskill(sd,SK_MG_CASTCANCEL))>0)
		base_status->int_ += skill * 2;
	if((skill=pc_checkskill(sd,SK_AC_HAWK))>0)
		base_status->dex += skill * 2;
	if((skill=pc_checkskill(sd,SK_RG_VULTURE))>0)
		base_status->dex += skill * 2;
	if((skill=pc_checkskill(sd,SK_AC_AGI))>0)
		base_status->agi += skill * 2;
	if((skill=pc_checkskill(sd,SK_MC_VENDING))>0){
		base_status->agi += skill;
		base_status->int_ += skill;
		base_status->dex += skill;
		base_status->vit += skill;
		base_status->str += skill;
		base_status->luk += skill;
	}
		

	
	if((skill=pc_checkskill(sd,SK_AM_PLANTCULTIVATION))>0){
		base_status->int_ += skill * 2;
	}

	if((skill=pc_checkskill(sd,SK_AC_MAKINGARROW))>0){
		base_status->matk_max += skill * 5;
		base_status->matk_min += skill * 5;
		base_status->rhw.matk += skill * 5;
	}
	

	// Bonuses from cards and equipment as well as base stat, remember to avoid overflows.
	i = base_status->str + sd->status.str + sd->indexed_bonus.param_bonus[0] + sd->indexed_bonus.param_equip[0];
	base_status->str = cap_value(i,0,USHRT_MAX);
	i = base_status->agi + sd->status.agi + sd->indexed_bonus.param_bonus[1] + sd->indexed_bonus.param_equip[1];
	base_status->agi = cap_value(i,0,USHRT_MAX);
	i = base_status->vit + sd->status.vit + sd->indexed_bonus.param_bonus[2] + sd->indexed_bonus.param_equip[2];
	base_status->vit = cap_value(i,0,USHRT_MAX);
	i = base_status->int_+ sd->status.int_+ sd->indexed_bonus.param_bonus[3] + sd->indexed_bonus.param_equip[3];
	base_status->int_ = cap_value(i,0,USHRT_MAX);
	i = base_status->dex + sd->status.dex + sd->indexed_bonus.param_bonus[4] + sd->indexed_bonus.param_equip[4];
	base_status->dex = cap_value(i,0,USHRT_MAX);
	i = base_status->luk + sd->status.luk + sd->indexed_bonus.param_bonus[5] + sd->indexed_bonus.param_equip[5];
	base_status->luk = cap_value(i,0,USHRT_MAX);

	if (sd->special_state.no_walk_delay) {
		if (sc->data[STATUS_ENDURE]) {
			if (sc->data[STATUS_ENDURE]->val4)
				sc->data[STATUS_ENDURE]->val4 = 0;
			status_change_end(&sd->bl, STATUS_ENDURE, INVALID_TIMER);
		}
		clif_status_load(&sd->bl, EFST_ENDURE, 1);
		base_status->mdef++;
	}

// ------ ATTACK CALCULATION ------

	// Base batk value is set in status_calc_misc
#ifndef RENEWAL
	// !FIXME: Weapon-type bonus (Why is the weapon_atk bonus applied to base attack?)
	if (sd->status.weapon < MAX_WEAPON_TYPE && sd->indexed_bonus.weapon_atk[sd->status.weapon])
		base_status->batk += sd->indexed_bonus.weapon_atk[sd->status.weapon];
	// Absolute modifiers from passive skills
	if((skill=pc_checkskill(sd,BS_HILTBINDING))>0)
		base_status->batk += 4;
#else
	base_status->watk = status_weapon_atk(base_status->rhw);
	base_status->watk2 = status_weapon_atk(base_status->lhw);
	base_status->eatk = sd->bonus.eatk;
#endif

// ----- HP MAX CALCULATION -----
	base_status->max_hp = sd->status.max_hp = status_calc_maxhpsp_pc(sd,base_status->vit,true);

	if(battle_config.hp_rate != 100)
		base_status->max_hp = (unsigned int)(battle_config.hp_rate * (base_status->max_hp/100.));

	if (sd->status.base_level < 100)
		base_status->max_hp = cap_value(base_status->max_hp,1,(unsigned int)battle_config.max_hp_lv99);
	else if (sd->status.base_level < 151)
		base_status->max_hp = cap_value(base_status->max_hp,1,(unsigned int)battle_config.max_hp_lv150);
	else
		base_status->max_hp = cap_value(base_status->max_hp,1,(unsigned int)battle_config.max_hp);

// ----- SP MAX CALCULATION -----
	base_status->max_sp = sd->status.max_sp = status_calc_maxhpsp_pc(sd,base_status->int_,false);

	if(battle_config.sp_rate != 100)
		base_status->max_sp = (unsigned int)(battle_config.sp_rate * (base_status->max_sp/100.));

	base_status->max_sp = cap_value(base_status->max_sp,1,(unsigned int)battle_config.max_sp);

// ----- RESPAWN HP/SP -----

	// Calc respawn hp and store it on base_status
	if (sd->special_state.restart_full_recover) {
		base_status->hp = base_status->max_hp;
		base_status->sp = base_status->max_sp;
	} else {
		if((sd->class_&MAPID_BASEMASK) == MAPID_NOVICE && !(sd->class_&JOBL_2)
			&& battle_config.restart_hp_rate < 50)
			base_status->hp = base_status->max_hp>>1;
		else
			base_status->hp = (int64)base_status->max_hp * battle_config.restart_hp_rate/100;
		if(!base_status->hp)
			base_status->hp = 1;

		base_status->sp = (int64)base_status->max_sp * battle_config.restart_sp_rate /100;

		if( !base_status->sp ) // The minimum for the respawn setting is SP:1
			base_status->sp = 1;
	}

// ----- MISC CALCULATION -----
	status_calc_misc(&sd->bl, base_status, sd->status.base_level);

	// Equipment modifiers for misc settings
	if(sd->matk_rate < 0)
		sd->matk_rate = 0;

	if(sd->matk_rate != 100) {
		base_status->matk_max = base_status->matk_max * sd->matk_rate/100;
		base_status->matk_min = base_status->matk_min * sd->matk_rate/100;
	}

	if(sd->hit_rate < 0)
		sd->hit_rate = 0;
	if(sd->hit_rate != 100)
		base_status->hit = base_status->hit * sd->hit_rate/100;

	if(sd->flee_rate < 0)
		sd->flee_rate = 0;
	if(sd->flee_rate != 100)
		base_status->flee = base_status->flee * sd->flee_rate/100;

	if(sd->def2_rate < 0)
		sd->def2_rate = 0;
	if(sd->def2_rate != 100)
		base_status->def2 = base_status->def2 * sd->def2_rate/100;

	if(sd->mdef2_rate < 0)
		sd->mdef2_rate = 0;
	if(sd->mdef2_rate != 100)
		base_status->mdef2 = base_status->mdef2 * sd->mdef2_rate/100;

	if(sd->critical_rate < 0)
		sd->critical_rate = 0;
	if(sd->critical_rate != 100)
		base_status->cri = cap_value(base_status->cri * sd->critical_rate/100,SHRT_MIN,SHRT_MAX);

	if(sd->flee2_rate < 0)
		sd->flee2_rate = 0;
	if(sd->flee2_rate != 100)
		base_status->flee2 = base_status->flee2 * sd->flee2_rate/100;

// ----- HIT CALCULATION -----

	// Absolute modifiers from passive skills

	if((skill=pc_checkskill(sd,SK_AC_VULTURE))>0) {
		if(sd->status.weapon == W_BOW)
			base_status->rhw.range += skill*2;
	}
	if((skill=pc_checkskill(sd,SK_RG_VULTURE))>0) {
		if(sd->status.weapon == W_BOW)
			base_status->rhw.range += skill*2;
	}
	
	
	

	
// ----- FLEE CALCULATION -----

	// Absolute modifiers from passive skills
	

// ----- CRITICAL CALCULATION -----

#ifdef RENEWAL
	
	if ((skill = pc_checkskill(sd, SK_AL_MACEMASTERY)) > 0 && (sd->status.weapon == W_MACE || sd->status.weapon == W_2HMACE))
		base_status->cri += skill * 10;
#endif

// ----- EQUIPMENT-DEF CALCULATION -----

	// Apply relative modifiers from equipment
	if(sd->def_rate < 0)
		sd->def_rate = 0;
	if(sd->def_rate != 100) {
		i = base_status->def * sd->def_rate/100;
		base_status->def = cap_value(i, DEFTYPE_MIN, DEFTYPE_MAX);
	}

	


// ----- EQUIPMENT-MDEF CALCULATION -----

	// Apply relative modifiers from equipment
	if(sd->mdef_rate < 0)
		sd->mdef_rate = 0;
	if(sd->mdef_rate != 100) {
		i =  base_status->mdef * sd->mdef_rate/100;
		base_status->mdef = cap_value(i, DEFTYPE_MIN, DEFTYPE_MAX);
	}



// ----- ASPD CALCULATION -----

	/// Unlike other stats, ASPD rate modifiers from skills/SCs/items/etc are first all added together, then the final modifier is applied

	// Basic ASPD value
	i = status_base_amotion_pc(sd,base_status);
	base_status->amotion = cap_value(i,pc_maxaspd(sd),2000);

	// Relative modifiers from passive skills
	
	if((skill=pc_checkskill(sd,SK_EX_SONICACCELERATION))>0){
		int right_hand_index = sd->equip_index[EQI_HAND_R];
		int left_hand_index = sd->equip_index[EQI_HAND_L];
		if(sd 
			&& right_hand_index >= 0 
			&& left_hand_index >= 0
			&& sd->status.weapon != W_KATAR
			&& sd->inventory_data[right_hand_index]->type == IT_WEAPON
			&& sd->inventory_data[left_hand_index]->type == IT_WEAPON
			){
			base_status->aspd_rate += 20*skill;
		}		
	}

	if((skill=pc_checkskill(sd,SK_RG_AMBIDEXTERITY))>0){
		base_status->aspd_rate += 5*skill;
	}

	base_status->adelay = 2*base_status->amotion;


// ----- DMOTION -----

	i =  800-base_status->agi*4;
	base_status->dmotion = cap_value(i, 400, 800);
	if(battle_config.pc_damage_delay_rate != 100)
		base_status->dmotion = base_status->dmotion*battle_config.pc_damage_delay_rate/100;

// ----- MISC CALCULATIONS -----

	// Weight
	status_calc_weight(sd, CALCWT_MAXBONUS);
	status_calc_cart_weight(sd, CALCWT_MAXBONUS);

	
	sd->regen.state.walk = 0;


	// Underflow protections.
	if(sd->dsprate < 0)
		sd->dsprate = 0;
	if(sd->castrate < 0)
		sd->castrate = 0;
	if(sd->delayrate < 0)
		sd->delayrate = 0;
	if(sd->hprecov_rate < 0)
		sd->hprecov_rate = 0;
	if(sd->sprecov_rate < 0)
		sd->sprecov_rate = 0;

	// Anti-element and anti-race
	if((skill=pc_checkskill(sd,SK_SM_INNER_STRENGTH))>0)
		sd->indexed_bonus.subele[ELE_HOLY] += skill*10;
	if((skill=pc_checkskill(sd,SK_BS_SKINTEMPERING))>0) {
		sd->indexed_bonus.subele[ELE_NEUTRAL] += skill*4;
		sd->indexed_bonus.subele[ELE_FIRE] += skill*8;
	}
	
	

	if(sc->count) {
		if(sc->data[STATUS_IMPROVECONCENTRATION]) { // Update the card-bonus data
			sc->data[STATUS_IMPROVECONCENTRATION]->val3 = sd->indexed_bonus.param_bonus[1]; // Agi
			sc->data[STATUS_IMPROVECONCENTRATION]->val4 = sd->indexed_bonus.param_bonus[4]; // Dex
		}

		
	}
	status_cpy(&sd->battle_status, base_status);

// ----- CLIENT-SIDE REFRESH -----
	if(!sd->bl.prev) {
		// Will update on LoadEndAck
		calculating = 0;
		return 0;
	}
	if(memcmp(b_skill,sd->status.skill,sizeof(sd->status.skill)))
		clif_skillinfoblock(sd);

	// If the skill is learned, the status is infinite.
	
	calculating = 0;

	return 0;
}

/// Intermediate function since C++ does not have a try-finally syntax
int status_calc_pc_( struct map_session_data* sd, enum e_status_calc_opt opt ){
	// Save the old script the player was attached to
	struct script_state* previous_st = sd->st;

	// Store the return value of the original function
	int ret = status_calc_pc_sub( sd, opt );

	// If an old script is present
	if( previous_st ){
		// Reattach the player to it, so that the limitations of that script kick back in
		script_attach_state( previous_st );
	}

	// Return the original return value
	return ret;
}

/**
 * Calculates Mercenary data
 * @param md: Mercenary object
 * @param opt: Whether it is first calc or not (0 on level up or status)
 * @return 0
 */
int status_calc_mercenary_(struct mercenary_data *md, enum e_status_calc_opt opt)
{
	
	return 0;
}

/**
 * Calculates Homunculus data
 * @param hd: Homunculus object
 * @param opt: Whether it is first calc or not (0 on level up or status)
 * @return 1
 */
int status_calc_homunculus_(struct homun_data *hd, enum e_status_calc_opt opt)
{
	return 1;
}

/**
 * Calculates Elemental data
 * @param ed: Elemental object
 * @param opt: Whether it is first calc or not (0 on status change)
 * @return 0
 */
int status_calc_elemental_(struct elemental_data *ed, enum e_status_calc_opt opt)
{
	struct status_data *status = &ed->base_status;
	struct s_elemental *ele = &ed->elemental;
	struct map_session_data *sd = ed->master;

	if( !sd )
		return 0;

	if (opt&SCO_FIRST) {
		memcpy(status, &ed->db->status, sizeof(struct status_data));
		if( !ele->mode )
			status->mode = EL_MODE_PASSIVE;
		else
			status->mode = ele->mode;

		status->class_ = CLASS_NORMAL;
		status_calc_misc(&ed->bl, status, 0);

		status->max_hp = ele->max_hp;
		status->max_sp = ele->max_sp;
		status->hp = ele->hp;
		status->sp = ele->sp;
		status->rhw.atk = ele->atk;
		status->rhw.atk2 = ele->atk2;
		status->matk_min += ele->matk;
		status->def += ele->def;
		status->mdef += ele->mdef;
		status->flee = ele->flee;
		status->hit = ele->hit;

		if (ed->master)
			status->speed = status_get_speed(&ed->master->bl);

		memcpy(&ed->battle_status,status,sizeof(struct status_data));
	} else {
		status_calc_misc(&ed->bl, status, 0);
		status_cpy(&ed->battle_status, status);
	}

	return 0;
}

/**
 * Calculates NPC data
 * @param nd: NPC object
 * @param opt: Whether it is first calc or not (what?)
 * @return 0
 */
int status_calc_npc_(struct npc_data *nd, enum e_status_calc_opt opt)
{
	struct status_data *status = &nd->status;

	if (!nd)
		return 0;

	if (opt&SCO_FIRST) {
		status->hp = 1;
		status->sp = 1;
		status->max_hp = 1;
		status->max_sp = 1;

		status->def_ele = ELE_NEUTRAL;
		status->ele_lv = 1;
		status->race = RC_DEMIHUMAN;
		status->class_ = CLASS_NORMAL;
		status->size = nd->size;
		status->rhw.range = 1 + status->size;
		status->mode = static_cast<e_mode>(MD_CANMOVE|MD_CANATTACK);
		status->speed = nd->speed;
	}

	status->str = nd->stat_point + nd->params.str;
	status->agi = nd->stat_point + nd->params.agi;
	status->vit = nd->stat_point + nd->params.vit;
	status->int_= nd->stat_point + nd->params.int_;
	status->dex = nd->stat_point + nd->params.dex;
	status->luk = nd->stat_point + nd->params.luk;

	status_calc_misc(&nd->bl, status, nd->level);
	status_cpy(&nd->status, status);

	return 0;
}

/**
 * Calculates regeneration values
 * Applies passive skill regeneration additions
 * @param bl: Object to calculate regen for [PC|HOM|MER|ELEM]
 * @param status: Object's status
 * @param regen: Object's base regeneration data
 */
void status_calc_regen(struct block_list *bl, struct status_data *status, struct regen_data *regen)
{
	struct map_session_data *sd;
	struct status_change *sc;
	int val, skill, reg_flag;

	if( !(bl->type&BL_REGEN) || !regen )
		return;

	sd = BL_CAST(BL_PC,bl);
	sc = status_get_sc(bl);

	val = 1 + (status->vit/5) + (status->max_hp/200);

	if( sd && sd->hprecov_rate != 100 )
		val = val*sd->hprecov_rate/100;

	reg_flag = bl->type == BL_PC ? 0 : 1;

	regen->hp = cap_value(val, reg_flag, SHRT_MAX);

	val = 1 + (status->int_/6) + (status->max_sp/100);
	if( status->int_ >= 120 )
		val += ((status->int_-120)>>1) + 4;

	if( sd && sd->sprecov_rate != 100 )
		val = val*sd->sprecov_rate/100;

	regen->sp = cap_value(val, reg_flag, SHRT_MAX);

	if( sd ) {
		struct regen_data_sub *sregen;
		// Only players have skill/sitting skill regen for now.
		sregen = regen->sregen;

		val = 0;
		if( (skill=pc_checkskill(sd,SK_SM_RECOVERY)) > 0 )
			val += skill*10 + skill*status->max_hp/500;

		

		sregen->hp = cap_value(val, 0, SHRT_MAX);

		val = 0;
		if( (skill=(pc_checkskill(sd,SK_SM_SRECOVERY))) > 0 )
			val += skill*6;
		if( (skill=(pc_checkskill(sd,SK_MG_SRECOVERY))) > 0 )
			val += skill*6;
		

		sregen->sp = cap_value(val, 0, SHRT_MAX);

		// Skill-related recovery (only when sit)
		sregen = regen->ssregen;

		val = 0;
		

		sregen->hp = cap_value(val, 0, SHRT_MAX);

		val = 0;
		
		sregen->sp = cap_value(val, 0, SHRT_MAX);
	}

	if( bl->type == BL_HOM ) {
		struct homun_data *hd = (TBL_HOM*)bl;
	
	} else if( bl->type == BL_MER ) {
		val = (status->max_hp * status->vit / 10000 + 1) * 6;
		regen->hp = cap_value(val, 1, SHRT_MAX);

		val = (status->max_sp * (status->int_ + 10) / 750) + 1;
		regen->sp = cap_value(val, 1, SHRT_MAX);
	} else if( bl->type == BL_ELEM ) {
		val = (status->max_hp * status->vit / 10000 + 1) * 6;
		regen->hp = cap_value(val, 1, SHRT_MAX);

		val = (status->max_sp * (status->int_ + 10) / 750) + 1;
		regen->sp = cap_value(val, 1, SHRT_MAX);
	}
}

/**
 * Calculates SC (Status Changes) regeneration values
 * @param bl: Object to calculate regen for [PC|HOM|MER|ELEM]
 * @param regen: Object's base regeneration data
 * @param sc: Object's status change data
 */
void status_calc_regen_rate(struct block_list *bl, struct regen_data *regen, struct status_change *sc)
{
	if (!(bl->type&BL_REGEN) || !regen)
		return;

	regen->flag = RGN_HP|RGN_SP;
	if(regen->sregen) {
		if (regen->sregen->hp)
			regen->flag |= RGN_SHP;

		if (regen->sregen->sp)
			regen->flag |= RGN_SSP;
		regen->sregen->rate.hp = regen->sregen->rate.sp = 100;
	}
	if (regen->ssregen) {
		if (regen->ssregen->hp)
			regen->flag |= RGN_SHP;

		if (regen->ssregen->sp)
			regen->flag |= RGN_SSP;
		regen->ssregen->rate.hp = regen->ssregen->rate.sp = 100;
	}
	regen->rate.hp = regen->rate.sp = 100;

	if (!sc || !sc->count)
		return;

	// No HP or SP regen
	if ((sc->data[STATUS_POISON] )
		|| (sc->data[STATUS_DPOISON])
		|| sc->data[STATUS_TRICKDEAD]
		|| sc->data[STATUS_BLEEDING])

		regen->flag = RGN_NONE;

	// No natural SP regen
	if (sc->data[STATUS_MAXIMIZEPOWER])
		regen->flag &= ~RGN_SP;

	

	if (sc->data[STATUS_MAGNIFICAT])
		regen->rate.sp += 100;




	if (bl->type == BL_ELEM) { // Recovery bonus only applies to the Elementals.
		int ele_class = status_get_class(bl);

		switch (ele_class) {
		case ELEMENTALID_AGNI_S:
		case PETID_MANDRAKE:
		case PETID_AGNI:

			break;
		case ELEMENTALID_AQUA_S:
		case PETID_PHOENIX:
		case PETID_AQUA:

			break;
		case ELEMENTALID_VENTUS_S:
		case PETID_BASILISK:
		case PETID_VENTUS:

			break;
		case ELEMENTALID_TERA_S:
		case PETID_BEHOLDER:
		case PETID_TERA:
			
			break;
		}
	}



}

/**
 * Applies a state to a unit - See [StatusChangeStateTable]
 * @param bl: Object to change state on [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change data
 * @param flag: Which state to apply to bl
 * @param start: (1) start state, (0) remove state
 */
void status_calc_state( struct block_list *bl, struct status_change *sc, enum scs_flag flag, bool start )
{

	/// No sc at all, we can zero without any extra weight over our conciousness
	if( !sc->count ) {
		memset(&sc->cant, 0, sizeof (sc->cant));
		return;
	}

	// Can't move
	if( flag&SCS_NOMOVE ) {
		if( !(flag&SCS_NOMOVECOND) )
			sc->cant.move += ( start ? 1 : ((sc->cant.move)? -1:0) );
		else if(
				     (sc->data[STATUS_GOSPEL] && sc->data[STATUS_GOSPEL]->val4 == BCT_SELF)	// cannot move while gospel is in effect
#ifndef RENEWAL
				  || (sc->data[SC_BASILICA] && sc->data[SC_BASILICA]->val4 == bl->id) // Basilica caster cannot move
				  || (sc->data[SC_GRAVITATION] && sc->data[SC_GRAVITATION]->val3 == BCT_SELF)
#endif
				  || (sc->data[STATUS_CAMOUFLAGE] && sc->data[STATUS_CAMOUFLAGE]->val1 < 3)
				)
			sc->cant.move += ( start ? 1 : ((sc->cant.move)? -1:0) );
	}

	// Can't use skills
	if( flag&SCS_NOCAST ) {
		if( !(flag&SCS_NOCASTCOND) )
			sc->cant.cast += ( start ? 1 : -1 );
		
	}

	// Can't chat
	if( flag&SCS_NOCHAT ) {
		if( !(flag&SCS_NOCHATCOND) )
			sc->cant.chat += ( start ? 1 : -1 );
		else if(sc->data[STATUS_NOCHAT] && sc->data[STATUS_NOCHAT]->val1&MANNER_NOCHAT)
			sc->cant.chat += ( start ? 1 : -1 );
	}

	// Player-only states
	if( bl->type == BL_PC ) {
		// Can't pick-up items
		if( flag&SCS_NOPICKITEM ) {
			if( !(flag&SCS_NOPICKITEMCOND) )
				sc->cant.pickup += ( start ? 1 : -1 );
			else if( (sc->data[STATUS_NOCHAT] && sc->data[STATUS_NOCHAT]->val1&MANNER_NOITEM) )
				sc->cant.pickup += ( start ? 1 : -1 );
		}

		// Can't drop items
		if( flag&SCS_NODROPITEM ) {
			if( !(flag&SCS_NODROPITEMCOND) )
				sc->cant.drop += ( start ? 1 : -1 );
			else if( (sc->data[STATUS_NOCHAT] && sc->data[STATUS_NOCHAT]->val1&MANNER_NOITEM) )
				sc->cant.drop += ( start ? 1 : -1 );
		}
	}

	return;
}

/**
 * Recalculates parts of an objects status according to specified flags
 * See [set_sc] [add_sc]
 * @param bl: Object whose status has changed [PC|MOB|HOM|MER|ELEM]
 * @param flag: Which status has changed on bl
 */
void status_calc_bl_main(struct block_list *bl, /*enum scb_flag*/int flag)
{
	const struct status_data *b_status = status_get_base_status(bl); // Base Status
	struct status_data *status = status_get_status_data(bl); // Battle Status
	struct status_change *sc = status_get_sc(bl);
	TBL_PC *sd = BL_CAST(BL_PC,bl);
	int temp;

	if (!b_status || !status)
		return;

	/** [Playtester]
	* This needs to be done even if there is currently no status change active, because
	* we need to update the speed on the client when the last status change ends.
	**/
	if(flag&SCB_SPEED) {
		struct unit_data *ud = unit_bl2ud(bl);
		/** [Skotlex]
		* Re-walk to adjust speed (we do not check if walktimer != INVALID_TIMER
		* because if you step on something while walking, the moment this
		* piece of code triggers the walk-timer is set on INVALID_TIMER)
		**/
		if (ud)
			ud->state.change_walk_target = ud->state.speed_changed = 1;
	}

	if(flag&SCB_STR) {
		status->str = status_calc_str(bl, sc, b_status->str);
		flag|=SCB_BATK;
		if( bl->type&BL_HOM )
			flag |= SCB_WATK;
	}

	if(flag&SCB_AGI) {
		status->agi = status_calc_agi(bl, sc, b_status->agi);
		flag|=SCB_FLEE
#ifdef RENEWAL
			|SCB_DEF2
#endif
			;
		if( bl->type&(BL_PC|BL_HOM) )
			flag |= SCB_ASPD|SCB_DSPD;
	}

	if(flag&SCB_VIT) {
		status->vit = status_calc_vit(bl, sc, b_status->vit);
		flag|=SCB_DEF2|SCB_MDEF2;
		if( bl->type&(BL_PC|BL_HOM|BL_MER|BL_ELEM) )
			flag |= SCB_MAXHP;
		if( bl->type&BL_HOM )
			flag |= SCB_DEF;
	}

	if(flag&SCB_INT) {
		status->int_ = status_calc_int(bl, sc, b_status->int_);
		flag|=SCB_MATK|SCB_MDEF2;
		if( bl->type&(BL_PC|BL_HOM|BL_MER|BL_ELEM) )
			flag |= SCB_MAXSP;
		if( bl->type&BL_HOM )
			flag |= SCB_MDEF;
	}

	if(flag&SCB_DEX) {
		status->dex = status_calc_dex(bl, sc, b_status->dex);
		flag|=SCB_BATK|SCB_HIT
#ifdef RENEWAL
			|SCB_MATK|SCB_MDEF2
#endif
			;
		if( bl->type&(BL_PC|BL_HOM) )
			flag |= SCB_ASPD;
		if( bl->type&BL_HOM )
			flag |= SCB_WATK;
	}

	if(flag&SCB_LUK) {
		status->luk = status_calc_luk(bl, sc, b_status->luk);
		flag|=SCB_BATK|SCB_CRI|SCB_FLEE2
#ifdef RENEWAL
			|SCB_MATK|SCB_HIT|SCB_FLEE
#endif
			;
	}

	if(flag&SCB_BATK && b_status->batk) {
		int lv = status_get_lv(bl);
		status->batk = status_base_atk(bl, status, lv);
		temp = b_status->batk - status_base_atk(bl, b_status, lv);
		if (temp) {
			temp += status->batk;
			status->batk = cap_value(temp, 0, USHRT_MAX);
		}
		status->batk = status_calc_batk(bl, sc, status->batk);
	}

	if(flag&SCB_WATK) {

		if(!b_status->watk) { // We only have left-hand weapon
			status->watk = 0;
			status->watk2 = status_calc_watk(bl, sc, b_status->watk2);
		}
		else status->watk = status_calc_watk(bl, sc, b_status->watk);
	}

	if(flag&SCB_HIT) {
		if (status->dex == b_status->dex

			&& status->luk == b_status->luk

			)
			status->hit = status_calc_hit(bl, sc, b_status->hit);
		else
			status->hit = status_calc_hit(bl, sc, b_status->hit + (status->dex - b_status->dex)

			 + (status->luk/3 - b_status->luk/3)

			 );
	}

	if(flag&SCB_FLEE) {
		if (status->agi == b_status->agi

			&& status->luk == b_status->luk

			)
			status->flee = status_calc_flee(bl, sc, b_status->flee);
		else
			status->flee = status_calc_flee(bl, sc, b_status->flee +(status->agi - b_status->agi)

			+ (status->luk/5 - b_status->luk/5)

			);
	}

	if(flag&SCB_DEF) {
		status->def = status_calc_def(bl, sc, b_status->def);

		if( bl->type&BL_HOM )
			status->def += (status->vit/5 - b_status->vit/5);
	}

	if(flag&SCB_DEF2) {
		status->def2 = status_calc_def2(bl, sc, b_status->def2);
	}

	if(flag&SCB_MDEF) {
		status->mdef = status_calc_mdef(bl, sc, b_status->mdef);

		if( bl->type&BL_HOM )
			status->mdef += (status->int_/5 - b_status->int_/5);
	}

	if(flag&SCB_MDEF2) {
		if (status->int_ == b_status->int_ && status->vit == b_status->vit

			&& status->dex == b_status->dex

			)
			status->mdef2 = status_calc_mdef2(bl, sc, b_status->mdef2);
		else
			status->mdef2 = status_calc_mdef2(bl, sc, b_status->mdef2 +(status->int_ - b_status->int_)

			+ (int)( ((float)status->dex/5 - (float)b_status->dex/5) + ((float)status->vit/5 - (float)b_status->vit/5) )
			);
	}

	if(flag&SCB_SPEED) {
		status->speed = status_calc_speed(bl, sc, b_status->speed);

		if( bl->type&BL_PC && !(sd && sd->state.permanent_speed) && status->speed < battle_config.max_walk_speed )
			status->speed = battle_config.max_walk_speed;

		if( bl->type&BL_PET && ((TBL_PET*)bl)->master)
			status->speed = status_get_speed(&((TBL_PET*)bl)->master->bl);
		if( bl->type&BL_ELEM && ((TBL_ELEM*)bl)->master)
			status->speed = status_get_speed(&((TBL_ELEM*)bl)->master->bl);
	}

	if(flag&SCB_CRI && b_status->cri) {
		if (status->luk == b_status->luk)
			status->cri = status_calc_critical(bl, sc, b_status->cri);
		else
			status->cri = status_calc_critical(bl, sc, b_status->cri + 3*(status->luk - b_status->luk));

		/// After status_calc_critical so the bonus is applied despite if you have or not a sc bugreport:5240
		if (sd) {
			if (sd->status.weapon == W_KATAR)
				status->cri <<= 1;
		}
	}

	if(flag&SCB_FLEE2 && b_status->flee2) {
		if (status->luk == b_status->luk)
			status->flee2 = status_calc_flee2(bl, sc, b_status->flee2);
		else
			status->flee2 = status_calc_flee2(bl, sc, b_status->flee2 +(status->luk - b_status->luk));
	}

	if(flag&SCB_ATK_ELE) {
		status->rhw.ele = status_calc_attack_element(bl, sc, b_status->rhw.ele);
		if (sd) sd->state.lr_flag = 1;
		status->lhw.ele = status_calc_attack_element(bl, sc, b_status->lhw.ele);
		if (sd) sd->state.lr_flag = 0;
	}

	if(flag&SCB_DEF_ELE) {
		status->def_ele = status_calc_element(bl, sc, b_status->def_ele);
		status->ele_lv = status_calc_element_lv(bl, sc, b_status->ele_lv);
	}

	if(flag&SCB_MODE) {
		status->mode = status_calc_mode(bl, sc, b_status->mode);

		if (status->class_ != CLASS_BATTLEFIELD && status_has_mode(status, MD_STATUSIMMUNE|MD_SKILLIMMUNE))
			status->class_ = CLASS_BATTLEFIELD;
		else if (status->class_ != CLASS_BOSS && status_has_mode(status, MD_STATUSIMMUNE|MD_KNOCKBACKIMMUNE|MD_DETECTOR))
			status->class_ = CLASS_BOSS;
		else if (status->class_ != CLASS_GUARDIAN && status_has_mode(status, MD_STATUSIMMUNE))
			status->class_ = CLASS_GUARDIAN;
		else if (status->class_ != CLASS_NORMAL)
			status->class_ = CLASS_NORMAL;

		// Since mode changed, reset their state.
		if (!status_has_mode(status,MD_CANATTACK))
			unit_stop_attack(bl);
		if (!status_has_mode(status,MD_CANMOVE))
			unit_stop_walking(bl,1);
	}

	/**
	* No status changes alter these yet.
	* if(flag&SCB_SIZE)
	* if(flag&SCB_RACE)
	* if(flag&SCB_RANGE)
	**/

	if(flag&SCB_MAXHP) {
		if( bl->type&BL_PC ) {
			status->max_hp = status_calc_maxhpsp_pc(sd,status->vit,true);

			if(battle_config.hp_rate != 100)
				status->max_hp = (unsigned int)(battle_config.hp_rate * (status->max_hp/100.));

			if (sd->status.base_level < 100)
				status->max_hp = umin(status->max_hp,(unsigned int)battle_config.max_hp_lv99);
			else if (sd->status.base_level < 151)
				status->max_hp = umin(status->max_hp,(unsigned int)battle_config.max_hp_lv150);
			else
				status->max_hp = umin(status->max_hp,(unsigned int)battle_config.max_hp);
		}
		else
			status->max_hp = status_calc_maxhp(bl, b_status->max_hp);

		if( status->hp > status->max_hp ) { // !FIXME: Should perhaps a status_zap should be issued?
			status->hp = status->max_hp;
			if( sd ) clif_updatestatus(sd,SP_HP);
		}
	}

	if(flag&SCB_MAXSP) {
		if( bl->type&BL_PC ) {
			status->max_sp = status_calc_maxhpsp_pc(sd,status->int_,false);

			if(battle_config.sp_rate != 100)
				status->max_sp = (unsigned int)(battle_config.sp_rate * (status->max_sp/100.));

			status->max_sp = umin(status->max_sp,(unsigned int)battle_config.max_sp);
		}
		else
			status->max_sp = status_calc_maxsp(bl, b_status->max_sp);

		if( status->sp > status->max_sp ) {
			status->sp = status->max_sp;
			if( sd ) clif_updatestatus(sd,SP_SP);
		}
	}

	if(flag&SCB_MATK) {

		/**
		 * RE MATK Formula (from irowiki:http:// irowiki.org/wiki/MATK)
		 * MATK = (sMATK + wMATK + eMATK) * Multiplicative Modifiers
		 **/
		int lv = status_get_lv(bl);
		status->matk_min = status_base_matk_min(bl, status, lv);
		status->matk_max = status_base_matk_max(bl, status, lv);

		switch( bl->type ) {
			case BL_PC: {
				int wMatk = 0;
				int variance = 0;

				// Any +MATK you get from skills and cards, including cards in weapon, is added here.
				if (sd) {
					uint16 skill_lv;

					if (sd->bonus.ematk > 0)
						status->matk_min += sd->bonus.ematk;
					
				}

				status->matk_min = status_calc_ematk(bl, sc, status->matk_min);
				status->matk_max = status->matk_min;

				// This is the only portion in MATK that varies depending on the weapon level and refinement rate.
				if (b_status->lhw.matk) {
					if (sd) {
						//sd->state.lr_flag = 1; //?? why was that set here
						status->lhw.matk = b_status->lhw.matk;
						sd->state.lr_flag = 0;
					} else {
						status->lhw.matk = b_status->lhw.matk;
					}
				}

				if (b_status->rhw.matk) {
					status->rhw.matk = b_status->rhw.matk;
				}

				if (status->rhw.matk) {
					wMatk += status->rhw.matk;
					variance += wMatk * status->rhw.wlv / 10;
				}

				if (status->lhw.matk) {
					wMatk += status->lhw.matk;
					variance += status->lhw.matk * status->lhw.wlv / 10;
				}

				status->matk_min += wMatk - variance;
				status->matk_max += wMatk + variance;
				}
				break;
		}
		if (bl->type&BL_PC && sd->matk_rate != 100) {
			status->matk_max = status->matk_max * sd->matk_rate/100;
			status->matk_min = status->matk_min * sd->matk_rate/100;
		}

		

		if( sd && sd->right_weapon.overrefine > 0) {
			status->matk_min++;
			status->matk_max += sd->right_weapon.overrefine - 1;
		}


		status->matk_max = status_calc_matk(bl, sc, status->matk_max);
		status->matk_min = status_calc_matk(bl, sc, status->matk_min);
		status->rhw.matk += status_calc_matk(bl, sc, 0);
	}

	if(flag&SCB_ASPD) {
		int amotion;

		if ( bl->type&BL_PC ) {
			uint16 skill_lv;

			amotion = status_base_amotion_pc(sd,status);

			// Absolute ASPD % modifiers
			amotion = amotion * status->aspd_rate / 1000;
			// if (sd->ud.skilltimer != INVALID_TIMER )

			// 	amotion = amotion * 5 * (5 + 10) / 100; //SA_FREECAST casting motion

			// RE ASPD % modifier
			amotion += (max(0xc3 - amotion, 2) * (status->aspd_rate2 + status_calc_aspd(bl, sc, false))) / 100;
			amotion = 10 * (200 - amotion);

			amotion += sd->bonus.aspd_add;

			amotion = status_calc_fix_aspd(bl, sc, amotion);
			status->amotion = cap_value(amotion,pc_maxaspd(sd),2000);

			status->adelay = 2 * status->amotion;
		} else { // Mercenary and mobs
			amotion = b_status->amotion;
			status->aspd_rate = status_calc_aspd_rate(bl, sc, b_status->aspd_rate);
			amotion = amotion*status->aspd_rate/1000;

			amotion = status_calc_fix_aspd(bl, sc, amotion);
			status->amotion = cap_value(amotion, battle_config.monster_max_aspd, 2000);

			temp = b_status->adelay*status->aspd_rate/1000;
			status->adelay = cap_value(temp, battle_config.monster_max_aspd*2, 4000);
		}
	}

	if(flag&SCB_DSPD) {
		int dmotion;
		if( bl->type&BL_PC ) {
			if (b_status->agi == status->agi)
				status->dmotion = status_calc_dmotion(bl, sc, b_status->dmotion);
			else {
				dmotion = 800-status->agi*4;
				status->dmotion = cap_value(dmotion, 400, 800);
				if(battle_config.pc_damage_delay_rate != 100)
					status->dmotion = status->dmotion*battle_config.pc_damage_delay_rate/100;
				// It's safe to ignore b_status->dmotion since no bonus affects it.
				status->dmotion = status_calc_dmotion(bl, sc, status->dmotion);
			}
		} else if( bl->type&BL_HOM ) {
			dmotion = 800-status->agi*4;
			status->dmotion = cap_value(dmotion, 400, 800);
			status->dmotion = status_calc_dmotion(bl, sc, b_status->dmotion);
		} else { // Mercenary and mobs
			status->dmotion = status_calc_dmotion(bl, sc, b_status->dmotion);
		}
	}

	if(flag&(SCB_VIT|SCB_MAXHP|SCB_INT|SCB_MAXSP) && bl->type&BL_REGEN)
		status_calc_regen(bl, status, status_get_regen_data(bl));

	if(flag&SCB_REGEN && bl->type&BL_REGEN)
		status_calc_regen_rate(bl, status_get_regen_data(bl), sc);
}

/**
 * Recalculates parts of an objects status according to specified flags
 * Also sends updates to the client when necessary
 * See [set_sc] [add_sc]
 * @param bl: Object whose status has changed [PC|MOB|HOM|MER|ELEM]
 * @param flag: Which status has changed on bl
 * @param opt: If true, will cause status_calc_* functions to run their base status initialization code
 */
void status_calc_bl_(struct block_list* bl, enum scb_flag flag, enum e_status_calc_opt opt)
{
	struct status_data b_status; // Previous battle status
	struct status_data* status; // Pointer to current battle status

	if (bl->type == BL_PC) {
		struct map_session_data *sd = BL_CAST(BL_PC, bl);

		if (sd->delayed_damage != 0) {
			if (opt&SCO_FORCE)
				sd->state.hold_recalc = false; // Clear and move on
			else {
				sd->state.hold_recalc = true; // Flag and stop
				return;
			}
		}
	}

	// Remember previous values
	status = status_get_status_data(bl);
	memcpy(&b_status, status, sizeof(struct status_data));

	if( flag&SCB_BASE ) { // Calculate the object's base status too
		switch( bl->type ) {
		case BL_PC:  status_calc_pc_(BL_CAST(BL_PC,bl), opt);          break;
		case BL_MOB: status_calc_mob_(BL_CAST(BL_MOB,bl), opt);        break;
		case BL_PET: status_calc_pet_(BL_CAST(BL_PET,bl), opt);        break;
		case BL_HOM: status_calc_homunculus_(BL_CAST(BL_HOM,bl), opt); break;
		case BL_MER: status_calc_mercenary_(BL_CAST(BL_MER,bl), opt);  break;
		case BL_ELEM: status_calc_elemental_(BL_CAST(BL_ELEM,bl), opt);  break;
		case BL_NPC: status_calc_npc_(BL_CAST(BL_NPC,bl), opt); break;
		}
	}

	if( bl->type == BL_PET )
		return; // Pets are not affected by statuses

	if (opt&SCO_FIRST && bl->type == BL_MOB)
		return; // Assume there will be no statuses active

	status_calc_bl_main(bl, flag);

	if (opt&SCO_FIRST && bl->type == BL_HOM)
		return; // Client update handled by caller

	// Compare against new values and send client updates
	if( bl->type == BL_PC ) {
		TBL_PC* sd = BL_CAST(BL_PC, bl);

		if(b_status.str != status->str)
			clif_updatestatus(sd,SP_STR);
		if(b_status.agi != status->agi)
			clif_updatestatus(sd,SP_AGI);
		if(b_status.vit != status->vit)
			clif_updatestatus(sd,SP_VIT);
		if(b_status.int_ != status->int_)
			clif_updatestatus(sd,SP_INT);
		if(b_status.dex != status->dex)
			clif_updatestatus(sd,SP_DEX);
		if(b_status.luk != status->luk)
			clif_updatestatus(sd,SP_LUK);
		if(b_status.hit != status->hit)
			clif_updatestatus(sd,SP_HIT);
		if(b_status.flee != status->flee)
			clif_updatestatus(sd,SP_FLEE1);
		if(b_status.amotion != status->amotion)
			clif_updatestatus(sd,SP_ASPD);
		if(b_status.speed != status->speed)
			clif_updatestatus(sd,SP_SPEED);

		if(b_status.batk != status->batk
#ifndef RENEWAL
			|| b_status.rhw.atk != status->rhw.atk || b_status.lhw.atk != status->lhw.atk
#endif
			)
			clif_updatestatus(sd,SP_ATK1);

		if(b_status.def != status->def) {
			clif_updatestatus(sd,SP_DEF1);
#ifdef RENEWAL
			clif_updatestatus(sd,SP_DEF2);
#endif
		}

		if(
#ifdef RENEWAL
			b_status.watk != status->watk || b_status.watk2 != status->watk2 || b_status.eatk != status->eatk
#else
			b_status.rhw.atk2 != status->rhw.atk2 || b_status.lhw.atk2 != status->lhw.atk2
#endif
			)
			clif_updatestatus(sd,SP_ATK2);

		if(b_status.def2 != status->def2) {
			clif_updatestatus(sd,SP_DEF2);
#ifdef RENEWAL
			clif_updatestatus(sd,SP_DEF1);
#endif
		}
		if(b_status.flee2 != status->flee2)
			clif_updatestatus(sd,SP_FLEE2);
		if(b_status.cri != status->cri)
			clif_updatestatus(sd,SP_CRITICAL);
#ifndef RENEWAL
		if(b_status.matk_max != status->matk_max)
			clif_updatestatus(sd,SP_MATK1);
		if(b_status.matk_min != status->matk_min)
			clif_updatestatus(sd,SP_MATK2);
#else
		if(b_status.matk_max != status->matk_max || b_status.matk_min != status->matk_min) {
			clif_updatestatus(sd,SP_MATK2);
			clif_updatestatus(sd,SP_MATK1);
		}
#endif
		if(b_status.mdef != status->mdef) {
			clif_updatestatus(sd,SP_MDEF1);
#ifdef RENEWAL
			clif_updatestatus(sd,SP_MDEF2);
#endif
		}
		if(b_status.mdef2 != status->mdef2) {
			clif_updatestatus(sd,SP_MDEF2);
#ifdef RENEWAL
			clif_updatestatus(sd,SP_MDEF1);
#endif
		}
		if(b_status.rhw.range != status->rhw.range)
			clif_updatestatus(sd,SP_ATTACKRANGE);
		if(b_status.max_hp != status->max_hp)
			clif_updatestatus(sd,SP_MAXHP);
		if(b_status.max_sp != status->max_sp)
			clif_updatestatus(sd,SP_MAXSP);
		if(b_status.hp != status->hp)
			clif_updatestatus(sd,SP_HP);
		if(b_status.sp != status->sp)
			clif_updatestatus(sd,SP_SP);
	} else if( bl->type == BL_ELEM ) {
		TBL_ELEM* ed = BL_CAST(BL_ELEM, bl);

		if (!ed->master)
			return;

		if( b_status.max_hp != status->max_hp )
			clif_elemental_updatestatus(ed->master, SP_MAXHP);
		if( b_status.max_sp != status->max_sp )
			clif_elemental_updatestatus(ed->master, SP_MAXSP);
		if( b_status.hp != status->hp )
			clif_elemental_updatestatus(ed->master, SP_HP);
		if( b_status.sp != status->sp )
			clif_mercenary_updatestatus(ed->master, SP_SP);
	}
}

/**
 * Adds strength modifications based on status changes
 * @param bl: Object to change str [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param str: Initial str
 * @return modified str with cap_value(str,0,USHRT_MAX)
 */
static unsigned short status_calc_str(struct block_list *bl, struct status_change *sc, int str)
{
	if(!sc || !sc->count)
		return cap_value(str,0,USHRT_MAX);


	if( sc->data[STATUS_ACCOUSTICRYTHM] )
		str += sc->data[STATUS_ACCOUSTICRYTHM]->val2;
	
	if(sc->data[STATUS_CHASEWALK2])
		str += sc->data[STATUS_CHASEWALK]->val1*2;
	
	if(sc->data[STATUS_STRFOOD])
		str += sc->data[STATUS_STRFOOD]->val1;
	
	if(sc->data[STATUS_SPIRITGROWTH])
		str += sc->data[STATUS_SPIRITGROWTH]->val1;
	if(sc->data[STATUS_LOUDEXCLAMATION])
		str += sc->data[STATUS_LOUDEXCLAMATION]->val1*2;
	if(sc->data[STATUS_SPEARSTANCE])
		str += sc->data[STATUS_SPEARSTANCE]->val1;
	
	if(sc->data[STATUS_BLESSING]) {
		if(sc->data[STATUS_BLESSING]->val2)
			str += sc->data[STATUS_BLESSING]->val2;
		else
			str >>= 1;
	}
	if(sc->data[STATUS_MARIONETTE2])
		str += ((sc->data[STATUS_MARIONETTE2]->val3)>>16)&0xFF;

	if(sc->data[STATUS_SWORDCLAN])
		str += 1;
	if(sc->data[STATUS_JUMPINGCLAN])
		str += 1;
	
	if(sc->data[STATUS_STRIPACCESSORY] && bl->type != BL_PC)
		str -= str * sc->data[STATUS_STRIPACCESSORY]->val2 / 100;
	if( sc->data[STATUS_ULTRAINSTINCT] )
		str += sc->data[STATUS_ULTRAINSTINCT]->val2;


	return (unsigned short)cap_value(str,0,USHRT_MAX);
}

/**
 * Adds agility modifications based on status changes
 * @param bl: Object to change agi [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param agi: Initial agi
 * @return modified agi with cap_value(agi,0,USHRT_MAX)
 */
static unsigned short status_calc_agi(struct block_list *bl, struct status_change *sc, int agi)
{
	if(!sc || !sc->count)
		return cap_value(agi,0,USHRT_MAX);

	if(sc->data[STATUS_BENEDICTIO])
		agi += sc->data[STATUS_BENEDICTIO]->val1*2;
	if(sc->data[STATUS_WINDRACER])
		agi += sc->data[STATUS_WINDRACER]->val1*3;
	if(sc->data[STATUS_IMPRESSIVERIFF])
		agi += sc->data[STATUS_IMPRESSIVERIFF]->val2;
	if(sc->data[STATUS_IMPROVECONCENTRATION] && !sc->data[STATUS_QUAGMIRE])
		agi += (agi-sc->data[STATUS_IMPROVECONCENTRATION]->val3 * 2)*(sc->data[STATUS_IMPROVECONCENTRATION]->val2 * 2)/100;
	
	if(sc->data[STATUS_AGIFOOD])
		agi += sc->data[STATUS_AGIFOOD]->val1;
	
	if(sc->data[STATUS_INCREASEAGI])
		// agi +=(sc->data[STATUS_INCREASEAGI]->val1/agi)*100;
		agi += sc->data[STATUS_INCREASEAGI]->val1 * 2;
	
	if(sc->data[STATUS_DECREASEAGI])
		agi -=(sc->data[STATUS_DECREASEAGI]->val1/agi)*100;
	if(sc->data[STATUS_QUAGMIRE])
		agi -= sc->data[STATUS_QUAGMIRE]->val2;
	
	if(sc->data[STATUS_MARIONETTE2])
		agi += ((sc->data[STATUS_MARIONETTE2]->val3)>>8)&0xFF;
	
	if(sc->data[STATUS_CROSSBOWCLAN])
		agi += 1;
	if(sc->data[STATUS_JUMPINGCLAN])
		agi += 1;
	
	if(sc->data[STATUS_STRIPACCESSORY] && bl->type != BL_PC)
		agi -= agi * sc->data[STATUS_STRIPACCESSORY]->val2 / 100;
	if( sc->data[STATUS_ULTRAINSTINCT] )
		agi += sc->data[STATUS_ULTRAINSTINCT]->val2;


	return (unsigned short)cap_value(agi,0,USHRT_MAX);
}

/**
 * Adds vitality modifications based on status changes
 * @param bl: Object to change vit [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param vit: Initial vit
 * @return modified vit with cap_value(vit,0,USHRT_MAX)
 */
static unsigned short status_calc_vit(struct block_list *bl, struct status_change *sc, int vit)
{
	if(!sc || !sc->count)
		return cap_value(vit,0,USHRT_MAX);

	
	if(sc->data[STATUS_BENEDICTIO])
		vit += sc->data[STATUS_BENEDICTIO]->val1*2;
	if( sc->data[STATUS_ACCOUSTICRYTHM] )
		vit += sc->data[STATUS_ACCOUSTICRYTHM]->val2;
	if(sc->data[STATUS_VIGOR])
		vit += sc->data[STATUS_VIGOR]->val1*2;
	
	if(sc->data[STATUS_VITFOOD])
		vit += sc->data[STATUS_VITFOOD]->val1;
	
	if(sc->data[STATUS_SPEARSTANCE])
		vit += sc->data[STATUS_SPEARSTANCE]->val1;
	
	if(sc->data[STATUS_MARIONETTE2])
		vit += sc->data[STATUS_MARIONETTE2]->val3&0xFF;
	
	if(sc->data[STATUS_SWORDCLAN])
		vit += 1;
	if(sc->data[STATUS_JUMPINGCLAN])
		vit += 1;
	
	if(sc->data[STATUS_STRIPACCESSORY] && bl->type != BL_PC)
		vit -= vit * sc->data[STATUS_STRIPACCESSORY]->val2 / 100;
	if( sc->data[STATUS_ULTRAINSTINCT] )
		vit += sc->data[STATUS_ULTRAINSTINCT]->val2;


	return (unsigned short)cap_value(vit,0,USHRT_MAX);
}

/**
 * Adds intelligence modifications based on status changes
 * @param bl: Object to change int [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param int_: Initial int
 * @return modified int with cap_value(int_,0,USHRT_MAX)
 */
static unsigned short status_calc_int(struct block_list *bl, struct status_change *sc, int int_)
{
	if(!sc || !sc->count)
		return cap_value(int_,0,USHRT_MAX);



	if(sc->data[STATUS_SPIRITANIMAL])
		int_ += sc->data[STATUS_SPIRITANIMAL]->val1*3;
	if(sc->data[STATUS_MAGICSTRINGS])
		int_ += sc->data[STATUS_MAGICSTRINGS]->val2;
	if(sc->data[STATUS_SPIRITGROWTH])
		int_ += sc->data[STATUS_SPIRITGROWTH]->val1;
	
	if(sc->data[STATUS_INTFOOD])
		int_ += sc->data[STATUS_INTFOOD]->val1;
	if(sc->data[STATUS_BLESSING]) {
		if (sc->data[STATUS_BLESSING]->val2)
			int_ += sc->data[STATUS_BLESSING]->val2;
		else
			int_ >>= 1;
	}
	
	if(sc->data[STATUS_MARIONETTE2])
		int_ += ((sc->data[STATUS_MARIONETTE2]->val4)>>16)&0xFF;
	if(sc->data[STATUS_ARCWANDCLAN])
		int_ += 1;
	if(sc->data[STATUS_GOLDENMACECLAN])
		int_ += 1;
	if(sc->data[STATUS_JUMPINGCLAN])
		int_ += 1;
	

	if(bl->type != BL_PC) {
		if(sc->data[STATUS_STRIPACCESSORY])
			int_ -= int_ * sc->data[STATUS_STRIPACCESSORY]->val2 / 100;
	}


	return (unsigned short)cap_value(int_,0,USHRT_MAX);
}

/**
 * Adds dexterity modifications based on status changes
 * @param bl: Object to change dex [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param dex: Initial dex
 * @return modified dex with cap_value(dex,0,USHRT_MAX)
 */
static unsigned short status_calc_dex(struct block_list *bl, struct status_change *sc, int dex)
{
	if(!sc || !sc->count)
		return cap_value(dex,0,USHRT_MAX);

	
	if(sc->data[STATUS_MAGICSTRINGS])
		dex += sc->data[STATUS_MAGICSTRINGS]->val2;
	if(sc->data[STATUS_IMPROVECONCENTRATION] && !sc->data[STATUS_QUAGMIRE])
		dex += (dex-sc->data[STATUS_IMPROVECONCENTRATION]->val4 * 2)*(sc->data[STATUS_IMPROVECONCENTRATION]->val2 *2)/100;
	
	if(sc->data[STATUS_DEXFOOD])
		dex += sc->data[STATUS_DEXFOOD]->val1;
	if(sc->data[STATUS_CHASEWALK2])
		dex += sc->data[STATUS_CHASEWALK]->val1*2;
	
	if(sc->data[STATUS_COINFLIP_DEX]){
		int skill_lv = 0;
		struct map_session_data *sd = map_id2sd(bl->id);
		if ((skill_lv = pc_checkskill(sd, SK_MC_COINFLIP)) > 0)
			dex += 3 * skill_lv;

	}
		
	if(sc->data[STATUS_QUAGMIRE])
		dex -= sc->data[STATUS_QUAGMIRE]->val2;
	if(sc->data[STATUS_BLESSING]) {
		if (sc->data[STATUS_BLESSING]->val2)
			dex += sc->data[STATUS_BLESSING]->val2;
		else
			dex >>= 1;
	}
	
	if(sc->data[STATUS_MARIONETTE2])
		dex += ((sc->data[STATUS_MARIONETTE2]->val4)>>8)&0xFF;
	
	if(sc->data[STATUS_ARCWANDCLAN])
		dex += 1;
	if(sc->data[STATUS_CROSSBOWCLAN])
		dex += 1;
	if(sc->data[STATUS_JUMPINGCLAN])
		dex += 1;
	if(sc->data[STATUS_STRIPACCESSORY] && bl->type != BL_PC)
		dex -= dex * sc->data[STATUS_STRIPACCESSORY]->val2 / 100;

	return (unsigned short)cap_value(dex,0,USHRT_MAX);
}

/**
 * Adds luck modifications based on status changes
 * @param bl: Object to change luk [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param luk: Initial luk
 * @return modified luk with cap_value(luk,0,USHRT_MAX)
 */
static unsigned short status_calc_luk(struct block_list *bl, struct status_change *sc, int luk)
{
	if(!sc || !sc->count)
		return cap_value(luk,0,USHRT_MAX);

	
	if(sc->data[STATUS_CURSE])
		return 0;
	if(sc->data[STATUS_BENEDICTIO])
		luk += sc->data[STATUS_BENEDICTIO]->val1*2;
	if(sc->data[STATUS_IMPRESSIVERIFF])
		luk += sc->data[STATUS_IMPRESSIVERIFF]->val2;
	
	if(sc->data[STATUS_LUKFOOD])
		luk += sc->data[STATUS_LUKFOOD]->val1;
	
	if(sc->data[STATUS_COINFLIP_LUK]) {
		int skill_lv = 0;
		struct map_session_data *sd = map_id2sd(bl->id);
		if ((skill_lv = pc_checkskill(sd, SK_MC_COINFLIP)) > 0)
			luk += 3 * skill_lv;
	}

	if(sc->data[STATUS_MARIONETTE2])
		luk += sc->data[STATUS_MARIONETTE2]->val4&0xFF;
	
	if(sc->data[STATUS_STRIPACCESSORY] && bl->type != BL_PC)
		luk -= luk * sc->data[STATUS_STRIPACCESSORY]->val2 / 100;
	
	if(sc->data[STATUS_GOLDENMACECLAN])
		luk += 1;
	if(sc->data[STATUS_JUMPINGCLAN])
		luk += 1;
	
	return (unsigned short)cap_value(luk,0,USHRT_MAX);
}

/**
 * Adds base attack modifications based on status changes
 * @param bl: Object to change batk [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param batk: Initial batk
 * @return modified batk with cap_value(batk,0,USHRT_MAX)
 */
static unsigned short status_calc_batk(struct block_list *bl, struct status_change *sc, int batk)
{
	if(!sc || !sc->count)
		return cap_value(batk,0,USHRT_MAX);
	if(sc->data[STATUS_INCATKRATE])
		batk += batk * sc->data[STATUS_INCATKRATE]->val1/100;
	if(sc->data[STATUS_PROVOKE])
		batk += batk * sc->data[STATUS_PROVOKE]->val3/100;
	if(sc->data[STATUS_CURSE])
		batk -= batk * 25/100;

	return (unsigned short)cap_value(batk,0,USHRT_MAX);
}

/**
 * Adds weapon attack modifications based on status changes
 * @param bl: Object to change watk [PC]
 * @param sc: Object's status change information
 * @param watk: Initial watk
 * @return modified watk with cap_value(watk,0,USHRT_MAX)
 */
static unsigned short status_calc_watk(struct block_list *bl, struct status_change *sc, int watk)
{
	if(!sc || !sc->count)
		return cap_value(watk,0,USHRT_MAX);
	if(sc->data[STATUS_BENEDICTIO])
		watk += sc->data[STATUS_BENEDICTIO]->val1*5;
	if(sc->data[STATUS_BATTLEDRUMS])
		watk += sc->data[STATUS_BATTLEDRUMS]->val2;
	if(sc->data[STATUS_MELTDOWN])
		watk += sc->data[STATUS_MELTDOWN]->val1*8;
	if(sc->data[STATUS_INLUCEMBATALIA])
		watk += sc->data[STATUS_INLUCEMBATALIA]->val1*8;
	if (sc->data[STATUS_IMPOSITIOMANUS])
		watk += sc->data[STATUS_IMPOSITIOMANUS]->val2;
	if (sc->data[STATUS_WEAPONSHARPENING])
		watk += sc->data[STATUS_WEAPONSHARPENING]->val2 * sc->data[STATUS_WEAPONSHARPENING]->val1;
	if (sc->data[STATUS_AXEQUICKEN])
		watk += 3*sc->data[STATUS_AXEQUICKEN]->val1;
	if (sc->data[STATUS_DAGGERQUICKEN])
		watk += 3*sc->data[STATUS_DAGGERQUICKEN]->val1;
	if (sc->data[STATUS_ONEHANDQUICKEN])
		watk += 3*sc->data[STATUS_ONEHANDQUICKEN]->val1;
	if (sc->data[STATUS_DANCINGBLADES])
		watk += 10*sc->data[STATUS_DANCINGBLADES]->val1;
	if (sc->data[STATUS_SPEARQUICKEN])
		watk += 3*sc->data[STATUS_SPEARQUICKEN]->val1;
	if (sc->data[STATUS_TWOHANDQUICKEN])
		watk += 3*sc->data[STATUS_TWOHANDQUICKEN]->val1;
	if (sc->data[STATUS_MACEQUICKEN])
		watk += 3*sc->data[STATUS_MACEQUICKEN]->val1;
	if (sc->data[STATUS_KNUCKLEQUICKEN])
		watk += 3*sc->data[STATUS_KNUCKLEQUICKEN]->val1;
	if (sc->data[STATUS_BOWQUICKEN])
		watk += 3*sc->data[STATUS_BOWQUICKEN]->val1;
	if (sc->data[STATUS_BOOKQUICKEN])
		watk += 3*sc->data[STATUS_BOOKQUICKEN]->val1;
	if (sc->data[STATUS_CYCLONICCHARGE])
		watk += 4*sc->data[STATUS_CYCLONICCHARGE]->val1;
	if (sc->data[STATUS_CRAZYUPROAR])
		watk += 5*sc->data[STATUS_CRAZYUPROAR]->val1;
	if( sc->data[STATUS_FURY] )
		watk += 15;
	if( sc->data[STATUS_ULTRAINSTINCT] )
		watk += sc->data[STATUS_ULTRAINSTINCT]->val3;
	if (sc->data[STATUS_KILLERINSTINCT])
		watk += sc->data[STATUS_KILLERINSTINCT]->val3;
	if(sc->data[STATUS_INCATKRATE])
		watk += watk * sc->data[STATUS_INCATKRATE]->val1/100;
	if(sc->data[STATUS_PROVOKE])
		watk += watk * sc->data[STATUS_PROVOKE]->val3/100;
	if(sc->data[STATUS_CURSE])
		watk -= watk * 25/100;
	if(sc->data[STATUS_STRIPWEAPON] && bl->type != BL_PC)
		watk -= watk * sc->data[STATUS_STRIPWEAPON]->val2/100;
	if(sc->data[STATUS_WINDMILLPOEM])
		watk += sc->data[STATUS_WINDMILLPOEM]->val3;
	if(sc->data[STATUS_CARTBOOST])
		watk += sc->data[STATUS_CARTBOOST]->val1*3;

	int skill_lv = 0;
	struct map_session_data *sd = map_id2sd(bl->id);
	if ((skill_lv = pc_checkskill(sd, SK_BS_REPAIRWEAPON)) > 0)
		watk += (skill_lv*2);
	if ((skill_lv = pc_checkskill(sd, SK_MS_WEAPONREFINE)) > 0)
		watk += (skill_lv * 5);
	
	return (unsigned short)cap_value(watk,0,USHRT_MAX);
}

#ifdef RENEWAL
/**
 * Adds equip magic attack modifications based on status changes [RENEWAL]
 * @param bl: Object to change matk [PC]
 * @param sc: Object's status change information
 * @param matk: Initial matk
 * @return modified matk with cap_value(matk,0,USHRT_MAX)
 */
static unsigned short status_calc_ematk(struct block_list *bl, struct status_change *sc, int matk)
{
	if (!sc || !sc->count)
		return cap_value(matk,0,USHRT_MAX);
	

	return (unsigned short)cap_value(matk,0,USHRT_MAX);
}
#endif

/**
 * Adds magic attack modifications based on status changes
 * @param bl: Object to change matk [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param matk: Initial matk
 * @return modified matk with cap_value(matk,0,USHRT_MAX)
 */
static unsigned short status_calc_matk(struct block_list *bl, struct status_change *sc, int matk)
{
	if(!sc || !sc->count)
		return cap_value(matk,0,USHRT_MAX);
	if(sc->data[STATUS_BENEDICTIO])
		matk += sc->data[STATUS_BENEDICTIO]->val1*5;
	if(sc->data[STATUS_SPIRITANIMAL])
		matk += sc->data[STATUS_SPIRITANIMAL]->val1*8;
	if(sc->data[STATUS_CRUCIS_PLAYER])
		matk += sc->data[STATUS_CRUCIS_PLAYER]->val3;
	if(sc->data[STATUS_BATTLEDRUMS])
		matk += sc->data[STATUS_BATTLEDRUMS]->val2;
	if(sc->data[STATUS_INLUCEMBATALIA])
		matk += sc->data[STATUS_INLUCEMBATALIA]->val1*8;
	if(sc->data[STATUS_ECHONOCTURNAE])
		matk += sc->data[STATUS_ECHONOCTURNAE]->val3;
	if(sc->data[STATUS_ENERGYCOAT])
		matk += sc->data[STATUS_ENERGYCOAT]->val3;
	if (sc->data[STATUS_MYSTICALAMPLIFICATION] && sc->data[STATUS_MYSTICALAMPLIFICATION]->val4)
		matk += matk * sc->data[STATUS_MYSTICALAMPLIFICATION]->val3/100;
	if (sc->data[STATUS_MINDGAMES])
		matk += matk * sc->data[STATUS_MINDGAMES]->val2/100;
	return (unsigned short)cap_value(matk,0,USHRT_MAX);
}

/**
 * Adds critical modifications based on status changes
 * @param bl: Object to change critical [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param critical: Initial critical
 * @return modified critical with cap_value(critical,10,USHRT_MAX)
 */
static signed short status_calc_critical(struct block_list *bl, struct status_change *sc, int critical)
{
	if(!sc || !sc->count)
		return cap_value(critical,10,SHRT_MAX);

	if(sc->data[STATUS_IMPRESSIVERIFF])
		critical += sc->data[STATUS_IMPRESSIVERIFF]->val3*10;
	if (sc->data[STATUS_FURY])
		critical += sc->data[STATUS_FURY]->val2;
	if (sc->data[STATUS_KILLERINSTINCT])
		critical += sc->data[STATUS_KILLERINSTINCT]->val2;
	if( sc->data[STATUS_WEAPONSHARPENING] )
		critical += sc->data[STATUS_WEAPONSHARPENING]->val3;

	int skill_lv = 0;
	struct map_session_data *sd = map_id2sd(bl->id);
	if ((skill_lv = pc_checkskill(sd, SK_HT_TRACKINGBREEZE)) > 0)
		critical += (skill_lv*30);

	return (short)cap_value(critical,10,SHRT_MAX);
}

/**
 * Adds hit modifications based on status changes
 * @param bl: Object to change hit [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param hit: Initial hit
 * @return modified hit with cap_value(hit,1,USHRT_MAX)
 */
static signed short status_calc_hit(struct block_list *bl, struct status_change *sc, int hit)
{
	if(!sc || !sc->count)
		return cap_value(hit,1,SHRT_MAX);


	if(sc->data[STATUS_SPEARDYNAMO])
		hit += sc->data[STATUS_SPEARDYNAMO]->val3;
	if(sc->data[STATUS_BLIND])
		hit -= hit * 25/100;
	if (sc->data[STATUS_BLESSING])
		hit += sc->data[STATUS_BLESSING]->val1 * 2;
	if (sc->data[STATUS_WEAPONRYEXPERTISE])
		hit += sc->data[STATUS_WEAPONRYEXPERTISE]->val1 * sc->data[STATUS_WEAPONRYEXPERTISE]->val3;
	if (sc->data[STATUS_ADRENALINERUSH])
		hit += sc->data[STATUS_ADRENALINERUSH]->val1 * 3 + 5;
	int skill_lv = 0;
	struct map_session_data *sd = map_id2sd(bl->id);
	if ((skill_lv = pc_checkskill(sd, SK_HT_TRACKINGBREEZE)) > 0)
		hit += (skill_lv*4);
	return (short)cap_value(hit,1,SHRT_MAX);
}

/**
 * Adds flee modifications based on status changes
 * @param bl: Object to change flee [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param flee: Initial flee
 * @return modified flee with cap_value(flee,1,USHRT_MAX)
 */
static signed short status_calc_flee(struct block_list *bl, struct status_change *sc, int flee)
{
	if( bl->type == BL_PC ) {
		struct map_data *mapdata = map_getmapdata(bl->m);

		if( mapdata_flag_gvg(mapdata) )
			flee -= flee * battle_config.gvg_flee_penalty/100;
		else if( mapdata->flag[MF_BATTLEGROUND] )
			flee -= flee * battle_config.bg_flee_penalty/100;
	}

	if(!sc || !sc->count)
		return cap_value(flee,1,SHRT_MAX);

	// Fixed value
	if (sc->data[STATUS_CYCLONICCHARGE])
		flee += 4*sc->data[STATUS_CYCLONICCHARGE]->val1;
	
	if(sc->data[STATUS_CLOSECONFINE])
		flee += sc->data[STATUS_CLOSECONFINE]->val1*10;
	
	if( sc->data[STATUS_HALLUCINATIONWALK] )
		flee += sc->data[STATUS_HALLUCINATIONWALK]->val2;

	// Rate value
	if(sc->data[STATUS_BLIND])
		flee -= flee * 25/100;
	

	return (short)cap_value(flee,1,SHRT_MAX);
}

/**
 * Adds perfect flee modifications based on status changes
 * @param bl: Object to change flee2 [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param flee2: Initial flee2
 * @return modified flee2 with cap_value(flee2,10,USHRT_MAX)
 */
static signed short status_calc_flee2(struct block_list *bl, struct status_change *sc, int flee2)
{
	if(!sc || !sc->count)
		return cap_value(flee2,10,SHRT_MAX);

	

	return (short)cap_value(flee2,10,SHRT_MAX);
}

/**
 * Adds defense (left-side) modifications based on status changes
 * @param bl: Object to change def [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param def: Initial def
 * @return modified def with cap_value(def,DEFTYPE_MIN,DEFTYPE_MAX)
 */
static defType status_calc_def(struct block_list *bl, struct status_change *sc, int def)
{
	if(!sc || !sc->count)
		return (defType)cap_value(def,DEFTYPE_MIN,DEFTYPE_MAX);

	
	if(sc->data[STATUS_DEFSET])
		return sc->data[STATUS_DEFSET]->val1;
	if(sc->data[STATUS_BATTLEDRUMS])
		def += sc->data[STATUS_BATTLEDRUMS]->val2;
	if(sc->data[STATUS_INCDEFRATE])
		def += def * sc->data[STATUS_INCDEFRATE]->val1/100;
	if(sc->data[STATUS_STONESKIN])
		def += sc->data[STATUS_STONESKIN]->val1;
	if(sc->data[STATUS_DEFENSIVESTANCE])
		def += sc->data[STATUS_DEFENSIVESTANCE]->val1;
	if(sc->data[STATUS_STONECURSE] && sc->opt1 == OPT1_STONE)
		def >>=1;
	if(sc->data[STATUS_FREEZE])
		def >>=1;
	if(sc->data[STATUS_PROVOKE] && bl->type != BL_PC) // Provoke doesn't alter player defense->
		def -= def * sc->data[STATUS_PROVOKE]->val4/100;
	if(sc->data[STATUS_STRIPSHIELD] && bl->type != BL_PC) // Player doesn't have def reduction only equip removed
		def -= def * sc->data[STATUS_STRIPSHIELD]->val2/100;
	if( sc->data[STATUS_VANGUARDFORCE] )
		def += sc->data[STATUS_VANGUARDFORCE]->val3;
	if( sc->data[STATUS_ARMORREINFORCEMENT] )
		def += sc->data[STATUS_ARMORREINFORCEMENT]->val1*sc->data[STATUS_ARMORREINFORCEMENT]->val3;
	if(sc->data[STATUS_WINDMILLPOEM])
		def += sc->data[STATUS_WINDMILLPOEM]->val3;
	if(sc->data[STATUS_ENDURE])
		def += sc->data[STATUS_ENDURE]->val1*4;
	if(sc->data[STATUS_FORTIFY])
		def += sc->data[STATUS_FORTIFY]->val2;
	if( sc->data[STATUS_ECHONOCTURNAE] )
		def += sc->data[STATUS_ECHONOCTURNAE]->val3;
	if( sc->data[STATUS_CAMOUFLAGE] )
		def -= def * 5 * sc->data[STATUS_CAMOUFLAGE]->val3 / 100;

	int skill_lv = 0;
	struct map_session_data *sd = map_id2sd(bl->id);
	if ((skill_lv = pc_checkskill(sd, SK_BS_REPAIRWEAPON)) > 0)
		def += (skill_lv*2);
	if ((skill_lv = pc_checkskill(sd, SK_MS_WEAPONREFINE)) > 0)
		def += (skill_lv * 5);

	return (defType)cap_value(def,DEFTYPE_MIN,DEFTYPE_MAX);
}

/**
 * Adds defense (right-side) modifications based on status changes
 * @param bl: Object to change def2 [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param def2: Initial def2
 * @return modified def2 with cap_value(def2,SHRT_MIN,SHRT_MAX)
 */
static signed short status_calc_def2(struct block_list *bl, struct status_change *sc, int def2)
{
	if(!sc || !sc->count)
#ifdef RENEWAL
		return (short)cap_value(def2,SHRT_MIN,SHRT_MAX);
#else
		return (short)cap_value(def2,1,SHRT_MAX);
#endif

	
	if(sc->data[STATUS_DEFSET])
		return sc->data[STATUS_DEFSET]->val1;

	
	if(sc->data[STATUS_ANGELUS]) {
		def2 += sc->data[STATUS_ANGELUS]->val2;
	}
	if(sc->data[STATUS_POISON])
		def2 -= def2 * 25/100;
	if(sc->data[STATUS_DPOISON])
		def2 -= def2 * 25/100;
	
	if(sc->data[STATUS_PROVOKE])
		def2 -= def2 * sc->data[STATUS_PROVOKE]->val4/100;
	
	if (sc->data[STATUS_PARALYSIS])
		def2 -= def2 * sc->data[STATUS_PARALYSIS]->val2 / 100;
	
	if( sc->data[STATUS_CAMOUFLAGE] )
		def2 -= def2 * 5 * sc->data[STATUS_CAMOUFLAGE]->val3 / 100;

#ifdef RENEWAL
	return (short)cap_value(def2,SHRT_MIN,SHRT_MAX);
#else
	return (short)cap_value(def2,1,SHRT_MAX);
#endif
}

/**
 * Adds magic defense (left-side) modifications based on status changes
 * @param bl: Object to change mdef [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param mdef: Initial mdef
 * @return modified mdef with cap_value(mdef,DEFTYPE_MIN,DEFTYPE_MAX)
 */
static defType status_calc_mdef(struct block_list *bl, struct status_change *sc, int mdef)
{
	if(!sc || !sc->count)
		return (defType)cap_value(mdef,DEFTYPE_MIN,DEFTYPE_MAX);

	
	if(sc->data[STATUS_MDEFSET])
		return sc->data[STATUS_MDEFSET]->val1;
	if(sc->data[STATUS_ECHONOCTURNAE])
		mdef += sc->data[STATUS_ECHONOCTURNAE]->val3;
	if(sc->data[STATUS_CRUCIS_PLAYER])
		mdef += sc->data[STATUS_CRUCIS_PLAYER]->val4;
	if(sc->data[STATUS_BATTLEDRUMS])
		mdef += sc->data[STATUS_BATTLEDRUMS]->val2;
	
	if(sc->data[STATUS_ENDURE])
		mdef += sc->data[STATUS_ENDURE]->val1*4;
	if(sc->data[STATUS_FORTIFY])
		mdef += sc->data[STATUS_FORTIFY]->val2;
	
	if(sc->data[STATUS_STONESKIN])
		mdef += sc->data[STATUS_STONESKIN]->val1;
	if(sc->data[STATUS_DEFENSIVESTANCE])
		mdef += sc->data[STATUS_DEFENSIVESTANCE]->val1;
	if(sc->data[STATUS_STONECURSE] && sc->opt1 == OPT1_STONE)
		mdef += 25 * mdef / 100;
	if(sc->data[STATUS_FREEZE])
		mdef += 25 * mdef / 100;
	if(sc->data[STATUS_MINDGAMES])
		mdef -= mdef * sc->data[STATUS_MINDGAMES]->val3/100;

	int skill_lv = 0;
	struct map_session_data* sd = map_id2sd(bl->id);
	if ((skill_lv = pc_checkskill(sd, SK_BS_REPAIRWEAPON)) > 0)
		mdef += (skill_lv * 2);
	if ((skill_lv = pc_checkskill(sd, SK_MS_WEAPONREFINE)) > 0)
		mdef += (skill_lv * 5);
	return (defType)cap_value(mdef,DEFTYPE_MIN,DEFTYPE_MAX);
}

/**
 * Adds magic defense (right-side) modifications based on status changes
 * @param bl: Object to change mdef2 [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param mdef2: Initial mdef2
 * @return modified mdef2 with cap_value(mdef2,SHRT_MIN,SHRT_MAX)
 */
static signed short status_calc_mdef2(struct block_list *bl, struct status_change *sc, int mdef2)
{
	if(!sc || !sc->count)
		return (short)cap_value(mdef2,SHRT_MIN,SHRT_MAX);

	if(sc->data[STATUS_MDEFSET])
		return sc->data[STATUS_MDEFSET]->val1;

	return (short)cap_value(mdef2,SHRT_MIN,SHRT_MAX);

}

/**
 * Adds speed modifications based on status changes
 * @param bl: Object to change speed [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param speed: Initial speed
 * @return modified speed with cap_value(speed,10,USHRT_MAX)
 */
static unsigned short status_calc_speed(struct block_list *bl, struct status_change *sc, int speed)
{
	TBL_PC* sd = BL_CAST(BL_PC, bl);
	int speed_rate = 100;

	if (sc == NULL || (sd && sd->state.permanent_speed))
		return (unsigned short)cap_value(speed, MIN_WALK_SPEED, MAX_WALK_SPEED);

	
	if( sd && sd->ud.skilltimer != INVALID_TIMER ) {
		
			speed_rate = 175 - 5 * 15; //SA_FREECAST casting speed rate
	} else {
		int val = 0;

		if( sd ) {
			if( pc_isriding(sd) || sd->sc.option&OPTION_DRAGON )
				val = 25; // Same bonus
			else if( sc->data[STATUS_ALL_RIDING] )
				val = battle_config.rental_mount_speed_boost;
		}
		speed_rate -= val;

		// GetMoveSlowValue()
		if( sd && sc->data[STATUS_HIDING] )
			val = 120 - 6 * pc_checkskill(sd,SK_TF_HIDING);
		else if( sd && sc->data[STATUS_CHASEWALK] && sc->data[STATUS_CHASEWALK]->val3 < 0 )
			val = sc->data[STATUS_CHASEWALK]->val3;
		else {
			val = 0;
			// Longing for Freedom/Special Singer cancels song/dance penalty
		
			
			if( sc->data[STATUS_DECREASEAGI] )
				val = max( val, 25 );
			if( sc->data[STATUS_QUAGMIRE]) 
				val = max( val, 50 );
			
			if( sc->data[STATUS_CURSE] )
				val = max( val, 300 );
			if( sc->data[STATUS_CHASEWALK] )
				val = max( val, sc->data[STATUS_CHASEWALK]->val3 );
			
			if( sc->data[STATUS_CLOAKING] && (sc->data[STATUS_CLOAKING]->val4&1) == 0 )
				val = max( val, sc->data[STATUS_CLOAKING]->val1 < 3 ? 300 : 30 - 3 * (sc->data[STATUS_CLOAKING]->val1*5) );
		
			if( sc->data[STATUS_CAMOUFLAGE] && sc->data[STATUS_CAMOUFLAGE]->val1 > 2 )
				val = max( val, 25 * (5 - sc->data[STATUS_CAMOUFLAGE]->val1) );
			

			if( sd && sd->bonus.speed_rate + sd->bonus.speed_add_rate > 0 ) // Permanent item-based speedup
				val = max( val, sd->bonus.speed_rate + sd->bonus.speed_add_rate );
		}
		speed_rate += val;
		val = 0;

	

		// GetMoveHasteValue1()
		
		if( sc->data[STATUS_INCREASEAGI] )
			val = max( val, 25 );
		if( sc->data[STATUS_WINDRACER] )
			val = max( val, 2 * sc->data[STATUS_WINDRACER]->val1 );
		if( sc->data[STATUS_CARTBOOST] )
			val = max( val, 20 );
		if( sc->data[STATUS_CLOAKING] && (sc->data[STATUS_CLOAKING]->val4&1) == 1 )
			val = max( val, sc->data[STATUS_CLOAKING]->val1 >= 10 ? 25 : 3 * sc->data[STATUS_CLOAKING]->val1 - 3 );
		if( sc->data[STATUS_FRENZY] )
			val = max( val, 25 );
		

		// !FIXME: official items use a single bonus for this [ultramage]
		
		if( sd && sd->bonus.speed_rate + sd->bonus.speed_add_rate < 0 ) // Permanent item-based speedup
			val = max( val, -(sd->bonus.speed_rate + sd->bonus.speed_add_rate) );

		speed_rate -= val;

		if( speed_rate < 40 )
			speed_rate = 40;
	}

	// GetSpeed()
	// if( sd && pc_iscarton(sd) )
	// 	speed += speed * (50 - 5 * pc_checkskill(sd,MC_PUSHCART)) / 100;
	
	if( speed_rate != 100 )
		speed = speed * speed_rate / 100;
	if( sc->data[STATUS_MENTALSTRENGTH] )
		speed = 200;
	if( sc->data[STATUS_DEFENDINGAURA] )
		speed = max(speed, 200);
	

	return (unsigned short)cap_value(speed, MIN_WALK_SPEED, MAX_WALK_SPEED);
}

#ifdef RENEWAL_ASPD
/**
 * Renewal attack speed modifiers based on status changes
 * This function only affects RENEWAL players and comes after base calculation
 * @param bl: Object to change aspd [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param fixed: True - fixed value [malufett]
 *               False - percentage value
 * @return modified aspd
 */
static short status_calc_aspd(struct block_list *bl, struct status_change *sc, bool fixed)
{
	int bonus = 0;

	if (!sc || !sc->count)
		return 0;

	if (fixed) {
		enum sc_type sc_val;

		if (!sc->data[STATUS_QUAGMIRE]) {
			// !TODO: How does Two-Hand Quicken, Adrenaline Rush, and Spear quick change? (+10%)
			if (bonus < 7 && ( sc->data[STATUS_WEAPONRYEXPERTISE] ))
				bonus = 7;
		}

		if (sc->data[STATUS_IMPRESSIVERIFF] && bonus < sc->data[STATUS_IMPRESSIVERIFF]->val3) {
			bonus += sc->data[STATUS_IMPRESSIVERIFF]->val3;
		}



		if (sc->data[sc_val = STATUS_ASPDPOTION3] || sc->data[sc_val = STATUS_ASPDPOTION2] || sc->data[sc_val = STATUS_ASPDPOTION1] || sc->data[sc_val = STATUS_ASPDPOTION0])
			bonus += sc->data[sc_val]->val1;
	} else {
		

		if (sc->data[STATUS_MENTALSTRENGTH])
			bonus -= 25;

		
		if (sc->data[STATUS_DEFENDINGAURA])
			bonus -= sc->data[STATUS_DEFENDINGAURA]->val4 / 10;

	
		if (sc->data[STATUS_INCREASEAGI])
			bonus += sc->data[STATUS_INCREASEAGI]->val1 * 2;
		

		struct map_session_data* sd = BL_CAST(BL_PC, bl);
		uint8 skill_lv;

		// if (sd && (skill_lv = (pc_checkskill(sd, SK_BA_SHOWMANSHIP)*2)) > 0)
		// 	bonus += skill_lv;
		
		
	}

	return bonus;
}
#endif

/**
 * Modifies ASPD by a number, rather than a percentage (10 = 1 ASPD)
 * A subtraction reduces the delay, meaning an increase in ASPD
 * This comes after the percentage changes and is based on status changes
 * @param bl: Object to change aspd [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param aspd: Object's current ASPD
 * @return modified aspd
 */
static short status_calc_fix_aspd(struct block_list *bl, struct status_change *sc, int aspd)
{
	if (!sc || !sc->count)
		return cap_value(aspd, 0, 2000);
	
	
	if (sc->data[STATUS_WEAPONRYEXPERTISE])
		aspd -= sc->data[STATUS_WEAPONRYEXPERTISE]->val1*sc->data[STATUS_WEAPONRYEXPERTISE]->val2;
	if (sc->data[STATUS_ADRENALINERUSH])
		aspd -= sc->data[STATUS_ADRENALINERUSH]->val1*10;
	if (sc->data[STATUS_AXEQUICKEN])
		aspd -= sc->data[STATUS_AXEQUICKEN]->val1*10;
	if (sc->data[STATUS_DAGGERQUICKEN])
		aspd -= sc->data[STATUS_DAGGERQUICKEN]->val1*10;
	if (sc->data[STATUS_ONEHANDQUICKEN])
		aspd -= sc->data[STATUS_ONEHANDQUICKEN]->val1*10;
	if (sc->data[STATUS_SPEARQUICKEN])
		aspd -= sc->data[STATUS_SPEARQUICKEN]->val1*10;
	if (sc->data[STATUS_TWOHANDQUICKEN])
		aspd -= sc->data[STATUS_TWOHANDQUICKEN]->val1*10;
	if (sc->data[STATUS_MACEQUICKEN])
		aspd -= sc->data[STATUS_MACEQUICKEN]->val1*10;
	if (sc->data[STATUS_KNUCKLEQUICKEN])
		aspd -= sc->data[STATUS_KNUCKLEQUICKEN]->val1*10;
	if (sc->data[STATUS_INLUCEMBATALIA])
		aspd -= sc->data[STATUS_INLUCEMBATALIA]->val1*20;
	if (sc->data[STATUS_BOWQUICKEN])
		aspd -= sc->data[STATUS_BOWQUICKEN]->val1*10;
	if (sc->data[STATUS_BOOKQUICKEN])
		aspd -= sc->data[STATUS_BOOKQUICKEN]->val1*10;
	if (sc->data[STATUS_FRENZY]){
		aspd -= sc->data[STATUS_FRENZY]->val1*20;
	}


	return cap_value(aspd, 0, 2000); // Will be recap for proper bl anyway
}

/**
 * Calculates an object's ASPD modifier based on status changes (alters amotion value)
 * Note: The scale of aspd_rate is 1000 = 100%
 * Note2: This only affects Homunculus, Mercenaries, and Pre-renewal Players
 * @param bl: Object to change aspd [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param aspd_rate: Object's current ASPD
 * @return modified aspd_rate
 */
static short status_calc_aspd_rate(struct block_list *bl, struct status_change *sc, int aspd_rate)
{
	int i;

	if(!sc || !sc->count)
		return cap_value(aspd_rate,0,SHRT_MAX);

	if( !sc->data[STATUS_QUAGMIRE] ) {
		int max = 0;
		


		if(sc->data[STATUS_WEAPONRYEXPERTISE] &&
			max < sc->data[STATUS_WEAPONRYEXPERTISE]->val3)
			max = sc->data[STATUS_WEAPONRYEXPERTISE]->val3;

		if(sc->data[STATUS_ADRENALINERUSH] &&
			max < sc->data[STATUS_ADRENALINERUSH]->val3)
			max = sc->data[STATUS_ADRENALINERUSH]->val3;
		
		

		if(sc->data[STATUS_IMPRESSIVERIFF] && max < sc->data[STATUS_IMPRESSIVERIFF]->val3) {
			if (bl->type!=BL_PC)
				max = sc->data[STATUS_IMPRESSIVERIFF]->val3;
			else
				switch(((TBL_PC*)bl)->status.weapon) {
					case W_BOW:
					case W_REVOLVER:
					case W_RIFLE:
					case W_GATLING:
					case W_SHOTGUN:
					case W_GRENADE:
						break;
					default:
						max = sc->data[STATUS_IMPRESSIVERIFF]->val3;
			}
		}
		aspd_rate -= max;

		if(sc->data[STATUS_FRENZY])
			aspd_rate -= 300;
	}

	if( sc->data[i=STATUS_ASPDPOTION3] ||
		sc->data[i=STATUS_ASPDPOTION2] ||
		sc->data[i=STATUS_ASPDPOTION1] ||
		sc->data[i=STATUS_ASPDPOTION0] )
		aspd_rate -= sc->data[i]->val2;


	if(sc->data[STATUS_MENTALSTRENGTH])
		aspd_rate += 250;
	
	if(sc->data[STATUS_DEFENDINGAURA])
		aspd_rate += sc->data[STATUS_DEFENDINGAURA]->val4;



	return (short)cap_value(aspd_rate,0,SHRT_MAX);
}

/**
 * Modifies the damage delay time based on status changes
 * The lower your delay, the quicker you can act after taking damage
 * @param bl: Object to change aspd [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param dmotion: Object's current damage delay
 * @return modified delay rate
 */
static unsigned short status_calc_dmotion(struct block_list *bl, struct status_change *sc, int dmotion)
{
	if( !sc || !sc->count || map_flag_gvg2(bl->m) || map_getmapflag(bl->m, MF_BATTLEGROUND) )
		return cap_value(dmotion,0,USHRT_MAX);

	/// It has been confirmed on official servers that MvP mobs have no dmotion even without endure
	if( bl->type == BL_MOB && status_get_class_(bl) == CLASS_BOSS )
		return 0;
	if (bl->type == BL_PC && ((TBL_PC *)bl)->special_state.no_walk_delay)
		return 0;
	if( sc->data[STATUS_ENDURE] )
		return 0;

	return (unsigned short)cap_value(dmotion,0,USHRT_MAX);
}

/**
 * Calculates a max HP based on status changes
 * Values can either be percentages or fixed, based on how equations are formulated
 * @param bl: Object's block_list data
 * @param maxhp: Object's current max HP
 * @return modified maxhp
 */
static unsigned int status_calc_maxhp(struct block_list *bl, uint64 maxhp)
{
	int rate = 100;

	maxhp += status_get_hpbonus(bl,STATUS_BONUS_FIX);

	if ((rate += status_get_hpbonus(bl,STATUS_BONUS_RATE)) != 100)
		maxhp = maxhp * rate / 100;
	
	
	return (unsigned int)cap_value(maxhp,1,UINT_MAX);
}

/**
 * Calculates a max SP based on status changes
 * Values can either be percentages or fixed, bas ed on how equations are formulated
 * @param bl: Object's block_list data
 * @param maxsp: Object's current max SP
 * @return modified maxsp
 */
static unsigned int status_calc_maxsp(struct block_list *bl, uint64 maxsp)
{
	int rate = 100;

	maxsp += status_get_spbonus(bl,STATUS_BONUS_FIX);
	
	if ((rate += status_get_spbonus(bl,STATUS_BONUS_RATE)) != 100)
		maxsp = maxsp * rate / 100;
	
	return (unsigned int)cap_value(maxsp,1,UINT_MAX);
}

/**
 * Changes a player's element based on status changes
 * @param bl: Object to change aspd [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param element: Object's current element
 * @return new element
 */
static unsigned char status_calc_element(struct block_list *bl, struct status_change *sc, int element)
{
	if(!sc || !sc->count)
		return cap_value(element, 0, UCHAR_MAX);

	if(sc->data[STATUS_FREEZE])
		return ELE_WATER;
	if(sc->data[STATUS_STONECURSE] && sc->opt1 == OPT1_STONE)
		return ELE_EARTH;
	if(sc->data[STATUS_BENEDICTIO])
		return ELE_HOLY;

	return (unsigned char)cap_value(element,0,UCHAR_MAX);
}

/**
 * Changes a player's element level based on status changes
 * @param bl: Object to change aspd [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param lv: Object's current element level
 * @return new element level
 */
static unsigned char status_calc_element_lv(struct block_list *bl, struct status_change *sc, int lv)
{
	if(!sc || !sc->count)
		return cap_value(lv, 1, 4);

	if(sc->data[STATUS_FREEZE])
		return 1;
	if(sc->data[STATUS_STONECURSE] && sc->opt1 == OPT1_STONE)
		return 1;
	if(sc->data[STATUS_BENEDICTIO])
		return 1;
	

	return (unsigned char)cap_value(lv,1,4);
}

/**
 * Changes a player's attack element based on status changes
 * @param bl: Object to change aspd [PC|MOB|HOM|MER|ELEM]
 * @param sc: Object's status change information
 * @param element: Object's current attack element
 * @return new attack element
 */
unsigned char status_calc_attack_element(struct block_list *bl, struct status_change *sc, int element)
{
	if(!sc || !sc->count)
		return cap_value(element, 0, UCHAR_MAX);
	
	if(sc->data[STATUS_ENCHANTPOISON])
		return ELE_POISON;
	if(sc->data[STATUS_ASPERSIO])
		return ELE_HOLY;
	
	return (unsigned char)cap_value(element,0,UCHAR_MAX);
}

/**
 * Changes the mode of an object
 * @param bl: Object whose mode to change [PC|MOB|PET|HOM|NPC]
 * @param sc: Object's status change data
 * @param mode: Original mode
 * @return mode with cap_value(mode, 0, INT_MAX)
 */
static enum e_mode status_calc_mode(struct block_list *bl, struct status_change *sc, enum e_mode mode)
{
	if(!sc || !sc->count)
		return cap_value(mode, MD_NONE, static_cast<e_mode>(INT_MAX));
	
	return cap_value(mode, MD_NONE, static_cast<e_mode>(INT_MAX));
}

/**
 * Changes the mode of a slave mob
 * @param md: Slave mob whose mode to change
 * @param mmd: Master of slave mob
 */
void status_calc_slave_mode(struct mob_data *md, struct mob_data *mmd)
{
	//switch (battle_config.slaves_inherit_mode) {
	//	case 1: //Always aggressive
	//		if (!status_has_mode(&md->status,MD_AGGRESSIVE))
	//			sc_start4(NULL, &md->bl, SC_MODECHANGE, 100, 1, 0, MD_AGGRESSIVE, 0, 0);
	//		break;
	//	case 2: //Always passive
	//		if (status_has_mode(&md->status,MD_AGGRESSIVE))
	//			sc_start4(NULL, &md->bl, SC_MODECHANGE, 100, 1, 0, 0, MD_AGGRESSIVE, 0);
	//		break;
	//	case 4: // Overwrite with slave mode
	//		sc_start4(NULL, &md->bl, SC_MODECHANGE, 100, 1, MD_CANMOVE|MD_NORANDOMWALK|MD_CANATTACK, 0, 0, 0);
	//		break;
	//	default: //Copy master
	//		if (status_has_mode(&mmd->status,MD_AGGRESSIVE))
	//			sc_start4(NULL, &md->bl, SC_MODECHANGE, 100, 1, 0, MD_AGGRESSIVE, 0, 0);
	//		else
	//			sc_start4(NULL, &md->bl, SC_MODECHANGE, 100, 1, 0, 0, MD_AGGRESSIVE, 0);
	//		break;
	//}
}

/**
 * Gets the name of the given bl
 * @param bl: Object whose name to get [PC|MOB|PET|HOM|NPC]
 * @return name or "Unknown" if any other bl->type than noted above
 */
const char* status_get_name(struct block_list *bl)
{
	nullpo_ret(bl);
	switch (bl->type) {
		case BL_PC:	return ((TBL_PC *)bl)->fakename[0] != '\0' ? ((TBL_PC*)bl)->fakename : ((TBL_PC*)bl)->status.name;
		case BL_MOB:	return ((TBL_MOB*)bl)->name;
		case BL_PET:	return ((TBL_PET*)bl)->pet.name;
		case BL_NPC:	return ((TBL_NPC*)bl)->name;
	}
	return "Unknown";
}

/**
 * Gets the class/sprite id of the given bl
 * @param bl: Object whose class to get [PC|MOB|PET|HOM|MER|NPC|ELEM]
 * @return class or 0 if any other bl->type than noted above
 */
int status_get_class(struct block_list *bl)
{
	nullpo_ret(bl);
	switch( bl->type ) {
		case BL_PC:	return ((TBL_PC*)bl)->status.class_;
		case BL_MOB:	return ((TBL_MOB*)bl)->vd->class_; // Class used on all code should be the view class of the mob.
		case BL_PET:	return ((TBL_PET*)bl)->pet.class_;
		case BL_NPC:	return ((TBL_NPC*)bl)->class_;
		case BL_ELEM:	return ((TBL_ELEM*)bl)->elemental.class_;
	}
	return 0;
}

/**
 * Gets the base level of the given bl
 * @param bl: Object whose base level to get [PC|MOB|PET|HOM|MER|NPC|ELEM]
 * @return base level or 1 if any other bl->type than noted above
 */
int status_get_lv(struct block_list *bl)
{
	nullpo_ret(bl);
	switch (bl->type) {
		case BL_PC:	return ((TBL_PC*)bl)->status.base_level;
		case BL_MOB:	return ((TBL_MOB*)bl)->level;
		case BL_PET:	return ((TBL_PET*)bl)->pet.level;
		case BL_ELEM:	return ((TBL_PC*)battle_get_master(bl))->status.base_level;
		case BL_NPC:	return ((TBL_NPC*)bl)->level;
	}
	return 1;
}

/**
 * Gets the regeneration info of the given bl
 * @param bl: Object whose regen info to get [PC|HOM|MER|ELEM]
 * @return regen data or NULL if any other bl->type than noted above
 */
struct regen_data *status_get_regen_data(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	switch (bl->type) {
		case BL_PC:	return &((TBL_PC*)bl)->regen;
		case BL_ELEM:	return &((TBL_ELEM*)bl)->regen;
		default:
			return NULL;
	}
}

/**
 * Gets the status data of the given bl
 * @param bl: Object whose status to get [PC|MOB|PET|HOM|MER|ELEM|NPC]
 * @return status or "dummy_status" if any other bl->type than noted above
 */
struct status_data *status_get_status_data(struct block_list *bl)
{
	nullpo_retr(&dummy_status, bl);

	switch (bl->type) {
		case BL_PC: 	return &((TBL_PC*)bl)->battle_status;
		case BL_MOB:	return &((TBL_MOB*)bl)->status;
		case BL_PET:	return &((TBL_PET*)bl)->status;
		case BL_ELEM:	return &((TBL_ELEM*)bl)->battle_status;
		case BL_NPC:	return ((mobdb_checkid(((TBL_NPC*)bl)->class_) == 0) ? &((TBL_NPC*)bl)->status : &dummy_status);
		default:
			return &dummy_status;
	}
}

/**
 * Gets the base status data of the given bl
 * @param bl: Object whose status to get [PC|MOB|PET|HOM|MER|ELEM|NPC]
 * @return base_status or NULL if any other bl->type than noted above
 */
struct status_data *status_get_base_status(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	switch (bl->type) {
		case BL_PC:	return &((TBL_PC*)bl)->base_status;
		case BL_MOB:	return ((TBL_MOB*)bl)->base_status ? ((TBL_MOB*)bl)->base_status : &((TBL_MOB*)bl)->db->status;
		case BL_PET:	return &((TBL_PET*)bl)->db->status;
		case BL_ELEM:	return &((TBL_ELEM*)bl)->base_status;
		case BL_NPC:	return ((mobdb_checkid(((TBL_NPC*)bl)->class_) == 0) ? &((TBL_NPC*)bl)->status : NULL);
		default:
			return NULL;
	}
}

/**
 * Gets the defense of the given bl
 * @param bl: Object whose defense to get [PC|MOB|HOM|MER|ELEM]
 * @return defense with cap_value(def, DEFTYPE_MIN, DEFTYPE_MAX)
 */
defType status_get_def(struct block_list *bl)
{
	struct unit_data *ud;
	struct status_data *status = status_get_status_data(bl);
	int def = status?status->def:0;
	ud = unit_bl2ud(bl);
	if (ud && ud->skilltimer != INVALID_TIMER)
		def -= def * skill_get_castdef(ud->skill_id)/100;

	return cap_value(def, DEFTYPE_MIN, DEFTYPE_MAX);
}

/**
 * Gets the walking speed of the given bl
 * @param bl: Object whose speed to get [PC|MOB|PET|HOM|MER|ELEM|NPC]
 * @return speed
 */
unsigned short status_get_speed(struct block_list *bl)
{
	if(bl->type==BL_NPC)// Only BL with speed data but no status_data [Skotlex]
		return ((struct npc_data *)bl)->speed;
	return status_get_status_data(bl)->speed;
}

/**
 * Gets the party ID of the given bl
 * @param bl: Object whose party ID to get [PC|MOB|PET|HOM|MER|SKILL|ELEM]
 * @return party ID
 */
int status_get_party_id(struct block_list *bl)
{
	nullpo_ret(bl);
	switch (bl->type) {
		case BL_PC:
			return ((TBL_PC*)bl)->status.party_id;
		case BL_PET:
			if (((TBL_PET*)bl)->master)
				return ((TBL_PET*)bl)->master->status.party_id;
			break;
		case BL_MOB: {
				struct mob_data *md=(TBL_MOB*)bl;
				if( md->master_id > 0 ) {
					struct map_session_data *msd;
					if (md->special_state.ai && (msd = map_id2sd(md->master_id)) != NULL)
						return msd->status.party_id;
					return -md->master_id;
				}
			}
			break;
		case BL_SKILL:
			if (((TBL_SKILL*)bl)->group)
				return ((TBL_SKILL*)bl)->group->party_id;
			break;
		case BL_ELEM:
			if (((TBL_ELEM*)bl)->master)
				return ((TBL_ELEM*)bl)->master->status.party_id;
			break;
	}
	return 0;
}

/**
 * Gets the guild ID of the given bl
 * @param bl: Object whose guild ID to get [PC|MOB|PET|HOM|MER|SKILL|ELEM|NPC]
 * @return guild ID
 */
int status_get_guild_id(struct block_list *bl)
{
	nullpo_ret(bl);
	switch (bl->type) {
		case BL_PC:
			return ((TBL_PC*)bl)->status.guild_id;
		case BL_PET:
			if (((TBL_PET*)bl)->master)
				return ((TBL_PET*)bl)->master->status.guild_id;
			break;
		case BL_MOB:
			{
				struct map_session_data *msd;
				struct mob_data *md = (struct mob_data *)bl;
				if (md->guardian_data)	// Guardian's guild [Skotlex]
					return md->guardian_data->guild_id;
				if (md->special_state.ai && (msd = map_id2sd(md->master_id)) != NULL)
					return msd->status.guild_id; // Alchemist's mobs [Skotlex]
			}
			break;
		
		case BL_NPC:
			if (((TBL_NPC*)bl)->subtype == NPCTYPE_SCRIPT)
				return ((TBL_NPC*)bl)->u.scr.guild_id;
			break;
		case BL_SKILL:
			if (((TBL_SKILL*)bl)->group)
				return ((TBL_SKILL*)bl)->group->guild_id;
			break;
		case BL_ELEM:
			if (((TBL_ELEM*)bl)->master)
				return ((TBL_ELEM*)bl)->master->status.guild_id;
			break;
	}
	return 0;
}

/**
 * Gets the guild emblem ID of the given bl
 * @param bl: Object whose emblem ID to get [PC|MOB|PET|HOM|MER|SKILL|ELEM|NPC]
 * @return guild emblem ID
 */
int status_get_emblem_id(struct block_list *bl)
{
	nullpo_ret(bl);
	switch (bl->type) {
		case BL_PC:
			return ((TBL_PC*)bl)->guild_emblem_id;
		case BL_PET:
			if (((TBL_PET*)bl)->master)
				return ((TBL_PET*)bl)->master->guild_emblem_id;
			break;
		case BL_MOB:
			{
				struct map_session_data *msd;
				struct mob_data *md = (struct mob_data *)bl;
				if (md->guardian_data)	// Guardian's guild [Skotlex]
					return md->guardian_data->emblem_id;
				if (md->special_state.ai && (msd = map_id2sd(md->master_id)) != NULL)
					return msd->guild_emblem_id; // Alchemist's mobs [Skotlex]
			}
			break;
		case BL_NPC:
			if (((TBL_NPC*)bl)->subtype == NPCTYPE_SCRIPT && ((TBL_NPC*)bl)->u.scr.guild_id > 0) {
				struct guild *g = guild_search(((TBL_NPC*)bl)->u.scr.guild_id);
				if (g)
					return g->emblem_id;
			}
			break;
		case BL_ELEM:
			if (((TBL_ELEM*)bl)->master)
				return ((TBL_ELEM*)bl)->master->guild_emblem_id;
			break;
	}
	return 0;
}

/**
 * Gets the race2 of a mob or pet
 * @param bl: Object whose race2 to get [MOB|PET]
 * @return race2
 */
std::vector<e_race2> status_get_race2(struct block_list *bl)
{
	nullpo_retr(std::vector<e_race2>(),bl);

	if (bl->type == BL_MOB)
		return ((struct mob_data *)bl)->db->race2;
	if (bl->type == BL_PET)
		return ((struct pet_data *)bl)->db->race2;
	return std::vector<e_race2>();
}

/**
 * Checks if an object is dead 
 * @param bl: Object to check [PC|MOB|HOM|MER|ELEM]
 * @return 1: Is dead or 0: Is alive
 */
int status_isdead(struct block_list *bl)
{
	nullpo_ret(bl);
	return status_get_status_data(bl)->hp == 0;
}

/**
 * Checks if an object is immune to magic 
 * @param bl: Object to check [PC|MOB|HOM|MER|ELEM]
 * @return value of magic damage to be blocked
 */
int status_isimmune(struct block_list *bl)
{
	struct status_change *sc =status_get_sc(bl);
	

	if (bl->type == BL_PC &&
		((TBL_PC*)bl)->special_state.no_magic_damage >= battle_config.gtb_sc_immunity)
		return ((TBL_PC*)bl)->special_state.no_magic_damage;
	return 0;
}

/**
 * Get view data of an object 
 * @param bl: Object whose view data to get [PC|MOB|PET|HOM|MER|ELEM|NPC]
 * @return view data structure bl->vd
 */
struct view_data* status_get_viewdata(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	switch (bl->type) {
		case BL_PC:  return &((TBL_PC*)bl)->vd;
		case BL_MOB: return ((TBL_MOB*)bl)->vd;
		case BL_PET: return &((TBL_PET*)bl)->vd;
		case BL_NPC: return &((TBL_NPC*)bl)->vd;
		case BL_ELEM: return ((TBL_ELEM*)bl)->vd;
	}
	return NULL;
}

/**
 * Set view data of an object
 * This function deals with class, mount, and item views
 * SC views are set in clif_getareachar_unit() 
 * @param bl: Object whose view data to set [PC|MOB|PET|HOM|MER|ELEM|NPC]
 * @param class_: class of the object
 */
void status_set_viewdata(struct block_list *bl, int class_)
{
	struct view_data* vd;
	nullpo_retv(bl);
	if (mobdb_checkid(class_) || mob_is_clone(class_))
		vd = mob_get_viewdata(class_);
	else if (npcdb_checkid(class_))
		vd = npc_get_viewdata(class_);
	else if (elemental_class(class_))
		vd = elemental_get_viewdata(class_);
	else
		vd = NULL;

	switch (bl->type) {
	case BL_PC:
		{
			TBL_PC* sd = (TBL_PC*)bl;
			if (pcdb_checkid(class_)) {
				if (sd->sc.option&OPTION_RIDING) {
					switch (class_) { // Adapt class to a Mounted one.
						case JOB_KNIGHT:
							class_ = JOB_KNIGHT2;
							break;
						case JOB_CRUSADER:
							class_ = JOB_CRUSADER2;
							break;
						case JOB_LORD_KNIGHT:
							class_ = JOB_LORD_KNIGHT2;
							break;
						case JOB_PALADIN:
							class_ = JOB_PALADIN2;
							break;
						case JOB_BABY_KNIGHT:
							class_ = JOB_BABY_KNIGHT2;
							break;
						case JOB_BABY_CRUSADER:
							class_ = JOB_BABY_CRUSADER2;
							break;
					}
				}
				sd->vd.class_ = class_;
				clif_get_weapon_view(sd, &sd->vd.weapon, &sd->vd.shield);
				sd->vd.head_top = sd->status.head_top;
				sd->vd.head_mid = sd->status.head_mid;
				sd->vd.head_bottom = sd->status.head_bottom;
				sd->vd.hair_style = cap_value(sd->status.hair, MIN_HAIR_STYLE, MAX_HAIR_STYLE);
				sd->vd.hair_color = cap_value(sd->status.hair_color, MIN_HAIR_COLOR, MAX_HAIR_COLOR);
				sd->vd.cloth_color = cap_value(sd->status.clothes_color, MIN_CLOTH_COLOR, MAX_CLOTH_COLOR);
				sd->vd.body_style = cap_value(sd->status.body, MIN_BODY_STYLE, MAX_BODY_STYLE);
				sd->vd.sex = sd->status.sex;

				if (sd->vd.cloth_color) {
					if(sd->sc.option&OPTION_WEDDING && battle_config.wedding_ignorepalette)
						sd->vd.cloth_color = 0;
					if(sd->sc.option&OPTION_XMAS && battle_config.xmas_ignorepalette)
						sd->vd.cloth_color = 0;
					if(sd->sc.option&(OPTION_SUMMER|OPTION_SUMMER2) && battle_config.summer_ignorepalette)
						sd->vd.cloth_color = 0;
					if(sd->sc.option&OPTION_HANBOK && battle_config.hanbok_ignorepalette)
						sd->vd.cloth_color = 0;
					if(sd->sc.option&OPTION_OKTOBERFEST && battle_config.oktoberfest_ignorepalette)
						sd->vd.cloth_color = 0;
				}
				if ( sd->vd.body_style && sd->sc.option&OPTION_COSTUME)
 					sd->vd.body_style = 0;
			} else if (vd)
				memcpy(&sd->vd, vd, sizeof(struct view_data));
			else
				ShowError("status_set_viewdata (PC): No view data for class %d\n", class_);
		}
	break;
	case BL_MOB:
		{
			TBL_MOB* md = (TBL_MOB*)bl;
			if (vd){
				mob_free_dynamic_viewdata( md );

				md->vd = vd;
			}else if( pcdb_checkid( class_ ) ){
				mob_set_dynamic_viewdata( md );

				md->vd->class_ = class_;
				md->vd->hair_style = cap_value(md->vd->hair_style, MIN_HAIR_STYLE, MAX_HAIR_STYLE);
				md->vd->hair_color = cap_value(md->vd->hair_color, MIN_HAIR_COLOR, MAX_HAIR_COLOR);
			}else
				ShowError("status_set_viewdata (MOB): No view data for class %d\n", class_);
		}
	break;
	case BL_PET:
		{
			TBL_PET* pd = (TBL_PET*)bl;
			if (vd) {
				memcpy(&pd->vd, vd, sizeof(struct view_data));
				if (!pcdb_checkid(vd->class_)) {
					pd->vd.hair_style = battle_config.pet_hair_style;
					if(pd->pet.equip) {
						pd->vd.head_bottom = itemdb_viewid(pd->pet.equip);
						if (!pd->vd.head_bottom)
							pd->vd.head_bottom = pd->pet.equip;
					}
				}
			} else
				ShowError("status_set_viewdata (PET): No view data for class %d\n", class_);
		}
	break;
	case BL_NPC:
		{
			TBL_NPC* nd = (TBL_NPC*)bl;
			if (vd)
				memcpy(&nd->vd, vd, sizeof(struct view_data));
			else if (pcdb_checkid(class_)) {
				memset(&nd->vd, 0, sizeof(struct view_data));
				nd->vd.class_ = class_;
				nd->vd.hair_style = cap_value(nd->vd.hair_style, MIN_HAIR_STYLE, MAX_HAIR_STYLE);
			} else {
				if (bl->m >= 0)
					ShowDebug("Source (NPC): %s at %s (%d,%d)\n", nd->name, map_mapid2mapname(bl->m), bl->x, bl->y);
				else
					ShowDebug("Source (NPC): %s (invisible/not on a map)\n", nd->name);
			}
			break;
		}
	break;
	
	
	case BL_ELEM:
		{
			struct elemental_data *ed = (struct elemental_data*)bl;
			if (vd)
				ed->vd = vd;
			else
				ShowError("status_set_viewdata (ELEMENTAL): No view data for class %d\n", class_);
		}
		break;
	}
}

/**
 * Get status change data of an object
 * @param bl: Object whose sc data to get [PC|MOB|HOM|MER|ELEM|NPC]
 * @return status change data structure bl->sc
 */
struct status_change *status_get_sc(struct block_list *bl)
{
	if( bl )
	switch (bl->type) {
		case BL_PC:  return &((TBL_PC*)bl)->sc;
		case BL_MOB: return &((TBL_MOB*)bl)->sc;
		case BL_NPC: return &((TBL_NPC*)bl)->sc;
		case BL_ELEM: return &((TBL_ELEM*)bl)->sc;
	}
	return NULL;
}

/**
 * Initiate (memset) the status change data of an object
 * @param bl: Object whose sc data to memset [PC|MOB|HOM|MER|ELEM|NPC]
 */
void status_change_init(struct block_list *bl)
{
	struct status_change *sc = status_get_sc(bl);
	nullpo_retv(sc);
	memset(sc, 0, sizeof (struct status_change));
}

/*========================================== [Playtester]
* Returns the interval for status changes that iterate multiple times
* through the timer (e.g. those that deal damage in regular intervals)
* @param type: Status change (SC_*)
*------------------------------------------*/
static int status_get_sc_interval(enum sc_type type)
{
	switch (type) {
		case STATUS_POISON:
		case STATUS_BLEEDING:
		case STATUS_DPOISON:
			return 1000;
		case STATUS_STONECURSE:
			return 5000;
		default:
			break;
	}
	return 0;
}

/**
 * Applies SC defense to a given status change
 * This function also determines whether or not the status change will be applied
 * @param src: Source of the status change [PC|MOB|HOM|MER|ELEM|NPC]
 * @param bl: Target of the status change
 * @param type: Status change (SC_*)
 * @param rate: Initial percentage rate of affecting bl (0~10000)
 * @param tick: Initial duration that the status change affects bl
 * @param flag: Value which determines what parts to calculate. See e_status_change_start_flags
 * @return adjusted duration based on flag values
 */
t_tick status_get_sc_def(struct block_list *src, struct block_list *bl, enum sc_type type, int rate, t_tick tick, unsigned char flag)
{
	/// Resistance rate: 10000 = 100%
	/// Example:	50% (5000) -> sc_def = 5000 -> 25%;
	///				5000ms -> tick_def = 5000 -> 2500ms
	int sc_def = 0, tick_def = -1; // -1 = use sc_def
	/// Fixed resistance value (after rate calculation)
	/// Example:	25% (2500) -> sc_def2 = 2000 -> 5%;
	///				2500ms -> tick_def2=2000 -> 500ms
	int sc_def2 = 0, tick_def2 = 0;

	struct status_data *status, *status_src;
	struct status_change *sc;
	struct map_session_data *sd;

	nullpo_ret(bl);
	if (src == NULL)
		return tick?tick:1; // This should not happen in current implementation, but leave it anyway

	// Skills (magic type) that are blocked by Golden Thief Bug card or Wand of Hermod
	if (status_isimmune(bl)) {
		std::shared_ptr<s_skill_db> skill = skill_db.find(battle_getcurrentskill(src));

		if (skill != nullptr && skill->skill_type == BF_MAGIC)
			return 0;
	}

	rate = cap_value(rate, 0, 10000);
	sd = BL_CAST(BL_PC,bl);
	status = status_get_status_data(bl);
	status_src = status_get_status_data(src);
	sc = status_get_sc(bl);
	if( sc && !sc->count )
		sc = NULL;

	switch (type) {
		case STATUS_POISON:
		case STATUS_DPOISON:
			sc_def = status->vit*100;

			break;
		case STATUS_STUN:
			sc_def = status->vit*100;
			sc_def2 = status->luk*10 + status_get_lv(bl)*10 - status_get_lv(src)*10;
			tick_def2 = status->luk*10;
			break;
		case STATUS_SLEEP:

			sc_def = status->agi*100;
			sc_def2 = (status->int_ + status->luk) * 5 + status_get_lv(bl)*10 - status_get_lv(src)*10;

			tick_def2 = status->luk*10;
			break;
		case STATUS_STONECURSE:
			sc_def = status->mdef*100;
			sc_def2 = status->luk*10 + status_get_lv(bl)*10 - status_get_lv(src)*10;
			tick_def = 0; // No duration reduction
			break;
		case STATUS_FREEZE:
			sc_def = status->mdef*100;
			sc_def2 = status->luk*10 + status_get_lv(bl)*10 - status_get_lv(src)*10;
			tick_def2 = status_src->luk*-10; // Caster can increase final duration with luk
			break;
		case STATUS_CURSE:
			// Special property: immunity when luk is zero
			if (status->luk == 0)
				return 0;
			sc_def = status->luk*100;
			sc_def2 = status->luk*10 - status_get_lv(src)*10; // Curse only has a level penalty and no resistance
			tick_def = status->vit*100;
			tick_def2 = status->luk*10;
			break;
		case STATUS_BLIND:
			sc_def = (status->vit + status->int_)*50;
			sc_def2 = status->luk*10 + status_get_lv(bl)*10 - status_get_lv(src)*10;
			tick_def2 = status->luk*10;
			break;
		case STATUS_CONFUSION:
			sc_def = (status->str + status->int_)*50;
			sc_def2 = status_get_lv(src)*10 - status_get_lv(bl)*10 - status->luk*10; // Reversed sc_def2
			tick_def2 = status->luk*10;
			break;
		case STATUS_DECREASEAGI:
			if (sd)
				tick >>= 1; // Half duration for players.
			sc_def2 = status->mdef*100;
			break;
		
		case STATUS_STASIS:
			// 10 second (fixed) + { Stasis Skill level * 10 - (Target's VIT + DEX) / 20 }
			tick_def2 = (status->vit + status->dex) * 50;
			break;
		case STATUS_MAGICCELL:
			if( tick == 5000 ) // 100% on caster
				break;
			if( bl->type == BL_PC )
				tick_def2 = status_get_lv(bl)*20 + status->vit*25 + status->agi*10;
			else
				tick_def2 = (status->vit + status->luk)*50;
			break;
		
		case STATUS_PARALYSIS:
			tick_def2 = (status->vit + status->luk)*50;
			break;
		
		case STATUS_NORECOVER_STATE:
			tick_def2 = status->luk * 100;
			break;
		default:
			// Effect that cannot be reduced? Likely a buff.
			if (!(rnd()%10000 < rate))
				return 0;
			return tick ? tick : 1;
	}

	if (sd) {
		if (battle_config.pc_STATUS_DEF_RATE != 100) {
			sc_def = sc_def*battle_config.pc_STATUS_DEF_RATE/100;
			sc_def2 = sc_def2*battle_config.pc_STATUS_DEF_RATE/100;
		}

		sc_def = min(sc_def, battle_config.pc_max_sc_def*100);
		sc_def2 = min(sc_def2, battle_config.pc_max_sc_def*100);

		if (battle_config.pc_STATUS_DEF_RATE != 100) {
			tick_def = tick_def*battle_config.pc_STATUS_DEF_RATE/100;
			tick_def2 = tick_def2*battle_config.pc_STATUS_DEF_RATE/100;
		}
	} else {
		if (battle_config.mob_STATUS_DEF_RATE != 100) {
			sc_def = sc_def*battle_config.mob_STATUS_DEF_RATE/100;
			sc_def2 = sc_def2*battle_config.mob_STATUS_DEF_RATE/100;
		}

		sc_def = min(sc_def, battle_config.mob_max_sc_def*100);
		sc_def2 = min(sc_def2, battle_config.mob_max_sc_def*100);

		if (battle_config.mob_STATUS_DEF_RATE != 100) {
			tick_def = tick_def*battle_config.mob_STATUS_DEF_RATE/100;
			tick_def2 = tick_def2*battle_config.mob_STATUS_DEF_RATE/100;
		}
	}



	// When tick def not set, reduction is the same for both.
	if(tick_def == -1)
		tick_def = sc_def;

	// Natural resistance
	if (!(flag&SCSTART_NORATEDEF)) {
		rate -= rate*sc_def/10000;
		rate -= sc_def2;



		// Item resistance (only applies to rate%)
		if (sd) {
			for (const auto &it : sd->reseff) {
				if (it.id == type)
					rate -= rate * it.val / 10000;
			}
			
		}

		// Aegis accuracy
		if(rate > 0 && rate%10 != 0) rate += (10 - rate%10);
	}

	if (!(rnd()%10000 < rate))
		return 0;

	// Duration cannot be reduced
	if (flag&SCSTART_NOTICKDEF)
		return i64max(tick, 1);

	tick -= tick*tick_def/10000;
	tick -= tick_def2;

	// Minimum durations
	switch (type) {

		case STATUS_STASIS:
			tick = i64max(tick, 10000); // Minimum duration 10s
			break;
		default:
			// Skills need to trigger even if the duration is reduced below 1ms
			tick = i64max(tick, 1);
			break;
	}

	return tick;
}

/**
 * Applies SC effect
 * @param bl: Source to apply effect
 * @param type: Status change (SC_*)
 * @param dval1~3: Depends on type of status change
 * Author: Ind
 */
void status_display_add(struct block_list *bl, enum sc_type type, int dval1, int dval2, int dval3) {
	struct eri *eri;
	struct sc_display_entry **sc_display;
	struct sc_display_entry ***sc_display_ptr;
	struct sc_display_entry *entry;
	int i;
	unsigned char sc_display_count;
	unsigned char *sc_display_count_ptr;

	nullpo_retv(bl);

	switch( bl->type ){
		case BL_PC: {
			struct map_session_data* sd = (struct map_session_data*)bl;

			sc_display_ptr = &sd->sc_display;
			sc_display_count_ptr = &sd->sc_display_count;
			eri = pc_sc_display_ers;
			}
			break;
		case BL_NPC: {
			struct npc_data* nd = (struct npc_data*)bl;

			sc_display_ptr = &nd->sc_display;
			sc_display_count_ptr = &nd->sc_display_count;
			eri = npc_sc_display_ers;
			}
			break;
		default:
			return;
	}

	sc_display = *sc_display_ptr;
	sc_display_count = *sc_display_count_ptr;

	ARR_FIND(0, sc_display_count, i, sc_display[i]->type == type);

	if( i != sc_display_count ) {
		sc_display[i]->val1 = dval1;
		sc_display[i]->val2 = dval2;
		sc_display[i]->val3 = dval3;
		return;
	}

	entry = ers_alloc(eri, struct sc_display_entry);

	entry->type = type;
	entry->val1 = dval1;
	entry->val2 = dval2;
	entry->val3 = dval3;

	RECREATE(sc_display, struct sc_display_entry *, ++sc_display_count);
	sc_display[sc_display_count - 1] = entry;
	*sc_display_ptr = sc_display;
	*sc_display_count_ptr = sc_display_count;
}

/**
 * Removes SC effect
 * @param bl: Source to remove effect
 * @param type: Status change (SC_*)
 * Author: Ind
 */
void status_display_remove(struct block_list *bl, enum sc_type type) {
	struct eri *eri;
	struct sc_display_entry **sc_display;
	struct sc_display_entry ***sc_display_ptr;
	int i;
	unsigned char sc_display_count;
	unsigned char *sc_display_count_ptr;

	nullpo_retv(bl);

	switch( bl->type ){
		case BL_PC: {
			struct map_session_data* sd = (struct map_session_data*)bl;

			sc_display_ptr = &sd->sc_display;
			sc_display_count_ptr = &sd->sc_display_count;
			eri = pc_sc_display_ers;
			}
			break;
		case BL_NPC: {
			struct npc_data* nd = (struct npc_data*)bl;

			sc_display_ptr = &nd->sc_display;
			sc_display_count_ptr = &nd->sc_display_count;
			eri = npc_sc_display_ers;
			}
			break;
		default:
			return;
	}

	sc_display = *sc_display_ptr;
	sc_display_count = *sc_display_count_ptr;

	ARR_FIND(0, sc_display_count, i, sc_display[i]->type == type);

	if( i != sc_display_count ) {
		int cursor;

		ers_free(eri, sc_display[i]);
		sc_display[i] = NULL;

		/* The all-mighty compact-o-matic */
		for( i = 0, cursor = 0; i < sc_display_count; i++ ) {
			if( sc_display[i] == NULL )
				continue;

			if( i != cursor )
				sc_display[cursor] = sc_display[i];

			cursor++;
		}

		if( !(sc_display_count = cursor) ) {
			aFree(sc_display);
			sc_display = NULL;
		}

		*sc_display_ptr = sc_display;
		*sc_display_count_ptr = sc_display_count;
	}
}

/**
 * Applies SC defense to a given status change
 * This function also determines whether or not the status change will be applied
 * @param src: Source of the status change [PC|MOB|HOM|MER|ELEM|NPC]
 * @param bl: Target of the status change (See: enum sc_type)
 * @param type: Status change (SC_*)
 * @param rate: Initial percentage rate of affecting bl (0~10000)
 * @param val1~4: Depends on type of status change
 * @param tick: Initial duration that the status change affects bl
 * @param flag: Value which determines what parts to calculate. See e_status_change_start_flags
 * @return adjusted duration based on flag values
 */
int status_change_start(struct block_list* src, struct block_list* bl,enum sc_type type,int rate,int val1,int val2,int val3,int val4,t_tick duration,unsigned char flag) {
	struct map_session_data *sd = NULL;
	struct status_change* sc;
	struct status_change_entry* sce;
	struct status_data *status;
	struct view_data *vd;
	int opt_flag, calc_flag, undead_flag, val_flag = 0, tick_time = 0;
	bool sc_isnew = true;

	nullpo_ret(bl);
	sc = status_get_sc(bl);
	status = status_get_status_data(bl);

	if( type <= STATUS_NONE || type >= SC_MAX ) {
		ShowError("status_change_start: invalid status change (%d)!\n", type);
		return 0;
	}

	if( !sc )
		return 0; // Unable to receive status changes

	if( bl->type != BL_NPC && status_isdead(bl) && ( type != STATUS_NOCHAT ) ) // STATUS_NOCHAT and SC_JAILED should work even on dead characters
		return 0;

	if (status_change_isDisabledOnMap(type, map_getmapdata(bl->m)))
		return 0;

	
	if( sc->data[STATUS_MILLENIUMSHIELDS] ) {
		if( type >= STATUS_COMMON_MIN && type <= STATUS_COMMON_MAX )
			return 0; // Immune to status ailments
	}

	// Statuses from Merchant family skills that can be blocked while using Madogear; see pc.cpp::pc_setoption for cancellation
	if (sc->option & OPTION_MADOGEAR) {
		for (const auto &madosc : mado_statuses) {
			if (type != madosc)
				continue;

			uint16 skill_id = status_sc2skill(type);

			if (skill_id > 0 && !skill_get_inf2(skill_id, INF2_ALLOWONMADO))
				return 0;
		}
	}

	// Adjust tick according to status resistances
	if( !(flag&(SCSTART_NOAVOID|SCSTART_LOADED)) ) {
		duration = status_get_sc_def(src, bl, type, rate, duration, flag);
		if( !duration )
			return 0;
	}

	int tick = (int)duration;

	sd = BL_CAST(BL_PC, bl);
	vd = status_get_viewdata(bl);

	undead_flag = battle_check_undead(status->race,status->def_ele);
	// Check for immunities / sc fails
	switch (type) {
	case STATUS_DECREASEAGI:
	case STATUS_QUAGMIRE:
		break;
	case STATUS_STONECURSE:
		// Undead are immune to Stone
		if (undead_flag && !(flag&SCSTART_NOAVOID))
			return 0;
		if (sc->opt1)
			return 0; //Cannot override other OPT1 status changes [Skotlex]
		break;
	case STATUS_FREEZE:
		// Undead are immune to Freeze
		if (undead_flag && !(flag&SCSTART_NOAVOID))
			return 0;
		if (sc->opt1)
			return 0; // Cannot override other opt1 status changes. [Skotlex]
		break;
	case STATUS_SLEEP:
		if (sc->opt1)
			return 0; // Cannot override other opt1 status changes. [Skotlex]
		break;
	case STATUS_STUN:
		if (sc->opt1)
			return 0; // Cannot override other opt1 status changes. [Skotlex]
		break;
	case STATUS_BLIND:
		break;
	case STATUS_CURSE:
		break;
	case STATUS_SILENCE:
		break;
	case STATUS_ALL_RIDING:
		if( !sd || sc->option&(OPTION_RIDING|OPTION_DRAGON|OPTION_WUGRIDER|OPTION_MADOGEAR) )
			return 0;
		if( sc->data[type] )
		{	// Already mounted, just dismount.
			status_change_end(bl, STATUS_ALL_RIDING, INVALID_TIMER);
			return 0;
		}
		break;
	// Fall through
	case STATUS_MAGICCELL:
		if (sc->opt1)
			return 0; //Cannot override other OPT1 status changes [Skotlex]
		break;
	case STATUS_LEXAETERNA:
		if( (sc->data[STATUS_STONECURSE] && sc->opt1 == OPT1_STONE) || sc->data[STATUS_FREEZE] )
			return 0;
	break;
	case STATUS_KYRIE:
		if (bl->type == BL_MOB)
			return 0;
	break;
	case STATUS_WEAPONRYEXPERTISE:
	case STATUS_ADRENALINERUSH:
		if (sc->data[STATUS_QUAGMIRE] || sc->data[STATUS_DECREASEAGI])
			return 0;
	break;
	case STATUS_MAGNIFICAT:
		if( sc->option&OPTION_MADOGEAR ) // Mado is immune to magnificat
			return 0;
		break;
		if(sc->data[STATUS_DECREASEAGI])
			return 0;
	case STATUS_INCREASEAGI:
	case STATUS_IMPROVECONCENTRATION:

	case STATUS_WINDRACER:
	case STATUS_IMPRESSIVERIFF:
		if (sc->option&OPTION_MADOGEAR)
			return 0; // Mado is immune to the above [Ind]
	case STATUS_CARTBOOST:
		if (sc->data[STATUS_QUAGMIRE])
			return 0;
	break;
	case STATUS_CLOAKING:
		// Avoid cloaking with no wall and low skill level. [Skotlex]
		// Due to the cloaking card, we have to check the wall versus to known
		// skill level rather than the used one. [Skotlex]
		// if (sd && val1 < 3 && skill_check_cloaking(bl,NULL))
		if( sd && pc_checkskill(sd, SK_AS_CLOAKING) < 3 && !skill_check_cloaking(bl,NULL) )
			return 0;
	break;
	
	// Strip skills, need to divest something or it fails.
	case STATUS_STRIPWEAPON:
		if (val2 == 1)
			val2 = 0; // Brandish Spear/Bowling Bash effet. Do not take weapon off.
		else if (sd && !(flag&SCSTART_LOADED)) { // Apply sc anyway if loading saved sc_data
			short i;
			opt_flag = 0; // Reuse to check success condition.
			if(sd->bonus.unstripable_equip&EQP_WEAPON)
				return 0;
			i = sd->equip_index[EQI_HAND_L];
			if (i>=0 && sd->inventory_data[i] && sd->inventory_data[i]->type == IT_WEAPON) {
				opt_flag|=1;
				pc_unequipitem(sd,i,3); // Left-hand weapon
			}

			i = sd->equip_index[EQI_HAND_R];
			if (i>=0 && sd->inventory_data[i] && sd->inventory_data[i]->type == IT_WEAPON) {
				opt_flag|=2;
				pc_unequipitem(sd,i,3);
			}
			if (!opt_flag) return 0;
		}
		if (tick == 1) return 1; // Minimal duration: Only strip without causing the SC
	break;
	case STATUS_STRIPSHIELD:
		if( val2 == 1 ) val2 = 0; // GX effect. Do not take shield off..
		else
		if (sd && !(flag&SCSTART_LOADED)) {
			short i;
			if(sd->bonus.unstripable_equip&EQP_SHIELD)
				return 0;
			i = sd->equip_index[EQI_HAND_L];
			if ( i < 0 || !sd->inventory_data[i] || sd->inventory_data[i]->type != IT_ARMOR )
				return 0;
			pc_unequipitem(sd,i,3);
		}
		if (tick == 1) return 1; // Minimal duration: Only strip without causing the SC
	break;
	case STATUS_STRIPARMOR:
		if (sd && !(flag&SCSTART_LOADED)) {
			short i;
			if(sd->bonus.unstripable_equip&EQP_ARMOR)
				return 0;
			i = sd->equip_index[EQI_ARMOR];
			if ( i < 0 || !sd->inventory_data[i] )
				return 0;
			pc_unequipitem(sd,i,3);
		}
		if (tick == 1) return 1; // Minimal duration: Only strip without causing the SC
	break;
	case STATUS_STRIPHELM:
		if (sd && !(flag&SCSTART_LOADED)) {
			short i;
			if(sd->bonus.unstripable_equip&EQP_HELM)
				return 0;
			i = sd->equip_index[EQI_HEAD_TOP];
			if ( i < 0 || !sd->inventory_data[i] )
				return 0;
			pc_unequipitem(sd,i,3);
		}
		if (tick == 1) return 1; // Minimal duration: Only strip without causing the SC
	break;
	
	break;
	case STATUS_STRFOOD:
	
	break;
	case STATUS_AGIFOOD:
	
	break;
	case STATUS_VITFOOD:
		
	break;
	case STATUS_INTFOOD:
		
	break;
	case STATUS_DEXFOOD:
		
	break;
	case STATUS_LUKFOOD:
		
	break;
	case STATUS_CAMOUFLAGE:
		if( sd && pc_checkskill(sd, SK_HT_CAMOUFLAGE) < 3 && !skill_check_camouflage(bl,NULL) )
			return 0;
	break;
	case STATUS_STRIPACCESSORY:
		if( sd ) {
			short i = -1;
			if( !(sd->bonus.unstripable_equip&EQP_ACC_L) ) {
				i = sd->equip_index[EQI_ACC_L];
				if( i >= 0 && sd->inventory_data[i] && sd->inventory_data[i]->type == IT_ARMOR )
					pc_unequipitem(sd,i,3); // Left-Accessory
			}
			if( !(sd->bonus.unstripable_equip&EQP_ACC_R) ) {
				i = sd->equip_index[EQI_ACC_R];
				if( i >= 0 && sd->inventory_data[i] && sd->inventory_data[i]->type == IT_ARMOR )
					pc_unequipitem(sd,i,3); // Right-Accessory
			}
			if( i < 0 )
				return 0;
		}
		if (tick == 1) return 1; // Minimal duration: Only strip without causing the SC
	break;
	case STATUS_MILLENIUMSHIELDS:
		if (sc->data[STATUS_SWORNPROTECTOR] || sc->data[STATUS_MAGICCELL])
			return 0;
		break;
	}

	// Check for resistances
	if(status_has_mode(status,MD_STATUSIMMUNE) && !(flag&SCSTART_NOAVOID)) {
		if (type>=STATUS_COMMON_MIN && type <= STATUS_COMMON_MAX)
			return 0;
		switch (type) {
			case STATUS_BLESSING:
			case STATUS_DECREASEAGI:
			case STATUS_PROVOKE:
			case STATUS_COMA:
			case STATUS_STRIPWEAPON:
			case STATUS_STRIPARMOR:
			case STATUS_STRIPSHIELD:
			case STATUS_STRIPHELM:
			case STATUS_ACCOUSTICRYTHM:
			case STATUS_MAGICCELL:
			case STATUS_PARALYSIS:
				return 0;
		}
	}
	// Check for mvp resistance // atm only those who OS
	if(status_has_mode(status,MD_MVP) && !(flag&SCSTART_NOAVOID)) {
		 switch (type) {
		 case STATUS_COMA:
		// continue list...
		     return 0;
		}
	}

	// Before overlapping fail, one must check for status cured.
	switch (type) {
	case STATUS_BLESSING:
		// !TODO: Blessing and Agi up should do 1 damage against players on Undead Status, even on PvM
		// !but cannot be plagiarized (this requires aegis investigation on packets and official behavior) [Brainstorm]
		if ((!undead_flag && status->race != RC_DEMON) || bl->type == BL_PC) {
			if (sc->data[STATUS_STONECURSE] && sc->opt1 == OPT1_STONE)
				status_change_end(bl, STATUS_STONECURSE, INVALID_TIMER);
			if (sc->data[STATUS_CURSE]) {
					status_change_end(bl, STATUS_CURSE, INVALID_TIMER);
					return 1; // End Curse and do not give stat boost
			}
		}
		break;
	case STATUS_INCREASEAGI:
		status_change_end(bl, STATUS_DECREASEAGI, INVALID_TIMER);
		/*if(sc->data[SC_SPIRIT] && sc->data[SC_SPIRIT]->val2 == SL_HIGH)
			status_change_end(bl, SC_SPIRIT, INVALID_TIMER);*/
		break;
	case STATUS_QUAGMIRE:
		status_change_end(bl, STATUS_LOUDEXCLAMATION, INVALID_TIMER);
		status_change_end(bl, STATUS_IMPROVECONCENTRATION, INVALID_TIMER);
		status_change_end(bl, STATUS_WINDRACER, INVALID_TIMER);
		// Also blocks the ones below...
	case STATUS_DECREASEAGI:
		if (type == STATUS_DECREASEAGI) {
			status_change_end(bl, STATUS_DECREASEAGI, INVALID_TIMER);
		}
		status_change_end(bl, STATUS_CARTBOOST, INVALID_TIMER);
		// Also blocks the ones below...
		status_change_end(bl, STATUS_INCREASEAGI, INVALID_TIMER);
		status_change_end(bl, STATUS_WEAPONRYEXPERTISE, INVALID_TIMER);
		break;
	case STATUS_BATTLEDRUMS:
	case STATUS_PAGANPARTY:
		status_change_end(bl, STATUS_BATTLEDRUMS, INVALID_TIMER);
		status_change_end(bl, STATUS_PAGANPARTY, INVALID_TIMER);
		break;
	case STATUS_IMPRESSIVERIFF:
	case STATUS_MAGICSTRINGS:
	case STATUS_ACCOUSTICRYTHM:
		status_change_end(bl, STATUS_IMPRESSIVERIFF, INVALID_TIMER);
		status_change_end(bl, STATUS_MAGICSTRINGS, INVALID_TIMER);
		status_change_end(bl, STATUS_ACCOUSTICRYTHM, INVALID_TIMER);
		break;
	
	
	case STATUS_OFFERTORIUM:
		status_change_end(bl, STATUS_BLIND, INVALID_TIMER);
		status_change_end(bl, STATUS_CURSE, INVALID_TIMER);
		status_change_end(bl, STATUS_POISON, INVALID_TIMER);
		status_change_end(bl, STATUS_CONFUSION, INVALID_TIMER);
		status_change_end(bl, STATUS_BLEEDING, INVALID_TIMER);
		break;
	case STATUS_SILENCE:
		if (sc->data[STATUS_GOSPEL] && sc->data[STATUS_GOSPEL]->val4 == BCT_SELF)
			status_change_end(bl, STATUS_GOSPEL, INVALID_TIMER);
		break;
	case STATUS_FREEZE:
		status_change_end(bl, STATUS_LEXAETERNA, INVALID_TIMER);
		break;
	case STATUS_HIDING:
		status_change_end(bl, STATUS_CLOSECONFINE, INVALID_TIMER);
		status_change_end(bl, STATUS_CLOSECONFINE2, INVALID_TIMER);
		break;
	case STATUS_ASSUMPTIO:
		break;
	case STATUS_CARTBOOST:
		if(sc->data[STATUS_DECREASEAGI]) { // Cancel Decrease Agi, but take no further effect [Skotlex]
			status_change_end(bl, STATUS_DECREASEAGI, INVALID_TIMER);
			return 0;
		}
		break;
	
	case STATUS_STRFOOD:
		break;
	case STATUS_AGIFOOD:
		break;
	case STATUS_VITFOOD:
		break;
	case STATUS_INTFOOD:
		break;
	case STATUS_DEXFOOD:
		break;
	case STATUS_LUKFOOD:
		break;
	
	
	case STATUS_WINDMILLPOEM:
	case STATUS_ECHONOCTURNAE:
		if (type != STATUS_WINDMILLPOEM) status_change_end(bl, STATUS_WINDMILLPOEM, INVALID_TIMER);
		if (type != STATUS_ECHONOCTURNAE) status_change_end(bl, STATUS_ECHONOCTURNAE, INVALID_TIMER);
		break;
	case STATUS_REFLECTSHIELD:
		break;

	case STATUS_MAGICCELL:
		status_change_end(bl, STATUS_FREEZE, INVALID_TIMER);
		status_change_end(bl, STATUS_STONECURSE, INVALID_TIMER);
		break;
	
	case STATUS_MYSTICALAMPLIFICATION:
	case STATUS_IMPOSITIOMANUS:
			status_change_end(bl, type, INVALID_TIMER);
		break;
	case STATUS_ENDURE:
		if (sd && sd->special_state.no_walk_delay)
			return 1;
		break;
	
	}

	// Check for overlapping fails
	if( (sce = sc->data[type]) ) {
		switch( type ) {
			
			case STATUS_WEAPONRYEXPERTISE:
			case STATUS_WEAPONSHARPENING:
			case STATUS_POWERTHRUST:
			case STATUS_ADRENALINERUSH:
				if (sce->val2 > val2)
					return 0;
				break;
			case STATUS_STUN:
			case STATUS_SLEEP:
			case STATUS_POISON:
			case STATUS_CURSE:
			case STATUS_SILENCE:
			case STATUS_CONFUSION:
			case STATUS_BLIND:
			case STATUS_BLEEDING:
			case STATUS_DPOISON:
			case STATUS_CLOSECONFINE2: // Can't be re-closed in.
			case STATUS_MARIONETTE:
			case STATUS_MARIONETTE2:
			case STATUS_NOCHAT:
			case STATUS_MAGICCELL:
			case STATUS_ALL_RIDING_REUSE_LIMIT:
				return 0;
			case STATUS_PUSHCART:
			case STATUS_SWORNPROTECTOR:
			case STATUS_ASPDPOTION0:
			case STATUS_ASPDPOTION1:
			case STATUS_ASPDPOTION2:
			case STATUS_ASPDPOTION3:
				break;
			case STATUS_ENDURE:
				if(sce->val4 && !val4)
					return 1; // Don't let you override infinite endure.
				if(sce->val1 > val1)
					return 1;
				break;
			
			default:
				if(sce->val1 > val1)
					return 1; // Return true to not mess up skill animations. [Skotlex]
		}
	}

	calc_flag = StatusChangeFlagTable[type];
	if(!(flag&SCSTART_LOADED)) // &4 - Do not parse val settings when loading SCs
	switch(type)
	{
		/* Permanent effects */
		case STATUS_LEXAETERNA:
		case STATUS_WEIGHT50:
		case STATUS_WEIGHT90:
		case STATUS_PUSHCART:
		case STATUS_CLAN_INFO:
			tick = INFINITE_TICK;
			break;
		case STATUS_DECREASEAGI:
		case STATUS_INCREASEAGI:
			val2 = 2 + val1; // Agi change
			break;
		case STATUS_ENDURE:
			val2 = 2147483647; // Hit-count [Celest]
			if( !(flag&SCSTART_NOAVOID) && (bl->type&(BL_PC|BL_MER)) && !map_flag_gvg2(bl->m) && !map_getmapflag(bl->m, MF_BATTLEGROUND) && !val4 ) {
				struct map_session_data *tsd;
				if( sd ) {
					int i;
					for( i = 0; i < MAX_DEVOTION; i++ ) {
						if( sd->devotion[i] && (tsd = map_id2sd(sd->devotion[i])) )
							status_change_start(src,&tsd->bl, type, 10000, val1, val2, val3, val4, tick, SCSTART_NOAVOID|SCSTART_NOICON);
					}
				}
				
			}
			if( val4 )
				tick = INFINITE_TICK;
			break;
		case STATUS_ANGER:
			if (status->hp < status->max_hp/2 &&
				(!sc->data[STATUS_PROVOKE] || sc->data[STATUS_PROVOKE]->val2==0))
					sc_start4(src,bl,STATUS_PROVOKE,100,sc->data[STATUS_ANGER]->val1,1,0,0,60000);
			tick = INFINITE_TICK;
			break;
		case STATUS_MAXIMIZEPOWER:
			tick_time = val2 = tick>0?tick:60000;
			tick = INFINITE_TICK; // Duration sent to the client should be infinite
			break;
		case STATUS_ENCHANTDEADLYPOISON:
			val2 = val1 * 3; // Chance to Poison enemies.
			break;
		case STATUS_POISONREACT:
			val2=2*val1; // number of counters
			val3=10*val1; // Chance to counter. [Skotlex]
			break;
		case STATUS_KYRIE:
			val2 = status->max_hp * (val1 * 4 + 10) / 100;
			val3 = (val1 * 4);
			break;
		case STATUS_MYSTICALAMPLIFICATION:
			// val1: Skill lv
			val2 = 1; // Lasts 1 invocation
			val3 = 10 * val1; // Matk% increase
			val4 = 0; // 0 = ready to be used, 1 = activated and running
			break;
		case STATUS_ZEPHYRSNIPING:
			val2 = 999999; // Lasts 99999 hits
			tick = INFINITE_TICK;
			break;
		case STATUS_LIVINGTORNADO:
		case STATUS_RECKONING:
			val2 = 5; // Lasts 5 hits
			tick = INFINITE_TICK;
			break;
		case STATUS_ENCHANTPOISON:
			val2= 500*val1; // Poisoning Chance
		case STATUS_ASPERSIO:
			skill_enchant_elemental_end(bl,type);
			break;
		case STATUS_REFLECTSHIELD:
			val2 = 10+val1*3; // %Dmg reflected
			// val4 used to mark if reflect shield is an inheritance bonus from Devotion
			if( !(flag&SCSTART_NOAVOID) && (bl->type&(BL_PC|BL_MER)) ) {
				struct map_session_data *tsd;
				if( sd ) {
					int i;
					for( i = 0; i < MAX_DEVOTION; i++ ) {
						if( sd->devotion[i] && (tsd = map_id2sd(sd->devotion[i])) )
							status_change_start(src,&tsd->bl, type, 10000, val1, val2, 0, 1, tick, SCSTART_NOAVOID|SCSTART_NOICON);
					}
				}
			}
			break;
		case STATUS_STRIPWEAPON:
			if (!sd) // Watk reduction
				val2 = 40;
			break;
		case STATUS_STRIPSHIELD:
			if (!sd) // Def reduction
				val2 = 40;
			break;
		case STATUS_AUTOSPELL:
			// Val1 Skill LV of Autospell
			// Val2 Skill ID to cast
			// Val3 Max Lv to cast
			val4 = val1 * 3; // Chance of casting
			break;
		case STATUS_IMPRESSIVERIFF:
		case STATUS_ACCOUSTICRYTHM:
		case STATUS_MAGICSTRINGS:
			val2 = 2 * val1; // Status increases
			val3 = 6 * val1; // Attrs increase /reductions
			break;
		case STATUS_BATTLEDRUMS:
			val2 = val1 * 10; 
			break;
	
		case STATUS_FURY:
			val2 = 75 + 25*val1; // Cri bonus
			break;
		case STATUS_ULTRAINSTINCT:
			val2 = 2*val1; // stat bonus
			val3 = 8*val1; // atk bonus
			break;

		case STATUS_ASPDPOTION0:
		case STATUS_ASPDPOTION1:
		case STATUS_ASPDPOTION2:
		case STATUS_ASPDPOTION3:
			val2 = 50*(2+type-STATUS_ASPDPOTION0);
			break;

		case STATUS_NOCHAT:
			// A hardcoded interval of 60 seconds is expected, as the time that STATUS_NOCHAT uses is defined by
			// mmocharstatus.manner, each negative point results in 1 minute with this status activated.
			// This is done this way because the message that the client displays is hardcoded, and only
			// shows how many minutes are remaining. [Panikon]
			tick = 60000;
			val1 = battle_config.manner_system; // Mute filters.
			if (sd) {
				clif_changestatus(sd,SP_MANNER,sd->status.manner);
				clif_updatestatus(sd,SP_MANNER);
			}
			break;

		case STATUS_STONECURSE:
			val3 = max(val3, 100); // Incubation time
			val4 = max(tick-val3, 100); // Petrify time
			tick = val3;
			calc_flag = 0; // Actual status changes take effect on petrified state.
			break;

		case STATUS_DPOISON:
			// Lose 10/15% of your life as long as it doesn't brings life below 25%
			if (status->hp > status->max_hp>>2) {
				int diff = status->max_hp*(bl->type==BL_PC?10:15)/100;
				if (status->hp - diff < status->max_hp>>2)
					diff = status->hp - (status->max_hp>>2);
				status_zap(bl, diff, 0);
			}
			// Fall through
		case STATUS_POISON:
		case STATUS_BLEEDING:
			tick_time = status_get_sc_interval(type);
			val4 = tick - tick_time; // Remaining time
			break;
		

		case STATUS_CONFUSION:
			if (!val4)
				clif_emotion(bl,ET_QUESTION);
			break;
		
		case STATUS_HIDING:
			val2 = tick/1000;
			tick_time = 1000; // [GodLesZ] tick time
			val3 = 0; // Unused, previously speed adjustment
			val4 = val1+3; // Seconds before SP substraction happen.
			break;
		case STATUS_CHASEWALK:
			val2 = tick>0?tick:10000; // Interval at which SP is drained.
			val3 = 35 - 5 * val1; // Speed adjustment.
			val4 = 10+val1*2; // SP cost.
			if (map_flag_gvg2(bl->m) || map_getmapflag(bl->m, MF_BATTLEGROUND)) val4 *= 5;
			break;
		case STATUS_CLOAKING:
			if (!sd) // Monsters should be able to walk with no penalties. [Skotlex]
				val1 = 10;
			tick_time = val2 = tick>0?tick:60000; // SP consumption rate.
			tick = INFINITE_TICK; // Duration sent to the client should be infinite
			val3 = 0; // Unused, previously walk speed adjustment
			// val4&1 signals the presence of a wall.
			// val4&2 makes cloak not end on normal attacks [Skotlex]
			// val4&4 makes cloak not end on using skills
			if (bl->type == BL_PC || (bl->type == BL_MOB && ((TBL_MOB*)bl)->special_state.clone) )	// Standard cloaking.
				val4 |= battle_config.pc_cloak_check_type&7;
			else
				val4 |= battle_config.monster_cloak_check_type&7;
			break;
		case STATUS_RUWACH:
			val3 = skill_get_splash(val2, val1); // Val2 should bring the skill-id.
			val2 = tick/20;
			tick_time = 20; // [GodLesZ] tick time
			break;

		case STATUS_AUTOGUARD:
			if( !(flag&SCSTART_NOAVOID) ) {
				struct map_session_data *tsd;
				int i;
				for( i = val2 = 0; i < val1; i++) {
					int t = 5-(i>>1);
					val2 += (t < 0)? 1:t;
				}

				if( bl->type&(BL_PC|BL_MER) ) {
					if( sd ) {
						for( i = 0; i < MAX_DEVOTION; i++ ) {
							if( sd->devotion[i] && (tsd = map_id2sd(sd->devotion[i])) )
								status_change_start(src,&tsd->bl, type, 10000, val1, val2, 0, 0, tick, SCSTART_NOAVOID|SCSTART_NOICON);
						}
					}
				}
			}
			break;

		case STATUS_DEFENDINGAURA:
			if (!(flag&SCSTART_NOAVOID)) {
				val2 = 5 + 15*val1; // Damage reduction
				val3 = 0; // Unused, previously speed adjustment
				val4 = 450 - 50*val1; // Aspd adjustment

				

				if (sd) {
					struct map_session_data *tsd;
					int i;
					for (i = 0; i < MAX_DEVOTION; i++) { // See if there are devoted characters, and pass the status to them. [Skotlex]
						if (sd->devotion[i] && (tsd = map_id2sd(sd->devotion[i])))
							status_change_start(src,&tsd->bl,type,10000,val1,val2,val3,val4,tick,SCSTART_NOAVOID);
					}
				}
			}
			break;

		case STATUS_PARRY:
		    val2 = val1*8; // Block Chance
			break;

		case STATUS_FRENZY:
			
			// HP healing is performing after the calc_status call.
			// Val2 holds HP penalty
			if (!val4) val4 = skill_get_time2(status_sc2skill(type),val1);
			if (!val4) val4 = 10000; // Val4 holds damage interval
			val3 = tick/val4; // val3 holds skill duration
			tick_time = val4; // [GodLesZ] tick time
			break;

		case STATUS_MARIONETTE:
		{
			int stat;
			float stats_to_pass = pc_checkskill(sd, SK_CL_MARIONETTECONTROL) * 0.1;
			val3 = 0;
			val4 = 0;
			stat = ( sd ? sd->status.str : status_get_base_status(bl)->str ) * stats_to_pass; val3 |= cap_value(stat,0,0xFF)<<16;
			stat = ( sd ? sd->status.agi : status_get_base_status(bl)->agi ) * stats_to_pass; val3 |= cap_value(stat,0,0xFF)<<8;
			stat = ( sd ? sd->status.vit : status_get_base_status(bl)->vit ) * stats_to_pass; val3 |= cap_value(stat,0,0xFF);
			stat = ( sd ? sd->status.int_: status_get_base_status(bl)->int_) * stats_to_pass; val4 |= cap_value(stat,0,0xFF)<<16;
			stat = ( sd ? sd->status.dex : status_get_base_status(bl)->dex ) * stats_to_pass; val4 |= cap_value(stat,0,0xFF)<<8;
			stat = ( sd ? sd->status.luk : status_get_base_status(bl)->luk ) * stats_to_pass; val4 |= cap_value(stat,0,0xFF);
			break;
		}
		case STATUS_MARIONETTE2:
		{
			int stat,max_stat;
			// Fetch caster information
			struct block_list *pbl = map_id2bl(val1);
			struct status_change *psc = pbl?status_get_sc(pbl):NULL;
			struct status_change_entry *psce = psc?psc->data[STATUS_MARIONETTE]:NULL;
			// Fetch target's stats
			struct status_data* status2 = status_get_status_data(bl); // Battle status

			if (!psce)
				return 0;

			val3 = 0;
			val4 = 0;
			max_stat = 1000; // Cap to 1000 (default)
			stat = (psce->val3 >>16)&0xFF; stat = min(stat, max_stat - status2->str ); val3 |= cap_value(stat,0,0xFF)<<16;
			stat = (psce->val3 >> 8)&0xFF; stat = min(stat, max_stat - status2->agi ); val3 |= cap_value(stat,0,0xFF)<<8;
			stat = (psce->val3 >> 0)&0xFF; stat = min(stat, max_stat - status2->vit ); val3 |= cap_value(stat,0,0xFF);
			stat = (psce->val4 >>16)&0xFF; stat = min(stat, max_stat - status2->int_); val4 |= cap_value(stat,0,0xFF)<<16;
			stat = (psce->val4 >> 8)&0xFF; stat = min(stat, max_stat - status2->dex ); val4 |= cap_value(stat,0,0xFF)<<8;
			stat = (psce->val4 >> 0)&0xFF; stat = min(stat, max_stat - status2->luk ); val4 |= cap_value(stat,0,0xFF);
			break;
		}
		
		case STATUS_KILLERINSTINCT:
			val2 = 75 + 25*val1; //crit
			val3 = 8*val1; // Atk
			// tick = INFINITE_TICK;
			break;

		case STATUS_FORESIGHT:
			val2 = val1*2; // Memorized casts.
			tick = INFINITE_TICK;
			break;

		case STATUS_SWORNPROTECTOR:
		{
			struct block_list *d_bl;
			struct status_change *d_sc;

			if( (d_bl = map_id2bl(val1)) && (d_sc = status_get_sc(d_bl)) && d_sc->count ) { // Inherits Status From Source
				const enum sc_type types[] = { STATUS_AUTOGUARD, STATUS_DEFENDINGAURA, STATUS_REFLECTSHIELD, STATUS_ENDURE };
				int i = (map_flag_gvg2(bl->m) || map_getmapflag(bl->m, MF_BATTLEGROUND))?2:3;
				while( i >= 0 ) {
					enum sc_type type2 = types[i];
					if( d_sc->data[type2] )
						status_change_start(d_bl, bl, type2, 10000, d_sc->data[type2]->val1, 0, 0, (type2 == STATUS_REFLECTSHIELD ? 1 : 0), skill_get_time(status_sc2skill(type2),d_sc->data[type2]->val1), (type2 == STATUS_DEFENDINGAURA) ? SCSTART_NOAVOID : SCSTART_NOAVOID|SCSTART_NOICON);
					i--;
				}
			}
			break;
		}

		case STATUS_COMA: // Coma. Sends a char to 1HP. If val2, do not zap sp
			status_zap(bl, status->hp-1, val2?0:status->sp);
			return 1;
			break;
		case STATUS_CLOSECONFINE2:
		{
			struct block_list *src2 = val2?map_id2bl(val2):NULL;
			struct status_change *sc2 = src2?status_get_sc(src2):NULL;
			struct status_change_entry *sce2 = sc2?sc2->data[STATUS_CLOSECONFINE]:NULL;

			if (src2 && sc2) {
				if (!sce2) // Start lock on caster.
					sc_start4(src2,src2,STATUS_CLOSECONFINE,100,val1,1,0,0,tick+1000);
				else { // Increase count of locked enemies and refresh time.
					(sce2->val2)++;
					delete_timer(sce2->timer, status_change_timer);
					sce2->timer = add_timer(gettick()+tick+1000, status_change_timer, src2->id, STATUS_CLOSECONFINE);
				}
			} else // Status failed.
				return 0;
		}
			break;
		
		case STATUS_BLESSING:
			if ((!undead_flag && status->race!=RC_DEMON) || bl->type == BL_PC)
				val2 = val1*2;
			else
				val2 = 0; // 0 -> Half stat.
			break;
		case STATUS_TRICKDEAD:
			if (vd) vd->dead_sit = 1;
			tick = INFINITE_TICK;
			break;
		case STATUS_IMPROVECONCENTRATION:
			val2 = 2 + val1;
			if (sd) { // Store the card-bonus data that should not count in the %
				val3 = sd->indexed_bonus.param_bonus[1]; // Agi
				val4 = sd->indexed_bonus.param_bonus[4]; // Dex
			} else
				val3 = val4 = 0;
			break;
		case STATUS_MAXPOWERTHRUST:
			{
				val2 = 20 * val1; // Power increase
				int skill, mastercraft_lvl;
				short weapon_l_index, weapon_r_index;
				struct map_session_data* sd = BL_CAST(BL_PC, src);
				struct map_session_data* tsd = BL_CAST(BL_PC, bl);
				weapon_r_index = tsd->equip_index[EQI_HAND_R];
				weapon_l_index = tsd->equip_index[EQI_HAND_L];
				mastercraft_lvl = pc_checkskill(sd, SK_MS_MASTERCRAFT);

				if (
					(tsd->inventory.u.items_inventory[weapon_l_index].card[0] == CARD0_FORGE ||
					tsd->inventory.u.items_inventory[weapon_r_index].card[0] == CARD0_FORGE) &&
					(sd->status.char_id == tsd->inventory.u.items_inventory[weapon_l_index].card[2] ||
						sd->status.char_id == tsd->inventory.u.items_inventory[weapon_r_index].card[2]) &&
					(skill = pc_checkskill(sd, BS_AXE)) == 5) {
					val2 = (30 * val1) * (1 + (mastercraft_lvl * 0.2));
				}
			}
			break;
		case STATUS_POWERTHRUST:
		{
			val3 = 5 * val1; // Dmg increase
			int skill, mastercraft_lvl;
			short weapon_l_index, weapon_r_index;
			struct map_session_data* sd = BL_CAST(BL_PC, src);
			struct map_session_data* tsd = BL_CAST(BL_PC, bl);
			weapon_r_index = tsd->equip_index[EQI_HAND_R];
			weapon_l_index = tsd->equip_index[EQI_HAND_L];
			mastercraft_lvl = pc_checkskill(sd, SK_MS_MASTERCRAFT);

			if (
				(tsd->inventory.u.items_inventory[weapon_l_index].card[0] == CARD0_FORGE ||
				tsd->inventory.u.items_inventory[weapon_r_index].card[0] == CARD0_FORGE) &&
				(sd->status.char_id == tsd->inventory.u.items_inventory[weapon_l_index].card[2] ||
					sd->status.char_id == tsd->inventory.u.items_inventory[weapon_r_index].card[2]) &&
				(skill = pc_checkskill(sd, BS_AXE)) == 5) {
				val3 = (8 * val1) * (1 + (mastercraft_lvl * 0.2));
			}
		}
		break;
		case STATUS_FORTIFY:
			val2 = 25*val1; // Damage Increase
			break;
		case STATUS_SPEARDYNAMO:
			val2 = 10*val1; // Damage Increase
			val3 = 6*val1; // Hit Increase
			break;
		case STATUS_ANGELUS:
			val2 = 10*val1; // def increase
			break;
		case STATUS_IMPOSITIOMANUS:
			val2 = (5*val1) + 5; // WATK increase
			break;
		case STATUS_MELTDOWN:
			val2 = 100*val1*2; // Chance to break weapon
			val3 = 100*val1*2; // Change to break armor
			break;
		
		case STATUS_QUAGMIRE:
			val2 = (sd?10:10)*val1; // Agi/Dex decrease.
			break;

		// gs_something1 [Vicious]
		
		case STATUS_PROVOKE:
			// val2 signals autoprovoke.
			val3 = 2+3*(val1*2); // Atk increase
			val4 = 5+5*(val1*2); // Def reduction.
			break;
		case STATUS_ENERGYCOAT:
			val3 = 5*(val1); // MAtk increase
			break;
		case STATUS_CRUCIS_PLAYER:
			// val2 signals autoprovoke.
			val3 = 4*(val1); // MAtk increase
			val4 = 4*(val1); // MDef increase.
			break;
		case STATUS_WEAPONRYEXPERTISE:
		{
			val2 = 10; // ASPD Increase
			val3 = 4; // HIT increase
			int skill, mastercraft_lvl;
			short weapon_l_index, weapon_r_index;
			struct map_session_data* sd = BL_CAST(BL_PC, src);
			struct map_session_data* tsd = BL_CAST(BL_PC, bl);
			weapon_r_index = tsd->equip_index[EQI_HAND_R];
			weapon_l_index = tsd->equip_index[EQI_HAND_L];
			mastercraft_lvl = pc_checkskill(sd, SK_MS_MASTERCRAFT);

			if (
				(tsd->inventory.u.items_inventory[weapon_l_index].card[0] == CARD0_FORGE ||
				tsd->inventory.u.items_inventory[weapon_r_index].card[0] == CARD0_FORGE) &&
				(sd->status.char_id == tsd->inventory.u.items_inventory[weapon_l_index].card[2] ||
					sd->status.char_id == tsd->inventory.u.items_inventory[weapon_r_index].card[2]) &&
				(skill = pc_checkskill(sd, BS_AXE)) == 5) {
				val2 = 15 * (1 + (mastercraft_lvl * 0.2));
				val3 = 6 * (1 + (mastercraft_lvl * 0.2));
			}
		}
		break;
		case STATUS_WEAPONSHARPENING:
			{
				val2 = 3; // ATK Increase
				val3 = 150; // CRIT increase
				int skill, mastercraft_lvl;
				short weapon_l_index, weapon_r_index;
				struct map_session_data* sd = BL_CAST(BL_PC, src);
				struct map_session_data* tsd = BL_CAST(BL_PC, bl);
				weapon_r_index = tsd->equip_index[EQI_HAND_R];
				weapon_l_index = tsd->equip_index[EQI_HAND_L];
				mastercraft_lvl = pc_checkskill(sd, SK_MS_MASTERCRAFT);

				if (
					(tsd->inventory.u.items_inventory[weapon_l_index].card[0] == CARD0_FORGE ||
					tsd->inventory.u.items_inventory[weapon_r_index].card[0] == CARD0_FORGE) &&
					(sd->status.char_id == tsd->inventory.u.items_inventory[weapon_l_index].card[2] ||
					sd->status.char_id == tsd->inventory.u.items_inventory[weapon_r_index].card[2]) &&
					(skill = pc_checkskill(sd, BS_AXE)) == 5) {
					val2 = 5 * (1 + (mastercraft_lvl * 0.2));
					val3 = 225 * (1 + (mastercraft_lvl * 0.2));
				}
			}
			break;
		case STATUS_ARMORREINFORCEMENT:
			{
				val2 = 0.03; // Damage reduction
				val3 = 5; // Def increase
				int skill, mastercraft_lvl;
				short armor_index;
				struct map_session_data* sd = BL_CAST( BL_PC, src );
				struct map_session_data* tsd = BL_CAST( BL_PC, bl );
				armor_index = tsd->equip_index[EQI_ARMOR];
				mastercraft_lvl = pc_checkskill(sd, SK_MS_MASTERCRAFT);

				if (
					(tsd->inventory.u.items_inventory[armor_index].card[0] = CARD0_FORGE) &&
					sd->status.char_id == tsd->inventory.u.items_inventory[armor_index].card[2]
					&& (skill = pc_checkskill(sd, BS_AXE)) == 5){
					val2 = 0.045 * (1 + (mastercraft_lvl * 0.2));
					val3 = 7.5 * (1 + (mastercraft_lvl * 0.2));
				}
			}
			break;
		
		case STATUS_MINDGAMES:
			val2 = 5*val1; // matk increase.
			val3 = 5*val1; // mdef reduction.
			break;
		
		
		case STATUS_SUFFRAGIUM:
			val2 = val1 * 5; // Speed cast decrease
			break;
		
		case STATUS_MAGICSQUARED:
			val2 = 30+10*val1; // Trigger rate
			break;
		
		case STATUS_DEFENSIVESTANCE:
			if (sd)
				val1 = pc_checkskill(sd, SK_MO_BODYRELOCATION) * 4; // DEF/MDEF Increase
			break;
		case STATUS_STONESKIN:
			if (sd)
				val1 = pc_checkskill(sd, SK_KN_STONESKIN) * 4; // DEF/MDEF Increase
			break;
		
		case STATUS_PAGANPARTY:
			val2 = status_get_lv(src);
			val3 = status_get_int(src);
			val4 = tick / 8000;
			tick_time = 8000;
			break;
		case STATUS_MOONLIGHTSONATA:
			val4 = tick / 15000;
			tick_time = 15000;
			break;
		case STATUS_RENEW:
			val4 = tick / 10000;
			tick_time = 10000;
			break;
		
		case STATUS_VENOMIMPRESS:
			val2 = 10 * val1;
			break;
		
		case STATUS_HALLUCINATIONWALK:
			val2 = 50 * val1; // Evasion rate of physical attacks. Flee
			val3 = 10 * val1; // Evasion rate of magical attacks.
			break;
		case STATUS_STORE_SPELLBOOK:
			// val2 = sp drain per 10 seconds
			tick_time = 10000; // [GodLesZ] tick time
			break;
		case STATUS_SPHERE_1:
		case STATUS_SPHERE_2:
		case STATUS_SPHERE_3:
		case STATUS_SPHERE_4:
		case STATUS_SPHERE_5:
			if( !sd )
				return 0;	// Should only work on players.
			val4 = tick / 1000;
			if( val4 < 1 )
				val4 = 1;
			tick_time = 1000; // [GodLesZ] tick time
			break;
		
		case STATUS_CAMOUFLAGE:
			val4 = tick/1000;
			tick_time = 1000; // [GodLesZ] tick time
			break;
		
		case STATUS_STALK:
			{
				struct map_session_data * s_sd = map_id2sd(val2);
				if( s_sd )
					s_sd->shadowform_id = bl->id;
				val4 = tick / 1000;
				tick_time = 1000; // [GodLesZ] tick time
			}
			break;
		case STATUS_STRIPACCESSORY:
			if (!sd)
				val2 = 20;
			break;
		case STATUS_INVISIBILITY:
			val4 = tick / 1000;
			tick = INFINITE_TICK; // Duration sent to the client should be infinite
			tick_time = 1000; // [GodLesZ] tick time
			break;
		
		case STATUS_ULLREAGLETOTEM:
			val2 = src->id;
			tick_time = 300;
			val4 = tick / tick_time;
			break;
		
		case STATUS_ECHONOCTURNAE: // MATK/MDEF Increase
		case STATUS_WINDMILLPOEM: // ATK/DEC Increase
			val3 = 3 * val1;
			break;
		
		case STATUS_VANGUARDFORCE:
			val2 = val1 * 5;	// Chance to evade magic damage.
			val3 = val1 * 5; // Defence added
			break;
		
		case STATUS_PARALYSIS: // [Lighta] need real info
			val2 = 2*val1; // def reduction
			val3 = 500*val1; // varcast augmentation
			break;
		
		case STATUS_MILLENIUMSHIELDS:
			tick_time = 1000;
			val4 = tick / tick_time;
			break;
		case STATUS_VOIDMAGEPRODIGY:
		case STATUS_LAUDATEDOMINIUM:
			val3 = 20 * val1; // magic dmg bonus
			break;
		case STATUS_OFFERTORIUM:
			val2 = 1 + val1*0.3; // heal power bonus
			val3 = 100 + 20 * val1; // sp cost inc
			break;
	
		case STATUS_DARKCLAW:
			val2 = 30 * val1; // ATK bonus
			break;
		
		case STATUS_SWORDCLAN:
		case STATUS_ARCWANDCLAN:
		case STATUS_GOLDENMACECLAN:
		case STATUS_CROSSBOWCLAN:
		case STATUS_JUMPINGCLAN:
			tick = INFINITE_TICK;
			status_change_start(src,bl,STATUS_CLAN_INFO,10000,0,val2,0,0,INFINITE_TICK,flag);
			break;
		

		default:
			if( calc_flag == SCB_NONE && StatusSkillChangeTable[type] == -1 && StatusIconChangeTable[type] == EFST_BLANK ) {
				// Status change with no calc, no icon, and no skill associated...?
				ShowError("UnknownStatusChange [%d]\n", type);
				return 0;
			}
	} else // Special considerations when loading SC data.
		switch( type ) {
			
			case STATUS_STONECURSE:
				if (val3 > 0)
					break; //Incubation time still active
				//Fall through
			case STATUS_POISON:
			case STATUS_DPOISON:
			case STATUS_BLEEDING:
				tick_time = tick;
				tick = tick_time + max(val4, 0);
				break;
			case STATUS_SWORDCLAN:
			case STATUS_ARCWANDCLAN:
			case STATUS_GOLDENMACECLAN:
			case STATUS_CROSSBOWCLAN:
			case STATUS_JUMPINGCLAN:
			case STATUS_CLAN_INFO:
				// If the player still has a clan status, but was removed from his clan
				if( sd && sd->status.clan_id == 0 ){
					return 0;
				}
				break;
		}

	// Values that must be set regardless of flag&4 e.g. val_flag [Ind]
	switch(type) {
		// Start |1 val_flag setting
		case STATUS_ROLLINGCUTTER:
		case STATUS_SPHERE_1:
		case STATUS_SPHERE_2:
		case STATUS_SPHERE_3:
		case STATUS_SPHERE_4:
		case STATUS_SPHERE_5:
		case STATUS_PUSHCART:
		case STATUS_SWORDCLAN:
		case STATUS_ARCWANDCLAN:
		case STATUS_GOLDENMACECLAN:
		case STATUS_CROSSBOWCLAN:
		case STATUS_JUMPINGCLAN:
			val_flag |= 1;
			break;
		// Start |1|2 val_flag setting
		case STATUS_VENOMIMPRESS:
		case STATUS_INVISIBILITY:
		case STATUS_VANGUARDFORCE:
		case STATUS_CLAN_INFO:
			val_flag |= 1|2;
			break;
		// Start |1|2|4 val_flag setting
		case STATUS_HALLUCINATIONWALK:
		case STATUS_STALK:
		case STATUS_SPELLFIST:
			val_flag |= 1|2|4;
			break;
	}

	if (current_equip_combo_pos && tick == INFINITE_TICK) {
		ShowWarning("sc_start: Item combo contains an INFINITE_TICK duration. Skipping bonus.\n");
		return 0;
	}

	/* [Ind] */
	if (StatusDisplayType[type]&bl->type) {
		int dval1 = 0, dval2 = 0, dval3 = 0;

		switch (type) {
			case STATUS_ALL_RIDING:
				dval1 = 1;
				break;
			case STATUS_CLAN_INFO:
				dval1 = val1;
				dval2 = val2;
				dval3 = val3;
				break;
			default: /* All others: just copy val1 */
				dval1 = val1;
				break;
		}
		status_display_add(bl,type,dval1,dval2,dval3);
	}

	// Those that make you stop attacking/walking....
	switch (type) {
		case STATUS_MAGICCELL:
			if (sd && pc_issit(sd)) // Avoid sprite sync problems.
				pc_setstand(sd, true);
		case STATUS_SLEEP:
		case STATUS_STONECURSE:
			// Cancel cast when get status [LuzZza]
			if (battle_config.sc_castcancel&bl->type)
				unit_skillcastcancel(bl, 0);
		// Fall through
		case STATUS_MILLENIUMSHIELDS:
			unit_stop_attack(bl);
			if (type == STATUS_FREEZE || type == STATUS_STUN || type == STATUS_SLEEP || type == STATUS_STONECURSE)
				break;
		// Fall through
		case STATUS_CONFUSION:
		case STATUS_CLOSECONFINE:
		case STATUS_CLOSECONFINE2:
		case STATUS_PARALYSIS:
		//case SC__CHAOS:
			if (!unit_blown_immune(bl,0x1))
				unit_stop_walking(bl,1);
			break;
		
		case STATUS_HIDING:
		case STATUS_CLOAKING:
		case STATUS_CHASEWALK:
		case STATUS_WEIGHT90:
		case STATUS_CAMOUFLAGE:
			unit_stop_attack(bl);
			break;
		case STATUS_SILENCE:
			if (battle_config.sc_castcancel&bl->type)
				unit_skillcastcancel(bl, 0);
			break;
	}

	// Set option as needed.
	opt_flag = 1;
	switch(type) {
		// OPT1
		case STATUS_STONECURSE: 
			if (val3 > 0)
				sc->opt1 = OPT1_STONEWAIT;
			else
				sc->opt1 = OPT1_STONE;
			break;
		case STATUS_FREEZE:		sc->opt1 = OPT1_FREEZE;		break;
		case STATUS_STUN:		sc->opt1 = OPT1_STUN;		break;
		case STATUS_SLEEP:		sc->opt1 = OPT1_SLEEP;		break;
		case STATUS_MAGICCELL:  sc->opt1 = OPT1_IMPRISON;	break;
		// OPT2
		case STATUS_POISON:       sc->opt2 |= OPT2_POISON;		break;
		case STATUS_CURSE:        sc->opt2 |= OPT2_CURSE;		break;
		case STATUS_SILENCE:      sc->opt2 |= OPT2_SILENCE;		break;
		case STATUS_BLIND:        sc->opt2 |= OPT2_BLIND;		break;
		case STATUS_ANGELUS:      sc->opt2 |= OPT2_ANGELUS;		break;
		case STATUS_BLEEDING:     sc->opt2 |= OPT2_BLEEDING;	break;
		case STATUS_DPOISON:      sc->opt2 |= OPT2_DPOISON;		break;
		// OPT3
		case STATUS_SPEARDYNAMO:
			sc->opt3 |= OPT3_QUICKEN;
			opt_flag = 0;
			break;
		case STATUS_MAXPOWERTHRUST:
		case STATUS_POWERTHRUST:
			sc->opt3 |= OPT3_OVERTHRUST;
			opt_flag = 0;
			break;
		case STATUS_ENERGYCOAT:
			sc->opt3 |= OPT3_ENERGYCOAT;
			opt_flag = 0;
			break;
		case STATUS_INCATKRATE:
			// Simulate Explosion Spirits effect for NPC_POWERUP [Skotlex]
			if (bl->type != BL_MOB) {
				opt_flag = 0;
				break;
			}
		case STATUS_FURY:
			sc->opt3 |= OPT3_EXPLOSIONSPIRITS;
			opt_flag = 0;
			break;
		case STATUS_ULTRAINSTINCT:
			sc->opt3 |= OPT3_EXPLOSIONSPIRITS;
			opt_flag = 0;
			break;
		case STATUS_MENTALSTRENGTH:
			sc->opt3 |= OPT3_STEELBODY;
			opt_flag = 0;
			break;
		case STATUS_GRAPPLE:
			sc->opt3 |= OPT3_BLADESTOP;
			opt_flag = 0;
			break;
		case STATUS_AURABLADE:
			sc->opt3 |= OPT3_AURABLADE;
			opt_flag = 0;
			break;
		case STATUS_FRENZY:
			opt_flag = 0;
			sc->opt3 |= OPT3_BERSERK;
			break;

		case STATUS_MARIONETTE:
		case STATUS_MARIONETTE2:
			sc->opt3 |= OPT3_MARIONETTE;
			opt_flag = 0;
			break;
		case STATUS_ARMORREINFORCEMENT:
		case STATUS_ASSUMPTIO:
			sc->opt3 |= OPT3_ASSUMPTIO;
			opt_flag = 0;
			break;
	
		case STATUS_HIDING:
			sc->option |= OPTION_HIDE;
			opt_flag = 2;
			break;
		case STATUS_CLOAKING:
		case STATUS_INVISIBILITY:
			sc->option |= OPTION_CLOAK;
		case STATUS_CAMOUFLAGE:
		case STATUS_STALK:
			opt_flag = 2;
			break;
		case STATUS_CHASEWALK:
			sc->option |= OPTION_CHASEWALK|OPTION_CLOAK;
			opt_flag = 2;
			break;
		case STATUS_RUWACH:
			sc->option |= OPTION_RUWACH;
			break;
		default:
			opt_flag = 0;
	}

	// On Aegis, when turning on a status change, first goes the option packet, then the sc packet.
	if(opt_flag) {
		clif_changeoption(bl);
		if(sd && (opt_flag&0x4)) {
			clif_changelook(bl,LOOK_BASE,vd->class_);
			clif_changelook(bl,LOOK_WEAPON,0);
			clif_changelook(bl,LOOK_SHIELD,0);
			clif_changelook(bl,LOOK_CLOTHES_COLOR,vd->cloth_color);
		}
	}
	if (calc_flag&SCB_DYE) { // Reset DYE color
		if (vd && vd->cloth_color) {
			val4 = vd->cloth_color;
			clif_changelook(bl,LOOK_CLOTHES_COLOR,0);
		}
		calc_flag&=~SCB_DYE;
	}

	/*if (calc_flag&SCB_BODY)// Might be needed in the future. [Rytech]
	{	//Reset body style
		if (vd && vd->body_style)
		{
			val4 = vd->body_style;
			clif_changelook(bl,LOOK_BODY2,0);
		}
		calc_flag&=~SCB_BODY;
	}*/

	if (!(flag&SCSTART_NOICON) && !(flag&SCSTART_LOADED && StatusDisplayType[type])) {
		int status_icon = StatusIconChangeTable[type];

#if PACKETVER < 20151104
		if (status_icon == EFST_WEAPONPROPERTY)
			status_icon = EFST_ATTACK_PROPERTY_NOTHING + val1; // Assign status icon for older clients
#endif

		clif_status_change(bl, status_icon, 1, tick, (val_flag & 1) ? val1 : 1, (val_flag & 2) ? val2 : 0, (val_flag & 4) ? val3 : 0);
	}

	// Used as temporary storage for scs with interval ticks, so that the actual duration is sent to the client first.
	if( tick_time )
		tick = tick_time;

	// Don't trust the previous sce assignment, in case the SC ended somewhere between there and here.
	if((sce=sc->data[type])) { // reuse old sc
		if( sce->timer != INVALID_TIMER )
			delete_timer(sce->timer, status_change_timer);
		sc_isnew = false;
	} else { // New sc
		++(sc->count);
		sce = sc->data[type] = ers_alloc(sc_data_ers, struct status_change_entry);
	}
	sce->val1 = val1;
	sce->val2 = val2;
	sce->val3 = val3;
	sce->val4 = val4;
	if (tick >= 0)
		sce->timer = add_timer(gettick() + tick, status_change_timer, bl->id, type);
	else
		sce->timer = INVALID_TIMER; // Infinite duration

	if (calc_flag) {
		if (sd) {
			switch(type) {
				// Statuses that adjust HP/SP and heal after starting
				case STATUS_FRENZY:
					status_calc_bl_(bl, static_cast<scb_flag>(calc_flag), SCO_FORCE);
					break;
				default:
					status_calc_bl(bl, calc_flag);
					break;
			}
		} else
			status_calc_bl(bl, calc_flag);
	}

	if ( sc_isnew && StatusChangeStateTable[type] ) // Non-zero
		status_calc_state(bl,sc,( enum scs_flag ) StatusChangeStateTable[type],true);

	if (sd && sd->pd)
		pet_sc_check(sd, type); // Skotlex: Pet Status Effect Healing

	
	if( opt_flag&2 && sd && !sd->npc_ontouch_.empty() )
		npc_touchnext_areanpc(sd,false); // Run OnTouch_ on next char in range

	return 1;
}

/**
 * End all statuses except those listed
 * TODO: May be useful for dispel instead resetting a list there
 * @param src: Source of the status change [PC|MOB|HOM|MER|ELEM|NPC]
 * @param type: Changes behaviour of the function
 * 	0: PC killed -> Place here statuses that do not dispel on death.
 * 	1: If for some reason status_change_end decides to still keep the status when quitting.
 * 	2: Do clif_changeoption()
 * 	3: Do not remove some permanent/time-independent effects
 * @return 1: Success 0: Fail
 */
int status_change_clear(struct block_list* bl, int type)
{
	struct status_change* sc;
	int i;

	sc = status_get_sc(bl);

	if (!sc)
		return 0;

	// Cleaning all extras vars
	sc->comet_x = 0;
	sc->comet_y = 0;
#ifndef RENEWAL
	sc->sg_counter = 0;
#endif
	sc->bs_counter = 0;

	if (!sc->count)
		return 0;

	for(i = 0; i < SC_MAX; i++) {
		if(!sc->data[i])
			continue;

		if(type == 0) {
			switch (i) { // Type 0: PC killed -> Place here statuses that do not dispel on death.
			
			case STATUS_WEIGHT50:
			case STATUS_WEIGHT90:
			case STATUS_ENCHANTDEADLYPOISON:
			case STATUS_MELTDOWN:
			
			case STATUS_NOCHAT:
			
			case STATUS_IMPRESSIVERIFF:
			case STATUS_MAGICSTRINGS:
			
			case STATUS_DEF_RATE:
			case STATUS_MDEF_RATE:
			
			case STATUS_PUSHCART:
			
			// Clans
			case STATUS_CLAN_INFO:
			case STATUS_SWORDCLAN:
			case STATUS_ARCWANDCLAN:
			case STATUS_GOLDENMACECLAN:
			case STATUS_CROSSBOWCLAN:
			case STATUS_JUMPINGCLAN:
				continue;
			}
		}

		if( type == 3 ) {
			switch (i) { // !TODO: This list may be incomplete
			case STATUS_WEIGHT50:
			case STATUS_WEIGHT90:
			case STATUS_NOCHAT:
			case STATUS_PUSHCART:
			case STATUS_ALL_RIDING:
			
			// Clans
			case STATUS_CLAN_INFO:
			case STATUS_SWORDCLAN:
			case STATUS_ARCWANDCLAN:
			case STATUS_GOLDENMACECLAN:
			case STATUS_CROSSBOWCLAN:
			case STATUS_JUMPINGCLAN:
				continue;
			}
		}

		status_change_end(bl, (sc_type)i, INVALID_TIMER);

		if( type == 1 && sc->data[i] ) { // If for some reason status_change_end decides to still keep the status when quitting. [Skotlex]
			(sc->count)--;
			if (sc->data[i]->timer != INVALID_TIMER)
				delete_timer(sc->data[i]->timer, status_change_timer);
			ers_free(sc_data_ers, sc->data[i]);
			sc->data[i] = NULL;
		}
	}

	sc->opt1 = 0;
	sc->opt2 = 0;
	sc->opt3 = 0;

	if( type == 0 || type == 2 )
		clif_changeoption(bl);

	return 1;
}

/**
 * End a specific status after checking
 * @param bl: Source of the status change [PC|MOB|HOM|MER|ELEM|NPC]
 * @param type: Status change (SC_*)
 * @param tid: Timer
 * @param file: Used for dancing save
 * @param line: Used for dancing save
 * @return 1: Success 0: Fail
 */
int status_change_end_(struct block_list* bl, enum sc_type type, int tid, const char* file, int line)
{
	struct map_session_data *sd;
	struct status_change *sc;
	struct status_change_entry *sce;
	struct status_data *status;
	struct view_data *vd;
	int opt_flag = 0;
	enum scb_flag calc_flag = SCB_NONE;
	nullpo_ret(bl);

	sc = status_get_sc(bl);
	status = status_get_status_data(bl);
	if(type < 0 || type >= SC_MAX || !sc || !(sce = sc->data[type]))
		return 0;

	sd = BL_CAST(BL_PC,bl);

	if (sce->timer != tid && tid != INVALID_TIMER)
		return 0;

	if (tid == INVALID_TIMER) {
		if (type == STATUS_ENDURE && sce->val4)
			// Do not end infinite endure.
			return 0;
		
		if (sce->timer != INVALID_TIMER) // Could be a SC with infinite duration
			delete_timer(sce->timer,status_change_timer);
		if (sc->opt1)
			switch (type) {
				// "Ugly workaround"  [Skotlex]
				// delays status change ending so that a skill that sets opt1 fails to
				// trigger when it also removed one
				case STATUS_STONECURSE:
					sce->val4 = -1; // Petrify time
				case STATUS_FREEZE:
				case STATUS_STUN:
				case STATUS_SLEEP:
					if (sce->val1) {
						// Removing the 'level' shouldn't affect anything in the code
						// since these SC are not affected by it, and it lets us know
						// if we have already delayed this attack or not.
						sce->val1 = 0;
						sce->timer = add_timer(gettick()+10, status_change_timer, bl->id, type);
						return 1;
					}
			}
	}

	(sc->count)--;

	if ( StatusChangeStateTable[type] )
		status_calc_state(bl,sc,( enum scs_flag ) StatusChangeStateTable[type],false);

	sc->data[type] = NULL;

	if (StatusDisplayType[type]&bl->type)
		status_display_remove(bl,type);

	vd = status_get_viewdata(bl);
	calc_flag = static_cast<scb_flag>(StatusChangeFlagTable[type]);
	switch(type) {
		
		
		case STATUS_ANGER:
			if (sc->data[STATUS_PROVOKE] && sc->data[STATUS_PROVOKE]->val2 == 1)
				status_change_end(bl, STATUS_PROVOKE, INVALID_TIMER);
			break;

		case STATUS_ENDURE:
		case STATUS_DEFENDINGAURA:
		case STATUS_REFLECTSHIELD:
		case STATUS_AUTOGUARD:
			{
				struct map_session_data *tsd;
				if( bl->type == BL_PC ) { // Clear Status from others
					int i;
					for( i = 0; i < MAX_DEVOTION; i++ ) {
						if( sd->devotion[i] && (tsd = map_id2sd(sd->devotion[i])) && tsd->sc.data[type] )
							status_change_end(&tsd->bl, type, INVALID_TIMER);
					}
				}
			}
			break;
		case STATUS_SWORNPROTECTOR:
			{
				struct block_list *d_bl = map_id2bl(sce->val1);
				if( d_bl ) {
					if( d_bl->type == BL_PC )
						((TBL_PC*)d_bl)->devotion[sce->val2] = 0;
					clif_devotion(d_bl, NULL);
				}

				status_change_end(bl, STATUS_AUTOGUARD, INVALID_TIMER);
				status_change_end(bl, STATUS_DEFENDINGAURA, INVALID_TIMER);
				status_change_end(bl, STATUS_REFLECTSHIELD, INVALID_TIMER);
				status_change_end(bl, STATUS_ENDURE, INVALID_TIMER);
			}
			break;

		


		case STATUS_GRAPPLE:
			if(sce->val4) {
				int tid2 = sce->val4; //stop the status for the other guy of bladestop as well
				struct block_list *tbl = map_id2bl(tid2);
				struct status_change *tsc = status_get_sc(tbl);
				sce->val4 = 0;
				if(tbl && tsc && tsc->data[STATUS_GRAPPLE]) {
					tsc->data[STATUS_GRAPPLE]->val4 = 0;
					status_change_end(tbl, STATUS_GRAPPLE, INVALID_TIMER);
				}
				clif_bladestop(bl, tid2, 0);
			}
			break;
		
		case STATUS_NOCHAT:
			if (sd && sd->status.manner < 0 && tid != INVALID_TIMER)
				sd->status.manner = 0;
			if (sd && tid == INVALID_TIMER) {
				clif_changestatus(sd,SP_MANNER,sd->status.manner);
				clif_updatestatus(sd,SP_MANNER);
			}
			break;
		case STATUS_VENOMSPLASHER:
			{
				struct block_list *src=map_id2bl(sce->val3);
				if(src && tid != INVALID_TIMER)
					skill_castend_damage_id(src, bl, sce->val2, sce->val1, gettick(), SD_LEVEL );
			}
			break;
		case STATUS_CLOSECONFINE2:{
			struct block_list *src = sce->val2?map_id2bl(sce->val2):NULL;
			struct status_change *sc2 = src?status_get_sc(src):NULL;
			if (src && sc2 && sc2->data[STATUS_CLOSECONFINE]) {
				// If status was already ended, do nothing.
				// Decrease count
				if (--(sc2->data[STATUS_CLOSECONFINE]->val1) <= 0) // No more holds, free him up.
					status_change_end(src, STATUS_CLOSECONFINE, INVALID_TIMER);
			}
		}
		case STATUS_CLOSECONFINE:
			if (sce->val2 > 0) {
				// Caster has been unlocked... nearby chars need to be unlocked.
				int range = 1
					+ skill_get_range2(bl, status_sc2skill(type), sce->val1, true)
					+ skill_get_range2(bl, SK_TF_BACKSLIDING, 1, true); // Since most people use this to escape the hold....
				map_foreachinallarea(status_change_timer_sub,
					bl->m, bl->x-range, bl->y-range, bl->x+range,bl->y+range,BL_CHAR,bl,sce,type,gettick());
			}
			break;
		case STATUS_MARIONETTE:
		case STATUS_MARIONETTE2: // Marionette target
			if (sce->val1) { // Check for partner and end their marionette status as well
				enum sc_type type2 = (type == STATUS_MARIONETTE) ? STATUS_MARIONETTE2 : STATUS_MARIONETTE;
				struct block_list *pbl = map_id2bl(sce->val1);
				struct status_change* sc2 = pbl?status_get_sc(pbl):NULL;

				if (sc2 && sc2->data[type2]) {
					sc2->data[type2]->val1 = 0;
					status_change_end(pbl, type2, INVALID_TIMER);
				}
			}
			break;

		case STATUS_TRICKDEAD:
			if (vd) vd->dead_sit = 0;
			break;
		case STATUS_STALK:
			{
				struct map_session_data *s_sd = map_id2sd(sce->val2);

				if (s_sd) s_sd->shadowform_id = 0;
			}
			break;
		
		case STATUS_SWORDCLAN:
		case STATUS_ARCWANDCLAN:
		case STATUS_GOLDENMACECLAN:
		case STATUS_CROSSBOWCLAN:
		case STATUS_JUMPINGCLAN:
			status_change_end(bl,STATUS_CLAN_INFO,INVALID_TIMER);
			break;
		
	}

	opt_flag = 1;
	switch(type) {
	case STATUS_STONECURSE:
	case STATUS_FREEZE:
	case STATUS_STUN:
	case STATUS_SLEEP:
	case STATUS_MAGICCELL:
		sc->opt1 = 0;
		break;

	case STATUS_POISON:
	case STATUS_CURSE:
	case STATUS_SILENCE:
	case STATUS_BLIND:
		sc->opt2 &= ~(1<<(type-STATUS_POISON));
		break;
	case STATUS_DPOISON:
		sc->opt2 &= ~OPT2_DPOISON;
		break;
	

	case STATUS_HIDING:
		sc->option &= ~OPTION_HIDE;
		opt_flag |= 2|4; // Check for warp trigger + AoE trigger
		break;
	case STATUS_CLOAKING:
	case STATUS_INVISIBILITY:
		sc->option &= ~OPTION_CLOAK;
	case STATUS_CAMOUFLAGE:
	case STATUS_STALK:
		opt_flag |= 2;
		break;
	case STATUS_CHASEWALK:
		sc->option &= ~(OPTION_CHASEWALK|OPTION_CLOAK);
		opt_flag |= 2;
		break;
	
	case STATUS_RUWACH:
		sc->option &= ~OPTION_RUWACH;
		break;
	
	
	case STATUS_POWERTHRUST:
	case STATUS_MAXPOWERTHRUST:
		sc->opt3 &= ~OPT3_OVERTHRUST;
		opt_flag = 0;
		break;
	case STATUS_ENERGYCOAT:
		sc->opt3 &= ~OPT3_ENERGYCOAT;
		opt_flag = 0;
		break;
	case STATUS_INCATKRATE: // Simulated Explosion spirits effect.
		if (bl->type != BL_MOB) {
			opt_flag = 0;
			break;
		}
	case STATUS_FURY:
		sc->opt3 &= ~OPT3_EXPLOSIONSPIRITS;
		opt_flag = 0;
		break;
	case STATUS_ULTRAINSTINCT:
		sc->opt3 &= ~OPT3_EXPLOSIONSPIRITS;
		opt_flag = 0;
		break;
	case STATUS_MENTALSTRENGTH:
	
	case STATUS_GRAPPLE:
		sc->opt3 &= ~OPT3_BLADESTOP;
		opt_flag = 0;
		break;
	case STATUS_AURABLADE:
		sc->opt3 &= ~OPT3_AURABLADE;
		opt_flag = 0;
		break;
	case STATUS_FRENZY:
		opt_flag = 0;
		sc->opt3 &= ~OPT3_BERSERK;
		break;

	case STATUS_MARIONETTE:
	case STATUS_MARIONETTE2:
		sc->opt3 &= ~OPT3_MARIONETTE;
		opt_flag = 0;
		break;
	case STATUS_ARMORREINFORCEMENT:
	case STATUS_ASSUMPTIO:
		sc->opt3 &= ~OPT3_ASSUMPTIO;
		opt_flag = 0;
		break;
	default:
		opt_flag = 0;
	}

	if (calc_flag&SCB_DYE) { // Restore DYE color
		if (vd && !vd->cloth_color && sce->val4)
			clif_changelook(bl,LOOK_CLOTHES_COLOR,sce->val4);
		calc_flag = static_cast<scb_flag>(calc_flag&~SCB_DYE);
	}

	/*if (calc_flag&SCB_BODY)// Might be needed in the future. [Rytech]
	{	//Restore body style
		if (vd && !vd->body_style && sce->val4)
			clif_changelook(bl,LOOK_BODY2,sce->val4);
		calc_flag = static_cast<scb_flag>(calc_flag&~SCB_BODY);
	}*/

	// On Aegis, when turning off a status change, first goes the sc packet, then the option packet.
	int status_icon = StatusIconChangeTable[type];

#if PACKETVER < 20151104
	if (status_icon == EFST_WEAPONPROPERTY)
		status_icon = EFST_ATTACK_PROPERTY_NOTHING + sce->val1; // Assign status icon for older clients
#endif

	clif_status_change(bl,status_icon,0,0,0,0,0);

	if( opt_flag&8 ) // bugreport:681
		clif_changeoption2(bl);
	else if(opt_flag) {
		clif_changeoption(bl);
		if (sd && (opt_flag&0x4)) {
			clif_changelook(bl,LOOK_BASE,sd->vd.class_);
			clif_get_weapon_view(sd,&sd->vd.weapon,&sd->vd.shield);
			clif_changelook(bl,LOOK_WEAPON,sd->vd.weapon);
			clif_changelook(bl,LOOK_SHIELD,sd->vd.shield);
			clif_changelook(bl,LOOK_CLOTHES_COLOR,cap_value(sd->status.clothes_color,0,battle_config.max_cloth_color));
			clif_changelook(bl,LOOK_BODY2,cap_value(sd->status.body,0,battle_config.max_body_style));
		}
	}
	if (calc_flag) {
		switch (type) {
		case STATUS_MYSTICALAMPLIFICATION:
			//If Mystical Amplification ends, MATK is immediately recalculated
			status_calc_bl_(bl, calc_flag, SCO_FORCE);
			break;
		default:
			status_calc_bl(bl, calc_flag);
			break;
		}
	}

	if(opt_flag&4) // Out of hiding, invoke on place.
		skill_unit_move(bl,gettick(),1);

	if(opt_flag&2 && sd && !sd->state.warping && map_getcell(bl->m,bl->x,bl->y,CELL_CHKNPC))
		npc_touch_area_allnpc(sd,bl->m,bl->x,bl->y); // Trigger on-touch event.

	ers_free(sc_data_ers, sce);
	return 1;
}

/**
 * Resets timers for statuses
 * Used with reoccurring status effects, such as dropping SP every 5 seconds
 * @param tid: Timer ID
 * @param tick: How long before next call
 * @param id: ID of character
 * @param data: Information passed through the timer call
 * @return 1: Success 0: Fail
 */
TIMER_FUNC(status_change_timer){
	enum sc_type type = (sc_type)data;
	struct block_list *bl;
	struct map_session_data *sd;
	int interval = status_get_sc_interval(type);
	bool dounlock = false;

	bl = map_id2bl(id);
	if(!bl) {
		ShowDebug("status_change_timer: Null pointer id: %d data: %" PRIdPTR "\n", id, data);
		return 0;
	}

	struct status_change * const sc = status_get_sc(bl);
	struct status_data * const status = status_get_status_data(bl);
	if(!sc) {
		ShowDebug("status_change_timer: Null pointer id: %d data: %" PRIdPTR " bl-type: %d\n", id, data, bl->type);
		return 0;
	}
	
	struct status_change_entry * const sce = sc->data[type];
	if(!sce) {
		ShowDebug("status_change_timer: Null pointer id: %d data: %" PRIdPTR " bl-type: %d\n", id, data, bl->type);
		return 0;
	}
	if( sce->timer != tid ) {
		ShowError("status_change_timer: Mismatch for type %d: %d != %d (bl id %d)\n",type,tid,sce->timer, bl->id);
		return 0;
	}

	sd = BL_CAST(BL_PC, bl);

	std::function<void (t_tick)> sc_timer_next = [&sce, &bl, &data](t_tick t) {
		sce->timer = add_timer(t, status_change_timer, bl->id, data);
	};
	
	switch(type) {
	case STATUS_MAXIMIZEPOWER:
	case STATUS_CLOAKING:
		if(!status_charge(bl, 0, 1))
			break; // Not enough SP to continue.
		sc_timer_next(sce->val2+tick);
		return 0;

	case STATUS_CHASEWALK:
		if(!status_charge(bl, 0, sce->val4))
			break; // Not enough SP to continue.

		if (!sc->data[STATUS_CHASEWALK2]) {
			sc_start(bl,bl, STATUS_CHASEWALK2,100,1<<(sce->val1-1),
				(t_tick)(1) // SL bonus -> x10 duration
				*skill_get_time2(status_sc2skill(type),sce->val1));
		}
		sc_timer_next(sce->val2+tick);
		return 0;
	break;

	case STATUS_HIDING:
		if(--(sce->val2)>0) {

			if(sce->val2 % sce->val4 == 0 && !status_charge(bl, 0, 1))
				break; // Fail if it's time to substract SP and there isn't.

			sc_timer_next(1000+tick);
			return 0;
		}
	break;

	case STATUS_RUWACH:
		
		map_foreachinallrange( status_change_timer_sub, bl, sce->val3, BL_CHAR, bl, sce, type, tick);
		skill_reveal_trap_inarea(bl, sce->val3, bl->x, bl->y);
		

		if( --(sce->val2)>0 ) {
			sce->val4 += 20; // Use for Shadow Form 2 seconds checking.
			sc_timer_next(20+tick);
			return 0;
		}
		break;

	case STATUS_PROVOKE:
		if(sce->val2) { // Auto-provoke (it is ended in status_heal)
			sc_timer_next(1000*60+tick);
			return 0;
		}
		break;

	case STATUS_STONECURSE:
		if (sc->opt1 == OPT1_STONEWAIT && sce->val4) {
			sce->val3 = 0; //Incubation time used up
			unit_stop_attack(bl);
			
			status_change_end(bl, STATUS_LEXAETERNA, INVALID_TIMER);
			sc->opt1 = OPT1_STONE;
			clif_changeoption(bl);
			sc_timer_next(min(sce->val4, interval) + tick);
			sce->val4 -= interval; //Remaining time
			status_calc_bl(bl, StatusChangeFlagTable[type]);
			return 0;
		}
		if (sce->val4 >= 0 && !(sce->val3) && status->hp > status->max_hp / 4) {
			status_percent_damage(NULL, bl, 1, 0, false);
		}
		break;

	case STATUS_POISON:
	case STATUS_DPOISON:
		if (sce->val4 >= 0) {
			unsigned int damage = 0;
			if (sd)
				damage = (type == STATUS_DPOISON) ? 2 + status->max_hp / 50 : 2 + status->max_hp * 3 / 200;
			else
				damage = (type == STATUS_DPOISON) ? 2 + status->max_hp / 100 : 2 + status->max_hp / 200;
			if (status->hp > umax(status->max_hp / 4, damage)) // Stop damaging after 25% HP left.
				status_zap(bl, damage, 0);		
		}
		break;
	
	case STATUS_FRENZY:
		// 5% every 10 seconds [DracoRPG]
		if( --( sce->val3 ) > 0 && status_charge(bl, sce->val2, 0) && status->hp > 100 ) {
			sc_timer_next(sce->val4+tick);
			return 0;
		}
		break;

	case STATUS_NOCHAT:
		if(sd) {
			sd->status.manner++;
			clif_changestatus(sd,SP_MANNER,sd->status.manner);
			clif_updatestatus(sd,SP_MANNER);
			if (sd->status.manner < 0) { // Every 60 seconds your manner goes up by 1 until it gets back to 0.
				sc_timer_next(60000+tick);
				return 0;
			}
		}
		break;

	case STATUS_VENOMSPLASHER:
		if((sce->val4 -= 500) > 0) {
			sc_timer_next(500 + tick);
			return 0;
		}
		break;

	case STATUS_MARIONETTE:
	case STATUS_MARIONETTE2:
		{
			struct block_list *pbl = map_id2bl(sce->val1);
			if( pbl && check_distance_bl(bl, pbl, 11) ) {
				sc_timer_next(1000 + tick);
				return 0;
			}
		}
		break;

	/*case SC_JAILED:
		if(sce->val1 == INT_MAX || --(sce->val1) > 0) {
			sc_timer_next(60000+tick);
			return 0;
		}
		break;*/


	case STATUS_PAGANPARTY:
		if( --(sce->val4) > 0 ) {
			int healing, matk = 0;
			struct status_data *status;
			struct block_list *src = map_id2bl(sce->val1);
			status = status_get_status_data(&sd->bl);
			matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
			healing = (200 * sce->val1) + (sce->val2 * 3) + (sce->val3 * 3) + (matk * 3);
			clif_specialeffect(bl, EF_FOOD04, AREA);
			clif_specialeffect(bl, 1407, AREA); //new_cannon_spear_12_clock
			clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing,1);
			status_heal(bl,healing,0,0);
			sc_timer_next(8000 + tick);
			return 0;
		}
		break;
	case STATUS_MOONLIGHTSONATA:
		if( --(sce->val4) > 0 ) {
			int heal = status->max_hp * (sce->val1) / 100;
			int heal_sp = status->max_sp * (sce->val1) / 100;
			status_heal(bl, heal, heal_sp, 3);
			sc_timer_next(15000 + tick);
			return 0;
		}
		break;
	case STATUS_RENEW:
		if( --(sce->val4) > 0 ) {
			int heal = status->max_hp * (sce->val1) / 100;
			status_heal(bl, heal, 0, 3);
			sc_timer_next(10000 + tick);
			return 0;
		}
		break;

	case STATUS_SPHERE_1:
	case STATUS_SPHERE_2:
	case STATUS_SPHERE_3:
	case STATUS_SPHERE_4:
	case STATUS_SPHERE_5:
		if( --(sce->val4) >= 0 ) {
			sc_timer_next(1000 + tick);
			return 0;
		}
		break;

	case STATUS_STORE_SPELLBOOK:
		if( !status_charge(bl, 0, sce->val2) ) {
			int i;
			for(i = STATUS_SPELLBOOK1; i <= STATUS_MAXSPELLBOOK; i++) // Also remove stored spell as well.
				status_change_end(bl, (sc_type)i, INVALID_TIMER);
			break;
		}
		sc_timer_next(10000 + tick);
		return 0;

	case STATUS_CAMOUFLAGE:
		if (!status_charge(bl, 0, 7 - sce->val1))
			break;
		if (--sce->val4 >= 0)
			sce->val3++;
		sc_timer_next(1000 + tick);
		return 0;

	case STATUS_REPRODUCE:
		if(!status_charge(bl, 0, 1))
			break;
		sc_timer_next(1000+tick);
		return 0;

	case STATUS_STALK:
		if( --(sce->val4) >= 0 ) {
			if( !status_charge(bl, 0, 10 - sce->val1) )
				break;
			sc_timer_next(1000 + tick);
			return 0;
		}
		break;

	case STATUS_INVISIBILITY:
		if( !status_charge(bl, 0, (11 - 2 * sce->val1) * status->max_sp / 100) ) // 6% - skill_lv.
			break;
		sc_timer_next(1000 + tick);
		return 0;

	case STATUS_ULLREAGLETOTEM:
		if (--(sce->val4) > 0) {
			struct block_list *src = map_id2bl(sce->val2);
			if( !src )
				break;
			map_foreachinrange(skill_area_sub,bl,skill_get_splash(SK_RA_ULLREAGLETOTEM_ATK,sce->val1),BL_CHAR,
				src,SK_RA_ULLREAGLETOTEM_ATK,sce->val1,tick,BCT_ENEMY,skill_castend_damage_id);
			sc_timer_next(300 + tick);
			return 0;
		}
		break;
	}

	// If status has an interval and there is at least 100ms remaining time, wait for next interval
	if(interval > 0 && sc->data[type] && sce->val4 >= 100) {
		sc_timer_next(min(sce->val4,interval)+tick);
		sce->val4 -= interval;
		if (dounlock)
			map_freeblock_unlock();
		return 0;
	}

	if (dounlock)
		map_freeblock_unlock();

	// Default for all non-handled control paths is to end the status
	return status_change_end( bl,type,tid );
}

/**
 * For each iteration of repetitive status
 * @param bl: Object [PC|MOB|HOM|MER|ELEM]
 * @param ap: va_list arguments (src, sce, type, tick)
 */
int status_change_timer_sub(struct block_list* bl, va_list ap)
{
	struct status_change* tsc;

	struct block_list* src = va_arg(ap,struct block_list*);
	struct status_change_entry* sce = va_arg(ap,struct status_change_entry*);
	enum sc_type type = (sc_type)va_arg(ap,int); // gcc: enum args get promoted to int
	t_tick tick = va_arg(ap,t_tick);

	if (status_isdead(bl))
		return 0;

	tsc = status_get_sc(bl);

	switch( type ) {
	case STATUS_IMPROVECONCENTRATION:
		status_change_end(bl, STATUS_HIDING, INVALID_TIMER);
		status_change_end(bl, STATUS_CLOAKING, INVALID_TIMER);
		status_change_end(bl, STATUS_CAMOUFLAGE, INVALID_TIMER);
		if (tsc && tsc->data[STATUS_STALK] && (sce && sce->val4 > 0 && sce->val4%2000 == 0) && // For every 2 seconds do the checking
			rnd()%100 < 100 - tsc->data[STATUS_STALK]->val1 * 10) // [100 - (Skill Level x 10)] %
				status_change_end(bl, STATUS_STALK, INVALID_TIMER);
		break;
	case STATUS_RUWACH: // Reveal hidden target and deal little dammages if enemy
		if (tsc && (tsc->data[STATUS_HIDING] || tsc->data[STATUS_CLOAKING] ||
				tsc->data[STATUS_CAMOUFLAGE] || tsc->data[STATUS_INVISIBILITY] || tsc->data[STATUS_STALK] )) {
			status_change_end(bl, STATUS_HIDING, INVALID_TIMER);
			status_change_end(bl, STATUS_CLOAKING, INVALID_TIMER);
			status_change_end(bl, STATUS_CAMOUFLAGE, INVALID_TIMER);
			if(battle_check_target( src, bl, BCT_ENEMY ) > 0)
				skill_attack(BF_MAGIC,src,src,bl,SK_AL_RUWACH,1,tick,0);
		}
		break;
	case STATUS_CLOSECONFINE:
		// Lock char has released the hold on everyone...
		if (tsc && tsc->data[STATUS_CLOSECONFINE2] && tsc->data[STATUS_CLOSECONFINE2]->val2 == src->id) {
			tsc->data[STATUS_CLOSECONFINE2]->val2 = 0;
			status_change_end(bl, STATUS_CLOSECONFINE2, INVALID_TIMER);
		}
		break;
	}

	return 0;
}

/**
 * Clears buffs/debuffs on an object
 * @param bl: Object to clear [PC|MOB|HOM|MER|ELEM]
 * @param type: Type to remove
 *  SCCB_BUFFS: Clear Buffs
 *  SCCB_DEBUFFS: Clear Debuffs
 *  SCCB_REFRESH: Clear specific debuffs through RK_REFRESH
 *  SCCB_CHEM_PROTECT: Clear SK_CR_CP_ARMOR/HELM/SHIELD/WEAPON
 *  SCCB_LUXANIMA: Bonus Script removed through RK_LUXANIMA
 */
void status_change_clear_buffs(struct block_list* bl, uint8 type)
{
	int i;
	struct status_change *sc= status_get_sc(bl);

	if (!sc || !sc->count)
		return;

	if (type&(SCCB_DEBUFFS|SCCB_REFRESH)) // Debuffs
		for (i = STATUS_COMMON_MIN; i <= STATUS_COMMON_MAX; i++)
			status_change_end(bl, (sc_type)i, INVALID_TIMER);

	for( i = STATUS_COMMON_MAX+1; i < SC_MAX; i++ ) {
		if(!sc->data[i])
			continue;

		switch (i) {
			// Stuff that cannot be removed
			case STATUS_WEIGHT50:
			case STATUS_WEIGHT90:
			
			case STATUS_NOCHAT:
			case STATUS_GRAPPLE:
			case STATUS_STRFOOD:
			case STATUS_AGIFOOD:
			case STATUS_VITFOOD:
			case STATUS_INTFOOD:
			case STATUS_DEXFOOD:
			case STATUS_LUKFOOD:
			case STATUS_STONESKIN:
			case STATUS_VIGOR:
			
			case STATUS_PUSHCART:
			case STATUS_ALL_RIDING:
			// Clans
			case STATUS_CLAN_INFO:
			case STATUS_SWORDCLAN:
			case STATUS_ARCWANDCLAN:
			case STATUS_GOLDENMACECLAN:
			case STATUS_CROSSBOWCLAN:
			case STATUS_JUMPINGCLAN:
				continue;
			// Chemical Protection is only removed by some skills
			case STATUS_CP_WEAPON:
			case STATUS_CP_SHIELD:
			case STATUS_CP_ARMOR:
			case STATUS_CP_HELM:
				if(!(type&SCCB_CHEM_PROTECT))
					continue;
				break;
			
			case STATUS_QUAGMIRE:
			case STATUS_DECREASEAGI:
			case STATUS_MINDGAMES:
			case STATUS_STRIPWEAPON:
			case STATUS_STRIPSHIELD:
			case STATUS_STRIPARMOR:
			case STATUS_STRIPHELM:
				if (!(type&SCCB_DEBUFFS))
					continue;
				break;
			// The rest are buffs that can be removed.
			case STATUS_FRENZY:
				if (!(type&SCCB_BUFFS))
					continue;
				sc->data[i]->val2 = 0;
				break;
			default:
				if (!(type&SCCB_BUFFS))
					continue;
				break;
		}
		status_change_end(bl, (sc_type)i, INVALID_TIMER);
	}

	//Removes bonus_script
	if (bl->type == BL_PC) {
		i = 0;
		if (type&SCCB_BUFFS)    i |= BSF_REM_BUFF;
		if (type&SCCB_DEBUFFS)  i |= BSF_REM_DEBUFF;
		if (type&SCCB_REFRESH)  i |= BSF_REM_ON_REFRESH;
		if (type&SCCB_LUXANIMA) i |= BSF_REM_ON_LUXANIMA;
		pc_bonus_script_clear(BL_CAST(BL_PC,bl),i);
	}

	// Cleaning all extras vars
	sc->comet_x = 0;
	sc->comet_y = 0;
#ifndef RENEWAL
	sc->sg_counter = 0;
#endif
	sc->bs_counter = 0;

	return;
}

/**
 * Infect a user with status effects (SC_DEADLYINFECT)
 * @param src: Object initiating change on bl [PC|MOB|HOM|MER|ELEM]
 * @param bl: Object to change
 * @param type: 0 - Shadow Chaser attacking, 1 - Shadow Chaser being attacked
 * @return 1: Success 0: Fail
 */
int status_change_spread(struct block_list *src, struct block_list *bl, bool type)
{
	int i, flag = 0;
	struct status_change *sc = status_get_sc(src);
	const struct TimerData *timer = NULL;
	t_tick tick;
	struct status_change_data data;

	if( !sc || !sc->count )
		return 0;

	tick = gettick();

	// Status Immunity resistance
	if (status_bl_has_mode(src,MD_STATUSIMMUNE) || status_bl_has_mode(bl,MD_STATUSIMMUNE))
		return 0;

	for( i = STATUS_COMMON_MIN; i < SC_MAX; i++ ) {
		if( !sc->data[i] || i == STATUS_COMMON_MAX )
			continue;
		if (sc->data[i]->timer != INVALID_TIMER) {
			timer = get_timer(sc->data[i]->timer);
			if (timer == NULL || timer->func != status_change_timer || DIFF_TICK(timer->tick, tick) < 0)
				continue;
		}

		switch( i ) {
			// Debuffs that can be spread.
			// NOTE: We'll add/delete SCs when we are able to confirm it.
			case STATUS_CURSE:
			case STATUS_SILENCE:
			case STATUS_CONFUSION:
			case STATUS_BLIND:
			case STATUS_DECREASEAGI:
				if (sc->data[i]->timer != INVALID_TIMER)
					data.tick = DIFF_TICK(timer->tick, tick);
				else
					data.tick = INFINITE_TICK;
				break;
		
			case STATUS_POISON:
			case STATUS_DPOISON:
			case STATUS_BLEEDING:
				if (sc->data[i]->timer != INVALID_TIMER)
					data.tick = DIFF_TICK(timer->tick, tick) + sc->data[i]->val4;
				else
					data.tick = INFINITE_TICK;
				break;
			default:
				continue;
		}
		if( i ) {
			data.val1 = sc->data[i]->val1;
			data.val2 = sc->data[i]->val2;
			data.val3 = sc->data[i]->val3;
			data.val4 = sc->data[i]->val4;
			status_change_start(src,bl,(sc_type)i,10000,data.val1,data.val2,data.val3,data.val4,data.tick,SCSTART_NOAVOID|SCSTART_NOTICKDEF|SCSTART_NORATEDEF);
			flag = 1;
		}
	}

	return flag;
}

/**
 * Applying natural heal bonuses (sit, skill, homun, etc...)
 * TODO: the va_list doesn't seem to be used, safe to remove?
 * @param bl: Object applying bonuses to [PC|HOM|MER|ELEM]
 * @param args: va_list arguments
 * @return which regeneration bonuses have been applied (flag)
 */
static t_tick naturSK_AL_HEAL_prev_tick,naturSK_AL_HEAL_diff_tick;
static int status_naturSK_AL_HEAL(struct block_list* bl, va_list args)
{
	struct regen_data *regen;
	struct status_data *status;
	struct status_change *sc;
	struct unit_data *ud;
	struct view_data *vd = NULL;
	struct regen_data_sub *sregen;
	struct map_session_data *sd;
	int rate, multi = 1, flag;

	regen = status_get_regen_data(bl);
	if (!regen)
		return 0;
	status = status_get_status_data(bl);
	sc = status_get_sc(bl);
	if (sc && !sc->count)
		sc = NULL;
	sd = BL_CAST(BL_PC,bl);

	flag = regen->flag;
	if (flag&RGN_HP && (status->hp >= status->max_hp || regen->state.block&1))
		flag &= ~(RGN_HP|RGN_SHP);
	if (flag&RGN_SP && (status->sp >= status->max_sp || regen->state.block&2))
		flag &= ~(RGN_SP|RGN_SSP);

	if (flag && (
		status_isdead(bl) ||
		(sc && (sc->option&(OPTION_HIDE|OPTION_CLOAK|OPTION_CHASEWALK) || sc->data[STATUS_INVISIBILITY]))
	))
		flag = RGN_NONE;

	if (sd) {
		if (sd->hp_loss.value || sd->sp_loss.value)
			pc_bleeding(sd, naturSK_AL_HEAL_diff_tick);
		if (sd->hp_regen.value || sd->sp_regen.value || sd->percent_hp_regen.value || sd->percent_sp_regen.value)
			pc_regen(sd, naturSK_AL_HEAL_diff_tick);
	}

	if(flag&(RGN_SHP|RGN_SSP) && regen->ssregen &&
		(vd = status_get_viewdata(bl)) && vd->dead_sit == 2)
	{ // Apply sitting regen bonus.
		sregen = regen->ssregen;
		if(flag&(RGN_SHP)) { // Sitting HP regen
			rate = (int)(naturSK_AL_HEAL_diff_tick * (sregen->rate.hp / 100.));
			if (regen->state.overweight)
				rate >>= 1; // Half as fast when overweight.
			sregen->tick.hp += rate;
			while(sregen->tick.hp >= (unsigned int)battle_config.natural_heal_skill_interval) {
				sregen->tick.hp -= battle_config.natural_heal_skill_interval;
				if(status_heal(bl, sregen->hp, 0, 3) < sregen->hp) { // Full
					flag &= ~(RGN_HP|RGN_SHP);
					break;
				}
			}
		}
		if(flag&(RGN_SSP)) { // Sitting SP regen
			rate = (int)(naturSK_AL_HEAL_diff_tick * (sregen->rate.sp / 100.));
			if (regen->state.overweight)
				rate >>= 1; // Half as fast when overweight.
			sregen->tick.sp += rate;
			while(sregen->tick.sp >= (unsigned int)battle_config.natural_heal_skill_interval) {
				sregen->tick.sp -= battle_config.natural_heal_skill_interval;
				if(status_heal(bl, 0, sregen->sp, 3) < sregen->sp) { // Full
					flag &= ~(RGN_SP|RGN_SSP);
					break;
				}
			}
		}
	}

	if (flag && regen->state.overweight)
		flag = RGN_NONE;

	ud = unit_bl2ud(bl);

	if (flag&(RGN_HP|RGN_SHP|RGN_SSP) && ud && ud->walktimer != INVALID_TIMER) {
		flag &= ~(RGN_SHP|RGN_SSP);
		if(!regen->state.walk)
			flag &= ~RGN_HP;
	}

	if (!flag)
		return 0;

	if (flag&(RGN_HP|RGN_SP)) {
		if(!vd)
			vd = status_get_viewdata(bl);
		if(vd && vd->dead_sit == 2)
			multi += 1; //This causes the interval to be halved
		if(regen->state.gc)
			multi += 1; //This causes the interval to be halved
	}

	// Natural Hp regen
	if (flag&RGN_HP) {
		rate = (int)(naturSK_AL_HEAL_diff_tick * (regen->rate.hp/100. * multi));
		if (ud && ud->walktimer != INVALID_TIMER)
			rate /= 2;
		// Homun HP regen fix (they should regen as if they were sitting (twice as fast)
		if(bl->type == BL_HOM)
			rate *= 2;

		regen->tick.hp += rate;

		if(regen->tick.hp >= (unsigned int)battle_config.natural_heal_hp_interval) {
			int val = 0;
			do {
				val += regen->hp;
				regen->tick.hp -= battle_config.natural_heal_hp_interval;
			} while(regen->tick.hp >= (unsigned int)battle_config.natural_heal_hp_interval);
			if (status_heal(bl, val, 0, 1) < val)
				flag &= ~RGN_SHP; // Full.
		}
	}

	// Natural SP regen
	if(flag&RGN_SP) {
		rate = (int)(naturSK_AL_HEAL_diff_tick * (regen->rate.sp/100. * multi));
		// Homun SP regen fix (they should regen as if they were sitting (twice as fast)
		if(bl->type==BL_HOM)
			rate *= 2;
		regen->tick.sp += rate;

		if(regen->tick.sp >= (unsigned int)battle_config.natural_heal_sp_interval) {
			int val = 0;
			do {
				val += regen->sp;
				regen->tick.sp -= battle_config.natural_heal_sp_interval;
			} while(regen->tick.sp >= (unsigned int)battle_config.natural_heal_sp_interval);
			if (status_heal(bl, 0, val, 1) < val)
				flag &= ~RGN_SSP; // full.
		}
	}

	if (!regen->sregen)
		return flag;

	// Skill regen
	sregen = regen->sregen;

	if(flag&RGN_SHP) { // Skill HP regen
		sregen->tick.hp += (int)(naturSK_AL_HEAL_diff_tick * (sregen->rate.hp / 100.));

		while(sregen->tick.hp >= (unsigned int)battle_config.natural_heal_skill_interval) {
			sregen->tick.hp -= battle_config.natural_heal_skill_interval;
			if(status_heal(bl, sregen->hp, 0, 3) < sregen->hp)
				break; // Full
		}
	}
	if(flag&RGN_SSP) { // Skill SP regen
		sregen->tick.sp += (int)(naturSK_AL_HEAL_diff_tick * (sregen->rate.sp /100.));
		while(sregen->tick.sp >= (unsigned int)battle_config.natural_heal_skill_interval) {
			int val = sregen->sp;
			if (sd && sd->state.doridori) {
				val *= 2;
				sd->state.doridori = 0;
				if (
					(sd->class_&MAPID_UPPERMASK) == MAPID_STAR_GLADIATOR &&
					rnd()%10000 < battle_config.sg_angel_skill_ratio
				) { // Angel of the Sun/Moon/Star
					clif_feel_hate_reset(sd);
					pc_resethate(sd);
					pc_resetfeel(sd);
				}
			}
			sregen->tick.sp -= battle_config.natural_heal_skill_interval;
			if(status_heal(bl, 0, val, 3) < val)
				break; // Full
		}
	}
	return flag;
}

/**
 * Natural heal main timer
 * @param tid: Timer ID
 * @param tick: Current tick (time)
 * @param id: Object ID to heal
 * @param data: data pushed through timer function
 * @return 0
 */
static TIMER_FUNC(status_naturSK_AL_HEAL_timer){
	naturSK_AL_HEAL_diff_tick = DIFF_TICK(tick,naturSK_AL_HEAL_prev_tick);
	map_foreachregen(status_naturSK_AL_HEAL);
	naturSK_AL_HEAL_prev_tick = tick;
	return 0;
}

/**
 * Get the chance to upgrade a piece of equipment
 * @param wlv: The weapon type of the item to refine (see see enum refine_type)
 * @param refine: The target's refine level
 * @return The chance to refine the item, in percent (0~100)
 */
int status_get_refine_chance(enum refine_type wlv, int refine, bool enriched)
{
	if ( refine < 0 || refine >= MAX_REFINE)
		return 0;
	
	int type = enriched ? 1 : 0;
	if (battle_config.event_refine_chance)
		type |= 2;

	return refine_info[wlv].chance[type][refine];
}

/**
 * Check if status is disabled on a map
 * @param type: Status Change data
 * @param mapIsVS: If the map is a map_flag_vs type
 * @param mapisPVP: If the map is a PvP type
 * @param mapIsGVG: If the map is a map_flag_gvg type
 * @param mapIsBG: If the map is a Battleground type
 * @param mapZone: Map Zone type
 * @param mapIsTE: If the map us WOE TE
 * @return True - SC disabled on map; False - SC not disabled on map/Invalid SC
 */
static bool status_change_isDisabledOnMap_(sc_type type, bool mapIsVS, bool mapIsPVP, bool mapIsGVG, bool mapIsBG, unsigned int mapZone, bool mapIsTE)
{
	if (type <= STATUS_NONE || type >= SC_MAX)
		return false;

	if ((!mapIsVS && SCDisabled[type]&1) ||
		(mapIsPVP && SCDisabled[type]&2) ||
		(mapIsGVG && SCDisabled[type]&4) ||
		(mapIsBG && SCDisabled[type]&8) ||
		(mapIsTE && SCDisabled[type]&16) ||
		(SCDisabled[type]&(mapZone)))
	{
		return true;
	}

	return false;
}

/**
 * Clear a status if it is disabled on a map
 * @param bl: Block list data
 * @param sc: Status Change data
 */
void status_change_clear_onChangeMap(struct block_list *bl, struct status_change *sc)
{
	nullpo_retv(bl);

	if (sc && sc->count) {
		struct map_data *mapdata = map_getmapdata(bl->m);
		unsigned short i;
		bool mapIsVS = mapdata_flag_vs2(mapdata);
		bool mapIsPVP = mapdata->flag[MF_PVP] != 0;
		bool mapIsGVG = mapdata_flag_gvg2_no_te(mapdata);
		bool mapIsBG = mapdata->flag[MF_BATTLEGROUND] != 0;
		bool mapIsTE = mapdata_flag_gvg2_te(mapdata);

		for (i = 0; i < SC_MAX; i++) {
			if (!sc->data[i] || !SCDisabled[i])
				continue;

			if (status_change_isDisabledOnMap_((sc_type)i, mapIsVS, mapIsPVP, mapIsGVG, mapIsBG, mapdata->zone, mapIsTE))
				status_change_end(bl, (sc_type)i, INVALID_TIMER);
		}
	}
}

/**
 * Read status_disabled.txt file
 * @param str: Fields passed from sv_readdb
 * @param columns: Columns passed from sv_readdb function call
 * @param current: Current row being read into SCDisabled array
 * @return True - Successfully stored, False - Invalid SC
 */
static bool status_readdb_status_disabled(char **str, int columns, int current)
{
	// int64 type = STATUS_NONE;

	// if (ISDIGIT(str[0][0]))
	// 	type = atoi(str[0]);
	// else {
	// 	if (!script_get_constant(str[0],&type))
	// 		type = STATUS_NONE;
	// }

	// if (type <= STATUS_NONE || type >= SC_MAX) {
	// 	ShowError("status_readdb_status_disabled: Invalid SC with type %s.\n", str[0]);
	// 	return false;
	// }

	// SCDisabled[type] = (unsigned int)atol(str[1]);
	return true;
}

/**
 * Reads and parses an entry from the refine_db
 * @param node: The YAML node containing the entry
 * @param refine_info_index: The sequential index of the current entry
 * @param file_name: File name for displaying only
 * @return True on success or false on failure
 */
static bool status_yaml_readdb_refine_sub(const YAML::Node &node, int refine_info_index, const std::string &file_name) {
	if (refine_info_index < 0 || refine_info_index >= REFINE_TYPE_MAX)
		return false;

	int bonus_per_level = node["StatsPerLevel"].as<int>();
	int random_bonus_start_level = node["RandomBonusStartLevel"].as<int>();
	int random_bonus = node["RandomBonusValue"].as<int>();

	if (file_name.find("import") != std::string::npos) { // Import file, reset refine bonus before calculation
		for (int refine_level = 0; refine_level < MAX_REFINE; ++refine_level)
			refine_info[refine_info_index].bonus[refine_level] = 0;
	}

	const YAML::Node &costs = node["Costs"];

	for (const auto costit : costs) {
		int64 idx_tmp = 0;
		const YAML::Node &type = costit;
		int idx = 0, price;
		t_itemid material;
		const std::string keys[] = { "Type", "Price", "Material" };

		for (int i = 0; i < ARRAYLENGTH(keys); i++) {
			if (!type[keys[i]].IsDefined())
				ShowWarning("status_yaml_readdb_refine_sub: Invalid refine cost with undefined " CL_WHITE "%s" CL_RESET "in file" CL_WHITE "%s" CL_RESET ".\n", keys[i].c_str(), file_name.c_str());
		}

		std::string refine_cost_const = type["Type"].as<std::string>();
		if (ISDIGIT(refine_cost_const[0]))
			idx = atoi(refine_cost_const.c_str());
		else {
			script_get_constant(refine_cost_const.c_str(), &idx_tmp);
			idx = static_cast<int>(idx_tmp);
		}
		price = type["Price"].as<int>();
		// TODO: item id verification
		material = type["Material"].as<t_itemid>();

		refine_info[refine_info_index].cost[idx].nameid = material;
		refine_info[refine_info_index].cost[idx].zeny = price;
	}

	const YAML::Node &rates = node["Rates"];

	for (const auto rateit : rates) {
		const YAML::Node &level = rateit;
		int refine_level = level["Level"].as<int>() - 1;

		if (refine_level >= MAX_REFINE)
			continue;

		if (level["NormalChance"].IsDefined())
			refine_info[refine_info_index].chance[REFINE_CHANCE_NORMAL][refine_level] = level["NormalChance"].as<int>();
		if (level["EnrichedChance"].IsDefined())
			refine_info[refine_info_index].chance[REFINE_CHANCE_ENRICHED][refine_level] = level["EnrichedChance"].as<int>();
		if (level["EventNormalChance"].IsDefined())
			refine_info[refine_info_index].chance[REFINE_CHANCE_EVENT_NORMAL][refine_level] = level["EventNormalChance"].as<int>();
		if (level["EventEnrichedChance"].IsDefined())
			refine_info[refine_info_index].chance[REFINE_CHANCE_EVENT_ENRICHED][refine_level] = level["EventEnrichedChance"].as<int>();
		if (level["Bonus"].IsDefined())
			refine_info[refine_info_index].bonus[refine_level] = level["Bonus"].as<int>();

		if (refine_level >= random_bonus_start_level - 1)
			refine_info[refine_info_index].randombonus_max[refine_level] = random_bonus * (refine_level - random_bonus_start_level + 2);
	}
	for (int refine_level = 0; refine_level < MAX_REFINE; ++refine_level)
		refine_info[refine_info_index].bonus[refine_level] += bonus_per_level + (refine_level > 0 ? refine_info[refine_info_index].bonus[refine_level - 1] : 0);

	return true;
}

/**
 * Loads refine values from the refine_db
 * @param directory: Location of refine_db file
 * @param file: File name
 */
static void status_yaml_readdb_refine(const std::string &directory, const std::string &file) {
	int count = 0;
	const std::string labels[] = { "Armor", "WeaponLv1", "WeaponLv2", "WeaponLv3", "WeaponLv4", "Shadow" };
	const std::string current_file = directory + "/" + file;
	YAML::Node config;

	try {
		config = YAML::LoadFile(current_file);
	}
	catch (...) {
		ShowError("Failed to read '" CL_WHITE "%s" CL_RESET "'.\n", current_file.c_str());
		return;
	}

	for (int i = 0; i < ARRAYLENGTH(labels); i++) {
		const YAML::Node &node = config[labels[i]];

		if (node.IsDefined() && status_yaml_readdb_refine_sub(node, i, current_file))
			count++;
	}
	//ShowStatus("Done reading '" CL_WHITE "%d" CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "'.\n", count, current_file.c_str());
}

/**
 * Returns refine cost (zeny or item) for a weapon level.
 * @param weapon_lv Weapon level
 * @param type Refine type (can be retrieved from refine_cost_type enum)
 * @param what true = returns zeny, false = returns item id
 * @return Refine cost for a weapon level
 */
int status_get_refine_cost(int weapon_lv, int type, bool what) {
	return what ? refine_info[weapon_lv].cost[type].zeny : refine_info[weapon_lv].cost[type].nameid;
}

/**
 * Read attribute fix database for attack calculations
 * Function stores information in the attr_fix_table
 * @return True
 */
static bool status_readdb_attrfix(const char *basedir,bool silent)
{
	FILE *fp;
	char line[512], path[512];
	int entries = 0;


	sprintf(path, "%s/attr_fix.txt", basedir);
	fp = fopen(path,"r");
	if (fp == NULL) {
		if (silent==0)
			ShowError("Can't read %s\n", path);
		return 1;
	}
	while (fgets(line, sizeof(line), fp)) {
		int lv, i, j;
		if (line[0] == '/' && line[1] == '/')
			continue;

		lv = atoi(line);
		if (!CHK_ELEMENT_LEVEL(lv))
			continue;

		for (i = 0; i < ELE_ALL;) {
			char *p;
			if (!fgets(line, sizeof(line), fp))
				break;
			if (line[0]=='/' && line[1]=='/')
				continue;

			for (j = 0, p = line; j < ELE_ALL && p; j++) {
				while (*p == 32) //skipping space (32=' ')
					p++;
                                //TODO seem unsafe to continue without check
				attr_fix_table[lv-1][i][j] = atoi(p);
				p = strchr(p,',');
				if(p)
					*p++=0;
			}

			i++;
		}
		entries++;
	}
	fclose(fp);
	//ShowStatus("Done reading '" CL_WHITE "%d" CL_RESET "' entries in '" CL_WHITE "%s" CL_RESET "'.\n", entries, path);
	return true;
}

/**
 * Sets defaults in tables and starts read db functions
 * sv_readdb reads the file, outputting the information line-by-line to
 * previous functions above, separating information by delimiter
 * DBs being read:
 *	attr_fix.txt: Attribute adjustment table for attacks
 *	size_fix.yml: Size adjustment table for weapons
 *	refine_db.txt: Refining data table
 * @return 0
 */
int status_readdb(void)
{
	int i, j, k;
	const char* dbsubpath[] = {
		"",
		"/" DBIMPORT,
		//add other path here
	};
	// Initialize databases to default

	memset(SCDisabled, 0, sizeof(SCDisabled));
	// refine_db.yml
	for(i=0;i<ARRAYLENGTH(refine_info);i++)
	{
		memset(refine_info[i].cost, 0, sizeof(struct refine_cost));
		for(j = 0; j < REFINE_CHANCE_TYPE_MAX; j++)
			for(k=0;k<MAX_REFINE; k++)
			{
				refine_info[i].chance[j][k] = 100;
				refine_info[i].bonus[k] = 0;
				refine_info[i].randombonus_max[k] = 0;
			}
	}
	// attr_fix.txt
	for(i=0;i<MAX_ELE_LEVEL;i++)
		for(j=0;j<ELE_ALL;j++)
			for(k=0;k<ELE_ALL;k++)
				attr_fix_table[i][j][k]=100;

	// read databases
	// path,filename,separator,mincol,maxcol,maxrow,func_parsor
	for(i=0; i<ARRAYLENGTH(dbsubpath); i++){
		size_t n1 = strlen(db_path)+strlen(dbsubpath[i])+1;
		size_t n2 = strlen(db_path)+strlen(DBPATH)+strlen(dbsubpath[i])+1;
		char* dbsubpath1 = (char*)aMalloc(n1+1);
		char* dbsubpath2 = (char*)aMalloc(n2+1);

		if(i==0) {
			safesnprintf(dbsubpath1,n1,"%s%s",db_path,dbsubpath[i]);
			safesnprintf(dbsubpath2,n2,"%s/%s%s",db_path,DBPATH,dbsubpath[i]);
		}
		else {
			safesnprintf(dbsubpath1,n1,"%s%s",db_path,dbsubpath[i]);
			safesnprintf(dbsubpath2,n1,"%s%s",db_path,dbsubpath[i]);
		}

		status_readdb_attrfix(dbsubpath2,i > 0); // !TODO use sv_readdb ?
		// sv_readdb(dbsubpath1, "status_disabled.txt", ',', 2, 2, -1, &status_readdb_status_disabled, i > 0);

		status_yaml_readdb_refine(dbsubpath2, "refine_db.yml");
		aFree(dbsubpath1);
		aFree(dbsubpath2);
	}

	size_fix_db.load();

	return 0;
}

/**
 * Status db init and destroy.
 */
int do_init_status(void)
{
	add_timer_func_list(status_change_timer,"status_change_timer");
	add_timer_func_list(status_naturSK_AL_HEAL_timer,"status_naturSK_AL_HEAL_timer");
	initChangeTables();
	initDummyData();
	status_readdb();
	naturSK_AL_HEAL_prev_tick = gettick();
	sc_data_ers = ers_new(sizeof(struct status_change_entry),"status.cpp::sc_data_ers",ERS_OPT_NONE);
	add_timer_interval(naturSK_AL_HEAL_prev_tick + NATURSK_AL_HEAL_INTERVAL, status_naturSK_AL_HEAL_timer, 0, 0, NATURSK_AL_HEAL_INTERVAL);
	return 0;
}
void do_final_status(void)
{
	ers_destroy(sc_data_ers);
}
