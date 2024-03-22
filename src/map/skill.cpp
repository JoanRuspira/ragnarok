// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "skill.hpp"

#include <array>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
#include "chrif.hpp"
#include "clif.hpp"
#include "date.hpp"
#include "elemental.hpp"
#include "guild.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "log.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "path.hpp"
#include "pc.hpp"
#include "pc_groups.hpp"
#include "pet.hpp"
#include "script.hpp"
#include "status.hpp"
#include "unit.hpp"

using namespace rathena;

#define SKILLUNITTIMER_INTERVAL	100
#define TIMERSKILL_INTERVAL	150

// ranges reserved for mapping skill ids to skilldb offsets
#define HM_SKILLRANGEMIN 700
#define HM_SKILLRANGEMAX HM_SKILLRANGEMIN + MAX_HOMUNSKILL
#define MC_SKILLRANGEMIN HM_SKILLRANGEMAX + 1
#define MC_SKILLRANGEMAX MC_SKILLRANGEMIN + MAX_MERCSKILL
#define EL_SKILLRANGEMIN MC_SKILLRANGEMAX + 1
#define EL_SKILLRANGEMAX EL_SKILLRANGEMIN + MAX_ELEMENTALSKILL
#define GD_SKILLRANGEMIN EL_SKILLRANGEMAX + 1
#define GD_SKILLRANGEMAX GD_SKILLRANGEMIN + MAX_GUILDSKILL
#if GD_SKILLRANGEMAX > 999
	#error GD_SKILLRANGEMAX is greater than 999
#endif

static uint16 skilldb_id2idx[(UINT16_MAX + 1)];	/// Skill ID to Index lookup: skill_index = skill_get_index(skill_id) - [FWI] 20160423 the whole index thing should be removed.
static uint16 skill_num = 1;			 		/// Skill count, also as last index

static struct eri *skill_unit_ers = NULL; //For handling skill_unit's [Skotlex]
static struct eri *skill_timer_ers = NULL; //For handling skill_timerskills [Skotlex]
static DBMap* bowling_db = NULL; // int mob_id -> struct mob_data*

DBMap* skillunit_db = NULL; // int id -> struct skill_unit*

/**
 * Skill Unit Persistency during endack routes (mostly for songs see bugreport:4574)
 */
DBMap* skillusave_db = NULL; // char_id -> struct skill_usave
struct skill_usave {
	uint16 skill_id, skill_lv;
};

struct s_skill_produce_db skill_produce_db[MAX_SKILL_PRODUCE_DB];
static unsigned short skill_produce_count;

struct s_skill_arrow_db skill_arrow_db[MAX_SKILL_ARROW_DB];
static unsigned short skill_arrow_count;

ReadingSpellbookDatabase reading_spellbook_db;

#define MAX_SKILL_CHANGEMATERIAL_DB 75
#define MAX_SKILL_CHANGEMATERIAL_SET 3
struct s_skill_changematerial_db {
	t_itemid nameid;
	unsigned short rate;
	unsigned short qty[MAX_SKILL_CHANGEMATERIAL_SET];
	unsigned short qty_rate[MAX_SKILL_CHANGEMATERIAL_SET];
};
struct s_skill_changematerial_db skill_changematerial_db[MAX_SKILL_CHANGEMATERIAL_DB];
static unsigned short skill_changematerial_count;


struct s_skill_unit_layout skill_unit_layout[MAX_SKILL_UNIT_LAYOUT];
int firewall_unit_pos;
int icewall_unit_pos;
int earthstrain_unit_pos;
int firerain_unit_pos;
int wallofthorn_unit_pos;

struct s_skill_nounit_layout skill_nounit_layout[MAX_SKILL_UNIT_LAYOUT2];
int overbrand_nounit_pos;
int overbrand_brandish_nounit_pos;

static char dir_ka = -1; // Holds temporary direction to the target for SR_KNUCKLEARROW

//Early declaration
bool skill_strip_equip(struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv);
static int skill_check_unit_range (struct block_list *bl, int x, int y, uint16 skill_id, uint16 skill_lv);
static int skill_check_unit_range2 (struct block_list *bl, int x, int y, uint16 skill_id, uint16 skill_lv, bool isNearNPC);
static int skill_destroy_trap( struct block_list *bl, va_list ap );
static int skill_check_condition_mob_master_sub (struct block_list *bl, va_list ap);
static bool skill_check_condition_sc_required(struct map_session_data *sd, unsigned short skill_id, struct s_skill_condition *require);
static bool skill_check_unit_movepos(uint8 check_flag, struct block_list *bl, short dst_x, short dst_y, int easy, bool checkpath);

// Use this function for splash skills that can't hit icewall when cast by players
static inline int splash_target(struct block_list* bl) {
	return ( bl->type == BL_MOB ) ? BL_SKILL|BL_CHAR : BL_CHAR;
}

uint16 SKILL_MAX_DB(void) {
	return skill_num;
}

/**
 * Get skill id from name
 * @param name
 * @return Skill ID of the skill, or 0 if not found.
 **/
uint16 skill_name2id(const char* name) {
	if (name == nullptr)
		return 0;

	for (const auto &it : skill_db) {
		if (strcmpi(it.second->name, name) == 0)
			return it.first;
	}

	return 0;
}

/**
 * Get skill index from skill_db array. The index is also being used for skill lookup in mmo_charstatus::skill[]
 * @param skill_id
 * @param silent If Skill is undefined, show error message!
 * @return Skill Index or 0 if not found/unset
 **/
uint16 skill_get_index_(uint16 skill_id, bool silent, const char *func, const char *file, int line) {
	uint16 idx = skilldb_id2idx[skill_id];

	if (!idx && skill_id != 0 && !silent)
		ShowError("Skill '%d' is undefined! %s:%d::%s\n", skill_id, file, line, func);
	return idx;
}

/**
 * Get Skill name
 * @param skill_id
 * @return AEGIS Skill name
 **/
const char* skill_get_name( uint16 skill_id ) {
	return skill_db.find(skill_id)->name;
}

/**
 * Get Skill name
 * @param skill_id
 * @return English Skill name
 **/
const char* skill_get_desc( uint16 skill_id ) {
	return skill_db.find(skill_id)->desc;
}

static bool skill_check(uint16 id) {
	if (id == 0 || skill_get_index(id) == 0)
		return false;
	return true;
}

#define skill_get(id, var) do {\
	if (!skill_check(id))\
		return 0;\
	return var;\
} while(0)

#define skill_get_lv(id, lv, arrvar) do {\
	if (!skill_check(id))\
		return 0;\
	int lv_idx = min(lv, MAX_SKILL_LEVEL) - 1;\
	if (lv > MAX_SKILL_LEVEL && arrvar[lv_idx] > 1 && lv_idx > 1) {\
		int a__ = arrvar[lv_idx - 2];\
		int b__ = arrvar[lv_idx - 1];\
		int c__ = arrvar[lv_idx];\
		return (c__ + ((lv - MAX_SKILL_LEVEL + 1) * (b__ - a__) / 2) + ((lv - MAX_SKILL_LEVEL) * (c__ - b__) / 2));\
	}\
	return arrvar[lv_idx];\
} while(0)

// Skill DB
e_damage_type skill_get_hit( uint16 skill_id )                     { if (!skill_check(skill_id)) return DMG_NORMAL; return skill_db.find(skill_id)->hit; }
int skill_get_inf( uint16 skill_id )                               { skill_get(skill_id, skill_db.find(skill_id)->inf); }
int skill_get_ele( uint16 skill_id , uint16 skill_lv )             { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->element); }
int skill_get_max( uint16 skill_id )                               { skill_get(skill_id, skill_db.find(skill_id)->max); }
int skill_get_range( uint16 skill_id , uint16 skill_lv )           { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->range); }
int skill_get_splash_( uint16 skill_id , uint16 skill_lv )         { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->splash);  }
int skill_get_num( uint16 skill_id ,uint16 skill_lv )              { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->num); }
int skill_get_cast( uint16 skill_id ,uint16 skill_lv )             { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->cast); }
int skill_get_delay( uint16 skill_id ,uint16 skill_lv )            { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->delay); }
int skill_get_walkdelay( uint16 skill_id ,uint16 skill_lv )        { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->walkdelay); }
int skill_get_time( uint16 skill_id ,uint16 skill_lv )             { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->upkeep_time); }
int skill_get_time2( uint16 skill_id ,uint16 skill_lv )            { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->upkeep_time2); }
int skill_get_castdef( uint16 skill_id )                           { skill_get(skill_id, skill_db.find(skill_id)->cast_def_rate); }
int skill_get_castcancel( uint16 skill_id )                        { skill_get(skill_id, skill_db.find(skill_id)->castcancel); }
int skill_get_maxcount( uint16 skill_id ,uint16 skill_lv )         { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->maxcount); }
int skill_get_blewcount( uint16 skill_id ,uint16 skill_lv )        { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->blewcount); }
int skill_get_castnodex( uint16 skill_id )                         { skill_get(skill_id, skill_db.find(skill_id)->castnodex); }
int skill_get_delaynodex( uint16 skill_id )                        { skill_get(skill_id, skill_db.find(skill_id)->delaynodex); }
int skill_get_nocast ( uint16 skill_id )                           { skill_get(skill_id, skill_db.find(skill_id)->nocast); }
int skill_get_type( uint16 skill_id )                              { skill_get(skill_id, skill_db.find(skill_id)->skill_type); }
int skill_get_unit_id ( uint16 skill_id )                          { skill_get(skill_id, skill_db.find(skill_id)->unit_id); }
int skill_get_unit_id2 ( uint16 skill_id )                         { skill_get(skill_id, skill_db.find(skill_id)->unit_id2); }
int skill_get_unit_interval( uint16 skill_id )                     { skill_get(skill_id, skill_db.find(skill_id)->unit_interval); }
int skill_get_unit_range( uint16 skill_id, uint16 skill_lv )       { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->unit_range); }
int skill_get_unit_target( uint16 skill_id )                       { skill_get(skill_id, skill_db.find(skill_id)->unit_target&BCT_ALL); }
int skill_get_unit_bl_target( uint16 skill_id )                    { skill_get(skill_id, skill_db.find(skill_id)->unit_target&BL_ALL); }
int skill_get_unit_layout_type( uint16 skill_id ,uint16 skill_lv ) { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->unit_layout_type); }
int skill_get_cooldown( uint16 skill_id, uint16 skill_lv )         { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->cooldown); }
#ifdef RENEWAL_CAST
int skill_get_fixed_cast( uint16 skill_id ,uint16 skill_lv )       { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->fixed_cast); }
#endif
// Skill requirements
int skill_get_hp( uint16 skill_id ,uint16 skill_lv )               { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->require.hp); }
int skill_get_mhp( uint16 skill_id ,uint16 skill_lv )              { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->require.mhp); }
int skill_get_sp( uint16 skill_id ,uint16 skill_lv )               { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->require.sp); }
int skill_get_hp_rate( uint16 skill_id, uint16 skill_lv )          { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->require.hp_rate); }
int skill_get_sp_rate( uint16 skill_id, uint16 skill_lv )          { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->require.sp_rate); }
int skill_get_zeny( uint16 skill_id ,uint16 skill_lv )             { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->require.zeny); }
int skill_get_weapontype( uint16 skill_id )                        { skill_get(skill_id, skill_db.find(skill_id)->require.weapon); }
int skill_get_ammotype( uint16 skill_id )                          { skill_get(skill_id, skill_db.find(skill_id)->require.ammo); }
int skill_get_ammo_qty( uint16 skill_id, uint16 skill_lv )         { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->require.ammo_qty); }
int skill_get_state( uint16 skill_id )                             { skill_get(skill_id, skill_db.find(skill_id)->require.state); }
int skill_get_status_count( uint16 skill_id )                      { skill_get(skill_id, skill_db.find(skill_id)->require.status.size()); }
int skill_get_spiritball( uint16 skill_id, uint16 skill_lv )       { skill_get_lv(skill_id, skill_lv, skill_db.find(skill_id)->require.spiritball); }

int skill_get_splash( uint16 skill_id , uint16 skill_lv ) {
	int splash = skill_get_splash_(skill_id, skill_lv);
	if (splash < 0)
		return AREA_SIZE;
	return splash;
}

bool skill_get_nk_(uint16 skill_id, std::vector<e_skill_nk> nk) {
	if( skill_id == 0 ){
		return false;
	}

	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);

	if (!skill)
		return false;

	for (const auto &nkit : nk) {
		if (skill->nk[nkit])
			return true;
	}

	return false;
}

bool skill_get_inf2_(uint16 skill_id, std::vector<e_skill_inf2> inf2) {
	if( skill_id == 0 ){
		return false;
	}

	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);

	if (!skill)
		return false;

	for (const auto &inf2it : inf2) {
		if (skill->inf2[inf2it])
			return true;
	}

	return false;
}

bool skill_get_unit_flag_(uint16 skill_id, std::vector<e_skill_unit_flag> unit) {
	if( skill_id == 0 ){
		return false;
	}

	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);

	if (!skill)
		return false;

	for (const auto &unitit : unit) {
		if (skill->unit_flag[unitit])
			return true;
	}

	return false;
}

int skill_tree_get_max(uint16 skill_id, int b_class)
{
	int i;
	b_class = pc_class2idx(b_class);

	ARR_FIND( 0, MAX_SKILL_TREE, i, skill_tree[b_class][i].skill_id == 0 || skill_tree[b_class][i].skill_id == skill_id );
	if( i < MAX_SKILL_TREE && skill_tree[b_class][i].skill_id == skill_id )
		return skill_tree[b_class][i].skill_lv;
	else
		return skill_get_max(skill_id);
}

int skill_frostjoke_scream(struct block_list *bl,va_list ap);
int skill_attack_area(struct block_list *bl,va_list ap);
struct skill_unit_group *skill_locate_element_field(struct block_list *bl); // [Skotlex]
int skill_graffitiremover(struct block_list *bl, va_list ap); // [Valaris]
int skill_greed(struct block_list *bl, va_list ap);
static int skill_cell_overlap(struct block_list *bl, va_list ap);
static int skill_trap_splash(struct block_list *bl, va_list ap);
struct skill_unit_group_tickset *skill_unitgrouptickset_search(struct block_list *bl,struct skill_unit_group *sg,t_tick tick);
static int skill_unit_onplace(struct skill_unit *src,struct block_list *bl,t_tick tick);
int skill_unit_onleft(uint16 skill_id, struct block_list *bl,t_tick tick);
static int skill_unit_effect(struct block_list *bl,va_list ap);
static int skill_bind_trap(struct block_list *bl, va_list ap);

e_cast_type skill_get_casttype (uint16 skill_id) {
	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);

	if( skill == nullptr ){
		return CAST_DAMAGE;
	}

	if (skill->inf&(INF_GROUND_SKILL))
		return CAST_GROUND;
	if (skill->inf&INF_SUPPORT_SKILL)
		return CAST_NODAMAGE;

	if (skill->inf&INF_SELF_SKILL) {
		if(skill->inf2[INF2_NOTARGETSELF])
			return CAST_DAMAGE; //Combo skill.
		return CAST_NODAMAGE;
	}
	if (skill->nk[NK_NODAMAGE])
		return CAST_NODAMAGE;
	return CAST_DAMAGE;
}

//Returns actual skill range taking into account attack range and SK_AC_OWL [Skotlex]
int skill_get_range2(struct block_list *bl, uint16 skill_id, uint16 skill_lv, bool isServer) {
	if( bl->type == BL_MOB && battle_config.mob_ai&0x400 )
		return 9; //Mobs have a range of 9 regardless of skill used.

	int32 range = skill_get_range(skill_id, skill_lv);

	if( range < 0 ) {
		if( battle_config.use_weapon_skill_range&bl->type )
			return status_get_range(bl);
		range *=-1;
	}

	if (isServer && range > 14) {
		range = 14; // Server-sided base range can't be above 14
	}

	std::bitset<INF2_MAX> inf2 = skill_db.find(skill_id)->inf2;

	if(inf2[INF2_ALTERRANGEVULTURE] || inf2[INF2_ALTERRANGESNAKEEYE] ){
		if( bl->type == BL_PC ) {
			if(inf2[INF2_ALTERRANGEVULTURE]) range += (pc_checkskill((TBL_PC*)bl, SK_AC_VULTURE) *2);
			// added to allow GS skills to be effected by the range of Snake Eyes [Reddozen]
			
		} else
			range += battle_config.mob_eye_range_bonus;
	}
	if( inf2[INF2_ALTERRANGERADIUS]  ){
		if( bl->type == BL_PC ) {
		
			if(inf2[INF2_ALTERRANGERADIUS]) range += pc_checkskill((TBL_PC*)bl, SK_WZ_RADIUS);
			
		}
	}

	if( !range && bl->type != BL_PC )
		return 9; // Enable non players to use self skills on others. [Skotlex]
	return range;
}

/** Copy Referral: dummy skills should point to their source.
 * @param skill_id Dummy skill ID
 * @return Real skill id if found
 **/
unsigned short skill_dummy2skill_id(unsigned short skill_id) {
	switch (skill_id) {
		case SK_PR_DUPLELUX_MELEE:
		case SK_PR_DUPLELUX_MAGIC:
			return SK_PR_DUPLELUX;
		case SK_SO_TETRAVORTEX_FIRE:
		case SK_SO_TETRAVORTEX_WATER:
		case SK_SO_TETRAVORTEX_WIND:
		case SK_SO_TETRAVORTEX_GROUND:
		case SK_SO_TETRAVORTEX_NEUTRAL:
			return SK_SO_TETRAVORTEX;
		/*case WL_SUMMON_ATK_FIRE:
			return SK_SO_SUMMONELEMENTALSPHERES;*/

		case SK_CL_SEVERERAINSTORM_MELEE:
			return SK_CL_SEVERERAINSTORM;
		case SK_CR_MANDRAKERAID_ATK:
			return SK_CR_MANDRAKERAID;
		case SK_CR_GEOGRAFIELD_ATK:
			return SK_CR_GEOGRAFIELD;
		case SK_RA_ULLREAGLETOTEM_ATK:
			return SK_RA_ULLREAGLETOTEM;
	}
	return skill_id;
}

/**
 * Check skill unit maxcount
 * @param src: Caster to check against
 * @param x: X location of skill
 * @param y: Y location of skill
 * @param skill_id: Skill used
 * @param skill_lv: Skill level used
 * @param type: Type of unit to check against for battle_config checks
 * @param display_failure: Display skill failure message
 * @return True on skill cast success or false on failure
 */
bool skill_pos_maxcount_check(struct block_list *src, int16 x, int16 y, uint16 skill_id, uint16 skill_lv, enum bl_type type, bool display_failure) {
	if (!src)
		return false;

	struct unit_data *ud = unit_bl2ud(src);
	struct map_session_data *sd = map_id2sd(src->id);
	int maxcount = 0;
	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);

	if (!(type&battle_config.skill_reiteration) && skill->unit_flag[UF_NOREITERATION] && skill_check_unit_range(src, x, y, skill_id, skill_lv)) {
		if (sd && display_failure)
			clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
		return false;
	}
	if (type&battle_config.skill_nofootset && skill->unit_flag[UF_NOFOOTSET] && skill_check_unit_range2(src, x, y, skill_id, skill_lv, false)) {
		if (sd && display_failure)
			clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
		return false;
	}
	if (type&battle_config.land_skill_limit && (maxcount = skill_get_maxcount(skill_id, skill_lv)) > 0) {
		for (int i = 0; i < MAX_SKILLUNITGROUP && ud->skillunit[i] && maxcount; i++) {
			if (ud->skillunit[i]->skill_id == skill_id)
				maxcount--;
		}
		if (maxcount == 0) {
			if (sd && display_failure)
				clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
			return false;
		}
	}

	return true;
}

/**
 * Calculates heal value of skill's effect
 * @param src: Unit casting heal
 * @param target: Target of src
 * @param skill_id: Skill ID used
 * @param skill_lv: Skill Level used
 * @param heal: True if it's the heal part or false if it's the damage part of the skill
 * @return modified heal value
 */
int skill_calc_heal(struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv, bool heal) {
	int skill, hp = 0;
#ifdef RENEWAL
	int hp_bonus = 0;
	double global_bonus = 1;
#endif
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	struct map_session_data *tsd = BL_CAST(BL_PC, target);
	struct status_change *sc, *tsc;

	sc = status_get_sc(src);
	tsc = status_get_sc(target);

	switch( skill_id ) {
		default:
			if (skill_lv >= battle_config.max_heal_lv)
				return battle_config.max_heal;
			/**
			 * Renewal Heal Formula
			 * Formula: ( [(Base Level + INT) / 5] x 30 ) x (Heal Level / 10) x (Modifiers) + MATK
			 */
			if (src->type == BL_ELEM) {
				struct map_session_data* sd2 = BL_CAST(BL_PC, battle_get_master(src));
				hp = (sd2->status.base_level + sd2->battle_status.int_) / 5 * 30 * skill_lv / 10;
			} else {
				hp = (status_get_lv(src) + status_get_int(src)) / 5 * 30 * skill_lv / 10;
			}			


			if (sd && ((skill = (pc_checkskill(sd, SK_SM_MEDITATE)*2)) > 0))
				hp_bonus += skill * 4;
			if (sd && tsd && sd->status.partner_id == tsd->status.char_id && (sd->class_&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE && sd->status.sex == 0)
				hp *= 2;
			break;
	}

	if( (!heal || (target && target->type == BL_MER)) )
		hp >>= 1;

	if (sd) {
		


	if (skill = pc_skillheal_bonus(sd, skill_id))
		hp_bonus += skill;
	}

	if (tsd && (skill = pc_skillheal2_bonus(tsd, skill_id)))
		hp_bonus += skill;

	
	

	if (hp_bonus)
		hp += hp * hp_bonus / 100;

	// MATK part of the RE heal formula [malufett]
	// Note: in this part matk bonuses from items or skills are not applied
	switch( skill_id ) {
		case SK_PR_SANCTUARIO:
			{
				int healing, matk = 0;
				struct status_data *status;
				status = status_get_status_data(&sd->bl);
				matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
				healing = (5* skill_lv) + (status_get_lv(src) * 3) + (status_get_int(src) * 3) + (matk * 3);
				return healing;
			}
			break;
		case SK_BI_EPICLESIS:
			{
				int healing, matk = 0;
				struct status_data *status;
				status = status_get_status_data(&sd->bl);
				matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
				healing = (400 * skill_lv) + (status_get_lv(src) * 4) + (status_get_int(src) * 4) + (matk * 4);
				return healing;
			}
			break;	
		default:
			{
				struct status_data *status = status_get_status_data(src);
				int min, max;

				min = status_base_matk_min(src, status, status_get_lv(src));
				max = status_base_matk_max(src, status, status_get_lv(src));
				if( status->rhw.matk > 0 ){
					int wMatk, variance;
					wMatk = status->rhw.matk;
					variance = wMatk * status->rhw.wlv / 10;
					min += wMatk - variance;
					max += wMatk + variance;
				}

				if( sc && sc->data[STATUS_HINDSIGHT] )
					min = max;

				if( sd && sd->right_weapon.overrefine > 0 ){
					min++;
					max += sd->right_weapon.overrefine - 1;
				}

				if(max > min)
					hp += min+rnd()%(max-min);
				else
					hp += min;
			}
	}



	if (heal && tsc && tsc->count) {
		uint8 penalty = 0;
		if (tsc->data[STATUS_NORECOVER_STATE])
			penalty = 100;
		if (penalty > 0) {
			penalty = cap_value(penalty, 1, 100);
			global_bonus *= (100 - penalty) / 100.f;
		}
	}
	hp = (int)(hp * global_bonus);

	return (heal) ? max(1, hp) : hp;
}

/**
 * Making Plagiarism and Reproduce check their own function
 * Previous prevention for NPC skills, Wedding skills, and INF3_DIS_PLAGIA are removed since we use skill_db.yml [Cydh]
 * @param sd: Player who will copy the skill
 * @param skill_id: Target skill
 * @return 0 - Cannot be copied; 1 - Can be copied by Plagiarism 2 - Can be copied by Reproduce
 * @author Aru - for previous check; Jobbie for class restriction idea; Cydh expands the copyable skill
 */
static int8 skill_isCopyable(struct map_session_data *sd, uint16 skill_id) {
	uint16 skill_idx = skill_get_index(skill_id);
	if (!skill_idx)
		return 0;

	// Only copy skill that player doesn't have or the skill is old clone
	if (sd->status.skill[skill_idx].id != 0 && sd->status.skill[skill_idx].flag != SKILL_FLAG_PLAGIARIZED)
		return 0;

	s_skill_copyable copyable = skill_db.find(skill_id)->copyable;

	//Plagiarism only able to copy skill while SC_PRESERVE is not active and skill is copyable by Plagiarism
	if (copyable.option & SKILL_COPY_PLAGIARISM && pc_checkskill(sd,SK_RG_PLAGIARISM) && sd->sc.data[STATUS_PLAGIARISM] && sd->sc.data[STATUS_PLAGIARISM]->val1)
		return 1;

	//Reproduce can copy skill if STATUS_REPRODUCE is active and the skill is copyable by Reproduce
	if (copyable.option & SKILL_COPY_REPRODUCE && pc_checkskill(sd,SK_ST_REPRODUCE) && sd->sc.data[STATUS_REPRODUCE] && sd->sc.data[STATUS_REPRODUCE]->val1)
		return 2;

	return 0;
}

/**
 * Check if the skill is ok to cast and when.
 * Done before skill_check_condition_castbegin, requirement
 * @param skill_id: Skill ID that casted
 * @param sd: Player who casted
 * @return true: Skill cannot be used, false: otherwise
 * @author [MouseJstr]
 */
bool skill_isNotOk(uint16 skill_id, struct map_session_data *sd)
{
	nullpo_retr(1,sd);

	if (pc_has_permission(sd,PC_PERM_SKILL_UNCONDITIONAL))
		return false; // can do any damn thing they want

	struct map_data *mapdata = map_getmapdata(sd->bl.m);

	if (mapdata->flag[MF_NOSKILL] && skill_id != SK_NV_EQSWITCH && !sd->skillitem) //Item skills bypass noskill
		return true;

	// Epoque:
	// This code will compare the player's attack motion value which is influenced by ASPD before
	// allowing a skill to be cast. This is to prevent no-delay ACT files from spamming skills such as
	// SK_AC_DOUBLESTRAFE which do not have a skill delay and are not regarded in terms of attack motion.
	if (!sd->state.autocast && sd->skillitem != skill_id && sd->canskill_tick &&
		DIFF_TICK(gettick(),sd->canskill_tick) < (sd->battle_status.amotion * (battle_config.skill_amotion_leniency) / 100))
	{// attempted to cast a skill before the attack motion has finished
		return true;
	}

	if (skill_blockpc_get(sd, skill_id) != -1){
		clif_skill_fail(sd,skill_id,USESKILL_FAIL_SKILLINTERVAL,0);
		return true;
	}

	/**
	 * It has been confirmed on a official server (thanks to Yommy) that item-cast skills bypass all mapflag restrictions
	 * Also, without this check, an exploit where an item casting + healing (or any other kind buff) isn't deleted after used on a restricted map
	 */
	if( sd->skillitem == skill_id && !sd->skillitem_keep_requirement && !sd->state.abra_flag)
		return false;

	uint32 skill_nocast = skill_get_nocast(skill_id);
	// Check skill restrictions [Celest]
	if( (skill_nocast&1 && !mapdata_flag_vs2(mapdata)) ||
		(skill_nocast&2 && mapdata->flag[MF_PVP]) ||
		(skill_nocast&4 && mapdata_flag_gvg2_no_te(mapdata)) ||
		(skill_nocast&8 && mapdata->flag[MF_BATTLEGROUND]) ||
		(skill_nocast&16 && mapdata_flag_gvg2_te(mapdata)) || // WOE:TE
		(mapdata->zone && skill_nocast&(mapdata->zone) && mapdata->flag[MF_RESTRICTED]) ){
			clif_msg(sd, SKILL_CANT_USE_AREA); // This skill cannot be used within this area
			return true;
	}

	if( sd->sc.data[STATUS_ALL_RIDING] )
		return true; //You can't use skills while in the new mounts (The client doesn't let you, this is to make cheat-safe)

	switch (skill_id) {
		
		case SK_MC_VENDING:
			if( map_getmapflag(sd->bl.m, MF_NOVENDING) ) {
				clif_displaymessage (sd->fd, msg_txt(sd,276)); // "You can't open a shop on this map"
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return true;
			}
			if( map_getcell(sd->bl.m,sd->bl.x,sd->bl.y,CELL_CHKNOVENDING) ) {
				clif_displaymessage (sd->fd, msg_txt(sd,204)); // "You can't open a shop on this cell."
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return true;
			}
			if( npc_isnear(&sd->bl) ) {
				// uncomment to send msg_txt.
				//char output[150];
				//sprintf(output, msg_txt(662), battle_config.min_npc_vendchat_distance);
				//clif_displaymessage(sd->fd, output);
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_THERE_ARE_NPC_AROUND,0);
				return true;
			}
		case SK_MG_ICEWALL:
			// noicewall flag [Valaris]
			if (mapdata->flag[MF_NOICEWALL]) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return true;
			}
			break;
		case GD_EMERGENCYCALL:
		case GD_ITEMEMERGENCYCALL:
			if (
				!(battle_config.emergency_call&((is_agit_start())?2:1)) ||
				!(battle_config.emergency_call&(mapdata_flag_gvg2(mapdata)?8:4)) ||
				(battle_config.emergency_call&16 && mapdata->flag[MF_NOWARPTO] && !(mapdata->flag[MF_GVG_CASTLE] || mapdata->flag[MF_GVG_TE_CASTLE]))
			)	{
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return true;
			}
			break;
		case SK_CL_DEEPSLEEPLULLABY:
			if( !mapdata_flag_vs(mapdata) ) {
				clif_skill_teleportmessage(sd,2); // This skill uses this msg instead of skill fails.
				return true;
			}
			break;

	}
	return false;
}

/**
 * Check if the homunculus skill is ok to be processed
 * After checking from Homunculus side, also check the master condition
 * @param hd: Homunculus who casted
 * @param skill_id: Skill ID casted
 * @param skill_lv: Skill level casted
 * @return true: Skill cannot be used, false: otherwise
 */
bool skill_isNotOk_hom(struct homun_data *hd, uint16 skill_id, uint16 skill_lv)
{
	
	return 1;
}

/**
 * Check if the mercenary skill is ok to be processed
 * After checking from Homunculus side, also check the master condition
 * @param skill_id: Skill ID that casted
 * @param md: Mercenary who casted
 * @return true: Skill cannot be used, false: otherwise
 */
bool skill_isNotOk_mercenary(uint16 skill_id, struct mercenary_data *md)
{


	return true;
}

/**
 * Check if the skill can be casted near NPC or not
 * @param src Object who casted
 * @param skill_id Skill ID that casted
 * @param skill_lv Skill Lv
 * @param pos_x Position x of the target
 * @param pos_y Position y of the target
 * @return true: Skill cannot be used, false: otherwise
 * @author [Cydh]
 */
bool skill_isNotOk_npcRange(struct block_list *src, uint16 skill_id, uint16 skill_lv, int pos_x, int pos_y) {
	if (!src)
		return false;

	if (src->type == BL_PC && pc_has_permission(BL_CAST(BL_PC,src),PC_PERM_SKILL_UNCONDITIONAL))
		return false;

	//if self skill
	if (skill_get_inf(skill_id)&INF_SELF_SKILL) {
		pos_x = src->x;
		pos_y = src->y;
	}

	if (pos_x <= 0) pos_x = src->x;
	if (pos_y <= 0) pos_y = src->y;

	return skill_check_unit_range2(src,pos_x,pos_y,skill_id,skill_lv,true) != 0;
}

struct s_skill_unit_layout *skill_get_unit_layout(uint16 skill_id, uint16 skill_lv, struct block_list* src, int x, int y)
{
	int pos = skill_get_unit_layout_type(skill_id,skill_lv);
	uint8 dir;

	if (pos < -1 || pos >= MAX_SKILL_UNIT_LAYOUT) {
		ShowError("skill_get_unit_layout: unsupported layout type %d for skill %d (level %d)\n", pos, skill_id, skill_lv);
		pos = cap_value(pos, 0, MAX_SQUARE_LAYOUT); // cap to nearest square layout
	}

	nullpo_retr(NULL, src);

	
	if (pos != -1) // simple single-definition layout
		return &skill_unit_layout[pos];

	dir = (src->x == x && src->y == y) ? 6 : map_calc_dir(src,x,y); // 6 - default aegis direction

	if (skill_id == SK_MG_ICEWALL)
		return &skill_unit_layout [icewall_unit_pos + dir];
	else if( skill_id == SK_AM_WILDTHORNS )
		return &skill_unit_layout[wallofthorn_unit_pos + dir];

	ShowError("skill_get_unit_layout: unknown unit layout for skill %d (level %d)\n", skill_id, skill_lv);
	return &skill_unit_layout[0]; // default 1x1 layout
}

struct s_skill_nounit_layout* skill_get_nounit_layout(uint16 skill_id, uint16 skill_lv, struct block_list* src, int x, int y, int dir)
{
	

	ShowError("skill_get_nounit_layout: unknown no-unit layout for skill %d (level %d)\n", skill_id, skill_lv);
	return &skill_nounit_layout[0];
}

/** Stores temporary values.
 * Common usages:
 * [0] holds number of targets in area
 * [1] holds the id of the original target
 * [2] counts how many targets have been processed. counter is added in skill_area_sub if the foreach function flag is: flag&(SD_SPLASH|SD_PREAMBLE)
 */
static int skill_area_temp[8];

int skill_onskillusage(struct map_session_data *sd, struct block_list *bl, uint16 skill_id, t_tick tick) {
	struct block_list *tbl;
	if( sd == NULL || !skill_id )
		return 0;
	if (skill_id == SK_PF_STASIS) {
		clif_soundeffectall(&sd->bl, "cd_presens_acies.wav", 0, AREA);
		clif_specialeffect(&sd->bl, 922, AREA);
	}
	for (auto &it : sd->autospell3) {
		int skill, skill_lv, type;

		if (it.flag != skill_id)
			continue;

		if (it.lock)
			continue;  // autospell already being executed

		skill = it.id;
		sd->state.autocast = 1; //set this to bypass sd->canskill_tick check

		if( skill_isNotOk((skill > 0) ? skill : skill*-1, sd) ) {
			sd->state.autocast = 0;
			continue;
		}

		sd->state.autocast = 0;

		if( skill >= 0 && bl == NULL )
			continue; // No target
		if( rnd()%1000 >= it.rate )
			continue;

		skill_lv = it.lv ? it.lv : 1;
		if( skill < 0 ) {
			tbl = &sd->bl;
			skill *= -1;
			skill_lv = 1 + rnd()%(-skill_lv); //random skill_lv
		}
		else
			tbl = bl;

		if ((type = skill_get_casttype(skill)) == CAST_GROUND) {
			if (!skill_pos_maxcount_check(&sd->bl, tbl->x, tbl->y, skill_id, skill_lv, BL_PC, false))
				continue;
		}
		if (battle_config.autospell_check_range &&
			!battle_check_range(bl, tbl, skill_get_range2(&sd->bl, skill, skill_lv, true)))
			continue;

		sd->state.autocast = 1;
		it.lock = true;
		skill_consume_requirement(sd,skill,skill_lv,1);
		switch( type ) {
			case CAST_GROUND:   skill_castend_pos2(&sd->bl, tbl->x, tbl->y, skill, skill_lv, tick, 0); break;
			case CAST_NODAMAGE: skill_castend_nodamage_id(&sd->bl, tbl, skill, skill_lv, tick, 0); break;
			case CAST_DAMAGE:   skill_castend_damage_id(&sd->bl, tbl, skill, skill_lv, tick, 0); break;
		}
		it.lock = false;
		sd->state.autocast = 0;
	}

	if( sd && !sd->autobonus3.empty() ) {
		for (auto &it : sd->autobonus3) {
			if (rnd()%1000 >= it.rate)
				continue;
			if (it.atk_type != skill_id)
				continue;
			pc_exeautobonus(sd, &sd->autobonus3, &it);
		}
	}

	return 1;
}

/* Splitted off from skill_additional_effect, which is never called when the
 * attack skill kills the enemy. Place in this function counter status effects
 * when using skills (eg: Asura's sp regen penalty, or counter-status effects
 * from cards) that will take effect on the source, not the target. [Skotlex]
 * Note: Currently this function only applies to Extremity Fist and BF_WEAPON
 * type of skills, so not every instance of skill_additional_effect needs a call
 * to this one.
 */
int skill_counter_additional_effect (struct block_list* src, struct block_list *bl, uint16 skill_id, uint16 skill_lv, int attack_type, t_tick tick)
{
	int rate;
	struct map_session_data *sd=NULL;
	struct map_session_data *dstsd=NULL;

	nullpo_ret(src);
	nullpo_ret(bl);

	if(skill_id > 0 && !skill_lv) return 0;	// don't forget auto attacks! - celest

	sd = BL_CAST(BL_PC, src);
	dstsd = BL_CAST(BL_PC, bl);

	if(dstsd && attack_type&BF_WEAPON) {	//Counter effects.
		enum sc_type type;
		unsigned int time;

		for (const auto &it : dstsd->addeff_atked) {
			rate = it.rate;
			if (attack_type&BF_LONG)
				rate += it.arrow_rate;
			if (!rate)
				continue;

			if ((it.flag&(ATF_LONG|ATF_SHORT)) != (ATF_LONG|ATF_SHORT)) {	//Trigger has range consideration.
				if((it.flag&ATF_LONG && !(attack_type&BF_LONG)) ||
					(it.flag&ATF_SHORT && !(attack_type&BF_SHORT)))
					continue; //Range Failed.
			}
			type = it.sc;
			time = it.duration;

			if (it.flag&ATF_TARGET && src != bl)
				status_change_start(src,src,type,rate,7,0,0,0,time,SCSTART_NONE);

			if (it.flag&ATF_SELF && !status_isdead(bl))
				status_change_start(src,bl,type,rate,7,0,0,0,time,SCSTART_NONE);
		}
	}

	
	if(sd && (sd->class_&MAPID_UPPERMASK) == MAPID_STAR_GLADIATOR &&
		map_getmapflag(sd->bl.m, MF_NOSUNMOONSTARMIRACLE) == 0)	//SG_MIRACLE [Komurka]
		
	//if(sd && skill_id && attack_type&BF_MAGIC && status_isdead(bl) &&
	// 	!(skill_get_inf(skill_id)&(INF_GROUND_SKILL|INF_SELF_SKILL)) &&
	//	(rate=pc_checkskill(sd,HW_SOULDRAIN))>0
	//){	//Soul Drain should only work on targetted spells [Skotlex]
	//	if (pc_issit(sd)) pc_setstand(sd, true); //Character stuck in attacking animation while 'sitting' fix. [Skotlex]
	//	clif_skill_nodamage(src,bl,HW_SOULDRAIN,rate,1);
	//	status_heal(src, 0, status_get_lv(bl)*(95+15*rate)/100, 2);
	//}

	if( sd && status_isdead(bl) ) {
		int sp = 0, hp = 0;
		if( (attack_type&(BF_WEAPON|BF_SHORT)) == (BF_WEAPON|BF_SHORT) ) {
			sp += sd->bonus.sp_gain_value;
			sp += sd->indexed_bonus.sp_gain_race[status_get_race(bl)] + sd->indexed_bonus.sp_gain_race[RC_ALL];
			hp += sd->bonus.hp_gain_value;
		}
		if( (attack_type&(BF_WEAPON|BF_LONG)) == (BF_WEAPON|BF_LONG) ) {
			sp += sd->bonus.long_sp_gain_value;
			hp += sd->bonus.long_hp_gain_value;
		}
		if( attack_type&BF_MAGIC ) {
			sp += sd->bonus.magic_sp_gain_value;
			hp += sd->bonus.magic_hp_gain_value;
		}
		if( hp || sp ) { // updated to force healing to allow healing through berserk
			status_heal(src, hp, sp, battle_config.show_hp_sp_gain ? 3 : 1);
		}
	}

	if (dstsd && !status_isdead(bl) && !(skill_id && skill_get_nk(skill_id, NK_NODAMAGE))) {
		struct status_change *sc = status_get_sc(bl);

		
	}

	// Trigger counter-spells to retaliate against damage causing skills.
	if(dstsd && !status_isdead(bl) && !dstsd->autospell2.empty() &&
		!(skill_id && skill_get_nk(skill_id, NK_NODAMAGE)))
	{
		struct block_list *tbl;
		struct unit_data *ud;
		int autospl_skill_id, autospl_skill_lv, autospl_rate, type;

		for (const auto &it : dstsd->autospell2) {
			if (!(((it.flag)&attack_type)&BF_WEAPONMASK &&
				  ((it.flag)&attack_type)&BF_RANGEMASK &&
				  ((it.flag)&attack_type)&BF_SKILLMASK))
				continue; // one or more trigger conditions were not fulfilled

			autospl_skill_id = (it.id > 0) ? it.id : -it.id;
			autospl_skill_lv = it.lv ? it.lv : 1;
			if (autospl_skill_lv < 0) autospl_skill_lv = 1+rnd()%(-autospl_skill_lv);

			autospl_rate = it.rate;
			//Physical range attacks only trigger autospells half of the time
			if ((attack_type&(BF_WEAPON|BF_LONG)) == (BF_WEAPON|BF_LONG))
				 autospl_rate>>=1;

			dstsd->state.autocast = 1;
			if ( skill_isNotOk(autospl_skill_id, dstsd) ) {
				dstsd->state.autocast = 0;
				continue;
			}
			dstsd->state.autocast = 0;

			if (rnd()%1000 >= autospl_rate)
				continue;

			tbl = (it.id < 0) ? bl : src;
			if ((type = skill_get_casttype(autospl_skill_id)) == CAST_GROUND) {
				if (!skill_pos_maxcount_check(bl, tbl->x, tbl->y, autospl_skill_id, autospl_skill_lv, BL_PC, false))
					continue;
			}

			if (!battle_check_range(bl, tbl, skill_get_range2(src, autospl_skill_id, autospl_skill_lv, true)) && battle_config.autospell_check_range)
				continue;

			dstsd->state.autocast = 1;
			skill_consume_requirement(dstsd,autospl_skill_id,autospl_skill_lv,1);
			switch (type) {
				case CAST_GROUND:
					skill_castend_pos2(bl, tbl->x, tbl->y, autospl_skill_id, autospl_skill_lv, tick, 0);
					break;
				case CAST_NODAMAGE:
					skill_castend_nodamage_id(bl, tbl, autospl_skill_id, autospl_skill_lv, tick, 0);
					break;
				case CAST_DAMAGE:
					skill_castend_damage_id(bl, tbl, autospl_skill_id, autospl_skill_lv, tick, 0);
					break;
			}
			dstsd->state.autocast = 0;
			//Set canact delay. [Skotlex]
			ud = unit_bl2ud(bl);
			if (ud) {
				autospl_rate = skill_delayfix(bl, autospl_skill_id, autospl_skill_lv);
				if (DIFF_TICK(ud->canact_tick, tick + autospl_rate) < 0){
					ud->canact_tick = i64max(tick + autospl_rate, ud->canact_tick);
					if ( battle_config.display_status_timers && dstsd )
						clif_status_change(bl, EFST_POSTDELAY, 1, autospl_rate, 0, 0, 0);
				}
			}
		}
	}

	//Autobonus when attacked
	if( dstsd && !status_isdead(bl) && !dstsd->autobonus2.empty() && !(skill_id && skill_get_nk(skill_id, NK_NODAMAGE)) ) {
		for (auto &it : dstsd->autobonus2) {
			if (rnd()%1000 >= it.rate)
				continue;
			if (!(((it.atk_type)&attack_type)&BF_WEAPONMASK &&
				  ((it.atk_type)&attack_type)&BF_RANGEMASK &&
				  ((it.atk_type)&attack_type)&BF_SKILLMASK))
				continue; // one or more trigger conditions were not fulfilled
			pc_exeautobonus(dstsd, &dstsd->autobonus2, &it);
		}
	}

	return 0;
}

/*=========================================================================
 Breaks equipment. On-non players causes the corresponding strip effect.
 - rate goes from 0 to 10000 (100.00%)
 - flag is a BCT_ flag to indicate which type of adjustment should be used
   (BCT_ENEMY/BCT_PARTY/BCT_SELF) are the valid values.
--------------------------------------------------------------------------*/
int skill_break_equip(struct block_list *src, struct block_list *bl, unsigned short where, int rate, int flag)
{
	const int where_list[4]     = {EQP_WEAPON, EQP_ARMOR, EQP_SHIELD, EQP_HELM};
	const enum sc_type scatk[4] = {STATUS_STRIPWEAPON, STATUS_STRIPARMOR, STATUS_STRIPSHIELD, STATUS_STRIPHELM};
	const enum sc_type scdef[4] = {STATUS_CP_WEAPON, STATUS_CP_ARMOR, STATUS_CP_SHIELD, STATUS_CP_HELM};
	struct status_change *sc = status_get_sc(bl);
	int i;
	TBL_PC *sd;
	sd = BL_CAST(BL_PC, bl);
	if (sc && !sc->count)
		sc = NULL;
	
	if (sd) {
		if (sd->bonus.unbreakable_equip)
			where &= ~sd->bonus.unbreakable_equip;
		if (sd->bonus.unbreakable)
			rate -= rate*sd->bonus.unbreakable/100;
		if (where&EQP_WEAPON) {
			switch (sd->status.weapon) {
				case W_FIST:	//Bare fists should not break :P
				case W_1HAXE:
				case W_2HAXE:
				case W_MACE: // Axes and Maces can't be broken [DracoRPG]
				case W_2HMACE:
				case W_STAFF:
				case W_2HSTAFF:
				case W_BOOK: //Rods and Books can't be broken [Skotlex]
				case W_HUUMA:
				case W_DOUBLE_AA:	// Axe usage during dual wield should also prevent breaking [Neutral]
				case W_DOUBLE_DA:
				case W_DOUBLE_SA:
					where &= ~EQP_WEAPON;
			}
		}
	}
	if (flag&BCT_ENEMY) {
		if (battle_config.equip_skill_break_rate != 100)
			rate = rate*battle_config.equip_skill_break_rate/100;
	} else if (flag&(BCT_PARTY|BCT_SELF)) {
		if (battle_config.equip_self_break_rate != 100)
			rate = rate*battle_config.equip_self_break_rate/100;
	}

	for (i = 0; i < 4; i++) {
		if (where&where_list[i]) {
			if (sc && sc->count && sc->data[scdef[i]])
				where&=~where_list[i];
			else if (rnd()%10000 >= rate)
				where&=~where_list[i];
			else if (!sd){
				//Cause Strip effect.
				int result;
				clif_specialeffect(bl, EF_SONIC_CLAW, AREA);
				result = rand() % 2;
				if(result == 0){
					sc_start(src, bl, STATUS_INCATKRATE, 100, -30, 5000);
				} else {
					sc_start(src, bl, STATUS_INCDEFRATE, 100, -30, 5000);
				}
			} 
		}
	}
	if (!where) //Nothing to break.
		return 0;
	if (sd) {
		for (i = 0; i < EQI_MAX; i++) {
			short j = sd->equip_index[i];
			if (j < 0 || sd->inventory.u.items_inventory[j].attribute == 1 || !sd->inventory_data[j])
				continue;

			switch(i) {
				case EQI_HEAD_TOP: //Upper Head
					flag = (where&EQP_HELM);
					break;
				case EQI_ARMOR: //Body
					flag = (where&EQP_ARMOR);
					break;
				case EQI_HAND_R: //Left/Right hands
				case EQI_HAND_L:
					flag = (
						(where&EQP_WEAPON && sd->inventory_data[j]->type == IT_WEAPON) ||
						(where&EQP_SHIELD && sd->inventory_data[j]->type == IT_ARMOR));
					break;
				case EQI_SHOES:
					flag = (where&EQP_SHOES);
					break;
				case EQI_GARMENT:
					flag = (where&EQP_GARMENT);
					break;
				default:
					continue;
			}
			if (flag) {
				sd->inventory.u.items_inventory[j].attribute = 1;
				clif_specialeffect(bl, EF_SONIC_CLAW, AREA);
				pc_unequipitem(sd, j, 3);
			}
		}
		clif_equiplist(sd);
	}

	return where; //Return list of pieces broken.
}

/**
 * Strip equipment from a target
 * @param src: Source of call
 * @param target: Target to strip
 * @param skill_id: Skill used
 * @param skill_lv: Skill level used
 * @return True on successful strip or false otherwise
 */
bool skill_strip_equip(struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv)
{
	nullpo_retr(false, src);
	nullpo_retr(false, target);

	struct status_change *tsc = status_get_sc(target);

	if (!tsc || tsc->option&OPTION_MADOGEAR) // Mado Gear cannot be divested [Ind]
		return false;

	const int pos[5]             = {EQP_WEAPON, EQP_SHIELD, EQP_ARMOR, EQP_HELM, EQP_ACC};
	const enum sc_type sc_atk[5] = {STATUS_STRIPWEAPON, STATUS_STRIPSHIELD, STATUS_STRIPARMOR, STATUS_STRIPHELM, STATUS_STRIPACCESSORY};
	const enum sc_type sc_def[5] = {STATUS_CP_WEAPON, STATUS_CP_SHIELD, STATUS_CP_ARMOR, STATUS_CP_HELM, STATUS_NONE};
	struct status_data *sstatus = status_get_status_data(src), *tstatus = status_get_status_data(target);
	int rate, time, location, mod = 100;


	switch (skill_id) { // Duration
		case SK_RG_STRIPACCESSORY:
		case SK_ST_FULLSTRIP:
			time = skill_get_time(skill_id, skill_lv);
			break;
	}

	switch (skill_id) { // Location
		case SK_ST_FULLSTRIP:
			location = EQP_WEAPON|EQP_SHIELD|EQP_ARMOR|EQP_HELM;
			break;
		case SK_RG_STRIPACCESSORY:
			location = EQP_ACC;
			break;
	}

	for (uint8 i = 0; i < ARRAYLENGTH(pos); i++) {
		if (location&pos[i] && sc_def[i] > STATUS_NONE && tsc->data[sc_def[i]])
			location &=~ pos[i];
	}
	if (!location)
		return false;

	for (uint8 i = 0; i < ARRAYLENGTH(pos); i++) {
		if (location&pos[i] && !sc_start(src, target, sc_atk[i], 100, skill_lv, time))
			location &=~ pos[i];
	}
	return location ? true : false;
}

/**
 * Used to knock back players, monsters, traps, etc
 * @param src Object that give knock back
 * @param target Object that receive knock back
 * @param count Number of knock back cell requested
 * @param dir Direction indicates the way OPPOSITE to the knockback direction (or -1 for default behavior)
 * @param flag
		BLOWN_DONT_SEND_PACKET - position update packets must not be sent
		BLOWN_IGNORE_NO_KNOCKBACK - ignores players' special_state.no_knockback
			These flags "return 'count' instead of 0 if target is cannot be knocked back":
		BLOWN_NO_KNOCKBACK_MAP - at WOE/BG map
		BLOWN_MD_KNOCKBACK_IMMUNE - if target is MD_KNOCKBACK_IMMUNE
		BLOWN_TARGET_NO_KNOCKBACK - if target has 'special_state.no_knockback'
		BLOWN_TARGET_BASILICA - if target is in Basilica area (Pre-Renewal)
 * @return Number of knocked back cells done
 */
short skill_blown(struct block_list* src, struct block_list* target, char count, int8 dir, enum e_skill_blown flag)
{
	int dx = 0, dy = 0;
	uint8 checkflag = 0;
	struct status_change *tsc = status_get_sc(target);
	enum e_unit_blown reason = UB_KNOCKABLE;

	nullpo_ret(src);
	nullpo_ret(target);

	if (!count)
		return count; // Actual knockback distance is 0.

	// Create flag needed in unit_blown_immune
	if(src != target)
		checkflag |= 0x1; // Offensive
	if(!(flag&BLOWN_IGNORE_NO_KNOCKBACK))
		checkflag |= 0x2; // Knockback type
	if(status_get_class_(src) == CLASS_BOSS)
		checkflag |= 0x4; // Boss attack

	// Get reason and check for flags
	reason = unit_blown_immune(target, checkflag);
	switch(reason) {
		case UB_NO_KNOCKBACK_MAP: return ((flag&BLOWN_NO_KNOCKBACK_MAP) ? count : 0); // No knocking back in WoE / BG
		case UB_MD_KNOCKBACK_IMMUNE: return ((flag&BLOWN_MD_KNOCKBACK_IMMUNE) ? count : 0); // Immune can't be knocked back
		case UB_TARGET_BASILICA: return ((flag&BLOWN_TARGET_BASILICA) ? count : 0); // Basilica caster can't be knocked-back by normal monsters.
		case UB_TARGET_NO_KNOCKBACK: return ((flag&BLOWN_TARGET_NO_KNOCKBACK) ? count : 0); // Target has special_state.no_knockback (equip)
		case UB_TARGET_TRAP: return count; // Trap cannot be knocked back
	}

	if (dir == -1) // <optimized>: do the computation here instead of outside
		dir = map_calc_dir(target, src->x, src->y); // Direction from src to target, reversed

	if (dir >= 0 && dir < 8) { // Take the reversed 'direction' and reverse it
		dx = -dirx[dir];
		dy = -diry[dir];
	}

	if (tsc) {
		
		if (tsc->data[STATUS_ROLLINGCUTTER])
			status_change_end(target, STATUS_ROLLINGCUTTER, INVALID_TIMER);
		
	}

	return unit_blown(target, dx, dy, count, flag);	// Send over the proper flag
}

// Checks if 'bl' should reflect back a spell cast by 'src'.
// type is the type of magic attack: 0: indirect (aoe), 1: direct (targetted)
// In case of success returns type of reflection, otherwise 0
//		1 - Regular reflection (Maya)
//		2 - SL_KAITE reflection
static int skill_magic_reflect(struct block_list* src, struct block_list* bl, int type)
{
	struct status_change *sc = status_get_sc(bl);
	struct map_session_data* sd = BL_CAST(BL_PC, bl);

	if (!sc || true) { // Kyomu doesn't reflect
		// Item-based reflection - Bypasses Boss check
		if (sd && sd->bonus.magic_damage_return && type && rnd()%100 < sd->bonus.magic_damage_return)
			return 1;
	}


	if( status_get_class_(src) == CLASS_BOSS )
		return 0;

	// status-based reflection
	if( !sc || sc->count == 0 )
		return 0;

	return 0;
}

/**
 * Checks whether a skill can be used in combos or not
 * @param skill_id: Target skill
 * @return	0: Skill is not a combo
 *			1: Skill is a normal combo
 *			2: Skill is combo that prioritizes auto-target even if val2 is set 
 * @author Panikon
 */
int skill_is_combo(uint16 skill_id) {
	switch(skill_id) {
		case SK_SH_GUILLOTINEFISTS:
			return 1;
	}
	return 0;
}

/*
 * Combo handler, start stop combo status
 */
void skill_combo_toggle_inf(struct block_list* bl, uint16 skill_id, int inf){
	TBL_PC *sd = BL_CAST(BL_PC, bl);

}

/**
 * Copy skill by Plagiarism or Reproduce
 * @param src: The caster
 * @param bl: The target
 * @param skill_id: Skill that casted
 * @param skill_lv: Skill level of the casted skill
 */
static void skill_do_copy(struct block_list* src,struct block_list *bl, uint16 skill_id, uint16 skill_lv)
{
	TBL_PC *tsd = BL_CAST(BL_PC, bl);

	if (!tsd || (!pc_checkskill(tsd,SK_ST_REPRODUCE) && !pc_checkskill(tsd,SK_RG_PLAGIARISM)))
		return;
	//If SC_PRESERVE is active and STATUS_REPRODUCE is not active, nothing to do
	/*else if (tsd->sc.data[SC_PRESERVE] && !tsd->sc.data[STATUS_REPRODUCE])
		return;*/
	else {
		uint16 idx;
		uint8 lv;

		skill_id = skill_dummy2skill_id(skill_id);

		//Use skill index, avoiding out-of-bound array [Cydh]
		if (!(idx = skill_get_index(skill_id)))
			return;
		// ShowStatus("Index %d.\n", idx);
		switch (skill_isCopyable(tsd,skill_id)) {
			case 1: //Copied by Plagiarism
				{
					struct status_change *tsc = status_get_sc(bl);
					//Already did SC check
					//Skill level copied depends on Reproduce skill that used
					lv = (tsc) ? tsc->data[STATUS_PLAGIARISM]->val1 : 1;
					if( tsd->cloneskill_idx > 0 && tsd->status.skill[tsd->cloneskill_idx].flag == SKILL_FLAG_PLAGIARIZED ) {
						clif_deleteskill(tsd,tsd->status.skill[tsd->cloneskill_idx].id);
						tsd->status.skill[tsd->cloneskill_idx].id = 0;
						tsd->status.skill[tsd->cloneskill_idx].lv = 0;
						tsd->status.skill[tsd->cloneskill_idx].flag = SKILL_FLAG_PERMANENT;
					}

					//Level dependent and limitation.
					if (src->type == BL_PC) //If player, max skill level is skill_get_max(skill_id)
						lv = min(lv,skill_get_max(skill_id));
					else //Monster might used skill level > allowed player max skill lv. Ex. Drake with Waterball lv. 10
						lv = min(lv,skill_lv);

					tsd->cloneskill_idx = idx;
					pc_setglobalreg(tsd, add_str(SKILL_VAR_PLAGIARISM), skill_id);
					pc_setglobalreg(tsd, add_str(SKILL_VAR_PLAGIARISM_LV), lv);
				}
				break;
			case 2: //Copied by Reproduce
				{
					struct status_change *tsc = status_get_sc(bl);
					//Already did SC check
					//Skill level copied depends on Reproduce skill that used
					lv = (tsc) ? tsc->data[STATUS_REPRODUCE]->val1 : 1;
					if( tsd->reproduceskill_idx > 0 && tsd->status.skill[tsd->reproduceskill_idx].flag == SKILL_FLAG_PLAGIARIZED ) {
						clif_deleteskill(tsd,tsd->status.skill[tsd->reproduceskill_idx].id);
						tsd->status.skill[tsd->reproduceskill_idx].id = 0;
						tsd->status.skill[tsd->reproduceskill_idx].lv = 0;
						tsd->status.skill[tsd->reproduceskill_idx].flag = SKILL_FLAG_PERMANENT;
					}

					//Level dependent and limitation.
					if (src->type == BL_PC) //If player, max skill level is skill_get_max(skill_id)
						lv = min(lv,skill_get_max(skill_id));
					else //Monster might used skill level > allowed player max skill lv. Ex. Drake with Waterball lv. 10
						lv = min(lv,skill_lv);

					tsd->reproduceskill_idx = idx;
					pc_setglobalreg(tsd, add_str(SKILL_VAR_REPRODUCE), skill_id);
					pc_setglobalreg(tsd, add_str(SKILL_VAR_REPRODUCE_LV), lv);
				}
				break;
			default: return;
		}
		tsd->status.skill[idx].id = skill_id;
		tsd->status.skill[idx].lv = lv;
		tsd->status.skill[idx].flag = SKILL_FLAG_PLAGIARIZED;
		clif_addskill(tsd,skill_id);
	}
}

/**
 * Knockback the target on skill_attack
 * @param src is the master behind the attack
 * @param dsrc is the actual originator of the damage, can be the same as src, or a BL_SKILL
 * @param target is the target to be attacked.
 * @param blewcount
 * @param skill_id
 * @param skill_lv
 * @param damage
 * @param tick
 * @param flag can hold a bunch of information:
 */
void skill_attack_blow(struct block_list *src, struct block_list *dsrc, struct block_list *target, uint8 blewcount, uint16 skill_id, uint16 skill_lv, int64 damage, t_tick tick, int flag) {
	int8 dir = -1; // Default direction
	//Only knockback if it's still alive, otherwise a "ghost" is left behind. [Skotlex]
	//Reflected spells do not bounce back (src == dsrc since it only happens for direct skills)
	if (!blewcount || target == dsrc || status_isdead(target))
		return;

	// Skill specific direction
	switch (skill_id) {
		
		// This ensures the storm randomly pushes instead of exactly a cell backwards per official mechanics.
		case SK_WZ_STORMGUST:
			if(!battle_config.stormgust_knockback)
				dir = rnd()%8;
			break;
		case SK_AC_ARROWSHOWER:
		case SK_AC_ARROWRAIN:
		case SK_WZ_ICEBERG:
		case SK_MG_EARTHBOLT:
		case SK_WZ_CRIMSONROCK:
		case SK_SO_SHADOWBOMB:
		case SK_WZ_THUNDERSTORM:
		case SK_SO_PHANTOMSPEAR:
			if (!battle_config.arrow_shower_knockback && (skill_id == SK_AC_ARROWSHOWER || skill_id == SK_AC_ARROWRAIN))
				dir = map_calc_dir(target, src->x, src->y);
			else
				dir = map_calc_dir(target, skill_area_temp[4], skill_area_temp[5]);
			break;
	}

	// Blown-specific handling
	switch( skill_id ) {
		default:
			skill_blown(dsrc,target,blewcount,dir, BLOWN_NONE);
			if (!blewcount && target->type == BL_SKILL && damage > 0) {
				TBL_SKILL *su = (TBL_SKILL*)target;
				
			}
			break;
	}
	clif_fixpos(target);
}

/*
 * =========================================================================
 * Does a skill attack with the given properties.
 * @param src is the master behind the attack (player/mob/pet)
 * @param dsrc is the actual originator of the damage, can be the same as src, or a BL_SKILL
 * @param bl is the target to be attacked.
 * @param flag can hold a bunch of information:
 *        flag&1
 *        flag&2 - Disable re-triggered by double casting
 *        flag&4 - Skip to blow target (because already knocked back before skill_attack somewhere)
 *        flag&8 - STATUS_COMBO state used to deal bonus damage
 *
 *        flag&0xFFF is passed to the underlying battle_calc_attack for processing.
 *             (usually holds number of targets, or just 1 for simple splash attacks)
 *
 *        flag&0xF000 - Values from enum e_skill_display
 *        flag&0x3F0000 - Values from enum e_battle_check_target
 * 
 *        flag&0x1000000 - Return 0 if damage was reflected
 *-------------------------------------------------------------------------*/
int64 skill_attack (int attack_type, struct block_list* src, struct block_list *dsrc, struct block_list *bl, uint16 skill_id, uint16 skill_lv, t_tick tick, int flag)
{
	struct Damage dmg;
	struct status_data *sstatus, *tstatus;
	struct status_change *sc, *tsc;
	struct map_session_data *sd, *tsd;
	int64 damage;
	bool rmdamage = false;//magic reflected
	int type;
	enum e_damage_type dmg_type;
	bool shadow_flag = false;
	bool additional_effects = true;

	if(skill_id > 0 && !skill_lv)
		return 0;

	nullpo_ret(src);	//Source is the master behind the attack (player/mob/pet)
	nullpo_ret(dsrc);	//dsrc is the actual originator of the damage, can be the same as src, or a skill casted by src.
	nullpo_ret(bl);		//Target to be attacked.

	if (status_bl_has_mode(bl,MD_SKILLIMMUNE) || (status_get_class(bl) == MOBID_EMPERIUM && !skill_get_inf2(skill_id, INF2_TARGETEMPERIUM)))
		return 0;

	if (src != dsrc) {
		//When caster is not the src of attack, this is a ground skill, and as such, do the relevant target checking. [Skotlex]
		if (!status_check_skilluse(battle_config.skill_caster_check?src:NULL, bl, skill_id, 2))
			return 0;
	} else if ((flag&SD_ANIMATION) && skill_get_nk(skill_id, NK_SPLASH)) {
		//Note that splash attacks often only check versus the targetted mob, those around the splash area normally don't get checked for being hidden/cloaked/etc. [Skotlex]
		if (!status_check_skilluse(src, bl, skill_id, 2))
			return 0;
	}

	sd = BL_CAST(BL_PC, src);
	tsd = BL_CAST(BL_PC, bl);

	sstatus = status_get_status_data(src);
	tstatus = status_get_status_data(bl);
	sc= status_get_sc(src);
	tsc= status_get_sc(bl);
	if (tsc && !tsc->count)
		tsc = NULL; //Don't need it.

	 //Trick Dead protects you from damage, but not from buffs and the like, hence it's placed here.
	if (tsc && tsc->data[STATUS_TRICKDEAD])
		return 0;
	
	if (skill_id == SK_AM_FIREDEMONSTRATION || skill_id == SK_CR_INCENDIARYBOMB || skill_id == SK_CR_MANDRAKERAID || skill_id == SK_CR_MANDRAKERAID_ATK)
		attack_type = BF_MAGIC;
	dmg = battle_calc_attack(attack_type,src,bl,skill_id,skill_lv,flag&0xFFF);

	//If the damage source is a unit, the damage is not delayed
	if (src != dsrc )
		dmg.amotion = 0;

	//! CHECKME: This check maybe breaks the battle_calc_attack, and maybe need better calculation.
	// Adjusted to the new system [Skotlex]
	if( src->type == BL_PET ) { // [Valaris]
		struct pet_data *pd = (TBL_PET*)src;
		if (pd->a_skill && pd->a_skill->div_ && pd->a_skill->id == skill_id) { //petskillattack2
			if (battle_config.pet_ignore_infinite_def || !is_infinite_defense(bl,dmg.flag)) {
				int element = skill_get_ele(skill_id, skill_lv);
				/*if (skill_id == -1) Does it ever worked?
					element = sstatus->rhw.ele;*/
				if (element != ELE_NEUTRAL || !(battle_config.attack_attr_none&BL_PET))
					dmg.damage = battle_attr_fix(src, bl, pd->a_skill->damage, element, tstatus->def_ele, tstatus->ele_lv);
				else
					dmg.damage = pd->a_skill->damage; // Fixed damage
				
			}
			else
				dmg.damage = 1*pd->a_skill->div_;
			dmg.damage2 = 0;
			dmg.div_= pd->a_skill->div_;
		}
	}

	if( dmg.flag&BF_MAGIC && ( (battle_config.eq_single_target_reflectable && (flag&0xFFF) == 1) ) )
	{ // Earthquake on multiple targets is not counted as a target skill. [Inkfish]
		if( (dmg.damage || dmg.damage2) && (type = skill_magic_reflect(src, bl, src==dsrc)) )
		{	//Magic reflection, switch caster/target
			struct block_list *tbl = bl;
			rmdamage = true;
			bl = src;
			src = tbl;
			dsrc = tbl;
			sd = BL_CAST(BL_PC, src);
			tsd = BL_CAST(BL_PC, bl);
			tsc = status_get_sc(bl);
			if (tsc && !tsc->count)
				tsc = NULL; //Don't need it.
			/* bugreport:2564 flag&2 disables double casting trigger */
			flag |= 2;
			//Reflected magic damage will not cause the caster to be knocked back [Playtester]
			flag |= 4;
			//Spirit of Wizard blocks Kaite's reflection
			if( type == 2 && tsc && false )
			{	//Consume one Fragment per hit of the casted skill? [Skotlex]
				
			} else if( type != 2 ) /* Kaite bypasses */
				additional_effects = false;

			// Official Magic Reflection Behavior : damage reflected depends on gears caster wears, not target
#if MAGIC_REFLECTION_TYPE
#ifdef RENEWAL
			if( dmg.dmg_lv != ATK_MISS ) { //Wiz SL cancelled and consumed fragment
#else
			// issue:6415 in pre-renewal Kaite reflected the entire damage received
			// regardless of caster's equipment (Aegis 11.1)
			if( dmg.dmg_lv != ATK_MISS && type == 1 ) { //Wiz SL cancelled and consumed fragment
#endif
				short s_ele = skill_get_ele(skill_id, skill_lv);

				if (s_ele == ELE_WEAPON) // the skill takes the weapon's element
					s_ele = sstatus->rhw.ele;
				else if (s_ele == ELE_ENDOWED) //Use status element
					s_ele = status_get_attack_sc_element(src,status_get_sc(src));
				else if( s_ele == ELE_RANDOM) //Use random element
					s_ele = rnd()%ELE_ALL;

				dmg.damage = battle_attr_fix(bl, bl, dmg.damage, s_ele, status_get_element(bl), status_get_element_level(bl));

				if( tsc && tsc->data[STATUS_ENERGYCOAT] ) {
					struct status_data *status = status_get_status_data(bl);
					int per = 100*status->sp / status->max_sp -1; //100% should be counted as the 80~99% interval
					per /=20; //Uses 20% SP intervals.
					//SP Cost: 1% + 0.5% per every 20% SP
					if (!status_charge(bl, 0, (10+5*per)*status->max_sp/1000))
						status_change_end(bl, STATUS_ENERGYCOAT, INVALID_TIMER);
					//Reduction: 6% + 6% every 20%
					dmg.damage -= dmg.damage * (6 * (1+per)) / 100;
				}

				if (dmg.damage > 0 && tsd && tsd->bonus.reduce_damage_return != 0) {
					dmg.damage -= dmg.damage * tsd->bonus.reduce_damage_return / 100;
					dmg.damage = i64max(dmg.damage, 1);
				}
			}
#endif
		}

		
		if( (dmg.damage || dmg.damage2) && tsc && (tsc->data[STATUS_HALLUCINATIONWALK] && rnd()%100 < tsc->data[STATUS_HALLUCINATIONWALK]->val3) ) {
			dmg.damage = dmg.damage2 = 0;
			dmg.dmg_lv = ATK_MISS;
		}
	}

	damage = dmg.damage + dmg.damage2;

	

	

	//Skill hit type
	dmg_type = (skill_id == 0) ? DMG_SPLASH : skill_get_hit(skill_id);

	switch( skill_id ) {
		// case SK_RG_TRIANGLESHOT:
		// 	if( rnd()%100 > (1 + skill_lv) )
		// 		dmg.blewcount = 0;
		// 	break;
		default:
			if (damage < dmg.div_ && skill_id != SK_MO_PALMSTRIKE)
				dmg.blewcount = 0; //only pushback when it hit for other
			break;
	}

	switch( skill_id ) {
		case SK_CR_GRANDCROSS:
			if( battle_config.gx_disptype)
				dsrc = src;
			if( src == bl)
				dmg_type = DMG_ENDURE;
			else
				flag|= SD_ANIMATION;
			break;
	}


	//Display damage.
	switch( skill_id ) {
	
		case SK_KN_COUNTERATTACK:
		case SK_AS_DOUBLEATTACK:
			dmg.dmotion = clif_damage(src,bl,tick,dmg.amotion,dmg.dmotion,damage,dmg.div_,dmg.type,dmg.damage2,false);
			break;

		case SK_AS_VENOMSPLASHER:
			if( flag&SD_ANIMATION ) // the surrounding targets
				dmg.dmotion = clif_skill_damage(dsrc,bl,tick, dmg.amotion, dmg.dmotion, damage, dmg.div_, skill_id, -1, DMG_SPLASH); // needs -1 as skill level
			else // the central target doesn't display an animation
				dmg.dmotion = clif_skill_damage(dsrc,bl,tick, dmg.amotion, dmg.dmotion, damage, dmg.div_, skill_id, -2, DMG_SPLASH); // needs -2(!) as skill level
			break;
		
		case SK_MG_VOIDEXPANSION:
			dmg.dmotion = clif_skill_damage(src,bl,tick,dmg.amotion,dmg.dmotion,damage,1, SK_MG_VOIDEXPANSION,-2,DMG_SINGLE);
			//dmg.dmotion = clif_skill_damage(src,bl,tick,dmg.amotion,dmg.dmotion,damage,1,WL_CHAINLIGHTNING_ATK,-2,DMG_SINGLE);
			break;
	
		case SK_SA_FIREBALL:
		case SK_SA_ICICLE:
	
		case SK_SA_WINDSLASH:
		
		case SK_AM_BASILISK1:
		case SK_AM_BEHOLDER1:
		case SK_CR_BEHOLDER3:
		case SK_AM_BASILISK2:
		case SK_AM_BEHOLDER2:
			dmg.dmotion = clif_skill_damage(src,bl,tick,dmg.amotion,dmg.dmotion,damage,dmg.div_,skill_id,-1,DMG_SPLASH);
			break;
		case SK_CL_SEVERERAINSTORM_MELEE:
			dmg.dmotion = clif_skill_damage(src,bl,tick,dmg.amotion,dmg.dmotion,damage,dmg.div_,SK_CL_SEVERERAINSTORM,-2,DMG_SPLASH);
			break;
		case SK_PR_DUPLELUX_MELEE:
		case SK_PR_DUPLELUX_MAGIC:
			dmg.amotion = 300;/* makes the damage value not overlap with previous damage (when displayed by the client) */
		default:
			if( flag&SD_ANIMATION && dmg.div_ < 2 ) //Disabling skill animation doesn't works on multi-hit.
				dmg_type = DMG_SPLASH;
			if (src->type == BL_SKILL) {
				TBL_SKILL *su = (TBL_SKILL*)src;
				if (su->group && skill_get_inf2(su->group->skill_id, INF2_ISTRAP)) { // show damage on trap targets
					clif_skill_damage(src, bl, tick, dmg.amotion, dmg.dmotion, damage, dmg.div_, skill_id, flag&SD_LEVEL ? -1 : skill_lv, DMG_SPLASH);
					break;
				}
			}
			dmg.dmotion = clif_skill_damage(dsrc,bl,tick, dmg.amotion, dmg.dmotion, damage, dmg.div_, skill_id, flag&SD_LEVEL?-1:skill_lv, dmg_type);
			break;
	}

	map_freeblock_lock();

	if (bl->type == BL_PC && skill_id && skill_db.find(skill_id)->copyable.option && //Only copy skill that copyable [Cydh]
		dmg.flag&BF_SKILL && dmg.damage+dmg.damage2 > 0 && damage < status_get_hp(bl)) //Cannot copy skills if the blow will kill you. [Skotlex]
		skill_do_copy(src,bl,skill_id,skill_lv);

	if (dmg.dmg_lv >= ATK_MISS && (type = skill_get_walkdelay(skill_id, skill_lv)) > 0)
	{	//Skills with can't walk delay also stop normal attacking for that
		//duration when the attack connects. [Skotlex]
		struct unit_data *ud = unit_bl2ud(src);
		if (ud && DIFF_TICK(ud->attackabletime, tick + type) < 0)
			ud->attackabletime = tick + type;
	}

	shadow_flag = skill_check_shadowform(bl, damage, dmg.div_);

	// Instant damage
	if( !dmg.amotion ) {
		if( (!tsc || (!tsc->data[STATUS_SWORNPROTECTOR] && skill_id != SK_CR_REFLECTSHIELD)

			) && !shadow_flag )
			status_fix_damage(src,bl,damage,dmg.dmotion,skill_id); //Deal damage before knockback to allow stuff like firewall+storm gust combo.
		if( !status_isdead(bl) && additional_effects )
			SkillAdditionalEffects::skill_additional_effect(src,bl,skill_id,skill_lv,dmg.flag,dmg.dmg_lv,tick);
		if( damage > 0 ) //Counter status effects [Skotlex]
			skill_counter_additional_effect(src,bl,skill_id,skill_lv,dmg.flag,tick);
	}

	// Blow!
	if (!(flag&4))
		skill_attack_blow(src, dsrc, bl, (uint8)dmg.blewcount, skill_id, skill_lv, damage, tick, flag);

	// Delayed damage must be dealt after the knockback (it needs to know actual position of target)
	if( dmg.amotion ) {
		if( shadow_flag ) {
			if( !status_isdead(bl) && additional_effects )
				SkillAdditionalEffects::skill_additional_effect(src, bl, skill_id, skill_lv, dmg.flag, dmg.dmg_lv, tick);
			if( dmg.flag > ATK_BLOCK )
				skill_counter_additional_effect(src, bl, skill_id, skill_lv, dmg.flag, tick);
		} else
			battle_delay_damage(tick, dmg.amotion,src,bl,dmg.flag,skill_id,skill_lv,damage,dmg.dmg_lv,dmg.dmotion, additional_effects, false);
	}

	if (tsc  
		) {
		if (tsc->data[STATUS_SWORNPROTECTOR]) {
			struct status_change_entry *sce = tsc->data[STATUS_SWORNPROTECTOR];
			struct block_list *d_bl = map_id2bl(sce->val1);

			if (d_bl && (
				(d_bl->type == BL_PC && ((TBL_PC*)d_bl)->devotion[sce->val2] == bl->id)
				) && check_distance_bl(bl, d_bl, sce->val3) )
			{
				if (!rmdamage) {
					clif_damage(d_bl, d_bl, gettick(), 0, 0, damage, 0, DMG_NORMAL, 0, false);
					status_fix_damage(NULL, d_bl, damage, 0, 0);
				} else {
					bool isDevotRdamage = false;

					if (battle_config.devotion_rdamage && battle_config.devotion_rdamage > rnd()%100)
						isDevotRdamage = true;
					// If !isDevotRdamage, reflected magics are done directly on the target not on paladin
					// This check is only for magical skill.
					// For BF_WEAPON skills types track var rdamage and function battle_calc_return_damage
					clif_damage(bl, (!isDevotRdamage) ? bl : d_bl, gettick(), 0, 0, damage, 0, DMG_NORMAL, 0, false);
					status_fix_damage(bl, (!isDevotRdamage) ? bl : d_bl, damage, 0, 0);
				}
			} else {
				status_change_end(bl, STATUS_SWORNPROTECTOR, INVALID_TIMER);
				if (!dmg.amotion)
					status_fix_damage(src, bl, damage, dmg.dmotion, 0);
			}
		}
		
	}

	if(damage > 0 && !status_has_mode(tstatus,MD_STATUSIMMUNE)) {
		
	}

	


	if( damage > 0 ) { // Post-damage effects
		
		if( sd )
			skill_onskillusage(sd, bl, skill_id, tick);
	}

	if (!(flag&2)) {
		switch (skill_id) {
			case SK_WZ_STALAGMITE:
			case SK_MG_COLDBOLT:
			case SK_MG_FIREBOLT:
			case SK_MG_LIGHTNINGBOLT:
			case SK_MG_SOULSTRIKE:
			case SK_MG_DARKSTRIKE:
				if (sc && sc->data[STATUS_MAGICSQUARED] && rnd() % 100 < sc->data[STATUS_MAGICSQUARED]->val2)
					//skill_addtimerskill(src, tick + dmg.div_*dmg.amotion, bl->id, 0, 0, skill_id, skill_lv, BF_MAGIC, flag|2);
					skill_addtimerskill(src, tick + dmg.amotion, bl->id, 0, 0, skill_id, skill_lv, BF_MAGIC, flag|2);
				break;
		}
	}

	map_freeblock_unlock();

	if ((flag&0x1000000) && rmdamage)
		return 0; //Should return 0 when damage was reflected

	return damage;
}

/*==========================================
 * Sub function for recursive skill call.
 * Checking bl battle flag and display damage
 * then call func with source,target,skill_id,skill_lv,tick,flag
 *------------------------------------------*/
typedef int (*SkillFunc)(struct block_list *, struct block_list *, int, int, t_tick, int);
int skill_area_sub(struct block_list *bl, va_list ap)
{
	struct block_list *src;
	uint16 skill_id,skill_lv;
	int flag;
	t_tick tick;
	SkillFunc func;

	nullpo_ret(bl);

	src = va_arg(ap,struct block_list *);
	skill_id = va_arg(ap,int);
	skill_lv = va_arg(ap,int);
	tick = va_arg(ap,t_tick);
	flag = va_arg(ap,int);
	func = va_arg(ap,SkillFunc);

	if (flag&BCT_WOS && src == bl)
		return 0;

	if(battle_check_target(src,bl,flag) > 0) {
		// several splash skills need this initial dummy packet to display correctly
		if (flag&SD_PREAMBLE && skill_area_temp[2] == 0)
			clif_skill_damage(src,bl,tick, status_get_amotion(src), 0, -30000, 1, skill_id, skill_lv, DMG_SINGLE);

		if (flag&(SD_SPLASH|SD_PREAMBLE))
			skill_area_temp[2]++;

		return func(src,bl,skill_id,skill_lv,tick,flag);
	}
	return 0;
}

static int skill_check_unit_range_sub(struct block_list *bl, va_list ap)
{
	struct skill_unit *unit;
	uint16 skill_id,g_skill_id;

	unit = (struct skill_unit *)bl;

	if(bl->prev == NULL || bl->type != BL_SKILL)
		return 0;

	if(!unit->alive)
		return 0;

	skill_id = va_arg(ap,int);
	g_skill_id = unit->group->skill_id;

	return 1;
}

static int skill_check_unit_range (struct block_list *bl, int x, int y, uint16 skill_id, uint16 skill_lv)
{
	//Non players do not check for the skill's splash-trigger area.
	int range = bl->type==BL_PC?skill_get_unit_range(skill_id, skill_lv):0;
	int layout_type = skill_get_unit_layout_type(skill_id,skill_lv);
	if (layout_type==-1 || layout_type>MAX_SQUARE_LAYOUT) {
		ShowError("skill_check_unit_range: unsupported layout type %d for skill %d\n",layout_type,skill_id);
		return 0;
	}

	range += layout_type;
	return map_foreachinallarea(skill_check_unit_range_sub,bl->m,x-range,y-range,x+range,y+range,BL_SKILL,skill_id);
}

static int skill_check_unit_range2_sub (struct block_list *bl, va_list ap)
{
	uint16 skill_id;

	if(bl->prev == NULL)
		return 0;

	skill_id = va_arg(ap,int);

	if( status_isdead(bl) )
		return 0;

#ifndef RENEWAL
	if( skill_id == HP_BASILICA && bl->type == BL_PC )
		return 0;
#endif

	if( skill_id == SK_AM_FIREDEMONSTRATION || skill_id == SK_CR_INCENDIARYBOMB )
		return 0; //Allow casting Bomb/Demonstration Right under things
	return 1;
}

/**
 * Used to check range condition of the casted skill. Used if the skill has UF_NOFOOTSET or INF2_DISABLENEARNPC
 * @param bl Object that casted skill
 * @param x Position x of the target
 * @param y Position y of the target
 * @param skill_id The casted skill
 * @param skill_lv The skill Lv
 * @param isNearNPC 'true' means, check the range between target and nearer NPC by using npc_isnear and range calculation [Cydh]
 * @return 0: No object (BL_CHAR or BL_PC) within the range. If 'isNearNPC' the target oject is BL_NPC
 */
static int skill_check_unit_range2 (struct block_list *bl, int x, int y, uint16 skill_id, uint16 skill_lv, bool isNearNPC)
{
	int range = 0, type;

	//Range for INF2_DISABLENEARNPC is using skill splash value [Cydh]
	if (isNearNPC)
		range = skill_get_splash(skill_id,skill_lv);

	//While checking INF2_DISABLENEARNPC and the range from splash is 0, get the range from skill_unit range and layout. [Cydh]
	if (!isNearNPC || !range) {
		switch (skill_id) {	// to be expanded later
			case SK_MG_ICEWALL:
				range = 2;
				break;
			default: {
					int layout_type = skill_get_unit_layout_type(skill_id,skill_lv);

					if (layout_type == -1 || layout_type > MAX_SQUARE_LAYOUT) {
						ShowError("skill_check_unit_range2: unsupported layout type %d for skill %d\n",layout_type,skill_id);
						return 0;
					}
					range = skill_get_unit_range(skill_id,skill_lv) + layout_type;
				}
				break;
		}
	}

	uint16 skill_npc_range = skill_db.find(skill_id)->unit_nonearnpc_range;

	//Check the additional range [Cydh]
	if (isNearNPC && skill_npc_range > 0)
		range += skill_npc_range;

	if (!isNearNPC) { //Doesn't check the NPC range
		//If the caster is a monster/NPC, only check for players. Otherwise just check characters
		if (bl->type&battle_config.skill_nofootset)
			type = BL_CHAR;
		else if(bl->type == BL_MOB)
			type = BL_MOB; //Monsters can never place traps on top of each other regardless of setting
		else
			return 0; //Don't check
	} else
		type = BL_NPC;

	return (!isNearNPC) ?
		//!isNearNPC is used for UF_NOFOOTSET, regardless the NPC position, only check the BL_CHAR or BL_PC
		map_foreachinallarea(skill_check_unit_range2_sub,bl->m,x - range,y - range,x + range,y + range,type,skill_id):
		//isNearNPC is used to check range from NPC
		map_foreachinallarea(npc_isnear_sub,bl->m,x - range,y - range,x + range,y + range,type,skill_id);
}

/*==========================================
 * Checks that you have the requirements for casting a skill for homunculus/mercenary.
 * Flag:
 * &1: finished casting the skill (invoke hp/sp/item consumption)
 * &2: picked menu entry (Warp Portal, Teleport and other menu based skills)
 *------------------------------------------*/
static int skill_check_condition_mercenary(struct block_list *bl, uint16 skill_id, uint16 skill_lv, int type)
{
	struct status_data *status;
	struct map_session_data *sd = NULL;
	int i, hp, sp, hp_rate, sp_rate, state, mhp;
	t_itemid itemid[MAX_SKILL_ITEM_REQUIRE];
	int amount[ARRAYLENGTH(itemid)], index[ARRAYLENGTH(itemid)];

	nullpo_retr(0, bl);

	

	status = status_get_status_data(bl);
	skill_lv = cap_value(skill_lv, 1, MAX_SKILL_LEVEL);

	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);

	if (skill == nullptr)
		return 0;

	// Requirements
	for( i = 0; i < ARRAYLENGTH(itemid); i++ )
	{
		itemid[i] = skill->require.itemid[i];
		amount[i] = skill->require.amount[i];
	}
	hp = skill->require.hp[skill_lv - 1];
	sp = skill->require.sp[skill_lv - 1];
	hp_rate = skill->require.hp_rate[skill_lv - 1];
	sp_rate = skill->require.sp_rate[skill_lv - 1];
	state = skill->require.state;
	if ((mhp = skill->require.mhp[skill_lv - 1]) > 0)
		hp += (status->max_hp * mhp) / 100;
	if( hp_rate > 0 )
		hp += (status->hp * hp_rate) / 100;
	else
		hp += (status->max_hp * (-hp_rate)) / 100;
	if( sp_rate > 0 )
		sp += (status->sp * sp_rate) / 100;
	else
		sp += (status->max_sp * (-sp_rate)) / 100;

	if( !(type&2) )
	{
		if( hp > 0 && status->hp <= (unsigned int)hp )
		{
			clif_skill_fail(sd, skill_id, USESKILL_FAIL_HP_INSUFFICIENT, 0);
			return 0;
		}
		if( sp > 0 && status->sp <= (unsigned int)sp )
		{
			clif_skill_fail(sd, skill_id, USESKILL_FAIL_SP_INSUFFICIENT, 0);
			return 0;
		}
	}

	if( !type )
		switch( state )
		{
			case ST_MOVE_ENABLE:
				if( !unit_can_move(bl) )
				{
					clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
					return 0;
				}
				break;
		}
	if( !(type&1) )
		return 1;

	// Check item existences
	for( i = 0; i < ARRAYLENGTH(itemid); i++ )
	{
		index[i] = -1;
		if( itemid[i] == 0 ) continue; // No item
		index[i] = pc_search_inventory(sd, itemid[i]);
		if( index[i] < 0 || sd->inventory.u.items_inventory[index[i]].amount < amount[i] )
		{
			clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
			return 0;
		}
	}

	// Consume items
	for( i = 0; i < ARRAYLENGTH(itemid); i++ )
	{
		if( index[i] >= 0 ) pc_delitem(sd, index[i], amount[i], 0, 1, LOG_TYPE_CONSUME);
	}

	if( type&2 )
		return 1;

	if( sp || hp )
		status_zap(bl, hp, sp);

	return 1;
}

/*==========================================
 *
 *------------------------------------------*/
int skill_area_sub_count (struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv, t_tick tick, int flag)
{
	
	return 1;
}

/*==========================================
 *
 *------------------------------------------*/
static TIMER_FUNC(skill_timerskill){
	struct block_list *src = map_id2bl(id),*target;
	struct unit_data *ud = unit_bl2ud(src);
	struct skill_timerskill *skl;
	struct skill_unit *unit = NULL;
	int range;

	nullpo_ret(src);
	nullpo_ret(ud);
	skl = ud->skilltimerskill[data];
	nullpo_ret(skl);
	ud->skilltimerskill[data] = NULL;

	do {
		if(src->prev == NULL)
			break; // Source not on Map
		if(skl->target_id) {
			target = map_id2bl(skl->target_id);
			

			

			if(target == NULL)
				break; // Target offline?
			if(target->prev == NULL)
				break; // Target not on Map
			if(src->m != target->m)
				break; // Different Maps

			if(status_isdead(src)) {
				switch(skl->skill_id) {
					case SK_SO_TETRAVORTEX_FIRE:
					case SK_SO_TETRAVORTEX_WATER:
					case SK_SO_TETRAVORTEX_WIND:
					case SK_SO_TETRAVORTEX_GROUND:
					case SK_SO_TETRAVORTEX_NEUTRAL:
					// For SR_FLASHCOMBO
					case SK_SH_GUILLOTINEFISTS:
						if (src->type != BL_PC)
							continue;
						break; // Exceptions
					default:
						continue; // Caster is Dead
				}
			}
			if(status_isdead(target) )
				break;

			switch(skl->skill_id) {
				case SK_KN_COUNTERATTACK:
					clif_skill_nodamage(src,target,skl->skill_id,skl->skill_lv,1);
					break;
				
				case SK_SA_SILENCE:
					if (src->type == BL_MOB) {
						// Monsters use the default duration when casting Lex Divina
						sc_start(src, target, status_skill2sc(skl->skill_id), skl->type, skl->skill_lv, skill_get_time2(status_sc2skill(status_skill2sc(skl->skill_id)), 1));
						break;
					}
					// Fall through
				
				case SK_SO_TETRAVORTEX_FIRE:
				case SK_SO_TETRAVORTEX_WATER:
				case SK_SO_TETRAVORTEX_WIND:
				case SK_SO_TETRAVORTEX_GROUND:
				case SK_SO_TETRAVORTEX_NEUTRAL:
					clif_skill_nodamage(src,target,skl->skill_id,skl->skill_lv,1);
					skill_attack(BF_MAGIC,src,src,target,skl->skill_id,skl->skill_lv,tick,skl->flag|SD_LEVEL|SD_ANIMATION);
					break;
				case SK_ST_FATALMENACE:
					skill_attack(BF_WEAPON, src, src, target, skl->skill_id, skl->skill_lv, tick, skl->flag|SD_LEVEL);
					break;
				case SK_MO_PALMSTRIKE:
					{
						skill_attack(skl->type,src,src,target,skl->skill_id,skl->skill_lv,tick,skl->flag);
						break;
					}
				case SK_SH_GUILLOTINEFISTS:
					if( src->type == BL_PC ) {
						if( distance_xy(src->x, src->y, target->x, target->y) >= 3 )
							break;
						skill_castend_damage_id(src, target, skl->skill_id, pc_checkskill(((TBL_PC *)src), skl->skill_id), tick, 0);
					}
					break;
				default:
					skill_attack(skl->type,src,src,target,skl->skill_id,skl->skill_lv,tick,skl->flag);
					break;
			}
		}
		else {
			if(src->m != skl->map)
				break;
			switch( skl->skill_id )
			{
				case SK_CR_GEOGRAFIELD_ATK:
				case SK_CR_MANDRAKERAID_ATK:
					{
						int dummy = 1, i = skill_get_unit_range(skl->skill_id,skl->skill_lv);
						map_foreachinarea(skill_cell_overlap, src->m, skl->x-i, skl->y-i, skl->x+i, skl->y+i, BL_SKILL, skl->skill_id, &dummy, src);
					}
			}
		}
	} while (0);
	//Free skl now that it is no longer needed.
	ers_free(skill_timer_ers, skl);
	return 0;
}

/*==========================================
 *
 *------------------------------------------*/
int skill_addtimerskill (struct block_list *src, t_tick tick, int target, int x,int y, uint16 skill_id, uint16 skill_lv, int type, int flag)
{
	int i;
	struct unit_data *ud;
	nullpo_retr(1, src);
	if (src->prev == NULL)
		return 0;
	ud = unit_bl2ud(src);
	nullpo_retr(1, ud);

	ARR_FIND( 0, MAX_SKILLTIMERSKILL, i, ud->skilltimerskill[i] == 0 );
	if( i == MAX_SKILLTIMERSKILL ) return 1;

	ud->skilltimerskill[i] = ers_alloc(skill_timer_ers, struct skill_timerskill);
	ud->skilltimerskill[i]->timer = add_timer(tick, skill_timerskill, src->id, i);
	ud->skilltimerskill[i]->src_id = src->id;
	ud->skilltimerskill[i]->target_id = target;
	ud->skilltimerskill[i]->skill_id = skill_id;
	ud->skilltimerskill[i]->skill_lv = skill_lv;
	ud->skilltimerskill[i]->map = src->m;
	ud->skilltimerskill[i]->x = x;
	ud->skilltimerskill[i]->y = y;
	ud->skilltimerskill[i]->type = type;
	ud->skilltimerskill[i]->flag = flag;
	return 0;
}

/*==========================================
 *
 *------------------------------------------*/
int skill_cleartimerskill (struct block_list *src)
{
	int i;
	struct unit_data *ud;
	nullpo_ret(src);
	ud = unit_bl2ud(src);
	nullpo_ret(ud);

	for(i=0;i<MAX_SKILLTIMERSKILL;i++) {
		if(ud->skilltimerskill[i]) {
			switch(ud->skilltimerskill[i]->skill_id) {
				case SK_SO_TETRAVORTEX_FIRE:
				case SK_SO_TETRAVORTEX_WATER:
				case SK_SO_TETRAVORTEX_WIND:
				case SK_SO_TETRAVORTEX_GROUND:
				case SK_SO_TETRAVORTEX_NEUTRAL:
				// For SR_FLASHCOMBO
				case SK_SH_GUILLOTINEFISTS:
					if (src->type != BL_PC)
						break;
					continue;
			}
			delete_timer(ud->skilltimerskill[i]->timer, skill_timerskill);
			ers_free(skill_timer_ers, ud->skilltimerskill[i]);
			ud->skilltimerskill[i]=NULL;
		}
	}
	return 1;
}
static int skill_active_reverberation(struct block_list *bl, va_list ap) {
	struct skill_unit *su = (TBL_SKILL*)bl;
	struct skill_unit_group *sg = NULL;

	/*nullpo_ret(su);

	if (bl->type != BL_SKILL)
		return 0;
	if (su->alive && (sg = su->group) && sg->skill_id == NPC_REVERBERATION) {
		map_foreachinallrange(skill_trap_splash, bl, skill_get_splash(sg->skill_id, sg->skill_lv), sg->bl_flag, bl, gettick());
		su->limit = DIFF_TICK(gettick(), sg->tick);
		sg->unit_id = UNT_USED_TRAPS;
	}*/
	return 1;
}

/**
 * Reveal hidden trap
 **/
static int skill_reveal_trap(struct block_list *bl, va_list ap)
{
	TBL_SKILL *su = (TBL_SKILL*)bl;

	if (su->alive && su->group && su->hidden && skill_get_inf2(su->group->skill_id, INF2_ISTRAP)) {
		//Change look is not good enough, the client ignores it as an actual trap still. [Skotlex]
		//clif_changetraplook(bl, su->group->unit_id);

		su->hidden = false;
		skill_getareachar_skillunit_visibilty(su, AREA);
		return 1;
	}
	return 0;
}

/**
 * Attempt to reveal trap in area
 * @param src Skill caster
 * @param range Affected range
 * @param x
 * @param y
 * TODO: Remove hardcode usages for this function
 **/
void skill_reveal_trap_inarea(struct block_list *src, int range, int x, int y) {
	if (!battle_config.traps_setting)
		return;
	nullpo_retv(src);
	map_foreachinallarea(skill_reveal_trap, src->m, x-range, y-range, x+range, y+range, BL_SKILL);
}

/*========================================== [Playtester]
* Process tarot card's effects
* @param src: Source of the tarot card effect
* @param target: Target of the tartor card effect
* @param skill_id: ID of the skill used
* @param skill_lv: Level of the skill used
* @param tick: Processing tick time
* @return Card number
*------------------------------------------*/
static int skill_tarotcard(struct block_list* src, struct block_list *target, uint16 skill_id, uint16 skill_lv, t_tick tick)
{
	int card = 0;

//	if (battle_config.tarotcard_equal_chance) {
//		//eAthena equal chances
//		card = rnd() % 14 + 1;
//	}
//	else {
//		//Official chances
//		int rate = rnd() % 100;
//		if (rate < 10) card = 1; // THE FOOL
//		else if (rate < 20) card = 2; // THE MAGICIAN
//		else if (rate < 30) card = 3; // THE HIGH PRIESTESS
//		else if (rate < 37) card = 4; // THE CHARIOT
//		else if (rate < 47) card = 5; // STRENGTH
//		else if (rate < 62) card = 6; // THE LOVERS
//		else if (rate < 63) card = 7; // WHEEL OF FORTUNE
//		else if (rate < 69) card = 8; // THE HANGED MAN
//		else if (rate < 74) card = 9; // DEATH
//		else if (rate < 82) card = 10; // TEMPERANCE
//		else if (rate < 83) card = 11; // THE DEVIL
//		else if (rate < 85) card = 12; // THE TOWER
//		else if (rate < 90) card = 13; // THE STAR
//		else card = 14; // THE SUN
//	}
//
//	switch (card) {
//	case 1: // THE FOOL - heals SP to 0
//	{
//		status_percent_damage(src, target, 0, 100, false);
//		break;
//	}
//	case 2: // THE MAGICIAN - matk halved
//	{
//		/*sc_start(src, target, SC_INCMATKRATE, 100, -50, skill_get_time2(skill_id, skill_lv));*/
//		break;
//	}
//	case 3: // THE HIGH PRIESTESS - all buffs removed
//	{
//		status_change_clear_buffs(target, SCCB_BUFFS | SCCB_CHEM_PROTECT);
//		break;
//	}
//	case 4: // THE CHARIOT - 1000 damage, random armor destroyed
//	{
//		status_fix_damage(src, target, 1000, 0, skill_id);
//		clif_damage(src, target, tick, 0, 0, 1000, 0, DMG_NORMAL, 0, false);
//		if (!status_isdead(target))
//		{
//			unsigned short where[] = { EQP_ARMOR, EQP_SHIELD, EQP_HELM };
//			skill_break_equip(src, target, where[rnd() % 3], 10000, BCT_ENEMY);
//		}
//		break;
//	}
//	case 5: // STRENGTH - atk halved
//	{
//		sc_start(src, target, STATUS_INCATKRATE, 100, -50, skill_get_time2(skill_id, skill_lv));
//		break;
//	}
//	case 6: // THE LOVERS - 2000HP heal, random teleported
//	{
//		status_heal(target, 2000, 0, 0);
//		if (!map_flag_vs(target->m))
//			unit_warp(target, -1, -1, -1, CLR_TELEPORT);
//		break;
//	}
//	case 7: // WHEEL OF FORTUNE - random 2 other effects
//	{
//		// Recursive call
//		skill_tarotcard(src, target, skill_id, skill_lv, tick);
//		skill_tarotcard(src, target, skill_id, skill_lv, tick);
//		break;
//	}
//	case 8: // THE HANGED MAN - stop, freeze or stoned
//	{
//	/*	enum sc_type sc[] = { SC_STOP, STATUS_FREEZE, STATUS_STONECURSE };
//		uint8 rand_eff = rnd() % 3;
//		int time = ((rand_eff == 0) ? skill_get_time2(skill_id, skill_lv) : skill_get_time2(status_sc2skill(sc[rand_eff]), 1));
//		sc_start(src, target, sc[rand_eff], 100, skill_lv, time);*/
//		break;
//	}
//	case 9: // DEATH - curse, coma and poison
//	{
//		status_change_start(src, target, STATUS_COMA, 10000, skill_lv, 0, src->id, 0, 0, SCSTART_NONE);
//		sc_start(src, target, STATUS_CURSE, 100, skill_lv, skill_get_time2(status_sc2skill(STATUS_CURSE), 1));
//		sc_start2(src, target, STATUS_POISON, 100, skill_lv, src->id, skill_get_time2(status_sc2skill(STATUS_POISON), 1));
//		break;
//	}
//	case 10: // TEMPERANCE - confusion
//	{
//		sc_start(src, target, STATUS_CONFUSION, 100, skill_lv, skill_get_time2(skill_id, skill_lv));
//		break;
//	}
//	case 11: // THE DEVIL - 6666 damage, atk and matk halved, cursed
//	{
//		status_fix_damage(src, target, 6666, 0, skill_id);
//		clif_damage(src, target, tick, 0, 0, 6666, 0, DMG_NORMAL, 0, false);
//		sc_start(src, target, STATUS_INCATKRATE, 100, -50, skill_get_time2(skill_id, skill_lv));
//		sc_start(src, target, SC_INCMATKRATE, 100, -50, skill_get_time2(skill_id, skill_lv));
//		sc_start(src, target, STATUS_CURSE, skill_lv, 100, skill_get_time2(status_sc2skill(STATUS_CURSE), 1));
//		break;
//	}
//	case 12: // THE TOWER - 4444 damage
//	{
//		status_fix_damage(src, target, 4444, 0, skill_id);
//		clif_damage(src, target, tick, 0, 0, 4444, 0, DMG_NORMAL, 0, false);
//		break;
//	}
//	case 13: // THE STAR - stun
//	{
//		sc_start(src, target, STATUS_STUN, 100, skill_lv, skill_get_time2(status_sc2skill(STATUS_STUN), 1));
//		break;
//	}
//	default: // THE SUN - atk, matk, hit, flee and def reduced, immune to more tarot card effects
//	{
//#ifdef RENEWAL
//		//In renewal, this card gives the SC_TAROTCARD status change which makes you immune to other cards
//		sc_start(src, target, SC_TAROTCARD, 100, skill_lv, skill_get_time2(skill_id, skill_lv));
//#endif
//		sc_start(src, target, STATUS_INCATKRATE, 100, -20, skill_get_time2(skill_id, skill_lv));
//		sc_start(src, target, SC_INCMATKRATE, 100, -20, skill_get_time2(skill_id, skill_lv));
//		sc_start(src, target, SC_INCHITRATE, 100, -20, skill_get_time2(skill_id, skill_lv));
//		sc_start(src, target, SC_INCFLEERATE, 100, -20, skill_get_time2(skill_id, skill_lv));
//		sc_start(src, target, STATUS_INCDEFRATE, 100, -20, skill_get_time2(skill_id, skill_lv));
//		return 14; //To make sure a valid number is returned
//	}
//	}

	return card;
}

/*==========================================
 *
 *
 *------------------------------------------*/
int skill_castend_damage_id (struct block_list* src, struct block_list *bl, uint16 skill_id, uint16 skill_lv, t_tick tick, int flag, bool is_automatic)
{
	struct map_session_data *sd = NULL;
	struct status_data *tstatus;
	struct status_change *sc, *tsc;

	if (skill_id > 0 && !skill_lv) return 0;

	nullpo_retr(1, src);
	nullpo_retr(1, bl);

	if (src->m != bl->m)
		return 1;

	if (bl->prev == NULL)
		return 1;

	sd = BL_CAST(BL_PC, src);

	if (status_isdead(bl))
		return 1;

	if (skill_id && skill_get_type(skill_id) == BF_MAGIC && status_isimmune(bl) == 100)
	{	//GTB makes all targetted magic display miss with a single bolt.
		sc_type sct = status_skill2sc(skill_id);
		if(sct != STATUS_NONE)
			status_change_end(bl, sct, INVALID_TIMER);
		clif_skill_damage(src, bl, tick, status_get_amotion(src), status_get_dmotion(bl), 0, 1, skill_id, skill_lv, skill_get_hit(skill_id));
		return 1;
	}

	sc = status_get_sc(src);
	tsc = status_get_sc(bl);
	if (sc && !sc->count)
		sc = NULL; //Unneeded
	if (tsc && !tsc->count)
		tsc = NULL;

	tstatus = status_get_status_data(bl);

	map_freeblock_lock();
	if (!is_automatic){
		status_change_end(src, STATUS_INVISIBILITY, INVALID_TIMER);
	}
	switch(skill_id) {
	case SK_MC_CARTQUAKE:
	case SK_MC_CARTCYCLONE:
	case SK_MC_CARTBRUME:
	case SK_KN_WINDCUTTER:
	case SK_MC_CARTFIREWORKS:
	case SK_MS_CARTTERMINATION:
	case SK_MO_KIBLAST:
	case SK_SM_MAGNUM:
		if( flag&1 ) {
			// For players, damage depends on distance, so add it to flag if it is > 1
			// Cannot hit hidden targets
			skill_attack(skill_get_type(skill_id), src, src, bl, skill_id, skill_lv, tick, flag|SD_ANIMATION|(sd?distance_bl(src, bl):0));
		}
		break;
	case SK_BS_CARTSHRAPNEL:
		clif_soundeffectall(bl, "shrapnel.wav", 0, AREA);
		if( flag&1 ) {
			skill_attack(skill_get_type(skill_id), src, src, bl, skill_id, skill_lv, tick, flag|SD_ANIMATION|(sd?distance_bl(src, bl):0));
		}
		break;
	case SK_SM_BASH:
	case SK_MO_FALINGFIST:
	case SK_SM_TRAUMATIC_STRIKE:
	case SK_SM_LIGHTNING_STRIKE:
	case SK_SM_SPEARSTAB:
	case SK_MC_MAMMONITE:
	case SK_AS_DOUBLEATTACK:
	case SK_AC_DOUBLESTRAFE:
	case SK_AC_SPIRITUALSTRAFE:
	case SK_AC_CHARGEDARROW:
	case SK_AC_TRANQUILIZINGDART:
	case SK_AC_PARALIZINGDART:
	case SK_KN_PIERCE:
	case SK_KN_SPEARCANNON:
	case SK_TF_SNATCH:
	case SK_TF_SANDATTACK:
	case SK_AM_ACIDTERROR:
	case SK_CR_ACIDBOMB:
	case SK_BA_MELODYSTRIKE:
	case SK_CR_HOLYCROSS:
	case SK_CR_SMITE:
	case SK_CR_SHIELDBOOMERANG:
	case SK_CM_AURABLADE:
	case SK_CM_CLASHINGSPIRAL:
	case SK_CL_ARROWVULCAN:
	case SK_CL_DUMMY_ARROWVULCAN:
	case SK_CM_DUMMY_HUNDREDSPEAR:
	case SK_AS_DUMMY_SONICBLOW:
	case SK_HT_MAGICTOMAHAWK:
	case SK_PA_RAPIDSMITING:	// Shield Chain
	case SK_KN_RECKONING:
	case SK_HT_LIVINGTORNADO:
	case SK_RA_ZEPHYRSNIPING:
	case SK_TF_VENOMKNIFE:
	case SK_MS_TRIGGERHAPPYCART:
	case SK_AS_SHURIKEN:
	case SK_AS_KUNAI:
	case SK_EX_SOULDESTROYER:
	case SK_KN_SONICWAVE:
	case SK_PR_DUPLELUX_MELEE:
	case SK_RG_QUICKSHOT:
	case SK_BS_AXEBOOMERANG:
	case SK_MS_POWERSWING:
	case SK_RG_TRIANGLESHOT:
	case SK_CL_SEVERERAINSTORM_MELEE:
	case SK_BA_GREATECHO:
	case SK_AS_SONICBLOW:
	case SK_BS_HAMMERFALL:
	case SK_CR_MANDRAKERAID_ATK:
	case SK_RA_ULLREAGLETOTEM_ATK:
		skill_attack(BF_WEAPON,src,src,bl,skill_id,skill_lv,tick,flag);
		break;
	case SK_MS_HAMMERDOWNPROTOCOL:
		{
			if (flag & 1) {
				clif_soundeffectall(&sd->bl, "hammerdown_protocol.wav", 0, AREA);
				skill_attack(skill_get_type(skill_id), src, src, bl, skill_id, skill_lv, tick, (skill_area_temp[0]) > 0 ? SD_ANIMATION | skill_area_temp[0] : skill_area_temp[0]);
				skill_blown(src, bl, skill_get_blewcount(skill_id, skill_lv), -1, BLOWN_NONE);
			} else {
				clif_soundeffectall(&sd->bl, "hammerdown_protocol.wav", 0, AREA);
				skill_area_temp[0] = map_foreachinallrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), BL_CHAR, src, skill_id, skill_lv, tick, BCT_ENEMY, skill_area_sub_count);
				map_foreachinrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), BL_CHAR|BL_SKILL, src, skill_id, skill_lv, tick, flag | BCT_ENEMY | SD_SPLASH | 1, skill_castend_damage_id);
			}
		}
		break;
	case SK_MO_TRIPLEARMCANNON:
		skill_attack(BF_WEAPON,src,src,bl,skill_id,skill_lv,tick,flag);
		status_change_end(src, STATUS_GRAPPLE, INVALID_TIMER);
		break;
	case SK_SH_GATEOFHELL:
		{
			struct status_change *sc = status_get_sc(src);
			if(sc && !MonkSkillAttackRatioCalculator::is_in_combo(sc)){
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			if(sc && (MonkSkillAttackRatioCalculator::get_combo_counter(sc) < 3)){
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			skill_attack(BF_WEAPON,src,src,bl,skill_id,skill_lv,tick,flag);
		}
		break;
	case SK_SH_DRAGONDARKNESSFLAME:
		{
			struct status_change *sc = status_get_sc(src);
			if(sc && !MonkSkillAttackRatioCalculator::is_in_combo(sc)){
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			if(sc && (MonkSkillAttackRatioCalculator::get_combo_counter(sc) < 5)){
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			skill_attack(BF_WEAPON,src,src,bl,skill_id,skill_lv,tick,flag);
		}
		break;
	case SK_TF_POISONSLASH:
		skill_attack(BF_WEAPON,src,src,bl,skill_id,skill_lv,tick,flag);
		// skill_attack(BF_WEAPON,src,src,bl,SK_SM_BASH,skill_lv,tick,flag);
		break;
	

		
	case SK_ST_SHARPSHOOTING:
		clif_specialeffect(bl, 816, AREA);
		skill_area_temp[1] = bl->id;
		if (battle_config.skill_eightpath_algorithm) {
			//Use official AoE algorithm
			if (!(map_foreachindir(skill_attack_area, src->m, src->x, src->y, bl->x, bl->y,
			   skill_get_splash(skill_id, skill_lv), skill_get_maxcount(skill_id, skill_lv), 0, splash_target(src),
			   skill_get_type(skill_id), src, src, skill_id, skill_lv, tick, flag, BCT_ENEMY))) {
				//These skills hit at least the target if the AoE doesn't hit
				skill_attack(skill_get_type(skill_id), src, src, bl, skill_id, skill_lv, tick, flag);
			}
		} else {
			map_foreachinpath(skill_attack_area, src->m, src->x, src->y, bl->x, bl->y,
				skill_get_splash(skill_id, skill_lv), skill_get_maxcount(skill_id, skill_lv), splash_target(src),
				skill_get_type(skill_id), src, src, skill_id, skill_lv, tick, flag, BCT_ENEMY);
		}
		status_change_end(src, STATUS_CAMOUFLAGE, INVALID_TIMER);
		break;


	case SK_MO_OCCULTIMPACT:
		skill_attack(BF_WEAPON,src,src,bl,skill_id,skill_lv,tick,flag);
		status_change_end(src, STATUS_GRAPPLE, INVALID_TIMER);
		break;

	case SK_RG_BACKSTAB:
		{
			if (!check_distance_bl(src, bl, 0)) {
				uint8 dir = map_calc_dir(src, bl->x, bl->y);
				short x, y;

				if (dir > 0 && dir < 4)
					x = -1;
				else if (dir > 4)
					x = 1;
				else
					x = 0;

				if (dir > 2 && dir < 6)
					y = -1;
				else if (dir == 7 || dir < 2)
					y = 1;
				else
					y = 0;

				if (battle_check_target(src, bl, BCT_ENEMY) > 0 && unit_movepos(src, bl->x + x, bl->y + y, 2, true)) { // Display movement + animation.

					status_change_end(src, STATUS_HIDING, INVALID_TIMER);
					dir = dir < 4 ? dir+4 : dir-4; // change direction [Celest]
					unit_setdir(bl,dir);

					clif_blown(src);
					skill_attack(BF_WEAPON, src, src, bl, skill_id, skill_lv, tick, flag);
				}
				else if (sd)
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			}
		}
		break;

	case SK_MO_TRHOWSPIRITSPHERE:
		skill_attack(BF_WEAPON,src,src,bl,skill_id,skill_lv,tick,flag);
		if (battle_config.finger_offensive_type && sd) {
			for (int i = 1; i < sd->spiritball_old; i++)
				skill_addtimerskill(src, tick + i * 200, bl->id, 0, 0, skill_id, skill_lv, BF_WEAPON, flag);
		}
		status_change_end(src, STATUS_GRAPPLE, INVALID_TIMER);
		break;


	case SK_SH_ASURASTRIKE:
		{
			struct block_list *mbl = bl; // For NJ_ISSEN
			short x, y, i = 2; // Move 2 cells (From target)
			short dir = map_calc_dir(src,bl->x,bl->y);
			struct status_data *status = status_get_status_data(src);
			if(status->sp<100){
				clif_skill_fail(sd, skill_id, USESKILL_FAIL, 0);
				break;
			}
			if (skill_id == SK_SH_ASURASTRIKE && sd && sd->spiritball_old > 5)
				flag |= 1; // Give +100% damage increase
			status = status_get_status_data(bl);
			skill_attack(BF_WEAPON,src,src,bl,skill_id,skill_lv,tick,flag);
			if (skill_id == SK_SH_ASURASTRIKE) {
				status_set_sp(src, 0, 0);
				status_change_end(src, STATUS_GRAPPLE, INVALID_TIMER);
			} else {
				status_set_hp(src, 1, 0);
				status_change_end(src, STATUS_HIDING, INVALID_TIMER);
			}
			if (skill_id == SK_SH_ASURASTRIKE) {
				mbl = src; // For SK_SH_ASURASTRIKE
				i = 3; // Move 3 cells (From caster)
			}
			if (dir > 0 && dir < 4)
				x = -i;
			else if (dir > 4)
				x = i;
			else
				x = 0;
			if (dir > 2 && dir < 6)
				y = -i;
			else if (dir == 7 || dir < 2)
				y = i;
			else
				y = 0;
			// Ashura Strike still has slide effect in GVG
			if ((mbl == src || (!map_flag_gvg2(src->m) && !map_getmapflag(src->m, MF_BATTLEGROUND))) &&
				unit_movepos(src, mbl->x + x, mbl->y + y, 1, 1)) {
				clif_blown(src);
				clif_spiritball(src);
			}
		}
		break;

	

	//Splash attack skills.
	case SK_AS_PHANTOMMENACE:
	case SK_AS_GRIMTOOTH:
		flag |= SD_PREAMBLE; // a fake packet will be sent for the first target to be hit
	case SK_AS_VENOMSPLASHER:
	case SK_WG_SLASH:
	case SK_FC_BLITZBEAT:
	case SK_AC_ARROWSHOWER:
	case SK_AC_ARROWRAIN:
	case SK_AL_HOLYGHOST:
	case SK_AL_SACREDWAVE:
	case SK_MO_TIGERFIST:
	case SK_EX_METEORASSAULT:
	case SK_BS_CROWDCONTROLSHOT:
	case SK_CM_IGNITIONBREAK:
	case SK_CM_HUNDREDSPEAR:
	case SK_PA_SHIELDSLAM:
	case SK_PR_JUDEX:
	case SK_WZ_EXTREMEVACUUM:
	case SK_WZ_REALITYBREAKER:
	case SK_SO_ASTRALSTRIKE:
	case SK_SO_DOOM:
	case SK_SO_DOOM_GHOST:
	case SK_SO_DIAMONDDUST:
	case SK_BI_PENITENTIA:
	case SK_WZ_MAGICCRASHER:
	case SK_WZ_PSYCHICWAVE:
	case SK_PR_SPIRITUSANCTI:
	case SK_WZ_ICONOFSIN:
	case SK_PR_UNHOLYCROSS:
	case SK_BI_SENTENTIA:
	case SK_BI_DIABOLICRUCIATUS:
	case SK_MG_VOIDEXPANSION:
	case SK_MG_EARTHBOLT:
	case SK_WZ_ILLUSIONARYBLADES:
	case SK_WZ_ICEBERG:
	case SK_WZ_CRIMSONROCK:
	case SK_SO_SHADOWBOMB:
	case SK_WZ_THUNDERSTORM:
	case SK_SO_PHANTOMSPEAR:
	case SK_RG_ARROWSTORM:
	case SK_BS_AXETORNADO:
	case SK_AS_ROLLINGCUTTER:
	case SK_RG_HACKANDSLASH:
	case SK_ST_FATALMENACE:
	case SK_BS_CARTCANNON:
	case SK_AM_BOMB:
	case SK_AM_FIREDEMONSTRATION:
	case SK_CR_INCENDIARYBOMB:
		if(skill_id == SK_BI_DIABOLICRUCIATUS)
			clif_soundeffectall(&sd->bl, "diabolicruciatus.wav", 0, AREA);
		if(skill_id == SK_BI_PENITENTIA){
			clif_specialeffect(src, EF_DARKCASTING2, AREA);
			clif_specialeffect(bl, EF_ADO_STR, AREA); //ado.str
			clif_soundeffectall(&sd->bl, "penitentia.wav", 0, AREA);
		}
		if(skill_id == SK_SO_SHADOWBOMB)
			clif_soundeffectall(&sd->bl, "SK_SO_SHADOWBOMB.wav", 0, AREA);
		if(skill_id == SK_WZ_REALITYBREAKER)
			clif_soundeffectall(&sd->bl, "from_the_abyss.wav", 0, AREA);
		if(skill_id == SK_SO_DIAMONDDUST)
			clif_soundeffectall(&sd->bl, "diamonddust.wav", 0, AREA);
		if(skill_id == SK_SO_ASTRALSTRIKE){
			clif_soundeffectall(&sd->bl, "astralstrike.wav", 0, AREA);
			sc_start4(src,bl,STATUS_ASTRALSTRIKE_DEBUFF,100,3,20,0,0,10000);
		}
		if(skill_id == SK_SO_DOOM){
			clif_soundeffectall(&sd->bl, "doom.wav", 0, AREA);
			skill_addtimerskill(src, tick + 500, bl->id, 0, 0, SK_SO_DOOM_GHOST, skill_lv, BF_MAGIC, flag);
		}
		if( flag&1 ) {//Recursive invocation
			int sflag = skill_area_temp[0] & 0xFFF;
			int heal = 0;
			std::bitset<INF2_MAX> inf2 = skill_db.find(skill_id)->inf2;

			

			if (skill_id == SK_BI_SENTENTIA && map_getcell(bl->m, bl->x, bl->y, CELL_CHKLANDPROTECTOR))
				break; // No damage should happen if the target is on Land Protector

			if( flag&SD_LEVEL )
				sflag |= SD_LEVEL; // -1 will be used in packets instead of the skill level
			if( skill_area_temp[1] != bl->id && !inf2[INF2_ISNPC] )
				sflag |= SD_ANIMATION; // original target gets no animation (as well as all NPC skills)

			
			heal = (int)skill_attack(skill_get_type(skill_id), src, src, bl, skill_id, skill_lv, tick, sflag);
			

			
		} else {
			int starget = BL_CHAR|BL_SKILL;

			skill_area_temp[0] = 0;
			skill_area_temp[1] = bl->id;
			skill_area_temp[2] = 0;

			if (sd && (false) && !battle_config.allow_es_magic_pc && bl->type != BL_MOB) {
				status_change_start(src, bl, STATUS_STUN, 10000, skill_lv, 0, 0, 0, 500, 10);
				clif_skill_fail(sd, skill_id, USESKILL_FAIL, 0);
				break;
			}

		
			switch ( skill_id ) {
				
				case SK_BS_CARTCANNON:
					clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
					break;
				case SK_MO_TIGERFIST:
				case SK_ST_FATALMENACE:
					clif_skill_damage(src,bl,tick, status_get_amotion(src), 0, -30000, 1, skill_id, skill_lv, DMG_SINGLE);
					break;
			
				case SK_MG_EARTHBOLT:
				case SK_WZ_ICEBERG:
				case SK_WZ_CRIMSONROCK:
				case SK_SO_SHADOWBOMB:
				case SK_WZ_THUNDERSTORM:
				case SK_SO_PHANTOMSPEAR:
					skill_area_temp[4] = bl->x;
					skill_area_temp[5] = bl->y;
					break;
			
			}

			// if skill damage should be split among targets, count them
			//SD_LEVEL -> Forced splash damage for Auto Blitz-Beat -> count targets
			//special case: Venom Splasher uses a different range for searching than for splashing
			if( flag&SD_LEVEL || skill_get_nk(skill_id, NK_SPLASHSPLIT) )
				skill_area_temp[0] = map_foreachinallrange(skill_area_sub, bl, (skill_id == SK_AS_VENOMSPLASHER)?1:skill_get_splash(skill_id, skill_lv), BL_CHAR, src, skill_id, skill_lv, tick, BCT_ENEMY, skill_area_sub_count);

			// recursive invocation of skill_castend_damage_id() with flag|1
			map_foreachinrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), starget, src, skill_id, skill_lv, tick, flag|BCT_ENEMY|SD_SPLASH|1, skill_castend_damage_id);

			if (skill_id == SK_RG_ARROWSTORM)
				status_change_end(src, STATUS_CAMOUFLAGE, INVALID_TIMER);
			if( skill_id == SK_AS_VENOMSPLASHER ) {
				map_freeblock_unlock(); // Don't consume a second gemstone.
				return 0;
			}
		}
		break;

	case SK_KN_BRANDISHSPEAR:
		skill_attack(skill_get_type(skill_id), src, src, bl, skill_id, skill_lv, tick, flag);
		break;
	
	case SK_KN_BOWLINGBASH:
		if (flag & 1) {
			skill_attack(skill_get_type(skill_id), src, src, bl, skill_id, skill_lv, tick, (skill_area_temp[0]) > 0 ? SD_ANIMATION | skill_area_temp[0] : skill_area_temp[0]);
			skill_blown(src, bl, skill_get_blewcount(skill_id, skill_lv), -1, BLOWN_NONE);
		} else {
			skill_area_temp[0] = map_foreachinallrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), BL_CHAR, src, skill_id, skill_lv, tick, BCT_ENEMY, skill_area_sub_count);
			map_foreachinrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), BL_CHAR|BL_SKILL, src, skill_id, skill_lv, tick, flag | BCT_ENEMY | SD_SPLASH | 1, skill_castend_damage_id);
		}
		break;
	

	case SK_MO_PALMSTRIKE: //	Palm Strike takes effect 1sec after casting. [Skotlex]
		clif_damage(src,bl,tick,status_get_amotion(src),0,-1,1,DMG_ENDURE,0,false); //Display an absorbed damage attack.
		skill_addtimerskill(src, tick + (100+status_get_amotion(src)), bl->id, 0, 0, skill_id, skill_lv, BF_WEAPON, flag);
		status_change_end(src, STATUS_GRAPPLE, INVALID_TIMER);
		break;
	case SK_PR_RESURRECTIO:
		if (!battle_check_undead(tstatus->race, tstatus->def_ele))
			break;
		skill_attack(BF_MAGIC,src,src,bl,skill_id,skill_lv,tick,flag);
		break;
	case SK_MG_CORRUPT:
	case SK_MG_SOULSTRIKE:
	case SK_MG_DARKSTRIKE:
	case SK_MG_COLDBOLT:
	case SK_MG_FIREBOLT:
	case SK_MG_LIGHTNINGBOLT:
	case SK_WZ_STALAGMITE:
	// case SK_CR_SWORDSTOPLOWSHARES:
	case SK_AL_HEAL:
	
	case SK_PR_ASPERSIO:
	case SK_MG_FROSTDIVER:

	case SK_PR_DUPLELUX_MAGIC:
	case SK_PA_GENESISRAY:
	case SK_BA_METALLICSOUND:
	case SK_BA_REVERBERATION:
	case SK_CL_METALLICFURY:
	
		skill_attack(BF_MAGIC,src,src,bl,skill_id,skill_lv,tick,flag);
		break;



	case SK_BI_BENEDICTIO:
		//Should attack undead and demons. [Skotlex]
		if (battle_check_undead(tstatus->race, tstatus->def_ele) || tstatus->race == RC_DEMON){
			clif_specialeffect(bl, EF_HOLYHIT, AREA);
			skill_attack(BF_MAGIC, src, src, bl, skill_id, skill_lv, tick, flag);
		}
		break;

	case SK_RA_FALCONASSAULT:
	case SK_TF_THROWSTONE:
		skill_attack(skill_get_type(skill_id),src,src,bl,skill_id,skill_lv,tick,flag);
		break;

	case SK_CM_DRAGONBREATH:
		if( tsc && tsc->data[STATUS_HIDING] )
			clif_skill_nodamage(src,src,skill_id,skill_lv,1);
		else {
			skill_attack(BF_MAGIC,src,src,bl,skill_id,skill_lv,tick,flag);
		}
		break;


	// Celest
	case SK_SA_SOULBURN:
		clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		skill_attack(BF_MAGIC,src,src,bl,skill_id,skill_lv,tick,flag);
		status_percent_damage(src, bl, 0, skill_lv*6.6, false);
		break;

	case SK_PA_GLORIADOMINI:
		skill_attack(BF_MAGIC,src,src,bl,skill_id,skill_lv,tick,flag);
		status_percent_damage(src, bl, 0, skill_lv*4, false);
		break;

	case SK_RG_SHADYSLASH:
		if( !map_flag_gvg2(src->m) && !map_getmapflag(src->m, MF_BATTLEGROUND) )
		{	//You don't move on GVG grounds.
			short x, y;
			map_search_freecell(bl, 0, &x, &y, 1, 1, 0);
			if (unit_movepos(src, x, y, 0, 0)) {
				clif_blown(src);
			}
		}
		status_change_end(src, STATUS_HIDING, INVALID_TIMER);
		skill_attack(BF_WEAPON,src,src,bl,skill_id,skill_lv,tick,flag);
		break;
	

	case SK_AS_CROSSRIPPERSLASHER:
		// if( sd && !(sc && sc->data[STATUS_ROLLINGCUTTER]) )
		// 	clif_skill_fail(sd,skill_id,USESKILL_FAIL_CONDITION,0);
		// else
		// {
		skill_attack(BF_WEAPON,src,src,bl,skill_id,skill_lv,tick,flag);
		// }
		break;
	case SK_EX_DUMMY_CROSSIMPACT:
		if (skill_check_unit_movepos(0, src, bl->x, bl->y, 1, 1)) {
			skill_blown(src, src, 1, (map_calc_dir(bl, src->x, src->y) + 4) % 8, BLOWN_IGNORE_NO_KNOCKBACK); // Target position is actually one cell next to the target
			skill_attack(BF_WEAPON, src, src, bl, SK_EX_CROSSIMPACT, skill_lv, tick, flag);
		} else {
			if (sd)
				clif_skill_fail(sd, SK_EX_CROSSIMPACT, USESKILL_FAIL, 0);
		}
		break;

	case SK_EX_DARKCLAW:
		skill_attack(BF_WEAPON, src, src, bl, skill_id, skill_lv, tick, flag);
		sc_start(src, bl, status_skill2sc(skill_id), 100, skill_lv, skill_get_time(skill_id, skill_lv)); // Should be applied even on miss
		break;

	case SK_SA_DRAINLIFE:
		{
			int heal = (int)skill_attack(skill_get_type(skill_id), src, src, bl, skill_id, skill_lv, tick, flag);
			heal = heal * (5 + 5 * skill_lv) / 100;

			if( bl->type == BL_SKILL )
				heal = 0; // Don't absorb heal from Ice Walls or other skill units.

			if( heal )
			{
				status_heal(src, heal, 0, 0);
				clif_skill_nodamage(NULL, src, SK_AL_HEAL, heal, 1);
			}
		}
		break;

	case SK_SO_TETRAVORTEX_FIRE:
	case SK_SO_TETRAVORTEX_WATER:
	case SK_SO_TETRAVORTEX_WIND:
	case SK_SO_TETRAVORTEX_GROUND:
	case SK_SO_TETRAVORTEX_NEUTRAL:
		skill_addtimerskill(src, tick + skill_area_temp[0] * 200, bl->id, skill_area_temp[1], 0, skill_id, skill_lv, 0, flag);
		break;



	case SK_SO_TETRAVORTEX:
		if (sd == nullptr) { // Monster usage
			uint8 i = 0;
			const static std::vector<std::vector<uint16>> tetra_skills = { { SK_SO_TETRAVORTEX_FIRE, 1 },
																		   { SK_SO_TETRAVORTEX_WIND, 4 },
																		   { SK_SO_TETRAVORTEX_WATER, 2 },
																		   { SK_SO_TETRAVORTEX_GROUND, 8 } };

			for (const auto &skill : tetra_skills) {
				if (skill_lv > 5) {
					skill_area_temp[0] = i;
					skill_area_temp[1] = skill[1];
					map_foreachinallrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), BL_CHAR, src, skill[0], skill_lv, tick, flag | BCT_ENEMY, skill_castend_damage_id);
				} else
					skill_addtimerskill(src, tick + i * 200, bl->id, skill[1], 0, skill[0], skill_lv, i, flag);
				i++;
			}
		} else if (sc) { // No SC? No spheres
			int i, k = 0;
			bool fire, wind, water, stone;
			fire = false;
			wind = false;
			water = false;
			stone = false;

			if (sc->data[STATUS_SPHERE_5]) // If 5 spheres, remove last one (based on reverse order) and only do 4 actions (Official behavior)
				status_change_end(src, STATUS_SPHERE_1, INVALID_TIMER);
			
			for (i = STATUS_SPHERE_5; i >= STATUS_SPHERE_1; i--) { // Loop should always be 4 for regular players, but unconditional_skill could be less
				if (sc->data[static_cast<sc_type>(i)] == nullptr)
					continue;

				uint16 subskill = 0;

				switch (sc->data[static_cast<sc_type>(i)]->val1) {
					case WLS_FIRE:
						subskill = SK_SO_TETRAVORTEX_FIRE;
						fire = true;
						k |= 1;
						break;
					case WLS_WIND:
						subskill = SK_SO_TETRAVORTEX_WIND;
						wind = true;
						k |= 4;
						break;
					case WLS_WATER:
						subskill = SK_SO_TETRAVORTEX_WATER;
						water = true;
						k |= 2;
						break;
					case WLS_STONE:
						subskill = SK_SO_TETRAVORTEX_GROUND;
						stone = true;
						k |= 8;
						break;
				}

				skill_area_temp[0] = abs(i - STATUS_SPHERE_5);
				skill_area_temp[1] = k;
				map_foreachinallrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), BL_CHAR, src, subskill, skill_lv, tick, flag | BCT_ENEMY, skill_castend_damage_id);
				status_change_end(src, static_cast<sc_type>(i), INVALID_TIMER);
			}
			if (fire && wind && water && stone){
				map_foreachinallrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), BL_CHAR, src, SK_SO_TETRAVORTEX_NEUTRAL, skill_lv, tick, flag | BCT_ENEMY, skill_castend_damage_id);
			}

		}
		break;

	case SK_MG_READING_SB:
		if (sc == nullptr) {
			break;
		}
		if (sd) {
			int i;
			skill_toggle_magicpower(src, skill_id); // No hit will be amplified
			bool found_spell = false;

			for (i = STATUS_MAXSPELLBOOK; i >= STATUS_SPELLBOOK1; i--) { // List all available spell to be released
				if (sc->data[i] != nullptr) {
					found_spell = true;
					break;
				}
			}
			if (!found_spell)
				break;
			// Now extract the data from the preserved spell
			uint16 pres_skill_id = sc->data[i]->val1;
			uint16 pres_skill_lv = sc->data[i]->val2;
			uint16 point = sc->data[i]->val3;

			status_change_end(src, static_cast<sc_type>(i), INVALID_TIMER);


			if( !skill_check_condition_castbegin(sd, pres_skill_id, pres_skill_lv) )
				break;

			// Get the requirement for the preserved skill
			skill_consume_requirement(sd, pres_skill_id, pres_skill_lv, 1);

			switch( skill_get_casttype(pres_skill_id) )
			{
				case CAST_GROUND:
					skill_castend_pos2(src, bl->x, bl->y, pres_skill_id, pres_skill_lv, tick, 0);
					break;
				case CAST_NODAMAGE:
					skill_castend_nodamage_id(src, bl, pres_skill_id, pres_skill_lv, tick, 0);
					break;
				case CAST_DAMAGE:
					skill_castend_damage_id(src, bl, pres_skill_id, pres_skill_lv, tick, 0);
					break;
			}

			sd->ud.canact_tick = i64max(tick + skill_delayfix(src, pres_skill_id, pres_skill_lv), sd->ud.canact_tick);
			clif_status_change(src, EFST_POSTDELAY, 1, skill_delayfix(src, pres_skill_id, pres_skill_lv), 0, 0, 0);

			int cooldown = pc_get_skillcooldown(sd,pres_skill_id, pres_skill_lv);

			if( cooldown > 0 )
				skill_blockpc_start(sd, pres_skill_id, cooldown);
		}
		break;
	
	// case SK_WG_SLASH:
	// 	if( sd && pc_isridingwug(sd) ){
	// 		short x[8]={0,-1,-1,-1,0,1,1,1};
	// 		short y[8]={1,1,0,-1,-1,-1,0,1};
	// 		uint8 dir = map_calc_dir(bl, src->x, src->y);

	// 		if( unit_movepos(src, bl->x+x[dir], bl->y+y[dir], 1, 1) ) {
	// 			clif_blown(src);
	// 			// skill_attack(BF_WEAPON, src, src, bl, skill_id, skill_lv, tick, flag);
	// 		}
	// 		break;
	// 	}
	

	case SK_SH_GUILLOTINEFISTS:
		skill_attack(BF_WEAPON,src,src,bl,skill_id,skill_lv,tick,flag);
		break;


	case SK_MO_CIRCULARFISTS:
		if( flag&1 ) { //by default cloaking skills are remove by aoe skills so no more checking/removing except hiding.
			skill_attack(BF_WEAPON, src, src, bl, skill_id, skill_lv, tick, flag);
			status_change_end(bl, STATUS_HIDING, INVALID_TIMER);
		} else {
			map_foreachinrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), BL_CHAR|BL_SKILL, src, skill_id, skill_lv, tick, flag|BCT_ENEMY|SD_SPLASH|1, skill_castend_damage_id);
			clif_skill_damage(src, src, tick, status_get_amotion(src), 0, -30000, 1, skill_id, skill_lv, DMG_SINGLE);
		}
		break;

	case SK_EX_POISONBUSTER:
		if( tsc && tsc->data[STATUS_POISON] ) {
			skill_attack(skill_get_type(skill_id), src, src, bl, skill_id, skill_lv, tick, flag);
			status_change_end(bl, STATUS_POISON, INVALID_TIMER);
		}
		else if( sd )
			clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
		break;
	/*case EL_FIRE_BOMB:
	case EL_FIRE_WAVE:
	case EL_WATER_SCREW:
	case EL_HURRICANE:
	case EL_TYPOON_MIS:
		if( flag&1 )
			skill_attack(skill_get_type(skill_id+1),src,src,bl,skill_id+1,skill_lv,tick,flag);
		else {
			int i = skill_get_splash(skill_id,skill_lv);
			clif_skill_nodamage(src,battle_get_master(src),skill_id,skill_lv,1);
			clif_skill_damage(src, bl, tick, status_get_amotion(src), 0, -30000, 1, skill_id, skill_lv, DMG_SINGLE);
			map_foreachinrange(skill_area_sub,bl,i,BL_CHAR,src,skill_id,skill_lv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id);
		}
		break;*/

	// case EL_ROCK_CRUSHER:
	// 	clif_skill_nodamage(src,battle_get_master(src),skill_id,skill_lv,1);
	// 	clif_skill_damage(src, src, tick, status_get_amotion(src), 0, -30000, 1, skill_id, skill_lv, DMG_SINGLE);
	// 	skill_attack(BF_MAGIC,src,src,bl,skill_id,skill_lv,tick,flag);

	case SK_SA_FIREBALL:
	case SK_SA_ICICLE:
	case SK_SA_WINDSLASH:
	case SK_SA_EARTHSPIKE:
	case EL_TORNADO_JG:
	case SK_SA_ROCKCRUSHER:
	case SK_SA_WATERBLAST:
	case SK_SA_FIREBOMB:
	case SK_AM_BASILISK1:
	case SK_AM_BEHOLDER1:
	case SK_CR_BEHOLDER3:
	case SK_AM_BASILISK2:
	case SK_AM_BEHOLDER2:
	case SK_AM_PETROLOGY:
	case SK_CR_HARMONIZE:
	case SK_AM_PYROTECHNIA:
	case SK_SM_PROVOKE:
		clif_skill_nodamage(src,battle_get_master(src),skill_id,skill_lv,1);
		clif_skill_damage(src, bl, tick, status_get_amotion(src), 0, -30000, 1, skill_id, skill_lv, DMG_SINGLE);
		skill_attack(skill_get_type(skill_id),src,src,bl,skill_id,skill_lv,tick,flag);
		break;


	case 0:/* no skill - basic/normal attack */
		if(sd) {
			if (flag & 3){
				if (bl->id != skill_area_temp[1])
					skill_attack(BF_WEAPON, src, src, bl, skill_id, skill_lv, tick, SD_LEVEL|flag);
			} else {
				skill_area_temp[1] = bl->id;
				map_foreachinallrange(skill_area_sub, bl,
					sd->bonus.splash_range, BL_CHAR,
					src, skill_id, skill_lv, tick, flag | BCT_ENEMY | 1,
					skill_castend_damage_id);
				flag|=1; //Set flag to 1 so ammo is not double-consumed. [Skotlex]
			}
		}
		break;


	default:
		ShowWarning("skill_castend_damage_id: Unknown skill used:%d\n",skill_id);
		clif_skill_damage(src, bl, tick, status_get_amotion(src), tstatus->dmotion,
			0, abs(skill_get_num(skill_id, skill_lv)),
			skill_id, skill_lv, skill_get_hit(skill_id));
		map_freeblock_unlock();
		return 1;
	}


	map_freeblock_unlock();

	if( sd && !(flag&1) )
	{// ensure that the skill last-cast tick is recorded
		sd->canskill_tick = gettick();

		if( sd->state.arrow_atk )
		{// consume arrow on last invocation to this skill.
			battle_consume_ammo(sd, skill_id, skill_lv);
		}

		// perform skill requirement consumption
		skill_consume_requirement(sd,skill_id,skill_lv,2);
	}

	return 0;
}

/**
 * Give a song's buff/debuff or damage to all targets around
 * @param target: Target
 * @param ap: Argument list
 * @return 1 on success or 0 otherwise
 */
static int skill_apply_songs(struct block_list* target, va_list ap)
{
	int flag = va_arg(ap, int);
	struct block_list* src = va_arg(ap, struct block_list*);
	uint16 skill_id = static_cast<uint16>(va_arg(ap, int));
	uint16 skill_lv = static_cast<uint16>(va_arg(ap, int));
	t_tick tick = va_arg(ap, t_tick);

	if (flag & BCT_WOS && src == target)
		return 0;

	if (battle_check_target(src, target, flag) > 0) {
		switch (skill_id) {
		default: // Buff/Debuff type songs
			return sc_start(src, target, status_skill2sc(skill_id), 100, skill_lv, skill_get_time(skill_id, skill_lv));
		}
	}

	return 0;
}

/**
 * Calculate a song's bonus values
 * @param src: Caster
 * @param skill_id: Song skill ID
 * @param skill_lv: Song skill level
 * @param tick: Timer tick
 * @return Number of targets or 0 otherwise
 */
static int skill_castend_song(struct block_list* src, uint16 skill_id, uint16 skill_lv, t_tick tick)
{
	nullpo_ret(src);

	if (src->type != BL_PC) {
		ShowWarning("skill_castend_song: Expected player type for src!\n");
		return 0;
	}

	// if (!(skill_get_inf2_(skill_id, { INF2_ISSONG, INF2_ISENSEMBLE }))) {
	// 	ShowWarning("skill_castend_song: Unknown song skill ID: %u\n", skill_id);
	// 	return 0;
	// }

	struct map_session_data* sd = BL_CAST(BL_PC, src);
	int flag = BCT_PARTY;

	clif_skill_nodamage(src, src, skill_id, skill_lv, 1);
	sd->skill_id_dance = skill_id;
	sd->skill_lv_dance = skill_lv;

	if (skill_get_inf2(skill_id, INF2_ISENSEMBLE))
		skill_check_pc_partner(sd, skill_id, &skill_lv, 3, 1);

	return map_foreachinrange(skill_apply_songs, src, skill_get_splash(skill_id, skill_lv), splash_target(src), flag, src, skill_id, skill_lv, tick);
}

/**
 * Use no-damage skill from 'src' to 'bl
 * @param src Caster
 * @param bl Target of the skill, bl maybe same with src for self skill
 * @param skill_id
 * @param skill_lv
 * @param tick
 * @param flag Various value, &1: Recursive effect
 **/
int skill_castend_nodamage_id (struct block_list *src, struct block_list *bl, uint16 skill_id, uint16 skill_lv, t_tick tick, int flag)
{
	struct map_session_data *sd, *dstsd;
	struct mob_data *md, *dstmd;
	struct elemental_data* ed;
	struct homun_data *hd;
	struct mercenary_data *mer;
	struct status_data *sstatus, *tstatus;
	struct status_change *tsc;
	struct status_change_entry *tsce;

	int i = 0;
	enum sc_type type;

	if(skill_id > 0 && !skill_lv) return 0;	// celest

	nullpo_retr(1, src);
	nullpo_retr(1, bl);

	if (src->m != bl->m)
		return 1;

	sd = BL_CAST(BL_PC, src);
	ed = BL_CAST(BL_ELEM, src);
	hd = BL_CAST(BL_HOM, src);
	md = BL_CAST(BL_MOB, src);
	mer = BL_CAST(BL_MER, src);

	dstsd = BL_CAST(BL_PC, bl);
	dstmd = BL_CAST(BL_MOB, bl);

	if(bl->prev == NULL)
		return 1;
	if(status_isdead(src))
		return 1;

	if( src != bl && status_isdead(bl) ) {
		switch( skill_id ) { // Skills that may be cast on dead targets
	
			case SK_PR_RESURRECTIO:
			case SK_BA_ALLNIGHTLONG:
				break;
			default:
				return 1;
		}
	}

	tstatus = status_get_status_data(bl);
	sstatus = status_get_status_data(src);

	if (bl->type == BL_PC && skill_id && skill_db.find(skill_id)->copyable.option) 
		skill_do_copy(src,bl,skill_id,skill_lv);
	
	//Check for undead skills that convert a no-damage skill into a damage one. [Skotlex]
	switch (skill_id) {
		case SK_CR_SUNLIGHT:
		case SK_AM_WARMWIND:
			if (bl->type != BL_HOM) {
				if (sd) clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0) ;
				break ;
			}
		// case SK_CR_BINDINGOATH:
		// 	clif_specialeffect(bl, EF_FOOD06, AREA);
		// 	clif_specialeffect(src, EF_FOOD06, AREA);
		// 	if (sd && battle_check_undead(tstatus->race,tstatus->def_ele)) {
		// 		if (battle_check_target(src, bl, BCT_ENEMY) < 1) {
		// 			//Offensive heal does not works on non-enemies. [Skotlex]
		// 			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
		// 			return 0;
		// 		}
		// 		return skill_castend_damage_id (src, bl, skill_id, skill_lv, tick, flag);
		// 	}
		// break;
		// case SK_CR_SWORDSTOPLOWSHARES:
		// 	clif_specialeffect(bl, EF_FOOD05, AREA);
		// 	if (sd && battle_check_undead(tstatus->race,tstatus->def_ele)) {
		// 		if (battle_check_target(src, bl, BCT_ENEMY) < 1) {
		// 			//Offensive heal does not works on non-enemies. [Skotlex]
		// 			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
		// 			return 0;
		// 		}
		// 		return skill_castend_damage_id (src, bl, skill_id, skill_lv, tick, flag);
		// 	}
		// 	break;
		case SK_AL_HEAL:
		case SK_PR_RESURRECTIO:
		case SK_PR_ASPERSIO:
			//Apparently only player casted skills can be offensive like this.
			if (sd && battle_check_undead(tstatus->race,tstatus->def_ele)) {
				if (battle_check_target(src, bl, BCT_ENEMY) < 1) {
					//Offensive heal does not works on non-enemies. [Skotlex]
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
					return 0;
				}
				return skill_castend_damage_id (src, bl, skill_id, skill_lv, tick, flag);
			}
			break;
		
		default:
			//Skill is actually ground placed.
			if (src == bl && skill_get_unit_id(skill_id))
				return skill_castend_pos2(src,bl->x,bl->y,skill_id,skill_lv,tick,0);
	}

	type = status_skill2sc(skill_id);
	tsc = status_get_sc(bl);
	tsce = (tsc && type != -1)?tsc->data[type]:NULL;

	if (skill_id != SK_EX_INVISIBILITY && tsc && tsc->data[STATUS_INVISIBILITY] ) {
		status_change_end(bl, STATUS_INVISIBILITY, INVALID_TIMER);
	}

	if (src!=bl && type > -1 &&
		CHK_ELEMENT((i = skill_get_ele(skill_id, skill_lv))) && i > ELE_NEUTRAL &&
		skill_get_inf(skill_id) != INF_SUPPORT_SKILL &&
		battle_attr_fix(NULL, NULL, 100, i, tstatus->def_ele, tstatus->ele_lv) <= 0)
		return 1; //Skills that cause an status should be blocked if the target element blocks its element.

	map_freeblock_lock();
	switch(skill_id)
	{
	case SK_CR_GEOGRAFIELD: {
		clif_specialeffect(&sd->bl, EF_SPR_PLANT, AREA);
		int area = skill_get_splash(SK_CR_GEOGRAFIELD, skill_lv);
		// for( i = 0; i < 3 + (skill_lv/2); i++ ) {
			int x1 = sd->bl.x - area + rnd()%(area * 2 + 1);
			int y1 = sd->bl.y - area + rnd()%(area * 2 + 1);
			skill_addtimerskill(src,tick+i*150,0,x1,y1,SK_CR_GEOGRAFIELD_ATK,skill_lv,-1,0);
		// }

		if (sd == NULL || sd->status.party_id == 0 || (flag & 1)) {
			sc_start2(src, bl, STATUS_GEOGRAFIELD, 100, skill_lv, (src == bl) ? 1 : 0, 20000);
		} else if (sd) {
			party_foreachsamemap(skill_area_sub,
				sd,skill_get_splash(skill_id, skill_lv),
				src,skill_id,skill_lv,tick, flag|BCT_PARTY|1,
				skill_castend_nodamage_id);
		}
	}
	break;
	case SK_AL_HEAL:
	// case SK_CR_SWORDSTOPLOWSHARES:
		{
			int heal = skill_calc_heal(src, bl, skill_id, skill_lv, true);
			int heal_get_jobexp;

			if (status_isimmune(bl) || (dstmd && (status_get_class(bl) == MOBID_EMPERIUM || status_get_class_(bl) == CLASS_BATTLEFIELD)))
				heal = 0;
			if( tsc && tsc->count ) {
				
			}
			
			clif_skill_nodamage (src, bl, skill_id, heal, 1);
			
			heal_get_jobexp = status_heal(bl,heal,0,0);
			

			if(sd && dstsd && heal > 0 && sd != dstsd && battle_config.heal_exp > 0){
				heal_get_jobexp = heal_get_jobexp * battle_config.heal_exp / 100;
				if (heal_get_jobexp <= 0)
					heal_get_jobexp = 1;
				pc_gainexp (sd, bl, 0, heal_get_jobexp, 0);
			}
		}
		break;

	

	case SK_PR_RESURRECTIO:
		if(sd && (map_flag_gvg2(bl->m) || map_getmapflag(bl->m, MF_BATTLEGROUND)))
		{	//No reviving in WoE grounds!
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			break;
		}
		if (!status_isdead(bl))
			break;
		{
			int per = 0, sper = 0;
			

			if (map_getmapflag(bl->m, MF_PVP) && dstsd && dstsd->pvp_point < 0)
				break;

			switch(skill_lv){
			case 1: per=10; break;
			case 2: per=30; break;
			case 3: per=50; break;
			case 4: per=80; break;
			case 5: per=100; break;
			}
			if(dstsd && dstsd->special_state.restart_full_recover)
				per = sper = 100;
			if (status_revive(bl, per, sper))
			{
				clif_skill_nodamage(src,bl,SK_PR_RESURRECTIO,skill_lv,1); //Both Redemptio and Res show this skill-animation.
				if(sd && dstsd && battle_config.resurrection_exp > 0)
				{
					t_exp exp = 0,jexp = 0;
					int lv = dstsd->status.base_level - sd->status.base_level, jlv = dstsd->status.job_level - sd->status.job_level;
					if(lv > 0 && pc_nextbaseexp(dstsd)) {
						exp = (t_exp)(dstsd->status.base_exp * lv * battle_config.resurrection_exp / 1000000.);
						if (exp < 1) exp = 1;
					}
					if(jlv > 0 && pc_nextjobexp(dstsd)) {
						jexp = (t_exp)(dstsd->status.job_exp * lv * battle_config.resurrection_exp / 1000000.);
						if (jexp < 1) jexp = 1;
					}
					if(exp > 0 || jexp > 0)
						pc_gainexp (sd, bl, exp, jexp, 0);
				}
			}
		}
		break;
	case SK_AM_HEALPULSE:
		if( !BL_CAST( BL_PC, bl ) || BL_CAST( BL_PC, bl )->status.party_id == 0 || flag&1 ) {
				int healing,research_skill_lv, matk = 0;
				struct map_session_data *msd = BL_CAST(BL_PC, bl);
				research_skill_lv = pc_checkskill(msd, SK_CR_HOMUNCULUSRESEARCH);
				matk = rand()%(ed->elemental.matk-ed->elemental.matk_min + 1) + ed->elemental.matk_min;

				healing = (200 * skill_lv) + (status_get_lv(src) * 3) + (status_get_int(src) * 3) + (matk * 3) + (research_skill_lv*200);
				clif_specialeffect(bl, 1000, AREA);
				clif_specialeffect(src, EF_GROUNDSHAKE, AREA);
				clif_skill_nodamage(src,bl,SK_AM_HEALPULSE,healing,1);
				clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing,1);
				status_heal(bl,healing,0,0);
		} else if( BL_CAST( BL_PC, bl ) ){
			party_foreachsamemap(skill_area_sub, BL_CAST( BL_PC, bl ), skill_get_splash(skill_id, skill_lv), bl, skill_id, skill_lv, tick, flag|BCT_PARTY|1, skill_castend_nodamage_id);
		}
		break;
	case SK_AM_PYROTECHNIA:
		if( !BL_CAST( BL_PC, bl ) || BL_CAST( BL_PC, bl )->status.party_id == 0 || flag&1 ) {
				clif_soundeffectall(bl, "pyrotechnia.wav", 0, AREA);
				clif_specialeffect(bl, EF_EL_TROPIC, AREA);
				clif_specialeffect(bl, 1270, AREA);
				clif_specialeffect(src, EF_GUMGANG7, AREA);	
				clif_skill_nodamage (src, bl, skill_id, skill_lv,
					sc_start(src,bl, type, 100, skill_lv*2, skill_get_time(skill_id,skill_lv)));
		} else if( BL_CAST( BL_PC, bl )) {
			party_foreachsamemap(skill_area_sub, BL_CAST( BL_PC, bl ), skill_get_splash(skill_id, skill_lv),  bl, skill_id, skill_lv, tick, flag|BCT_PARTY|1, skill_castend_nodamage_id);
		}
		break;
	case SK_AM_PETROLOGY:
		clif_soundeffectall(bl, "petology.wav", 0, AREA);
		clif_specialeffect(bl, EF_EL_CURSED_SOIL, AREA);
		clif_specialeffect(bl, 1273, AREA);
    	clif_specialeffect(src, EF_GUMGANG9, AREA);	
		clif_skill_nodamage (src, bl, skill_id, skill_lv,
			sc_start(src,bl, type, 100, skill_lv*2, skill_get_time(skill_id,skill_lv)));
		break;
	case SK_CR_HARMONIZE:
		clif_soundeffectall(bl, "harmonize.wav", 0, AREA);
		clif_specialeffect(bl, 1385, AREA);
		clif_specialeffect(bl, 1272, AREA);
    	clif_specialeffect(src, EF_GUMGANG8, AREA);	
		clif_skill_nodamage (src, bl, skill_id, skill_lv,
			sc_start(src,bl, type, 100, skill_lv*2, skill_get_time(skill_id,skill_lv)));
		break;
	
	// case SK_MO_DECAGI:
	

	case SK_AL_SIGNUMCRUCIS:
		if (flag&1)
			sc_start(src,bl,type, 23+skill_lv*4 +status_get_lv(src) -status_get_lv(bl), skill_lv,skill_get_time(skill_id,skill_lv));
		else {
			map_foreachinallrange(skill_area_sub, src, skill_get_splash(skill_id, skill_lv), BL_CHAR,
				src, skill_id, skill_lv, tick, flag|BCT_ENEMY|1, skill_castend_nodamage_id);
			clif_skill_nodamage(src, bl, skill_id, skill_lv, 1);
		}
		sc_start4(src, src, STATUS_CRUCIS_PLAYER, 100, skill_lv, 20, 0, 0, 20000*skill_lv);
		break;

	

	case SK_SA_SILENCE:
		if (tsce)
			status_change_end(bl, type, INVALID_TIMER);
		else
			skill_addtimerskill(src, tick+1000, bl->id, 0, 0, skill_id, skill_lv, 100, flag);
		clif_skill_nodamage (src, bl, skill_id, skill_lv, 1);
		break;

	case SK_CL_MARIONETTECONTROL:
		{
			struct status_change* sc = status_get_sc(src);

			if( sd && dstsd && (dstsd->class_ == JOB_CLOWN || dstsd->class_ == JOB_GYPSY))
			{// Cannot cast on another bard/dancer-type class of the same gender as caster
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				map_freeblock_unlock();
				return 1;
			}

			if( sc && tsc )
			{
				if( !sc->data[STATUS_MARIONETTE] && !tsc->data[STATUS_MARIONETTE2] )
				{
					clif_specialeffect(bl, 269, AREA);
					sc_start(src,src,STATUS_MARIONETTE,100,bl->id,skill_get_time(skill_id,skill_lv));
					sc_start(src,bl,STATUS_MARIONETTE2,100,src->id,skill_get_time(skill_id,skill_lv));
					clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
				}
				else
				if(  sc->data[STATUS_MARIONETTE ] &&  sc->data[STATUS_MARIONETTE ]->val1 == bl->id &&
					tsc->data[STATUS_MARIONETTE2] && tsc->data[STATUS_MARIONETTE2]->val1 == src->id )
				{
					status_change_end(src, STATUS_MARIONETTE, INVALID_TIMER);
					status_change_end(bl, STATUS_MARIONETTE2, INVALID_TIMER);
				}
				else
				{
					if( sd )
						clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);

					map_freeblock_unlock();
					return 1;
				}
			}
		}
		break;

	case SK_RG_CLOSECONFINE:
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start4(src,bl,type,100,skill_lv,src->id,0,0,skill_get_time(skill_id,skill_lv)));
		break;


	case SK_PR_ASPERSIO:
		if (sd && dstmd) {
			clif_skill_nodamage(src,bl,skill_id,skill_lv,0);
			break;
		}
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;

	

	case SK_PR_KYRIE:
		clif_skill_nodamage(bl,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_MC_CARTQUAKE:
		MerchntSkillAtkRatioCalculator::add_cart_quake_effects(src);
		skill_area_temp[1] = 0;
			map_foreachinshootrange(skill_area_sub, src, skill_get_splash(skill_id, skill_lv), BL_SKILL|BL_CHAR,
				src,skill_id,skill_lv,tick, flag|BCT_ENEMY|1, skill_castend_damage_id);
			clif_skill_nodamage (src,src,skill_id,skill_lv,1);
			break;
	case SK_MC_CARTCYCLONE:
		MerchntSkillAtkRatioCalculator::add_cart_cyclone_special_effects(src);
		skill_area_temp[1] = 0;
			map_foreachinshootrange(skill_area_sub, src, skill_get_splash(skill_id, skill_lv), BL_SKILL|BL_CHAR,
				src,skill_id,skill_lv,tick, flag|BCT_ENEMY|1, skill_castend_damage_id);
			clif_skill_nodamage (src,src,skill_id,skill_lv,1);
			break;
	case SK_MC_CARTBRUME:
		MerchntSkillAtkRatioCalculator::add_cart_brume_special_effects(src);
		skill_area_temp[1] = 0;
			map_foreachinshootrange(skill_area_sub, src, skill_get_splash(skill_id, skill_lv), BL_SKILL|BL_CHAR,
				src,skill_id,skill_lv,tick, flag|BCT_ENEMY|1, skill_castend_damage_id);
			clif_skill_nodamage (src,src,skill_id,skill_lv,1);
			break;
	case SK_BS_CARTSHRAPNEL:
		MerchntSkillAtkRatioCalculator::add_cart_sharpnel_special_effects(src);
		skill_area_temp[1] = 0;
			map_foreachinshootrange(skill_area_sub, src, skill_get_splash(skill_id, skill_lv), BL_SKILL|BL_CHAR,
				src,skill_id,skill_lv,tick, flag|BCT_ENEMY|1, skill_castend_damage_id);
			clif_skill_nodamage (src,src,skill_id,skill_lv,1);
			break;
	case SK_MC_CARTFIREWORKS:
		clif_soundeffectall(&sd->bl, "fireworks.wav", 0, AREA);
		MerchntSkillAtkRatioCalculator::add_cart_fireworks_special_effects(src);
		skill_area_temp[1] = 0;
			map_foreachinshootrange(skill_area_sub, src, skill_get_splash(skill_id, skill_lv), BL_SKILL|BL_CHAR,
				src,skill_id,skill_lv,tick, flag|BCT_ENEMY|1, skill_castend_damage_id);
			clif_skill_nodamage (src,src,skill_id,skill_lv,1);
			break;
	case SK_AM_BASILISK2:
		clif_soundeffectall(src, "daunting_blade.wav", 0, AREA);
		clif_specialeffect(src, 156, AREA); //jobchange
		skill_area_temp[1] = 0;
			map_foreachinshootrange(skill_area_sub, src, skill_get_splash(skill_id, skill_lv), BL_SKILL|BL_CHAR,
				src,skill_id,skill_lv,tick, flag|BCT_ENEMY|1, skill_castend_damage_id);
			clif_skill_nodamage (src,src,skill_id,skill_lv,1);
			break;
	case SK_AM_BEHOLDER2:
		clif_soundeffectall(src, "arcaneblast.wav", 0, AREA);
		clif_specialeffect(src, 1218, AREA);
		skill_area_temp[1] = 0;
			map_foreachinshootrange(skill_area_sub, src, skill_get_splash(skill_id, skill_lv), BL_SKILL|BL_CHAR,
				src,skill_id,skill_lv,tick, flag|BCT_ENEMY|1, skill_castend_damage_id);
			clif_skill_nodamage (src,src,skill_id,skill_lv,1);
			break;
	case SK_MS_CARTTERMINATION:
		clif_specialeffect(src, EF_CARTTER, AREA); 
		clif_specialeffect(src, 1375, AREA); //new_banishingpoint_07
		skill_area_temp[1] = 0;
			map_foreachinshootrange(skill_area_sub, src, skill_get_splash(skill_id, skill_lv), BL_SKILL|BL_CHAR,
				src,skill_id,skill_lv,tick, flag|BCT_ENEMY|1, skill_castend_damage_id);
			clif_skill_nodamage (src,src,skill_id,skill_lv,1);
			break;
	case SK_KN_WINDCUTTER:
		clif_specialeffect(src, EF_WINDCUTTER, AREA); 
		skill_area_temp[1] = 0;
		map_foreachinshootrange(skill_area_sub, src, skill_get_splash(skill_id, skill_lv), BL_SKILL|BL_CHAR,
			src,skill_id,skill_lv,tick, flag|BCT_ENEMY|1, skill_castend_damage_id);
		clif_skill_nodamage (src,src,skill_id,skill_lv,1);
		break;
	case SK_MO_KIBLAST:
		clif_specialeffect(src, EF_ENERVATION6, AREA); //weakness
		clif_soundeffectall(&sd->bl, "SK_MO_KIBLAST.wav", 0, AREA);

		skill_area_temp[1] = 0;
		map_foreachinshootrange(skill_area_sub, src, skill_get_splash(skill_id, skill_lv), BL_SKILL|BL_CHAR,
		src,skill_id,skill_lv,tick, flag|BCT_ENEMY|1, skill_castend_damage_id);
		clif_skill_nodamage (src,src,skill_id,skill_lv,1);
		break;
	case SK_SM_MAGNUM:
		skill_area_temp[1] = 0;
		map_foreachinshootrange(skill_area_sub, src, skill_get_splash(skill_id, skill_lv), BL_SKILL|BL_CHAR,
			src,skill_id,skill_lv,tick, flag|BCT_ENEMY|1, skill_castend_damage_id);
		clif_skill_nodamage (src,src,skill_id,skill_lv,1);
		// Initiate 20% of your damage becomes fire element.
		sc_start4(src,src,STATUS_WATK_ELEMENT,100,3,20,0,0,30000);
		break;
	

	case SK_BI_BENEDICTIO:
		if (!battle_check_undead(tstatus->race, tstatus->def_ele) && tstatus->race != RC_DEMON){
			clif_specialeffect(bl, EF_HOLYHIT, AREA);
			clif_skill_nodamage(src, bl, skill_id, skill_lv, sc_start(src, bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv)));
		}else{
			skill_castend_damage_id(src, src, skill_id, skill_lv, tick, flag);
		}
		break;
	case SK_CM_AURABLADE:
		clif_specialeffect(src, 110, AREA);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_CM_FRENZY:	
	case SK_CR_REFLECTSHIELD:
	case SK_TF_POISONREACT:
	case SK_MG_ENERGYCOAT:
	case SK_SH_ULTRAINSTINCT:
	case SK_SH_MENTALSTRENGTH:
	case SK_SH_GRAPPLE:
	case SK_CM_PARRY:
	case SK_CM_SPEAR_DYNAMO:
	case SK_BS_CARTBOOST:
	case SK_RA_FALCONEYES:
	case SK_ST_KILLERINSTIINCT:
	case SK_EX_ENCHANTDEADLYPOISON:
	case SK_PR_DUPLELUX:
	case SK_BI_OFFERTORIUM:
	case SK_SO_HINDSIGHT:
	case SK_AS_VENOMIMPRESS:
	case SK_CR_VANGUARDFORCE:
	case SK_SO_VOIDMAGEPRODIGY:
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_SO_MYSTICALAMPLIFICATION:
		clif_specialeffect(src, 100, AREA);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_PF_MAGICSQUARED:
		clif_specialeffect(src, 1259, AREA);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_RA_ULLREAGLETOTEM:
		clif_specialeffect(bl, 1381, AREA);//new_dragonbreath_05_clock
		clif_soundeffectall(bl, "ullr.wav", 0, AREA);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_KN_RECKONING:
		clif_specialeffect(src, 1138, AREA);
		clif_soundeffectall(bl, "reckoning.wav", 0, AREA);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_BI_INLUCEMBATALIA:
		clif_soundeffectall(bl, "ultor.wav", 0, AREA);
		clif_specialeffect(bl, 1260, AREA);
		clif_specialeffect(bl, EF_RECOVERY, AREA);
		clif_specialeffect(bl, EF_LIGHTSPHERE, AREA);
		clif_skill_nodamage(src, bl, skill_id, skill_lv,
			sc_start(src, bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv)));
		break;
	case SK_BI_EXPIATIO:
		clif_specialeffect(bl, EF_RK_LUXANIMA, AREA);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_BI_LAUDATEDOMINIUM:
		clif_specialeffect(bl, EF_ANGEL2, AREA);
		clif_specialeffect(bl, EF_ITEM_LIGHT, AREA);
		clif_specialeffect(bl, EF_FLOWERCAST, AREA);
		clif_specialeffect(bl, EF_LAUAGNUS_STR, AREA);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_KN_FURY:
		clif_specialeffect(bl, 847, AREA);
		clif_specialeffect(bl, 626, AREA);
		clif_specialeffect(bl, EF_MADNESS_RED, AREA);
		clif_soundeffectall(&sd->bl, "SK_KN_FURY.wav", 0, AREA);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_MC_COINFLIP:
		clif_specialeffect(bl, 18, AREA);
		int result;
		result = rand() % 2;
		if(result == 0){
			clif_skill_nodamage(src,bl,skill_id,skill_lv,sc_start4(src,src,STATUS_COINFLIP_DEX,100,3,20,0,0,90000));
			status_change_end(src, STATUS_COINFLIP_LUK, INVALID_TIMER);

		} else {
			clif_skill_nodamage(src,bl,skill_id,skill_lv, sc_start4(src,src,STATUS_COINFLIP_LUK,100,3,20,0,0,90000));
			status_change_end(src, STATUS_COINFLIP_DEX	, INVALID_TIMER);
		}
		break;
	case SK_ST_UNLIMIT:
    	clif_specialeffect(bl, 813, AREA);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_PA_FORTIFY:
		clif_specialeffect(src, EF_MILSHIELD_STR, AREA);
		clif_specialeffect(src, EF_SHRINK, AREA);
		clif_specialeffect(src, EF_CHEMICAL3S, AREA);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_WZ_FORESIGHT:
		clif_specialeffect(src, EF_SECRA, AREA);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_RA_ZEPHYRSNIPING:
		clif_specialeffect(src, 1403, AREA);//new_cannon_spear_05_clock
		clif_soundeffectall(&sd->bl, "zephyr_sniping.wav", 0, AREA);
		clif_skill_nodamage(src, bl, skill_id, skill_lv,
			sc_start(src, bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv)));
		break;

	case SK_HT_LIVINGTORNADO:
		clif_specialeffect(src, 1401, AREA);//new_cannon_spear_01_clock
		clif_soundeffectall(&sd->bl, "tornado_fury.wav", 0, AREA);
		clif_skill_nodamage(src, bl, skill_id, skill_lv,
			sc_start(src, bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv)));
		break;
	

	case SK_KN_COUNTERATTACK:
		sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv));
		skill_addtimerskill(src,tick + 100,bl->id,0,0,skill_id,skill_lv,BF_WEAPON,flag);
		break;

	case SK_MS_ARMORREINFORCEMENT:
		if( sd && dstmd ) {
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
		}else {
			short i;
			struct map_session_data* tsd = BL_CAST( BL_PC, bl );
			i = tsd->equip_index[EQI_ARMOR];
			if ( i < 0 || !tsd->inventory_data[i] ) {
			 	clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			 	break;
			}
			clif_specialeffect(bl, EF_HAMIDEFENCE, AREA);
			clif_specialeffect(bl, EF_REFINEOK, AREA);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,
				sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		}
		break;
			
	case SK_BI_ASSUMPTIO:
		if( sd && dstmd )
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
		else
			clif_skill_nodamage(src,bl,skill_id,skill_lv,
				sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_AL_RUWACH:
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start2(src,bl,type,100,skill_lv,skill_id,skill_get_time(skill_id,skill_lv)));
		break;
	
/* Was modified to only affect targetted char.	[Skotlex]
	case SK_BI_ASSUMPTIO:
		if (flag&1)
			sc_start(bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv));
		else
		{
			map_foreachinallrange(skill_area_sub, bl,
				skill_get_splash(skill_id, skill_lv), BL_PC,
				src, skill_id, skill_lv, tick, flag|BCT_ALL|1,
				skill_castend_nodamage_id);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		}
		break;
*/
	case SK_BS_ENDURE:
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;

	case SK_AS_ENCHANTPOISON: // Prevent spamming [Valaris]
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;

	case SK_AC_IMPROVECONCENTRATION:
		{
			int splash = skill_get_splash(skill_id, skill_lv);
			clif_specialeffect(src, EF_INCAGIDEX, AREA);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,
				sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
			skill_reveal_trap_inarea(src, splash, src->x, src->y);
			map_foreachinallrange( status_change_timer_sub, src,
				splash, BL_CHAR, src, NULL, type, tick);
		}
		break;
	case SK_RA_SPIRITANIMAL:
		clif_specialeffect(src, EF_LINELINK3, AREA);
		clif_specialeffect(src, EF_VIOLENTGALE, AREA);
		clif_specialeffect(src, 1237, AREA);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
				sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;

	case SK_SM_PROVOKE:
		if( status_has_mode(tstatus,MD_STATUSIMMUNE))  { //|| battle_check_undead(tstatus->race,tstatus->def_ele)
			map_freeblock_unlock();
			return 1;
		}
		// always 100% chance to provoke
		if(!(i = sc_start(src, bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv))))
		{
			if( sd ){
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			}		
			map_freeblock_unlock();
			return 0;
		}
		clif_skill_nodamage(src, bl, false ? SK_SM_PROVOKE : skill_id, skill_lv, i);
		//clif_skill_nodamage(src, bl, skill_id == SM_SELFPROVOKE ? SK_SM_PROVOKE : skill_id, skill_lv, i);
		unit_skillcastcancel(bl, 2);

		if( tsc && tsc->count )
		{
			status_change_end(bl, STATUS_FREEZE, INVALID_TIMER);
			if( tsc->data[STATUS_STONECURSE] && tsc->opt1 == OPT1_STONE )
				status_change_end(bl, STATUS_STONECURSE, INVALID_TIMER);
			status_change_end(bl, STATUS_SLEEP, INVALID_TIMER);
			status_change_end(bl, STATUS_TRICKDEAD, INVALID_TIMER);
		}
		if( dstmd )
		{
			dstmd->state.provoke_flag = src->id;
			mob_target(dstmd, src, skill_get_range2(src, skill_id, skill_lv, true));
		}
		break;

	case SK_CR_SWORNPROTECTOR:
		{
			int count, lv;
			if( !dstsd || (!sd && !mer) )
			{ // Only players can be devoted
				if( sd )
					clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
				break;
			}

			if( (lv = status_get_lv(src) - dstsd->status.base_level) < 0 )
				lv = -lv;
			if( lv > battle_config.devotion_level_difference || // Level difference requeriments
				(dstsd->sc.data[type] && dstsd->sc.data[type]->val1 != src->id) || // Cannot Devote a player devoted from another source
				(dstsd->class_&MAPID_UPPERMASK) == MAPID_CRUSADER // Crusaders Cannot be devoted
				// (dstsd->class_&MAPID_UPPERMASK) == MAPID_KNIGHT || // KNights Cannot be devoted
				// (dstsd->class_&MAPID_UPPERMASK) == MAPID_MONK || // Monks Cannot be devoted
				// (dstsd->class_&MAPID_UPPERMASK) == MAPID_BLACKSMITH || // Blacksmiths Cannot be devoted
			){
				if( sd )
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				map_freeblock_unlock();
				return 1;
			}

			i = 0;
			count = (sd)? min(skill_lv,MAX_DEVOTION) : 1; // Mercenary only can Devote owner
			if( sd )
			{ // Player Devoting Player
				ARR_FIND(0, count, i, sd->devotion[i] == bl->id );
				if( i == count )
				{
					ARR_FIND(0, count, i, sd->devotion[i] == 0 );
					if( i == count )
					{ // No free slots, skill Fail
						clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
						map_freeblock_unlock();
						return 1;
					}
				}

				sd->devotion[i] = bl->id;
			}
			

			clif_skill_nodamage(src, bl, skill_id, skill_lv,
				sc_start4(src, bl, type, 10000, src->id, i, skill_get_range2(src, skill_id, skill_lv, true), 0, skill_get_time2(skill_id, skill_lv)));
			clif_devotion(src, NULL);
		}
		break;


	case SK_MO_SUMMONSPIRITSPHERE:
		if(sd) {
			int limit = 5;
			if( pc_checkskill(sd, SK_SH_ZEN) )
				limit += pc_checkskill(sd, SK_SH_ZEN);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			for (i = 0; i < skill_lv; i++) {
				if (sd->spiritball < limit) {
					pc_addspiritball(sd,skill_get_time(skill_id,skill_lv),limit);
				}
			}
		}
		break;


	case SK_MO_ABSORBSPIRITSPHERE:
		{
			int sp_healed = 0;
			int hp_healed = 0;
			if (dstsd && (battle_check_target(src, bl, BCT_SELF) > 0 || 
				(battle_check_target(src, bl, BCT_ENEMY) > 0 && (map_flag_vs(src->m) || (sd && sd->duel_group && sd->duel_group == dstsd->duel_group))))){
				if (dstsd->spiritball > 0) {
					hp_healed = skill_lv * 300;
					sp_healed = skill_lv * 20;
					pc_delspiritball(dstsd,1,0);
				} else {
					clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
					break;
				}
			}
			if (sp_healed){
				status_heal(src, hp_healed, sp_healed, 2);
			}
			clif_skill_nodamage(src,bl,skill_id,skill_lv,i?1:0);
			break;
		}
	case SK_AC_MAKINGARROW:
		if(sd) {
			sd->menuskill_val2 = skill_lv;
			clif_arrow_create_list(sd);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		}
		break;

	case SK_AM_PHARMACY:
		if(sd) {
			clif_skill_produce_mix_list(sd,skill_id,22);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		}
		break;


	// case SK_BS_HAMMERFALL:
	// 	skill_addtimerskill(src, tick+1000, bl->id, 0, 0, skill_id, skill_lv, min(20+10*skill_lv, 50+5*skill_lv), flag);
	// 	break;

	case SK_AS_PHANTOMMENACE:
		skill_area_temp[1] = 0;
		clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		map_foreachinrange(skill_area_sub, bl,
			skill_get_splash(skill_id, skill_lv), BL_CHAR|BL_SKILL,
			src,skill_id,skill_lv,tick, flag|BCT_ENEMY|1,
			skill_castend_damage_id);
		break;

	//List of self skills that give damage around caster
	case SK_EX_METEORASSAULT:
	case SK_BS_CROWDCONTROLSHOT:
	case SK_BS_AXETORNADO:
	case SK_RG_HACKANDSLASH:
	{
		struct status_change *sc = status_get_sc(src);
		int starget = BL_CHAR|BL_SKILL;

		
		
		

		skill_area_temp[1] = 0;
		clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		i = map_foreachinrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), starget,
				src, skill_id, skill_lv, tick, flag|BCT_ENEMY|SD_SPLASH|1, skill_castend_damage_id);
		if( !i && ( skill_id == SK_KN_WINDCUTTER || skill_id == SK_BS_AXETORNADO ) )
			clif_skill_damage(src,src,tick, status_get_amotion(src), 0, -30000, 1, skill_id, skill_lv, DMG_SINGLE);
	}
		break;

	
	case SK_CM_IGNITIONBREAK:
		skill_area_temp[1] = 0;
#if PACKETVER >= 20180207
		clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
#else
		clif_skill_damage(src, src, tick, status_get_amotion(src), 0, -30000, 1, skill_id, skill_lv, DMG_SINGLE);
#endif
		map_foreachinrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), BL_CHAR|BL_SKILL, src, skill_id, skill_lv, tick, flag|BCT_ENEMY|SD_SPLASH|1, skill_castend_damage_id);
		break;

	case SK_MO_CIRCULARFISTS:
		clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
	
	case SK_ST_FATALMENACE:
		skill_castend_damage_id(src, src, skill_id, skill_lv, tick, flag);
		break;
		skill_castend_damage_id(src, src, skill_id, skill_lv, tick, flag);
		break;
	case SK_KN_BRANDISHSPEAR:
		map_foreachindir(skill_area_sub, src->m, src->x, src->y, bl->x, bl->y,
			skill_get_splash(skill_id, skill_lv), skill_get_maxcount(skill_id, skill_lv), 0, splash_target(src),
			src, skill_id, skill_lv, tick, flag | BCT_ENEMY | 0,
			skill_castend_damage_id);
		break;
	
	case SK_AL_ANGELUS:
	case SK_PR_SUFFRAGIUM:
	case SK_PR_IMPOSITIO:
	case SK_PR_MAGNIFICAT:
		if (sd == NULL || sd->status.party_id == 0 || (flag & 1)) {

			// Animations don't play when outside visible range
			if (check_distance_bl(src, bl, AREA_SIZE))
				clif_skill_nodamage(bl, bl, skill_id, skill_lv, 1);

			sc_start(src, bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv));
		}
		else if (sd)
			party_foreachsamemap(skill_area_sub, sd, skill_get_splash(skill_id, skill_lv), src, skill_id, skill_lv, tick, flag | BCT_PARTY | 1, skill_castend_nodamage_id);
		break;
	case SK_EX_DANCINGBLADES:
		{
			int right_hand_index = sd->equip_index[EQI_HAND_R];
			int left_hand_index = sd->equip_index[EQI_HAND_L];
			if(sd 
				&& right_hand_index >= 0 
				&& left_hand_index >= 0
				&& sd->status.weapon != W_KATAR
				&& sd->inventory_data[right_hand_index]->type == IT_WEAPON
				&& sd->inventory_data[left_hand_index]->type == IT_WEAPON
				){
				clif_specialeffect(src, 1134, AREA);
				clif_specialeffect(src, 59, AREA);
				clif_skill_nodamage(bl, bl, skill_id, skill_lv, sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
			}else{
				clif_skill_fail(sd,skill_id, USESKILL_FAIL,0);
			}
		}
		break;
	case SK_CR_OHQUICKEN:
	case SK_KN_OHQUICKEN:
	case SK_KN_SQUICKEN:
	case SK_KN_THQUICKEN:
	case SK_RG_DAGGERQUICKEN:
	case SK_BS_AXEQUICKEN:
	case SK_HT_BOWQUICKEN:
	case SK_SA_BOOKQUICKEN:
	case SK_MO_KNUCKLEQUICKEN:
	case SK_AL_MACEQUICKEN:
		clif_specialeffect(src, EF_TWOHANDQUICKEN, AREA);
		clif_skill_nodamage(bl, bl, skill_id, skill_lv, sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_HT_CYCLONICCHARGE:
		// clif_specialeffect(bl, EF_TURNUNDEAD, AREA);
		clif_soundeffectall(&sd->bl, "cyclonic_charge.wav", 0, AREA);

		clif_specialeffect(bl, 1400, AREA);//new_cannon_spear_12_clock
		clif_skill_nodamage(bl, bl, skill_id, skill_lv, sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_MC_UPROAR:
		clif_specialeffect(src, EF_DRAGONFEAR, AREA);
		if( sd == NULL || sd->status.party_id == 0 || (flag & 1) )
			clif_skill_nodamage(bl, bl, skill_id, skill_lv, sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		else if( sd )
			party_foreachsamemap(skill_area_sub, sd, skill_get_splash(skill_id, skill_lv), src, skill_id, skill_lv, tick, flag|BCT_PARTY|1, skill_castend_nodamage_id);
		break;
	case SK_SM_SPIRITGROWTH:
		clif_specialeffect(src, EF_BLUEBODY, AREA);
		clif_skill_nodamage(bl, bl, skill_id, skill_lv, sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_MC_LOUD:
		clif_skill_nodamage(bl, bl, skill_id, skill_lv, sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_SM_SPEARSTANCE:
		clif_specialeffect(src, EF_BEGINSPELL2, AREA);
		clif_skill_nodamage(bl, bl, skill_id, skill_lv, sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	case SK_HT_WINDRACER:
		if( sd == NULL || sd->status.party_id == 0 || (flag & 1) )
			clif_skill_nodamage(bl, bl, skill_id, skill_lv, sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		else if( sd )
			party_foreachsamemap(skill_area_sub, sd, skill_get_splash(skill_id, skill_lv), src, skill_id, skill_lv, tick, flag|BCT_PARTY|1, skill_castend_nodamage_id);
		break;
	
	case SK_KN_ADRENALINERUSH:
		// if (sd == NULL || sd->status.party_id == 0 || (flag & 1)) {
		// 	int weapontype = skill_get_weapontype(skill_id);
			// if (!weapontype || !dstsd || pc_check_weapontype(dstsd, weapontype)) {
		clif_specialeffect(src, EF_CONCENTRATION2, AREA);
		clif_skill_nodamage(bl, bl, skill_id, skill_lv,
			sc_start2(src, bl, type, 100, skill_lv, (src == bl) ? 1 : 0, skill_get_time(skill_id, skill_lv)));
		// 	}
		// }
		
		break;
	case SK_BS_WEAPONRYEXPERTISE:
	case SK_BS_WEAPONSHARPENING:
	case SK_BS_POWERTHRUST:
	case SK_MS_MAXPOWERTHRUST:
	case SK_MS_MELTDOWN:
		if (skill_id == SK_MS_MAXPOWERTHRUST){
			clif_specialeffect(src, 1138, AREA);
		}
		if (skill_id == SK_AM_PYROTECHNIA){
			clif_soundeffectall(bl, "pyrotechnia.wav", 0, AREA);
			clif_specialeffect(bl, EF_EL_TROPIC, AREA);
			clif_specialeffect(bl, 1270, AREA);
			clif_specialeffect(src, EF_GUMGANG7, AREA);	
		}
		if (sd == NULL || sd->status.party_id == 0 || (flag & 1)) {
			int weapontype = skill_get_weapontype(skill_id);
			if (!weapontype || !dstsd || pc_check_weapontype(dstsd, weapontype)) {
				clif_skill_nodamage(bl, bl, skill_id, skill_lv,
					sc_start2(src, bl, type, 100, skill_lv, (src == bl) ? 1 : 0, skill_get_time(skill_id, skill_lv)));
			}
		} else if (sd) {
			party_foreachsamemap(skill_area_sub,
				sd,skill_get_splash(skill_id, skill_lv),
				src,skill_id,skill_lv,tick, flag|BCT_PARTY|1,
				skill_castend_nodamage_id);
		}
		break;

	case SK_PA_DEFENDINGAURA:
		clif_specialeffect(src, 271, AREA);
		if( tsce )
		{
			clif_skill_nodamage(src,bl,skill_id,skill_lv,status_change_end(bl, type, INVALID_TIMER));
			map_freeblock_unlock();
			return 0;
		}

		
			clif_skill_nodamage(src, bl, skill_id, skill_lv, sc_start(src, bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv)));
		
		break;
	case SK_BS_MAXIMIZEPOWER:
	case SK_NV_TRICKDEAD:

	case SK_CR_AUTOGUARD:
	
		if( tsce )
		{
			clif_skill_nodamage(src,bl,skill_id,skill_lv,status_change_end(bl, type, INVALID_TIMER));
			map_freeblock_unlock();
			return 0;
		}

		
			clif_skill_nodamage(src, bl, skill_id, skill_lv, sc_start(src, bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv)));
	
		break;
	
	case SK_SM_AUTOBERSERK:
		clif_specialeffect(src, EF_MASS_SPIRAL, AREA);
		clif_soundeffectall(src, "anger.wav", 0, AREA);
		if( tsce )
			i = status_change_end(bl, type, INVALID_TIMER);
		else
			i = sc_start(src,bl,type,100,skill_lv,60000);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,i);
		break;
	case SK_TF_HIDING:
	case SK_AS_STEALTH:
		if (tsce)
		{
			clif_skill_nodamage(src,bl,skill_id,-1,status_change_end(bl, type, INVALID_TIMER)); //Hide skill-scream animation.
			map_freeblock_unlock();
			return 0;
		}
		clif_skill_nodamage(src,bl,skill_id,-1,sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		break;
	

	case SK_RG_PLAGIARISM:
		if (tsce) {
			i = status_change_end(bl, type, INVALID_TIMER);

			if( i ){
				clif_skill_nodamage(src,bl,skill_id,( skill_id == SK_HT_CAMOUFLAGE ) ? skill_lv : -1,i);
				status_change_end(bl, STATUS_REPRODUCE, INVALID_TIMER);
			}
			else if( sd )
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			map_freeblock_unlock();
			return 0;
		}
		i = sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv));
		clif_specialeffect(bl, EF_BLUEBODY, AREA);
		clif_specialeffect(bl, EF_GUARD2, AREA);
		if( i ){
			clif_skill_nodamage(src,bl,skill_id,(skill_id == SK_HT_CAMOUFLAGE ) ? skill_lv : -1,i);
			status_change_end(bl, STATUS_REPRODUCE, INVALID_TIMER);
		}
		else if( sd )
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
		break;
	case SK_ST_REPRODUCE:
		if (tsce) {
			i = status_change_end(bl, type, INVALID_TIMER);

			if( i ){
				clif_skill_nodamage(src,bl,skill_id,( skill_id == SK_HT_CAMOUFLAGE ) ? skill_lv : -1,i);
				status_change_end(bl, STATUS_PLAGIARISM, INVALID_TIMER);
			}
			else if( sd )
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			map_freeblock_unlock();
			return 0;
		}
		i = sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv));
		clif_specialeffect(bl, EF_DETOXICATION, AREA);
		if( i ){
			clif_skill_nodamage(src,bl,skill_id,(  skill_id == SK_HT_CAMOUFLAGE ) ? skill_lv : -1,i);
			status_change_end(bl, STATUS_PLAGIARISM, INVALID_TIMER);
		}
		else if( sd )
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
		break;
	case SK_AS_CLOAKING:
	case SK_HT_CAMOUFLAGE:
		if (tsce) {
			i = status_change_end(bl, type, INVALID_TIMER);
			if( i )
				clif_skill_nodamage(src,bl,skill_id,( skill_id == SK_HT_CAMOUFLAGE ) ? skill_lv : -1,i);
			else if( sd )
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			map_freeblock_unlock();
			return 0;
		}
		i = sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv));
		if( i )
			clif_skill_nodamage(src,bl,skill_id,( skill_id == SK_HT_CAMOUFLAGE ) ? skill_lv : -1,i);
		else if( sd )
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
		break;
	case SK_EX_INVISIBILITY:
		if (tsce) {
			i = status_change_end(bl, type, INVALID_TIMER);
			if( i )
				clif_skill_nodamage(src,bl,skill_id,(  skill_id == SK_HT_CAMOUFLAGE ) ? skill_lv : -1,i);
			else if( sd )
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			map_freeblock_unlock();
			return 0;
		}
		clif_specialeffect(bl, EF_POISONHIT, AREA);
		i = sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv));
		if( i )
			clif_skill_nodamage(src,bl,skill_id,(skill_id == SK_HT_CAMOUFLAGE ) ? skill_lv : -1,i);
		else if( sd )
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
		break;


	case SK_BA_MAGICSTRINGS:
	case SK_CL_BATTLEDRUMS:
	case SK_BA_ACOUSTICRYTHM:
	case SK_BA_IMPRESSIVERIFF:
		clif_specialeffect(bl, 139, AREA);
		skill_castend_song(src, skill_id, skill_lv, tick);
		break;
	case SK_BA_MOONLIGHTSONATA:
		clif_specialeffect(src, 1229, AREA);
		skill_castend_song(src, skill_id, skill_lv, tick);
		break;
	case SK_CL_PAGANPARTY:
		clif_specialeffect(src, 1407, AREA); //new_cannon_spear_12_clock
		skill_castend_song(src, skill_id, skill_lv, tick);
		break;
		skill_castend_song(src, skill_id, skill_lv, tick);
		break;


	case SK_TF_STEAL:
		if(sd) {
			if(pc_steal_item(sd,bl,skill_lv))
				clif_skill_nodamage(src,bl,skill_id,skill_lv *2,1);
			else
				clif_skill_fail(sd,skill_id,USESKILL_FAIL,0);
		}
		break;

	case SK_MG_STONECURSE:
		MageAdditionalEffectsCalculator::apply_stone_curse_additional_effect(src, bl, skill_lv);
		break;

	case SK_NV_FIRSTAID:
		clif_skill_nodamage(src,bl,skill_id,5,1);
		status_heal(bl,5,0,0);
		break;

	case SK_BS_REPAIRWEAPON:
		if(sd && dstsd)
			clif_item_repair_list(sd,dstsd,skill_lv);
		break;
	case BS_AXE:
		if(sd && dstsd)
			clif_skill_produce_mix_list(sd,-1, 1);
		break;


	// Weapon Refining [Celest]
	case SK_MS_WEAPONREFINE:
		if(sd)
			clif_item_refine_list(sd);
		break;

	case SK_MC_VENDING:
		if(sd)
		{	//Prevent vending of GMs with unnecessary Level to trade/drop. [Skotlex]
			if ( !pc_can_give_items(sd) )
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			else {
				sd->state.prevend = 1;
				sd->state.workinprogress = WIP_DISABLE_ALL;
				sd->vend_skill_lv = skill_lv*2;
				ARR_FIND(0, MAX_CART, i, sd->cart.u.items_cart[i].nameid && sd->cart.u.items_cart[i].id == 0);
				if (i < MAX_CART)
					intif_storage_save(sd, &sd->cart);
				else
					clif_openvendingreq(sd,2+skill_lv);
			}
		}
		break;


	case SK_ST_STRIPWEAPON:
	case SK_ST_STRIPSHIELD:
	case SK_ST_STRIPARMOR:
	case SK_ST_STRIPHELM:
	case SK_ST_FULLSTRIP:
	{
		bool i;

		//Special message when trying to use strip on FCP [Jobbie]
		if( sd && skill_id == SK_ST_FULLSTRIP && tsc && tsc->data[STATUS_CP_WEAPON] && tsc->data[STATUS_CP_HELM] && tsc->data[STATUS_CP_ARMOR] && tsc->data[STATUS_CP_SHIELD])
		{
			clif_gospel_info(sd, 0x28);
			break;
		}

		if( (i = skill_strip_equip(src, bl, skill_id, skill_lv)) || (skill_id != SK_ST_FULLSTRIP) )
			clif_skill_nodamage(src,bl,skill_id,skill_lv,i);

		//Nothing stripped.
		if( sd && !i )
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
		break;
	}
	case SK_RG_STRIPACCESSORY:
	{
		bool i;

		//Special message when trying to use strip on FCP [Jobbie]
		if( sd && skill_id == SK_ST_FULLSTRIP && tsc && tsc->data[STATUS_CP_WEAPON] && tsc->data[STATUS_CP_HELM] && tsc->data[STATUS_CP_ARMOR] && tsc->data[STATUS_CP_SHIELD])
		{
			clif_gospel_info(sd, 0x28);
			break;
		}

		if( (i = skill_strip_equip(src, bl, skill_id, skill_lv)) || (skill_id != SK_ST_FULLSTRIP ) ){
			clif_specialeffect(bl, EF_RG_COIN2, AREA);
			clif_specialeffect(bl, 1063, AREA);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,i);
		}
		//Nothing stripped.
		if( sd && !i )
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
		break;
	}
	case SK_AL_JGHEAL:
		{
			int healing, matk = 0;
			struct status_data *status;
			struct status_change *sc;
			status = status_get_status_data(&sd->bl);
			matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
			healing = (200 * skill_lv) + (status_get_lv(src) * 3) + (status_get_int(src) * 3) + (matk * 3);
			sc = status_get_sc(src);
			if (sc && sc->data[STATUS_OFFERTORIUM])
				healing = healing*sc->data[STATUS_OFFERTORIUM]->val2;
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing,1);
			status_heal(bl,healing,0,0);
		}
		break;
	case SK_CR_SUNLIGHT:
		{
			int healing, buff_skill_lv, matk = 0;
			struct elemental_data *ed = (TBL_ELEM*) map_id2bl(src->id);
			struct block_list *mbl;
			mbl = battle_get_master(&ed->bl);
			struct map_session_data *msd = BL_CAST(BL_PC, bl);

			buff_skill_lv = pc_checkskill(msd, SK_CR_HOMUNCULUSRESEARCH);
			matk = rand()%(ed->elemental.matk-ed->elemental.matk_min + 1) + ed->elemental.matk_min;
			healing = (250 * skill_lv) + (status_get_lv(src) * 3) + (status_get_int(src) * 3) + (matk * 3) + (buff_skill_lv*200);
			clif_skill_nodamage(src,bl,SK_CR_SUNLIGHT,healing,1);
			clif_specialeffect(bl, 1383, AREA);//new_dragonbreath_11_clock
			clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing,1);
			clif_soundeffectall(bl, "sunlight.wav", 0, AREA);
			status_heal(bl,healing,0,0);
		}
		break;	
	case SK_AM_WARMWIND:
		{
			int healing, buff_skill_lv, matk = 0;
			struct elemental_data *ed = (TBL_ELEM*) map_id2bl(src->id);
			struct block_list *mbl;
			mbl = battle_get_master(&ed->bl);
			struct map_session_data *msd = BL_CAST(BL_PC, bl);

			buff_skill_lv = pc_checkskill(msd, SK_CR_HOMUNCULUSRESEARCH);
			matk = rand()%(ed->elemental.matk-ed->elemental.matk_min + 1) + ed->elemental.matk_min;
			healing = (200 * skill_lv) + (status_get_lv(src) * 3) + (status_get_int(src) * 3) + (matk * 3) + (buff_skill_lv*200);
			clif_skill_nodamage(src,bl,SK_AM_WARMWIND,healing,1);
			clif_specialeffect(bl, EF_STORMKICK6, AREA);
			status_heal(bl,healing,0,0);
		}
		break;
	case SK_CR_SWORDSTOPLOWSHARES:
		{
			int healing, matk = 0;
			struct status_data *status;
			status = status_get_status_data(&sd->bl);
			matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
			healing = (200 * skill_lv) + (status_get_lv(src) * 3) + (status_get_int(src) * 3) + (matk * 3);
			clif_specialeffect(bl, EF_LAMADAN, AREA);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing,1);
			status_heal(bl,healing,0,0);
		}
		break;
	case SK_CR_BINDINGOATH:
		{
			int healing_self, healing_target, matk = 0;
			struct status_data *status;
			status = status_get_status_data(&sd->bl);
			matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
			healing_self = (100 * skill_lv) + (status_get_lv(src) * 3) + (status_get_int(src) * 3) + (matk * 3);
			healing_target = (100 * skill_lv) + (status_get_lv(src) * 3) + (status_get_int(src) * 3) + (matk * 3);
			if(bl == src) {
				clif_specialeffect(bl, EF_FOOD06, AREA);
				clif_skill_nodamage(src, bl, skill_id, healing_self, 1);
				clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing_target,1);
				status_heal(bl,healing_target,0,0);
			} else {
				clif_specialeffect(bl, EF_FOOD06, AREA);
				clif_specialeffect(src, EF_FOOD06, AREA);
				clif_skill_nodamage(src, bl, skill_id, healing_self, 1);
				clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing_target,1);
				clif_skill_nodamage(NULL,src,SK_AL_HEAL,healing_self,1);
				status_heal(src,healing_self,0,0);
				status_heal(bl,healing_target,0,0);
			}
		}
		break;
	case SK_PR_SUBLIMITASHEAL:
		{
			int healing, matk = 0;
			struct status_data *status;
			struct status_change *sc;
			status = status_get_status_data(&sd->bl);
			matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
			healing = (400 * skill_lv) + (status_get_lv(src) * 4) + (status_get_int(src) * 4) + (matk * 4);
			sc = status_get_sc(src);
			if (sc && sc->data[STATUS_OFFERTORIUM])
				healing = healing*sc->data[STATUS_OFFERTORIUM]->val2;
			clif_specialeffect(bl, EF_MOCHI, AREA);
			clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing,1);
			status_heal(bl,healing,0,0);
		}
		break;
	case SK_PA_KINGSGRACE:
		if( !sd || sd->status.party_id == 0 || flag&1 ) {
			if( sd && tstatus ) {
				int healing, matk = 0;
				struct status_data *status;
				status = status_get_status_data(&sd->bl);
				matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
				healing = (400 * skill_lv) + (status_get_lv(src) * 4) + (status_get_int(src) * 4) + (matk * 4);
				clif_specialeffect(bl, EF_RECOVERY, AREA);
				clif_skill_nodamage(src, bl, skill_id, skill_lv,1);
				clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing,1);
				status_heal(bl, healing, 0, 0);
				status_change_end(bl, STATUS_FREEZE, INVALID_TIMER);
				status_change_end(bl, STATUS_STONECURSE, INVALID_TIMER);
				status_change_end(bl, STATUS_SLEEP, INVALID_TIMER);
				status_change_end(bl, STATUS_PARALYSIS, INVALID_TIMER);
				status_change_end(bl, STATUS_STUN, INVALID_TIMER);
				status_change_end(bl, STATUS_MAGICCELL, INVALID_TIMER);
				status_change_end(bl, STATUS_BLEEDING, INVALID_TIMER);
				status_change_end(bl, STATUS_BLIND, INVALID_TIMER);
				status_change_end(bl, STATUS_CURSE, INVALID_TIMER);
			}
		} else if( sd ){
			party_foreachsamemap(skill_area_sub, sd, skill_get_splash(skill_id, skill_lv), src, skill_id, skill_lv, tick, flag|BCT_PARTY|1, skill_castend_nodamage_id);
		}
		break;
	case SK_PR_COLUSEOHEAL:
		if( !sd || sd->status.party_id == 0 || flag&1 ) {
			if( sd && tstatus ) {
				int healing, matk = 0;
				struct status_data *status;
				struct status_change *sc;
				status = status_get_status_data(&sd->bl);
				matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
				healing = (75 * skill_lv) + (status_get_lv(src) * 3) + (status_get_int(src) * 3) + (matk * 3);
				sc = status_get_sc(src);
				if (sc && sc->data[STATUS_OFFERTORIUM])
					healing = healing*sc->data[STATUS_OFFERTORIUM]->val2;
				clif_skill_nodamage(src, bl, skill_id, healing, 1);
				status_heal(bl, healing, 0, 0);
			}
		} else if( sd ){
			party_foreachsamemap(skill_area_sub, sd, skill_get_splash(skill_id, skill_lv), src, skill_id, skill_lv, tick, flag|BCT_PARTY|1, skill_castend_nodamage_id);
		}
		break;
	case SK_CR_DIVINELIGHT:
		{
			int healing, matk = 0;
			struct status_data *status;
			status = status_get_status_data(&sd->bl);
			matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
			healing = (400 * skill_lv) + (status_get_lv(src) * 4) + (status_get_int(src) * 4) + (matk * 4);
			clif_specialeffect(bl, EF_FOOD06, AREA);
			clif_specialeffect(bl, EF_GLORIA, AREA);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing,1);
			status_heal(bl,healing,0,0);
		}
		break;
	case SK_CL_ALLSTAR:
		if( !sd || sd->status.party_id == 0 || flag&1 ) {
			if( sd && tstatus ) {
				int healing, matk = 0;
				struct status_data *status;
				status = status_get_status_data(&sd->bl);
				matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
				healing = (400 * skill_lv) + (status_get_lv(src) * 3) + (status_get_int(src) * 3) + (matk * 3);
				clif_specialeffect(bl, EF_FOOD04, AREA);
				clif_specialeffect(bl, 699, AREA);
				clif_soundeffectall(&sd->bl, "all_star.wav", 0, AREA);
				clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
				clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing,1);
				status_heal(bl,healing,0,0);
			}
		} else if( sd ){
			party_foreachsamemap(skill_area_sub, sd, skill_get_splash(skill_id, skill_lv), src, skill_id, skill_lv, tick, flag|BCT_PARTY|1, skill_castend_nodamage_id);
		}
		break;
	case SK_BA_TWILIGHTCHORD:
		{
			int healing, matk = 0;
			struct status_data *status;
			status = status_get_status_data(&sd->bl);
			matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
			healing = (200 * skill_lv) + (status_get_lv(src) * 3) + (status_get_int(src) * 3) + (matk * 3);
			clif_specialeffect(bl, EF_FOOD04, AREA);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing,1);
			status_heal(bl,healing,0,0);
		}
		break;
	case SK_BA_ALLNIGHTLONG:
		if( !status_isdead(bl) ) {
			int healing, matk = 0;
			struct status_data *status;
			status = status_get_status_data(&sd->bl);
			matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
			healing = (400 * skill_lv) + (status_get_lv(src) * 3) + (status_get_int(src) * 3) + (matk * 3);
			clif_specialeffect(bl, EF_ITEM315, AREA);
			clif_specialeffect(bl, EF_FOOD04, AREA);
			clif_soundeffectall(&sd->bl, "allnightlong.wav", 0, AREA);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing,1);
			status_heal(bl,healing,0,0);
		} else {
			int heal = tstatus->sp;
			if( heal <= 0 )
				heal = 1;
			tstatus->hp = heal;
			tstatus->sp -= tstatus->sp * ( 60 - 10 * skill_lv ) / 100;
			clif_specialeffect(bl, EF_ITEM315, AREA);
			clif_specialeffect(bl, EF_FOOD04, AREA);
			clif_soundeffectall(&sd->bl, "allnightlong.wav", 0, AREA);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			pc_revive((TBL_PC*)bl,heal,0);
			clif_resurrection(bl,1);
		}
		// }
		break;
	case SK_AM_AIDCONDENSEDPOTION:
	case SK_CR_POTIONPITCHER:
	case SK_AM_AIDPOTION: 
		{
			int healing_hp = 0, healing_sp = 0, matk = 0;
			struct status_data *status;
			int x, j;
			struct s_skill_condition require = skill_get_requirement(sd, skill_id, skill_lv);


			x = skill_lv%11 - 1;
			status = status_get_status_data(&sd->bl);
			matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
			j = pc_search_inventory(sd, require.itemid[x]);
			if (j < 0 || require.itemid[x] <= 0) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				map_freeblock_unlock();
				return 1;
			}
				if (sd->inventory_data[j] == NULL || sd->inventory.u.items_inventory[j].amount < require.amount[x]) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				map_freeblock_unlock();
				return 1;
			}

			run_script(sd->inventory_data[j]->script,0,sd->bl.id,0);
			if (skill_lv == 1) {
				healing_hp = 500;
			}
			if (skill_lv == 2) {
				healing_hp = 1000;
			}
			if (skill_lv == 3) {
				healing_hp = 1500;
			}
			if (skill_lv == 4) {
				healing_hp = 2000;
			}
			if (skill_lv == 5) {
				healing_sp = 200;
				healing_sp += (status_get_lv(src) / 2) + (status_get_int(src) / 2) + (matk / 2);
			}
			if (skill_lv != 5) {
				healing_hp += (status_get_lv(src) * 2) + (status_get_int(src) * 2) + (matk * 2);
			}
			if (skill_id == SK_AM_AIDCONDENSEDPOTION) {
				healing_hp += 2000;
				healing_sp += 200;
			}
			if (skill_lv != 5) {
				clif_skill_nodamage(NULL,bl,SK_AL_HEAL,healing_hp,1);	
			} else {
				clif_skill_nodamage(NULL,bl,SK_SM_SRECOVERY,healing_sp,1);
				clif_specialeffect(bl, EF_HEALSP, AREA);
			}
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			status_heal(bl,healing_hp,healing_sp,0);
		}
		break;
	case SK_CR_CP_WEAPON:
	case SK_CR_CP_SHIELD:
	case SK_CR_CP_ARMOR:
	case SK_CR_CP_HELM:
		{
			unsigned int equip[] = {EQP_WEAPON, EQP_SHIELD, EQP_ARMOR, EQP_HEAD_TOP};

			if( sd && ( bl->type != BL_PC || ( dstsd && pc_checkequip(dstsd,equip[skill_id - SK_CR_CP_WEAPON]) < 0 ) ) ){
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				map_freeblock_unlock(); // Don't consume item requirements
				return 0;
			}
			clif_skill_nodamage(src,bl,skill_id,skill_lv,
				sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		}
		break;
	
	case SK_SA_DISPELL:
		if (flag&1 || (i = skill_get_splash(skill_id, skill_lv)) < 1) {
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			if(dstsd && rnd()%100 >= 50+10*skill_lv)
			{
				if (sd)
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			if(status_isimmune(bl))
				break;
			
			clif_specialeffect(bl, EF_CONE, AREA);
			clif_specialeffect(bl, EF_DETOXICATION, AREA);
			//Remove bonus_script by Dispell
			if (dstsd)
				pc_bonus_script_clear(dstsd,BSF_REM_ON_DISPELL);

			{
				int sp;
				struct unit_data *ud = unit_bl2ud(bl);
				int bl_skill_id=0,bl_skill_lv=0,hp = 0;
				if (!ud || ud->skilltimer == INVALID_TIMER)
					break; //Nothing to cancel.
				bl_skill_id = ud->skill_id;
				bl_skill_lv = ud->skill_lv;
				hp = (tstatus->max_hp/50) * 2 * skill_lv; 
				clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
				unit_skillcastcancel(bl,0);
				sp = skill_get_sp(bl_skill_id, bl_skill_lv);
				status_zap(bl, hp, sp);

				if (hp && skill_lv >= 5)
					hp>>=1;	//Recover half damaged HP at level 5 [Skotlex]
				else
					hp = 0;

				if (sp) //Recover some of the SP used
					sp = sp*(25*(skill_lv-1))/100;

				if(hp || sp)
					status_heal(src, hp, sp, 2);
				
			}

			if(!tsc || !tsc->count)
				break;

			for(i=0;i<SC_MAX;i++) {
				if (!tsc->data[i])
					continue;
				switch (i) {
					case STATUS_WEIGHT50:		case STATUS_WEIGHT90:		
					case STATUS_STRIPWEAPON:	case STATUS_STRIPSHIELD:	case STATUS_STRIPARMOR:
					case STATUS_STRIPHELM:		case STATUS_CP_WEAPON:		case STATUS_CP_SHIELD:
					case STATUS_CP_ARMOR:		case STATUS_CP_HELM:
					case STATUS_STRFOOD:		case STATUS_AGIFOOD:		case STATUS_VITFOOD:
					case STATUS_INTFOOD:		case STATUS_DEXFOOD:		case STATUS_LUKFOOD:
					case STATUS_ENCHANTDEADLYPOISON:			case STATUS_ANGER:
					case STATUS_CARTBOOST:		case STATUS_MELTDOWN:		
					case STATUS_NOCHAT:
					case STATUS_AUTOSPELL:
					case STATUS_INCATKRATE:	
					case STATUS_BATTLEDRUMS:	
					case STATUS_MOONLIGHTSONATA:		case STATUS_PAGANPARTY:	
					case STATUS_STRIPACCESSORY:
					case STATUS_NEUTRALBARRIER_MASTER:		case STATUS_NEUTRALBARRIER:		
					case STATUS_STONESKIN:	case STATUS_VIGOR:
					case STATUS_STALK:
					case STATUS_HINDSIGHT:
					case STATUS_PUSHCART:
					case STATUS_DUPLELUX:		case STATUS_EXPIATIO:
					case STATUS_VOIDMAGEPRODIGY:	case STATUS_GEOGRAFIELD:
					case STATUS_ALL_RIDING:		
					case STATUS_CHASEWALK2:
					case STATUS_HIDING:			case STATUS_CLOAKING:		case STATUS_CHASEWALK:
					case STATUS_INVISIBILITY:	case STATUS_ALL_RIDING_REUSE_LIMIT:
					case STATUS_FRENZY:
					// Clans
					case STATUS_CLAN_INFO:
					case STATUS_SWORDCLAN:
					case STATUS_ARCWANDCLAN:
					case STATUS_GOLDENMACECLAN:
					case STATUS_CROSSBOWCLAN:
					case STATUS_JUMPINGCLAN:
						continue;
					case STATUS_IMPRESSIVERIFF:
					case STATUS_MAGICSTRINGS:
					case STATUS_ACCOUSTICRYTHM:
						if (!battle_config.dispel_song || tsc->data[i]->val4 == 0)
							continue; //If in song area don't end it, even if config enabled
						break;
					case STATUS_ARMORREINFORCEMENT:
					case STATUS_ASSUMPTIO:
						if( bl->type == BL_MOB )
							continue;
						break;
				}
				status_change_end(bl, (sc_type)i, INVALID_TIMER);
			}
			break;
		}

		//Affect all targets on splash area.
		map_foreachinallrange(skill_area_sub, bl, i, BL_CHAR,
			src, skill_id, skill_lv, tick, flag|1,
			skill_castend_damage_id);
		break;

	case SK_TF_BACKSLIDING: //This is the correct implementation as per packet logging information. [Skotlex]
		skill_blown(src,bl,skill_get_blewcount(skill_id,skill_lv),unit_getdir(bl),(enum e_skill_blown)(BLOWN_IGNORE_NO_KNOCKBACK|BLOWN_DONT_SEND_PACKET));
		clif_skill_nodamage(src, bl, skill_id, skill_lv, 1);
		clif_blown(src); // Always blow, otherwise it shows a casting animation. [Lemongrass]
		break;

	case SK_MG_CASTCANCEL:
	case SK_PF_SPELLFIST:
		clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		switch (sd->skill_id_old){
			case SK_MG_FIREBOLT:
				clif_specialeffect(src, 1270, AREA);
				break;
			case SK_MG_COLDBOLT:
				clif_specialeffect(src, 1271, AREA);
				break;
			case SK_MG_LIGHTNINGBOLT:
				clif_specialeffect(src, 1272, AREA);
				break;
			case SK_MG_EARTHBOLT:
				clif_specialeffect(src, 1273, AREA);
				break;
			case SK_MG_DARKSTRIKE:
				clif_specialeffect(src, 995, AREA);
				break;
			case SK_MG_SOULSTRIKE:
				clif_specialeffect(src, 992, AREA);
				break;				
		}
		unit_skillcastcancel(src,1);
		if(sd) {
			int sp = skill_get_sp(sd->skill_id_old,sd->skill_lv_old);
			if( skill_id == SK_PF_SPELLFIST ){
				sc_start4(src,src,type,100,skill_lv * 9999,skill_lv,sd->skill_id_old,sd->skill_lv_old,skill_get_time(skill_id,skill_lv));
				sd->skill_id_old = sd->skill_lv_old = 0;
				break;
			}
			sp = sp * (90 - (skill_lv-1)*20) / 100;
			if(sp < 0) sp = 0;
			status_zap(src, 0, sp);
		}
		break;

	case SK_SA_AUTOSPELL:
		clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		if (sd) {
			sd->state.workinprogress = WIP_DISABLE_ALL;
			clif_autospell(sd,skill_lv);
		} else {
			int maxlv=1,spellid=0;
			static const int spellarray[3] = { SK_MG_COLDBOLT,SK_MG_FIREBOLT,SK_MG_LIGHTNINGBOLT };

			if(skill_lv >= 10) {
				spellid = SK_MG_FROSTDIVER;
//				if (tsc && tsc->data[SC_SPIRIT] && tsc->data[SC_SPIRIT]->val2 == SA_SAGE)
//					maxlv = 10;
//				else
					maxlv = skill_lv - 9;
			}
			else if(skill_lv >=5) {
				spellid = SK_MG_SOULSTRIKE;
				maxlv = skill_lv - 4;
			}
			else if(skill_lv >=2) {
				int i_rnd = rnd()%3;
				spellid = spellarray[i_rnd];
				maxlv = skill_lv - 1;
			}
			/*else if(skill_lv > 0) {
				spellid = SK_AL_SACREDWAVE;
				maxlv = 3;
			}*/

			if(spellid > 0)
				sc_start4(src,src,STATUS_AUTOSPELL,100,skill_lv,spellid,maxlv,0,
					skill_get_time(SK_SA_AUTOSPELL,skill_lv));
		}
		break;

	case SK_MC_GREED:
		if(sd){
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			map_foreachinallrange(skill_greed,bl,
				skill_get_splash(skill_id, skill_lv),BL_ITEM,bl);
		}
		break;


	case SK_PF_INDULGE:
		{
			int hp, sp;
			hp = sstatus->max_hp/10;
			sp = hp * 10 * skill_lv / 100;
			if (!status_charge(src,hp,0)) {
				if (sd) clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			clif_skill_nodamage(src, bl, skill_id, skill_lv, 1);
			status_heal(bl,0,sp,2);
		}
		break;


	case SK_AS_VENOMSPLASHER:
		if( status_has_mode(tstatus,MD_STATUSIMMUNE)
		// Renewal dropped the 3/4 hp requirement

				) {
			if (sd) clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			map_freeblock_unlock();
			return 1;
		}
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start4(src,bl,type,100,skill_lv,skill_id,src->id,skill_get_time(skill_id,skill_lv),1000));

	break;

	case SK_SA_MINDGAMES:
		{
			clif_specialeffect(bl, EF_MAPAE, AREA);
			if (!clif_skill_nodamage(src,bl,skill_id,skill_lv,
				sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv))))
			{	
				if (sd) clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				map_freeblock_unlock();
				return 0;
			}
		}
		break;

	case SK_PF_SOULEXHALE:
		{
			unsigned int sp1 = 0, sp2 = 0;
			if (dstmd) {
				status_heal(src, 0, 25*skill_lv, 2);
				clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
				break;
			}
			sp1 = sstatus->sp;
			sp2 = tstatus->sp;
			if (tsc && tsc->data[STATUS_NORECOVER_STATE])
				sp1 = tstatus->sp;
			status_set_sp(src, sp2, 3);
			status_set_sp(bl, sp1, 3);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		}
		break;
	// Full Chemical Protection
	case SK_CR_FULLCHEMICALPROTECTION:
		{
			unsigned int equip[] = {EQP_WEAPON, EQP_SHIELD, EQP_ARMOR, EQP_HEAD_TOP};
			int i_eqp, s = 0, skilltime = skill_get_time(skill_id,skill_lv);

			for (i_eqp = 0; i_eqp < 4; i_eqp++) {
				if( bl->type != BL_PC || ( dstsd && pc_checkequip(dstsd,equip[i_eqp]) < 0 ) )
					continue;
				sc_start(src,bl,(sc_type)(STATUS_CP_WEAPON + i_eqp),100,skill_lv,skilltime);
				s++;
			}
			if( sd && !s ){
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				map_freeblock_unlock(); // Don't consume item requirements
				return 0;
			}
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		}
		break;


	// New guild skills [Celest]
	case GD_BATTLEORDER:
	case GD_REGENERATION:
	case GD_RESTORE:
	case GD_EMERGENCY_MOVE:
		if(flag&1) {
			if (status_get_guild_id(src) == status_get_guild_id(bl)) {				
				if( skill_id == GD_RESTORE )
					clif_skill_nodamage(src,bl,SK_AL_HEAL,status_percent_heal(bl,90,90),1);
				else
					sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id, skill_lv));
			}
		} else if (status_get_guild_id(src)) {
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			map_foreachinallrange(skill_area_sub, src,
				skill_get_splash(skill_id, skill_lv), BL_PC,
				src,skill_id,skill_lv,tick, flag|BCT_GUILD|1,
				skill_castend_nodamage_id);
			if (sd)
#ifdef RENEWAL
				skill_blockpc_start(sd, skill_id, skill_get_cooldown(skill_id, skill_lv));
#else
				guild_block_skill(sd, skill_get_time2(skill_id, skill_lv));
#endif
		}
		break;
	case GD_EMERGENCYCALL:
	case GD_ITEMEMERGENCYCALL:
		{
			int8 dx[9] = {-1, 1, 0, 0,-1, 1,-1, 1, 0};
			int8 dy[9] = { 0, 0, 1,-1, 1,-1,-1, 1, 0};
			uint8 j = 0, calls = 0, called = 0;
			struct guild *g;
			// i don't know if it actually summons in a circle, but oh well. ;P
			g = sd?sd->guild:guild_search(status_get_guild_id(src));
			if (!g)
				break;

			if (skill_id == GD_ITEMEMERGENCYCALL)
				switch (skill_lv) {
					case 1:	calls = 7; break;
					case 2:	calls = 12; break;
					case 3:	calls = 20; break;
					default: calls = 0;	break;
				}

			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			for (i = 0; i < g->max_member && (!calls || (calls && called < calls)); i++, j++) {
				if (j > 8)
					j = 0;
				if ((dstsd = g->member[i].sd) != NULL && sd != dstsd && !dstsd->state.autotrade && !pc_isdead(dstsd)) {
					if (map_getmapflag(dstsd->bl.m, MF_NOWARP) && !map_flag_gvg2(dstsd->bl.m))
						continue;
					if (!pc_job_can_entermap((enum e_job)dstsd->status.class_, src->m, dstsd->group_level))
						continue;
					if(map_getcell(src->m,src->x+dx[j],src->y+dy[j],CELL_CHKNOREACH))
						dx[j] = dy[j] = 0;
					if (!pc_setpos(dstsd, map_id2index(src->m), src->x+dx[j], src->y+dy[j], CLR_RESPAWN))
						called++;
				}
			}
			if (sd)
#ifdef RENEWAL
				skill_blockpc_start(sd, skill_id, skill_get_cooldown(skill_id, skill_lv));
#else
				guild_block_skill(sd, skill_get_time2(skill_id, skill_lv));
#endif
		}
		break;
	case GD_CHARGESHOUT_FLAG:
		if (sd && sd->guild && sd->state.gmaster_flag == 1) {
			mob_data *md = mob_once_spawn_sub(src, src->m, src->x, src->y, sd->guild->name, MOBID_GUILD_SKILL_FLAG, nullptr, SZ_SMALL, AI_GUILD);

			if (md) {
				sd->guild->chargeshout_flag_id = md->bl.id;
				md->master_id = src->id;

				if (md->deletetimer != INVALID_TIMER)
					delete_timer(md->deletetimer, mob_timer_delete);
				md->deletetimer = add_timer(gettick() + skill_get_time(GD_CHARGESHOUT_FLAG, skill_lv), mob_timer_delete, md->bl.id, 0);
				mob_spawn(md);
			}
		}
		break;
	case GD_CHARGESHOUT_BEATING:
		if (sd && sd->guild && map_blid_exists(sd->guild->chargeshout_flag_id)) {
			block_list *mob_bl = map_id2bl(sd->guild->chargeshout_flag_id);

			if (pc_setpos(sd, map_id2index(mob_bl->m), mob_bl->x, mob_bl->y, CLR_RESPAWN) != SETPOS_OK)
				clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
			else
				clif_skill_nodamage(src, bl, skill_id, skill_lv, 1);
		} else
			clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
		break;

	case SK_KN_ENCHANTBLADE:
		clif_skill_nodamage(src,bl,skill_id,skill_lv,
			sc_start2(src,bl,type,100,skill_lv,((100+20*skill_lv)*status_get_lv(src))/100+sstatus->int_,skill_get_time(skill_id,skill_lv)));
		break;
	
	case SK_KN_VIGOR:
		if (sc_start(src, bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv))){
			clif_specialeffect(src, EF_MAXPAIN, AREA);
			clif_skill_nodamage(src, bl, skill_id, skill_lv, 1);
		}
		break;
	case SK_KN_STONESKIN:
		if (sc_start(src, bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv))){
			clif_skill_nodamage(src, bl, skill_id, skill_lv, 1);
		}
		break;
	

	case SK_AS_ROLLINGCUTTER:
		{
			short count = 1;
			skill_area_temp[2] = 0;
			map_foreachinrange(skill_area_sub,src,skill_get_splash(skill_id,skill_lv),BL_CHAR,src,skill_id,skill_lv,tick,flag|BCT_ENEMY|SD_PREAMBLE|SD_SPLASH|1,skill_castend_damage_id);
			if( tsc && tsc->data[STATUS_ROLLINGCUTTER] )
			{ // Every time the skill is casted the status change is reseted adding a counter.
				count += (short)tsc->data[STATUS_ROLLINGCUTTER]->val1;
				if( count > 10 )
					count = 10; // Max coounter
				status_change_end(bl, STATUS_ROLLINGCUTTER, INVALID_TIMER);
			}
			sc_start(src,bl,STATUS_ROLLINGCUTTER,100,count,skill_get_time(skill_id,skill_lv));
			clif_skill_nodamage(src,src,skill_id,skill_lv,1);
		}
		break;


	case SK_EX_HALLUCINATIONWALK:
		{
			int heal = status_get_max_hp(bl) / 10;
			if( status_get_hp(bl) < heal ) { // if you haven't enough HP skill fails.
				if( sd ) clif_skill_fail(sd,skill_id,USESKILL_FAIL_HP_INSUFFICIENT,0);
				break;
			}
			if( !status_charge(bl,heal,0) )
			{
				if( sd ) clif_skill_fail(sd,skill_id,USESKILL_FAIL_HP_INSUFFICIENT,0);
				break;
			}
			clif_skill_nodamage(src,bl,skill_id,skill_lv,sc_start(src,bl,type,100,skill_lv,skill_get_time(skill_id,skill_lv)));
		}
		break;

	case SK_AL_RENEW:
		if( !sd || sd->status.party_id == 0 || flag&1 ) {
			
				clif_specialeffect(bl, EF_ALL_RAY_OF_PROTECTION, AREA);
				clif_skill_nodamage(bl, bl, skill_id, skill_lv, sc_start(src, bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv)));
			
		} else if( sd )
			party_foreachsamemap(skill_area_sub, sd, skill_get_splash(skill_id, skill_lv), src, skill_id, skill_lv, tick, flag|BCT_PARTY|1, skill_castend_nodamage_id);
		break;
	
	case SK_SA_WHITEIMPRISON:
		if( (src == bl || battle_check_target(src, bl, BCT_ENEMY)>0) && status_get_class_(bl) != CLASS_BOSS && !status_isimmune(bl) ) // Should not work with Bosses.
		{

			if( sd )
				skill_blockpc_start(sd,skill_id,4000);

			if( !(tsc && tsc->data[type]) ){
				i = sc_start2(src,bl,type,100,skill_lv,src->id,skill_get_time(skill_id,skill_lv));
				clif_skill_nodamage(src,bl,skill_id,skill_lv,i);
				if( !i )
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			}
		}else
		if( sd )
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_TOTARGET,0);
		break;

	case SK_SO_SUMMONELEMENTALSPHERES:
		if (sd) clif_magicdecoy_list(sd, skill_lv, 0, 0, skill_id);
		break;
	case SK_MG_READING_SB_READING:
		if (sd) {
			switch(skill_lv){
				case 1:
					skill_spellbook(sd, 6189);
					break;
				case 2:
					skill_spellbook(sd, 6190);
					break;
				case 3:
					skill_spellbook(sd, 6191);
					break;
				case 4:
					skill_spellbook(sd, 6201);
					break;
				case 5:
					skill_spellbook(sd, 6205);
					break;
				case 6:
					skill_spellbook(sd, 62051);
					break;
				case 7:
					skill_spellbook(sd, 62052);
					break;
				case 8:
					skill_spellbook(sd, 62053);
					break;
				case 9:
					skill_spellbook(sd, 62054);
					break;
				case 10:
					skill_spellbook(sd, 62055);
					break;
				case 11:
					skill_spellbook(sd, 62056);
					break;
				case 12:
					skill_spellbook(sd, 62057);
					break;
				case 13:
					skill_spellbook(sd, 62058);
					break;
				case 14:
					skill_spellbook(sd, 62059);
					break;
				case 15:
					skill_spellbook(sd, 62060);
					break;
				case 16:
					skill_spellbook(sd, 62061);
					break;
				case 17:
					skill_spellbook(sd, 62062);
					break;
				case 18:
					skill_spellbook(sd, 62063);
					break;
				case 19:
					skill_spellbook(sd, 62064);
					break;
				case 20:
					skill_spellbook(sd, 62065);
					break;
				case 21:
					skill_spellbook(sd, 62066);
					break;
				case 22:
					skill_spellbook(sd, 62067);
					break;
				case 23:
					skill_spellbook(sd, 62068);
					break;
				case 24:
					skill_spellbook(sd, 62069);
					break;
				case 25:
					skill_spellbook(sd, 62070);
					break;
				case 26:
					skill_spellbook(sd, 62071);
					break;
				case 27:
					skill_spellbook(sd, 62072);
					break;
				case 28:
					skill_spellbook(sd, 62073);
					break;
			}
			
		}
		break;

	case SK_RA_TYHPOONFLOW:
		clif_soundeffectall(&sd->bl, "typhoon_flow.wav", 0, AREA);
		clif_specialeffect(src, 1404, AREA);//new_cannon_spear_06_clock
		clif_skill_damage(src, src, tick, status_get_amotion(src), 0, -30000, 1, skill_id, skill_lv, DMG_SINGLE);
		clif_skill_nodamage(src, bl, skill_id, skill_lv, sc_start(src,bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv)));
		break;

	case SK_ST_AUTOSHADOWSPELL:
		if( sd ) {
			if( (sd->reproduceskill_idx > 0 && sd->status.skill[sd->reproduceskill_idx].id) ||
				(sd->cloneskill_idx > 0 && sd->status.skill[sd->cloneskill_idx].id) )
			{
				clif_autoshadowspell_list(sd);
				clif_skill_nodamage(src,bl,skill_id,1,1);
			}
			else
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_IMITATION_SKILL_NONE,0);
		}
		break;

	case SK_ST_STALK:
		if( sd && dstsd && src != bl && !dstsd->shadowform_id ) {
			int hits = skill_lv;
			if( clif_skill_nodamage(src,bl,skill_id,skill_lv,sc_start4(src,src,type,100,skill_lv,bl->id,hits,0,skill_get_time(skill_id, skill_lv))) )
				dstsd->shadowform_id = src->id;
			break;
		}
		else {
			if( dstmd && src != bl && !dstmd->shadowform_id ) {
				int hits = skill_lv;
				if( clif_skill_nodamage(src,bl,skill_id,skill_lv,sc_start4(src,src,type,100,skill_lv,bl->id,hits,0,skill_get_time(skill_id, skill_lv))) )
					dstmd->shadowform_id = src->id;
			}
			break;
		}
		clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
		break;



	case SK_BA_WINDMILLPOEM:
		if( !sd || !sd->status.party_id || (flag & 1) ) {
			clif_specialeffect(src, 1223, AREA);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			sc_start2(src,bl,type,100,skill_lv,(0),skill_get_time(skill_id,skill_lv));	
		} else if( sd ) {
			clif_specialeffect(bl, 1223, AREA);
			party_foreachsamemap(skill_area_sub, sd, skill_get_splash(skill_id, skill_lv), src, skill_id, skill_lv, tick, flag|BCT_PARTY|1, skill_castend_nodamage_id);
			sc_start2(src,bl,type,100,skill_lv,(0),skill_get_time(skill_id,skill_lv));
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		}
		break;
	case SK_BA_ECHONOCTURNAE:
		if( !sd || !sd->status.party_id || (flag & 1) ) {
			clif_specialeffect(src, 1219, AREA);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			sc_start2(src,bl,type,100,skill_lv,(0),skill_get_time(skill_id,skill_lv));
		} else if( sd ) {
			clif_specialeffect(bl, 1219, AREA);
			party_foreachsamemap(skill_area_sub, sd, skill_get_splash(skill_id, skill_lv), src, skill_id, skill_lv, tick, flag|BCT_PARTY|1, skill_castend_nodamage_id);
			sc_start2(src,bl,type,100,skill_lv,(0),skill_get_time(skill_id,skill_lv));
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		}
		break;

	case SK_PF_STASIS:
		if (flag&1) {
			int duration = 2000 * skill_lv;
			clif_specialeffect(bl, 1230, AREA);
			sc_start(src, bl, type, 100, skill_lv, duration);
			// status_change_start(src, bl, STATUS_MAGICCELL, 10000, skill_lv, 0, 0, 0, duration, SCSTART_NONE);
		} else {
			clif_skill_nodamage(src, bl, skill_id, skill_lv, 1);
			map_foreachinallrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), BL_CHAR, src, skill_id, skill_lv, tick, flag|BCT_ENEMY|1, skill_castend_nodamage_id);
		}
		break;
	case SK_CL_DEEPSLEEPLULLABY:
		if (flag&1) {
			int duration = 2000 * skill_lv;
			clif_specialeffect(bl, EF_SLEEPATTACK, AREA);
			sc_start(src, bl, type, 100, skill_lv, 10000);
		} else {
			clif_skill_nodamage(src, bl, skill_id, skill_lv, 1);
			map_foreachinallrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), BL_CHAR, src, skill_id, skill_lv, tick, flag|BCT_ENEMY|1, skill_castend_nodamage_id);
		}
		break;
	case SK_SA_ELEMENTALCONTROL:
	case SK_AM_BIOETHICS:
		{
			if( !sd->ed ) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			enum e_mode current_mode = status_get_mode(&sd->ed->bl);
			enum e_mode new_mode = static_cast<e_mode>(EL_MODE_AGGRESSIVE);
			if (current_mode == static_cast<e_mode>(EL_MODE_AGGRESSIVE)){
				new_mode = static_cast<e_mode>(EL_MODE_PASSIVE);
			} 
			elemental_change_mode(sd->ed,new_mode);
		}
		break;
	case SK_HT_FALCONRY: //FALCON
		if( sd ) {
			enum e_mode mode = EL_MODE_PASSIVE;	// Default mode.

			if( skill_lv != 5 ) {
				int required_item_id = 0;
				switch(skill_lv) {
					case 1:
						required_item_id = ITEMID_YELLOW_LIVE;
						break;
					case 2:
						required_item_id = ITEMID_YELLOW_LIVE;
						break;
					case 3:
						required_item_id = ITEMID_YELLOW_LIVE;
						break;
					case 4:
						required_item_id = ITEMID_YELLOW_LIVE;
						break;
				}
				int elemental_class = skill_get_elemental_type(skill_id,skill_lv);

				if( sd->ed ) {
					// Just remove elemental if its the same class
					if( sd->ed->elemental.class_ == elemental_class) {
						elemental_delete(sd->ed);
						break;
					}
					// // Remove previous elemental first.
					if( sd->ed->elemental.class_ != elemental_class) {
						elemental_delete(sd->ed);
					}
				}

				// Summoning new one elemental
				int index_inventory_cost = pc_search_inventory(sd,required_item_id);
				if(index_inventory_cost == -1) {
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_NEED_ITEM,1,required_item_id);
					break;
				}

				if( !elemental_create(sd,elemental_class,skill_get_time(skill_id,skill_lv)) ) {
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
					break;
				}
			}
			if( skill_lv == 5 ) {
				if( !sd->ed ) {
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
					break;
				}
				if( sd->ed->elemental.class_ != ELEMENTALID_VENTUS_S) {
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
					break;
				}
				enum e_mode current_mode = status_get_mode(&sd->ed->bl);
				enum e_mode new_mode = static_cast<e_mode>(EL_MODE_AGGRESSIVE);
				if (current_mode == static_cast<e_mode>(EL_MODE_AGGRESSIVE)){
					new_mode = static_cast<e_mode>(EL_MODE_PASSIVE);
				} 
				elemental_change_mode(sd->ed,new_mode);
			}
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		}
		break;
	case SK_HT_WARGTRAINING: //WARG
		if( sd ) {
			enum e_mode mode = EL_MODE_PASSIVE;	// Default mode.

			if( skill_lv != 5 ) {
				int required_item_id = 0;
				switch(skill_lv) {
					case 1:
						required_item_id = ITEMID_YELLOW_LIVE;
						break;
					case 2:
						required_item_id = ITEMID_YELLOW_LIVE;
						break;
					case 3:
						required_item_id = ITEMID_YELLOW_LIVE;
						break;
					case 4:
						required_item_id = ITEMID_YELLOW_LIVE;
						break;
				}
				int elemental_class = skill_get_elemental_type(skill_id,skill_lv);

				if( sd->ed ) {
					// Just remove elemental if its the same class
					if( sd->ed->elemental.class_ == elemental_class) {
						elemental_delete(sd->ed);
						break;
					}
					// // Remove previous elemental first.
					if( sd->ed->elemental.class_ != elemental_class) {
						elemental_delete(sd->ed);
					}
				}

				// Summoning new one elemental
				int index_inventory_cost = pc_search_inventory(sd,required_item_id);
				if(index_inventory_cost == -1) {
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_NEED_ITEM,1,required_item_id);
					break;
				}

				if( !elemental_create(sd,elemental_class,skill_get_time(skill_id,skill_lv)) ) {
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
					break;
				}
			}
			if( skill_lv == 5 ) {
				if( !sd->ed ) {
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
					break;
				}
				if( sd->ed->elemental.class_ != ELEMENTALID_AQUA_S) {
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
					break;
				}
				enum e_mode current_mode = status_get_mode(&sd->ed->bl);
				enum e_mode new_mode = static_cast<e_mode>(EL_MODE_AGGRESSIVE);
				if (current_mode == static_cast<e_mode>(EL_MODE_AGGRESSIVE)){
					new_mode = static_cast<e_mode>(EL_MODE_PASSIVE);
				} 
				elemental_change_mode(sd->ed,new_mode);
			}
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		}
		break;
	case SK_AM_HATCHHOMUNCULUS: //HOMUN
		if( sd ) {
			int bioethics_lvl = 0;
			bioethics_lvl = pc_checkskill(sd, SK_AM_BIOETHICS);
			if (bioethics_lvl < 2){
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			if (bioethics_lvl < (skill_lv + 1)){
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}

			struct status_change *sc = status_get_sc(src);
			
			enum e_mode mode = EL_MODE_PASSIVE;	// Default mode.
			int elemental_class = skill_get_elemental_type(skill_id,skill_lv);

			if( sd->ed ) {
				// Just remove elemental if its the same class
				if( sd->ed->elemental.class_ == elemental_class) {
					elemental_delete(sd->ed);
					break;
				}
				if (sc->data[STATUS_BIOETHICS]) {
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
					break;
				}
				// Remove previous elemental first.
				if( sd->ed->elemental.class_ != elemental_class) {
					elemental_delete(sd->ed);
				}
			}
			if (sc->data[STATUS_BIOETHICS]) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			// Summoning new one elemental
			if( !elemental_create(sd,elemental_class,skill_get_time(skill_id,skill_lv)) ) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			
			sc_start2(src,src, STATUS_BIOETHICS, 100, skill_lv, src->id, 60000);
		}
		break;
	case SK_SA_SUMMONELEMENTAL:
		if( sd ) {

			int elemental_control_lvl = 0;
			elemental_control_lvl = pc_checkskill(sd, SK_SA_ELEMENTALCONTROL);
			if (elemental_control_lvl < 2){
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			if (elemental_control_lvl < (skill_lv + 1)){
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}


			struct status_change *sc = status_get_sc(src);
			enum e_mode mode = EL_MODE_PASSIVE;	// Default mode.
			int elemental_class = skill_get_elemental_type(skill_id,skill_lv);

			if( sd->ed ) {
				// Just remove elemental if its the same class
				if( sd->ed->elemental.class_ == elemental_class) {
					elemental_delete(sd->ed);
					break;
				}
				if (sc->data[STATUS_BIOETHICS]) {
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
					break;
				}
				// Remove previous elemental first.
				if( sd->ed->elemental.class_ != elemental_class) {
					elemental_delete(sd->ed);
				}
			}
			if (sc->data[STATUS_BIOETHICS]) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			// Summoning new one elemental
			if( !elemental_create(sd,elemental_class,skill_get_time(skill_id,skill_lv)) ) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			
			sc_start2(src,src, STATUS_BIOETHICS, 100, skill_lv, src->id, 60000);
		}
		break;

	case SK_AM_HOMUNCULUSACTIONII:
	case SK_CR_HOMUNCULUSACTIONIII:
	case SK_SA_ELEMENTALACTION2:
		if( sd ) {
			int duration = 7000;
			if( !sd->ed )
				break;
			sd->skill_id_old = skill_id;
			elemental_action(sd->ed, bl, tick, skill_id, skill_lv);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			skill_blockpc_start(sd, skill_id, duration);
		}
		break;
	case SK_HT_BLITZBEAT:
		if( sd->ed ) {
			if( sd->ed->elemental.class_ != ELEMENTALID_VENTUS_S) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
		}
		if( sd ) {
				int duration = 5000;
			if( !sd->ed )
				break;
			sd->skill_id_old = skill_id;
			elemental_action(sd->ed, bl, tick, skill_id, skill_lv);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			skill_blockpc_start(sd, skill_id, duration);
		}
		break;
	case SK_HT_SLASH:
		if( sd->ed ) {
			if( sd->ed->elemental.class_ != ELEMENTALID_AQUA_S) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}
		}
		if( sd ) {
				int duration = 5000;
			if( !sd->ed )
				break;
			sd->skill_id_old = skill_id;
			elemental_action(sd->ed, bl, tick, skill_id, skill_lv);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			skill_blockpc_start(sd, skill_id, duration);
		}
		break;
	case SK_AM_HOMUNCULUSACTIONI:
	case SK_SA_ELEMENTALACTION1:
		if( sd ) {
				int duration = 5000;
			if( !sd->ed )
				break;
			sd->skill_id_old = skill_id;
			elemental_action(sd->ed, bl, tick, skill_id, skill_lv);
			clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
			skill_blockpc_start(sd, skill_id, duration);
		}
		break;
	case SK_SA_ELEMENTALTRANSFUSION:
		if( sd ) {
			struct elemental_data *ed = sd->ed;
			int s_hp, s_sp;

			if( !ed )
				break;

			s_hp = tstatus->max_hp * (skill_lv*2) / 100;
			s_sp = tstatus->max_sp * (skill_lv*2) / 100;

			if( !status_charge(&sd->bl,s_hp,s_sp) ) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				break;
			}

			status_heal(&ed->bl,s_hp,s_sp,3);
			clif_specialeffect(src, EF_ENERVATION7, AREA);
			clif_specialeffect(&ed->bl, EF_ENERVATION7, AREA);
			clif_skill_nodamage(NULL,&ed->bl,SK_AL_HEAL,s_hp,1);
			clif_skill_nodamage(NULL,&ed->bl,SK_SM_SRECOVERY,s_sp,1);
			clif_skill_nodamage(src,&ed->bl,skill_id,skill_lv,1);
		}
		break;
	case SK_AM_REGENETATION:
		if( sd ) {
			int s_hp, s_sp;
			struct elemental_data *ed = sd->ed;

			if( !ed )
				break;
	
			s_hp = tstatus->max_hp * (skill_lv*5) / 100;
			s_sp = tstatus->max_sp * (skill_lv*2) / 100;

			clif_skill_nodamage(src,&ed->bl,SK_AM_REGENETATION,s_hp,1);
			clif_skill_nodamage(NULL,&ed->bl,SK_AL_HEAL,s_hp,1);
			clif_skill_nodamage(NULL,&ed->bl,SK_SM_SRECOVERY,s_sp,1);
			clif_specialeffect(src, 1061, AREA);
			clif_specialeffect(&ed->bl, 1061, AREA);
			status_heal(&ed->bl,s_hp,s_sp,0);
	
		}
		break;

	case SK_AM_BOMB:
		clif_skill_nodamage(src, bl, skill_id, skill_lv, 1);
		skill_attack(skill_get_type(skill_id), src, src, bl, skill_id, skill_lv, tick, flag|8);
		break;
	
	case SK_NV_EQSWITCH:
		if( sd ){
			clif_specialeffect(src, EF_BEGINSPELL7, AREA);
			clif_equipswitch_reply( sd, false );
			for( int i = 0, position = 0; i < EQI_MAX; i++ ){
				if( sd->equip_switch_index[i] >= 0 && !( position & equip_bitmask[i] ) && (i != EQI_AMMO) ){
					
					position |= pc_equipswitch( sd, sd->equip_switch_index[i] );
				}
			}
		}
		break;

	case SK_PR_LEXAETERNA:
		if (flag&1)
			clif_skill_nodamage(src, bl, skill_id, skill_lv, sc_start(src, bl, type, 100, skill_lv, skill_get_time(skill_id, skill_lv)));
		else {
			map_foreachinrange(skill_area_sub, bl, skill_get_splash(skill_id, skill_lv), BL_CHAR, src, skill_id, skill_lv, tick, flag|BCT_ENEMY|1, skill_castend_nodamage_id);
			clif_skill_nodamage(src, bl, skill_id, skill_lv, 1);
		}
		break;

	default:
		ShowWarning("skill_castend_nodamage_id: Unknown skill used:%d\n",skill_id);
		clif_skill_nodamage(src,bl,skill_id,skill_lv,1);
		map_freeblock_unlock();
		return 1;
	}

	

	if (dstmd) { //Mob skill event for no damage skills (damage ones are handled in battle_calc_damage) [Skotlex]
		mob_log_damage(dstmd, src, 0); //Log interaction (counts as 'attacker' for the exp bonus)
		mobskill_event(dstmd, src, tick, MSC_SKILLUSED|(skill_id<<16));
	}

	if( sd && !(flag&1) )
	{// ensure that the skill last-cast tick is recorded
		sd->canskill_tick = gettick();

		if( sd->state.arrow_atk )
		{// consume arrow on last invocation to this skill.
			battle_consume_ammo(sd, skill_id, skill_lv);
		}
		skill_onskillusage(sd, bl, skill_id, tick);
		// perform skill requirement consumption
		skill_consume_requirement(sd,skill_id,skill_lv,2);
	}

	map_freeblock_unlock();
	return 0;
}

/**
 * Checking that causing skill failed
 * @param src Caster
 * @param target Target
 * @param skill_id
 * @param skill_lv
 * @return -1 success, others are failed @see enum useskill_fail_cause.
 **/
static int8 skill_castend_id_check(struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv) {
	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);
	int inf = skill->inf;
	struct status_change *tsc = status_get_sc(target);

	if (src != target && (status_bl_has_mode(target,MD_SKILLIMMUNE) || (status_get_class(target) == MOBID_EMPERIUM && !skill->inf2[INF2_TARGETEMPERIUM])) && skill_get_casttype(skill_id) == CAST_NODAMAGE)
		return USESKILL_FAIL_MAX; // Don't show a skill fail message (NoDamage type doesn't consume requirements)

	switch (skill_id) {
		// case SK_CR_SWORDSTOPLOWSHARES:
		case SK_AL_HEAL:
		case SK_AL_INCREASEAGI:
		case SK_MO_DECAGI:
		case SK_SA_DISPELL: // Mado Gear is immune to Dispell according to bugreport:49 [Ind]
		case SK_AL_RENEW:
			if (tsc && tsc->option&OPTION_MADOGEAR)
				return USESKILL_FAIL_TOTARGET;
			break;
		case SK_RG_BACKSTAB:
			{
				if (check_distance_bl(src, target, 0))
					return USESKILL_FAIL_MAX;
			}
			break;
		
		case SK_SA_SILENCE:
		
		case SK_WG_SLASH:
			// Check if path can be reached
			if (!path_search(NULL,src->m,src->x,src->y,target->x,target->y,1,CELL_CHKNOREACH))
				return USESKILL_FAIL_MAX;
			break;
		case SK_AL_SACREDWAVE:
		case SK_FC_BLITZBEAT:
		case SK_AS_GRIMTOOTH:
		case SK_AS_PHANTOMMENACE:
			// These can damage traps, but can't target traps directly
			if (target->type == BL_SKILL) {
				TBL_SKILL *su = (TBL_SKILL*)target;
				if (!su || !su->group)
					return USESKILL_FAIL_MAX;				
				if (skill_get_inf2(su->group->skill_id, INF2_ISTRAP))
					return USESKILL_FAIL_MAX;
			}
			break;
	}

	if (inf&INF_ATTACK_SKILL ||
		(inf&INF_SELF_SKILL && skill->inf2[INF2_NOTARGETSELF]) //Combo skills
		) // Casted through combo.
		inf = BCT_ENEMY; //Offensive skill.
	else if (skill->inf2[INF2_NOTARGETENEMY])
		inf = BCT_NOENEMY;
	else
		inf = 0;

	if ((skill->inf2[INF2_PARTYONLY] || skill->inf2[INF2_GUILDONLY]) && src != target) {
		inf |=
			(skill->inf2[INF2_PARTYONLY]?BCT_PARTY:0)|
			(skill->inf2[INF2_GUILDONLY]?BCT_GUILD:0);
		//Remove neutral targets (but allow enemy if skill is designed to be so)
		inf &= ~BCT_NEUTRAL;
	}

	

	if (inf && battle_check_target(src, target, inf) <= 0) {
		switch(skill_id) {
			
			default:
				return USESKILL_FAIL_LEVEL;
		}
	}

	

	return -1;
}

TIMER_FUNC( skill_keep_using ){
	struct map_session_data* sd = map_id2sd( id );

	if( sd && sd->skill_keep_using.skill_id ){
		clif_parse_skill_toid( sd, sd->skill_keep_using.skill_id, sd->skill_keep_using.level, sd->skill_keep_using.target );
	}

	return 0;
}

/**
 * Check & process skill to target on castend. Determines if skill is 'damage' or 'nodamage'
 * @param tid
 * @param tick
 * @param data
 **/
TIMER_FUNC(skill_castend_id){
	struct block_list *target, *src;
	struct map_session_data *sd;
	struct mob_data *md;
	struct unit_data *ud;
	struct status_change *sc = NULL;
	int flag = 0;

	src = map_id2bl(id);
	if( src == NULL )
	{
		ShowDebug("skill_castend_id: src == NULL (tid=%d, id=%d)\n", tid, id);
		return 0;// not found
	}

	ud = unit_bl2ud(src);
	if( ud == NULL )
	{
		ShowDebug("skill_castend_id: ud == NULL (tid=%d, id=%d)\n", tid, id);
		return 0;// ???
	}

	sd = BL_CAST(BL_PC,  src);
	md = BL_CAST(BL_MOB, src);

	if( src->prev == NULL ) {
		ud->skilltimer = INVALID_TIMER;
		return 0;
	}

	if(ud->skill_id != SK_MG_CASTCANCEL && ud->skill_id != SK_PF_SPELLFIST) {// otherwise handled in unit_skillcastcancel()
		if( ud->skilltimer != tid ) {
			ShowError("skill_castend_id: Timer mismatch %d!=%d!\n", ud->skilltimer, tid);
			ud->skilltimer = INVALID_TIMER;
			return 0;
		}

		if( sd && ud->skilltimer != INVALID_TIMER )
		{// restore original walk speed
			ud->skilltimer = INVALID_TIMER;
			status_calc_bl(&sd->bl, SCB_SPEED|SCB_ASPD);
		} else
			ud->skilltimer = INVALID_TIMER;
	}

	if (ud->skilltarget == id)
		target = src;
	else
		target = map_id2bl(ud->skilltarget);

	// Use a do so that you can break out of it when the skill fails.
	do {
		bool fail = false;
		int8 res = USESKILL_FAIL_LEVEL;

		if (!target || target->prev == NULL)
			break;

		if (src->m != target->m || status_isdead(src))
			break;

		//These should become skill_castend_pos
		switch (ud->skill_id) {
			
			case SK_AM_WILDTHORNS:
			
				ud->skillx = target->x;
				ud->skilly = target->y;
				ud->skilltimer = tid;
				return skill_castend_pos(tid,tick,id,data);
		}

		// Failing
		if (fail || (res = skill_castend_id_check(src, target, ud->skill_id, ud->skill_lv)) >= 0) {
			if (sd && res != USESKILL_FAIL_MAX)
				clif_skill_fail(sd, ud->skill_id, (enum useskill_fail_cause)res, 0);
			break;
		}

		//Avoid doing double checks for instant-cast skills.
		if (tid != INVALID_TIMER && !status_check_skilluse(src, target, ud->skill_id, 1))
			break;

		if(md) {
			md->last_thinktime=tick +MIN_MOBTHINKTIME;
			if(md->skill_idx >= 0 && md->db->skill[md->skill_idx]->emotion >= 0)
				clif_emotion(src, md->db->skill[md->skill_idx]->emotion);
		}

		if (src != target && battle_config.skill_add_range &&
			!check_distance_bl(src, target, skill_get_range2(src, ud->skill_id, ud->skill_lv, true) + battle_config.skill_add_range))
		{
			if (sd) {
				clif_skill_fail(sd, ud->skill_id, USESKILL_FAIL_LEVEL, 0);
				if (battle_config.skill_out_range_consume) //Consume items anyway. [Skotlex]
					skill_consume_requirement(sd, ud->skill_id, ud->skill_lv, 3);
			}
			break;
		}
#ifdef OFFICIAL_WALKPATH
		if(skill_get_casttype(ud->skill_id) != CAST_NODAMAGE && !path_search_long(NULL, src->m, src->x, src->y, target->x, target->y, CELL_CHKWALL))
		{
			if (sd) {
				clif_skill_fail(sd,ud->skill_id,USESKILL_FAIL_LEVEL,0);
				skill_consume_requirement(sd,ud->skill_id,ud->skill_lv,3); //Consume items anyway.
			}
			break;
		}
#endif
		if( sd )
		{
			if( !skill_check_condition_castend(sd, ud->skill_id, ud->skill_lv) )
				break;
			else {
				skill_consume_requirement(sd,ud->skill_id,ud->skill_lv,1);
				if (src != target && (status_bl_has_mode(target,MD_SKILLIMMUNE) || (status_get_class(target) == MOBID_EMPERIUM && !skill_get_inf2(ud->skill_id, INF2_TARGETEMPERIUM))) && skill_get_casttype(ud->skill_id) == CAST_DAMAGE) {
					clif_skill_fail(sd, ud->skill_id, USESKILL_FAIL_LEVEL, 0);
					break; // Show a skill fail message (Damage type consumes requirements)
				}
			}
		}

		if( (src->type == BL_MER || src->type == BL_HOM) && !skill_check_condition_mercenary(src, ud->skill_id, ud->skill_lv, 1) )
			break;

		
		if (ud->walktimer != INVALID_TIMER)
			unit_stop_walking(src,1);

		if (!sd || sd->skillitem != ud->skill_id || skill_get_delay(ud->skill_id, ud->skill_lv))
			ud->canact_tick = i64max(tick + skill_delayfix(src, ud->skill_id, ud->skill_lv), ud->canact_tick - SECURITY_CASTTIME);
		if (sd) { //Cooldown application
			int cooldown = pc_get_skillcooldown(sd,ud->skill_id, ud->skill_lv); // Increases/Decreases cooldown of a skill by item/card bonuses.
			if(cooldown) skill_blockpc_start(sd, ud->skill_id, cooldown);
		}
		if( battle_config.display_status_timers && sd )
			clif_status_change(src, EFST_POSTDELAY, 1, skill_delayfix(src, ud->skill_id, ud->skill_lv), 0, 0, 0);
		if( sd )
		{
			switch( ud->skill_id )
			{
			
			case SK_KN_BRANDISHSPEAR: {
				sc_type type;

				if (ud->skill_id == SK_KN_BRANDISHSPEAR)
					type = STATUS_STRIPWEAPON;
				else
					type = STATUS_STRIPSHIELD;

				if ((sc = status_get_sc(src)) && sc->data[type]) {
					const struct TimerData* timer = get_timer(sc->data[type]->timer);

					if (timer && timer->func == status_change_timer && DIFF_TICK(timer->tick, gettick() + skill_get_time(ud->skill_id, ud->skill_lv)) > 0)
						break;
				}
				sc_start2(src, src, type, 100, 0, 1, skill_get_time(ud->skill_id, ud->skill_lv));
				break;
			}
			}
		}
		if (skill_get_state(ud->skill_id) != ST_MOVE_ENABLE)
			unit_set_walkdelay(src, tick, battle_config.default_walk_delay+skill_get_walkdelay(ud->skill_id, ud->skill_lv), 1);

		if(battle_config.skill_log && battle_config.skill_log&src->type)
			ShowInfo("Type %d, ID %d skill castend id [id =%d, lv=%d, target ID %d]\n",
				src->type, src->id, ud->skill_id, ud->skill_lv, target->id);

		map_freeblock_lock();

		if (skill_get_casttype(ud->skill_id) == CAST_NODAMAGE)
			skill_castend_nodamage_id(src,target,ud->skill_id,ud->skill_lv,tick,flag);
		else
			skill_castend_damage_id(src,target,ud->skill_id,ud->skill_lv,tick,flag);

		if( sd && sd->skill_keep_using.skill_id > 0 && sd->skill_keep_using.skill_id == ud->skill_id && !skill_isNotOk(ud->skill_id, sd) && skill_check_condition_castbegin(sd, ud->skill_id, ud->skill_lv) ){
			sd->skill_keep_using.tid = add_timer( sd->ud.canact_tick + 100, skill_keep_using, sd->bl.id, 0 );
		}

		sc = status_get_sc(src);
		if(sc && sc->count) {
			if (ud->skill_id != SK_HT_CAMOUFLAGE)
				status_change_end(src, STATUS_CAMOUFLAGE, INVALID_TIMER); // Applies to the first skill if active

			

		}

		if (sd ) // they just set the data so leave it as it is.[Inkfish]
			sd->skillitem = sd->skillitemlv = sd->skillitem_keep_requirement = 0;

		if (ud->skilltimer == INVALID_TIMER) {
			if(md) md->skill_idx = -1;
			else ud->skill_id = 0; //mobs can't clear this one as it is used for skill condition 'afterskill'
			ud->skill_lv = ud->skilltarget = 0;
		}
		map_freeblock_unlock();
		return 1;
	} while(0);

	//Skill failed.
	if (ud->skill_id == SK_SH_ASURASTRIKE && sd && !(sc && false))
	{	//When Asura fails... (except when it fails from Wall of Fog)
		//Consume SP/spheres
		skill_consume_requirement(sd,ud->skill_id, ud->skill_lv,1);
		status_set_sp(src, 0, 0);
		sc = &sd->sc;
		if (sc->count)
		{	//End states
			status_change_end(src, STATUS_ULTRAINSTINCT, INVALID_TIMER);
			status_change_end(src, STATUS_GRAPPLE, INVALID_TIMER);

		}
		if( target && target->m == src->m ) { //Move character to target anyway.
			short x, y;
			short dir = map_calc_dir(src,target->x,target->y);

			//Move 3 cells (From Caster)
			if( dir > 0 && dir < 4 )
				x = -3;
			else if( dir > 4 )
				x = 3;
			else
				x = 0;
			if( dir > 2 && dir < 6 )
				y = -3;
			else if( dir == 7 || dir < 2 )
				y = 3;
			else
				y = 0;
			if( unit_movepos(src, src->x + x, src->y + y, 1, 1) ) { //Display movement + animation.
				clif_blown(src);
				clif_spiritball(src);
			}
			clif_skill_damage(src,target,tick,sd->battle_status.amotion,0,0,1,ud->skill_id,ud->skill_lv,DMG_SPLASH);
		}
	}

	ud->skill_id = ud->skilltarget = 0;
	if( !sd || sd->skillitem != ud->skill_id || skill_get_delay(ud->skill_id,ud->skill_lv) )
		ud->canact_tick = tick;
	//You can't place a skill failed packet here because it would be
	//sent in ALL cases, even cases where skill_check_condition fails
	//which would lead to double 'skill failed' messages u.u [Skotlex]
	if (sd) {
		sd->skillitem = sd->skillitemlv = sd->skillitem_keep_requirement = 0;
		if (sd->skill_keep_using.skill_id > 0) {
			sd->skill_keep_using.skill_id = 0;
			if (sd->skill_keep_using.tid != INVALID_TIMER) {
				delete_timer(sd->skill_keep_using.tid, skill_keep_using);
				sd->skill_keep_using.tid = INVALID_TIMER;
			}
		}
	} else if (md)
		md->skill_idx = -1;
	return 0;
}

/*==========================================
 *
 *------------------------------------------*/
TIMER_FUNC(skill_castend_pos){
	struct block_list* src = map_id2bl(id);
	struct map_session_data *sd;
	struct unit_data *ud = unit_bl2ud(src);
	struct mob_data *md;

	nullpo_ret(ud);

	sd = BL_CAST(BL_PC , src);
	md = BL_CAST(BL_MOB, src);

	if( src->prev == NULL ) {
		ud->skilltimer = INVALID_TIMER;
		return 0;
	}

	if( ud->skilltimer != tid )
	{
		ShowError("skill_castend_pos: Timer mismatch %d!=%d\n", ud->skilltimer, tid);
		ud->skilltimer = INVALID_TIMER;
		return 0;
	}

	if( sd && ud->skilltimer != INVALID_TIMER )
	{// restore original walk speed
		ud->skilltimer = INVALID_TIMER;
		status_calc_bl(&sd->bl, SCB_SPEED|SCB_ASPD);
	} else
		ud->skilltimer = INVALID_TIMER;

	do {
		if( status_isdead(src) )
			break;

		if (!skill_pos_maxcount_check(src, ud->skillx, ud->skilly, ud->skill_id, ud->skill_lv, src->type, true))
			break;

		if(tid != INVALID_TIMER)
		{	//Avoid double checks on instant cast skills. [Skotlex]
			if (!status_check_skilluse(src, NULL, ud->skill_id, 1))
				break;
			if (battle_config.skill_add_range &&
				!check_distance_blxy(src, ud->skillx, ud->skilly, skill_get_range2(src, ud->skill_id, ud->skill_lv, true) + battle_config.skill_add_range)) {
				if (sd && battle_config.skill_out_range_consume) //Consume items anyway.
					skill_consume_requirement(sd, ud->skill_id, ud->skill_lv, 3);
				break;
			}
		}

		if( sd )
		{
			if( !skill_check_condition_castend(sd, ud->skill_id, ud->skill_lv) )
				break;
			else
				skill_consume_requirement(sd,ud->skill_id,ud->skill_lv,1);
		}

		if( (src->type == BL_MER || src->type == BL_HOM) && !skill_check_condition_mercenary(src, ud->skill_id, ud->skill_lv, 1) )
			break;

		if(md) {
			md->last_thinktime=tick +MIN_MOBTHINKTIME;
			if(md->skill_idx >= 0 && md->db->skill[md->skill_idx]->emotion >= 0)
				clif_emotion(src, md->db->skill[md->skill_idx]->emotion);
		}

		if(battle_config.skill_log && battle_config.skill_log&src->type)
			ShowInfo("Type %d, ID %d skill castend pos [id =%d, lv=%d, (%d,%d)]\n",
				src->type, src->id, ud->skill_id, ud->skill_lv, ud->skillx, ud->skilly);

		if (ud->walktimer != INVALID_TIMER)
			unit_stop_walking(src,1);

		if (!sd || sd->skillitem != ud->skill_id || skill_get_delay(ud->skill_id, ud->skill_lv))
			ud->canact_tick = i64max(tick + skill_delayfix(src, ud->skill_id, ud->skill_lv), ud->canact_tick - SECURITY_CASTTIME);
		if (sd) { //Cooldown application
			int cooldown = pc_get_skillcooldown(sd,ud->skill_id, ud->skill_lv);
			if(cooldown) skill_blockpc_start(sd, ud->skill_id, cooldown);
		}
		if( battle_config.display_status_timers && sd )
			clif_status_change(src, EFST_POSTDELAY, 1, skill_delayfix(src, ud->skill_id, ud->skill_lv), 0, 0, 0);
//		if( sd )
//		{
//			switch( ud->skill_id )
//			{
//			case ????:
//				sd->canequip_tick = tick + ????;
//				break;
//			}
//		}
		unit_set_walkdelay(src, tick, battle_config.default_walk_delay+skill_get_walkdelay(ud->skill_id, ud->skill_lv), 1);
		map_freeblock_lock();
		skill_castend_pos2(src,ud->skillx,ud->skilly,ud->skill_id,ud->skill_lv,tick,0);

		if (ud->skill_id != SK_HT_CAMOUFLAGE)
			status_change_end(src, STATUS_CAMOUFLAGE, INVALID_TIMER); // Applies to the first skill if active

		if( sd ) // Warp-Portal thru items will clear data in skill_castend_map. [Inkfish]
			sd->skillitem = sd->skillitemlv = sd->skillitem_keep_requirement = 0;

		if (ud->skilltimer == INVALID_TIMER) {
			if (md) md->skill_idx = -1;
			else ud->skill_id = 0; //Non mobs can't clear this one as it is used for skill condition 'afterskill'
			ud->skill_lv = ud->skillx = ud->skilly = 0;
		}

		map_freeblock_unlock();
		return 1;
	} while(0);

	if( !sd || sd->skillitem != ud->skill_id || skill_get_delay(ud->skill_id,ud->skill_lv) )
		ud->canact_tick = tick;
	ud->skill_id = ud->skill_lv = 0;
	if(sd)
		sd->skillitem = sd->skillitemlv = sd->skillitem_keep_requirement = 0;
	else if(md)
		md->skill_idx  = -1;
	return 0;

}

/* skill count without self */
static int skill_count_wos(struct block_list *bl,va_list ap) {
	struct block_list* src = va_arg(ap, struct block_list*);
	if( src->id != bl->id ) {
		return 1;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------*/
int skill_castend_pos2(struct block_list* src, int x, int y, uint16 skill_id, uint16 skill_lv, t_tick tick, int flag)
{
	struct map_session_data* sd;
	struct status_change* sc;
	struct status_change_entry *sce;
	struct skill_unit_group* sg;
	enum sc_type type;
	int i;

	//if(skill_lv <= 0) return 0;
	if(skill_id > 0 && !skill_lv) return 0;	// celest

	nullpo_ret(src);

	if(status_isdead(src))
		return 0;

	sd = BL_CAST(BL_PC, src);

	sc = status_get_sc(src);
	type = status_skill2sc(skill_id);
	sce = (sc && type != -1)?sc->data[type]:NULL;

	switch (skill_id) { //Skill effect.
		case SK_WZ_METEORSTORM:
		case SK_MG_ICEWALL:
		case SK_MO_BODYRELOCATION:
			break; //Effect is displayed on respective switch case.
		default:
			if(skill_get_inf(skill_id)&INF_SELF_SKILL)
				clif_skill_nodamage(src,src,skill_id,skill_lv,1);
			else
				clif_skill_poseffect(src,skill_id,skill_lv,x,y,tick);
	}

	switch(skill_id)
	{
	case SK_BI_BENEDICTIO:
		skill_area_temp[1] = src->id;
		i = skill_get_splash(skill_id, skill_lv);
		map_foreachinallarea(skill_area_sub,
			src->m, x-i, y-i, x+i, y+i, BL_PC,
			src, skill_id, skill_lv, tick, flag|BCT_ALL|1,
			skill_castend_nodamage_id);
		map_foreachinallarea(skill_area_sub,
			src->m, x-i, y-i, x+i, y+i, BL_CHAR,
			src, skill_id, skill_lv, tick, flag|BCT_ENEMY|1,
			skill_castend_damage_id);
		break;
	case SK_MG_QUAGMIRE:
	case SK_WZ_LORDOFVERMILION:
	case SK_WZ_STORMGUST:
	case SK_WZ_MAGICCRASHER:
	case SK_WZ_PSYCHICWAVE:
	case SK_BI_PENITENTIA:
	case SK_WZ_REALITYBREAKER:
	case SK_SO_ASTRALSTRIKE:
	case SK_SO_DOOM:
	case SK_SO_DOOM_GHOST:
	case SK_SO_DIAMONDDUST:
	case SK_PR_SANCTUARIO:
	case SK_PR_MAGNUSEXORCISMUS:
	case SK_CR_GRANDCROSS:
	case SK_PF_LANDPROTECTOR:
	case SK_WZ_EARTHSTRAIN:
	case SK_CM_MILLENIUMSHIELDS:
		flag|=1;//Set flag to 1 to prevent deleting ammo (it will be deleted on group-delete).
	case SK_AM_WILDTHORNS:
	case SK_AM_FIREDEMONSTRATION:
	case SK_CR_INCENDIARYBOMB:
		skill_unitsetting(src,skill_id,skill_lv,x,y,0);
		break;
	case SK_MG_ICEWALL:
		flag|=1;
		if(skill_unitsetting(src,skill_id,skill_lv,x,y,0))
			clif_skill_poseffect(src,skill_id,skill_lv,x,y,tick);
		break;
	
	case SK_AS_POISONOUSCLOUD:
		flag |= 4;
		skill_unitsetting(src,skill_id,skill_lv,x,y,0);
		break;

	// Fall through
	case SK_WZ_METEORSTORM: {
			int area = skill_get_splash(skill_id, skill_lv);
			short tmpx = 0, tmpy = 0;

			for (i = 1; i <= skill_get_time(skill_id, skill_lv)/skill_get_unit_interval(skill_id); i++) {
				// Creates a random Cell in the Splash Area
				tmpx = x - area + rnd()%(area * 2 + 1);
				tmpy = y - area + rnd()%(area * 2 + 1);
				skill_unitsetting(src, skill_id, skill_lv, tmpx, tmpy, flag+i*skill_get_unit_interval(skill_id));
			}
		}
		break;

	
	case SK_MO_BODYRELOCATION:
		if (unit_movepos(src, x, y, 2, 1)) {
#if PACKETVER >= 20111005
			clif_snap(src, src->x, src->y);
#else
			clif_skill_poseffect(src,skill_id,skill_lv,src->x,src->y,tick);
#endif
			sc_start4(src,src,STATUS_DEFENSIVESTANCE,100,3,20,0,0,30000);
			status_change_end(src, STATUS_GRAPPLE, INVALID_TIMER);
			if (sd)
				skill_blockpc_start (sd, SK_SH_ASURASTRIKE, 2000);
		}
		break;
	

	// Slim Pitcher [Celest]
	case SK_CR_POTIONPITCHER:
		if (sd) {
		clif_specialeffect(&sd->bl, EF_TWILIGHT2, AREA);
		int i_lv = 0, j = 0;
		struct s_skill_condition require = skill_get_requirement(sd, skill_id, skill_lv);
		i_lv = skill_lv%11 - 1;
		j = pc_search_inventory(sd, require.itemid[i_lv]);
		if (j < 0 || require.itemid[i_lv] <= 0 || sd->inventory_data[j] == NULL || sd->inventory.u.items_inventory[j].amount < require.amount[i_lv])
			{
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return 1;
			}
			potion_flag = 1;
			potion_hp = 0;
			potion_sp = 0;
			run_script(sd->inventory_data[j]->script,0,sd->bl.id,0);
			potion_flag = 0;
			//Apply skill bonuses
			i_lv = pc_checkskill(sd,SK_CR_POTIONPITCHER)*10
				+ pc_checkskill(sd,SK_AM_AIDPOTION)*10
				+ pc_skillheal_bonus(sd, skill_id);

			potion_hp = potion_hp * (100+i_lv)/100;
			potion_sp = potion_sp * (100+i_lv)/100;

			if(potion_hp > 0 || potion_sp > 0) {
				i_lv = skill_get_splash(skill_id, skill_lv);
				map_foreachinallarea(skill_area_sub,
					src->m,x-i_lv,y-i_lv,x+i_lv,y+i_lv,BL_CHAR,
					src,skill_id,skill_lv,tick,flag|BCT_PARTY|BCT_GUILD|1,
					skill_castend_nodamage_id);
			}
		} else {
			struct item_data *item = itemdb_search(skill_db.find(skill_id)->require.itemid[skill_lv]);
			int id = skill_get_max(SK_CR_POTIONPITCHER) * 10;

			potion_flag = 1;
			potion_hp = 0;
			potion_sp = 0;
			run_script(item->script,0,src->id,0);
			potion_flag = 0;
			potion_hp = potion_hp * (100+id)/100;
			potion_sp = potion_sp * (100+id)/100;

			if(potion_hp > 0 || potion_sp > 0) {
				id = skill_get_splash(skill_id, skill_lv);
				map_foreachinallarea(skill_area_sub,
					src->m,x-id,y-id,x+id,y+id,BL_CHAR,
					src,skill_id,skill_lv,tick,flag|BCT_PARTY|BCT_GUILD|1,
						skill_castend_nodamage_id);
			}
		}
		break;



	case SK_PA_GOSPEL:
		if (sce && sce->val4 == BCT_SELF)
		{
			status_change_end(src, STATUS_GOSPEL, INVALID_TIMER);
			return 0;
		}
		else
		{
			sg = skill_unitsetting(src,skill_id,skill_lv,src->x,src->y,0);
			if (!sg) break;
			if (sce)
				status_change_end(src, type, INVALID_TIMER); //Was under someone else's Gospel. [Skotlex]
			sc_start4(src,src,type,100,skill_lv,0,sg->group_id,BCT_SELF,skill_get_time(skill_id,skill_lv));
			clif_skill_poseffect(src, skill_id, skill_lv, 0, 0, tick); // SK_PA_GOSPEL music packet
		}
		break;
	

	case SK_AC_ARROWRAIN:
	case SK_AC_ARROWSHOWER:
		status_change_end(src, STATUS_CAMOUFLAGE, INVALID_TIMER);

	case SK_CM_DRAGONBREATH:
		// Cast center might be relevant later (e.g. for knockback direction)
		skill_area_temp[4] = x;
		skill_area_temp[5] = y;
		i = skill_get_splash(skill_id,skill_lv);
		map_foreachinarea(skill_area_sub,src->m,x-i,y-i,x+i,y+i,BL_CHAR|BL_SKILL,src,skill_id,skill_lv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id);
		break;

	case SK_BI_EPICLESIS:
		if( (sg = skill_unitsetting(src, skill_id, skill_lv, x, y, 0)) ) {
			i = skill_get_splash(skill_id, skill_lv);
			map_foreachinallarea(skill_area_sub, src->m, x - i, y - i, x + i, y + i, BL_CHAR, src, SK_PR_RESURRECTIO, 1, tick, flag|BCT_NOENEMY|1,skill_castend_nodamage_id);
		}
		break;
	
	case SK_MS_NEUTRALBARRIER:
		if( (sc->data[STATUS_NEUTRALBARRIER_MASTER] && skill_id == SK_MS_NEUTRALBARRIER) ) {
			skill_clear_unitgroup(src);
			return 0;
		}
		skill_clear_unitgroup(src); // To remove previous skills - cannot used combined
		if( (sg = skill_unitsetting(src,skill_id,skill_lv,src->x,src->y,0)) != NULL ) {
			sc_start2(src,src,skill_id == SK_MS_NEUTRALBARRIER ? STATUS_NEUTRALBARRIER_MASTER : STATUS_NEUTRALBARRIER_MASTER,100,skill_lv,sg->group_id,skill_get_time(skill_id,skill_lv));
		}
		break;

	case SK_AM_PLANTCULTIVATION:
		if( sd ) clif_plant_cultivation_list(sd,skill_lv,x,y,skill_id);
		break;
	case SK_HT_MYSTICTOTEM:
	case SK_HT_ELEMENTALTOTEM:
		if( sd ) clif_magicdecoy_list(sd,skill_lv,x,y,skill_id);
		break;
	


	case SK_PA_GENESISRAY:
		i = skill_get_splash(skill_id,skill_lv);
		map_foreachinarea(skill_area_sub,src->m,x-i,y-i,x+i,y+i,BL_CHAR|BL_SKILL,src,skill_id,skill_lv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id);
		break;
	case SK_BA_REVERBERATION:
		i = skill_get_splash(skill_id,skill_lv);
		map_foreachinarea(skill_area_sub,src->m,x-i,y-i,x+i,y+i,BL_CHAR,src,skill_id,skill_lv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id);
		break;

	case SK_BA_GREATECHO:
		i = skill_get_splash(skill_id,skill_lv);
		map_foreachinarea(skill_area_sub,src->m,x-i,y-i,x+i,y+i,BL_CHAR,src,skill_id,skill_lv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id);
		break;

	case SK_CL_SEVERERAINSTORM:
		flag |= 1;
		if (sd)
			sd->canequip_tick = tick + skill_get_time(skill_id, skill_lv); // Can't switch equips for the duration of the skill.
		skill_unitsetting(src,skill_id,skill_lv,x,y,0);
		break;

	case SK_CR_MANDRAKERAID: {
			i = skill_get_splash(SK_CR_MANDRAKERAID_ATK, skill_lv);
			map_foreachinarea(skill_area_sub,src->m,x-i,y-i,x+i,y+i,BL_CHAR,src,SK_CR_MANDRAKERAID_ATK,skill_lv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id);

			
		}
		break;
	
	default:
		ShowWarning("skill_castend_pos2: Unknown skill used:%d\n",skill_id);
		return 1;
	}

	
	if( sd )
	{// ensure that the skill last-cast tick is recorded
		sd->canskill_tick = gettick();

		if( sd->state.arrow_atk && !(flag&1) )
		{// consume arrow if this is a ground skill
			battle_consume_ammo(sd, skill_id, skill_lv);
		}
		skill_onskillusage(sd, NULL, skill_id, tick);
		// perform skill requirement consumption
		skill_consume_requirement(sd,skill_id,skill_lv,2);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------*/
int skill_castend_map (struct map_session_data *sd, uint16 skill_id, const char *mapname)
{
	nullpo_ret(sd);

//Simplify skill_failed code.
#define skill_failed(sd) { sd->menuskill_id = sd->menuskill_val = 0; }
	if(skill_id != sd->menuskill_id)
		return 0;

	if( sd->bl.prev == NULL || pc_isdead(sd) ) {
		skill_failed(sd);
		return 0;
	}

	if( ( sd->sc.opt1 && sd->sc.opt1 != OPT1_BURNING ) || sd->sc.option&OPTION_HIDE ) {
		skill_failed(sd);
		return 0;
	}

	pc_stop_attack(sd);

	if(battle_config.skill_log && battle_config.skill_log&BL_PC)
		ShowInfo("PC %d skill castend skill =%d map=%s\n",sd->bl.id,skill_id,mapname);

	if(strcmp(mapname,"cancel")==0) {
		skill_failed(sd);
		return 0;
	}

	sd->menuskill_id = sd->menuskill_val = 0;
	return 0;
#undef skill_failed
}

/// transforms 'target' skill unit into dissonance (if conditions are met)
static int skill_dance_overlap_sub(struct block_list* bl, va_list ap)
{
	struct skill_unit* target = (struct skill_unit*)bl;
	struct skill_unit* src = va_arg(ap, struct skill_unit*);
	int flag = va_arg(ap, int);

	if (src == target)
		return 0;
	if (!target->group || !(target->group->state.song_dance&0x1))
		return 0;
	if (!(target->val2 & src->val2 & ~(1 << UF_ENSEMBLE))) //They don't match (song + dance) is valid.
		return 0;

	if (flag) //Set dissonance
		target->val2 |= (1 << UF_ENSEMBLE); //Add ensemble to signal this unit is overlapping.
	else //Remove dissonance
		target->val2 &= ~(1 << UF_ENSEMBLE);

	skill_getareachar_skillunit_visibilty(target, AREA);

	return 1;
}

//Does the song/dance overlapping -> dissonance check. [Skotlex]
//When flag is 0, this unit is about to be removed, cancel the dissonance effect
//When 1, this unit has been positioned, so start the cancel effect.
int skill_dance_overlap(struct skill_unit* unit, int flag)
{
	if (!unit || !unit->group || !(unit->group->state.song_dance&0x1))
		return 0;
	if (!flag && !(unit->val2&(1 << UF_ENSEMBLE)))
		return 0; //Nothing to remove, this unit is not overlapped.

	if (unit->val1 != unit->group->skill_id)
	{	//Reset state
		unit->val1 = unit->group->skill_id;
		unit->val2 &= ~(1 << UF_ENSEMBLE);
	}

	return map_foreachincell(skill_dance_overlap_sub, unit->bl.m,unit->bl.x,unit->bl.y,BL_SKILL, unit,flag);
}

/**
 * Converts this group information so that it is handled as a Dissonance or Ugly Dance cell.
 * @param unit Skill unit data (from BA_DISSONANCE or DC_UGLYDANCE)
 * @param flag 0 Convert
 * @param flag 1 Revert
 * @return true success
 * @TODO: This should be completely removed later and rewritten
 *	The entire execution of the overlapping songs instances is dirty and hacked together
 *	Overlapping cells should be checked on unit entry, not infinitely loop checked causing 1000's of executions a song/dance
 */
static bool skill_dance_switch(struct skill_unit* unit, int flag)
{
	static int prevflag = 1;  // by default the backup is empty
	static struct skill_unit_group backup;
	struct skill_unit_group* group;

	if( unit == NULL || (group = unit->group) == NULL )
		return false;

	//val2&(1 << UF_ENSEMBLE) is a hack to indicate dissonance
	if ( !((group->state.song_dance&0x1) && (unit->val2&(1 << UF_ENSEMBLE))) )
		return false;

	if( flag == prevflag ) { //Protection against attempts to read an empty backup/write to a full backup
		ShowError("skill_dance_switch: Attempted to %s (skill_id=%d, skill_lv=%d, src_id=%d).\n",
			flag ? "read an empty backup" : "write to a full backup",
			group->skill_id, group->skill_lv, group->src_id);
		return false;
	}

	prevflag = flag;

	if (!flag) { //Transform
		//uint16 skill_id = unit->val2 & (1 << UF_SONG) ? BA_DISSONANCE : DC_UGLYDANCE;
		uint16 skill_id =1;

		// backup
		backup.skill_id    = group->skill_id;
		backup.skill_lv    = group->skill_lv;
		backup.unit_id     = group->unit_id;
		backup.target_flag = group->target_flag;
		backup.bl_flag     = group->bl_flag;
		backup.interval    = group->interval;

		// replace
		group->skill_id    = skill_id;
		group->skill_lv    = 1;
		group->unit_id     = skill_get_unit_id(skill_id);
		group->target_flag = skill_get_unit_target(skill_id);
		group->bl_flag     = skill_get_unit_bl_target(skill_id);
		group->interval    = skill_get_unit_interval(skill_id);
	} else { //Restore
		group->skill_id    = backup.skill_id;
		group->skill_lv    = backup.skill_lv;
		group->unit_id     = backup.unit_id;
		group->target_flag = backup.target_flag;
		group->bl_flag     = backup.bl_flag;
		group->interval    = backup.interval;
	}

	return true;
}

/**
 * Initializes and sets a ground skill / skill unit. Usually called after skill_casted_pos() or skill_castend_map()
 * @param src Object that triggers the skill
 * @param skill_id Skill ID
 * @param skill_lv Skill level of used skill
 * @param x Position x
 * @param y Position y
 * @param flag &1: Used to determine when the skill 'morphs' (Warp portal becomes active, or Fire Pillar becomes active)
 *		xx_METEOR: flag &1 contains if the unit can cause curse, flag is also the duration of the unit in milliseconds
 * @return skill_unit_group
 */
struct skill_unit_group *skill_unitsetting(struct block_list *src, uint16 skill_id, uint16 skill_lv, int16 x, int16 y, int flag)
{
	struct skill_unit_group *group;
	int i, val1 = 0, val2 = 0, val3 = 0;
	t_tick limit;
	int link_group_id = 0;
	int target, interval, range;
	t_itemid req_item = 0;
	struct s_skill_unit_layout *layout;
	struct map_session_data *sd;
	struct status_data *status;
	struct status_change *sc;
	int active_flag = 1;
	int subunt = 0;
	bool hidden = false;
	struct map_data *mapdata;

	nullpo_retr(NULL, src);

	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);

	mapdata = map_getmapdata(src->m);
	limit = skill_get_time3(mapdata, skill_id,skill_lv);
	range = skill_get_unit_range(skill_id,skill_lv);
	interval = skill->unit_interval;
	target = skill_get_unit_target(skill_id);
	layout = skill_get_unit_layout(skill_id,skill_lv,src,x,y);

	sd = BL_CAST(BL_PC, src);
	status = status_get_status_data(src);
	sc = status_get_sc(src);	// for traps, firewall and fogwall - celest
	hidden = (skill->unit_flag[UF_HIDDENTRAP] && (battle_config.traps_setting == 2 || (battle_config.traps_setting == 1 && map_flag_vs(src->m))));

	switch( skill_id ) {
	
	case SK_PR_SANCTUARIO:
		val1=skill_lv+3;
		break;
	case SK_WZ_METEORSTORM:
		limit = flag;
		flag = 0; // Flag should not influence anything else for these skills
		break;
	case SK_MG_QUAGMIRE:	//The target changes to "all" if used in a gvg map. [Skotlex]
		if (battle_config.vs_traps_bctall && (src->type&battle_config.vs_traps_bctall) && map_flag_vs(src->m))
			target = BCT_ALL;
		break;


	case SK_PF_LANDPROTECTOR:
	{
		struct skill_unit_group *old_sg;
		if ((old_sg = skill_locate_element_field(src)) != NULL)
		{	//HelloKitty confirmed that these are interchangeable,
			//so you can change element and not consume gemstones.
			//if ((
			//	old_sg->skill_id == SA_VOLCANO ||
			//	old_sg->skill_id == SA_DELUGE ||
			//	old_sg->skill_id == SA_VIOLENTGALE
			//) && old_sg->limit > 0)
			//{	//Use the previous limit (minus the elapsed time) [Skotlex]
			//	limit = old_sg->limit - DIFF_TICK(gettick(), old_sg->tick);
			//	if (limit < 0)	//This can happen...
			//		limit = skill_get_time(skill_id,skill_lv);
			//}
			skill_clear_group(src,1);
		}
		break;
	}

	case SK_BA_MAGICSTRINGS:
		val1 = 3 * skill_lv + status->dex / 10; // Casting time reduction
		//For some reason at level 10 the base delay reduction is 50%.
		val2 = (skill_lv < 10 ? 3 * skill_lv : 50) + status->int_ / 5; // After-cast delay reduction
		if (sd) {
			val1 += pc_checkskill(sd, SK_BA_SHOWMANSHIP);
			val2 += 2 * pc_checkskill(sd, SK_BA_SHOWMANSHIP);
		}
		break;
	
	case SK_BA_IMPRESSIVERIFF:
		if (sd)
			val1 = pc_checkskill(sd, SK_BA_SHOWMANSHIP) / 2;
		val1 += 5 + skill_lv + (status->agi / 20);
		val1 *= 10; // ASPD works with 1000 as 100%
		break;
	
	case SK_CL_BATTLEDRUMS:
		val1 = (skill_lv+1)*25;	//Atk increase
		val2 = (skill_lv+1)*2;	//Def increase
		break;
	
	case SK_BA_ACOUSTICRYTHM:
		val1 = 25 + 11*skill_lv; //Exp increase bonus.
		break;
	case SK_CL_PAGANPARTY:
		val1 = 55 + skill_lv*5;	//Elemental Resistance
		val2 = skill_lv*10;	//Status ailment resistance
		break;
	case GD_LEADERSHIP:
	case GD_GLORYWOUNDS:
	case GD_SOULCOLD:
	case GD_HAWKEYES:
		limit = 1000000;//it doesn't matter
		break;
	
	case SK_CL_SEVERERAINSTORM:
		if( map_getcell(src->m, x, y, CELL_CHKLANDPROTECTOR) )
			return NULL;
		break;
	case SK_AS_POISONOUSCLOUD:
		skill_clear_group(src, 4);
		break;

	case SK_AM_WILDTHORNS:
		// Turns to Firewall
		if( flag&1 )
			limit = skill_lv * 3000;
		val3 = (x<<16)|y;
		break;
	case SK_AM_FIREDEMONSTRATION:
	case SK_CR_INCENDIARYBOMB:
		if (flag) { // Fire Expansion level 1
			limit = flag + 10000;
			flag = 0;
		}
		break;
	
	}

	// Init skill unit group
	nullpo_retr(NULL, (group = skill_initunitgroup(src,layout->count,skill_id,skill_lv,(flag & 1 ? skill->unit_id2 : skill->unit_id)+subunt, limit, interval)));
	group->val1 = val1;
	group->val2 = val2;
	group->val3 = val3;
	group->link_group_id = link_group_id;
	group->target_flag = target;
	group->bl_flag = skill_get_unit_bl_target(skill_id);
	group->state.ammo_consume = (sd && sd->state.arrow_atk ); //Store if this skill needs to consume ammo.
	group->state.song_dance = (((skill->unit_flag[UF_DANCE] || skill->unit_flag[UF_SONG])?1:0)|(skill->unit_flag[UF_ENSEMBLE]?2:0)); //Signals if this is a song/dance/duet
	group->state.guildaura = ( skill_id >= GD_LEADERSHIP && skill_id <= GD_HAWKEYES )?1:0;
	group->item_id = req_item;

	// If tick is greater than current, do not invoke onplace function just yet. [Skotlex]
	if (DIFF_TICK(group->tick, gettick()) > SKILLUNITTIMER_INTERVAL)
		active_flag = 0;

	
	// Dance skill
	if (group->state.song_dance) {
		if(sd) {
			sd->skill_id_dance = skill_id;
			sd->skill_lv_dance = skill_lv;
		}
		
	}

	// Set skill unit
	limit = group->limit;
	for( i = 0; i < layout->count; i++ ) {
		struct skill_unit *unit;
		int ux = x + layout->dx[i];
		int uy = y + layout->dy[i];
		int unit_val1 = skill_lv;
		int unit_val2 = 0;
		int alive = 1;

		// are the coordinates out of range?
		if( ux <= 0 || uy <= 0 || ux >= mapdata->xs || uy >= mapdata->ys ){
			continue;
		}

		if( !group->state.song_dance && !map_getcell(src->m,ux,uy,CELL_CHKREACH) )
			continue; // don't place skill units on walls (except for songs/dances/encores)
		if( battle_config.skill_wall_check && skill->unit_flag[UF_PATHCHECK] && !path_search_long(NULL,src->m,ux,uy,src->x,src->y,CELL_CHKWALL) )
			continue; // no path between cell and caster

		switch( skill_id ) {
			// HP for Skill unit that can be damaged, see also skill_unit_ondamaged
			
			case SK_MG_ICEWALL:
				unit_val1 = 200 + 400*skill_lv;
				unit_val2 = map_getcell(src->m, ux, uy, CELL_GETTYPE);
				break;
			
			case SK_AM_WILDTHORNS:
				if (flag&1) // Turned become Firewall
					break;
				unit_val1 = sd->status.max_hp; // HP
				break;
			
			default:
				if (group->state.song_dance&0x1)
					unit_val2 = (skill->unit_flag[UF_DANCE] ? (1 << UF_DANCE) : skill->unit_flag[UF_SONG] ? (1 << UF_SONG) : 0); //Store whether this is a song/dance
				break;
		}

		if (skill->unit_flag[UF_RANGEDSINGLEUNIT] && i == (layout->count / 2))
			unit_val2 |= (1 << UF_RANGEDSINGLEUNIT); // center.

		if( sd && map_getcell(src->m, ux, uy, CELL_CHKMAELSTROM) ) //Does not recover SP from monster skills
			map_foreachincell(skill_maelstrom_suction,src->m,ux,uy,BL_SKILL,skill_id,skill_lv);

		// Check active cell to failing or remove current unit
		map_foreachincell(skill_cell_overlap,src->m,ux,uy,BL_SKILL,skill_id, &alive, src);
		if( !alive )
			continue;

		nullpo_retr(NULL, (unit = skill_initunit(group,i,ux,uy,unit_val1,unit_val2,hidden)));
		unit->limit = limit;
		unit->range = range;

		

		// Execute on all targets standing on this cell
		if (range == 0 && active_flag)
			map_foreachincell(skill_unit_effect,unit->bl.m,unit->bl.x,unit->bl.y,group->bl_flag,&unit->bl,gettick(),1);
	}

	if (!group->alive_count)
	{	//No cells? Something that was blocked completely by Land Protector?
		skill_delunitgroup(group);
		return NULL;
	}

	

	return group;
}

/*==========================================
 *
 *------------------------------------------*/
void ext_skill_unit_onplace(struct skill_unit *unit, struct block_list *bl, t_tick tick)
{
	skill_unit_onplace(unit, bl, tick);
}

/**
 * Triggeres when 'target' (based on skill unit target) is stand at unit area
 * while skill unit initialized or moved (such by knock back).
 * As a follow of skill_unit_effect flag &1
 * @param unit
 * @param bl Target
 * @param tick
 */
static int skill_unit_onplace(struct skill_unit *unit, struct block_list *bl, t_tick tick)
{
	struct skill_unit_group *sg;
	struct block_list *ss; // Actual source that cast the skill unit
	struct status_change *sc;
	struct status_change_entry *sce;
	struct status_data *tstatus;
	enum sc_type type;
	uint16 skill_id;

	nullpo_ret(unit);
	nullpo_ret(bl);

	if(bl->prev == NULL || !unit->alive || status_isdead(bl))
		return 0;

	nullpo_ret(sg = unit->group);
	nullpo_ret(ss = map_id2bl(sg->src_id));

	tstatus = status_get_status_data(bl);

	if( (skill_get_type(sg->skill_id) == BF_MAGIC && ((battle_config.land_protector_behavior) ? map_getcell(bl->m, bl->x, bl->y, CELL_CHKLANDPROTECTOR) : map_getcell(unit->bl.m, unit->bl.x, unit->bl.y, CELL_CHKLANDPROTECTOR)) && sg->skill_id != SK_PF_LANDPROTECTOR) ||
		map_getcell(bl->m, bl->x, bl->y, CELL_CHKMAELSTROM) )
		return 0; //AoE skills are ineffective. [Skotlex]

	std::shared_ptr<s_skill_db> skill = skill_db.find(sg->skill_id);

	if( (skill->inf2[INF2_ISSONG] || skill->inf2[INF2_ISENSEMBLE]) && map_getcell(bl->m, bl->x, bl->y, CELL_CHKBASILICA) )
		return 0; //Songs don't work in Basilica

	sc = status_get_sc(bl);

	if (sc && sc->option&OPTION_HIDE && !skill->inf2[INF2_TARGETHIDDEN])
		return 0; //Hidden characters are immune to AoE skills except to these. [Skotlex]

	


	type = status_skill2sc(sg->skill_id);
	sce = (sc && type != STATUS_NONE) ? sc->data[type] : NULL;
	skill_id = sg->skill_id; //In case the group is deleted, we need to return the correct skill id, still.
	switch (sg->unit_id) {
		case UNT_SPIDERWEB:
			if (sc) {
				//Duration in PVM is: 1st - 8s, 2nd - 16s, 3rd - 8s
				//Duration in PVP is: 1st - 4s, 2nd - 8s, 3rd - 12s
				t_tick sec = skill_get_time2(sg->skill_id, sg->skill_lv);
				const struct TimerData* td;
				struct map_data *mapdata = map_getmapdata(bl->m);

				if (mapdata_flag_vs(mapdata))
					sec /= 2;
				if (sc->data[type]) {
					if (sc->data[type]->val2 && sc->data[type]->val3 && sc->data[type]->val4) {
						//Already triple affected, immune
						sg->limit = DIFF_TICK(tick, sg->tick);
						break;
					}
					//Don't increase val1 here, we need a higher val in status_change_start so it overwrites the old one
					if (mapdata_flag_vs(mapdata) && sc->data[type]->val1 < 3)
						sec *= (sc->data[type]->val1 + 1);
					else if(!mapdata_flag_vs(mapdata) && sc->data[type]->val1 < 2)
						sec *= (sc->data[type]->val1 + 1);
					//Add group id to status change
					if (sc->data[type]->val2 == 0)
						sc->data[type]->val2 = sg->group_id;
					else if (sc->data[type]->val3 == 0)
						sc->data[type]->val3 = sg->group_id;
					else if (sc->data[type]->val4 == 0)
						sc->data[type]->val4 = sg->group_id;
					//Overwrite status change with new duration
					if ((td = get_timer(sc->data[type]->timer))!=NULL)
						status_change_start(ss, bl, type, 10000, sc->data[type]->val1 + 1, sc->data[type]->val2, sc->data[type]->val3, sc->data[type]->val4,
							i64max(DIFF_TICK(td->tick, tick), sec), SCSTART_NORATEDEF);
				}
				else {
					if (status_change_start(ss, bl, type, 10000, 1, sg->group_id, 0, 0, sec, SCSTART_NORATEDEF)) {
						td = sc->data[type] ? get_timer(sc->data[type]->timer) : NULL;
						if (td)
							sec = DIFF_TICK(td->tick, tick);
						map_moveblock(bl, unit->bl.x, unit->bl.y, tick);
						clif_fixpos(bl);
					}
					else
						sec = 3000; //Couldn't trap it?
				}
				sg->val2 = bl->id;
				sg->limit = DIFF_TICK(tick, sg->tick) + sec;
			}
			break;
		case UNT_SAFETYWALL:
			if (!sce)
				sc_start4(ss, bl,type,100,sg->skill_lv,sg->skill_id,sg->group_id,0,sg->limit);
			break;

		

		case UNT_PNEUMA:
			if (!sce)
				sc_start4(ss, bl,type,100,sg->skill_lv,sg->group_id,0,0,sg->limit);
			break;

		case UNT_CHAOSPANIC:
			status_change_start(ss, bl, type, 3500 + (sg->skill_lv * 1500), sg->skill_lv, 0, 0, 1, sg->skill_lv * 4000, SCSTART_NOAVOID|SCSTART_NORATEDEF|SCSTART_NOTICKDEF);
			break;

		case UNT_WARP_WAITING: {
			int working = sg->val1&0xffff;

			if(bl->type==BL_PC && !working){
				struct map_session_data *sd = (struct map_session_data *)bl;
				if((!sd->chatID || battle_config.chat_warpportal)
					&& sd->ud.to_x == unit->bl.x && sd->ud.to_y == unit->bl.y)
				{
					int x = sg->val2>>16;
					int y = sg->val2&0xffff;
					int count = sg->val1>>16;
					unsigned short m = sg->val3;

					if( --count <= 0 )
						skill_delunitgroup(sg);

					if ( map_mapindex2mapid(m) == sd->bl.m && x == sd->bl.x && y == sd->bl.y )
						working = 1;/* we break it because officials break it, lovely stuff. */

					sg->val1 = (count<<16)|working;

					if (pc_job_can_entermap((enum e_job)sd->status.class_, map_mapindex2mapid(m), sd->group_level))
						pc_setpos(sd,m,x,y,CLR_TELEPORT);
				}
			} else if(bl->type == BL_MOB && battle_config.mob_warp&2) {
				int16 m = map_mapindex2mapid(sg->val3);
				if (m < 0) break; //Map not available on this map-server.
				unit_warp(bl,m,sg->val2>>16,sg->val2&0xffff,CLR_TELEPORT);
			}
		}
			break;

		case UNT_QUAGMIRE:
			if( !sce && battle_check_target(&unit->bl,bl,sg->target_flag) > 0 )
				sc_start4(ss, bl,type,100,sg->skill_lv,sg->group_id,0,0,sg->limit);
			break;

		case UNT_VOLCANO:
		case UNT_DELUGE:
		case UNT_VIOLENTGALE:
		case UNT_FIRE_INSIGNIA:
		case UNT_WATER_INSIGNIA:
		case UNT_WIND_INSIGNIA:
		case UNT_EARTH_INSIGNIA:
			if(!sce)
				sc_start(ss, bl,type,100,sg->skill_lv,sg->limit);
			break;

		case UNT_WATER_BARRIER:
		case UNT_ZEPHYR:
		case UNT_POWER_OF_GAIA:
			if (bl->id == ss->id)
				break; // Doesn't affect the Elemental
			if (!sce)
				sc_start(ss, bl, type, 100, sg->skill_lv, sg->limit);
			break;

		case UNT_SUITON:
			if(!sce)
				sc_start4(ss, bl,type,100,sg->skill_lv,
				map_flag_vs(bl->m) || battle_check_target(&unit->bl,bl,BCT_ENEMY)>0?1:0, //Send val3 =1 to reduce agi.
				0,0,sg->limit);
			break;

		case UNT_HERMODE:
			if (sg->src_id!=bl->id && battle_check_target(&unit->bl,bl,BCT_PARTY|BCT_GUILD) > 0)
				status_change_clear_buffs(bl, SCCB_BUFFS); //Should dispell only allies.
		case UNT_RICHMANKIM:
		case UNT_ETERNALCHAOS:
		case UNT_DRUMBATTLEFIELD:
		case UNT_RINGNIBELUNGEN:
		case UNT_ROKISWEIL:
		case UNT_INTOABYSS:
		case UNT_SIEGFRIED:
			 //Needed to check when a dancer/bard leaves their ensemble area.
			if (sg->src_id==bl->id && !(sc && false && false))
				return skill_id;
			if (!sce)
				sc_start4(ss, bl,type,100,sg->skill_lv,sg->val1,sg->val2,0,sg->limit);
			break;
		case UNT_WHISTLE:
		case UNT_ASSASSINCROSS:
		case UNT_POEMBRAGI:
		case UNT_APPLEIDUN:
		case UNT_HUMMING:
		case UNT_DONTFORGETME:
		case UNT_FORTUNEKISS:
		case UNT_SERVICEFORYOU:
			if (sg->src_id==bl->id && !(sc && false && false))
				return 0;

			if (!sc) return 0;
			if (!sce)
				sc_start4(ss, bl,type,100,sg->skill_lv,sg->val1,sg->val2,0,sg->limit);
			else if (sce->val4 == 1) { //Readjust timers since the effect will not last long.
				sce->val4 = 0; //remove the mark that we stepped out
				delete_timer(sce->timer, status_change_timer);
				sce->timer = add_timer(tick+sg->limit, status_change_timer, bl->id, type); //put duration back to 3min
			}
			break;

		case UNT_FOGWALL:
			if (!sce)
			{
				sc_start4(ss, bl, type, 100, sg->skill_lv, sg->val1, sg->val2, sg->group_id, sg->limit);
				if (battle_check_target(&unit->bl,bl,BCT_ENEMY)>0)
					SkillAdditionalEffects::skill_additional_effect (ss, bl, sg->skill_id, sg->skill_lv, BF_MISC, ATK_DEF, tick);
			}
			break;

#ifndef RENEWAL
		case UNT_GRAVITATION:
			if (!sce)
				sc_start4(ss, bl,type,100,sg->skill_lv,0,BCT_ENEMY,sg->group_id,sg->limit);
			break;

		case UNT_BASILICA:
			{
				int i = battle_check_target(bl, bl, BCT_ENEMY);

				if (i > 0) {
					skill_blown(ss, bl, skill_get_blewcount(skill_id, sg->skill_lv), unit_getdir(bl), BLOWN_NONE);
					break;
				}
				if (!sce && i <= 0)
					sc_start4(ss, bl, type, 100, 0, 0, sg->group_id, ss->id, sg->limit);
			}
			break;
#endif

		

		case UNT_REVERBERATION:
			if (sg->src_id == bl->id)
				break; //Does not affect the caster.
			clif_changetraplook(&unit->bl,UNT_USED_TRAPS);
			map_foreachinrange(skill_trap_splash,&unit->bl, skill_get_splash(sg->skill_id, sg->skill_lv), sg->bl_flag, &unit->bl,tick);
			sg->limit = DIFF_TICK(tick,sg->tick) + 1500;
			sg->unit_id = UNT_USED_TRAPS;
			break;

		case UNT_FIRE_EXPANSION_SMOKE_POWDER:
			if (!sce && battle_check_target(&unit->bl, bl, sg->target_flag) > 0)
				sc_start(ss, bl, type, 100, sg->skill_lv, sg->limit);
			break;

		

		

		case UNT_KINGS_GRACE:
			if (!sce) {
				int state = 0;

				if (!map_flag_vs(ss->m) && !map_flag_gvg2(ss->m))
					state |= BCT_GUILD;
				if (battle_check_target(&unit->bl, bl, BCT_SELF|BCT_PARTY|state) > 0)
					sc_start4(ss, bl, type, 100, sg->skill_lv, 0, ss->id, 0, sg->limit);
			}
			break;

		case UNT_STEALTHFIELD:
			if( bl->id == sg->src_id )
				break; // Doesn't work on self (video shows that)
			if (!sce)
				sc_start(ss, bl,type,100,sg->skill_lv,sg->limit);
			break;

		case UNT_NEUTRALBARRIER:
			if (!sce){
				TBL_PC* sd;
				TBL_PC* tsd;
				sd = BL_CAST(BL_PC, ss);
				tsd = BL_CAST(BL_PC, bl);
				if (sd && tsd){
					if(sd->status.party_id == tsd->status.party_id){
						 status_change_start(ss, bl, type, 10000, sg->skill_lv, 0, 0, 0, sg->limit, SCSTART_NOICON);		
						//sc_start2(ss,bl,STATUS_NEUTRALBARRIER,100,sg->skill_lv,sg->group_id,skill_get_time(sg->skill_id,sg->skill_lv));
					
					}
				}
			}
			break;

		case UNT_WARMER:
			if (!sce && bl->type == BL_PC && !battle_check_undead(tstatus->race, tstatus->def_ele) && tstatus->race != RC_DEMON)
				sc_start2(ss, bl, type, 100, sg->skill_lv, ss->id, skill_get_time(sg->skill_id, sg->skill_lv));
			break;

		case UNT_CATNIPPOWDER:
			if (sg->src_id == bl->id)
				break; // Does not affect the caster or Boss.
			if (!sce && battle_check_target(&unit->bl, bl, BCT_ENEMY) > 0)
				sc_start(ss, bl, type, 100, sg->skill_lv, skill_get_time(sg->skill_id, sg->skill_lv));
			break;

		case UNT_NYANGGRASS:
			if (!sce)
				sc_start(ss, bl, type, 100, sg->skill_lv, skill_get_time(sg->skill_id, sg->skill_lv));
			break;

		case UNT_CREATINGSTAR:
			if (!sce)
				sc_start4(ss, bl, type, 100, sg->skill_lv, ss->id, unit->bl.id, 0, sg->limit);
			break;

		case UNT_GD_LEADERSHIP:
		case UNT_GD_GLORYWOUNDS:
		case UNT_GD_SOULCOLD:
		case UNT_GD_HAWKEYES:
			if ( !sce && battle_check_target(&unit->bl, bl, sg->target_flag) > 0 )
				sc_start4(ss, bl,type,100,sg->skill_lv,0,0,0,1000);
			break;
	}
	return skill_id;
}

/**
 * Process skill unit each interval (sg->interval, see interval field of skill_db.yml)
 * @param unit Skill unit
 * @param bl Valid 'target' above the unit, that has been check in skill_unit_timer_sub_onplace
 * @param tick
 */
int skill_unit_onplace_timer(struct skill_unit *unit, struct block_list *bl, t_tick tick)
{
	struct skill_unit_group *sg;
	struct block_list *ss;
	TBL_PC* tsd;
	struct status_data *tstatus;
	struct status_change *sc, *tsc;
	struct skill_unit_group_tickset *ts;
	enum sc_type type;
	uint16 skill_id;
	t_tick diff = 0;

	nullpo_ret(unit);
	nullpo_ret(bl);

	if (bl->prev == NULL || !unit->alive || status_isdead(bl))
		return 0;

	nullpo_ret(sg = unit->group);
	nullpo_ret(ss = map_id2bl(sg->src_id));

	tsd = BL_CAST(BL_PC, bl);
	tsc = status_get_sc(bl);
	sc = status_get_sc(ss);
	tstatus = status_get_status_data(bl);
	type = status_skill2sc(sg->skill_id);
	skill_id = sg->skill_id;

	std::bitset<INF2_MAX> inf2 = skill_db.find(skill_id)->inf2;

	

	if (sg->interval == -1) {
		switch (sg->unit_id) {
			case UNT_ANKLESNARE: //These happen when a trap is splash-triggered by multiple targets on the same cell.
			case UNT_FIREPILLAR_ACTIVE:
			case UNT_ELECTRICSHOCKER:
			case UNT_MANHOLE:
				return 0;
			default:
				ShowError("skill_unit_onplace_timer: interval error (unit id %x)\n", sg->unit_id);
				return 0;
		}
	}

	if ((ts = skill_unitgrouptickset_search(bl,sg,tick)))
	{	//Not all have it, eg: Traps don't have it even though they can be hit by Heaven's Drive [Skotlex]
		diff = DIFF_TICK(tick,ts->tick);
		if (diff < 0)
			return 0;
		ts->tick = tick+sg->interval;

	}

	// Wall of Thorn damaged by Fire element unit [Cydh]
	//! TODO: This check doesn't matter the location, as long as one of units touched, this check will be executed.
	

	if (bl->type == BL_SKILL && skill_get_ele(sg->skill_id, sg->skill_lv) == ELE_FIRE) {
		struct skill_unit *su = (struct skill_unit *)bl;
		if (su && su->group && su->group->unit_id == UNT_WALLOFTHORN) {
			skill_unitsetting(map_id2bl(su->group->src_id), su->group->skill_id, su->group->skill_lv, su->group->val3>>16, su->group->val3&0xffff, 1);
			su->group->limit = sg->limit = 0;
			su->group->unit_id = sg->unit_id = UNT_USED_TRAPS;
			return skill_id;
		}
	}

	switch (sg->unit_id) {
		// Units that deals simple attack
		case UNT_GRAVITATION:
		case UNT_EARTHSTRAIN:
		case UNT_FIREWALK:
		case UNT_ELECTRICWALK:
		case UNT_PSYCHIC_WAVE:
		case UNT_MAKIBISHI:
		case UNT_VENOMFOG:
		case UNT_ICEMINE:
		case UNT_FLAMECROSS:
		case UNT_HELLBURNING:
			skill_attack(skill_get_type(sg->skill_id),ss,&unit->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
			break;

		case UNT_DUMMYSKILL:
			switch (sg->skill_id) {
				
				
				default:
					skill_attack(skill_get_type(sg->skill_id),ss,&unit->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
			}
			break;

		case UNT_FIREWALL:
		case UNT_KAEN: {
			int count = 0;
			const int x = bl->x, y = bl->y;

			if (skill_id == SK_AM_WILDTHORNS && battle_check_target(ss, bl, sg->target_flag) <= 0)
				break;

			//Take into account these hit more times than the timer interval can handle.
			do
				skill_attack(BF_MAGIC,ss,&unit->bl,bl,sg->skill_id,sg->skill_lv,tick+(t_tick)count*sg->interval,0);
			while(sg->interval > 0 && --unit->val2 && x == bl->x && y == bl->y &&
				++count < SKILLUNITTIMER_INTERVAL/sg->interval && !status_isdead(bl));

			if (unit->val2 <= 0)
				skill_delunit(unit);
		}
		break;
		case UNT_SANCTUARY:
			if( battle_check_undead(tstatus->race, tstatus->def_ele) || tstatus->race==RC_DEMON )
			{ //Only damage enemies with offensive Sanctuary. [Skotlex]
				if( battle_check_target(&unit->bl,bl,BCT_ENEMY) > 0 && skill_attack(BF_MAGIC, ss, &unit->bl, bl, sg->skill_id, sg->skill_lv, tick, 0) )
					sg->val1 -= 1; // Reduce the number of targets that can still be hit
			} else {
				int heal = skill_calc_heal(ss,bl,sg->skill_id,sg->skill_lv,true);
				struct mob_data *md = BL_CAST(BL_MOB, bl);
				if (md && md->mob_id == MOBID_EMPERIUM)
					break;
				if( tstatus->hp >= tstatus->max_hp )
					break;
				if( status_isimmune(bl) )
					heal = 0;
				clif_skill_nodamage(&unit->bl, bl, SK_AL_HEAL, heal, 1);
				
				status_heal(bl, heal, 0, 0);
			}
			break;

		case UNT_EVILLAND:
			//Will heal demon and undead element monsters, but not players.
			if ((bl->type == BL_PC) || (!battle_check_undead(tstatus->race, tstatus->def_ele) && tstatus->race!=RC_DEMON))
			{	//Damage enemies
				if(battle_check_target(&unit->bl,bl,BCT_ENEMY)>0)
					skill_attack(BF_MISC, ss, &unit->bl, bl, sg->skill_id, sg->skill_lv, tick, 0);
			} else {
				int heal = skill_calc_heal(ss,bl,sg->skill_id,sg->skill_lv,true);

				if (tstatus->hp >= tstatus->max_hp)
					break;
				if (status_isimmune(bl))
					heal = 0;
				clif_skill_nodamage(&unit->bl, bl, SK_AL_HEAL, heal, 1);
				status_heal(bl, heal, 0, 0);
			}
			break;

		case UNT_MAGNUS:
#ifndef RENEWAL
			if (!battle_check_undead(tstatus->race,tstatus->def_ele) && tstatus->race!=RC_DEMON)
				break;
#endif
			skill_attack(BF_MAGIC,ss,&unit->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
			break;

		case UNT_FIREPILLAR_WAITING:
			skill_unitsetting(ss,sg->skill_id,sg->skill_lv,unit->bl.x,unit->bl.y,1);
			skill_delunit(unit);
			break;

		

		case UNT_ANKLESNARE:
		case UNT_MANHOLE:
			if( sg->val2 == 0 && tsc && ((sg->unit_id == UNT_ANKLESNARE) || bl->id != sg->src_id) ) {
				t_tick sec = skill_get_time2(sg->skill_id,sg->skill_lv);

				if( status_change_start(ss, bl,type,10000,sg->skill_lv,sg->group_id,0,0,sec, SCSTART_NORATEDEF) ) {
					const struct TimerData* td = tsc->data[type]?get_timer(tsc->data[type]->timer):NULL;

					if( td )
						sec = DIFF_TICK(td->tick, tick);
					if( (sg->unit_id == UNT_MANHOLE && bl->type == BL_PC)
						|| !unit_blown_immune(bl,0x1) )
					{
						unit_movepos(bl, unit->bl.x, unit->bl.y, 0, 0);
						clif_fixpos(bl);
					}
					sg->val2 = bl->id;
				} else
					sec = 3000; //Couldn't trap it?
				if (sg->unit_id == UNT_ANKLESNARE) {
					clif_skillunit_update(&unit->bl);
					/**
					 * If you're snared from a trap that was invisible this makes the trap be
					 * visible again -- being you stepped on it (w/o this the trap remains invisible and you go "WTF WHY I CANT MOVE")
					 * bugreport:3961
					 **/
					clif_changetraplook(&unit->bl, UNT_ANKLESNARE);
				}
				sg->limit = DIFF_TICK(tick,sg->tick)+sec;
				sg->interval = -1;
				unit->range = 0;
			}
			break;

		case UNT_EARTHQUAKE:
			sg->val1++; // Hit count
			skill_attack(skill_get_type(sg->skill_id), ss, &unit->bl, bl, sg->skill_id, sg->skill_lv, tick, map_foreachinallrange(skill_area_sub, &unit->bl, skill_get_splash(sg->skill_id, sg->skill_lv), BL_CHAR, &unit->bl, sg->skill_id, sg->skill_lv, tick, BCT_ENEMY, skill_area_sub_count) | (sg->val1 == 1 ? NPC_EARTHQUAKE_FLAG : 0));
			break;

		case UNT_ELECTRICSHOCKER:
			if( bl->id != ss->id ) {
				if( status_change_start(ss, bl,type,10000,sg->skill_lv,sg->group_id,0,0,skill_get_time2(sg->skill_id, sg->skill_lv), SCSTART_NORATEDEF) ) {
					map_moveblock(bl, unit->bl.x, unit->bl.y, tick);
					clif_fixpos(bl);
				}
				map_foreachinallrange(skill_trap_splash, &unit->bl, skill_get_splash(sg->skill_id, sg->skill_lv), sg->bl_flag, &unit->bl, tick);
				sg->unit_id = UNT_USED_TRAPS; //Changed ID so it does not invoke a for each in area again.
			}
			break;

		case UNT_VENOMDUST:
			if(tsc && !tsc->data[type])
				status_change_start(ss,bl,type,10000,sg->skill_lv,sg->src_id,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),SCSTART_NONE);
			break;

		case UNT_LANDMINE:
			//Land Mine only hits single target
			skill_attack(skill_get_type(sg->skill_id),ss,&unit->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
			sg->unit_id = UNT_USED_TRAPS; //Changed ID so it does not invoke a for each in area again.
			sg->limit = 1500;
			break;
		case UNT_MAGENTATRAP:
		case UNT_COBALTTRAP:
		case UNT_MAIZETRAP:
		case UNT_VERDURETRAP:
			if( bl->type == BL_PC )// it won't work on players
				break;
		case UNT_FIRINGTRAP:
		case UNT_ICEBOUNDTRAP:
		case UNT_CLUSTERBOMB:
			if( bl->id == ss->id )// it won't trigger on caster
				break;
		case UNT_BLASTMINE:
		case UNT_SHOCKWAVE:
		case UNT_SANDMAN:
		case UNT_FLASHER:
		case UNT_FREEZINGTRAP:
		case UNT_FIREPILLAR_ACTIVE:
		case UNT_CLAYMORETRAP:
		{
			int bl_flag = sg->bl_flag;
			
			if (sg->unit_id == UNT_FIRINGTRAP || sg->unit_id == UNT_ICEBOUNDTRAP || sg->unit_id == UNT_CLAYMORETRAP)
				bl_flag = bl_flag|BL_SKILL|~BCT_SELF;
			map_foreachinrange(skill_trap_splash, &unit->bl, skill_get_splash(sg->skill_id, sg->skill_lv), bl_flag, &unit->bl, tick);
			if (sg->unit_id != UNT_FIREPILLAR_ACTIVE)
				clif_changetraplook(&unit->bl,(sg->unit_id == UNT_LANDMINE ? UNT_FIREPILLAR_ACTIVE : UNT_USED_TRAPS));
			sg->limit = DIFF_TICK(tick, sg->tick) +
				(sg->unit_id == UNT_CLUSTERBOMB || sg->unit_id == UNT_ICEBOUNDTRAP ? 1000 : 0) + // Cluster Bomb/Icebound has 1s to disappear once activated.
				(sg->unit_id == UNT_FIRINGTRAP ? 0 : 1500); // Firing Trap gets removed immediately once activated.
			sg->unit_id = UNT_USED_TRAPS; // Change ID so it does not invoke a for each in area again.
		}
			break;

		case UNT_TALKIEBOX:
			if (sg->src_id == bl->id)
				break;
			if (sg->val2 == 0) {
				clif_talkiebox(&unit->bl, sg->valstr);
				sg->unit_id = UNT_USED_TRAPS;
				clif_changetraplook(&unit->bl, UNT_USED_TRAPS);
				sg->limit = DIFF_TICK(tick, sg->tick) + 5000;
				sg->val2 = -1;
			}
			break;

		case UNT_LULLABY:
			if (ss->id == bl->id)
				break;
			SkillAdditionalEffects::skill_additional_effect(ss, bl, sg->skill_id, sg->skill_lv, BF_LONG|BF_SKILL|BF_MISC, ATK_DEF, tick);
			break;

		case UNT_UGLYDANCE:	//Ugly Dance [Skotlex]
			if (ss->id != bl->id)
				SkillAdditionalEffects::skill_additional_effect(ss, bl, sg->skill_id, sg->skill_lv, BF_LONG|BF_SKILL|BF_MISC, ATK_DEF, tick);
			break;

		case UNT_DISSONANCE:
			skill_attack(BF_MISC, ss, &unit->bl, bl, sg->skill_id, sg->skill_lv, tick, 0);
			break;

		case UNT_APPLEIDUN: { //Apple of Idun [Skotlex]
				int heal;
#ifdef RENEWAL
				struct mob_data *md = BL_CAST(BL_MOB, bl);

				if (md && md->mob_id == MOBID_EMPERIUM)
					break;
#endif
				if (sg->src_id == bl->id && !(tsc && false ))
					break; // affects self only when soullinked
				heal = skill_calc_heal(ss,bl,sg->skill_id, sg->skill_lv, true);
				
				clif_skill_nodamage(&unit->bl, bl, SK_AL_HEAL, heal, 1);
				status_heal(bl, heal, 0, 0);
			}
			break;

		case UNT_TATAMIGAESHI:
			skill_attack(BF_WEAPON,ss,&unit->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
			break;

		case UNT_GOSPEL:
			// if (rnd() % 100 >= 50 + sg->skill_lv * 5 || ss == bl)
			// 	break;
			if (battle_check_target(ss, bl, BCT_PARTY) > 0 && ss != bl)
			{ // Support Effect only on party, not guild
				int skill_lv, healing, matk = 0;
				struct status_data *status;
				struct map_session_data *sd = BL_CAST(BL_PC, ss);
				status = status_get_status_data(&sd->bl);
				skill_lv = pc_checkskill(sd,SK_PA_GOSPEL);
				matk = rand()%(status->matk_max-status->matk_min + 1) + status->matk_min;
				healing = (200 * skill_lv) + (status_get_lv(ss) * 3) + (status_get_int(ss) * 3) + (matk * 3);
				clif_skill_nodamage(ss, bl, SK_AL_HEAL, healing, 1);
				status_heal(bl,healing,0,0);

				// clif_skill_nodamage(ss, bl, SK_AL_HEAL, heal, 1);
				// status_heal(bl, heal, 0, 0);
			}
			// else if (battle_check_target(&unit->bl, bl, BCT_ENEMY) > 0)
			// { // Offensive Effect
			// 	// int i = rnd() % 10; // Negative buff count
			// 	skill_attack(BF_MISC, ss, &unit->bl, bl, sg->skill_id, sg->skill_lv, tick, 0);
			// 	// switch (i)
			// 	// {
			// 	// 	case 0: // Deal 3000~7999 damage reduced by DEF
			// 	// 	case 1: // Deal 1500~5499 damage unreducable
			// 	// 		skill_attack(BF_MISC, ss, &unit->bl, bl, sg->skill_id, sg->skill_lv, tick, i);
			// 	// 		break;
			// 	// 	case 2: // Curse
			// 	// 		sc_start(ss, bl, STATUS_CURSE, 100, 1, 1800000); //30 minutes
			// 	// 		break;
			// 	// 	case 3: // Blind
			// 	// 		sc_start(ss, bl, STATUS_BLIND, 100, 1, 1800000); //30 minutes
			// 	// 		break;
			// 	// 	case 4: // Poison
			// 	// 		sc_start2(ss, bl, STATUS_POISON, 100, 1, ss->id, 1800000); //30 minutes
			// 	// 		break;
			// 	// 	case 5: // Level 10 Provoke
			// 	// 		clif_skill_nodamage(NULL, bl, SK_SM_PROVOKE, 10, sc_start(ss, bl, STATUS_PROVOKE, 100, 10, INFINITE_TICK)); //Infinite
			// 	// 		break;
			// 	// 	case 6: // DEF -100%
			// 	// 		sc_start(ss, bl, STATUS_INCDEFRATE, 100, -100, 20000); //20 seconds
			// 	// 		break;
			// 	// 	case 7: // ATK -100%
			// 	// 		sc_start(ss, bl, STATUS_INCATKRATE, 100, -100, 20000); //20 seconds
			// 	// 		break;
			// 	// 	case 8: // Flee -100%
			// 	// 		sc_start(ss, bl, SC_INCFLEERATE, 100, -100, 20000); //20 seconds
			// 	// 		break;
			// 	// 	case 9: // Speed/ASPD -25%
			// 	// 		sc_start4(ss, bl, STATUS_GOSPEL, 100, 1, 0, 0, BCT_ENEMY, 20000); //20 seconds
			// 	// 		break;
			// 	// }
			// }
			break;

#ifndef RENEWAL
		case UNT_BASILICA:
			{
				int i = battle_check_target(&unit->bl, bl, BCT_ENEMY);

				if (i > 0) {
					skill_blown(&unit->bl, bl, skill_get_blewcount(skill_id, sg->skill_lv), unit_getdir(bl), BLOWN_NONE);
					break;
				}
				if (i <= 0 && (!tsc || !tsc->data[SC_BASILICA]))
					sc_start4(ss, bl, type, 100, 0, 0, sg->group_id, ss->id, sg->limit);
			}
			break;
#endif

		case UNT_GROUNDDRIFT_WIND:
		case UNT_GROUNDDRIFT_DARK:
		case UNT_GROUNDDRIFT_POISON:
		case UNT_GROUNDDRIFT_WATER:
		case UNT_GROUNDDRIFT_FIRE:
			map_foreachinrange(skill_trap_splash,&unit->bl,
				skill_get_splash(sg->skill_id, sg->skill_lv), sg->bl_flag,
				&unit->bl,tick);
			sg->unit_id = UNT_USED_TRAPS;
			//clif_changetraplook(&src->bl, UNT_FIREPILLAR_ACTIVE);
			sg->limit=DIFF_TICK(tick,sg->tick);
			break;

		

		case UNT_EPICLESIS:
			++sg->val1; // Increment outside of the check to get the exact interval of the skill unit
			if( bl->type == BL_PC ) {
				if (sg->val1 % 3 == 0) { // Recover players every 3 seconds
					int heal = skill_calc_heal(ss,bl,sg->skill_id,sg->skill_lv,true);
					clif_skill_nodamage(&unit->bl, bl, SK_AL_HEAL, heal, 1);
					status_heal(bl, heal, 0, 0);
					status_change_end(bl,STATUS_HIDING,INVALID_TIMER);
					status_change_end(bl,STATUS_CLOAKING,INVALID_TIMER);
					status_change_end(bl,STATUS_CAMOUFLAGE,INVALID_TIMER);
					status_change_end(bl,STATUS_INVISIBILITY,INVALID_TIMER);
					status_change_end(bl,STATUS_STALK,INVALID_TIMER);
				}
				sc_start(ss, bl, type, 100, sg->skill_lv, sg->interval + 100);
			}
			break;

		case UNT_DIMENSIONDOOR:
			if( tsd && !map_getmapflag(bl->m, MF_NOTELEPORT) )
				pc_randomwarp(tsd,CLR_TELEPORT);
			else if( bl->type == BL_MOB && battle_config.mob_warp&8 )
				unit_warp(bl,-1,-1,-1,CLR_TELEPORT);
			break;

		case UNT_REVERBERATION:
			clif_changetraplook(&unit->bl,UNT_USED_TRAPS);
			map_foreachinrange(skill_trap_splash, &unit->bl, skill_get_splash(sg->skill_id, sg->skill_lv), sg->bl_flag, &unit->bl, tick);
			sg->limit = DIFF_TICK(tick,sg->tick) + 1000;
			sg->unit_id = UNT_USED_TRAPS;
			break;

		case UNT_SEVERE_RAINSTORM:
			if( battle_check_target(&unit->bl, bl, BCT_ENEMY) > 0 ) {
				struct map_session_data *sd = BL_CAST(BL_PC, ss);
				int weapon_flag = 0;

				if (sd && (sd->status.weapon == W_MUSICAL || sd->status.weapon == W_WHIP))
					weapon_flag = 4;
				skill_attack(BF_WEAPON,ss,&unit->bl,bl,SK_CL_SEVERERAINSTORM_MELEE,sg->skill_lv,tick,weapon_flag);
			}
			break;
		case UNT_NETHERWORLD:
			if (!status_bl_has_mode(bl,MD_STATUSIMMUNE) || (!map_flag_gvg2(ss->m) && battle_check_target(&unit->bl,bl,BCT_PARTY) < 0)) {
				if (!(tsc && tsc->data[type])) {
					sc_start(ss, bl, type, 100, sg->skill_lv, skill_get_time2(sg->skill_id,sg->skill_lv));
					sg->limit = DIFF_TICK(tick,sg->tick);
					sg->unit_id = UNT_USED_TRAPS;
				}
			}
			break;
		

		case UNT_WALLOFTHORN:
			skill_blown(&unit->bl, bl, skill_get_blewcount(sg->skill_id, sg->skill_lv), unit_getdir(bl), BLOWN_IGNORE_NO_KNOCKBACK);
			skill_addtimerskill(ss, tick + 100, bl->id, unit->bl.x, unit->bl.y, sg->skill_id, sg->skill_lv, skill_get_type(sg->skill_id), 4|SD_LEVEL);
			break;
		case UNT_DEMONSTRATION:
		case UNT_DEMONIC_FIRE:
			switch( sg->val2 ) {
				case 1:
				default:
					skill_attack(skill_get_type(skill_id), ss, &unit->bl, bl, sg->skill_id, sg->skill_lv + 10 * sg->val2, tick, 0);
					break;
			}
			break;

		case UNT_ZEPHYR:
			if (ss == bl)
				break; // Doesn't affect the Elemental
			sc_start(ss, bl, type, 100, sg->skill_lv, sg->interval);
			break;

		case UNT_CLOUD_KILL:
			if (tsc && !tsc->data[type])
				status_change_start(ss, bl, type, 10000, sg->skill_lv, ss->id, 0, 0, skill_get_time2(sg->skill_id, sg->skill_lv), SCSTART_NOTICKDEF);
			skill_attack(skill_get_type(sg->skill_id), ss, &unit->bl, bl, sg->skill_id, sg->skill_lv, tick, 0);
			break;

		

		case UNT_FIRE_MANTLE:
			if( battle_check_target(&unit->bl, bl, BCT_ENEMY) > 0 )
				skill_attack(BF_MAGIC,ss,&unit->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
			break;

		

		case UNT_LAVA_SLIDE:
			skill_attack(BF_WEAPON, ss, &unit->bl, bl, sg->skill_id, sg->skill_lv, tick, 0);
			if(++sg->val1 > 4) //after 5 stop hit and destroy me
				sg->limit = DIFF_TICK(tick, sg->tick);
			break;
		case UNT_POISON_MIST:
			skill_attack(BF_MAGIC, ss, &unit->bl, bl, sg->skill_id, sg->skill_lv, tick, 0);
			status_change_start(ss, bl, STATUS_BLIND, (10 + 10 * sg->skill_lv)*100, sg->skill_lv, sg->skill_id, 0, 0, skill_get_time2(sg->skill_id, sg->skill_lv), SCSTART_NOTICKDEF|SCSTART_NORATEDEF);
			break;

		case UNT_CHAOSPANIC:
			if (tsc && tsc->data[type])
				break;
			status_change_start(ss, bl, type, 3500 + (sg->skill_lv * 1500), sg->skill_lv, 0, 0, 1, sg->skill_lv * 4000, SCSTART_NOAVOID|SCSTART_NORATEDEF|SCSTART_NOTICKDEF);
			break;

		case UNT_B_TRAP:
			if (tsc && tsc->data[type])
				break;
			sc_start(ss, bl, type, 100, sg->skill_lv, skill_get_time2(sg->skill_id,sg->skill_lv));
			unit->val2++; // Mark as ever been used
			break;

	

	
	}

	if (bl->type == BL_MOB && ss != bl)
		mobskill_event((TBL_MOB*)bl, ss, tick, MSC_SKILLUSED|(skill_id<<16));

	return skill_id;
}

/**
 * Triggered when a char steps out of a skill unit
 * @param src Skill unit from char moved out
 * @param bl Char
 * @param tick
 */
int skill_unit_onout(struct skill_unit *src, struct block_list *bl, t_tick tick)
{
	struct skill_unit_group *sg;
	struct status_change *sc;
	struct status_change_entry *sce;
	enum sc_type type;

	nullpo_ret(src);
	nullpo_ret(bl);
	nullpo_ret(sg=src->group);
	sc = status_get_sc(bl);
	type = status_skill2sc(sg->skill_id);
	sce = (sc && type != -1)?sc->data[type]:NULL;

	if (bl->prev == NULL || (status_isdead(bl) && sg->unit_id != UNT_ANKLESNARE)) //Need to delete the trap if the source died.
		return 0;

	switch(sg->unit_id){
		case UNT_SAFETYWALL:
		case UNT_PNEUMA:
		case UNT_EPICLESIS://Arch Bishop
			if (sce)
				status_change_end(bl, type, INVALID_TIMER);
			break;

#ifndef RENEWAL
		case UNT_BASILICA:
			if (sce && sce->val4 != bl->id)
				status_change_end(bl, type, INVALID_TIMER);
			break;
#endif

		case UNT_HERMODE:	//Clear Hermode if the owner moved.
			if (sce && sce->val3 == BCT_SELF && sce->val4 == sg->group_id)
				status_change_end(bl, type, INVALID_TIMER);
			break;

		
	
	}
	return sg->skill_id;
}

/**
 * Triggered when a char steps out of a skill group (entirely) [Skotlex]
 * @param skill_id Skill ID
 * @param bl A char
 * @param tick
 */
int skill_unit_onleft(uint16 skill_id, struct block_list *bl, t_tick tick)
{
	struct status_change *sc;
	struct status_change_entry *sce;
	enum sc_type type;

	sc = status_get_sc(bl);
	if (sc && !sc->count)
		sc = NULL;

	type = status_skill2sc(skill_id);
	sce = (sc && type != -1)?sc->data[type]:NULL;

	switch (skill_id)
	{
		case SK_MG_QUAGMIRE:
			if (bl->type==BL_MOB)
				break;
			if (sce)
				status_change_end(bl, type, INVALID_TIMER);
			break;

	

		case SK_CM_MILLENIUMSHIELDS:
		case SK_MS_NEUTRALBARRIER:
			if (sce)
				status_change_end(bl, type, INVALID_TIMER);
			break;
		
		case SK_BA_ACOUSTICRYTHM:
		case SK_BA_MAGICSTRINGS:
		case SK_BA_IMPRESSIVERIFF:
			if (sce)
			{
				delete_timer(sce->timer, status_change_timer);
				//NOTE: It'd be nice if we could get the skill_lv for a more accurate extra time, but alas...
				//not possible on our current implementation.
				sce->val4 = 1; //Store the fact that this is a "reduced" duration effect.
				sce->timer = add_timer(tick+skill_get_time2(skill_id,1), status_change_timer, bl->id, type);
			}
			break;
		
		case GD_LEADERSHIP:
		case GD_GLORYWOUNDS:
		case GD_SOULCOLD:
		case GD_HAWKEYES:
			if( !(sce && sce->val4) )
				status_change_end(bl, type, INVALID_TIMER);
			break;
	}

	return skill_id;
}

/*==========================================
 * Invoked when a unit cell has been placed/removed/deleted.
 * flag values:
 * flag&1: Invoke onplace function (otherwise invoke onout)
 * flag&4: Invoke a onleft call (the unit might be scheduled for deletion)
 * flag&8: Recursive
 *------------------------------------------*/
static int skill_unit_effect(struct block_list* bl, va_list ap)
{
	struct skill_unit* unit = va_arg(ap,struct skill_unit*);
	struct skill_unit_group* group = unit->group;
	t_tick tick = va_arg(ap,t_tick);
	unsigned int flag = va_arg(ap,unsigned int);
	uint16 skill_id;
	bool dissonance = false;
	bool isTarget = false;

	if( (!unit->alive && !(flag&4)) || bl->prev == NULL )
		return 0;

	nullpo_ret(group);

	if( !(flag&8) ) {
		dissonance = skill_dance_switch(unit, 0);
		//Target-type check.
		isTarget = group->bl_flag & bl->type && battle_check_target( &unit->bl, bl, group->target_flag ) > 0;
	}

	//Necessary in case the group is deleted after calling on_place/on_out [Skotlex]
	skill_id = group->skill_id;
	if( isTarget ){
		if( flag&1 )
			skill_unit_onplace(unit,bl,tick);
		else {
			if( skill_unit_onout(unit,bl,tick) == -1 )
				return 0; // Don't let a Bard/Dancer update their own song timer
		}

		if( flag&4 )
			skill_unit_onleft(skill_id, bl, tick);
	} else if( !isTarget && flag&4 && ( group->state.song_dance&0x1 || ( group->src_id == bl->id && group->state.song_dance&0x2 ) ) )
		skill_unit_onleft(skill_id, bl, tick);//Ensemble check to terminate it.

	if( dissonance ) {
		skill_dance_switch(unit, 1);
		//we placed a dissonance, let's update
		map_foreachincell(skill_unit_effect,unit->bl.m,unit->bl.x,unit->bl.y,group->bl_flag,&unit->bl,gettick(),4|8);
	}

	return 0;
}

/**
 * Check skill unit while receiving damage
 * @param unit Skill unit
 * @param damage Received damage
 * @return Damage
 */
int64 skill_unit_ondamaged(struct skill_unit *unit, int64 damage)
{
	struct skill_unit_group *sg;

	nullpo_ret(unit);
	nullpo_ret(sg = unit->group);

	switch( sg->unit_id ) {
		case UNT_BLASTMINE:
		case UNT_SKIDTRAP:
		case UNT_LANDMINE:
		case UNT_SHOCKWAVE:
		case UNT_SANDMAN:
		case UNT_FLASHER:
		case UNT_CLAYMORETRAP:
		case UNT_FREEZINGTRAP:
		case UNT_ANKLESNARE:
		case UNT_ICEWALL:
		case UNT_WALLOFTHORN:
		case UNT_REVERBERATION:
		case UNT_NETHERWORLD:
			unit->val1 -= (int)cap_value(damage,INT_MIN,INT_MAX);
			break;
		default:
			damage = 0;
			break;
	}

	return damage;
}

/**
 * Check char condition around the skill caster
 * @param bl Char around area
 * @param *c Counter for 'valid' condition found
 * @param *p_sd Stores 'rid' of char found
 * @param skill_id Skill ID
 * @param skill_lv Level of used skill
 */
int skill_check_condition_char_sub (struct block_list *bl, va_list ap)
{
	int *c, skill_id;
	struct block_list *src;
	struct map_session_data *sd;
	struct map_session_data *tsd;
	int *p_sd;	//Contains the list of characters found.

	nullpo_ret(bl);
	nullpo_ret(tsd=(struct map_session_data*)bl);
	nullpo_ret(src=va_arg(ap,struct block_list *));
	nullpo_ret(sd=(struct map_session_data*)src);

	c=va_arg(ap,int *);
	p_sd = va_arg(ap, int *);
	skill_id = va_arg(ap,int);

	bool is_chorus = skill_get_inf2(skill_id, INF2_ISCHORUS);


	if (is_chorus) {
		if (*c == MAX_PARTY) // Check for partners for Chorus; Cap if the entire party is accounted for.
			return 0;
	}
	else if (*c >= 1) // Check for one companion for all other cases.
		return 0;

	if (bl == src)
		return 0;

	if(pc_isdead(tsd))
		return 0;

	if (tsd->sc.data[STATUS_SILENCE] || ( tsd->sc.opt1 && tsd->sc.opt1 != OPT1_BURNING ))
		return 0;

	if( is_chorus ) {
		if( tsd->status.party_id && sd->status.party_id &&
				tsd->status.party_id == sd->status.party_id &&
				(tsd->class_&MAPID_THIRDMASK) == MAPID_MINSTRELWANDERER )
			p_sd[(*c)++] = tsd->bl.id;
		return 1;
	} else {

		switch(skill_id) {
			default: //Warning: Assuming Ensemble Dance/Songs for code speed. [Skotlex]
				{
					uint16 skill_lv;
					if(pc_issit(tsd) || !unit_can_move(&tsd->bl))
						return 0;
					if (sd->status.sex != tsd->status.sex &&
							(tsd->class_&MAPID_UPPERMASK) == MAPID_BARDDANCER &&
							(skill_lv = pc_checkskill(tsd, skill_id)) > 0 &&
							(tsd->weapontype1==W_MUSICAL || tsd->weapontype1==W_WHIP) &&
							sd->status.party_id && tsd->status.party_id &&
							sd->status.party_id == tsd->status.party_id)
					{
						p_sd[(*c)++]=tsd->bl.id;
						return skill_lv;
					}
				}
				break;
		}

	}
	return 0;
}

/**
 * Checks and stores partners for ensemble skills [Skotlex]
 * Max partners is 2.
 * @param sd Caster
 * @param skill_id
 * @param skill_lv
 * @param range Area range to check
 * @param cast_flag Special handle
 */
int skill_check_pc_partner(struct map_session_data *sd, uint16 skill_id, uint16 *skill_lv, int range, int cast_flag)
{
	static int c=0;
	static int p_sd[MAX_PARTY];
	int i;
	bool is_chorus = skill_get_inf2(skill_id, INF2_ISCHORUS);

	if (!sd)
		return 0;

	if (!battle_config.player_skill_partner_check || pc_has_permission(sd, PC_PERM_SKILL_UNCONDITIONAL))
		return is_chorus ? MAX_PARTY : 99; //As if there were infinite partners.

	if (cast_flag) {	//Execute the skill on the partners.
		struct map_session_data* tsd;
		switch (skill_id) {
			default:
				if( is_chorus )
					break;//Chorus skills are not to be parsed as ensembles
				
				return c;
		}
	}

	//Else: new search for partners.
	c = 0;
	memset (p_sd, 0, sizeof(p_sd));
	i = map_foreachinallrange(skill_check_condition_char_sub, &sd->bl, range, BL_PC, &sd->bl, &c, &p_sd, skill_id);

	
	*skill_lv = (i+(*skill_lv))/(c+1); //I know c should be one, but this shows how it could be used for the average of n partners.
	return c;
}

/**
 * Sub function to count how many spawned mob is around.
 * Some skills check with matched AI.
 * @param rid Source ID
 * @param mob_class Monster ID
 * @param skill_id Used skill
 * @param *c Counter for found monster
 */
static int skill_check_condition_mob_master_sub(struct block_list *bl, va_list ap)
{
	int *c,src_id,mob_class,skill;
	uint16 ai;
	struct mob_data *md;

	md=(struct mob_data*)bl;
	src_id=va_arg(ap,int);
	mob_class=va_arg(ap,int);
	skill=va_arg(ap,int);
	c=va_arg(ap,int *);

	ai = (unsigned)(skill == SK_HT_ELEMENTALTOTEM?AI_FAW:skill == SK_HT_MYSTICTOTEM?AI_FAW:AI_FLORA);
	if( md->master_id != src_id || md->special_state.ai != ai)
		return 0; //Non alchemist summoned mobs have nothing to do here.

	if(md->mob_id==mob_class)
		(*c)++;

	return 1;
}

/**
 * Determines if a given skill should be made to consume ammo
 * when used by the player. [Skotlex]
 * @param sd Player
 * @param skill_id Skill ID
 * @return True if skill is need ammo; False otherwise.
 */
int skill_isammotype(struct map_session_data *sd, unsigned short skill_id)
{
	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);

	return (
		battle_config.arrow_decrement == 2 &&
		(sd->status.weapon == W_BOW || (sd->status.weapon >= W_REVOLVER && sd->status.weapon <= W_GRENADE)) &&
		skill->skill_type == BF_WEAPON &&
		!skill->nk[NK_NODAMAGE] &&
		!skill_get_spiritball(skill_id,1) //Assume spirit spheres are used as ammo instead.
	);
}

/**
* Check SC required to cast a skill
* @param sc
* @param skill_id
* @return True if condition is met, False otherwise
**/
static bool skill_check_condition_sc_required(struct map_session_data *sd, unsigned short skill_id, struct s_skill_condition *require) {
	if (require == nullptr || require->status.empty())
		return true;

	nullpo_ret(sd);

	status_change *sc = &sd->sc;

	if (sc == nullptr) {
		clif_skill_fail(sd, skill_id, USESKILL_FAIL_LEVEL, 0);
		return false;
	}

	// May have multiple requirements
	for (const auto &reqStatus : require->status) {
		if (reqStatus == STATUS_NONE)
			continue;

		useskill_fail_cause cause;

		switch (reqStatus) {
			// Official fail message
			case STATUS_PUSHCART:
				cause = USESKILL_FAIL_CART;
				break;
			default:
				cause = USESKILL_FAIL_LEVEL;
				break;
		}

		if (!sc->data[reqStatus]) {
			clif_skill_fail(sd, skill_id, cause, 0);
			return false;
		}
	}

	return true;
}

/** 
 * Check skill condition when cast begin
 * For ammo, only check if the skill need ammo
 * For checking ammo requirement (type and amount) will be skill_check_condition_castend
 * @param sd Player who uses skill
 * @param skill_id ID of used skill
 * @param skill_lv Level of used skill
 * @return true: All condition passed, false: Failed
 */
bool skill_check_condition_castbegin(struct map_session_data* sd, uint16 skill_id, uint16 skill_lv)
{
	struct status_data *status;
	struct status_change *sc;
	struct s_skill_condition require;
	int i;

	nullpo_retr(false,sd);

	if (sd->chatID)
		return false;

	if( pc_has_permission(sd, PC_PERM_SKILL_UNCONDITIONAL) && sd->skillitem != skill_id )
	{	//GMs don't override the skillItem check, otherwise they can use items without them being consumed! [Skotlex]
		sd->state.arrow_atk = skill_get_ammotype(skill_id)?1:0; //Need to do arrow state check.
		sd->spiritball_old = sd->spiritball; //Need to do Spiritball check.
		sd->soulball_old = sd->soulball; //Need to do Soulball check.
		return true;
	}

	switch( sd->menuskill_id ) {
		case SK_AM_PHARMACY:
			switch( skill_id ) {
				case SK_AM_PHARMACY:
				case SK_AC_MAKINGARROW:
				case BS_AXE:
				case SK_BS_REPAIRWEAPON:                        
					return false;
			}
			break;

	}
	status = &sd->battle_status;
	sc = &sd->sc;
	if( !sc->count )
		sc = NULL;

	if( sd->skillitem == skill_id )
	{
		if( sd->state.abra_flag ) // Hocus-Pocus was used. [Inkfish]
			sd->state.abra_flag = 0;
		else
		{ // When a target was selected, consume items that were skipped in pc_use_item [Skotlex]
			if( (i = sd->itemindex) == -1 ||
				sd->inventory.u.items_inventory[i].nameid != sd->itemid ||
				sd->inventory_data[i] == NULL ||
				sd->inventory_data[i]->flag.delay_consume == DELAYCONSUME_NONE ||
				sd->inventory.u.items_inventory[i].amount < 1
				)
			{	//Something went wrong, item exploit?
				sd->itemid = 0;
				sd->itemindex = -1;
				return false;
			}
			//Consume
			sd->itemid = 0;
			sd->itemindex = -1;
			if(  sd->inventory_data[i]->flag.delay_consume & DELAYCONSUME_NOCONSUME ) // [marquis007]
				; //Do not consume item.
			else if( sd->inventory.u.items_inventory[i].expire_time == 0 )
				pc_delitem(sd,i,1,0,0,LOG_TYPE_CONSUME); // Rental usable items are not consumed until expiration
		}
		if(!sd->skillitem_keep_requirement)
			return true;
	}

	if( pc_is90overweight(sd) ) {
		clif_skill_fail(sd,skill_id,USESKILL_FAIL_WEIGHTOVER,0);
		return false;
	}


	//Checks if disabling skill - in which case no SP requirements are necessary
	if( sc && skill_disable_check(sc,skill_id))
		return true;

	std::bitset<INF2_MAX> inf2 = skill_db.find(skill_id)->inf2;

	// Check the skills that can be used while mounted on a warg
	if( pc_isridingwug(sd) ) {
		if(!inf2[INF2_ALLOWONWARG])
			return false; // in official there is no message.
	}
	if( pc_ismadogear(sd) ) {
		// Skills that are unusable when Mado is equipped. [Jobbie]
		if(!inf2[INF2_ALLOWONMADO]){
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_MADOGEAR_RIDE,0);
			return false;
		}
	}

	//if (skill_lv < 1 || skill_lv > MAX_SKILL_LEVEL)
	//	return false;

	require = skill_get_requirement(sd,skill_id,skill_lv);

	//Can only update state when weapon/arrow info is checked.
	sd->state.arrow_atk = require.ammo?1:0;

	// perform skill-group checks
	if(inf2[INF2_ISCHORUS]) {
		if (skill_check_pc_partner(sd, skill_id, &skill_lv, AREA_SIZE, 0) < 1) {
		    clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
		    return false;
		}
	}
	else if(inf2[INF2_ISENSEMBLE]) {
	    if (skill_check_pc_partner(sd, skill_id, &skill_lv, 1, 0) < 1) {
		    clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
		    return false;
	    }
	}
	// perform skill-specific checks (and actions)
	switch( skill_id ) {
		
		case SK_PF_SPELLFIST:
			if(sd->skill_id_old != SK_MG_FIREBOLT && sd->skill_id_old != SK_MG_COLDBOLT && sd->skill_id_old != SK_MG_LIGHTNINGBOLT
			 && sd->skill_id_old != SK_MG_EARTHBOLT && sd->skill_id_old != SK_MG_SOULSTRIKE && sd->skill_id_old != SK_MG_DARKSTRIKE) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
		case SK_MG_CASTCANCEL:
			if(sd->ud.skilltimer == INVALID_TIMER) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
			break;
		case SK_AS_CLOAKING:
		{
			if( skill_lv < 3 && ((sd->bl.type == BL_PC && battle_config.pc_cloak_check_type&1)
			||	(sd->bl.type != BL_PC && battle_config.monster_cloak_check_type&1) )) { //Check for walls.
				static int dx[] = { 0, 1, 0, -1, -1,  1, 1, -1};
				static int dy[] = {-1, 0, 1,  0, -1, -1, 1,  1};
				int di;
				ARR_FIND( 0, 8, di, map_getcell(sd->bl.m, sd->bl.x+dx[di], sd->bl.y+dy[di], CELL_CHKNOPASS) != 0 );
				if( di == 8 ) {
					clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
					return false;
				}
			}
			break;
		}
		
		case SK_MO_TRHOWSPIRITSPHERE:
			if( sd->spiritball > 0 && sd->spiritball < require.spiritball )
				sd->spiritball_old = require.spiritball = sd->spiritball;
			else
				sd->spiritball_old = require.spiritball;
			break;
		case SK_SH_ASURASTRIKE:
			if( sc && (sc->data[STATUS_GRAPPLE] ) )
				break;
			if( !unit_can_move(&sd->bl) ) { //Placed here as ST_MOVE_ENABLE should not apply if rooted or on a combo. [Skotlex]
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}

			sd->spiritball_old = sd->spiritball;
			break;
		
		case SK_EX_ENCHANTDEADLYPOISON:

			if (sd->weapontype1 == W_FIST && battle_config.switch_remove_edp&2) {

				clif_skill_fail(sd,skill_id,USESKILL_FAIL_THIS_WEAPON,0);
				return false;
			}
			break;
		
		case GD_BATTLEORDER:
		case GD_REGENERATION:
		case GD_RESTORE:
		case GD_CHARGESHOUT_FLAG:
		case GD_CHARGESHOUT_BEATING:
		case GD_EMERGENCY_MOVE:
			if (!map_flag_gvg2(sd->bl.m)) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
		case GD_EMERGENCYCALL:
		case GD_ITEMEMERGENCYCALL:
			// other checks were already done in skill_isNotOk()
			if (!sd->status.guild_id || (sd->state.gmaster_flag == 0 && skill_id != GD_CHARGESHOUT_BEATING))
				return false;
			break;

	
		case SK_PF_INDULGE:
			if (status->sp == status->max_sp)
				return false; //Unusable when at full SP.
			break;
		
		//case SK_SO_SUMMONELEMENTALSPHERES:
		//	if (skill_lv == 1 && sc) { // Failure only happens on level 1
		//		ARR_FIND(STATUS_SPHERE_1, STATUS_SPHERE_4 + 1, i, !sc->data[i]);

		//		if (i == STATUS_SPHERE_4 + 1) { // No more free slots
		//			clif_skill_fail(sd, skill_id, USESKILL_FAIL_SUMMON, 0);
		//			return false;
		//		}
		//	}
		//	break;
		case SK_SO_TETRAVORTEX: // bugreport:7598 moved sphere check to precast to avoid triggering cooldown per official behavior -helvetica
			 {
				int active_spheres = 0, req_spheres = 0;

				for (i = STATUS_SPHERE_1; i <= STATUS_SPHERE_5; i++) {
					if (sc && sc->data[i])
						active_spheres++;
				}

				// Cast requirement
				if (skill_id == SK_SO_TETRAVORTEX)
					req_spheres = 4;
				

				if (active_spheres < req_spheres) { // Need minimum amount of spheres
					clif_skill_fail(sd, skill_id, (false) ? USESKILL_FAIL_SUMMON_NONE : USESKILL_FAIL_LEVEL, 0);
					return false;
				}
			}
			break;
		case SK_EX_HALLUCINATIONWALK:
			if( sc && (sc->data[STATUS_HALLUCINATIONWALK]) ) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
			break;
		
		
	}

	/* check state required */
	switch (require.state) {
		case ST_HIDDEN:
			if(!pc_ishiding(sd)) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
			break;
		case ST_RIDING:
			if(!pc_isriding(sd) && !pc_isridingdragon(sd)) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
			break;
		case ST_FALCON:
			if(!pc_isfalcon(sd)) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
			break;
		case ST_CART:
			if(!pc_iscarton(sd)) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_CART,0);
				return false;
			}
			break;
		case ST_SHIELD:
			if(sd->status.shield <= 0) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
			break;
		case ST_RECOVER_WEIGHT_RATE:
#ifdef RENEWAL
			if(pc_is70overweight(sd)) {
#else
			if(pc_is50overweight(sd)) {
#endif
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
			break;
		case ST_MOVE_ENABLE:
			if (!unit_can_move(&sd->bl)) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
			break;
		case ST_WATER:
			
			if (map_getcell(sd->bl.m,sd->bl.x,sd->bl.y,CELL_CHKWATER) && !map_getcell(sd->bl.m,sd->bl.x,sd->bl.y,CELL_CHKLANDPROTECTOR))
				break;
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			return false;
		case ST_RIDINGDRAGON:
			if( !pc_isridingdragon(sd) ) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_DRAGON,0);
				return false;
			}
			break;
		case ST_WUG:
			if( !pc_iswug(sd) ) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
			break;
		case ST_RIDINGWUG:
			if( !pc_isridingwug(sd) ) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
			break;
		case ST_MADO:
			if( !pc_ismadogear(sd) ) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_MADOGEAR,0);
				return false;
			}
			break;
		case ST_ELEMENTALSPIRIT:
		case ST_ELEMENTALSPIRIT2:
			if(!sd->ed) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_EL_SUMMON,0);
				return false;
			}
			break;
		case ST_PECO:
			if(!pc_isriding(sd)) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
			break;
		
		
		
	}

	/* check the status required */
	if (!require.status.empty()) {
		switch (skill_id) {
			// Being checked later in skill_check_condition_castend()
			
			default:
				if (!skill_check_condition_sc_required(sd, skill_id, &require))
					return false;
				break;
		}
	}

	// Check for equipped item(s)
	if (!require.eqItem.empty()) {
		size_t count = require.eqItem.size();

		for (const auto &it : require.eqItem) {
			t_itemid reqeqit = it;

			if (!reqeqit)
				break; // Skill has no required item(s); get out of here
			switch(skill_id) { // Specific skills require multiple items while default will handle singular cases
				
				case SK_MS_NEUTRALBARRIER:
					if (pc_search_inventory(sd, reqeqit) == -1) {
						count--;
						if (!count) {
							clif_skill_fail(sd, skill_id, USESKILL_FAIL_NEED_EQUIPMENT, 0, require.eqItem[0]);
							return false;
						} else
							continue;
					}
					break;
				default:
					if (!pc_checkequip2(sd,reqeqit,EQI_ACC_L,EQI_MAX)) {
						clif_skill_fail(sd, skill_id, USESKILL_FAIL_NEED_EQUIPMENT, 0, reqeqit);
						return false;
					}
					break;
			}
		}
	}

	if(require.mhp > 0 && get_percentage(status->hp, status->max_hp) > require.mhp) {
		//mhp is the max-hp-requirement, that is,
		//you must have this % or less of HP to cast it.
		clif_skill_fail(sd,skill_id,USESKILL_FAIL_HP_INSUFFICIENT,0);
		return false;
	}

	if( require.weapon && !pc_check_weapontype(sd,require.weapon) ) {
		switch(skill_id) {
			case SK_RG_QUICKSHOT:
				break;
			default:
				switch((unsigned int)log2(require.weapon)) {
					case W_REVOLVER:
						clif_msg(sd, SKILL_NEED_REVOLVER);
						break;
					case W_RIFLE:
						clif_msg(sd, SKILL_NEED_RIFLE);
						break;
					case W_GATLING:
						clif_msg(sd, SKILL_NEED_GATLING);
						break;
					case W_SHOTGUN:
						clif_msg(sd, SKILL_NEED_SHOTGUN);
						break;
					case W_GRENADE:
						clif_msg(sd, SKILL_NEED_GRENADE);
						break;
					default:
						clif_skill_fail(sd, skill_id, USESKILL_FAIL_THIS_WEAPON, 0);
						break;
				}
				return false;
		}
	}

	if( require.sp > 0 && status->sp < (unsigned int)require.sp) {
		clif_skill_fail(sd,skill_id,USESKILL_FAIL_SP_INSUFFICIENT,0);
		return false;
	}

	if( require.zeny > 0 && sd->status.zeny < require.zeny ) {
		clif_skill_fail(sd,skill_id,USESKILL_FAIL_MONEY,0);
		return false;
	}

	if (require.spiritball > 0) { // Skills that require certain types of spheres to use.
		switch (skill_id) { // Skills that require soul spheres.
			

			default: // Skills that require spirit/coin spheres.
				if (sd->spiritball < require.spiritball) {
					if ((sd->class_&MAPID_BASEMASK) == MAPID_GUNSLINGER || (sd->class_&MAPID_UPPERMASK) == MAPID_REBELLION)
						clif_skill_fail(sd, skill_id, USESKILL_FAIL_COINS, (require.spiritball == -1) ? 1 : require.spiritball);
					else
						clif_skill_fail(sd, skill_id, USESKILL_FAIL_SPIRITS, (require.spiritball == -1) ? 1 : require.spiritball);
					return false;
				}
				break;
		}
	}

	return true;
}

/**
 * Check skill condition when cast end.
 * Checking ammo requirement (type and amount) will be here, not at skill_check_condition_castbegin
 * @param sd Player who uses skill
 * @param skill_id ID of used skill
 * @param skill_lv Level of used skill
 * @return true: All condition passed, false: Failed
 */
bool skill_check_condition_castend(struct map_session_data* sd, uint16 skill_id, uint16 skill_lv)
{
	struct s_skill_condition require;
	struct status_data *status;
	int i;
	short index[MAX_SKILL_ITEM_REQUIRE];

	nullpo_retr(false,sd);

	if( sd->chatID )
		return false;

	if( pc_has_permission(sd, PC_PERM_SKILL_UNCONDITIONAL) && sd->skillitem != skill_id ) {
		//GMs don't override the skillItem check, otherwise they can use items without them being consumed! [Skotlex]
		sd->state.arrow_atk = skill_get_ammotype(skill_id)?1:0; //Need to do arrow state check.
		sd->spiritball_old = sd->spiritball; //Need to do Spiritball check.
		sd->soulball_old = sd->soulball; //Need to do Soulball check.
		return true;
	}

	switch( sd->menuskill_id ) { // Cast start or cast end??
		case SK_AM_PHARMACY:
			switch( skill_id ) {
				case SK_AM_PHARMACY:
				case SK_AC_MAKINGARROW:
				case BS_AXE:
				case SK_BS_REPAIRWEAPON:

					return false;
			}
			break;
		
	}

	if( sd->skillitem == skill_id && !sd->skillitem_keep_requirement ) // Casting finished (Item skill or Hocus-Pocus)
		return true;

	if( pc_is90overweight(sd) ) {
		clif_skill_fail(sd,skill_id,USESKILL_FAIL_WEIGHTOVER,0);
		return false;
	}

	// perform skill-specific checks (and actions)
	switch( skill_id ) {
		
		case SK_HT_MYSTICTOTEM:
		case SK_HT_ELEMENTALTOTEM: {
				int c = 0;
				int maxcount = skill_get_maxcount(skill_id,skill_lv);
				int mob_class = (skill_id == SK_HT_ELEMENTALTOTEM )? MOBID_MAGICDECOY_FIRE : MOBID_SILVERSNIPER;
				if (skill_id == SK_HT_MYSTICTOTEM) {
					mob_class == MOBID_LICHTERN_B;
				}
				if( battle_config.land_skill_limit && maxcount > 0 && ( battle_config.land_skill_limit&BL_PC ) ) {
					if( skill_id == SK_HT_ELEMENTALTOTEM) {
						int j;
						for( j = mob_class; j <= MOBID_MAGICDECOY_WIND; j++ )
							map_foreachinmap(skill_check_condition_mob_master_sub, sd->bl.m, BL_MOB, sd->bl.id, j, skill_id, &c);
						if( c >= maxcount ) {
							clif_skill_fail(sd , skill_id, USESKILL_FAIL_LEVEL, 0);
							return false;
						}
					} 

					if( skill_id == SK_HT_MYSTICTOTEM) {
						int j;
						for( j = mob_class; j <= MOBID_LICHTERN_G; j++ )
							map_foreachinmap(skill_check_condition_mob_master_sub, sd->bl.m, BL_MOB, sd->bl.id, j, skill_id, &c);
						if( c >= maxcount ) {
							clif_skill_fail(sd , skill_id, USESKILL_FAIL_LEVEL, 0);
							return false;
						}
					} 
					map_foreachinmap(skill_check_condition_mob_master_sub, sd->bl.m, BL_MOB, sd->bl.id, mob_class, skill_id, &c);
					
				}
			}
			break;
		
	}

	status = &sd->battle_status;

	require = skill_get_requirement(sd,skill_id,skill_lv);

	if( require.hp > 0 && status->hp <= (unsigned int)require.hp) {
		clif_skill_fail(sd,skill_id,USESKILL_FAIL_HP_INSUFFICIENT,0);
		return false;
	}

	if( require.weapon && !pc_check_weapontype(sd,require.weapon) ) {
		clif_skill_fail(sd,skill_id,USESKILL_FAIL_THIS_WEAPON,0);
		return false;
	}

	if( require.ammo ) { //Skill requires stuff equipped in the ammo slot.
		uint8 extra_ammo = 0;

#ifdef RENEWAL
		switch(skill_id) { // 2016-10-26 kRO update made these skills require an extra ammo to cast
			case SK_CL_SEVERERAINSTORM:
				extra_ammo = 1;
				break;
			default:
				break;
		}
#endif
		if((i=sd->equip_index[EQI_AMMO]) < 0 || !sd->inventory_data[i] ) {
			clif_arrow_fail(sd,0);
			return false;
		} else if( sd->inventory.u.items_inventory[i].amount < require.ammo_qty + extra_ammo ) {
			char e_msg[100];
			if (require.ammo&(1<<AMMO_BULLET|1<<AMMO_GRENADE|1<<AMMO_SHELL)) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_NEED_MORE_BULLET,0);
				return false;
			}
			else if (require.ammo&(1<<AMMO_KUNAI)) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_NEED_EQUIPMENT_KUNAI,0);
				return false;
			}
			sprintf(e_msg,msg_txt(sd,381), //Skill Failed. [%s] requires %dx %s.
						skill_get_desc(skill_id),
						require.ammo_qty,
						itemdb_ename(sd->inventory.u.items_inventory[i].nameid));
			clif_messagecolor(&sd->bl,color_table[COLOR_RED],e_msg,false,SELF);
			return false;
		}
		if (!(require.ammo&1<<sd->inventory_data[i]->subtype)) { //Ammo type check. Send the "wrong weapon type" message
			//which is the closest we have to wrong ammo type. [Skotlex]
			clif_arrow_fail(sd,0); //Haplo suggested we just send the equip-arrows message instead. [Skotlex]
			//clif_skill_fail(sd,skill_id,USESKILL_FAIL_THIS_WEAPON,0);
			return false;
		}
	}

	for( i = 0; i < MAX_SKILL_ITEM_REQUIRE; ++i ) {
		if( !require.itemid[i] )
			continue;
		index[i] = pc_search_inventory(sd,require.itemid[i]);
		if( index[i] < 0 || sd->inventory.u.items_inventory[index[i]].amount < require.amount[i] ) {
			if( require.itemid[i] == ITEMID_HOLY_WATER )
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_HOLYWATER,0); //Holy water is required.
			else if( require.itemid[i] == ITEMID_RED_GEMSTONE )
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_REDJAMSTONE,0); //Red gemstone is required.
			else if( require.itemid[i] == ITEMID_BLUE_GEMSTONE )
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_BLUEJAMSTONE,0); //Blue gemstone is required.
			else if( require.itemid[i] == ITEMID_PAINT_BRUSH )
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_PAINTBRUSH,0); //Paint brush is required.
			else if( require.itemid[i] == ITEMID_ANCILLA )
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_ANCILLA,0); //Ancilla is required.
			else
				clif_skill_fail( sd, skill_id, USESKILL_FAIL_NEED_ITEM, require.amount[i], require.itemid[i] ); // [%s] required '%d' amount.
			return false;
		}
	}

	/* check the status required */
	if (!require.status.empty()) {
		switch (skill_id) {
			
			default:
				break;
		}
	}

	return true;
}

/** Consume skill requirement
 * @param sd Player who uses the skill
 * @param skill_id ID of used skill
 * @param skill_lv Level of used skill
 * @param type Consume type
 *  type&1: consume the others (before skill was used);
 *  type&2: consume items (after skill was used)
 */
void skill_consume_requirement(struct map_session_data *sd, uint16 skill_id, uint16 skill_lv, short type)
{
	struct s_skill_condition require;

	nullpo_retv(sd);

	require = skill_get_requirement(sd,skill_id,skill_lv);

	if( type&1 ) {
		switch( skill_id ) {
			
			default:
				if(sd->state.autocast)
					require.sp = 0;
			break;
		}
		if(require.hp || require.sp)
			status_zap(&sd->bl, require.hp, require.sp);

		if(require.spiritball > 0) { // Skills that require certain types of spheres to use
			switch (skill_id) { // Skills that require soul spheres.
				

				default: // Skills that require spirit/coin spheres.
					pc_delspiritball(sd, require.spiritball, 0);
					break;
			}
		}
		else if(require.spiritball == -1) {
			sd->spiritball_old = sd->spiritball;
			pc_delspiritball(sd,sd->spiritball,0);
		}

		if(require.zeny > 0)
		{
			if( sd->status.zeny < require.zeny )
				require.zeny = sd->status.zeny;
			pc_payzeny(sd,require.zeny,LOG_TYPE_CONSUME,NULL);
		}
	}

	if( type&2 ) {
		struct status_change *sc = &sd->sc;
		int n,i;

		if( !sc->count )
			sc = NULL;

		for( i = 0; i < MAX_SKILL_ITEM_REQUIRE; ++i )
		{
			if( !require.itemid[i] )
				continue;

			


			if( (n = pc_search_inventory(sd,require.itemid[i])) >= 0 )
				pc_delitem(sd,n,require.amount[i],0,1,LOG_TYPE_CONSUME);
		}
	}
}

/**
 * Get skill requirements and return the value after some additional/reduction condition (such item bonus and status change)
 * @param sd Player's that will be checked
 * @param skill_id Skill that's being used
 * @param skill_lv Skill level of used skill
 * @return s_skill_condition Struct 's_skill_condition' that store the modified skill requirements
 */
struct s_skill_condition skill_get_requirement(struct map_session_data* sd, uint16 skill_id, uint16 skill_lv)
{
	struct s_skill_condition req;
	struct status_data *status;
	struct status_change *sc;
	int i,hp_rate,sp_rate, sp_skill_rate_bonus = 100;
	bool level_dependent = false;

	memset(&req,0,sizeof(req));

	if( !sd )
		return req;

	if( sd->skillitem == skill_id && !sd->skillitem_keep_requirement )
		return req; // Item skills and Hocus-Pocus don't have requirements.[Inkfish]

	sc = &sd->sc;
	if( !sc->count )
		sc = NULL;

	//Checks if disabling skill - in which case no SP requirements are necessary
	if( sc && skill_disable_check(sc,skill_id) )
		return req;

	skill_lv = cap_value(skill_lv, 1, MAX_SKILL_LEVEL);

	status = &sd->battle_status;

	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);

	req.hp = skill->require.hp[skill_lv - 1];
	hp_rate = skill->require.hp_rate[skill_lv - 1];
	if(hp_rate > 0)
		req.hp += (status->hp * hp_rate)/100;
	else
		req.hp += (status->max_hp * (-hp_rate))/100;

	req.sp = skill->require.sp[skill_lv-1];
	if((false) && skill_id == sd->skill_id_dance)
		req.sp /= 2;
	sp_rate = skill->require.sp_rate[skill_lv-1];
	if(sp_rate > 0)
		req.sp += (status->sp * sp_rate)/100;
	else
		req.sp += (status->max_sp * (-sp_rate))/100;
	if( sd->dsprate != 100 )
		req.sp = req.sp * sd->dsprate / 100;

	for (auto &it : sd->skillusesprate) {
		if (it.id == skill_id) {
			sp_skill_rate_bonus -= it.val;
			break;
		}
	}

	for (auto &it : sd->skillusesp) {
		if (it.id == skill_id) {
			req.sp -= it.val;
			break;
		}
	}

	req.sp = cap_value(req.sp * sp_skill_rate_bonus / 100, 0, SHRT_MAX);

	if( sc ) {
		
		if( sc->data[STATUS_HINDSIGHT] )
			req.sp += req.sp / 4;
		if( sc->data[STATUS_OFFERTORIUM]  && ( skill_id == SK_AL_JGHEAL || skill_id == SK_PR_COLUSEOHEAL || skill_id == SK_PR_SUBLIMITASHEAL))
			req.sp += req.sp * sc->data[STATUS_OFFERTORIUM]->val3 / 100;
		
	}

	req.zeny = skill->require.zeny[skill_lv-1];

	

	req.spiritball = skill->require.spiritball[skill_lv-1];
	req.state = skill->require.state;

	req.mhp = skill->require.mhp[skill_lv-1];

	

	if ((pc_checkskill(sd, SK_RG_AMBIDEXTERITY)) == 5) {
		if (skill_id == sd->status.skill[sd->reproduceskill_idx].id
			|| skill_id == sd->status.skill[sd->cloneskill_idx].id
		){
			if (skill->require.weapon == 8192 || skill->require.weapon == 2048){
				req.weapon = 2048;
			}
		}
	} else {
		req.weapon = skill->require.weapon;
	}

	req.ammo_qty = skill->require.ammo_qty[skill_lv-1];
	if (req.ammo_qty)
		req.ammo = skill->require.ammo;

	if (!req.ammo && skill_id && skill_isammotype(sd, skill_id))
	{	//Assume this skill is using the weapon, therefore it requires arrows.
		req.ammo = AMMO_TYPE_ALL; //Enable use on all ammo types.
		req.ammo_qty = 1;
	}

	req.status = skill->require.status;
	req.eqItem = skill->require.eqItem;

	switch( skill_id ) {
		/* Skill level-dependent checks */

		case SK_AM_HATCHHOMUNCULUS:
		case SK_HT_WARGTRAINING:
		case SK_HT_FALCONRY:
		case SK_SA_SUMMONELEMENTAL:
			req.itemid[0] = skill->require.itemid[min(skill_lv-1,MAX_SKILL_ITEM_REQUIRE-1)];
			req.amount[0] = skill->require.amount[min(skill_lv-1,MAX_SKILL_ITEM_REQUIRE-1)];
			level_dependent = true;

		/* Normal skill requirements and gemstone checks */
		default:
			for( i = 0; i < ((!level_dependent) ? MAX_SKILL_ITEM_REQUIRE : 2); i++ ) {
				// Skip this for level_dependent requirement, just looking forward for gemstone removal. Assumed if there is gemstone there.
				if (!level_dependent) {
					switch( skill_id ) {
						case SK_AM_AIDCONDENSEDPOTION:
						case SK_AM_AIDPOTION:
						case SK_CR_POTIONPITCHER:
						// case SK_CR_SWORDSTOPLOWSHARES:
						// case SK_AM_PLANTCULTIVATION:
							if (i != skill_lv%11 - 1)
								continue;
							break;
						
					}

					req.itemid[i] = skill->require.itemid[i];
					req.amount[i] = skill->require.amount[i];

					
				}

				// Check requirement for gemstone.
				if (itemdb_group_item_exists(IG_GEMSTONE, req.itemid[i])) {
					if( sd->special_state.no_gemstone == 2 ) // Remove all Magic Stone required for all skills for VIP.
						req.itemid[i] = req.amount[i] = 0;
					else {
						if( sd->special_state.no_gemstone )
						{	// All gem skills except Hocus Pocus and Ganbantein can cast for free with Mistress card -helvetica
					
		 					req.itemid[i] = req.amount[i] = 0;
							
						}
					}
				}
				// Check requirement for Magic Gear Fuel
				if (req.itemid[i] == ITEMID_MAGIC_GEAR_FUEL) {
					if (sd->special_state.no_mado_fuel)
					{
						req.itemid[i] = req.amount[i] = 0;
					}
				}
			}
			break;
	}

	

	//Check if player is using the copied skill [Cydh]
	uint16 idx = skill_get_index(skill_id);

	if (sd->status.skill[idx].flag == SKILL_FLAG_PLAGIARIZED) {
		uint16 req_opt = skill->copyable.req_opt;

		if (req_opt & SKILL_REQ_HPCOST)
			req.hp = 0;
		if (req_opt & SKILL_REQ_MAXHPTRIGGER)
			req.mhp = 0;
		if (req_opt & SKILL_REQ_SPCOST)
			req.sp = 0;
		if (req_opt & SKILL_REQ_HPRATECOST)
			req.hp_rate = 0;
		if (req_opt & SKILL_REQ_SPRATECOST)
			req.sp_rate = 0;
		if (req_opt & SKILL_REQ_ZENYCOST)
			req.zeny = 0;
		if (req_opt & SKILL_REQ_WEAPON)
			req.weapon = 0;
		if (req_opt & SKILL_REQ_AMMO) {
			req.ammo = 0;
			req.ammo_qty = 0;
		}
		if (req_opt & SKILL_REQ_STATE)
			req.state = ST_NONE;
		if (req_opt & SKILL_REQ_STATUS) {
			req.status.clear();
			req.status.shrink_to_fit();
		}
		if (req_opt & SKILL_REQ_SPIRITSPHERECOST)
			req.spiritball = 0;
		if (req_opt & SKILL_REQ_ITEMCOST) {
			memset(req.itemid, 0, sizeof(req.itemid));
			memset(req.amount, 0, sizeof(req.amount));
		}
		if (req_opt & SKILL_REQ_EQUIPMENT) {
			req.eqItem.clear();
			req.eqItem.shrink_to_fit();
		}
	}

	return req;
}

/*==========================================
 * Does cast-time reductions based on dex, item bonuses and config setting
 *------------------------------------------*/
int skill_castfix(struct block_list *bl, uint16 skill_id, uint16 skill_lv) {
	double time = skill_get_cast(skill_id, skill_lv);

	nullpo_ret(bl);

	// config cast time multiplier
	if (battle_config.cast_rate != 100)
		time = time * battle_config.cast_rate / 100;
	// return final cast time
	time = max(time, 0);
	//ShowInfo("Castime castfix = %f\n",time);

	return (int)time;
}

/**
 * Get the skill cast time for RENEWAL_CAST.
 * FixedRate reduction never be stacked, always get the HIGHEST VALUE TO REDUCE (-20% vs 10%, -20% wins!)
 * Additive value:
 *    Variable CastTime : time  += value
 *    Fixed CastTime    : fixed += value
 * Multipicative value
 *    Variable CastTime : VARCAST_REDUCTION(value)
 *    Fixed CastTime    : FIXEDCASTRATE2(value)
 * @param bl: The caster
 * @param time: Cast time without reduction
 * @param skill_id: Skill ID of the casted skill
 * @param skill_lv: Skill level of the casted skill
 * @return time: Modified castime after status and bonus addition or reduction
 */
int skill_vfcastfix(struct block_list *bl, double time, uint16 skill_id, uint16 skill_lv)
{
	nullpo_ret(bl);

	if (time < 0)
		return 0;

	if (bl->type == BL_MOB || bl->type == BL_NPC)
		return (int)time;

	status_change *sc = status_get_sc(bl);
	map_session_data *sd = BL_CAST(BL_PC, bl);
	int fixed = skill_get_fixed_cast(skill_id, skill_lv), fixcast_r = 0, varcast_r = 0, reduce_cast_rate = 0;
	uint8 flag = skill_get_castnodex(skill_id);

	if (fixed < 0) {
		if (battle_config.default_fixed_castrate > 0) {
			fixed = (int)time * battle_config.default_fixed_castrate / 100; // fixed time
			time = time * (100 - battle_config.default_fixed_castrate) / 100; // variable time
		} else
			fixed = 0;
	}

	// Additive Variable Cast bonus adjustments by items
	if (sd && !(flag&4)) {
		if (sd->bonus.varcastrate != 0)
			reduce_cast_rate += sd->bonus.varcastrate; // bonus bVariableCastrate
		if (sd->bonus.fixcastrate != 0)
			fixcast_r -= sd->bonus.fixcastrate; // bonus bFixedCastrate
		if (sd->bonus.add_varcast != 0)
			time += sd->bonus.add_varcast; // bonus bVariableCast
		if (sd->bonus.add_fixcast != 0)
			fixed += sd->bonus.add_fixcast; // bonus bFixedCast
		for (const auto &it : sd->skillfixcast) {
			if (it.id == skill_id) { // bonus2 bSkillFixedCast
				fixed += it.val;
				break;
			}
		}
		for (const auto &it : sd->skillvarcast) {
			if (it.id == skill_id) { // bonus2 bSkillVariableCast
				time += it.val;
				break;
			}
		}
		for (const auto &it : sd->skillcastrate) {
			if (it.id == skill_id) { // bonus2 bVariableCastrate
				reduce_cast_rate += it.val;
				break;
			}
		}
		for (const auto &it : sd->skillfixcastrate) {
			if (it.id == skill_id) { // bonus2 bFixedCastrate
				fixcast_r = max(fixcast_r, it.val);
				break;
			}
		}
	}

	// Adjusted by active statuses
	if (sc && sc->count && !(flag&2)) {
		// Multiplicative Variable CastTime values
		
		if (sc->data[STATUS_SUFFRAGIUM]) {
			VARCAST_REDUCTION(sc->data[STATUS_SUFFRAGIUM]->val2);

		}
		if (sc->data[STATUS_FORESIGHT]) {
			reduce_cast_rate += 50;
			if ((--sc->data[STATUS_FORESIGHT]->val2) <= 0)
				status_change_end(bl, STATUS_FORESIGHT, INVALID_TIMER);
		}
		if (sc->data[STATUS_MAGICSTRINGS])
			reduce_cast_rate += sc->data[STATUS_MAGICSTRINGS]->val3;
		if (sd && (skill_lv = pc_checkskill(sd, SK_WZ_RADIUS)))
			VARCAST_REDUCTION(skill_lv*5);
		
	}
	
	if (varcast_r < 0)
		time = time * (1 - (float)min(varcast_r, 100) / 100);

	// Apply Variable CastTime calculation by INT & DEX
	if (!(flag&1))
		time = time * (1 - sqrt(((float)(status_get_dex(bl) * 2 + status_get_int(bl)) / battle_config.vcast_stat_scale)));

	time = time * (1 - (float)min(reduce_cast_rate, 100) / 100);
	time = max(time, 0) + (1 - (float)min(fixcast_r, 100) / 100) * max(fixed, 0); //Underflow checking/capping

	return (int)time;
}


/*==========================================
 * Does delay reductions based on dex/agi, sc data, item bonuses, ...
 *------------------------------------------*/
int skill_delayfix(struct block_list *bl, uint16 skill_id, uint16 skill_lv)
{
	int delaynodex = skill_get_delaynodex(skill_id);
	int time = skill_get_delay(skill_id, skill_lv);
	struct map_session_data *sd;
	struct status_change *sc = status_get_sc(bl);

	nullpo_ret(bl);
	sd = BL_CAST(BL_PC, bl);

	

	if (bl->type&battle_config.no_skill_delay)
		return battle_config.min_skill_delay_limit;

	if (time < 0)
		time = -time + status_get_amotion(bl);	// If set to <0, add to attack motion.

	// Delay reductions
	switch (skill_id) {	//Monk combo skills have their delay reduced by agi/dex.

		case SK_SH_GUILLOTINEFISTS:
			//If delay not specified, it will be 1000 - 4*agi - 2*dex
			if (time == 0)
				time = 1000;
			time -= (4 * status_get_agi(bl) + 2 * status_get_dex(bl));
			break;
#ifndef RENEWAL
		case HP_BASILICA:
			if (sc && !sc->data[SC_BASILICA])
				time = 0; // There is no Delay on Basilica creation, only on cancel
			break;
#endif
		default:
			if (battle_config.delay_dependon_dex && !(delaynodex&1)) { // if skill delay is allowed to be reduced by dex
				int scale = battle_config.castrate_dex_scale - status_get_dex(bl);

				if (scale > 0)
					time = time * scale / battle_config.castrate_dex_scale;
				else //To be capped later to minimum.
					time = 0;
			}
			if (battle_config.delay_dependon_agi && !(delaynodex&1)) { // if skill delay is allowed to be reduced by agi
				int scale = battle_config.castrate_dex_scale - status_get_agi(bl);

				if (scale > 0)
					time = time * scale / battle_config.castrate_dex_scale;
				else //To be capped later to minimum.
					time = 0;
			}
	}


	if (!(delaynodex&2)) {
		if (sc && sc->count) {
			if (sc->data[STATUS_MAGICSTRINGS])
				time -= time * sc->data[STATUS_MAGICSTRINGS]->val3 / 100;
		}
	}

	if (!(delaynodex&4) && sd) {
		if (sd->delayrate != 100)
			time = time * sd->delayrate / 100;

		for (auto &it : sd->skilldelay) {
			if (it.id == skill_id) {
				time += it.val;
				break;
			}
		}
	}

	if (battle_config.delay_rate != 100)
		time = time * battle_config.delay_rate / 100;

	//ShowInfo("Delay delayfix = %d\n",time);

	return max(time,0);
}


/*==========================================
 * Weapon Repair [Celest/DracoRPG]
 *------------------------------------------*/
void skill_repairweapon(struct map_session_data *sd, int idx) {
	t_itemid material, materials[4] = { ITEMID_PHRACON, ITEMID_EMVERETARCON, ITEMID_STEEL, ITEMID_ORIDECON_STONE };
	struct item *item;
	struct map_session_data *target_sd;

	nullpo_retv(sd);

	if ( !( target_sd = map_id2sd(sd->menuskill_val) ) ) //Failed....
		return;

	if( idx == 0xFFFF ) // No item selected ('Cancel' clicked)
		return;
	if( idx < 0 || idx >= MAX_INVENTORY )
		return; //Invalid index??

	item = &target_sd->inventory.u.items_inventory[idx];
	if( !item->nameid || !item->attribute )
		return; //Again invalid item....

	if (itemdb_ishatched_egg(item))
		return;

	if (sd != target_sd && !battle_check_range(&sd->bl, &target_sd->bl, skill_get_range2(&sd->bl, sd->menuskill_id, sd->menuskill_val2, true))) {
		clif_item_repaireffect(sd, idx, 1);
		return;
	}

	if ( target_sd->inventory_data[idx]->type == IT_WEAPON )
		material = materials [ target_sd->inventory_data[idx]->wlv - 1 ]; // Lv1/2/3/4 weapons consume 1 Iron phracon/envertracon/Steel/Rough Oridecon
	else
		material = materials [2]; // Armors consume 1 Steel
	if ( pc_search_inventory(sd,material) < 0 ) {
		clif_skill_fail(sd,sd->menuskill_id,USESKILL_FAIL_LEVEL,0);
		return;
	}

	clif_skill_nodamage(&sd->bl,&target_sd->bl,sd->menuskill_id,1,1);

	item->attribute = 0;/* clear broken state */

	clif_equiplist(target_sd);

	pc_delitem(sd,pc_search_inventory(sd,material),1,0,0,LOG_TYPE_CONSUME);

	clif_item_repaireffect(sd,idx,0);

	if( sd != target_sd )
		clif_item_repaireffect(target_sd,idx,0);
}

/*==========================================
 * Item Appraisal
 *------------------------------------------*/
void skill_identify(struct map_session_data *sd, int idx)
{
	int flag=1;

	nullpo_retv(sd);

	sd->state.workinprogress = WIP_DISABLE_NONE;

	if(idx >= 0 && idx < MAX_INVENTORY) {
		if(sd->inventory.u.items_inventory[idx].nameid > 0 && sd->inventory.u.items_inventory[idx].identify == 0 ){
			flag=0;
			sd->inventory.u.items_inventory[idx].identify = 1;
		}
	}
	clif_item_identified(sd,idx,flag);
}

/*==========================================
 * Weapon Refine [Celest]
 *------------------------------------------*/
void skill_weaponrefine(struct map_session_data *sd, int idx)
{
	nullpo_retv(sd);

	if (idx >= 0 && idx < MAX_INVENTORY)
	{
		struct item *item;
		struct status_data* status;
		struct item_data *ditem = sd->inventory_data[idx];
		item = &sd->inventory.u.items_inventory[idx];

		if(item->nameid > 0 && (ditem->type == IT_WEAPON || ditem->type == IT_ARMOR)) {
			int i = 0, per, extra_per;
			t_itemid material[6] = { ITEMID_ELUNIUM, ITEMID_PHRACON, ITEMID_EMVERETARCON, ITEMID_ORIDECON, ITEMID_ORIDECON };
			if( ditem->flag.no_refine ) { 	// if the item isn't refinable
				clif_skill_fail(sd,sd->menuskill_id,USESKILL_FAIL_LEVEL,0);
				return;
			}
			if( item->refine >= sd->menuskill_val || item->refine >= 10 ) {
				clif_upgrademessage(sd, 2, item->nameid);
				return;
			}
			if( (i = pc_search_inventory(sd, material [ditem->wlv])) < 0 ) {
				clif_upgrademessage(sd, 3, material[ditem->wlv]);
				return;
			}
			per = status_get_refine_chance(static_cast<refine_type>(ditem->wlv), (int)item->refine, false);
			status = status_get_status_data(&sd->bl);
			extra_per = (status->dex + status->luk) * 0.1;
			if (extra_per > 25) {
				extra_per = 25;
			}
			per += extra_per;
			pc_delitem(sd, i, 1, 0, 0, LOG_TYPE_OTHER);
			if (per > rnd() % 100) {
				int ep=0;
				log_pick_pc(sd, LOG_TYPE_OTHER, -1, item);
				item->refine++;
				log_pick_pc(sd, LOG_TYPE_OTHER,  1, item);
				if(item->equip) {
					ep = item->equip;
					pc_unequipitem(sd,idx,3);
				}
				clif_delitem(sd,idx,1,3);
				clif_upgrademessage(sd, 0, item->nameid);
				clif_inventorylist(sd);
				clif_refine(sd->fd,0,idx,item->refine);
				if (ep)
					pc_equipitem(sd,idx,ep);
				clif_misceffect(&sd->bl,3);
				if(item->refine == 10 &&
					item->card[0] == CARD0_FORGE &&
					(int)MakeDWord(item->card[2],item->card[3]) == sd->status.char_id)
				{ // Fame point system [DracoRPG]
					switch(ditem->wlv){
						case 1:
							pc_addfame(sd, battle_config.fame_refine_lv1); // Success to refine to +10 a lv1 weapon you forged = +1 fame point
							break;
						case 2:
							pc_addfame(sd, battle_config.fame_refine_lv2); // Success to refine to +10 a lv2 weapon you forged = +25 fame point
							break;
						case 3:
							pc_addfame(sd, battle_config.fame_refine_lv3); // Success to refine to +10 a lv3 weapon you forged = +1000 fame point
							break;
					}
				}
			} else {
				item->refine = 0;
				if(item->equip)
					pc_unequipitem(sd,idx,3);
				clif_upgrademessage(sd, 1, item->nameid);
				clif_refine(sd->fd,1,idx,item->refine);
				pc_delitem(sd,idx,1,0,2, LOG_TYPE_OTHER);
				clif_misceffect(&sd->bl,2);
				clif_emotion(&sd->bl, ET_HUK);
			}
		}
	}
}

/*==========================================
 *
 *------------------------------------------*/
int skill_autospell(struct map_session_data *sd, uint16 skill_id)
{
	nullpo_ret(sd);

	if (skill_id == 0 || skill_get_index_(skill_id, true, __FUNCTION__, __FILE__, __LINE__) == 0 || SKILL_CHK_GUILD(skill_id))
		return 0;

	uint16 lv = pc_checkskill(sd, skill_id), skill_lv = sd->menuskill_val;
	uint16 maxlv = 1;

	if (skill_lv == 0 || lv == 0)
		return 0; // Player must learn the skill before doing auto-spell [Lance]

	
		maxlv = skill_lv / 2; // Half of Autospell's level unless player learned a lower level (capped below)


	maxlv = min(lv, maxlv);

	sc_start4(&sd->bl,&sd->bl,STATUS_AUTOSPELL,100,skill_lv,skill_id,maxlv,0,
		skill_get_time(SK_SA_AUTOSPELL,skill_lv));
	return 0;
}

/**
 * Count the number of players with Gangster Paradise, Peaceful Break, or Happy Break.
 * @param bl: Player object
 * @param ap: va_arg list
 * @return 1 if the player has learned Gangster Paradise, Peaceful Break, or Happy Break otherwise 0
 */
static int skill_sit_count(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd = (struct map_session_data*)bl;
	int flag = va_arg(ap, int);

	if (!pc_issit(sd))
		return 0;



	return 0;
}

/**
 * Triggered when a player sits down to activate bonus states.
 * @param bl: Player object
 * @param ap: va_arg list
 * @return 0
 */
static int skill_sit_in(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd = (struct map_session_data*)bl;
	int flag = va_arg(ap, int);

	if (!pc_issit(sd))
		return 0;

	

	

	return 0;
}

/**
 * Triggered when a player stands up to deactivate bonus states.
 * @param bl: Player object
 * @param ap: va_arg list
 * @return 0
 */
static int skill_sit_out(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd = (struct map_session_data*)bl;
	int flag = va_arg(ap, int), range = va_arg(ap, int);

	if (map_foreachinallrange(skill_sit_count, &sd->bl, range, BL_PC, flag) > 1)
		return 0;

	if (flag&1 && sd->state.gangsterparadise)
		sd->state.gangsterparadise = 0;
	if (flag&2 && sd->state.rest) {
		sd->state.rest = 0;
		status_calc_regen(bl, &sd->battle_status, &sd->regen);
		status_calc_regen_rate(bl, &sd->regen, &sd->sc);
	}

	return 0;
}

/**
 * Toggle Sit icon and player bonuses when sitting/standing.
 * @param sd: Player data
 * @param sitting: True when sitting or false when standing
 * @return 0
 */
int skill_sit(struct map_session_data *sd, bool sitting)
{
	int flag = 0, range = 0, lv;

	nullpo_ret(sd);

	
	

	if (sitting)
		clif_status_load(&sd->bl, EFST_SIT, 1);
	else
		clif_status_load(&sd->bl, EFST_SIT, 0);

	if (!flag) // No need to count area if no skills are learned.
		return 0;

	if (sitting) {
		if (map_foreachinallrange(skill_sit_count, &sd->bl, range, BL_PC, flag) > 1)
			map_foreachinallrange(skill_sit_in, &sd->bl, range, BL_PC, flag);
	} else
		map_foreachinallrange(skill_sit_out, &sd->bl, range, BL_PC, flag, range);

	return 0;
}

/*==========================================
 * Do Forstjoke/Scream effect
 *------------------------------------------*/
int skill_frostjoke_scream(struct block_list *bl, va_list ap)
{
	struct block_list *src;
	uint16 skill_id,skill_lv;
	t_tick tick;

	nullpo_ret(bl);
	nullpo_ret(src = va_arg(ap,struct block_list*));

	skill_id = va_arg(ap,int);
	skill_lv = va_arg(ap,int);
	if(!skill_lv)
		return 0;
	tick = va_arg(ap,t_tick);

	if (src == bl || status_isdead(bl))
		return 0;
	if (bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if ( sd && sd->sc.option&(OPTION_INVISIBLE|OPTION_MADOGEAR) )
			return 0;//Frost Joke / Scream cannot target invisible or MADO Gear characters [Ind]
	}
	//It has been reported that Scream/Joke works the same regardless of woe-setting. [Skotlex]
	if(battle_check_target(src,bl,BCT_ENEMY) > 0)
		SkillAdditionalEffects::skill_additional_effect(src,bl,skill_id,skill_lv,BF_MISC,ATK_DEF,tick);
	else if(battle_check_target(src,bl,BCT_PARTY) > 0 && rnd()%100 < 10)
		SkillAdditionalEffects::skill_additional_effect(src,bl,skill_id,skill_lv,BF_MISC,ATK_DEF,tick);

	return 0;
}

/**
 * Set map cell flag as skill unit effect
 * @param src Skill unit
 * @param skill_id
 * @param skill_lv
 * @param cell Cell type cell_t
 * @param flag 0/1
 */
static void skill_unitsetmapcell(struct skill_unit *src, uint16 skill_id, uint16 skill_lv, cell_t cell, bool flag)
{
	int range = skill_get_unit_range(skill_id,skill_lv);
	int x, y;

	for( y = src->bl.y - range; y <= src->bl.y + range; ++y )
		for( x = src->bl.x - range; x <= src->bl.x + range; ++x )
			map_setcell(src->bl.m, x, y, cell, flag);
}

/**
 * Do skill attack area (such splash effect) around the 'first' target.
 * First target will skip skill condition, always receive damage. But,
 * around it, still need target/condition validation by
 * battle_check_target and status_check_skilluse
 * @param bl
 * @param ap { atk_type, src, dsrc, skill_id, skill_lv, tick, flag, type }
 */
int skill_attack_area(struct block_list *bl, va_list ap)
{
	struct block_list *src,*dsrc;
	int atk_type,skill_id,skill_lv,flag,type;
	t_tick tick;

	if(status_isdead(bl))
		return 0;
	
	atk_type = va_arg(ap,int);
	src = va_arg(ap,struct block_list*);
	dsrc = va_arg(ap,struct block_list*);
	skill_id = va_arg(ap,int);
	skill_lv = va_arg(ap,int);
	tick = va_arg(ap,t_tick);
	flag = va_arg(ap,int);
	type = va_arg(ap,int);

	if (skill_area_temp[1] == bl->id) { //This is the target of the skill, do a full attack and skip target checks.
		switch (skill_id) {
			
			default:
				return (int)skill_attack(atk_type,src,dsrc,bl,skill_id,skill_lv,tick,flag);
		}
	}

	if(battle_check_target(dsrc,bl,type) <= 0 ||
		!status_check_skilluse(NULL, bl, skill_id, 2))
		return 0;

	switch (skill_id) {
		default:
			//Area-splash, disable skill animation.
			return (int)skill_attack(atk_type,src,dsrc,bl,skill_id,skill_lv,tick,flag|SD_ANIMATION);
	}
}

/**
 * Clear skill unit group
 * @param bl
 * @param flag &1
 */
int skill_clear_group(struct block_list *bl, int flag)
{
	struct unit_data *ud = NULL;
	struct skill_unit_group *group[MAX_SKILLUNITGROUP];
	int i, count = 0;

	nullpo_ret(bl);

	if (!(ud = unit_bl2ud(bl)))
		return 0;

	// All groups to be deleted are first stored on an array since the array elements shift around when you delete them. [Skotlex]
	for (i = 0; i < MAX_SKILLUNITGROUP && ud->skillunit[i]; i++) {
		switch (ud->skillunit[i]->skill_id) {
			
			case SK_PF_LANDPROTECTOR:
	
				if (flag&1)
					group[count++] = ud->skillunit[i];
				break;
			case SK_AS_POISONOUSCLOUD:
				if( flag&4 )
					group[count++] = ud->skillunit[i];
				break;
			
			default:
				if (flag&2 && skill_get_inf2(ud->skillunit[i]->skill_id, INF2_ISTRAP))
					group[count++] = ud->skillunit[i];
				break;
		}

	}
	for (i = 0; i < count; i++)
		skill_delunitgroup(group[i]);
	return count;
}

/**
 * Returns the first element field found [Skotlex]
 * @param bl
 * @return skill_unit_group
 */
struct skill_unit_group *skill_locate_element_field(struct block_list *bl)
{
	struct unit_data *ud = NULL;
	int i;

	nullpo_ret(bl);

	if (!(ud = unit_bl2ud(bl)))
		return NULL;

	for (i = 0; i < MAX_SKILLUNITGROUP && ud->skillunit[i]; i++) {
		switch (ud->skillunit[i]->skill_id) {
			case SK_PF_LANDPROTECTOR:
			case SK_AS_POISONOUSCLOUD:
				return ud->skillunit[i];
		}
	}
	return NULL;
}

/// Graffiti cleaner [Valaris]
int skill_graffitiremover(struct block_list *bl, va_list ap)
{
	struct skill_unit *unit = NULL;
	int remove = va_arg(ap, int);

	nullpo_retr(0, bl);

	if (bl->type != BL_SKILL || (unit = (struct skill_unit *)bl) == NULL)
		return 0;

	if ((unit->group) && (unit->group->unit_id == UNT_GRAFFITI)) {
		if (remove == 1)
			skill_delunit(unit);
		return 1;
	}

	return 0;
}

/// Greed effect
int skill_greed(struct block_list *bl, va_list ap)
{
	struct block_list *src;
	struct map_session_data *sd = NULL;
	struct flooritem_data *fitem = NULL;

	nullpo_ret(bl);
	nullpo_ret(src = va_arg(ap, struct block_list *));

	if(src->type == BL_PC && (sd = (struct map_session_data *)src) && bl->type == BL_ITEM && (fitem = (struct flooritem_data *)bl))
		pc_takeitem(sd, fitem);

	return 0;
}

/// Ranger's Detonator [Jobbie/3CeAM]
int skill_detonator(struct block_list *bl, va_list ap)
{
	nullpo_ret(bl);

	if (bl->type != BL_SKILL)
		return 0;

	block_list *src = va_arg(ap, block_list *);
	skill_unit *unit = (skill_unit *)bl;

	if (unit == nullptr)
		return 0;

	skill_unit_group *group = unit->group;

	if (group == nullptr || group->src_id != src->id)
		return 0;

	int unit_id = group->unit_id;

	switch( unit_id )
	{ //List of Hunter and Ranger Traps that can be detonate.
		case UNT_BLASTMINE:
		case UNT_SANDMAN:
		case UNT_CLAYMORETRAP:
		case UNT_TALKIEBOX:
		case UNT_CLUSTERBOMB:
		case UNT_FIRINGTRAP:
		case UNT_ICEBOUNDTRAP:
			switch(unit_id) {
				case UNT_TALKIEBOX:
					clif_talkiebox(bl,group->valstr);
					group->val2 = -1;
					break;
				case UNT_CLAYMORETRAP:
				case UNT_FIRINGTRAP:
				case UNT_ICEBOUNDTRAP:
					map_foreachinrange(skill_trap_splash,bl,skill_get_splash(group->skill_id,group->skill_lv),group->bl_flag|BL_SKILL|~BCT_SELF,bl,group->tick);
					break;
				default:
					map_foreachinrange(skill_trap_splash,bl,skill_get_splash(group->skill_id,group->skill_lv),group->bl_flag,bl,group->tick);
					break;
			}
			if (unit->group == nullptr)
				return 0;
			clif_changetraplook(bl, UNT_USED_TRAPS);
			group->unit_id = UNT_USED_TRAPS;
			group->limit = DIFF_TICK(gettick(),group->tick) +
				(unit_id == UNT_TALKIEBOX ? 5000 : (unit_id == UNT_CLUSTERBOMB || unit_id == UNT_ICEBOUNDTRAP? 2500 : (unit_id == UNT_FIRINGTRAP ? 0 : 1500)) );
			break;
	}
	return 0;
}

/**
 * Calculate Royal Guard's Banding bonus
 * @param sd: Player data
 * @return Number of Royal Guard
 */
int skill_banding_count(struct map_session_data *sd)
{
 //	nullpo_ret(sd);

	//int range = skill_get_splash(LG_BANDING, 1);
	//uint8 count = party_foreachsamemap(party_sub_count_banding, sd, range, 0);
	//unsigned int group_hp = party_foreachsamemap(party_sub_count_banding, sd, range, 1);

 //	// HP is set to the average HP of the Banding group
	//if (count > 1)
	//	status_set_hp(&sd->bl, group_hp / count, 0);

 //	// Royal Guard count check for Banding
	//if (sd && sd->status.party_id) {
	//	if (count > MAX_PARTY)
	//		return MAX_PARTY;
	//	else if (count > 1)
	//		return count; // Effect bonus from additional Royal Guards if not above the max
	//}
 	return 0;
}

/**
 * Rebellion's Bind Trap explosion
 * @author [Cydh]
 */
static int skill_bind_trap(struct block_list *bl, va_list ap) {
	struct skill_unit *su = NULL;
	struct block_list *src = NULL;

	nullpo_ret(bl);

	src = va_arg(ap,struct block_list *);

	if (bl->type != BL_SKILL || !(su = (struct skill_unit *)bl) || !(su->group))
		return 0;
	if (su->group->unit_id != UNT_B_TRAP || su->group->src_id != src->id)
		return 0;

	map_foreachinallrange(skill_trap_splash, bl, su->range, BL_CHAR, bl,su->group->tick);
	clif_changetraplook(bl, UNT_USED_TRAPS);
	su->group->unit_id = UNT_USED_TRAPS;
	su->group->limit = DIFF_TICK(gettick(), su->group->tick) + 500;
	return 1;
}

/*==========================================
 * Check new skill unit cell when overlapping in other skill unit cell.
 * Catched skill in cell value pushed to *unit pointer.
 * Set (*alive) to 0 will ends 'new unit' check
 *------------------------------------------*/
static int skill_cell_overlap(struct block_list *bl, va_list ap)
{
	uint16 skill_id;
	int *alive;
	struct skill_unit *unit;

	skill_id = va_arg(ap,int);
	alive = va_arg(ap,int *);
	unit = (struct skill_unit *)bl;

	if (unit == NULL || unit->group == NULL || (*alive) == 0)
		return 0;

	if (unit->group->state.guildaura) /* guild auras are not cancelled! */
		return 0;

	switch (skill_id) {
		case SK_PF_LANDPROTECTOR: {
				if( unit->group->skill_id == SK_PF_LANDPROTECTOR ) {//Check for offensive Land Protector to delete both. [Skotlex]
					(*alive) = 0;
					skill_delunit(unit);
					return 1;
				}

				std::shared_ptr<s_skill_db> skill = skill_db.find(unit->group->skill_id);

				//It deletes everything except traps and barriers
				if ((!skill->inf2[INF2_ISTRAP] && !skill->inf2[INF2_IGNORELANDPROTECTOR]) ) {
					if (skill->unit_flag[UF_RANGEDSINGLEUNIT]) {
						if (unit->val2&(1 << UF_RANGEDSINGLEUNIT))
							skill_delunitgroup(unit->group);
					} else
						skill_delunit(unit);
					return 1;
				}
			}
			break;
		case SK_CR_GEOGRAFIELD_ATK:
		case SK_CR_MANDRAKERAID_ATK:
			if (skill_get_unit_flag(unit->group->skill_id, UF_CRAZYWEEDIMMUNE))
				break;
		
		case SK_MG_ICEWALL:

			//These can't be placed on top of themselves (duration can't be refreshed)
			if (unit->group->skill_id == skill_id)
			{
				(*alive) = 0;
				return 1;
			}
			break;
	}

	std::bitset<INF2_MAX> inf2 = skill_db.find(skill_id)->inf2;

	if (unit->group->skill_id == SK_PF_LANDPROTECTOR && !inf2[INF2_ISTRAP] && !inf2[INF2_IGNORELANDPROTECTOR] ) { //It deletes everything except traps and barriers
		(*alive) = 0;
		return 1;
	}

	return 0;
}

/*==========================================
 * Splash effect for skill unit 'trap type'.
 * Chance triggered when damaged, timeout, or char step on it.
 *------------------------------------------*/
static int skill_trap_splash(struct block_list *bl, va_list ap)
{
	struct block_list *src = va_arg(ap,struct block_list *);
	struct skill_unit *unit = NULL;
	t_tick tick = va_arg(ap,t_tick);
	struct skill_unit_group *sg;
	struct block_list *ss; //Skill src bl

	nullpo_ret(src);

	unit = (struct skill_unit *)src;

	if (!unit || !unit->alive || bl->prev == NULL)
		return 0;

	nullpo_ret(sg = unit->group);
	nullpo_ret(ss = map_id2bl(sg->src_id));

	if (battle_check_target(src,bl,sg->target_flag) <= 0)
		return 0;

	switch (sg->unit_id) {
		case UNT_B_TRAP:
			if (battle_check_target(ss, bl, sg->target_flag&~BCT_SELF) > 0)
				skill_castend_damage_id(ss, bl, sg->skill_id, sg->skill_lv, tick, SD_ANIMATION|SD_LEVEL|SD_SPLASH|1);
			break;
		case UNT_SHOCKWAVE:
		case UNT_SANDMAN:
		case UNT_FLASHER:
			SkillAdditionalEffects::skill_additional_effect(ss,bl,sg->skill_id,sg->skill_lv,BF_MISC,ATK_DEF,tick);
			break;
		case UNT_GROUNDDRIFT_WIND:
			if(skill_attack(skill_get_type(sg->skill_id),ss,src,bl,sg->skill_id,sg->skill_lv,tick,sg->val1))
				sc_start(ss,bl,STATUS_STUN,50,sg->skill_lv,skill_get_time2(sg->skill_id, 1));
			break;
		case UNT_GROUNDDRIFT_DARK:
			if(skill_attack(skill_get_type(sg->skill_id),ss,src,bl,sg->skill_id,sg->skill_lv,tick,sg->val1))
				sc_start(ss,bl,STATUS_BLIND,50,sg->skill_lv,skill_get_time2(sg->skill_id, 2));
			break;
		case UNT_GROUNDDRIFT_POISON:
			if(skill_attack(skill_get_type(sg->skill_id),ss,src,bl,sg->skill_id,sg->skill_lv,tick,sg->val1))
				sc_start2(ss,bl,STATUS_POISON,50,sg->skill_lv,ss->id,skill_get_time2(sg->skill_id, 3));
			break;
		case UNT_GROUNDDRIFT_WATER:
			if(skill_attack(skill_get_type(sg->skill_id),ss,src,bl,sg->skill_id,sg->skill_lv,tick,sg->val1))
				sc_start(ss,bl,STATUS_FREEZE,50,sg->skill_lv,skill_get_time2(sg->skill_id, 4));
			break;
		case UNT_GROUNDDRIFT_FIRE:
			if(skill_attack(skill_get_type(sg->skill_id),ss,src,bl,sg->skill_id,sg->skill_lv,tick,sg->val1))
				skill_blown(src,bl,skill_get_blewcount(sg->skill_id,sg->skill_lv),-1,BLOWN_NONE);
			break;
		
		case UNT_MAGENTATRAP:
		case UNT_COBALTTRAP:
		case UNT_MAIZETRAP:
		case UNT_VERDURETRAP:
			if( bl->type == BL_MOB && status_get_class_(bl) != CLASS_BOSS ) {
				struct status_data *status = status_get_status_data(bl);

				status->def_ele = skill_get_ele(sg->skill_id, sg->skill_lv);
				status->ele_lv = (unsigned char)sg->skill_lv;
			}
			break;
	
		case UNT_FIRINGTRAP:
		case UNT_ICEBOUNDTRAP:
			if( src->id == bl->id ) break;
			if( bl->type == BL_SKILL ) {
				struct skill_unit *su = (struct skill_unit *)bl;

				if (su && su->group->unit_id == UNT_USED_TRAPS)
					break;
			}
		case UNT_CLUSTERBOMB:
			if( ss != bl )
				skill_attack(BF_MISC,ss,src,bl,sg->skill_id,sg->skill_lv,tick,sg->val1|SD_LEVEL);
			break;
		case UNT_CLAYMORETRAP:
			if( src->id == bl->id ) break;
			if( bl->type == BL_SKILL ) {
				struct skill_unit *su = (struct skill_unit *)bl;

				if (!su)
					return 0;

				switch(su->group->unit_id) {
					case UNT_CLAYMORETRAP:
					case UNT_LANDMINE:
					case UNT_BLASTMINE:
					case UNT_SHOCKWAVE:
					case UNT_SANDMAN:
					case UNT_FLASHER:
					case UNT_FREEZINGTRAP:
					case UNT_FIRINGTRAP:
					case UNT_ICEBOUNDTRAP:
						clif_changetraplook(bl, UNT_USED_TRAPS);
						su->group->limit = DIFF_TICK(gettick(),su->group->tick) + 1500;
						su->group->unit_id = UNT_USED_TRAPS;
						break;
				}
			}
		default: {
				int split_count = 0;

				if (skill_get_nk(sg->skill_id, NK_SPLASHSPLIT))
					split_count = max(1, map_foreachinallrange(skill_area_sub, src, skill_get_splash(sg->skill_id, sg->skill_lv), BL_CHAR, src, sg->skill_id, sg->skill_lv, tick, BCT_ENEMY, skill_area_sub_count));
				skill_attack(skill_get_type(sg->skill_id), ss, src, bl, sg->skill_id, sg->skill_lv, tick, split_count);
			}
			break;
	}
	return 1;
}

int skill_maelstrom_suction(struct block_list *bl, va_list ap)
{
	uint16 skill_id, skill_lv;
	struct skill_unit *unit;

	nullpo_ret(bl);

	skill_id = va_arg(ap,int);
	skill_lv = va_arg(ap,int);
	unit = (struct skill_unit *)bl;

	if( unit == NULL || unit->group == NULL )
		return 0;

	if( skill_get_inf2(skill_id, INF2_ISTRAP) )
		return 0;

	

	return 0;
}

/**
 * Remove current enchanted element for new element
 * @param bl Char
 * @param type New element
 */
void skill_enchant_elemental_end(struct block_list *bl, int type)
{
	struct status_change *sc;
	const enum sc_type scs[] = { STATUS_ENCHANTPOISON, STATUS_ASPERSIO };
	int i;

	nullpo_retv(bl);
	nullpo_retv(sc= status_get_sc(bl));

	if (!sc->count)
		return;

	
	for (i = 0; i < ARRAYLENGTH(scs); i++)
		if (type != scs[i] && sc->data[scs[i]])
			status_change_end(bl, scs[i], INVALID_TIMER);
}

/**
 * Check cloaking condition
 * @param bl
 * @param sce
 * @return True if near wall; False otherwise
 */
bool skill_check_cloaking(struct block_list *bl, struct status_change_entry *sce)
{
	bool wall = true;

	if( (bl->type == BL_PC && battle_config.pc_cloak_check_type&1)
	||	(bl->type != BL_PC && battle_config.monster_cloak_check_type&1) )
	{	//Check for walls.
		static int dx[] = { 0, 1, 0, -1, -1,  1, 1, -1};
		static int dy[] = {-1, 0, 1,  0, -1, -1, 1,  1};
		int i;
		ARR_FIND( 0, 8, i, map_getcell(bl->m, bl->x+dx[i], bl->y+dy[i], CELL_CHKNOPASS) != 0 );
		if( i == 8 )
			wall = false;
	}

	if( sce ) {
		if( !wall ) {
			if( sce->val1 < 3 ) //End cloaking.
				status_change_end(bl, STATUS_CLOAKING, INVALID_TIMER);
			else if( sce->val4&1 ) { //Remove wall bonus
				sce->val4&=~1;
				status_calc_bl(bl,SCB_SPEED);
			}
		} else {
			if( !(sce->val4&1) ) { //Add wall speed bonus
				sce->val4|=1;
				status_calc_bl(bl,SCB_SPEED);
			}
		}
	}

	return wall;
}

/** Check Shadow Form on the target
 * @param bl: Target
 * @param damage: Damage amount
 * @param hit
 * @return true - in Shadow Form state; false - otherwise
 */
bool skill_check_shadowform(struct block_list *bl, int64 damage, int hit)
{
	struct status_change *sc;

	nullpo_retr(false,bl);

	if (!damage)
		return false;

	sc = status_get_sc(bl);

	if( sc && sc->data[STATUS_STALK] ) {
		struct block_list *src = map_id2bl(sc->data[STATUS_STALK]->val2);

		if( !src || src->m != bl->m ) { 
			status_change_end(bl, STATUS_STALK, INVALID_TIMER);
			return false;
		}

		if( src && (status_isdead(src) || !battle_check_target(bl,src,BCT_ENEMY)) ) {
			((TBL_PC*)src)->shadowform_id = 0;
			status_change_end(bl, STATUS_STALK, INVALID_TIMER);
			return false;
		}

		status_damage(bl, src, damage, 0, clif_damage(src, src, gettick(), 500, 500, damage, hit, (hit > 1 ? DMG_MULTI_HIT : DMG_NORMAL), 0, false), 0, STATUS_STALK);
		if( sc && sc->data[STATUS_STALK] && (--sc->data[STATUS_STALK]->val3) <= 0 ) {
			status_change_end(bl, STATUS_STALK, INVALID_TIMER);
			((TBL_PC*)src)->shadowform_id = 0;
		}
		return true;
	}
	return false;
}

/**
 * Check camouflage condition
 * @param bl
 * @param sce
 * @return True if near wall; False otherwise
 * @TODO: Seems wrong
 */
bool skill_check_camouflage(struct block_list *bl, struct status_change_entry *sce)
{
	bool wall = true;

	if( bl->type == BL_PC ) { //Check for walls.
		static int dx[] = { 0, 1, 0, -1, -1,  1, 1, -1};
		static int dy[] = {-1, 0, 1,  0, -1, -1, 1,  1};
		int i;
		ARR_FIND( 0, 8, i, map_getcell(bl->m, bl->x+dx[i], bl->y+dy[i], CELL_CHKNOPASS) != 0 );
		if( i == 8 )
			wall = false;
	}

	if( sce ) {
		if( !wall && sce->val1 < 3 ) //End camouflage.
			status_change_end(bl, STATUS_CAMOUFLAGE, INVALID_TIMER);
		status_calc_bl(bl,SCB_SPEED);
	}

	return wall;
}

/**
 * Process skill unit visibilty for single BL in area
 * @param bl
 * @param ap
 * @author [Cydh]
 **/
int skill_getareachar_skillunit_visibilty_sub(struct block_list *bl, va_list ap) {
	struct skill_unit *su = NULL;
	struct block_list *src = NULL;
	unsigned int party1 = 0;
	bool visible = true;

	nullpo_ret(bl);
	nullpo_ret((su = va_arg(ap, struct skill_unit*)));
	nullpo_ret((src = va_arg(ap, struct block_list*)));
	party1 = va_arg(ap, unsigned int);

	if (src != bl) {
		unsigned int party2 = status_get_party_id(bl);
		if (!party1 || !party2 || party1 != party2)
			visible = false;
	}

	clif_getareachar_skillunit(bl, su, SELF, visible);
	return 1;
}

/**
 * Check for skill unit visibilty in area on
 * - skill first placement
 * - skill moved (knocked back, moved dance)
 * @param su Skill unit
 * @param target Affected target for this visibility @see enum send_target
 * @author [Cydh]
 **/
void skill_getareachar_skillunit_visibilty(struct skill_unit *su, enum send_target target) {
	nullpo_retv(su);

	if (!su->hidden) // It's not hidden, just do this!
		clif_getareachar_skillunit(&su->bl, su, target, true);
	else {
		struct block_list *src = battle_get_master(&su->bl);
		map_foreachinallarea(skill_getareachar_skillunit_visibilty_sub, su->bl.m, su->bl.x-AREA_SIZE, su->bl.y-AREA_SIZE,
			su->bl.x+AREA_SIZE, su->bl.y+AREA_SIZE, BL_PC, su, src, status_get_party_id(src));
	}
}

/**
 * Check for skill unit visibilty on single BL on insight/spawn action
 * @param su Skill unit
 * @param bl Block list
 * @author [Cydh]
 **/
void skill_getareachar_skillunit_visibilty_single(struct skill_unit *su, struct block_list *bl) {
	bool visible = true;
	struct block_list *src = NULL;

	nullpo_retv(bl);
	nullpo_retv(su);
	nullpo_retv((src = battle_get_master(&su->bl)));

	if (su->hidden && src != bl) {
		unsigned int party1 = status_get_party_id(src);
		unsigned int party2 = status_get_party_id(bl);
		if (!party1 || !party2 || party1 != party2)
			visible = false;
	}

	clif_getareachar_skillunit(bl, su, SELF, visible);
}

/**
 * Initialize new skill unit for skill unit group.
 * Overall, Skill Unit makes skill unit group which each group holds their cell datas (skill unit)
 * @param group Skill unit group
 * @param idx
 * @param x
 * @param y
 * @param val1
 * @param val2
 */
struct skill_unit *skill_initunit(struct skill_unit_group *group, int idx, int x, int y, int val1, int val2, bool hidden)
{
	struct skill_unit *unit;

	nullpo_retr(NULL, group);
	nullpo_retr(NULL, group->unit); // crash-protection against poor coding
	nullpo_retr(NULL, (unit = &group->unit[idx]));

	if( map_getcell(map_id2bl(group->src_id)->m, x, y, CELL_CHKMAELSTROM) )
		return unit;

	if(!unit->alive)
		group->alive_count++;

	unit->bl.id = map_get_new_object_id();
	unit->bl.type = BL_SKILL;
	unit->bl.m = group->map;
	unit->bl.x = x;
	unit->bl.y = y;
	unit->group = group;
	unit->alive = 1;
	unit->val1 = val1;
	unit->val2 = val2;
	unit->hidden = hidden;

	// Stores new skill unit
	idb_put(skillunit_db, unit->bl.id, unit);
	map_addiddb(&unit->bl);
	if(map_addblock(&unit->bl))
		return NULL;

	// Perform oninit actions
	switch (group->skill_id) {
		case SK_MG_ICEWALL:
			map_setgatcell(unit->bl.m,unit->bl.x,unit->bl.y,5);
			clif_changemapcell(0,unit->bl.m,unit->bl.x,unit->bl.y,5,AREA);
			skill_unitsetmapcell(unit,SK_MG_ICEWALL,group->skill_lv,CELL_ICEWALL,true);
			break;
		case SK_PF_LANDPROTECTOR:
			skill_unitsetmapcell(unit,SK_PF_LANDPROTECTOR,group->skill_lv,CELL_LANDPROTECTOR,true);
			break;
#ifndef RENEWAL
		case HP_BASILICA:
			skill_unitsetmapcell(unit,HP_BASILICA,group->skill_lv,CELL_BASILICA,true);
			break;
#endif
		
		default:
			if (group->state.song_dance&0x1) //Check for dissonance.
				skill_dance_overlap(unit, 1);
			break;
	}

	skill_getareachar_skillunit_visibilty(unit, AREA);
	return unit;
}

/**
 * Remove unit
 * @param unit
 */
int skill_delunit(struct skill_unit* unit)
{
	struct skill_unit_group *group;

	nullpo_ret(unit);

	if( !unit->alive )
		return 0;

	unit->alive = 0;

	nullpo_ret(group = unit->group);

	if( group->state.song_dance&0x1 ) //Cancel dissonance effect.
		skill_dance_overlap(unit, 0);

	// invoke onout event
	if( !unit->range )
		map_foreachincell(skill_unit_effect,unit->bl.m,unit->bl.x,unit->bl.y,group->bl_flag,&unit->bl,gettick(),4);

	// perform ondelete actions
	switch (group->skill_id) {
	
		case SK_MG_ICEWALL:
			map_setgatcell(unit->bl.m,unit->bl.x,unit->bl.y,unit->val2);
			clif_changemapcell(0,unit->bl.m,unit->bl.x,unit->bl.y,unit->val2,ALL_SAMEMAP); // hack to avoid clientside cell bug
			skill_unitsetmapcell(unit,SK_MG_ICEWALL,group->skill_lv,CELL_ICEWALL,false);
			break;
		case SK_PF_LANDPROTECTOR:
			skill_unitsetmapcell(unit,SK_PF_LANDPROTECTOR,group->skill_lv,CELL_LANDPROTECTOR,false);
			break;
	}

	clif_skill_delunit(unit);

	unit->group=NULL;
	map_delblock(&unit->bl); // don't free yet
	map_deliddb(&unit->bl);
	idb_remove(skillunit_db, unit->bl.id);
	if(--group->alive_count==0)
		skill_delunitgroup(group);

	return 0;
}


static DBMap* skillunit_group_db = NULL; /// Skill unit group DB. Key int group_id -> struct skill_unit_group*

/// Returns the target skill_unit_group or NULL if not found.
struct skill_unit_group* skill_id2group(int group_id) {
	return (struct skill_unit_group*)idb_get(skillunit_group_db, group_id);
}

static int skill_unit_group_newid = MAX_SKILL; /// Skill Unit Group ID

/**
 * Returns a new group_id that isn't being used in skillunit_group_db.
 * Fatal error if nothing is available.
 */
static int skill_get_new_group_id(void)
{
	if( skill_unit_group_newid >= MAX_SKILL && skill_id2group(skill_unit_group_newid) == NULL )
		return skill_unit_group_newid++;// available
	{// find next id
		int base_id = skill_unit_group_newid;
		while( base_id != ++skill_unit_group_newid )
		{
			if( skill_unit_group_newid < MAX_SKILL )
				skill_unit_group_newid = MAX_SKILL;
			if( skill_id2group(skill_unit_group_newid) == NULL )
				return skill_unit_group_newid++;// available
		}
		// full loop, nothing available
		ShowFatalError("skill_get_new_group_id: All ids are taken. Exiting...");
		exit(1);
	}
}

/**
 * Initialize skill unit group called while setting new unit (skill unit/ground skill) in skill_unitsetting()
 * @param src Object that cast the skill
 * @param count How many 'cells' used that needed. Related with skill layout
 * @param skill_id ID of used skill
 * @param skill_lv Skill level of used skill
 * @param unit_id Unit ID (see skill.hpp::e_skill_unit_id)
 * @param limit Lifetime for skill unit, uses skill_get_time(skill_id, skill_lv)
 * @param interval Time interval
 * @return skill_unit_group
 */
struct skill_unit_group* skill_initunitgroup(struct block_list* src, int count, uint16 skill_id, uint16 skill_lv, int unit_id, t_tick limit, int interval)
{
	struct unit_data* ud = unit_bl2ud( src );
	struct skill_unit_group* group;
	int i;

	if(!(skill_id && skill_lv)) return 0;

	nullpo_retr(NULL, src);
	nullpo_retr(NULL, ud);

	// Find a free spot to store the new unit group
	// TODO: Make this flexible maybe by changing this fixed array?
	ARR_FIND( 0, MAX_SKILLUNITGROUP, i, ud->skillunit[i] == NULL );
	if(i == MAX_SKILLUNITGROUP) {
		// Array is full, make room by discarding oldest group
		int j = 0;
		t_tick maxdiff = 0, tick = gettick();
		for(i = 0; i < MAX_SKILLUNITGROUP && ud->skillunit[i];i++){
			t_tick x = DIFF_TICK(tick,ud->skillunit[i]->tick);
			if(x > maxdiff){
				maxdiff = x;
				j = i;
			}
		}
		skill_delunitgroup(ud->skillunit[j]);
		// Since elements must have shifted, we use the last slot.
		i = MAX_SKILLUNITGROUP-1;
	}

	group             = ers_alloc(skill_unit_ers, struct skill_unit_group);
	group->src_id     = src->id;
	group->party_id   = status_get_party_id(src);
	group->guild_id   = status_get_guild_id(src);
	group->group_id   = skill_get_new_group_id();
	group->link_group_id = 0;
	group->unit       = (struct skill_unit *)aCalloc(count,sizeof(struct skill_unit));
	group->unit_count = count;
	group->alive_count = 0;
	group->val1       = 0;
	group->val2       = 0;
	group->val3       = 0;
	group->skill_id   = skill_id;
	group->skill_lv   = skill_lv;
	group->unit_id    = unit_id;
	group->map        = src->m;
	group->limit      = limit;
	group->interval   = interval;
	group->tick       = gettick();
	group->valstr     = NULL;

	ud->skillunit[i] = group;

	// Stores this new group to DBMap
	idb_put(skillunit_group_db, group->group_id, group);
	return group;
}

/**
 * Remove skill unit group
 * @param group
 * @param file
 * @param line
 * @param *func
 */
int skill_delunitgroup_(struct skill_unit_group *group, const char* file, int line, const char* func)
{
	struct block_list* src;
	struct unit_data *ud;
	short i, j;
	int link_group_id;

	if( group == NULL ) {
		ShowDebug("skill_delunitgroup: group is NULL (source=%s:%d, %s)! Please report this! (#3504)\n", file, line, func);
		return 0;
	}

	src = map_id2bl(group->src_id);
	ud = unit_bl2ud(src);
	if (!src || !ud) {
		ShowError("skill_delunitgroup: Group's source not found! (src_id: %d skill_id: %d)\n", group->src_id, group->skill_id);
		return 0;
	}

	if( !status_isdead(src) && ((TBL_PC*)src)->state.warping && !((TBL_PC*)src)->state.changemap ) {
		switch( group->skill_id ) {
			case SK_BA_MAGICSTRINGS:
			case SK_BA_ACOUSTICRYTHM:
			case SK_BA_IMPRESSIVERIFF:
			case SK_MS_NEUTRALBARRIER:
				skill_usave_add(((TBL_PC*)src), group->skill_id, group->skill_lv);
				break;
		}
	}

	

	// End SC from the master when the skill group is deleted
	i = STATUS_NONE;
	switch (group->unit_id) {
		case UNT_GOSPEL:	i = STATUS_GOSPEL;		break;
#ifndef RENEWAL
		case UNT_BASILICA:	i = SC_BASILICA;	break;
#endif
	}
	if (i != STATUS_NONE) {
		struct status_change *sc = status_get_sc(src);
		if (sc && sc->data[i]) {
			sc->data[i]->val3 = 0; //Remove reference to this group. [Skotlex]
			status_change_end(src, (sc_type)i, INVALID_TIMER);
		}
	}

	switch( group->skill_id ) {
		
		
		case SK_MS_NEUTRALBARRIER:
			{
				struct status_change *sc = NULL;
				if( (sc = status_get_sc(src)) != NULL ) {
					if ( sc->data[STATUS_NEUTRALBARRIER_MASTER] )
					{
						sc->data[STATUS_NEUTRALBARRIER_MASTER]->val2 = 0;
						status_change_end(src,STATUS_NEUTRALBARRIER_MASTER,INVALID_TIMER);
					}
					status_change_end(src,STATUS_NEUTRALBARRIER,INVALID_TIMER);
				}
			}
			break;
		
	}

	if (src->type==BL_PC && group->state.ammo_consume)
		battle_consume_ammo((TBL_PC*)src, group->skill_id, group->skill_lv);

	group->alive_count=0;

	// remove all unit cells
	if(group->unit != NULL)
		for( i = 0; i < group->unit_count; i++ )
			skill_delunit(&group->unit[i]);

	// clear Talkie-box string
	if( group->valstr != NULL ) {
		aFree(group->valstr);
		group->valstr = NULL;
	}

	idb_remove(skillunit_group_db, group->group_id);
	map_freeblock(&group->unit->bl); // schedules deallocation of whole array (HACK)
	group->unit = NULL;
	group->group_id = 0;
	group->unit_count = 0;

	link_group_id = group->link_group_id;
	group->link_group_id = 0;

	// locate this group, swap with the last entry and delete it
	ARR_FIND( 0, MAX_SKILLUNITGROUP, i, ud->skillunit[i] == group );
	ARR_FIND( i, MAX_SKILLUNITGROUP, j, ud->skillunit[j] == NULL );
	 j--;
	if( i < MAX_SKILLUNITGROUP ) {
		ud->skillunit[i] = ud->skillunit[j];
		ud->skillunit[j] = NULL;
		ers_free(skill_unit_ers, group);
	} else
		ShowError("skill_delunitgroup: Group not found! (src_id: %d skill_id: %d)\n", group->src_id, group->skill_id);

	if(link_group_id) {
		struct skill_unit_group* group_cur = skill_id2group(link_group_id);
		if(group_cur)
			skill_delunitgroup(group_cur);
	}

	return 1;
}

/**
 * Clear all Skill Unit Group from an Object, example usage when player logged off or dead
 * @param src
 */
void skill_clear_unitgroup(struct block_list *src)
{
	struct unit_data *ud;

	nullpo_retv(src);
	nullpo_retv((ud = unit_bl2ud(src)));

	while (ud->skillunit[0])
		skill_delunitgroup(ud->skillunit[0]);
}

/**
 * Search tickset for skill unit in skill unit group
 * @param bl Block List for skill_unit
 * @param group Skill unit group
 * @param tick
 * @return skill_unit_group_tickset if found
 */
struct skill_unit_group_tickset *skill_unitgrouptickset_search(struct block_list *bl, struct skill_unit_group *group, t_tick tick)
{
	int i, j = -1, s, id;
	struct unit_data *ud;
	struct skill_unit_group_tickset *set;

	nullpo_ret(bl);
	if (group->interval == -1)
		return NULL;

	ud = unit_bl2ud(bl);
	if (!ud)
		return NULL;

	set = ud->skillunittick;

	if (skill_get_unit_flag(group->skill_id, UF_NOOVERLAP))
		id = s = group->skill_id;
	else
		id = s = group->group_id;

	for (i=0; i<MAX_SKILLUNITGROUPTICKSET; i++) {
		int k = (i+s) % MAX_SKILLUNITGROUPTICKSET;
		if (set[k].id == id)
			return &set[k];
		else if (j==-1 && (DIFF_TICK(tick,set[k].tick)>0 || set[k].id==0))
			j=k;
	}

	if (j == -1) {
		ShowWarning ("skill_unitgrouptickset_search: tickset is full\n");
		j = id % MAX_SKILLUNITGROUPTICKSET;
	}

	set[j].id = id;
	set[j].tick = tick;
	return &set[j];
}

/*==========================================
 * Check for validity skill unit that triggered by skill_unit_timer_sub
 * And trigger skill_unit_onplace_timer for object that maybe stands there (catched object is *bl)
 *------------------------------------------*/
int skill_unit_timer_sub_onplace(struct block_list* bl, va_list ap)
{
	struct skill_unit* unit = va_arg(ap,struct skill_unit *);
	struct skill_unit_group* group = NULL;
	t_tick tick = va_arg(ap,t_tick);
	nullpo_ret(unit);

	if( !unit->alive || bl->prev == NULL ){
		return 0;		

	}

	nullpo_ret(group = unit->group);

	std::shared_ptr<s_skill_db> skill = skill_db.find(group->skill_id);

	if( !(skill->inf2[INF2_ISSONG] || skill->inf2[INF2_ISTRAP]) && !skill->inf2[INF2_IGNORELANDPROTECTOR] && group->skill_id != SK_MS_NEUTRALBARRIER && (battle_config.land_protector_behavior ? map_getcell(bl->m, bl->x, bl->y, CELL_CHKLANDPROTECTOR) : map_getcell(unit->bl.m, unit->bl.x, unit->bl.y, CELL_CHKLANDPROTECTOR)) ){
		return 0; //AoE skills are ineffective. [Skotlex]
	}

	if( (battle_check_target(&unit->bl,bl,group->target_flag) <= 0) ){
		return 0;

	}

	skill_unit_onplace_timer(unit,bl,tick);
	return 1;
}

/**
 * @see DBApply
 * Sub function of skill_unit_timer for executing each skill unit from skillunit_db
 */
static int skill_unit_timer_sub(DBKey key, DBData *data, va_list ap)
{

	struct skill_unit* unit = (struct skill_unit*)db_data2ptr(data);
	struct skill_unit_group* group = NULL;
	t_tick tick = va_arg(ap,t_tick);
	bool dissonance;
	struct block_list* bl = &unit->bl;

	nullpo_ret(unit);

	if( !unit->alive )
		return 0;

	nullpo_ret(group = unit->group);

	// Check for expiration
	if( !group->state.guildaura && (DIFF_TICK(tick,group->tick) >= group->limit || DIFF_TICK(tick,group->tick) >= unit->limit) )
	{// skill unit expired (inlined from skill_unit_onlimit())
		switch( group->unit_id ) {
			case UNT_ICEWALL:
				unit->val1 -= 50; // icewall loses 50 hp every second
				group->limit = DIFF_TICK(tick + group->interval,group->tick);
				unit->limit = DIFF_TICK(tick + group->interval,group->tick);
				if( unit->val1 <= 0 )
					skill_delunit(unit);
			break;
			case UNT_BLASTMINE:
#ifdef RENEWAL
			case UNT_CLAYMORETRAP:
#endif
			case UNT_GROUNDDRIFT_WIND:
			case UNT_GROUNDDRIFT_DARK:
			case UNT_GROUNDDRIFT_POISON:
			case UNT_GROUNDDRIFT_WATER:
			case UNT_GROUNDDRIFT_FIRE:
				group->unit_id = UNT_USED_TRAPS;
				//clif_changetraplook(bl, UNT_FIREPILLAR_ACTIVE);
				group->limit=DIFF_TICK(tick+1500,group->tick);
				unit->limit=DIFF_TICK(tick+1500,group->tick);
			break;

			case UNT_ANKLESNARE:
			case UNT_ELECTRICSHOCKER:
				if (group->val2 > 0) { //Used Trap doesn't return back to item
					skill_delunit(unit);
					break;
				}
			case UNT_SKIDTRAP:
			case UNT_LANDMINE:
			case UNT_SHOCKWAVE:
			case UNT_SANDMAN:
			case UNT_FLASHER:
			case UNT_FREEZINGTRAP:
#ifndef RENEWAL
			case UNT_CLAYMORETRAP:
#endif
			case UNT_TALKIEBOX:
			case UNT_CLUSTERBOMB:
			case UNT_MAGENTATRAP:
			case UNT_COBALTTRAP:
			case UNT_MAIZETRAP:
			case UNT_VERDURETRAP:
			case UNT_FIRINGTRAP:
			case UNT_ICEBOUNDTRAP:
			{
				struct block_list* src;
				if( unit->val1 > 0 && (src = map_id2bl(group->src_id)) != NULL && src->type == BL_PC )
				{ // revert unit back into a trap
					struct item item_tmp;
					memset(&item_tmp,0,sizeof(item_tmp));
					item_tmp.nameid = group->item_id?group->item_id:ITEMID_TRAP;
					item_tmp.identify = 1;
					map_addflooritem(&item_tmp,1,bl->m,bl->x,bl->y,0,0,0,4,0);
				}
				skill_delunit(unit);
			}
			break;

			case UNT_WARP_ACTIVE:
				// warp portal opens (morph to a UNT_WARP_WAITING cell)
				group->unit_id = skill_get_unit_id2(group->skill_id); // UNT_WARP_WAITING
				clif_changelook(&unit->bl, LOOK_BASE, group->unit_id);
				// restart timers
				group->limit = skill_get_time(group->skill_id,group->skill_lv);
				unit->limit = skill_get_time(group->skill_id,group->skill_lv);
				// apply effect to all units standing on it
				map_foreachincell(skill_unit_effect,unit->bl.m,unit->bl.x,unit->bl.y,group->bl_flag,&unit->bl,gettick(),1);
			break;

			case UNT_CALLFAMILY:
			{
				struct map_session_data *sd = NULL;
				if(group->val1) {
					sd = map_charid2sd(group->val1);
					group->val1 = 0;
					if (sd && !map_getmapflag(sd->bl.m, MF_NOWARP) && pc_job_can_entermap((enum e_job)sd->status.class_, unit->bl.m, sd->group_level))
						pc_setpos(sd,map_id2index(unit->bl.m),unit->bl.x,unit->bl.y,CLR_TELEPORT);
				}
				if(group->val2) {
					sd = map_charid2sd(group->val2);
					group->val2 = 0;
					if (sd && !map_getmapflag(sd->bl.m, MF_NOWARP) && pc_job_can_entermap((enum e_job)sd->status.class_, unit->bl.m, sd->group_level))
						pc_setpos(sd,map_id2index(unit->bl.m),unit->bl.x,unit->bl.y,CLR_TELEPORT);
				}
				skill_delunit(unit);
			}
			break;

			case UNT_REVERBERATION:
			case UNT_NETHERWORLD:
				if( unit->val1 <= 0 ) { // If it was deactivated.
					skill_delunit(unit);
					break;
				}
				clif_changetraplook(bl,UNT_USED_TRAPS);
				if (group->unit_id == UNT_REVERBERATION)
					map_foreachinrange(skill_trap_splash, bl, skill_get_splash(group->skill_id, group->skill_lv), group->bl_flag, bl, tick);
				group->limit = DIFF_TICK(tick,group->tick) + 1000;
				unit->limit = DIFF_TICK(tick,group->tick) + 1000;
				group->unit_id = UNT_USED_TRAPS;
				break;

			case UNT_FEINTBOMB: {
				struct block_list *src = map_id2bl(group->src_id);

				if (src)
					map_foreachinrange(skill_area_sub, &unit->bl, unit->range, BL_CHAR|BL_SKILL, src, group->skill_id, group->skill_lv, tick, BCT_ENEMY|SD_ANIMATION|5, skill_castend_damage_id);
				skill_delunit(unit);
			}
			break;

			case UNT_BANDING:
			{
				struct block_list *src = map_id2bl(group->src_id);
				struct status_change *sc;
				if( !src || (sc = status_get_sc(src)) == NULL || true ) {
					skill_delunit(unit);
					break;
				}
				// This unit isn't removed while SC_BANDING is active.
				group->limit = DIFF_TICK(tick+group->interval,group->tick);
				unit->limit = DIFF_TICK(tick+group->interval,group->tick);
			}
			break;

			case UNT_B_TRAP:
				{
					struct block_list* src;
					if (group->item_id && unit->val2 <= 0 && (src = map_id2bl(group->src_id)) && src->type == BL_PC) {
						struct item item_tmp;
						memset(&item_tmp, 0, sizeof(item_tmp));
						item_tmp.nameid = group->item_id;
						item_tmp.identify = 1;
						map_addflooritem(&item_tmp, 1, bl->m, bl->x, bl->y, 0, 0, 0, 4, 0);
					}
					skill_delunit(unit);
				}
				break;

			default:
				if (group->val2 == 1 && (group->skill_id == SK_WZ_METEORSTORM )) {
					// Deal damage before expiration
					break;
				}
				skill_delunit(unit);
				break;
		}
	} else {// skill unit is still active
		switch( group->unit_id ) {
			case UNT_BLASTMINE:
			case UNT_SKIDTRAP:
			case UNT_LANDMINE:
			case UNT_SHOCKWAVE:
			case UNT_SANDMAN:
			case UNT_FLASHER:
			case UNT_CLAYMORETRAP:
			case UNT_FREEZINGTRAP:
			case UNT_TALKIEBOX:
			case UNT_ANKLESNARE:
			case UNT_B_TRAP:
				if( unit->val1 <= 0 ) {
					if( group->unit_id == UNT_ANKLESNARE && group->val2 > 0 )
						skill_delunit(unit);
					else {
						clif_changetraplook(bl, group->unit_id == UNT_LANDMINE ? UNT_FIREPILLAR_ACTIVE : UNT_USED_TRAPS);
						group->limit = DIFF_TICK(tick, group->tick) + 1500;
						group->unit_id = UNT_USED_TRAPS;
					}
				}
				break;
			case UNT_REVERBERATION:
			case UNT_NETHERWORLD:
				if (unit->val1 <= 0) {
					clif_changetraplook(bl,UNT_USED_TRAPS);
					if (group->unit_id == UNT_REVERBERATION)
						map_foreachinrange(skill_trap_splash, bl, skill_get_splash(group->skill_id, group->skill_lv), group->bl_flag, bl, tick);
					group->limit = DIFF_TICK(tick,group->tick) + 1000;
					unit->limit = DIFF_TICK(tick,group->tick) + 1000;
					group->unit_id = UNT_USED_TRAPS;
				}
				break;
			case UNT_WALLOFTHORN:
				if (group->val3 < 0) { // Remove if attacked by fire element, turned to Fire Wall
					skill_delunitgroup(group);
					break;
				}
				if (unit->val1 <= 0 ) // Remove the unit only if no HP 
					skill_delunit(unit);
				break;
			case UNT_SANCTUARY:
				if (group->val1 <= 0) {
					skill_delunitgroup(group);
				}
				break;
			default:
				if (group->skill_id == SK_WZ_METEORSTORM) {
					if (group->val2 == 0 && (DIFF_TICK(tick, group->tick) >= group->limit - group->interval || DIFF_TICK(tick, group->tick) >= unit->limit - group->interval)) {
						// Unit will expire the next interval, start dropping Meteor
						struct block_list* src;
						if ((src = map_id2bl(group->src_id)) != NULL) {
							clif_skill_poseffect(src, group->skill_id, group->skill_lv, bl->x, bl->y, tick);
							group->val2 = 1;
						}
					}
					// No damage until expiration
					return 0;
				}
				break;
		}
	}

	//Don't continue if unit or even group is expired and has been deleted.
	if( !group || !unit->alive )
		return 0;

	dissonance = skill_dance_switch(unit, 0);

	if( (unit->range >= 0 && group->interval != -1) )
	{
		map_foreachinrange(skill_unit_timer_sub_onplace, bl, unit->range, group->bl_flag, bl,tick);

		if(unit->range == -1) //Unit disabled, but it should not be deleted yet.
			group->unit_id = UNT_USED_TRAPS;
		else if( group->unit_id == UNT_TATAMIGAESHI ) {
			unit->range = -1; //Disable processed cell.
			if (--group->val1 <= 0) { // number of live cells
				//All tiles were processed, disable skill.
				group->target_flag=BCT_NOONE;
				group->bl_flag= BL_NUL;
			}
		}
		else if (group->skill_id == SK_WZ_METEORSTORM ) {
			skill_delunit(unit);
			return 0;
		}
	}

	if( dissonance )
		skill_dance_switch(unit, 1);

	return 0;
}

/*==========================================
 * Executes on all skill units every SKILLUNITTIMER_INTERVAL miliseconds.
 *------------------------------------------*/
TIMER_FUNC(skill_unit_timer){
	map_freeblock_lock();

	skillunit_db->foreach(skillunit_db, skill_unit_timer_sub, tick);

	map_freeblock_unlock();
	return 0;
}

static int skill_unit_temp[20];  // temporary storage for tracking skill unit skill ids as players move in/out of them
/*==========================================
 * flag :
 *	1 : store that skill_unit in array
 *	2 : clear that skill_unit
 *	4 : call_on_left
 *------------------------------------------*/
int skill_unit_move_sub(struct block_list* bl, va_list ap)
{
	struct skill_unit* unit = (struct skill_unit *)bl;
	struct skill_unit_group* group = NULL;

	struct block_list* target = va_arg(ap,struct block_list*);
	t_tick tick = va_arg(ap,t_tick);
	int flag = va_arg(ap,int);
	bool dissonance;
	uint16 skill_id;
	int i;

	nullpo_ret(unit);
	nullpo_ret(target);

	if( !unit->alive || target->prev == NULL )
		return 0;

	nullpo_ret(group = unit->group);

	

	dissonance = skill_dance_switch(unit, 0);

	//Necessary in case the group is deleted after calling on_place/on_out [Skotlex]
	skill_id = group->skill_id;

	if( group->interval != -1 && !skill_get_unit_flag(skill_id, UF_DUALMODE) ) //Lullaby is the exception, bugreport:411
	{	//Non-dualmode unit skills with a timer don't trigger when walking, so just return
		if( dissonance ) {
			skill_dance_switch(unit, 1);
			skill_unit_onleft(skill_unit_onout(unit,target,tick),target,tick); //we placed a dissonance, let's update
		}
		return 0;
	}

	//Target-type check.
	if( !(group->bl_flag&target->type && battle_check_target(&unit->bl,target,group->target_flag) > 0) ) {
		if( group->src_id == target->id && group->state.song_dance&0x2 ) { //Ensemble check to see if they went out/in of the area [Skotlex]
			if( flag&1 ) {
				if( flag&2 ) { //Clear this skill id.
					ARR_FIND( 0, ARRAYLENGTH(skill_unit_temp), i, skill_unit_temp[i] == skill_id );
					if( i < ARRAYLENGTH(skill_unit_temp) )
						skill_unit_temp[i] = 0;
				}
			} else {
				if( flag&2 ) { //Store this skill id.
					ARR_FIND( 0, ARRAYLENGTH(skill_unit_temp), i, skill_unit_temp[i] == 0 );
					if( i < ARRAYLENGTH(skill_unit_temp) )
						skill_unit_temp[i] = skill_id;
					else
						ShowError("skill_unit_move_sub: Reached limit of unit objects per cell! (skill_id: %hu)\n", skill_id );
				}
			}

			if( flag&4 )
				skill_unit_onleft(skill_id,target,tick);
		}

		if( dissonance )
			skill_dance_switch(unit, 1);

		return 0;
	} else {
		if( flag&1 ) {
			int result = skill_unit_onplace(unit,target,tick);

			if( flag&2 && result ) { //Clear skill ids we have stored in onout.
				ARR_FIND( 0, ARRAYLENGTH(skill_unit_temp), i, skill_unit_temp[i] == result );
				if( i < ARRAYLENGTH(skill_unit_temp) )
					skill_unit_temp[i] = 0;
			}
		} else {
			int result = skill_unit_onout(unit,target,tick);

			if( flag&2 && result ) { //Store this unit id.
				ARR_FIND( 0, ARRAYLENGTH(skill_unit_temp), i, skill_unit_temp[i] == 0 );
				if( i < ARRAYLENGTH(skill_unit_temp) )
					skill_unit_temp[i] = skill_id;
				else
					ShowError("skill_unit_move_sub: Reached limit of unit objects per cell! (skill_id: %hu)\n", skill_id );
			}
		}

		//TODO: Normally, this is dangerous since the unit and group could be freed
		//inside the onout/onplace functions. Currently it is safe because we know song/dance
		//cells do not get deleted within them. [Skotlex]
		if( dissonance )
			skill_dance_switch(unit, 1);

		if( flag&4 )
			skill_unit_onleft(skill_id,target,tick);

		return 1;
	}
}

/*==========================================
 * Invoked when a char has moved and unit cells must be invoked (onplace, onout, onleft)
 * Flag values:
 * flag&1: invoke skill_unit_onplace (otherwise invoke skill_unit_onout)
 * flag&2: this function is being invoked twice as a bl moves, store in memory the affected
 * units to figure out when they have left a group.
 * flag&4: Force a onleft event (triggered when the bl is killed, for example)
 *------------------------------------------*/
int skill_unit_move(struct block_list *bl, t_tick tick, int flag)
{
	nullpo_ret(bl);

	if( bl->prev == NULL )
		return 0;

	if( flag&2 && !(flag&1) ) //Onout, clear data
		memset(skill_unit_temp, 0, sizeof(skill_unit_temp));

	map_foreachincell(skill_unit_move_sub,bl->m,bl->x,bl->y,BL_SKILL,bl,tick,flag);

	if( flag&2 && flag&1 ) { //Onplace, check any skill units you have left.
		int i;

		for( i = 0; i < ARRAYLENGTH(skill_unit_temp); i++ )
			if( skill_unit_temp[i] )
				skill_unit_onleft(skill_unit_temp[i], bl, tick);
	}

	return 0;
}

/*==========================================
 * Moves skill unit to map m with coordinates x & y (example when knocked back)
 * @param bl Skill unit
 * @param m Map
 * @param dx
 * @param dy
 *------------------------------------------*/
void skill_unit_move_unit(struct block_list *bl, int dx, int dy) {
	t_tick tick = gettick();
	struct skill_unit *su;

	if (bl->type != BL_SKILL)
		return;
	if (!(su = (struct skill_unit *)bl))
		return;
	if (!su->alive)
		return;

	if (su->group && skill_get_unit_flag(su->group->skill_id, UF_ENSEMBLE))
		return; //Ensembles may not be moved around.

	if (!bl->prev) {
		bl->x = dx;
		bl->y = dy;
		return;
	}

	map_moveblock(bl, dx, dy, tick);
	map_foreachincell(skill_unit_effect,bl->m,bl->x,bl->y,su->group->bl_flag,bl,tick,1);
	skill_getareachar_skillunit_visibilty(su, AREA);
	return;
}

/**
 * Moves skill unit group to map m with coordinates x & y (example when knocked back)
 * @param group Skill Group
 * @param m Map
 * @param dx
 * @param dy
 */
void skill_unit_move_unit_group(struct skill_unit_group *group, int16 m, int16 dx, int16 dy)
{
	int i, j;
	t_tick tick = gettick();
	int *m_flag;
	struct skill_unit *unit1;
	struct skill_unit *unit2;

	if (group == NULL)
		return;

	if (group->unit_count <= 0)
		return;

	if (group->unit == NULL)
		return;

	if (skill_get_unit_flag(group->skill_id, UF_ENSEMBLE))
		return; //Ensembles may not be moved around.

	m_flag = (int *) aCalloc(group->unit_count, sizeof(int));
	//    m_flag
	//		0: Neither of the following (skill_unit_onplace & skill_unit_onout are needed)
	//		1: Unit will move to a slot that had another unit of the same group (skill_unit_onplace not needed)
	//		2: Another unit from same group will end up positioned on this unit (skill_unit_onout not needed)
	//		3: Both 1+2.
	for(i = 0; i < group->unit_count; i++) {
		unit1 =& group->unit[i];
		if (!unit1->alive || unit1->bl.m != m)
			continue;
		for(j = 0; j < group->unit_count; j++) {
			unit2 = &group->unit[j];
			if (!unit2->alive)
				continue;
			if (unit1->bl.x+dx == unit2->bl.x && unit1->bl.y+dy == unit2->bl.y)
				m_flag[i] |= 0x1;
			if (unit1->bl.x-dx == unit2->bl.x && unit1->bl.y-dy == unit2->bl.y)
				m_flag[i] |= 0x2;
		}
	}

	j = 0;

	for (i = 0; i < group->unit_count; i++) {
		unit1 = &group->unit[i];
		if (!unit1->alive)
			continue;
		if (!(m_flag[i]&0x2)) {
			if (group->state.song_dance&0x1) //Cancel dissonance effect.
				skill_dance_overlap(unit1, 0);
			map_foreachincell(skill_unit_effect,unit1->bl.m,unit1->bl.x,unit1->bl.y,group->bl_flag,&unit1->bl,tick,4);
		}
		//Move Cell using "smart" criteria (avoid useless moving around)
		switch(m_flag[i]) {
			case 0:
			//Cell moves independently, safely move it.
				map_foreachinmovearea(clif_outsight, &unit1->bl, AREA_SIZE, dx, dy, BL_PC, &unit1->bl);
				map_moveblock(&unit1->bl, unit1->bl.x+dx, unit1->bl.y+dy, tick);
				break;
			case 1:
			//Cell moves unto another cell, look for a replacement cell that won't collide
			//and has no cell moving into it (flag == 2)
				for(; j < group->unit_count; j++) {
					int dx2, dy2;
					if(m_flag[j] != 2 || !group->unit[j].alive)
						continue;
					//Move to where this cell would had moved.
					unit2 = &group->unit[j];
					dx2 = unit2->bl.x + dx - unit1->bl.x;
					dy2 = unit2->bl.y + dy - unit1->bl.y;
					map_foreachinmovearea(clif_outsight, &unit1->bl, AREA_SIZE, dx2, dy2, BL_PC, &unit1->bl);
					map_moveblock(&unit1->bl, unit2->bl.x+dx, unit2->bl.y+dy, tick);
					j++; //Skip this cell as we have used it.
					break;
				}
				break;
			case 2:
			case 3:
				break; //Don't move the cell as a cell will end on this tile anyway.
		}
		if (!(m_flag[i]&0x2)) { //We only moved the cell in 0-1
			if (group->state.song_dance&0x1) //Check for dissonance effect.
				skill_dance_overlap(unit1, 1);
			skill_getareachar_skillunit_visibilty(unit1, AREA);
			map_foreachincell(skill_unit_effect,unit1->bl.m,unit1->bl.x,unit1->bl.y,group->bl_flag,&unit1->bl,tick,1);
		}
	}

	aFree(m_flag);
}

/**
 * Checking product requirement in player's inventory.
 * Checking if player has the item or not, the amount, and the weight limit.
 * @param sd Player
 * @param nameid Product requested
 * @param trigger Trigger criteria to match will 'ItemLv'
 * @param qty Amount of item will be created
 * @return 0 If failed or Index+1 of item found on skill_produce_db[]
 */
short skill_can_produce_mix(struct map_session_data *sd, t_itemid nameid, int trigger, int qty, short req_skill)
{
	short i, j;
	nullpo_ret(sd);
	if (!nameid || !itemdb_exists(nameid))
		return 0;

	for (i = 0; i < MAX_SKILL_PRODUCE_DB; i++) {
		if (skill_produce_db[i].nameid == nameid) {
			if ((j = skill_produce_db[i].req_skill) > 0){
				if (req_skill == BS_AXE) {
					if(pc_checkskill(sd,j) < skill_produce_db[i].req_skill_lv){
						continue; // must iterate again to check other skills that produce it. [malufett]
					}
				}
				if (req_skill == SK_AM_PHARMACY) {
					if(pc_search_inventory(sd, skill_produce_db[i].req_skill_lv) < 0 ){
						continue; // must iterate again to check other skills that produce it. [malufett]
					}
				}
			}				
			if (j > 0 && sd->menuskill_id > 0 && sd->menuskill_id != j)
				continue; // special case
			break;
		}
	}

	//ShowMessage("I: %d\n", i);
	//if (i >= MAX_SKILL_PRODUCE_DB)
	//	return 0;

	if (req_skill == BS_AXE) {
		if (pc_search_inventory(sd, ITEMID_ANVIL) < 0 &&
			pc_search_inventory(sd, ITEMID_ORIDECON_ANVIL) < 0 &&
			pc_search_inventory(sd, ITEMID_GOLDEN_ANVIL) < 0 &&
			pc_search_inventory(sd, ITEMID_EMPERIUM_ANVIL) < 0)
			return 0;
		if (pc_search_inventory(sd, ITEMID_IRON_HAMMER) < 0 &&
			pc_search_inventory(sd, ITEMID_GOLDEN_HAMMER) < 0 &&
			pc_search_inventory(sd, ITEMID_ORIDECON_HAMMER) < 0)
			return 0;
		//if (pc_search_inventory(sd, ITEMID_BASIC_FORGING_MANUAL) < 0)
		//	return 0;
	}
	if (req_skill == SK_AM_PHARMACY) {
		if (pc_search_inventory(sd, ITEMID_MEDICINE_BOWL) < 0)
			return 0;
	}

	// Cannot carry the produced stuff
	if (pc_checkadditem(sd, nameid, qty) == CHKADDITEM_OVERAMOUNT)
		return 0;

	// Matching the requested produce list
	if (trigger >= 0) {
		if (trigger > 20) { // Non-weapon, non-food item (itemlv must match)
			if (skill_produce_db[i].itemlv != trigger)
				return 0;
		} else if (trigger > 10) { // Food (any item level between 10 and 20 will do)
			if (skill_produce_db[i].itemlv <= 10 || skill_produce_db[i].itemlv > 20)
				return 0;
		} else { // Weapon (itemlv must be higher or equal)
			if (skill_produce_db[i].itemlv > trigger)
				return 0;
		}
	}

	// Check on player's inventory
	for (j = 0; j < MAX_PRODUCE_RESOURCE; j++) {
		t_itemid nameid_produce;

		if (!(nameid_produce = skill_produce_db[i].mat_id[j]))
			continue;
		if (skill_produce_db[i].mat_amount[j] == 0) {
			if (pc_search_inventory(sd,nameid_produce) < 0)
				return 0;
		} else {
			unsigned short idx, amt;

			for (idx = 0, amt = 0; idx < MAX_INVENTORY; idx++)
				if (sd->inventory.u.items_inventory[idx].nameid == nameid_produce)
					amt += sd->inventory.u.items_inventory[idx].amount;
			if (amt < qty * skill_produce_db[i].mat_amount[j])
				return 0;
		}
	}

	return i + 1;
}

/**
 * Attempt to produce an item
 * @param sd Player
 * @param skill_id Skill used
 * @param nameid Requested product
 * @param slot1
 * @param slot2
 * @param slot3
 * @param qty Amount of requested item
 * @param produce_idx Index of produce entry in skill_produce_db[]. (Optional. Assumed the requirements are complete, checked somewhere)
 * @return True is success, False if failed
 */
bool skill_produce_mix(struct map_session_data *sd, uint16 skill_id, t_itemid nameid, int slot1, int slot2, int slot3, int qty, short produce_idx)
{
	ShowMessage("Prod 1/n");
	int slot[3];
	int i, j, sc, ele, idx, equip, wlv, make_per = 0, flag = 0, skill_lv = 0;
	int num = -1; // exclude the recipe
	struct status_data *status;

	nullpo_ret(sd);

	// Check on player's inventory
	for (j = 0; j < MAX_PRODUCE_RESOURCE; j++) {
		t_itemid nameid_produce;

		if (!(nameid_produce = skill_produce_db[produce_idx].mat_id[j]))
			continue;
		if (skill_produce_db[produce_idx
		].mat_amount[j] == 0) {
			if (pc_search_inventory(sd,nameid_produce) < 0)
				return false;
		} else {
			unsigned short idx, amt;

			for (idx = 0, amt = 0; idx < MAX_INVENTORY; idx++)
				if (sd->inventory.u.items_inventory[idx].nameid == nameid_produce)
					amt += sd->inventory.u.items_inventory[idx].amount;
			if (amt < qty * skill_produce_db[produce_idx].mat_amount[j])
				return false;
		}
	}



	status = status_get_status_data(&sd->bl);

	if( sd->skill_id_old == skill_id )
		skill_lv = sd->skill_lv_old;

	if (produce_idx == -1) {
		if( !(idx = skill_can_produce_mix(sd,nameid,-1, qty)) )
			return false;

		idx--;
	}
	else
		idx = produce_idx;


	if (qty < 1)
		qty = 1;

	if (!skill_id) //A skill can be specified for some override cases.
		skill_id = skill_produce_db[idx].req_skill;

	

	slot[0] = slot1;
	slot[1] = slot2;
	slot[2] = slot3;

	for (i = 0, sc = 0, ele = 0; i < 3; i++) { //Note that qty should always be one if you are using these!
		short j;
		if (slot[i] <= 0)
			continue;
		j = pc_search_inventory(sd,slot[i]);
		if (j < 0)
			continue;
		if (slot[i] == ITEMID_STAR_CRUMB) {
			pc_delitem(sd,j,1,1,0,LOG_TYPE_PRODUCE);
			sc++;
		}
		if (slot[i] >= ITEMID_FLAME_HEART && slot[i] <= ITEMID_GREAT_NATURE && ele == 0) {
			static const int ele_table[4] = { ELE_FIRE, ELE_WATER, ELE_WIND, ELE_EARTH };
			pc_delitem(sd,j,1,1,0,LOG_TYPE_PRODUCE);
			ele = ele_table[slot[i]-ITEMID_FLAME_HEART];
		}
	}

	for (i = 0; i < MAX_PRODUCE_RESOURCE; i++) {
		short x, j;
		t_itemid id;

		if (!(id = skill_produce_db[idx].mat_id[i]) || !itemdb_exists(id))
			continue;
		num++;
		x = ( qty) * skill_produce_db[idx].mat_amount[i];
		do {
			int y = 0;

			j = pc_search_inventory(sd,id);

			if (j >= 0) {
				y = sd->inventory.u.items_inventory[j].amount;
				if (y > x)
					y = x;
				pc_delitem(sd,j,y,0,0,LOG_TYPE_PRODUCE);
			} else {
				ShowError("skill_produce_mix: material item error\n");
				return false;
			}
			x -= y;
		} while( j >= 0 && x > 0 );
	}

	if ((equip = (itemdb_isequip(nameid)  )))
		wlv = itemdb_wlv(nameid);

	if (!equip) {
		switch (skill_id) {
			case BS_AXE:
				// Ores & Metals Refining - 100% success rate
				make_per = 100000;
				break;
			case SK_AM_PHARMACY: // Potion Preparation - reviewed with the help of various Ragnainfo sources [DracoRPG]
				switch(nameid){
					case ITEMID_RED_POTION:
					case ITEMID_YELLOW_POTION:
					case ITEMID_WHITE_POTION:
					case ITEMID_BLUE_POTION:
						make_per = 1500; //15% success
						break;
					case ITEMID_RED_SLIM_POTION:
					case ITEMID_YELLOW_SLIM_POTION:
					case ITEMID_WHITE_SLIM_POTION:
						make_per = 500; //5% success
						break;
					case ITEMID_FIRE_BOTTLE:
					case ITEMID_ACID_BOTTLE:
					case ITEMID_MAN_EATER_BOTTLE:
						make_per = 1000; //10% success
						break;
					case ITEMID_THUNDER_RESIST_POTION:
					case ITEMID_EARTH_RESIST_POTION:
					case ITEMID_COLD_RESIST_POTION:
					case ITEMID_FIRE_RESIST_POTION:
						make_per = 500; //5% success
						break;
					//Common items
					default:
						make_per = 1000; //10% success
						break;
				}
				make_per += status->dex*10 + status->int_*10; // Status 10% for each 100 points
				make_per += pc_checkskill(sd,SK_AM_PHARMACY)*1000; // Pharmacy skill bonus: +10/+20/+30/+40/+50
				break;
		}
	} else { // Weapon Forging - skill bonuses are straight from kRO website, other things from a jRO calculator [DracoRPG]
		make_per = 1500; // Base success
		make_per += pc_checkskill(sd,BS_AXE)*1000; // Forging skill bonus: +10/+20/+30/+40/+50
		make_per += status->dex*10 + status->luk*10; // Status 10 for each 100 points
		make_per -= (ele?2000:0) + sc*1500; // Element Stone: -20%, Star Crumb: -15% each
		if      (pc_search_inventory(sd,ITEMID_EMPERIUM_ANVIL) > -1) make_per+= 1000; // Emperium Anvil: +10
		else if (pc_search_inventory(sd,ITEMID_GOLDEN_ANVIL) > -1)   make_per+= 500; // Golden Anvil: +5
		else if (pc_search_inventory(sd,ITEMID_ORIDECON_ANVIL) > -1) make_per+= 250; // Oridecon Anvil: +3
		else if (pc_search_inventory(sd,ITEMID_ANVIL) > -1)          make_per+= 0; // Anvil: +0?

		if      (pc_search_inventory(sd,ITEMID_GOLDEN_HAMMER) > -1)   make_per+= 500; // Golden Hammer: +5
		else if (pc_search_inventory(sd,ITEMID_ORIDECON_HAMMER) > -1) make_per+= 250; // Oridecon Hammer: +2.5
		else if (pc_search_inventory(sd,ITEMID_IRON_HAMMER) > -1)     make_per+= 0; // Iron Hammer: +0

		if (battle_config.wp_rate != 100)
			make_per = make_per * battle_config.wp_rate / 100;
	}

	if (make_per < 1) make_per = 1;


	if (qty > 1 || rnd()%10000 < make_per){ //Success, or crafting multiple items.
		struct item tmp_item;
		memset(&tmp_item,0,sizeof(tmp_item));
		tmp_item.nameid = nameid;
		tmp_item.amount = 1;
		tmp_item.identify = 1;
		if (equip) {
			tmp_item.card[0] = CARD0_FORGE;
			tmp_item.card[1] = ((sc*5)<<8)+ele;
			tmp_item.card[2] = GetWord(sd->status.char_id,0); // CharId
			tmp_item.card[3] = GetWord(sd->status.char_id,1);
		} else {
			//Flag is only used on the end, so it can be used here. [Skotlex]
			switch (skill_id) {
				case BS_AXE:
					flag = battle_config.produce_item_name_input&0x1;
					break;
				case SK_AM_PHARMACY:
					flag = battle_config.produce_item_name_input&0x2;
					break;
				default:
					flag = battle_config.produce_item_name_input&0x80;
					break;
			}
			if (flag) {
				tmp_item.card[0] = CARD0_CREATE;
				tmp_item.card[1] = 0;
				tmp_item.card[2] = GetWord(sd->status.char_id,0); // CharId
				tmp_item.card[3] = GetWord(sd->status.char_id,1);
			}
		}

//		if(log_config.produce > 0)
//			log_produce(sd,nameid,slot1,slot2,slot3,1);
//TODO update PICKLOG

		if (equip) {
			clif_produceeffect(sd,0,nameid);
			clif_misceffect(&sd->bl,3);
			if (itemdb_wlv(nameid) >= 3 && ((ele? 1 : 0) + sc) >= 3) // Fame point system [DracoRPG]
				pc_addfame(sd, battle_config.fame_forge); // Success to forge a lv3 weapon with 3 additional ingredients = +10 fame point
		} else {
			int fame = 0;
			tmp_item.amount = 0;

			for (i = 0; i < qty; i++) {	//Apply quantity modifiers.
				if ((false) && make_per > 1) {
					tmp_item.amount = qty;
					break;
				}
				if (qty == 1 || rnd()%10000 < make_per) { //Success
					tmp_item.amount++;
					if (nameid < ITEMID_RED_SLIM_POTION || nameid > ITEMID_WHITE_SLIM_POTION)
						continue;
					if (skill_id != SK_AM_PHARMACY
						)
						continue;
					//Add fame as needed.
					switch(++sd->potion_success_counter) {
						case 3:
							fame += battle_config.fame_pharmacy_3; // Success to prepare 3 Condensed Potions in a row
							break;
						case 5:
							fame += battle_config.fame_pharmacy_5; // Success to prepare 5 Condensed Potions in a row
							break;
						case 7:
							fame += battle_config.fame_pharmacy_7; // Success to prepare 7 Condensed Potions in a row
							break;
						case 10:
							fame += battle_config.fame_pharmacy_10; // Success to prepare 10 Condensed Potions in a row
							sd->potion_success_counter = 0;
							break;
					}
				} else //Failure
					sd->potion_success_counter = 0;
			}

			if (fame)
				pc_addfame(sd,fame);
			//Visual effects and the like.
			switch (skill_id) {
				case SK_AM_PHARMACY:
					clif_produceeffect(sd,2,nameid);
					clif_misceffect(&sd->bl,5);
					break;
				default: //Those that don't require a skill?
					if (skill_produce_db[idx].itemlv > 10 && skill_produce_db[idx].itemlv <= 20) { //Cooking items.
						clif_specialeffect(&sd->bl, EF_COOKING_OK, AREA);
						if (sd->cook_mastery < 1999)
							pc_setglobalreg(sd, add_str(COOKMASTERY_VAR), sd->cook_mastery + ( 1 << ( (skill_produce_db[idx].itemlv - 11) / 2 ) ) * 5);
					}
					break;
			}
		}

		 if (tmp_item.amount) { //Success
			if ((flag = pc_additem(sd,&tmp_item,tmp_item.amount,LOG_TYPE_PRODUCE))) {
				clif_additem(sd,0,0,flag);
				if( battle_config.skill_drop_items_full ){
					map_addflooritem(&tmp_item,tmp_item.amount,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0,0);
				}
			}
			
			return true;
		}
	}
	//Failure
//	if(log_config.produce)
//		log_produce(sd,nameid,slot1,slot2,slot3,0);
//TODO update PICKLOG

	if (equip) {
		clif_produceeffect(sd,1,nameid);
		clif_misceffect(&sd->bl,2);
	} else {
		switch (skill_id) {

			case SK_AM_PHARMACY:

				clif_produceeffect(sd,3,nameid);
				clif_misceffect(&sd->bl,6);
				sd->potion_success_counter = 0; // Fame point system [DracoRPG]
				break;

			case BS_AXE:
				clif_produceeffect(sd,1,nameid);
				clif_misceffect(&sd->bl,2);
				break;

			
			
			default:
				if (skill_produce_db[idx].itemlv > 10 && skill_produce_db[idx].itemlv <= 20 ) { //Cooking items.
					clif_specialeffect(&sd->bl, EF_COOKING_FAIL, AREA);
					if (sd->cook_mastery > 0)
						pc_setglobalreg(sd, add_str(COOKMASTERY_VAR), sd->cook_mastery - ( 1 << ((skill_produce_db[idx].itemlv - 11) / 2) ) - ( ( ( 1 << ((skill_produce_db[idx].itemlv - 11) / 2) ) >> 1 ) * 3 ));
				}
				break;
		}
	}
	return false;
}

/**
 * Attempt to create arrow by specified material
 * @param sd Player
 * @param nameid Item ID of material
 * @return True if created, False is failed
 */
bool skill_arrow_create(struct map_session_data *sd, t_itemid nameid)
{
	short i, j, idx = -1;
	struct item tmp_item;

	nullpo_ret(sd);

	if (!nameid || !itemdb_exists(nameid) || !skill_arrow_count)
		return false;

	for (i = 0; i < MAX_SKILL_ARROW_DB;i++) {
		if (nameid == skill_arrow_db[i].nameid) {
			idx = i;
			break;
		}
	}

	if (idx < 0 || (j = pc_search_inventory(sd,nameid)) < 0)
		return false;

	pc_delitem(sd,j,1,0,0,LOG_TYPE_PRODUCE);
	for (i = 0; i < MAX_ARROW_RESULT; i++) {
		char flag = 0;

		if (skill_arrow_db[idx].cre_id[i] == 0 || !itemdb_exists(skill_arrow_db[idx].cre_id[i]) || skill_arrow_db[idx].cre_amount[i] == 0)
			continue;
		memset(&tmp_item,0,sizeof(tmp_item));
		tmp_item.identify = 1;
		tmp_item.nameid = skill_arrow_db[idx].cre_id[i];
		tmp_item.amount = skill_arrow_db[idx].cre_amount[i];
		if (battle_config.produce_item_name_input&0x4) {
			tmp_item.card[0] = CARD0_CREATE;
			tmp_item.card[1] = 0;
			tmp_item.card[2] = GetWord(sd->status.char_id,0); // CharId
			tmp_item.card[3] = GetWord(sd->status.char_id,1);
		}
		if ((flag = pc_additem(sd,&tmp_item,tmp_item.amount * sd->menuskill_val2,LOG_TYPE_PRODUCE))) {
			clif_additem(sd,0,0,flag);
			map_addflooritem(&tmp_item,tmp_item.amount,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0,0);
		}
	}
	return true;
}

/**
 * Enchant weapon with poison
 * @param sd Player
 * @nameid Item ID of poison type
 */
int skill_poisoningweapon(struct map_session_data *sd, t_itemid nameid)
{
//	nullpo_ret(sd);
//
//	if( !nameid || pc_delitem(sd,pc_search_inventory(sd,nameid),1,0,0,LOG_TYPE_CONSUME) ) {
//		clif_skill_fail(sd,GC_POISONINGWEAPON,USESKILL_FAIL_LEVEL,0);
//		return 0;
//	}
//
//	sc_type type;
//	int chance;
//	//uint16 msg = 1443; //Official is using msgstringtable.txt
//	char output[CHAT_SIZE_MAX];
//	const char *msg;
//
//	switch( nameid ) {
//		case ITEMID_PARALYSE:      type = SC_PARALYSE;      /*msg = 1444;*/ msg = "Paralyze"; break;
//		case ITEMID_PYREXIA:       type = SC_PYREXIA;		/*msg = 1448;*/ msg = "Pyrexia"; break;
//		case ITEMID_DEATHHURT:     type = SC_DEATHHURT;     /*msg = 1447;*/ msg = "Deathhurt"; break;
//		case ITEMID_LEECHESEND:    type = SC_LEECHESEND;    /*msg = 1450;*/ msg = "Leech End"; break;
//		case ITEMID_VENOMBLEED:    type = SC_VENOMBLEED;    /*msg = 1445;*/ msg = "Venom Bleed"; break;
//		case ITEMID_TOXIN:         type = SC_TOXIN;         /*msg = 1443;*/ msg = "Toxin"; break;
//		case ITEMID_MAGICMUSHROOM: type = SC_MAGICMUSHROOM; /*msg = 1446;*/ msg = "Magic Mushroom"; break;
//		case ITEMID_OBLIVIONCURSE: type = SC_OBLIVIONCURSE; /*msg = 1449;*/ msg = "Oblivion Curse"; break;
//		default:
//			clif_skill_fail(sd,GC_POISONINGWEAPON,USESKILL_FAIL_LEVEL,0);
//			return 0;
//	}
//
//	status_change_end(&sd->bl, STATUS_POISONINGWEAPON, INVALID_TIMER); // End the status so a new poison can be applied (if changed)
//	chance = 2 + 2 * sd->menuskill_val; // 2 + 2 * skill_lv
//	sc_start4(&sd->bl,&sd->bl, STATUS_POISONINGWEAPON, 100, pc_checkskill(sd, GC_RESEARCHNEWPOISON), //in Aegis it store the level of GC_RESEARCHNEWPOISON in val1
//		type, chance, 0, skill_get_time(GC_POISONINGWEAPON, sd->menuskill_val));
//	status_change_start(&sd->bl, &sd->bl, type, 10000, sd->menuskill_val, 0, 0, 0, skill_get_time(GC_POISONINGWEAPON, sd->menuskill_val), SCSTART_NOAVOID | SCSTART_NOICON); // Apply bonus to caster
//
//	sprintf(output, msg_txt(sd,721), msg);
//	clif_messagecolor(&sd->bl,color_table[COLOR_WHITE],output,false,SELF);
//
///*#if PACKETVER >= 20110208 //! TODO: Check the correct PACKVETVER
//	clif_msg(sd,msg);
//#endif*/
	return 0;
}

void skill_toggle_magicpower(struct block_list *bl, uint16 skill_id)
{
	struct status_change *sc = status_get_sc(bl);

	// non-offensive and non-magic skills do not affect the status
	if (skill_get_nk(skill_id, NK_NODAMAGE) || !(skill_get_type(skill_id)&BF_MAGIC))
		return;

	if (sc && sc->count && sc->data[STATUS_MYSTICALAMPLIFICATION]) {
		if (sc->data[STATUS_MYSTICALAMPLIFICATION]->val4) {
			status_change_end(bl, STATUS_MYSTICALAMPLIFICATION, INVALID_TIMER);
		} else {
			sc->data[STATUS_MYSTICALAMPLIFICATION]->val4 = 1;
			status_calc_bl(bl, status_sc2scb_flag(STATUS_MYSTICALAMPLIFICATION));
#ifndef RENEWAL
			if(bl->type == BL_PC){// update current display.
				clif_updatestatus(((TBL_PC *)bl),SP_MATK1);
				clif_updatestatus(((TBL_PC *)bl),SP_MATK2);
			}
#endif
		}
	}
}

int skill_plant_cultivation(struct map_session_data *sd, t_itemid nameid, int skill_id) {
	int x, y, i, class_, skill;
	int item_to_delete;
	struct mob_data *md;
	nullpo_ret(sd);
	skill = sd->menuskill_val;


	




	// Item picked decides the mob class
	switch(nameid) {
		case ITEMID_GREEN_PLANT:		class_ = MOBID_GREEN_PLANT;     item_to_delete = ITEMID_STEM;               break;
		case ITEMID_RED_PLANT:			class_ = MOBID_RED_PLANT;	    item_to_delete = ITEMID_STEM;               break;
		case ITEMID_ORANGE_PLANT:		class_ = MOBID_ORANGE_PLANT;    item_to_delete = ITEMID_STEM;               break;
		case ITEMID_YELLOW_PLANT:	    class_ = MOBID_YELLOW_PLANT;    item_to_delete = ITEMID_STEM;               break;
		case ITEMID_WHITE_PLANT:		class_ = MOBID_WHITE_PLANT;	    item_to_delete = ITEMID_STEM;               break;
		case ITEMID_BLUE_PLANT:		    class_ = MOBID_BLUE_PLANT;	    item_to_delete = ITEMID_STEM;               break;
		case ITEMID_GREEN_MUSHROOM:	    class_ = MOBID_GREEN_MUSHROOM;  item_to_delete = ITEMID_MUSHROOM_SPORE;     break;
		case ITEMID_YELLOW_MUSHROOM:	class_ = MOBID_YELLOW_MUSHROOM; item_to_delete = ITEMID_MUSHROOM_SPORE;     break;
		case ITEMID_BLUE_MUSHROOM:	    class_ = MOBID_BLUE_MUSHROOM;   item_to_delete = ITEMID_MUSHROOM_SPORE;     break;
		case ITEMID_RED_MUSHROOM:	    class_ = MOBID_RED_MUSHROOM;    item_to_delete = ITEMID_MUSHROOM_SPORE;     break;
		case ITEMID_SHINING_PLANT:	    class_ = MOBID_SHINING_PLANT;   item_to_delete = ITEMID_YGGDRASIL_SEED;     break;
		default:
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			return 0;
	}

	if (item_to_delete == 0){
		clif_skill_fail(sd,SK_AM_PLANTCULTIVATION,USESKILL_FAIL_LEVEL,0);
		return 0;
	}
	if( !nameid || (i = pc_search_inventory(sd,item_to_delete)) < 0 || !skill) {
		clif_skill_fail(sd,SK_AM_PLANTCULTIVATION,USESKILL_FAIL_LEVEL,0);
		return 0;
	}
	pc_delitem(sd,i,1,0,0,LOG_TYPE_OTHER);

	// Spawn Position
	// pc_delitem(sd,i,1,0,0,LOG_TYPE_CONSUME);
	x = sd->sc.comet_x;
	y = sd->sc.comet_y;
	sd->sc.comet_x = 0;
	sd->sc.comet_y = 0;
	sd->menuskill_val = 0;

	md = mob_once_spawn_sub(&sd->bl, sd->bl.m, x, y, sd->status.name, class_, "", SZ_SMALL, AI_NONE);
	if( md ) {
		struct unit_data *ud = unit_bl2ud(&md->bl);
		md->master_id = sd->bl.id;
		md->special_state.ai = AI_FAW;
		if(ud) {
			ud->skill_id = skill_id;
			ud->skill_lv = skill;
		}
		if( md->deletetimer != INVALID_TIMER )
			delete_timer(md->deletetimer, mob_timer_delete);
		md->deletetimer = add_timer (gettick() + skill_get_time(skill_id,skill), mob_timer_delete, md->bl.id, 0);
		mob_spawn(md);
	}

	return 0;
}

int skill_summon_elemental_sphere(struct map_session_data *sd, t_itemid nameid, int skill_id) {
	status_change *sc = status_get_sc(&sd->bl);
	int skill_lv = pc_checkskill(sd, skill_id);
	e_wl_spheres element;
	int i = 0;

	switch (nameid) {
		case 1:
			element = WLS_STONE;
			break;
		case 2:
			element = WLS_WIND;
			break;
		case 3:
			element = WLS_WATER;
			break;
		case 4:
			element = WLS_FIRE;
			break;
	}

	if (nameid != 5) {
		sc_type sphere = STATUS_NONE;
		for (i = STATUS_SPHERE_1; i <= STATUS_SPHERE_4; i++) {
			if (sc->data[i] == nullptr) {
				sphere = static_cast<sc_type>(i); // Take the free SC
				break;
			}
		}

		if (sphere == STATUS_NONE) {
			if (sd) // No free slots to put SC
				clif_skill_fail(sd, skill_id, USESKILL_FAIL_SUMMON, 0);
			return 0;
		}

		sc_start2(&sd->bl, &sd->bl, sphere, 100, element, skill_lv, skill_get_time(skill_id, skill_lv));
	} else {
			if (sc->data[STATUS_SPHERE_1] == nullptr &&
				sc->data[STATUS_SPHERE_2] == nullptr &&
				sc->data[STATUS_SPHERE_3] == nullptr &&
				sc->data[STATUS_SPHERE_4] == nullptr
			) {
				sc_start2(&sd->bl, &sd->bl, STATUS_SPHERE_1, 100, WLS_STONE, skill_lv, skill_get_time(skill_id, skill_lv));
				sc_start2(&sd->bl, &sd->bl, STATUS_SPHERE_2, 100, WLS_WIND, skill_lv, skill_get_time(skill_id, skill_lv));
				sc_start2(&sd->bl, &sd->bl, STATUS_SPHERE_3, 100, WLS_WATER, skill_lv, skill_get_time(skill_id, skill_lv));
				sc_start2(&sd->bl, &sd->bl, STATUS_SPHERE_4, 100, WLS_FIRE, skill_lv, skill_get_time(skill_id, skill_lv));
			}else{
				if (sd) // No free slots to put SC
					clif_skill_fail(sd, skill_id, USESKILL_FAIL_SUMMON, 0);
				return 0;
			}


	}
	clif_skill_nodamage(&sd->bl, &sd->bl, skill_id, 0, 0);
	return 0;
}


int skill_magicdecoy(struct map_session_data *sd, t_itemid nameid, int skill_id) {
	int x, y, i, j, class_, skill;
	int item_to_delete = 0;
	struct mob_data *md;
	nullpo_ret(sd);
	skill = sd->menuskill_val;

	switch(nameid) {
		case ITEMID_FIRE_BOLT_TOTEM:		item_to_delete = ITEMID_FIRE_ESSENCE;	break;
		case ITEMID_COLD_BOLT_TOTEM:		item_to_delete = ITEMID_WATER_ESSENCE;	break;
		case ITEMID_LIGHTNING_BOLT_TOTEM:	item_to_delete = ITEMID_WIND_ESSENCE;	break;
		case ITEMID_EARTH_BOLT_TOTEM:		item_to_delete = ITEMID_EARTH_ESSENCE;	break;

		case ITEMID_HOLY_GHOST_TOTEM:	item_to_delete = ITEMID_GHOST_ESSENCE;	break;
		case ITEMID_CORRUPT_TOTEM:		item_to_delete = ITEMID_SHADOW_ESSENCE;	break;
		case ITEMID_SACRED_WAVE_TOTEM:	item_to_delete = ITEMID_HOLY_ESSENCE;	break;
		case ITEMID_EVIL_EYE_TOTEM:		item_to_delete = ITEMID_NEUTRAL_ESSENCE;break;

	}

	if (item_to_delete == 0){
		clif_skill_fail(sd,SK_HT_ELEMENTALTOTEM,USESKILL_FAIL_LEVEL,0);
		return 0;
	}
	if( !nameid || (i = pc_search_inventory(sd,item_to_delete)) < 0 || (j = pc_search_inventory(sd, ITEMID_SOLID_TRUNK)) < 0 || !skill) {
		clif_skill_fail(sd,SK_HT_ELEMENTALTOTEM,USESKILL_FAIL_LEVEL,0);
		return 0;
	}
	pc_delitem(sd,i,1,0,0,LOG_TYPE_OTHER);
	pc_delitem(sd,j,1,0,0,LOG_TYPE_OTHER);
	// Spawn Position
	// pc_delitem(sd,i,1,0,0,LOG_TYPE_CONSUME);
	x = sd->sc.comet_x;
	y = sd->sc.comet_y;
	sd->sc.comet_x = 0;
	sd->sc.comet_y = 0;
	sd->menuskill_val = 0;

	// Item picked decides the mob class
	switch(nameid) {
		case ITEMID_FIRE_BOLT_TOTEM:		class_ = MOBID_MAGICDECOY_FIRE;		break;
		case ITEMID_COLD_BOLT_TOTEM:		class_ = MOBID_MAGICDECOY_WATER;	break;
		case ITEMID_LIGHTNING_BOLT_TOTEM:	class_ = MOBID_MAGICDECOY_WIND;		break;
		case ITEMID_EARTH_BOLT_TOTEM:		class_ = MOBID_MAGICDECOY_EARTH;	break;
		case ITEMID_HOLY_GHOST_TOTEM:	    class_ = MOBID_LICHTERN_Y;		    break;
		case ITEMID_CORRUPT_TOTEM:			class_ = MOBID_LICHTERN_R;		    break;
		case ITEMID_SACRED_WAVE_TOTEM:		class_ = MOBID_LICHTERN_B;		    break;
		case ITEMID_EVIL_EYE_TOTEM:			class_ = MOBID_LICHTERN_G;		    break;
		default:
			clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
			return 0;
	}

	md = mob_once_spawn_sub(&sd->bl, sd->bl.m, x, y, sd->status.name, class_, "", SZ_SMALL, AI_NONE);
	if( md ) {
		struct unit_data *ud = unit_bl2ud(&md->bl);
		md->master_id = sd->bl.id;
		md->special_state.ai = AI_FAW;
		if(ud) {
			ud->skill_id = skill_id;
			ud->skill_lv = skill;
		}
		if( md->deletetimer != INVALID_TIMER )
			delete_timer(md->deletetimer, mob_timer_delete);
		md->deletetimer = add_timer (gettick() + skill_get_time(skill_id,skill), mob_timer_delete, md->bl.id, 0);
		mob_spawn(md);
	}

	return 0;
}

/**
 * Process Warlock Spellbooks
 * @param sd: Player data
 * @param nameid: Spellbook item used
 */
void skill_spellbook(struct map_session_data *sd, t_itemid nameid) {
	nullpo_retv(sd);
	if (reading_spellbook_db.empty())
		return;
	struct status_change *sc = status_get_sc(&sd->bl);
	for (int i = STATUS_SPELLBOOK1; i <= STATUS_MAXSPELLBOOK; i++) {
		if (sc == nullptr || sc->data[i] == nullptr)
			break;
		if (i == STATUS_MAXSPELLBOOK) {
			clif_skill_fail(sd, SK_MG_READING_SB, USESKILL_FAIL_SPELLBOOK_READING, 0);
			return;
		}
	}
	std::shared_ptr<s_skill_spellbook_db> spell = reading_spellbook_db.findBook(nameid);
	if (spell == nullptr)
		return;
	uint16 skill_id = spell->skill_id, skill_lv = pc_checkskill(sd, skill_id);

	if (skill_lv == 0) { // Caster hasn't learned the skill
		sc_start(&sd->bl,&sd->bl, STATUS_SLEEP, 100, 1, skill_get_time(SK_MG_READING_SB, pc_checkskill(sd, SK_MG_READING_SB)));
		clif_skill_fail(sd, SK_MG_READING_SB, USESKILL_FAIL_SPELLBOOK_DIFFICULT_SLEEP, 0);
		return;
	}
	int points = spell->points;

	{
		sc_start4(&sd->bl, &sd->bl, STATUS_MAXSPELLBOOK, 100, skill_id, skill_lv, points, 0, INFINITE_TICK);
	}
	// Reading Spell Book SP cost same as the sealed spell.
	status_zap(&sd->bl, 0, skill_get_sp(skill_id, skill_lv));
}

int skill_select_menu(struct map_session_data *sd,uint16 skill_id) {
	int lv, prob, aslvl = 0;
	uint16 id, sk_idx = 0;
	nullpo_ret(sd);

	

	if (!skill_id || !(sk_idx = skill_get_index(skill_id)))
		return 0;

	if( !skill_get_inf2(skill_id, INF2_ISAUTOSHADOWSPELL) || (id = sd->status.skill[sk_idx].id) == 0 || sd->status.skill[sk_idx].flag != SKILL_FLAG_PLAGIARIZED ) {
		clif_skill_fail(sd,SK_ST_AUTOSHADOWSPELL,USESKILL_FAIL_LEVEL,0);
		return 0;
	}

	lv = (aslvl); // The level the skill will be autocasted
	prob = aslvl *4; // Probability to auto cast
	sc_start4(&sd->bl,&sd->bl,STATUS_AUTOSHADOWSPELL,100,id,lv,prob,0,skill_get_time(SK_ST_AUTOSHADOWSPELL,aslvl));
	return 0;
}

int skill_elementalanalysis(struct map_session_data* sd, int n, uint16 skill_lv, unsigned short* item_list) {
	//int i;

	//nullpo_ret(sd);
	//nullpo_ret(item_list);

	//if( n <= 0 )
	//	return 1;

	//for( i = 0; i < n; i++ ) {
	//	t_itemid nameid, product;
	//	int add_amount, del_amount, idx;
	//	struct item tmp_item;

	//	idx = item_list[i*2+0]-2;

	//	if( idx < 0 || idx >= MAX_INVENTORY ){
	//		return 1;
	//	}

	//	del_amount = item_list[i*2+1];

	//	if( skill_lv == 2 )
	//		del_amount -= (del_amount % 10);
	//	add_amount = (skill_lv == 1) ? del_amount * (5 + rnd()%5) : del_amount / 10 ;

	//	if( (nameid = sd->inventory.u.items_inventory[idx].nameid) <= 0 || del_amount > sd->inventory.u.items_inventory[idx].amount ) {
	//		clif_skill_fail(sd,SO_EL_ANALYSIS,USESKILL_FAIL_LEVEL,0);
	//		return 1;
	//	}

	//	switch( nameid ) {
	//		// Level 1
	//		case ITEMID_FLAME_HEART:		product = ITEMID_BLOODY_RED;		break;
	//		case ITEMID_MISTIC_FROZEN:		product = ITEMID_CRYSTAL_BLUE;		break;
	//		case ITEMID_ROUGH_WIND:			product = ITEMID_WIND_OF_VERDURE;	break;
	//		case ITEMID_GREAT_NATURE:		product = ITEMID_YELLOW_LIVE;		break;
	//		// Level 2
	//		case ITEMID_BLOODY_RED:			product = ITEMID_FLAME_HEART;		break;
	//		case ITEMID_CRYSTAL_BLUE:		product = ITEMID_MISTIC_FROZEN;		break;
	//		case ITEMID_WIND_OF_VERDURE:	product = ITEMID_ROUGH_WIND;		break;
	//		case ITEMID_YELLOW_LIVE:		product = ITEMID_GREAT_NATURE;		break;
	//		default:
	//			clif_skill_fail(sd,SO_EL_ANALYSIS,USESKILL_FAIL_LEVEL,0);
	//			return 1;
	//	}

	//	if( pc_delitem(sd,idx,del_amount,0,1,LOG_TYPE_CONSUME) ) {
	//		clif_skill_fail(sd,SO_EL_ANALYSIS,USESKILL_FAIL_LEVEL,0);
	//		return 1;
	//	}

	//	if( skill_lv == 2 && rnd()%100 < 25 ) {	// At level 2 have a fail chance. You loose your items if it fails.
	//		clif_skill_fail(sd,SO_EL_ANALYSIS,USESKILL_FAIL_LEVEL,0);
	//		return 1;
	//	}

	//	memset(&tmp_item,0,sizeof(tmp_item));
	//	tmp_item.nameid = product;
	//	tmp_item.amount = add_amount;
	//	tmp_item.identify = 1;

	//	if( tmp_item.amount ) {
	//		unsigned char flag = pc_additem(sd,&tmp_item,tmp_item.amount,LOG_TYPE_CONSUME);
	//		if( flag != 0 ) {
	//			clif_additem(sd,0,0,flag);
	//			map_addflooritem(&tmp_item,tmp_item.amount,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0,0);
	//		}
	//	}

	//}

	return 0;
}

int skill_changematerial(struct map_session_data *sd, int n, unsigned short *item_list) {
	//int i, j, k, c, p = 0, amount;
	//t_itemid nameid;

	//nullpo_ret(sd);
	//nullpo_ret(item_list);

	//// Search for objects that can be created.
	//for( i = 0; i < MAX_SKILL_PRODUCE_DB; i++ ) {
	//	if( skill_produce_db[i].itemlv == 26 && skill_produce_db[i].nameid > 0 ) {
	//		p = 0;
	//		do {
	//			c = 0;
	//			// Verification of overlap between the objects required and the list submitted.
	//			for( j = 0; j < MAX_PRODUCE_RESOURCE; j++ ) {
	//				if( skill_produce_db[i].mat_id[j] > 0 ) {
	//					for( k = 0; k < n; k++ ) {
	//						int idx = item_list[k*2+0]-2;

	//						if( idx < 0 || idx >= MAX_INVENTORY ){
	//							return 0;
	//						}

	//						nameid = sd->inventory.u.items_inventory[idx].nameid;
	//						amount = item_list[k*2+1];
	//						if( nameid > 0 && sd->inventory.u.items_inventory[idx].identify == 0 ){
	//							clif_msg_skill(sd,GN_CHANGEMATERIAL,ITEM_UNIDENTIFIED);
	//							return 0;
	//						}
	//						if( nameid == skill_produce_db[i].mat_id[j] && (amount-p*skill_produce_db[i].mat_amount[j]) >= skill_produce_db[i].mat_amount[j]
	//							&& (amount-p*skill_produce_db[i].mat_amount[j])%skill_produce_db[i].mat_amount[j] == 0 ) // must be in exact amount
	//							c++; // match
	//					}
	//				}
	//				else
	//					break;	// No more items required
	//			}
	//			p++;
	//		} while(n == j && c == n);
	//		p--;
	//		if ( p > 0 ) {
	//			skill_produce_mix(sd,GN_CHANGEMATERIAL,skill_produce_db[i].nameid,0,0,0,p,i);
	//			return 1;
	//		}
	//	}
	//}

	//if( p == 0)
	//	clif_msg_skill(sd,GN_CHANGEMATERIAL,ITEM_CANT_COMBINE);

	return 0;
}

/**
 * For Royal Guard's LG_TRAMPLE
 */
static int skill_destroy_trap(struct block_list *bl, va_list ap)
{
	struct skill_unit *su = (struct skill_unit *)bl;
	struct skill_unit_group *sg = NULL;
	t_tick tick;

	nullpo_ret(su);

	tick = va_arg(ap, t_tick);

	if (su->alive && (sg = su->group) && skill_get_inf2(sg->skill_id, INF2_ISTRAP)) {
		switch( sg->unit_id ) {
			case UNT_CLAYMORETRAP:
			case UNT_FIRINGTRAP:
			case UNT_ICEBOUNDTRAP:
				map_foreachinrange(skill_trap_splash,&su->bl, skill_get_splash(sg->skill_id, sg->skill_lv), sg->bl_flag|BL_SKILL|~BCT_SELF, &su->bl,tick);
				break;
			case UNT_LANDMINE:
			case UNT_BLASTMINE:
			case UNT_SHOCKWAVE:
			case UNT_SANDMAN:
			case UNT_FLASHER:
			case UNT_FREEZINGTRAP:
			case UNT_CLUSTERBOMB:
				if (battle_config.skill_wall_check && !skill_get_nk(sg->skill_id, NK_NODAMAGE))
					map_foreachinshootrange(skill_trap_splash,&su->bl, skill_get_splash(sg->skill_id, sg->skill_lv), sg->bl_flag, &su->bl,tick);
				else
					map_foreachinallrange(skill_trap_splash,&su->bl, skill_get_splash(sg->skill_id, sg->skill_lv), sg->bl_flag, &su->bl,tick);
				break;
		}
		// Traps aren't recovered.
		skill_delunit(su);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------*/
int skill_blockpc_get(struct map_session_data *sd, int skillid) {
	int i;

	nullpo_retr(-1, sd);

	ARR_FIND(0, MAX_SKILLCOOLDOWN, i, sd->scd[i] && sd->scd[i]->skill_id == skillid);
	return (i >= MAX_SKILLCOOLDOWN) ? -1 : i;
}

TIMER_FUNC(skill_blockpc_end){
	struct map_session_data *sd = map_id2sd(id);
	int i = (int)data;

	if (!sd || data < 0 || data >= MAX_SKILLCOOLDOWN)
		return 0;

	if (!sd->scd[i] || sd->scd[i]->timer != tid) {
		ShowWarning("skill_blockpc_end: Invalid Timer or not Skill Cooldown.\n");
		return 0;
	}

	aFree(sd->scd[i]);
	sd->scd[i] = NULL;
		return 1;
}

/**
 * Flags a singular skill as being blocked from persistent usage.
 * @param   sd        the player the skill delay affects
 * @param   skill_id   the skill which should be delayed
 * @param   tick      the length of time the delay should last
 * @param   load      whether this assignment is being loaded upon player login
 * @return  0 if successful, -1 otherwise
 */
int skill_blockpc_start(struct map_session_data *sd, int skill_id, t_tick tick) {
	int i;

	nullpo_retr(-1, sd);

	if (!skill_id || tick < 1)
		return -1;

	ARR_FIND(0, MAX_SKILLCOOLDOWN, i, sd->scd[i] && sd->scd[i]->skill_id == skill_id);
	if (i < MAX_SKILLCOOLDOWN) { // Skill already with cooldown
		delete_timer(sd->scd[i]->timer, skill_blockpc_end);
		aFree(sd->scd[i]);
		sd->scd[i] = NULL;
	}

	ARR_FIND(0, MAX_SKILLCOOLDOWN, i, !sd->scd[i]);
	if (i < MAX_SKILLCOOLDOWN) { // Free Slot found
		CREATE(sd->scd[i], struct skill_cooldown_entry, 1);
		sd->scd[i]->skill_id = skill_id;
		sd->scd[i]->timer = add_timer(gettick() + tick, skill_blockpc_end, sd->bl.id, i);

		if (battle_config.display_status_timers && tick > 0)
			clif_skill_cooldown(sd, skill_id, tick);

		return 1;
	} else {
		ShowWarning("skill_blockpc_start: Too many skillcooldowns, increase MAX_SKILLCOOLDOWN.\n");
		return 0;
	}
}

int skill_blockpc_clear(struct map_session_data *sd) {
	int i;

	nullpo_ret(sd);

	for (i = 0; i < MAX_SKILLCOOLDOWN; i++) {
		if (!sd->scd[i])
			continue;
		delete_timer(sd->scd[i]->timer, skill_blockpc_end);
		aFree(sd->scd[i]);
		sd->scd[i] = NULL;
	}
	return 1;
}

TIMER_FUNC(skill_blockhomun_end){
	

	return 1;
}

int skill_blockhomun_start(struct homun_data *hd, uint16 skill_id, int tick)	//[orn]
{
	
	return 1;
}

TIMER_FUNC(skill_blockmerc_end){
	
	return 1;
}

int skill_blockmerc_start(struct mercenary_data *md, uint16 skill_id, int tick)
{
	return 1;
}
/**
 * Adds a new skill unit entry for this player to recast after map load
 * @param sd: Player
 * @param skill_id: Skill ID to save
 * @param skill_lv: Skill level to save
 */
void skill_usave_add(struct map_session_data *sd, uint16 skill_id, uint16 skill_lv)
{
	struct skill_usave *sus = NULL;

	if (idb_exists(skillusave_db,sd->status.char_id))
		idb_remove(skillusave_db,sd->status.char_id);

	CREATE(sus, struct skill_usave, 1);
	idb_put(skillusave_db, sd->status.char_id, sus);

	sus->skill_id = skill_id;
	sus->skill_lv = skill_lv;
}

/**
 * Loads saved skill unit entries for this player after map load
 * @param sd: Player
 */
void skill_usave_trigger(struct map_session_data *sd)
{
	struct skill_usave *sus = NULL;
	struct skill_unit_group *group = NULL;

	if (!(sus = static_cast<skill_usave *>(idb_get(skillusave_db,sd->status.char_id))))
		return;

	if ((group = skill_unitsetting(&sd->bl, sus->skill_id, sus->skill_lv, sd->bl.x, sd->bl.y, 0)))
		if (sus->skill_id == SK_MS_NEUTRALBARRIER  )
			sc_start2(&sd->bl, &sd->bl, (sus->skill_id == SK_MS_NEUTRALBARRIER ? STATUS_NEUTRALBARRIER_MASTER : STATUS_NEUTRALBARRIER_MASTER), 100, sus->skill_lv, group->group_id, skill_get_time(sus->skill_id, sus->skill_lv));
	idb_remove(skillusave_db, sd->status.char_id);
}

/*
 *
 */
int skill_split_str (char *str, char **val, int num)
{
	int i;

	for( i = 0; i < num && str; i++ ) {
		val[i] = str;
		str = strchr(str,',');
		if( str )
			*str++ = 0;
	}

	return i;
}

/*
 *
 */
void skill_init_unit_layout (void) {
	int i,j,pos = 0;

	memset(skill_unit_layout,0,sizeof(skill_unit_layout));

	// standard square layouts go first
	for (i=0; i<=MAX_SQUARE_LAYOUT; i++) {
		int size = i*2+1;
		skill_unit_layout[i].count = size*size;
		for (j=0; j<size*size; j++) {
			skill_unit_layout[i].dx[j] = (j%size-i);
			skill_unit_layout[i].dy[j] = (j/size-i);
		}
	}

	// afterwards add special ones
	pos = i;
	for (const auto &it : skill_db) {
		std::shared_ptr<s_skill_db> skill = it.second;
		uint16 skill_id = skill->nameid;

		if (!skill->unit_id || skill->unit_layout_type[0] != -1)
			continue;

			switch (skill_id) {
				case SK_MG_ICEWALL:
					// these will be handled later
					break;
				case SK_PR_SANCTUARIO: {
						static const int dx[] = {
							-1, 0, 1,-2,-1, 0, 1, 2,-2,-1,
							 0, 1, 2,-2,-1, 0, 1, 2,-1, 0, 1};
						static const int dy[]={
							-2,-2,-2,-1,-1,-1,-1,-1, 0, 0,
							 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2};
						skill_unit_layout[pos].count = 21;
						memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
						memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
					}
					break;
				case SK_PR_MAGNUSEXORCISMUS: {
						static const int dx[] = {
							-1, 0, 1,-1, 0, 1,-3,-2,-1, 0,
							 1, 2, 3,-3,-2,-1, 0, 1, 2, 3,
							-3,-2,-1, 0, 1, 2, 3,-1, 0, 1,-1, 0, 1};
						static const int dy[] = {
							-3,-3,-3,-2,-2,-2,-1,-1,-1,-1,
							-1,-1,-1, 0, 0, 0, 0, 0, 0, 0,
							 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3};
						skill_unit_layout[pos].count = 33;
						memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
						memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
					}
					break;

				case SK_CR_GRANDCROSS:{
						static const int dx[] = {
							 0, 0,-1, 0, 1,-2,-1, 0, 1, 2,
							-4,-3,-2,-1, 0, 1, 2, 3, 4,-2,
							-1, 0, 1, 2,-1, 0, 1, 0, 0};
						static const int dy[] = {
							-4,-3,-2,-2,-2,-1,-1,-1,-1,-1,
							 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
							 1, 1, 1, 1, 2, 2, 2, 3, 4};
						skill_unit_layout[pos].count = 29;
						memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
						memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
					}
					break;

				case SK_PA_GOSPEL: {
						static const int dx[] = {
							-1, 0, 1,-1, 0, 1,-3,-2,-1, 0,
							 1, 2, 3,-3,-2,-1, 0, 1, 2, 3,
							-3,-2,-1, 0, 1, 2, 3,-1, 0, 1,
							-1, 0, 1};
						static const int dy[] = {
							-3,-3,-3,-2,-2,-2,-1,-1,-1,-1,
							-1,-1,-1, 0, 0, 0, 0, 0, 0, 0,
							 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
							 3, 3, 3};
						skill_unit_layout[pos].count = 33;
						memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
						memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
					}
					break;

				
				case SK_AM_WILDTHORNS: {
						static const int dx[] = {
							-1, 0, 1,-1, 0, 1,-3,-2,-1, 0,
							 1, 2, 3,-3,-2,-1, 0, 1, 2, 3,
							-3,-2,-1, 0, 1, 2, 3,-1, 0, 1,-1, 0, 1};
						static const int dy[] = {
							-3,-3,-3,-2,-2,-2,-1,-1,-1,-1,
							-1,-1,-1, 0, 0, 0, 0, 0, 0, 0,
							 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3};
						skill_unit_layout[pos].count = 33;
						memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
						memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
					}
					break;
				
				default:
					ShowError("unknown unit layout at skill %d\n",i);
					break;
			}
		
		if (!skill_unit_layout[pos].count)
			continue;
		for (j=0;j<MAX_SKILL_LEVEL;j++)
			skill->unit_layout_type[j] = pos;
		pos++;
	}

	// firewall and icewall have 8 layouts (direction-dependent)
	firewall_unit_pos = pos;
	for (i=0;i<8;i++) {
		if (i&1) {
			skill_unit_layout[pos].count = 5;
			if (i&0x2) {
				int dx[] = {-1,-1, 0, 0, 1};
				int dy[] = { 1, 0, 0,-1,-1};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			} else {
				int dx[] = { 1, 1 ,0, 0,-1};
				int dy[] = { 1, 0, 0,-1,-1};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
		} else {
			skill_unit_layout[pos].count = 3;
			if (i%4==0) {
				int dx[] = {-1, 0, 1};
				int dy[] = { 0, 0, 0};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			} else {
				int dx[] = { 0, 0, 0};
				int dy[] = {-1, 0, 1};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
		}
		pos++;
	}
	icewall_unit_pos = pos;
	for (i=0;i<8;i++) {
		skill_unit_layout[pos].count = 5;
		if (i&1) {
			if (i&0x2) {
				int dx[] = {-2,-1, 0, 1, 2};
				int dy[] = { 2, 1, 0,-1,-2};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			} else {
				int dx[] = { 2, 1 ,0,-1,-2};
				int dy[] = { 2, 1, 0,-1,-2};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
		} else {
			if (i%4==0) {
				int dx[] = {-2,-1, 0, 1, 2};
				int dy[] = { 0, 0, 0, 0, 0};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			} else {
				int dx[] = { 0, 0, 0, 0, 0};
				int dy[] = {-2,-1, 0, 1, 2};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
		}
		pos++;
	}
	earthstrain_unit_pos = pos;
	for( i = 0; i < 8; i++ )
	{ // For each Direction
		skill_unit_layout[pos].count = 15;
		switch( i )
		{
		case 0: case 1: case 3: case 4: case 5: case 7:
			{
				int dx[] = {-7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7};
				int dy[] = { 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
			break;
		case 2:
		case 6:
			{
				int dx[] = { 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0};
				int dy[] = {-7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
			break;
		}
		pos++;
	}
	firerain_unit_pos = pos;
	for( i = 0; i < 8; i++ ) {
		skill_unit_layout[pos].count = 3;
		switch( i ) {
			case 0: case 1: case 3: case 4: case 5: case 7:
				{
					static const int dx[] = {-1, 0, 1};
					static const int dy[] = { 0, 0, 0};

					memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				}
				break;
			case 2:
			case 6:
				{
					static const int dx[] = { 0, 0, 0};
					static const int dy[] = {-1, 0, 1};

					memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				}
				break;
		}
		pos++;
	}

	if( pos >= MAX_SKILL_UNIT_LAYOUT )
		ShowError("skill_init_unit_layout: The skill_unit_layout has met the limit or overflowed (pos=%d)\n", pos);
}

void skill_init_nounit_layout (void) {
	int i, pos = 0;

	memset(skill_nounit_layout,0,sizeof(skill_nounit_layout));

	overbrand_nounit_pos = pos;
	for( i = 0; i < 8; i++ ) {
		if( i&1 ) {
			skill_nounit_layout[pos].count = 33;
			if( i&2 ) {
				if( i&4 ) { // 7
					int dx[] = { 5, 6, 7, 5, 6, 4, 5, 6, 4, 5, 3, 4, 5, 3, 4, 2, 3, 4, 2, 3, 1, 2, 3, 1, 2, 0, 1, 2, 0, 1,-1, 0, 1};
					int dy[] = { 7, 6, 5, 6, 5, 6, 5, 4, 5, 4, 5, 4, 3, 4, 3, 4, 3, 2, 3, 2, 3, 2, 1, 2, 1, 2, 1, 0, 1, 0, 1, 0,-1};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				} else { // 3
					int dx[] = {-5,-6,-7,-5,-6,-4,-5,-6,-4,-5,-3,-4,-5,-3,-4,-2,-3,-4,-2,-3,-1,-2,-3,-1,-2, 0,-1,-2, 0,-1, 1, 0,-1};
					int dy[] = {-7,-6,-5,-6,-5,-6,-5,-4,-5,-4,-5,-4,-3,-4,-3,-4,-3,-2,-3,-2,-3,-2,-1,-2,-1,-2,-1, 0,-1, 0,-1, 0, 1};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				}
			} else {
				if( i&4 ) { // 5
					int dx[] = { 7, 6, 5, 6, 5, 6, 5, 4, 5, 4, 5, 4, 3, 4, 3, 4, 3, 2, 3, 2, 3, 2, 1, 2, 1, 2, 1, 0, 1, 0, 1, 0,-1};
					int dy[] = {-5,-6,-7,-5,-6,-4,-5,-6,-4,-5,-3,-4,-5,-3,-4,-2,-3,-4,-2,-3,-1,-2,-3,-1,-2, 0,-1,-2, 0,-1, 1, 0,-1};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				} else { // 1
					int dx[] = {-7,-6,-5,-6,-5,-6,-5,-4,-5,-4,-5,-4,-3,-4,-3,-4,-3,-2,-3,-2,-3,-2,-1,-2,-1,-2,-1, 0,-1, 0,-1, 0, 1};
					int dy[] = { 5, 6, 7, 5, 6, 4, 5, 6, 4, 5, 3, 4, 5, 3, 4, 2, 3, 4, 2, 3, 1, 2, 3, 1, 2, 0, 1, 2, 0, 1,-1, 0, 1};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				}
			}
		} else {
			skill_nounit_layout[pos].count = 21;
			if( i&2 ) {
				if( i&4 ) { // 6
					int dx[] = { 0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6};
					int dy[] = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				} else { // 2
					int dx[] = {-6,-5,-4,-3,-2,-1, 0,-6,-5,-4,-3,-2,-1, 0,-6,-5,-4,-3,-2,-1, 0};
					int dy[] = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				}
			} else {
				if( i&4 ) { // 4
					int dx[] = {-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1};
					int dy[] = { 0, 0, 0,-1,-1,-1,-2,-2,-2,-3,-3,-3,-4,-4,-4,-5,-5,-5,-6,-6,-6};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				} else { // 0
					int dx[] = {-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1};
					int dy[] = { 6, 6, 6, 5, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 0, 0, 0};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				}
			}
		}
		pos++;
	}

	overbrand_brandish_nounit_pos = pos;
	for( i = 0; i < 8; i++ ) {
		if( i&1 ) {
			skill_nounit_layout[pos].count = 74;
			if( i&2 ) {
				if( i&4 ) { // 7
					int dx[] = {-2,-1, 0, 1, 2, 3, 4, 5, 6, 7, 8,-2,-1, 0, 1, 2, 3, 4, 5, 6, 7,
								-3,-2,-1, 0, 1, 2, 3, 4, 5, 6, 7,-3,-2,-1,-0, 1, 2, 3, 4, 5, 6,
								-4,-3,-2,-1, 0, 1, 2, 3, 4, 5, 6,-4,-3,-2,-1,-0, 1, 2, 3, 4, 5,
								-5,-4,-3,-2,-1, 0, 1, 2, 3, 4, 5};
					int dy[] = { 8, 7, 6, 5, 4, 3, 2, 1, 0,-1,-2, 7, 6, 5, 4, 3, 2, 1, 0,-1,-2,
								 7, 6, 5, 4, 3, 2, 1, 0,-1,-2,-3, 6, 5, 4, 3, 2, 1, 0,-1,-2,-3,
								 6, 5, 4, 3, 2, 1, 0,-1,-2,-3,-4, 5, 4, 3, 2, 1, 0,-1,-2,-3,-4,
								 5, 4, 3, 2, 1, 0,-1,-2,-3,-4,-5};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				} else { // 3
					int dx[] = { 2, 1, 0,-1,-2,-3,-4,-5,-6,-7,-8, 2, 1, 0,-1,-2,-3,-4,-5,-6,-7,
								 3, 2, 1, 0,-1,-2,-3,-4,-5,-6,-7, 3, 2, 1, 0,-1,-2,-3,-4,-5,-6,
								 4, 3, 2, 1, 0,-1,-2,-3,-4,-5,-6, 4, 3, 2, 1, 0,-1,-2,-3,-4,-5,
								 5, 4, 3, 2, 1, 0,-1,-2,-3,-4,-5};
					int dy[] = {-8,-7,-6,-5,-4,-3,-2,-1, 0, 1, 2,-7,-6,-5,-4,-3,-2,-1, 0, 1, 2,
								-7,-6,-5,-4,-3,-2,-1, 0, 1, 2, 3,-6,-5,-4,-3,-2,-1, 0, 1, 2, 3,
								-6,-5,-4,-3,-2,-1, 0, 1, 2, 3, 4,-5,-4,-3,-2,-1, 0, 1, 2, 3, 4,
								-5,-4,-3,-2,-1, 0, 1, 2, 3, 4, 5};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				}
			} else {
				if( i&4 ) { // 5
					int dx[] = { 8, 7, 6, 5, 4, 3, 2, 1, 0,-1,-2, 7, 6, 5, 4, 3, 2, 1, 0,-1,-2,
								 7, 6, 5, 4, 3, 2, 1, 0,-1,-2,-3, 6, 5, 4, 3, 2, 1, 0,-1,-2,-3,
								 6, 5, 4, 3, 2, 1, 0,-1,-2,-3,-4, 5, 4, 3, 2, 1, 0,-1,-2,-3,-4,
								 5, 4, 3, 2, 1, 0,-1,-2,-3,-4,-5};
					int dy[] = { 2, 1, 0,-1,-2,-3,-4,-5,-6,-7,-8, 2, 1, 0,-1,-2,-3,-4,-5,-6,-7,
								 3, 2, 1, 0,-1,-2,-3,-4,-5,-6,-7, 3, 2, 1, 0,-1,-2,-3,-4,-5,-6,
								 4, 3, 2, 1, 0,-1,-2,-3,-4,-5,-6, 4, 3, 2, 1, 0,-1,-2,-3,-4,-5,
								 5, 4, 3, 2, 1, 0,-1,-2,-3,-4,-5};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				} else { // 1
					int dx[] = {-8,-7,-6,-5,-4,-3,-2,-1, 0, 1, 2,-7,-6,-5,-4,-3,-2,-1, 0, 1, 2,
								-7,-6,-5,-4,-3,-2,-1, 0, 1, 2, 3,-6,-5,-4,-3,-2,-1, 0, 1, 2, 3,
								-6,-5,-4,-3,-2,-1, 0, 1, 2, 3, 4,-5,-4,-3,-2,-1, 0, 1, 2, 3, 4,
								-5,-4,-3,-2,-1, 0, 1, 2, 3, 4, 5};
					int dy[] = {-2,-1, 0, 1, 2, 3, 4, 5, 6, 7, 8,-2,-1, 0, 1, 2, 3, 4, 5, 6, 7,
								-3,-2,-1, 0, 1, 2, 3, 4, 5, 6, 7,-3,-2,-1, 0, 1, 2, 3, 4, 5, 6,
								-4,-3,-2,-1, 0, 1, 2, 3, 4, 5, 6,-4,-3,-2,-1, 0, 1, 2, 3, 4, 5,
								-5,-4,-3,-2,-1, 0, 1, 2, 3, 4, 5};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				}
			}
		} else {
			skill_nounit_layout[pos].count = 44;
			if( i&2 ) {
				if( i&4 ) { // 6
					int dx[] = { 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3};
					int dy[] = { 5, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0,-1,-1,-1,-1,-2,-2,-2,-2,-3,-3,-3,-3,-4,-4,-4,-4,-5,-5,-5,-5};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				} else { // 2
					int dx[] = {-3,-2,-1, 0,-3,-2,-1, 0,-3,-2,-1, 0,-3,-2,-1, 0,-3,-2,-1, 0,-3,-2,-1, 0,-3,-2,-1, 0,-3,-2,-1, 0,-3,-2,-1, 0,-3,-2,-1, 0,-3,-2,-1, 0};
					int dy[] = { 5, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0,-1,-1,-1,-1,-2,-2,-2,-2,-3,-3,-3,-3,-4,-4,-4,-4,-5,-5,-5,-5};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				}
			} else {
				if( i&4 ) { // 4
					int dx[] = { 5, 4, 3, 2, 1, 0,-1,-2,-3,-4,-5, 5, 4, 3, 2, 1, 0,-1,-2,-3,-4,-5, 5, 4, 3, 2, 1, 0,-1,-2,-3,-4,-5, 5, 4, 3, 2, 1, 0,-1,-2,-3,-4,-5};
					int dy[] = {-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				} else { // 0
					int dx[] = {-5,-4,-3,-2,-1, 0, 1, 2, 3, 4, 5,-5,-4,-3,-2,-1, 0, 1, 2, 3, 4, 5,-5,-4,-3,-2,-1, 0, 1, 2, 3, 4, 5,-5,-4,-3,-2,-1, 0, 1, 2, 3, 4, 5};
					int dy[] = { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

					memcpy(skill_nounit_layout[pos].dx,dx,sizeof(dx));
					memcpy(skill_nounit_layout[pos].dy,dy,sizeof(dy));
				}
			}
		}
		pos++;
	}

	if( pos >= MAX_SKILL_UNIT_LAYOUT2 )
		ShowError("skill_init_nounit_layout: The skill_nounit_layout has met the limit or overflowed (pos=%d)\n", pos);
}

int skill_block_check(struct block_list *bl, sc_type type , uint16 skill_id) {
	struct status_change *sc = status_get_sc(bl);

	if( !sc || !bl || !skill_id )
		return 0; // Can do it

	switch (type) {
		
		case STATUS_STASIS:
			if (bl->type == BL_PC && !skill_get_inf2(skill_id, INF2_IGNORESTASIS))
				return 1; // Can't do it.
			break;
		
	}

	return 0;
}

/* Determines whether a skill is currently active or not
 * Used for purposes of cancelling SP usage when disabling a skill
 */
int skill_disable_check(struct status_change *sc, uint16 skill_id)
{
	switch( skill_id ) { //HP & SP Consumption Check
		case SK_BS_MAXIMIZEPOWER:
		case SK_NV_TRICKDEAD:
		case SK_TF_HIDING:
		case SK_AS_CLOAKING:
		case SK_EX_INVISIBILITY:
		case SK_AS_STEALTH:
		case SK_PA_DEFENDINGAURA:
		case SK_CR_AUTOGUARD:
		case SK_PA_GOSPEL:
		case SK_HT_CAMOUFLAGE:
			if( sc->data[status_skill2sc(skill_id)] )
				return 1;
			break;

		// These 2 skills contain a master and are not correctly pulled using skill2sc
		case SK_MS_NEUTRALBARRIER:
			if( sc->data[STATUS_NEUTRALBARRIER_MASTER] )
				return 1;
			break;
		
	}

	return 0;
}

int skill_get_elemental_type( uint16 skill_id , uint16 skill_lv ) {
	int type = 0;

	if (skill_id == SK_HT_WARGTRAINING) { //WARG
		type = ELEMENTALID_AQUA_S;
	}
	if (skill_id == SK_HT_FALCONRY) { //FALCON
		type = ELEMENTALID_VENTUS_S;
	}

	if (skill_id == SK_SA_SUMMONELEMENTAL) { //ELEMENTAL
		switch( skill_lv ) {
			case 4:	type = ELEMENTALID_AGNI_L;		break;
			case 3:	type = ELEMENTALID_AQUA_L;		break;
			case 2:	type = ELEMENTALID_VENTUS_L;	break;
			case 1:	type = ELEMENTALID_TERA_L;		break;
		}
	}
	if (skill_id == SK_AM_HATCHHOMUNCULUS) { //HOMUN
		switch( skill_lv ) {
			case 4:	type = ELEMENTALID_AQUA_M;		break;
			case 3:	type = ELEMENTALID_AGNI_M;		break;
			case 2:	type = ELEMENTALID_TERA_M;	    break;
			case 1:	type = ELEMENTALID_VENTUS_M;		break;
		}
	}

	// type += skill_lv - 1;

	return type;
}

/**
 * Check before do `unit_movepos` call
 * @param check_flag Flags: 1:Check for BG maps, 2:Check for GVG maps on WOE times, 4:Check for GVG maps regardless Agit flags
 * @return True:If unit can be moved, False:If check on flags are met or unit cannot be moved.
 **/
static bool skill_check_unit_movepos(uint8 check_flag, struct block_list *bl, short dst_x, short dst_y, int easy, bool checkpath) {
	struct status_change *sc;

	nullpo_retr(false, bl);

	struct map_data *mapdata = map_getmapdata(bl->m);

	if (check_flag&1 && mapdata->flag[MF_BATTLEGROUND])
		return false;
	if (check_flag&2 && mapdata_flag_gvg(mapdata))
		return false;
	if (check_flag&4 && mapdata_flag_gvg2(mapdata))
		return false;

	sc = status_get_sc(bl);
	

	return unit_movepos(bl, dst_x, dst_y, easy, checkpath);
}

/**
 * Get skill duration after adjustments by skill_duration mapflag
 * @param mapdata: Source map data
 * @param skill_id: Skill ID
 * @param skill_lv: Skill level
 * @return Adjusted skill duration
 */
int skill_get_time3(struct map_data *mapdata, uint16 skill_id, uint16 skill_lv) {
	int time = 0;

	if (!(time = skill_get_time(skill_id, skill_lv)))
		return 0;

	if (mapdata && !mapdata->skill_duration.empty() && mapdata->skill_duration.find(skill_id) != mapdata->skill_duration.end())
		return time / 100 * mapdata->skill_duration[skill_id];
	return time;
}

const std::string SkillDatabase::getDefaultLocation() {
	return std::string(db_path) + "/skill_db.yml";
}

template<typename T, size_t S> bool SkillDatabase::parseNode(std::string nodeName, std::string subNodeName, YAML::Node node, T (&arr)[S]) {
	int32 value;

	if (node[nodeName].IsScalar()) {
		if (!this->asInt32(node, nodeName, value))
			return false;

		for (size_t i = 0; i < S; i++)
			arr[i] = value;
	} else {
		uint16 max_level = 0;

		for (const YAML::Node &it : node[nodeName]) {
			uint16 skill_lv;

			if (!this->asUInt16(it, "Level", skill_lv))
				continue;

			if (skill_lv > MAX_SKILL_LEVEL) {
				this->invalidWarning(it["Level"], "%s Level exceeds the maximum skill level of %d, skipping.\n", nodeName.c_str(), MAX_SKILL_LEVEL);
				return false;
			}

			if (!this->asInt32(it, subNodeName, value))
				continue;

			arr[skill_lv - 1] = value;
			max_level = max(max_level, skill_lv);
		}

		size_t i = max_level, j;

		// Check for linear change with increasing steps until we reach half of the data acquired.
		for (size_t step = 1; step <= i / 2; step++) {
			int diff = arr[i - 1] - arr[i - step - 1];

			for (j = i - 1; j >= step; j--) {
				if ((arr[j] - arr[j - step]) != diff)
					break;
			}

			if (j >= step) // No match, try next step.
				continue;

			for (; i < MAX_SKILL_LEVEL; i++) { // Apply linear increase
				arr[i] = arr[i - step] + diff;

				if (arr[i] < 1 && arr[i - 1] >= 0) { // Check if we have switched from + to -, cap the decrease to 0 in said cases.
					arr[i] = 1;
					diff = 0;
					step = 1;
				}
			}

			return true;
		}

		// Unable to determine linear trend, fill remaining array values with last value
		for (; i < S; i++)
			arr[i] = arr[max_level - 1];
	}

	return true;
}

/**
 * Reads and parses an entry from the skill_db.
 * @param node: YAML node containing the entry.
 * @return count of successfully parsed rows
 */
uint64 SkillDatabase::parseBodyNode(const YAML::Node &node) {
	uint16 skill_id;

	if (!this->asUInt16(node, "Id", skill_id))
		return 0;

	std::shared_ptr<s_skill_db> skill = this->find(skill_id);
	bool exists = skill != nullptr;

	if (!exists) {
		if (!this->nodesExist(node, { "Name", "Description", "MaxLevel" }))
			return 0;

		skill = std::make_shared<s_skill_db>();
		skill->nameid = skill_id;
	}

	if (this->nodeExists(node, "Name")) {
		std::string name;

		if (!this->asString(node, "Name", name))
			return 0;

		name.resize(SKILL_NAME_LENGTH);
		memcpy(skill->name, name.c_str(), sizeof(skill->name));
	}

	if (this->nodeExists(node, "Description")) {
		std::string name;

		if (!this->asString(node, "Description", name))
			return 0;

		name.resize(SKILL_DESC_LENGTH);
		memcpy(skill->desc, name.c_str(), sizeof(skill->desc));
	}

	if (this->nodeExists(node, "MaxLevel")) {
		uint16 skill_lv;

		if (!this->asUInt16(node, "MaxLevel", skill_lv))
			return 0;

		if (skill_lv == 0 || skill_lv > MAX_SKILL_LEVEL) {
			this->invalidWarning(node["MaxLevel"], "MaxLevel %hu does not meet the bounds of 1~%d.\n", skill_lv, MAX_SKILL_LEVEL);
			return 0;
		}

		skill->max = skill_lv;
	}

	if (this->nodeExists(node, "Type")) {
		std::string type;

		if (!this->asString(node, "Type", type))
			return 0;

		std::string type_constant = "BF_" + type;

		int64 constant;

		if (!script_get_constant(type_constant.c_str(), &constant)) {
			this->invalidWarning(node["Type"], "Type %s is invalid.\n", type.c_str());
			return 0;
		}

		if (constant < BF_NONE || constant > BF_MISC) {
			this->invalidWarning(node["Type"], "Constant Type %s is not a supported skill type.\n", type_constant.c_str());
			return 0;
		}

		skill->skill_type = static_cast<e_battle_flag>(constant);
	} else {
		if (!exists)
			skill->skill_type = BF_NONE;
	}

	if (this->nodeExists(node, "TargetType")) {
		std::string inf;

		if (!this->asString(node, "TargetType", inf))
			return 0;

		std::string inf_constant = "INF_" + inf + "_SKILL";
		int64 constant;

		if (!script_get_constant(inf_constant.c_str(), &constant)) {
			this->invalidWarning(node["TargetType"], "TargetType %s is invalid.\n", inf.c_str());
			return 0;
		}

		skill->inf = static_cast<uint16>(constant);
	}

	if (this->nodeExists(node, "DamageFlags")) {
		const YAML::Node &damageNode = node["DamageFlags"];

		for (const auto &it : damageNode) {
			std::string nk = it.first.as<std::string>(), nk_constant = "NK_" + nk;
			int64 constant;

			if (!script_get_constant(nk_constant.c_str(), &constant)) {
				this->invalidWarning(damageNode, "DamageFlags %s is invalid.\n", nk.c_str());
				return 0;
			}

			bool active;

			if (!this->asBool(damageNode, nk, active))
				return 0;

			if (active)
				skill->nk.set(static_cast<uint8>(constant));
			else
				skill->nk.reset(static_cast<uint8>(constant));
		}
	}

	if (this->nodeExists(node, "Flags")) {
		const YAML::Node &infoNode = node["Flags"];

		for (const auto &it : infoNode) {
			std::string inf2 = it.first.as<std::string>(), inf2_constant = "INF2_" + inf2;
			int64 constant;

			if (!script_get_constant(inf2_constant.c_str(), &constant)) {
				this->invalidWarning(infoNode, "Flag %s is invalid, skipping.\n", inf2.c_str());
				continue;
			}

			bool active;

			if (!this->asBool(infoNode, inf2, active))
				return 0;

			if (active)
				skill->inf2.set(static_cast<uint8>(constant));
			else
				skill->inf2.reset(static_cast<uint8>(constant));
		}
	}

	if (this->nodeExists(node, "Range")) {
		if (!this->parseNode("Range", "Size", node, skill->range))
			return 0;
	} else {
		if (!exists)
			memset(skill->range, 0, sizeof(skill->range));
	}

	if (this->nodeExists(node, "Hit")) {
		std::string hit;

		if (!this->asString(node, "Hit", hit))
			return 0;

		std::string hit_constant = "DMG_" + hit;
		int64 constant;

		if (!script_get_constant(hit_constant.c_str(), &constant)) {
			this->invalidWarning(node["Hit"], "Hit %s is invalid.\n", hit.c_str());
			return 0;
		}

		skill->hit = static_cast<e_damage_type>(constant);
	} else {
		if (!exists)
			skill->hit = DMG_NORMAL;
	}

	if (this->nodeExists(node, "HitCount")) {
		if (!this->parseNode("HitCount", "Count", node, skill->num))
			return 0;
	} else {
		if (!exists)
			memset(skill->num, 0, sizeof(skill->num));
	}

	if (this->nodeExists(node, "Element")) {
		const YAML::Node &elementNode = node["Element"];
		std::string element;

		if (elementNode.IsScalar()) {
			if (!this->asString(node, "Element", element))
				return 0;

			std::string element_constant = "ELE_" + element;
			int64 constant;

			if (!script_get_constant(element_constant.c_str(), &constant)) {
				this->invalidWarning(elementNode["Element"], "Element %s is invalid.\n", element.c_str());
				return 0;
			}

			if (constant == ELE_NONE) {
				this->invalidWarning(elementNode["Element"], "%s is not a valid element.\n", element.c_str());
				return 0;
			}

			memset(skill->element, static_cast<e_element>(constant), sizeof(skill->element));
		} else {
			for (const YAML::Node &it : elementNode) {
				uint16 skill_lv;

				if (!this->asUInt16(it, "Level", skill_lv))
					continue;

				if (skill_lv > MAX_SKILL_LEVEL) {
					this->invalidWarning(it["Level"], "Element Level exceeds the maximum skill level of %d, skipping.\n", MAX_SKILL_LEVEL);
					return false;
				}

				if (!this->asString(it, "Element", element))
					continue;

				std::string element_constant = "ELE_" + element;
				int64 constant;

				if (!script_get_constant(element_constant.c_str(), &constant)) {
					this->invalidWarning(elementNode["Element"], "Element %s is invalid.\n", element.c_str());
					return 0;
				}

				if (constant == ELE_NONE) {
					this->invalidWarning(elementNode["Element"], "%s is not a valid element.\n", element.c_str());
					return 0;
				}

				skill->element[skill_lv - 1] = static_cast<e_element>(constant);
			}
		}
	} else {
		if (!exists)
			memset(skill->element, ELE_NEUTRAL, sizeof(skill->element));
	}

	if (this->nodeExists(node, "SplashArea")) {
		if (!this->parseNode("SplashArea", "Area", node, skill->splash))
			return 0;
	} else {
		if (!exists)
			memset(skill->splash, 0, sizeof(skill->splash));
	}

	if (this->nodeExists(node, "ActiveInstance")) {
		if (!this->parseNode("ActiveInstance", "Max", node, skill->maxcount))
			return 0;
	} else {
		if (!exists)
			memset(skill->maxcount, 0, sizeof(skill->maxcount));
	}

	if (this->nodeExists(node, "Knockback")) {
		if (!this->parseNode("Knockback", "Amount", node, skill->blewcount))
			return 0;
	} else {
		if (!exists)
			memset(skill->blewcount, 0, sizeof(skill->blewcount));
	}

	if (this->nodeExists(node, "CopyFlags")) {
		const YAML::Node &copyNode = node["CopyFlags"];

		if (this->nodeExists(copyNode, "Skill")) {
			const YAML::Node &copyskillNode = copyNode["Skill"];

			if (this->nodeExists(copyskillNode, "Plagiarism")) {
				bool active;

				if (!this->asBool(copyskillNode, "Plagiarism", active))
					return 0;

				if (active)
					skill->copyable.option |= SKILL_COPY_PLAGIARISM;
				else
					skill->copyable.option &= SKILL_COPY_PLAGIARISM;
			}

			if (this->nodeExists(copyskillNode, "Reproduce")) {
				bool active;

				if (!this->asBool(copyskillNode, "Reproduce", active))
					return 0;

				if (active)
					skill->copyable.option |= SKILL_COPY_REPRODUCE;
				else
					skill->copyable.option &= SKILL_COPY_REPRODUCE;
			}
		} else {
			this->invalidWarning(copyNode, "CopyFlags requires a Skill copy type.\n");
			return 0;
		}

		if (this->nodeExists(copyNode, "RemoveRequirement")) {
			const YAML::Node &copyreqNode = copyNode["RemoveRequirement"];

			for (const auto &it : copyreqNode) {
				std::string req = it.first.as<std::string>(), req_constant = "SKILL_REQ_" + req;
				int64 constant;

				if (!script_get_constant(req_constant.c_str(), &constant)) {
					this->invalidWarning(copyreqNode, "CopyFlags RemoveRequirement %s is invalid.\n", req.c_str());
					return 0;
				}

				skill->copyable.req_opt |= constant;
			}
		} else {
			if (!exists)
				skill->copyable.req_opt = 0;
		}
	}

	if (this->nodeExists(node, "NoNearNpc")) {
		const YAML::Node &npcNode = node["NoNearNpc"];

		if (this->nodeExists(npcNode, "AdditionalRange")) {
			uint16 range;

			if (!this->asUInt16(npcNode, "AdditionalRange", range))
				return 0;

			skill->unit_nonearnpc_range = range;
		} else {
			if (!exists)
				skill->unit_nonearnpc_range = 0;
		}

		if (this->nodeExists(npcNode, "Type")) {
			const YAML::Node &npctypeNode = npcNode["Type"];

			for (const auto &it : npctypeNode) {
				std::string type = it.first.as<std::string>(), type_constant = "SKILL_NONEAR_" + type;
				int64 constant;

				if (!script_get_constant(type_constant.c_str(), &constant)) {
					this->invalidWarning(npctypeNode, "NoNearNPC Type %s is invalid.\n", type.c_str());
					return 0;
				}

				bool active;

				if (!this->asBool(npctypeNode, type, active))
					return 0;

				if (active)
					skill->unit_nonearnpc_type |= constant;
				else
					skill->unit_nonearnpc_type &= ~constant;
			}
		} else {
			if (!exists)
				skill->unit_nonearnpc_type = 0;
		}
	}

	if (this->nodeExists(node, "CastCancel")) {
		bool active;

		if (!this->asBool(node, "CastCancel", active))
			return 0;

		skill->castcancel = active;
	} else {
		if (!exists)
			skill->castcancel = false;
	}

	if (this->nodeExists(node, "CastDefenseReduction")) {
		uint16 reduction;

		if (!this->asUInt16(node, "CastDefenseReduction", reduction))
			return 0;

		skill->cast_def_rate = reduction;
	} else {
		if (!exists)
			skill->cast_def_rate = 0;
	}

	if (this->nodeExists(node, "CastTime")) {
		if (!this->parseNode("CastTime", "Time", node, skill->cast))
			return 0;
	} else {
		if (!exists)
			memset(skill->cast, 0, sizeof(skill->cast));
	}

	if (this->nodeExists(node, "AfterCastActDelay")) {
		if (!this->parseNode("AfterCastActDelay", "Time", node, skill->delay))
			return 0;
	} else {
		if (!exists)
			memset(skill->delay, 0, sizeof(skill->delay));
	}

	if (this->nodeExists(node, "AfterCastWalkDelay")) {
		if (!this->parseNode("AfterCastWalkDelay", "Time", node, skill->walkdelay))
			return 0;
	} else {
		if (!exists)
			memset(skill->walkdelay, 0, sizeof(skill->walkdelay));
	}

	if (this->nodeExists(node, "Duration1")) {
		if (!this->parseNode("Duration1", "Time", node, skill->upkeep_time))
			return 0;
	} else {
		if (!exists)
			memset(skill->upkeep_time, 0, sizeof(skill->upkeep_time));
	}

	if (this->nodeExists(node, "Duration2")) {
		if (!this->parseNode("Duration2", "Time", node, skill->upkeep_time2))
			return 0;
	} else {
		if (!exists)
			memset(skill->upkeep_time2, 0, sizeof(skill->upkeep_time2));
	}

	if (this->nodeExists(node, "Cooldown")) {
		if (!this->parseNode("Cooldown", "Time", node, skill->cooldown))
			return 0;
	} else {
		if (!exists)
			memset(skill->cooldown, 0, sizeof(skill->cooldown));
	}

#ifdef RENEWAL_CAST
	if (this->nodeExists(node, "FixedCastTime")) {
		if (!this->parseNode("FixedCastTime", "Time", node, skill->fixed_cast))
			return 0;
	} else {
		if (!exists)
			memset(skill->fixed_cast, 0, sizeof(skill->fixed_cast));
	}
#endif

	if (this->nodeExists(node, "CastTimeFlags")) {
		const YAML::Node &castNode = node["CastTimeFlags"];

		for (const auto &it : castNode) {
			std::string flag = it.first.as<std::string>(), flag_constant = "SKILL_CAST_" + flag;
			int64 constant;

			if (!script_get_constant(flag_constant.c_str(), &constant)) {
				this->invalidWarning(castNode, "CastTimeFlags %s option is invalid.\n", flag.c_str());
				return 0;
			}

			bool active;

			if (!this->asBool(castNode, flag, active))
				return 0;

			if (active)
				skill->castnodex |= constant;
			else
				skill->castnodex &= ~constant;
		}
	}

	if (this->nodeExists(node, "CastDelayFlags")) {
		const YAML::Node &castNode = node["CastDelayFlags"];

		for (const auto &it : castNode) {
			std::string flag = it.first.as<std::string>(), flag_constant = "SKILL_CAST_" + flag;
			int64 constant;

			if (!script_get_constant(flag_constant.c_str(), &constant)) {
				this->invalidWarning(castNode, "CastDelayFlags %s option is invalid.\n", flag.c_str());
				return 0;
			}

			bool active;

			if (!this->asBool(castNode, flag, active))
				return 0;

			if (active)
				skill->delaynodex |= constant;
			else
				skill->delaynodex &= ~constant;
		}
	}

	if (this->nodeExists(node, "Requires")) {
		const YAML::Node &requireNode = node["Requires"];

		if (this->nodeExists(requireNode, "HpCost")) {
			if (!this->parseNode("HpCost", "Amount", requireNode, skill->require.hp))
				return 0;
		} else {
			if (!exists)
				memset(skill->require.hp, 0, sizeof(skill->require.hp));
		}

		if (this->nodeExists(requireNode, "SpCost")) {
			if (!this->parseNode("SpCost", "Amount", requireNode, skill->require.sp))
				return 0;
		} else {
			if (!exists)
				memset(skill->require.sp, 0, sizeof(skill->require.sp));
		}

		if (this->nodeExists(requireNode, "HpRateCost")) {
			if (!this->parseNode("HpRateCost", "Amount", requireNode, skill->require.hp_rate))
				return 0;
		} else {
			if (!exists)
				memset(skill->require.hp_rate, 0, sizeof(skill->require.hp_rate));
		}

		if (this->nodeExists(requireNode, "SpRateCost")) {
			if (!this->parseNode("SpRateCost", "Amount", requireNode, skill->require.sp_rate))
				return 0;
		} else {
			if (!exists)
				memset(skill->require.sp_rate, 0, sizeof(skill->require.sp_rate));
		}

		if (this->nodeExists(requireNode, "MaxHpTrigger")) {
			if (!this->parseNode("MaxHpTrigger", "Amount", requireNode, skill->require.mhp))
				return 0;
		} else {
			if (!exists)
				memset(skill->require.mhp, 0, sizeof(skill->require.mhp));
		}

		if (this->nodeExists(requireNode, "ZenyCost")) {
			if (!this->parseNode("ZenyCost", "Amount", requireNode, skill->require.zeny))
				return 0;
		} else {
			if (!exists)
				memset(skill->require.zeny, 0, sizeof(skill->require.zeny));
		}

		if (this->nodeExists(requireNode, "Weapon")) {
			const YAML::Node &weaponNode = requireNode["Weapon"];

			if (this->nodeExists(weaponNode, "All")) {
				bool active;

				if (!this->asBool(weaponNode, "All", active))
					return 0;

				if (active)
					skill->require.weapon = 0;
			} else {
				for (const auto &it : weaponNode) {
					std::string weapon = it.first.as<std::string>(), weapon_constant = "W_" + weapon;
					int64 constant;

					if (!script_get_constant(weapon_constant.c_str(), &constant)) {
						this->invalidWarning(weaponNode, "Requires Weapon %s is invalid.\n", weapon.c_str());
						return 0;
					}

					bool active;

					if (!this->asBool(weaponNode, weapon, active))
						return 0;

					if (active)
						skill->require.weapon |= 1 << constant;
					else
						skill->require.weapon &= ~(1 << constant);
				}
			}
		} else {
			if (!exists)
				skill->require.weapon = 0;
		}

		if (this->nodeExists(requireNode, "Ammo")) {
			const YAML::Node &ammoNode = requireNode["Ammo"];

			if (this->nodeExists(ammoNode, "None")) {
				bool active;

				if (!this->asBool(ammoNode, "None", active))
					return 0;

				if (active)
					skill->require.ammo = 0;
			} else {
				for (const auto &it : ammoNode) {
					std::string ammo = it.first.as<std::string>(), ammo_constant = "AMMO_" + ammo;
					int64 constant;

					if (!script_get_constant(ammo_constant.c_str(), &constant)) {
						this->invalidWarning(ammoNode, "Requires Ammo %s is invalid.\n", ammo.c_str());
						return 0;
					}

					bool active;

					if (!this->asBool(ammoNode, ammo, active))
						return 0;

					if (active)
						skill->require.ammo |= 1 << constant;
					else
						skill->require.ammo &= ~(1 << constant);
				}
			}
		} else {
			if (!exists)
				skill->require.ammo = 0;
		}

		if (this->nodeExists(requireNode, "AmmoAmount")) {
			if (skill->require.ammo == 0) {
				this->invalidWarning(requireNode["AmmoAmount"], "An ammo type is required before specifying ammo amount.\n");
				return 0;
			}

			if (!this->parseNode("AmmoAmount", "Amount", requireNode, skill->require.ammo_qty))
				return 0;
		} else {
			if (!exists)
				memset(skill->require.ammo_qty, 0, sizeof(skill->require.ammo_qty));
		}

		if (this->nodeExists(requireNode, "State")) {
			std::string state;

			if (!this->asString(requireNode, "State", state))
				return 0;

			std::string state_constant = "ST_" + state;
			int64 constant;

			if (!script_get_constant(state_constant.c_str(), &constant)) {
				this->invalidWarning(requireNode["State"], "Requires State %s is invalid.\n", state.c_str());
				return 0;
			}

			skill->require.state = static_cast<int32>(constant);
		}

		if (this->nodeExists(requireNode, "Status")) {
			const YAML::Node &statusNode = requireNode["Status"];

			for (const auto &it : statusNode) {
				std::string status = it.first.as<std::string>(), status_constant = "SC_" + status;
				int64 constant;

				if (!script_get_constant(status_constant.c_str(), &constant)) {
					this->invalidWarning(statusNode, "Requires Status %s is invalid.\n", status.c_str());
					return 0;
				}

				bool active;

				if (!this->asBool(statusNode, status, active))
					return 0;

				auto status_exists = util::vector_get(skill->require.status, constant);

				if (active && status_exists == skill->require.status.end())
					skill->require.status.push_back(static_cast<sc_type>(constant));
				else if (!active && status_exists != skill->require.status.end())
					skill->require.status.erase(status_exists);
			}
		}

		if (this->nodeExists(requireNode, "SpiritSphereCost")) {
			if (!this->parseNode("SpiritSphereCost", "Amount", requireNode, skill->require.spiritball))
				return 0;
		} else {
			if (!exists)
				memset(skill->require.spiritball, 0, sizeof(skill->require.spiritball));
		}

		if (this->nodeExists(requireNode, "ItemCost")) {
			const YAML::Node &itemNode = requireNode["ItemCost"];
			int32 count = 0;

			for (const YAML::Node &it : itemNode) {
				std::string item_name;

				if (!this->asString(it, "Item", item_name))
					continue;

				struct item_data *item = itemdb_search_aegisname(item_name.c_str());

				if (item == nullptr) {
					this->invalidWarning(itemNode["Item"], "Requires ItemCost Item %s does not exist.\n", item_name.c_str());
					return 0;
				}

				int32 amount;

				if (!this->asInt32(it, "Amount", amount))
					continue;

				skill->require.itemid[count] = item->nameid;
				skill->require.amount[count] = amount;
				count++;
			}
		}

		if (this->nodeExists(requireNode, "Equipment")) {
			const YAML::Node &equipNode = requireNode["Equipment"];

			for (const auto &it : equipNode) {
				std::string item_name = it.first.as<std::string>();
				struct item_data *item = itemdb_search_aegisname(item_name.c_str());

				if (item == nullptr) {
					this->invalidWarning(equipNode, "Requires Equipment %s does not exist.\n", item_name.c_str());
					return 0;
				}

				bool active;

				if (!this->asBool(equipNode, item_name, active))
					return 0;

				auto equip_exists = util::vector_get(skill->require.eqItem, item->nameid);

				if (active && equip_exists == skill->require.eqItem.end())
					skill->require.eqItem.push_back(item->nameid);
				else if (!active && equip_exists != skill->require.eqItem.end())
					skill->require.eqItem.erase(equip_exists);
			}
		}
	}

	if (this->nodeExists(node, "Unit")) {
		const YAML::Node &unitNode = node["Unit"];

		if (this->nodeExists(unitNode, "Id")) {
			std::string unit;

			if (!this->asString(unitNode, "Id", unit))
				return 0;

			std::string unit_constant = "UNT_" + unit;
			int64 constant;

			if (!script_get_constant(unit_constant.c_str(), &constant)) {
				this->invalidWarning(unitNode["Id"], "Unit Id %s is invalid.\n", unit.c_str());
				return 0;
			}

			skill->unit_id = static_cast<uint16>(constant);
		} else {
			this->invalidWarning(unitNode["Id"], "Unit requires an Id.\n");
			return 0;
		}

		if (this->nodeExists(unitNode, "AlternateId")) {
			std::string unit;

			if (!this->asString(unitNode, "AlternateId", unit))
				return 0;

			std::string unit_constant = "UNT_" + unit;
			int64 constant;

			if (!script_get_constant(unit_constant.c_str(), &constant)) {
				this->invalidWarning(unitNode["AlternateId"], "Alternate Unit Id %s is invalid.\n", unit.c_str());
				return 0;
			}


			skill->unit_id2 = static_cast<uint16>(constant);
		} else {
			if (!exists)
				skill->unit_id2 = 0;
		}

		if (this->nodeExists(unitNode, "Layout")) {
			if (!this->parseNode("Layout", "Size", unitNode, skill->unit_layout_type))
				return 0;
		} else {
			if (!exists)
				memset(skill->unit_layout_type, 0, sizeof(skill->unit_layout_type));
		}

		if (this->nodeExists(unitNode, "Range")) {
			if (!this->parseNode("Range", "Size", unitNode, skill->unit_range))
				return 0;
		} else {
			if (!exists)
				memset(skill->unit_range, 0, sizeof(skill->unit_range));
		}

		if (this->nodeExists(unitNode, "Interval")) {
			int16 interval;

			if (!this->asInt16(unitNode, "Interval", interval))
				return 0;

			skill->unit_interval = interval;
		} else {
			if (!exists)
				skill->unit_interval = 0;
		}

		if (this->nodeExists(unitNode, "Target")) {
			std::string target;

			if (!this->asString(unitNode, "Target", target))
				return 0;

			std::string target_constant = "BCT_" + target;
			int64 constant;

			if (!script_get_constant(target_constant.c_str(), &constant)) {
				this->invalidWarning(unitNode["Target"], "Unit Target %s is invalid.\n", target.c_str());
				return 0;
			}

			skill->unit_target = static_cast<int32>(constant);
		} else {
			if (!exists)
				skill->unit_target = BCT_ALL;
		}

		if (this->nodeExists(unitNode, "Flag")) {
			const YAML::Node &flagNode = unitNode["Flag"];

			for (const auto &it : flagNode) {
				std::string flag = it.first.as<std::string>(), flag_constant = "UF_" + flag;
				int64 constant;

				if (!script_get_constant(flag_constant.c_str(), &constant)) {
					this->invalidWarning(flagNode, "Skill Unit Flag %s is invalid.\n", flag.c_str());
					return 0;
				}

				bool active;

				if (!this->asBool(flagNode, flag, active))
					return 0;

				if (active)
					skill->unit_flag.set(static_cast<uint8>(constant));
				else
					skill->unit_flag.reset(static_cast<uint8>(constant));
			}

			if (skill->unit_flag[UF_NOENEMY] && battle_config.defnotenemy)
				skill->unit_target = BCT_NOENEMY;

			// By default, target just characters.
			skill->unit_target |= BL_CHAR;
			if (skill->unit_flag[UF_NOPC])
				skill->unit_target &= ~BL_PC;
			if (skill->unit_flag[UF_NOMOB])
				skill->unit_target &= ~BL_MOB;
			if (skill->unit_flag[UF_SKILL])
				skill->unit_target |= BL_SKILL;
		} else {
			if (!exists){
				skill->unit_flag = UF_NONE;
				// By default, target just characters.
				skill->unit_target |= BL_CHAR;
			}
		}
	}

	if (!exists) {
		this->put(skill_id, skill);
		skilldb_id2idx[skill_id] = skill_num;
		skill_num++;
	}

	return 1;
}

void SkillDatabase::clear() {
	TypesafeCachedYamlDatabase::clear();
	memset(skilldb_id2idx, 0, sizeof(skilldb_id2idx));
	skill_num = 1;
}

SkillDatabase skill_db;

const std::string ReadingSpellbookDatabase::getDefaultLocation() {
	return std::string(db_path) + "/spellbook_db.yml";
}
/**
 * Reads and parses an entry from the spellbook_db.
 * @param node: YAML node containing the entry.
 * @return count of successfully parsed rows
 */
uint64 ReadingSpellbookDatabase::parseBodyNode(const YAML::Node &node) {
	std::string skill_name;

	if (!this->asString(node, "Skill", skill_name))
		return 0;

	uint16 skill_id = skill_name2id(skill_name.c_str());

	if (skill_id == 0) {
		this->invalidWarning(node["Skill"], "Invalid skill name \"%s\", skipping.\n", skill_name.c_str());
		return 0;
	}

	if (!skill_get_inf(skill_id)) {
		this->invalidWarning(node["Skill"], "Passive skill %s cannot be memorized in a Spell Book.\n", skill_name.c_str());
		return 0;
	}

	std::shared_ptr<s_skill_spellbook_db> spell = this->find(skill_id);
	bool exists = spell != nullptr;

	if (!exists) {
		if (!this->nodesExist(node, { "Book", "PreservePoints" }))
			return 0;

		spell = std::make_shared<s_skill_spellbook_db>();
		spell->skill_id = skill_id;
	}

	if (this->nodeExists(node, "Book")) {
		std::string book_name;

		if (!this->asString(node, "Book", book_name))
			return 0;

		struct item_data *item = itemdb_search_aegisname(book_name.c_str());

		if (item == nullptr) {
			this->invalidWarning(node["Book"], "Book item %s does not exist.\n", book_name.c_str());
			return 0;
		}

		spell->nameid = item->nameid;
	}

	if (this->nodeExists(node, "PreservePoints")) {
		uint16 points;

		if (!this->asUInt16(node, "PreservePoints", points))
			return 0;

		spell->points = points;
	}

	if (!exists)
		this->put(skill_id, spell);

	return 1;
}

/**
 * Check if the specified item is available in the spellbook_db or not
 * @param nameid: Book Item ID
 * @return Spell data or nullptr otherwise
 */
std::shared_ptr<s_skill_spellbook_db> ReadingSpellbookDatabase::findBook(t_itemid nameid) {
	if (nameid == 0 || !itemdb_exists(nameid) || reading_spellbook_db.size() == 0)
		return nullptr;

	for (const auto &spell : reading_spellbook_db) {
		if (spell.second->nameid == nameid)
			return spell.second;
	}

	return nullptr;
}




/** Reads skill no cast db
 * Structure: SkillID,Flag
 */
static bool skill_parse_row_nocastdb(char* split[], int columns, int current)
{
	std::shared_ptr<s_skill_db> skill = skill_db.find(atoi(split[0]));

	if (!skill)
		return false;

	skill->nocast |= atoi(split[1]);

	return true;
}

/** Reads Produce db
 * Structure: ProduceItemID,ItemLV,RequireSkill,Requireskill_lv,MaterialID1,MaterialAmount1,...
 */
static bool skill_parse_row_producedb(char* split[], int columns, int current)
{
	unsigned short x, y;
	unsigned short id = atoi(split[0]);
	t_itemid nameid = 0;
	bool found = false;

	if (id >= ARRAYLENGTH(skill_produce_db)) {
		ShowError("skill_parse_row_producedb: Maximum db entries reached.\n");
		return false;
	}

	// Clear previous data, for importing support
	memset(&skill_produce_db[id], 0, sizeof(skill_produce_db[id]));
	// Import just for clearing/disabling from original data
	if (!(nameid = strtoul(split[1], nullptr, 10))) {
		//ShowInfo("skill_parse_row_producedb: Product list with ID %d removed from list.\n", id);
		return true;
	}

	if (!itemdb_exists(nameid)) {
		ShowError("skill_parse_row_producedb: Invalid item %u.\n", nameid);
		return false;
	}

	skill_produce_db[id].nameid = nameid;
	skill_produce_db[id].itemlv = atoi(split[2]);
	skill_produce_db[id].req_skill = atoi(split[3]);
	skill_produce_db[id].req_skill_lv = atoi(split[4]);

	for (x = 5, y = 0; x+1 < columns && split[x] && split[x+1] && y < MAX_PRODUCE_RESOURCE; x += 2, y++) {
		skill_produce_db[id].mat_id[y] = strtoul(split[x], nullptr, 10);
		skill_produce_db[id].mat_amount[y] = atoi(split[x+1]);
	}

	if (!found)
		skill_produce_count++;

	return true;
}

/** Reads create arrow db
 * Sturcture: SourceID,MakeID1,MakeAmount1,...,MakeID5,MakeAmount5
 */
static bool skill_parse_row_createarrowdb(char* split[], int columns, int current)
{
	unsigned short x, y, i;
	t_itemid material_id = strtoul(split[0], nullptr, 10);

	if (!(itemdb_exists(material_id))) {
		ShowError("skill_parse_row_createarrowdb: Invalid item %u.\n", material_id);
		return false;
	}

	//search if we override something, (if not i=last idx)
	ARR_FIND(0, skill_arrow_count, i, skill_arrow_db[i].nameid == material_id);
	if (i >= ARRAYLENGTH(skill_arrow_db)) {
		ShowError("skill_parse_row_createarrowdb: Maximum db entries reached.\n");
		return false;
	}

	// Import just for clearing/disabling from original data
	if (strtoul(split[1], nullptr, 10) == 0) {
		memset(&skill_arrow_db[i], 0, sizeof(skill_arrow_db[i]));
		//ShowInfo("skill_parse_row_createarrowdb: Arrow creation with Material ID %u removed from list.\n", material_id);
		return true;
	}

	skill_arrow_db[i].nameid = material_id;
	for (x = 1, y = 0; x+1 < columns && split[x] && split[x+1] && y < MAX_ARROW_RESULT; x += 2, y++) {
		skill_arrow_db[i].cre_id[y] = strtoul(split[x], nullptr, 10);
		skill_arrow_db[i].cre_amount[y] = atoi(split[x+1]);
	}
	if (i == skill_arrow_count)
		skill_arrow_count++;

	return true;
}

/** Reads change material db
 * Structure: ProductID,BaseRate,MakeAmount1,MakeAmountRate1...,MakeAmount5,MakeAmountRate5
 */
static bool skill_parse_row_changematerialdb(char* split[], int columns, int current)
{
	uint16 id = atoi(split[0]);
	t_itemid nameid = strtoul(split[1], nullptr, 10);
	short rate = atoi(split[2]);
	bool found = false;
	int x, y;

	if (id >= MAX_SKILL_CHANGEMATERIAL_DB) {
		ShowError("skill_parse_row_changematerialdb: Maximum amount of entries reached (%d), increase MAX_SKILL_CHANGEMATERIAL_DB\n",MAX_SKILL_CHANGEMATERIAL_DB);
		return false;
	}

	// Clear previous data, for importing support
	if (id < ARRAYLENGTH(skill_changematerial_db) && skill_changematerial_db[id].nameid > 0) {
		found = true;
		memset(&skill_changematerial_db[id], 0, sizeof(skill_changematerial_db[id]));
	}

	// Import just for clearing/disabling from original data
	// NOTE: If import for disabling, better disable list from produce_db instead of here, or creation just failed with deleting requirements.
	if (nameid == 0) {
		memset(&skill_changematerial_db[id], 0, sizeof(skill_changematerial_db[id]));
		//ShowInfo("skill_parse_row_changematerialdb: Change Material list with ID %d removed from list.\n", id);
		return true;
	}

	// Entry must be exists in skill_produce_db and with required skill GN_CHANGEMATERIAL
	//for (x = 0; x < MAX_SKILL_PRODUCE_DB; x++) {
	//	if (skill_produce_db[x].nameid == nameid)
	//		/*if( skill_produce_db[x].req_skill == GN_CHANGEMATERIAL )
	//			break;*/
	//}

	if (x >= MAX_SKILL_PRODUCE_DB) {
		ShowError("skill_parse_row_changematerialdb: Not supported item ID (%u) for Change Material. \n", nameid);
		return false;
	}

	skill_changematerial_db[id].nameid = nameid;
	skill_changematerial_db[id].rate = rate;

	for (x = 3, y = 0; x+1 < columns && split[x] && split[x+1] && y < MAX_SKILL_CHANGEMATERIAL_SET; x += 2, y++) {
		skill_changematerial_db[id].qty[y] = atoi(split[x]);
		skill_changematerial_db[id].qty_rate[y] = atoi(split[x+1]);
	}

	if (!found)
		skill_changematerial_count++;

	return true;
}

/**
 * Reads skill damage adjustment
 * @author [Lilith]
 */
static bool skill_parse_row_skilldamage(char* split[], int columns, int current)
{
	int64 caster_tmp;
	uint16 id;
	int caster, value;
	char *result;

	trim(split[0]);
	if (ISDIGIT(split[0][0])) {
		value = strtol(split[0], &result, 10);

		if (*result) {
			ShowError("skill_parse_row_skilldamage: Invalid skill %s given, skipping.\n", result);
			return false;
		}

		id = value;
	} else
		id = skill_name2id(split[0]);

	std::shared_ptr<s_skill_db> skill = skill_db.find(id);

	if (!skill)
		return false;

	skill->damage = {};
	trim(split[1]);
	if (ISDIGIT(split[1][0])) {
		value = strtol(split[1], &result, 10);

		if (*result) {
			ShowError("skill_parse_row_skilldamage: Invalid caster %s given for skill %d, skipping.\n", result, id);
			return false;
		}

		caster = value;
	} else { // Try to parse caster as constant
		if (!script_get_constant(split[1], &caster_tmp)) {
			ShowError("skill_parse_row_skilldamage: Invalid caster constant given for skill %d, skipping.\n", id);
			return false;
		}
		caster = static_cast<uint16>(caster_tmp);
	}
	skill->damage.caster |= caster;

	value = strtol(split[2], &result, 10);

	if (*result) {
		ShowError("skill_parse_row_skilldamage: Invalid map %s given for skill %d, skipping.\n", result, id);
		return false;
	}

	skill->damage.map |= value;

	for(int offset = 3, i = SKILLDMG_PC; i < SKILLDMG_MAX && offset < columns; i++, offset++ ){
		value = strtol(split[offset], &result, 10);

		if (*result && *result != ' ') {
			ShowError("skill_parse_row_skilldamage: Invalid damage %s given for skill %d, defaulting to 0.\n", result, id);
			value = 0;
		}
		skill->damage.rate[i] = cap_value(value, -100, 100000);
	}

	return true;
}

/*===============================
 * DB reading.
 * skill_db.yml
 * skill_nocast_db.txt
 * produce_db.txt
 * create_arrow_db.txt
 *------------------------------*/
static void skill_readdb(void)
{
	int i;
	const char* dbsubpath[] = {
		"",
		"/" DBIMPORT,
		//add other path here
	};

	memset(skill_produce_db,0,sizeof(skill_produce_db));
	memset(skill_arrow_db,0,sizeof(skill_arrow_db));
	memset(skill_changematerial_db,0,sizeof(skill_changematerial_db));
	skill_produce_count = skill_arrow_count = skill_changematerial_count = 0;

	skill_db.load();

	for(i=0; i<ARRAYLENGTH(dbsubpath); i++){
		size_t n1 = strlen(db_path)+strlen(dbsubpath[i])+1;
		size_t n2 = strlen(db_path)+strlen(DBPATH)+strlen(dbsubpath[i])+1;
		char* dbsubpath1 = (char*)aMalloc(n1+1);
		char* dbsubpath2 = (char*)aMalloc(n2+1);

		if (i == 0) {
			safesnprintf(dbsubpath1,n1,"%s%s",db_path,dbsubpath[i]);
			safesnprintf(dbsubpath2,n2,"%s/%s%s",db_path,DBPATH,dbsubpath[i]);
		} else {
			safesnprintf(dbsubpath1,n1,"%s%s",db_path,dbsubpath[i]);
			safesnprintf(dbsubpath2,n1,"%s%s",db_path,dbsubpath[i]);
		}

		sv_readdb(dbsubpath2, "produce_db.txt"        , ',',   5,  5+2*MAX_PRODUCE_RESOURCE, MAX_SKILL_PRODUCE_DB, skill_parse_row_producedb, i > 0);
		sv_readdb(dbsubpath1, "create_arrow_db.txt"   , ',', 1+2,  1+2*MAX_ARROW_RESULT, MAX_SKILL_ARROW_DB, skill_parse_row_createarrowdb, i > 0);
	
		aFree(dbsubpath1);
		aFree(dbsubpath2);
	}

	reading_spellbook_db.load();
	skill_init_unit_layout();
	skill_init_nounit_layout();
}

void skill_reload (void) {
	skill_db.clear();
	reading_spellbook_db.clear();

	skill_readdb();
	initChangeTables(); // Re-init Status Change tables

	/* lets update all players skill tree : so that if any skill modes were changed they're properly updated */
	s_mapiterator *iter = mapit_getallusers();

	for( map_session_data *sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); sd = (TBL_PC*)mapit_next(iter) ) {
		pc_validate_skill(sd);
		clif_skillinfoblock(sd);
	}
	mapit_free(iter);
}

/*==========================================
 *
 *------------------------------------------*/
void do_init_skill(void)
{
	skill_readdb();

	skillunit_group_db = idb_alloc(DB_OPT_BASE);
	skillunit_db = idb_alloc(DB_OPT_BASE);
	skillusave_db = idb_alloc(DB_OPT_RELEASE_DATA);
	bowling_db = idb_alloc(DB_OPT_BASE);
	skill_unit_ers = ers_new(sizeof(struct skill_unit_group),"skill.cpp::skill_unit_ers",ERS_CACHE_OPTIONS);
	skill_timer_ers  = ers_new(sizeof(struct skill_timerskill),"skill.cpp::skill_timer_ers",ERS_CACHE_OPTIONS);

	ers_chunk_size(skill_unit_ers, 150);
	ers_chunk_size(skill_timer_ers, 150);

	add_timer_func_list(skill_unit_timer,"skill_unit_timer");
	add_timer_func_list(skill_castend_id,"skill_castend_id");
	add_timer_func_list(skill_castend_pos,"skill_castend_pos");
	add_timer_func_list(skill_timerskill,"skill_timerskill");
	add_timer_func_list(skill_blockpc_end, "skill_blockpc_end");
	add_timer_func_list(skill_keep_using, "skill_keep_using");

	add_timer_interval(gettick()+SKILLUNITTIMER_INTERVAL,skill_unit_timer,0,0,SKILLUNITTIMER_INTERVAL);
}

void do_final_skill(void)
{
	db_destroy(skillunit_group_db);
	db_destroy(skillunit_db);
	db_destroy(skillusave_db);
	db_destroy(bowling_db);
	ers_destroy(skill_unit_ers);
	ers_destroy(skill_timer_ers);
}
