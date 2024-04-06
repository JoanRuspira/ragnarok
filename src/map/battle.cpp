// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "battle.hpp"

#include <math.h>
#include <stdlib.h>

#include "../common/cbasetypes.hpp"
#include "../common/ers.hpp"
#include "../common/malloc.hpp"
#include "../common/nullpo.hpp"
#include "../common/random.hpp"
#include "../common/showmsg.hpp"
#include "../common/socket.hpp"
#include "../common/strlib.hpp"
#include "../common/timer.hpp"
#include "../common/utils.hpp"

#include "atcommand.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "elemental.hpp"
#include "guild.hpp"
#include "log.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "party.hpp"
#include "path.hpp"
#include "pc.hpp"
#include "pc_groups.hpp"
#include "pet.hpp"

int attr_fix_table[MAX_ELE_LEVEL][ELE_MAX][ELE_MAX];

struct Battle_Config battle_config;
static struct eri *delay_damage_ers; //For battle delay damage structures.

/**
 * Returns the current/list skill used by the bl
 * @param bl
 * @return skill_id
 */
uint16 battle_getcurrentskill(struct block_list *bl)
{
	struct unit_data *ud;

	if( bl->type == BL_SKILL ) {
		struct skill_unit *su = (struct skill_unit*)bl;
		return (su && su->group?su->group->skill_id:0);
	}

	ud = unit_bl2ud(bl);

	return (ud?ud->skill_id:0);
}

/**
 * Get random targeting enemy
 * @param bl
 * @param ap
 * @return Found target (1) or not found (0)
 */
static int battle_gettargeted_sub(struct block_list *bl, va_list ap)
{
	struct block_list **bl_list;
	struct unit_data *ud;
	int target_id;
	int *c;

	bl_list = va_arg(ap, struct block_list **);
	c = va_arg(ap, int *);
	target_id = va_arg(ap, int);

	if (bl->id == target_id)
		return 0;

	if (*c >= 24)
		return 0;

	if ( !(ud = unit_bl2ud(bl)) )
		return 0;

	if (ud->target == target_id || ud->skilltarget == target_id) {
		bl_list[(*c)++] = bl;
		return 1;
	}

	return 0;
}

/**
 * Returns list of targets
 * @param target
 * @return Target list
 */
struct block_list* battle_gettargeted(struct block_list *target)
{
	struct block_list *bl_list[24];
	int c = 0;
	nullpo_retr(NULL, target);

	memset(bl_list, 0, sizeof(bl_list));
	map_foreachinallrange(battle_gettargeted_sub, target, AREA_SIZE, BL_CHAR, bl_list, &c, target->id);
	if ( c == 0 )
		return NULL;
	if( c > 24 )
		c = 24;
	return bl_list[rnd()%c];
}

/**
 * Returns the ID of the current targeted character of the passed bl
 * @param bl
 * @return Target Unit ID
 * @author [Skotlex]
 */
int battle_gettarget(struct block_list* bl)
{

	switch (bl->type) {
		case BL_PC:  return ((struct map_session_data*)bl)->ud.target;
		case BL_MOB: return ((struct mob_data*)bl)->target_id;
		case BL_PET: return ((struct pet_data*)bl)->target_id;
		case BL_ELEM: return ((struct elemental_data*)bl)->ud.target;
	}

	return 0;
}

/**
 * Get random enemy
 * @param bl
 * @param ap
 * @return Found target (1) or not found (0)
 */
static int battle_getenemy_sub(struct block_list *bl, va_list ap)
{
	struct block_list **bl_list;
	struct block_list *target;
	int *c;

	bl_list = va_arg(ap, struct block_list **);
	c = va_arg(ap, int *);
	target = va_arg(ap, struct block_list *);

	if (bl->id == target->id)
		return 0;

	if (*c >= 24)
		return 0;

	if (status_isdead(bl))
		return 0;

	if (battle_check_target(target, bl, BCT_ENEMY) > 0) {
		bl_list[(*c)++] = bl;
		return 1;
	}

	return 0;
}

/**
 * Returns list of enemies within given range
 * @param target
 * @param type
 * @param range
 * @return Target list
 * @author [Skotlex]
 */
struct block_list* battle_getenemy(struct block_list *target, int type, int range)
{
	struct block_list *bl_list[24];
	int c = 0;

	memset(bl_list, 0, sizeof(bl_list));
	map_foreachinallrange(battle_getenemy_sub, target, range, type, bl_list, &c, target);

	if ( c == 0 )
		return NULL;

	if( c > 24 )
		c = 24;

	return bl_list[rnd()%c];
}

/**
 * Get random enemy within area
 * @param bl
 * @param ap
 * @return Found target (1) or not found (0)
 */
static int battle_getenemyarea_sub(struct block_list *bl, va_list ap)
{
	struct block_list **bl_list, *src;
	int *c, ignore_id;

	bl_list = va_arg(ap, struct block_list **);
	c = va_arg(ap, int *);
	src = va_arg(ap, struct block_list *);
	ignore_id = va_arg(ap, int);

	if( bl->id == src->id || bl->id == ignore_id )
		return 0; // Ignores Caster and a possible pre-target

	if( *c >= 23 )
		return 0;

	if( status_isdead(bl) )
		return 0;

	if( battle_check_target(src, bl, BCT_ENEMY) > 0 ) {// Is Enemy!...
		bl_list[(*c)++] = bl;
		return 1;
	}

	return 0;
}

/**
 * Returns list of enemies within an area
 * @param src
 * @param x
 * @param y
 * @param range
 * @param type
 * @param ignore_id
 * @return Target list
 */
struct block_list* battle_getenemyarea(struct block_list *src, int x, int y, int range, int type, int ignore_id)
{
	struct block_list *bl_list[24];
	int c = 0;

	memset(bl_list, 0, sizeof(bl_list));
	map_foreachinallarea(battle_getenemyarea_sub, src->m, x - range, y - range, x + range, y + range, type, bl_list, &c, src, ignore_id);

	if( c == 0 )
		return NULL;
	if( c >= 24 )
		c = 23;

	return bl_list[rnd()%c];
}

/*========================================== [Playtester]
* Deals damage without delay, applies additional effects and triggers monster events
* This function is called from battle_delay_damage or battle_delay_damage_sub
* @param src: Source of damage
* @param target: Target of damage
* @param damage: Damage to be dealt
* @param delay: Damage delay
* @param skill_lv: Level of skill used
* @param skill_id: ID o skill used
* @param dmg_lv: State of the attack (miss, etc.)
* @param attack_type: Type of the attack (BF_NORMAL|BF_SKILL|BF_SHORT|BF_LONG|BF_WEAPON|BF_MAGIC|BF_MISC)
* @param additional_effects: Whether additional effect should be applied
* @param isspdamage: If the damage is done to SP
* @param tick: Current tick
*------------------------------------------*/
void battle_damage(struct block_list *src, struct block_list *target, int64 damage, t_tick delay, uint16 skill_lv, uint16 skill_id, enum damage_lv dmg_lv, unsigned short attack_type, bool additional_effects, t_tick tick, bool isspdamage) {
	map_freeblock_lock();
	if (isspdamage)
		status_fix_spdamage(src, target, damage, delay, skill_id);
	else
		status_fix_damage(src, target, damage, delay, skill_id); // We have to separate here between reflect damage and others [icescope]
	if (attack_type && !status_isdead(target) && additional_effects)
		SkillAdditionalEffects::skill_additional_effect(src, target, skill_id, skill_lv, attack_type, dmg_lv, tick);
	if (dmg_lv > ATK_BLOCK && attack_type)
		skill_counter_additional_effect(src, target, skill_id, skill_lv, attack_type, tick);
	// This is the last place where we have access to the actual damage type, so any monster events depending on type must be placed here
	if (target->type == BL_MOB) {
		struct mob_data* md = BL_CAST(BL_MOB, target);

		if (!status_isdead(target) && src != target) {
			if (damage > 0 )
				mobskill_event(md, src, tick, attack_type);
			if (skill_id)
				mobskill_event(md, src, tick, MSC_SKILLUSED|(skill_id<<16));
		}

		if (damage && (attack_type&BF_NORMAL)) // Monsters differentiate whether they have been attacked by a skill or a normal attack
			md->norm_attacked_id = md->attacked_id;
	}
	map_freeblock_unlock();
}

/// Damage Delayed Structure
struct delay_damage {
	int src_id;
	int target_id;
	int64 damage;
	t_tick delay;
	unsigned short distance;
	uint16 skill_lv;
	uint16 skill_id;
	enum damage_lv dmg_lv;
	unsigned short attack_type;
	bool additional_effects;
	enum bl_type src_type;
	bool isspdamage;
};

TIMER_FUNC(battle_delay_damage_sub){
	struct delay_damage *dat = (struct delay_damage *)data;

	if ( dat ) {
		struct block_list* src = map_id2bl(dat->src_id);
		struct block_list* target = map_id2bl(dat->target_id);

		if (target && !status_isdead(target)) {
			if( src && target->m == src->m &&
				(target->type != BL_PC || ((TBL_PC*)target)->invincible_timer == INVALID_TIMER) &&
				check_distance_bl(src, target, dat->distance) ) //Check to see if you haven't teleported. [Skotlex]
			{
				//Deal damage
				battle_damage(src, target, dat->damage, dat->delay, dat->skill_lv, dat->skill_id, dat->dmg_lv, dat->attack_type, dat->additional_effects, tick, dat->isspdamage);
			} else if( !src && dat->skill_id == SK_CR_REFLECTSHIELD ) { // it was monster reflected damage, and the monster died, we pass the damage to the character as expected
				map_freeblock_lock();
				status_fix_damage(target, target, dat->damage, dat->delay, dat->skill_id);
				map_freeblock_unlock();
			}
		}

		struct map_session_data *sd = BL_CAST(BL_PC, src);

		if (sd && --sd->delayed_damage == 0 && sd->state.hold_recalc) {
			sd->state.hold_recalc = false;
			status_calc_pc(sd, SCO_FORCE);
		}
	}
	ers_free(delay_damage_ers, dat);
	return 0;
}

int battle_delay_damage(t_tick tick, int amotion, struct block_list *src, struct block_list *target, int attack_type, uint16 skill_id, uint16 skill_lv, int64 damage, enum damage_lv dmg_lv, t_tick ddelay, bool additional_effects, bool isspdamage)
{
	struct delay_damage *dat;
	struct status_change *sc;
	struct block_list *d_tbl = NULL;
	struct block_list *e_tbl = NULL;

	nullpo_ret(src);
	nullpo_ret(target);

	sc = status_get_sc(target);

	if (sc) {
		if (sc->data[STATUS_SWORNPROTECTOR] && sc->data[STATUS_SWORNPROTECTOR]->val1)
			d_tbl = map_id2bl(sc->data[STATUS_SWORNPROTECTOR]->val1);
	}

	if( ((d_tbl && check_distance_bl(target, d_tbl, sc->data[STATUS_SWORNPROTECTOR]->val3)) || e_tbl) &&
		damage > 0 && skill_id != SK_CR_REFLECTSHIELD
		) {
		struct map_session_data* tsd = BL_CAST( BL_PC, target );

		if( tsd && pc_issit( tsd ) && battle_config.devotion_standup_fix ){
			pc_setstand( tsd, true );
			skill_sit( tsd, 0 );
		}

		damage = 0;
	}

	if ( !battle_config.delay_battle_damage || amotion <= 1 ) {
		//Deal damage
		battle_damage(src, target, damage, ddelay, skill_lv, skill_id, dmg_lv, attack_type, additional_effects, gettick(), isspdamage);
		return 0;
	}
	dat = ers_alloc(delay_damage_ers, struct delay_damage);
	dat->src_id = src->id;
	dat->target_id = target->id;
	dat->skill_id = skill_id;
	dat->skill_lv = skill_lv;
	dat->attack_type = attack_type;
	dat->damage = damage;
	dat->dmg_lv = dmg_lv;
	dat->delay = ddelay;
	dat->distance = distance_bl(src, target) + (battle_config.snap_dodge ? 10 : AREA_SIZE);
	dat->additional_effects = additional_effects;
	dat->src_type = src->type;
	dat->isspdamage = isspdamage;
	if (src->type != BL_PC && amotion > 1000)
		amotion = 1000; //Aegis places a damage-delay cap of 1 sec to non player attacks. [Skotlex]

	if( src->type == BL_PC )
		((TBL_PC*)src)->delayed_damage++;

	add_timer(tick+amotion, battle_delay_damage_sub, 0, (intptr_t)dat);

	return 0;
}

/**
 * Get attribute ratio
 * @param atk_elem Attack element enum e_element
 * @param def_type Defense element enum e_element
 * @param def_lv Element level 1 ~ MAX_ELE_LEVEL
 */
int battle_attr_ratio(int atk_elem, int def_type, int def_lv)
{
	if (!CHK_ELEMENT(atk_elem) || !CHK_ELEMENT(def_type) || !CHK_ELEMENT_LEVEL(def_lv))
		return 100;

	return attr_fix_table[def_lv-1][atk_elem][def_type];
}

/**
 * Does attribute fix modifiers.
 * Added passing of the chars so that the status changes can affect it. [Skotlex]
 * Note: Passing src/target == NULL is perfectly valid, it skips SC_ checks.
 * @param src
 * @param target
 * @param damage
 * @param atk_elem
 * @param def_type
 * @param def_lv
 * @param flag
 * @return damage
 */
int64 battle_attr_fix(struct block_list *src, struct block_list *target, int64 damage,int atk_elem,int def_type, int def_lv)
{
	struct status_change *sc = NULL, *tsc = NULL;
	int ratio;

	if (src) sc = status_get_sc(src);
	if (target) tsc = status_get_sc(target);

	if (!CHK_ELEMENT(atk_elem))
		atk_elem = rnd()%ELE_ALL;

	if (!CHK_ELEMENT(def_type) || !CHK_ELEMENT_LEVEL(def_lv)) {
		ShowError("battle_attr_fix: unknown attribute type: atk=%d def_type=%d def_lv=%d\n",atk_elem,def_type,def_lv);
		return damage;
	}

	ratio = attr_fix_table[def_lv-1][atk_elem][def_type];
	if (sc && sc->count) { //increase dmg by src status
		switch(atk_elem){
			case ELE_FIRE:
				if (sc->data[STATUS_SPHERE_1] && sc->data[STATUS_SPHERE_1]->val1 == WLS_FIRE)
					ratio += 5;
				if (sc->data[STATUS_SPHERE_2] && sc->data[STATUS_SPHERE_2]->val1 == WLS_FIRE)
					ratio += 5;
				if (sc->data[STATUS_SPHERE_3] && sc->data[STATUS_SPHERE_3]->val1 == WLS_FIRE)
					ratio += 5;
				if (sc->data[STATUS_SPHERE_4] && sc->data[STATUS_SPHERE_4]->val1 == WLS_FIRE)
					ratio += 5;
				if (sc->data[STATUS_PYROTECHNIA])
					ratio += sc->data[STATUS_PYROTECHNIA]->val1*10;
				if (sc->data[STATUS_FIREINSIGNIA])
					ratio += sc->data[STATUS_FIREINSIGNIA]->val1*4;
				break;
			case ELE_WIND:
				if (sc->data[STATUS_SPHERE_1] && sc->data[STATUS_SPHERE_1]->val1 == WLS_WIND)
					ratio += 5;
				if (sc->data[STATUS_SPHERE_2] && sc->data[STATUS_SPHERE_2]->val1 == WLS_WIND)
					ratio += 5;
				if (sc->data[STATUS_SPHERE_3] && sc->data[STATUS_SPHERE_3]->val1 == WLS_WIND)
					ratio += 5;
				if (sc->data[STATUS_SPHERE_4] && sc->data[STATUS_SPHERE_4]->val1 == WLS_WIND)
					ratio += 5;
				if (sc->data[STATUS_WINDINSIGNIA])
					ratio += sc->data[STATUS_WINDINSIGNIA]->val1*4;
				break;
			case ELE_WATER:
				if (sc->data[STATUS_SPHERE_1] && sc->data[STATUS_SPHERE_1]->val1 == WLS_WATER)
					ratio += 5;
				if (sc->data[STATUS_SPHERE_2] && sc->data[STATUS_SPHERE_2]->val1 == WLS_WATER)
					ratio += 5;
				if (sc->data[STATUS_SPHERE_3] && sc->data[STATUS_SPHERE_3]->val1 == WLS_WATER)
					ratio += 5;
				if (sc->data[STATUS_SPHERE_4] && sc->data[STATUS_SPHERE_4]->val1 == WLS_WATER)
					ratio += 5;
				if (sc->data[STATUS_WATERINSIGNIA])
					ratio += sc->data[STATUS_WATERINSIGNIA]->val1*4;
				break;
			case ELE_EARTH:
				if (sc->data[STATUS_SPHERE_1] && sc->data[STATUS_SPHERE_1]->val1 == WLS_STONE)
					ratio += 5;
				if (sc->data[STATUS_SPHERE_2] && sc->data[STATUS_SPHERE_2]->val1 == WLS_STONE)
					ratio += 5;
				if (sc->data[STATUS_SPHERE_3] && sc->data[STATUS_SPHERE_3]->val1 == WLS_STONE)
					ratio += 5;
				if (sc->data[STATUS_SPHERE_4] && sc->data[STATUS_SPHERE_4]->val1 == WLS_STONE)
					ratio += 5;
				if (sc->data[STATUS_GEOGRAFIELD])
					ratio += sc->data[STATUS_GEOGRAFIELD]->val1 * 10;
				if (sc->data[STATUS_PETROLOGY])
					ratio += sc->data[STATUS_PETROLOGY]->val1*10;
				if (sc->data[STATUS_EARTHINSIGNIA])
					ratio += sc->data[STATUS_EARTHINSIGNIA]->val1*4;
				break;
			case ELE_GHOST:
				if (sc->data[STATUS_VOIDMAGEPRODIGY])
					ratio += sc->data[STATUS_VOIDMAGEPRODIGY]->val3;
				if (sc->data[STATUS_LAUDATEDOMINIUM])
					ratio += sc->data[STATUS_LAUDATEDOMINIUM]->val3;
				break;
			case ELE_DARK:
				if (sc->data[STATUS_VOIDMAGEPRODIGY])
					ratio += sc->data[STATUS_VOIDMAGEPRODIGY]->val3;
				break;
			case ELE_HOLY:
				if (sc->data[STATUS_LAUDATEDOMINIUM])
					ratio += sc->data[STATUS_LAUDATEDOMINIUM]->val3;
				break;
			case ELE_NEUTRAL:
				if (sc->data[STATUS_HARMONIZE])
					ratio += sc->data[STATUS_HARMONIZE]->val1*10;
				break;
		}
	}

	if (tsc && tsc->data[STATUS_ASTRALSTRIKE_DEBUFF])
		ratio += 50;
	
	if( target && target->type == BL_SKILL ) {
		if( atk_elem == ELE_FIRE && battle_getcurrentskill(target) == SK_AM_WILDTHORNS ) {
			struct skill_unit *su = (struct skill_unit*)target;
			struct skill_unit_group *sg;
			struct block_list *src2;

			if( !su || !su->alive || (sg = su->group) == NULL || !sg || sg->val3 == -1 ||
			   (src2 = map_id2bl(sg->src_id)) == NULL || status_isdead(src2) )
				return 0;

			if( sg->unit_id != UNT_FIREWALL ) {
				int x,y;
				x = sg->val3 >> 16;
				y = sg->val3 & 0xffff;
				skill_unitsetting(src2,su->group->skill_id,su->group->skill_lv,x,y,1);
				sg->val3 = -1;
				sg->limit = DIFF_TICK(gettick(),sg->tick)+300;
			}
		}
	}

	if (tsc && tsc->count) { //increase dmg by target status
		switch(atk_elem) {
			case ELE_FIRE:
				break;
				
			case ELE_HOLY:
				
				break;
			case ELE_POISON:
				if (tsc->data[STATUS_VENOMIMPRESS])
					ratio += tsc->data[STATUS_VENOMIMPRESS]->val2;

				break;
			case ELE_WIND:
				break;
			case ELE_WATER:
				break;
			case ELE_EARTH:
				break;
			case ELE_NEUTRAL:
				break;
			case ELE_DARK:
				break;
		}
	}

	if (battle_config.attr_recover == 0 && ratio < 0)
		ratio = 0;

#ifdef RENEWAL
	//In renewal, reductions are always rounded down so damage can never reach 0 unless ratio is 0
	damage = damage - (int64)((damage * (100 - ratio)) / 100);
#else
	damage = (int64)((damage*ratio)/100);
#endif

	//Damage can be negative, see battle_config.attr_recover
	return damage;
}

/**
 * Calculates card bonuses damage adjustments.
 * @param attack_type @see enum e_battle_flag
 * @param src Attacker
 * @param target Target
 * @param nk Skill's nk @see enum e_skill_nk [NK_IGNOREATKCARD|NK_IGNOREELEMENT|NK_IGNOREDEFCARD]
 * @param rh_ele Right-hand weapon element
 * @param lh_ele Left-hand weapon element (BF_MAGIC and BF_MISC ignore this value)
 * @param damage Original damage
 * @param left Left hand flag (BF_MISC and BF_MAGIC ignore flag value)
 *         3: Calculates attacker bonuses in both hands.
 *         2: Calculates attacker bonuses in right-hand only.
 *         0 or 1: Only calculates target bonuses.
 * @param flag Misc value of skill & damage flags
 * @return damage Damage diff between original damage and after calculation
 */
int battle_calc_cardfix(int attack_type, struct block_list *src, struct block_list *target, std::bitset<NK_MAX> nk, int rh_ele, int lh_ele, int64 damage, int left, int flag){
	struct map_session_data *sd, ///< Attacker session data if BL_PC
		*tsd; ///< Target session data if BL_PC
	short cardfix = 1000;
	int s_class, ///< Attacker class
		t_class; ///< Target class
	std::vector<e_race2> s_race2, /// Attacker Race2
		t_race2; ///< Target Race2
	enum e_element s_defele; ///< Attacker Element (not a weapon or skill element!)
	struct status_data *sstatus, ///< Attacker status data
		*tstatus; ///< Target status data
	int64 original_damage;

	if( !damage )
		return 0;

	original_damage = damage;

	sd = BL_CAST(BL_PC, src);
	tsd = BL_CAST(BL_PC, target);
	t_class = status_get_class(target);
	s_class = status_get_class(src);
	sstatus = status_get_status_data(src);
	tstatus = status_get_status_data(target);
	s_race2 = status_get_race2(src);
	t_race2 = status_get_race2(target);
	s_defele = (tsd) ? (enum e_element)status_get_element(src) : ELE_NONE;

//Official servers apply the cardfix value on a base of 1000 and round down the reduction/increase
#define APPLY_CARDFIX(damage, fix) { (damage) = (damage) - (int64)(((damage) * (1000 - (fix))) / 1000); }

	switch( attack_type ) {
		case BF_MAGIC:
			// Affected by attacker ATK bonuses
			if( sd && !nk[NK_IGNOREATKCARD] ) {
				int32 race2_val = 0;

				for (const auto &raceit : t_race2)
					race2_val += sd->indexed_bonus.magic_addrace2[raceit];
				cardfix = cardfix * (100 + sd->indexed_bonus.magic_addrace[tstatus->race] + sd->indexed_bonus.magic_addrace[RC_ALL] + race2_val) / 100;
				if( !nk[NK_IGNOREELEMENT] ) { // Affected by Element modifier bonuses
					cardfix = cardfix * (100 + sd->indexed_bonus.magic_addele[tstatus->def_ele] + sd->indexed_bonus.magic_addele[ELE_ALL] +
						sd->indexed_bonus.magic_addele_script[tstatus->def_ele] + sd->indexed_bonus.magic_addele_script[ELE_ALL]) / 100;
					cardfix = cardfix * (100 + sd->indexed_bonus.magic_atk_ele[rh_ele] + sd->indexed_bonus.magic_atk_ele[ELE_ALL]) / 100;
				}
				cardfix = cardfix * (100 + sd->indexed_bonus.magic_addsize[tstatus->size] + sd->indexed_bonus.magic_addsize[SZ_ALL]) / 100;
				cardfix = cardfix * (100 + sd->indexed_bonus.magic_addclass[tstatus->class_] + sd->indexed_bonus.magic_addclass[CLASS_ALL]) / 100;
				for (const auto &it : sd->add_mdmg) {
					if (it.id == t_class) {
						cardfix = cardfix * (100 + it.val) / 100;
						break;
					}
				}
				APPLY_CARDFIX(damage, cardfix);
			}

			// Affected by target DEF bonuses
			if( tsd && !nk[NK_IGNOREDEFCARD] ) {
				cardfix = 1000; // reset var for target

				if( !nk[NK_IGNOREELEMENT] ) { // Affected by Element modifier bonuses
					int ele_fix = tsd->indexed_bonus.subele[rh_ele] + tsd->indexed_bonus.subele[ELE_ALL] + tsd->indexed_bonus.subele_script[rh_ele] + tsd->indexed_bonus.subele_script[ELE_ALL];

					for (const auto &it : tsd->subele2) {
						if (it.ele != rh_ele)
							continue;
						if (!(((it.flag)&flag)&BF_WEAPONMASK &&
							((it.flag)&flag)&BF_RANGEMASK &&
							((it.flag)&flag)&BF_SKILLMASK))
							continue;
						ele_fix += it.rate;
					}
					if (s_defele != ELE_NONE)
						ele_fix += tsd->indexed_bonus.magic_subdefele[s_defele] + tsd->indexed_bonus.magic_subdefele[ELE_ALL];
					cardfix = cardfix * (100 - ele_fix) / 100;
				}
				cardfix = cardfix * (100 - tsd->indexed_bonus.subsize[sstatus->size] - tsd->indexed_bonus.subsize[SZ_ALL]) / 100;
				cardfix = cardfix * (100 - tsd->indexed_bonus.magic_subsize[sstatus->size] - tsd->indexed_bonus.magic_subsize[SZ_ALL]) / 100;

				int32 race_fix = 0;

				for (const auto &raceit : s_race2)
					race_fix += tsd->indexed_bonus.subrace2[raceit];
				cardfix = cardfix * (100 - race_fix) / 100;
				race_fix = tsd->indexed_bonus.subrace[sstatus->race] + tsd->indexed_bonus.subrace[RC_ALL];
				for (const auto &it : tsd->subrace3) {
					if (it.race != sstatus->race)
						continue;
					if (!(((it.flag)&flag)&BF_WEAPONMASK &&
						((it.flag)&flag)&BF_RANGEMASK &&
						((it.flag)&flag)&BF_SKILLMASK))
						continue;
					race_fix += it.rate;
				}
				cardfix = cardfix * (100 - race_fix) / 100;
				cardfix = cardfix * (100 - tsd->indexed_bonus.subclass[sstatus->class_] - tsd->indexed_bonus.subclass[CLASS_ALL]) / 100;

				for (const auto &it : tsd->add_mdef) {
					if (it.id == s_class) {
						cardfix = cardfix * (100 - it.val) / 100;
						break;
					}
				}
#ifndef RENEWAL
				//It was discovered that ranged defense also counts vs magic! [Skotlex]
				if( flag&BF_SHORT )
					cardfix = cardfix * (100 - tsd->bonus.near_attack_def_rate) / 100;
				else if (!nk[NK_IGNORELONGCARD])
					cardfix = cardfix * (100 - tsd->bonus.long_attack_def_rate) / 100;
#endif
				cardfix = cardfix * (100 - tsd->bonus.magic_def_rate) / 100;

				if( tsd->sc.data[STATUS_MDEF_RATE] )
					cardfix = cardfix * (100 - tsd->sc.data[STATUS_MDEF_RATE]->val1) / 100;
				APPLY_CARDFIX(damage, cardfix);
			}
			break;

		case BF_WEAPON:
			// Affected by attacker ATK bonuses
			if( sd && !nk[NK_IGNOREATKCARD] && (left&2) ) {
				short cardfix_ = 1000;

				if( sd->state.arrow_atk ) { // Ranged attack
					cardfix = cardfix * (100 + sd->right_weapon.addrace[tstatus->race] + sd->indexed_bonus.arrow_addrace[tstatus->race] +
						sd->right_weapon.addrace[RC_ALL] + sd->indexed_bonus.arrow_addrace[RC_ALL]) / 100;
					if( !nk[NK_IGNOREELEMENT] ) { // Affected by Element modifier bonuses
						int ele_fix = sd->right_weapon.addele[tstatus->def_ele] + sd->indexed_bonus.arrow_addele[tstatus->def_ele] +
							sd->right_weapon.addele[ELE_ALL] + sd->indexed_bonus.arrow_addele[ELE_ALL];

						for (const auto &it : sd->right_weapon.addele2) {
							if (it.ele != tstatus->def_ele)
								continue;
							if (!(((it.flag)&flag)&BF_WEAPONMASK &&
								((it.flag)&flag)&BF_RANGEMASK &&
								((it.flag)&flag)&BF_SKILLMASK))
								continue;
							ele_fix += it.rate;
						}
						cardfix = cardfix * (100 + ele_fix) / 100;
					}
					cardfix = cardfix * (100 + sd->right_weapon.addsize[tstatus->size] + sd->indexed_bonus.arrow_addsize[tstatus->size] +
						sd->right_weapon.addsize[SZ_ALL] + sd->indexed_bonus.arrow_addsize[SZ_ALL]) / 100;

					int32 race_fix = 0;

					for (const auto &raceit : t_race2)
						race_fix += sd->right_weapon.addrace2[raceit];
					cardfix = cardfix * (100 + race_fix) / 100;
					cardfix = cardfix * (100 + sd->right_weapon.addclass[tstatus->class_] + sd->indexed_bonus.arrow_addclass[tstatus->class_] +
						sd->right_weapon.addclass[CLASS_ALL] + sd->indexed_bonus.arrow_addclass[CLASS_ALL]) / 100;
				} else { // Melee attack
					int skill = 0;

					// Calculates each right & left hand weapon bonuses separatedly
					if( !battle_config.left_cardfix_to_right ) {
						// Right-handed weapon
						cardfix = cardfix * (100 + sd->right_weapon.addrace[tstatus->race] + sd->right_weapon.addrace[RC_ALL]) / 100;
						if( !nk[NK_IGNOREELEMENT] ) { // Affected by Element modifier bonuses
							int ele_fix = sd->right_weapon.addele[tstatus->def_ele] + sd->right_weapon.addele[ELE_ALL];

							for (const auto &it : sd->right_weapon.addele2) {
								if (it.ele != tstatus->def_ele)
									continue;
								if (!(((it.flag)&flag)&BF_WEAPONMASK &&
									((it.flag)&flag)&BF_RANGEMASK &&
									((it.flag)&flag)&BF_SKILLMASK))
									continue;
								ele_fix += it.rate;
							}
							cardfix = cardfix * (100 + ele_fix) / 100;
						}
						cardfix = cardfix * (100 + sd->right_weapon.addsize[tstatus->size] + sd->right_weapon.addsize[SZ_ALL]) / 100;
						for (const auto &raceit : t_race2)
							cardfix = cardfix * (100 + sd->right_weapon.addrace2[raceit]) / 100;
						cardfix = cardfix * (100 + sd->right_weapon.addclass[tstatus->class_] + sd->right_weapon.addclass[CLASS_ALL]) / 100;

						if( left&1 ) { // Left-handed weapon
							cardfix_ = cardfix_ * (100 + sd->left_weapon.addrace[tstatus->race] + sd->left_weapon.addrace[RC_ALL]) / 100;
							if( !nk[NK_IGNOREELEMENT] ) { // Affected by Element modifier bonuses
								int ele_fix_lh = sd->left_weapon.addele[tstatus->def_ele] + sd->left_weapon.addele[ELE_ALL];

								for (const auto &it : sd->left_weapon.addele2) {
									if (it.ele != tstatus->def_ele)
										continue;
									if (!(((it.flag)&flag)&BF_WEAPONMASK &&
										((it.flag)&flag)&BF_RANGEMASK &&
										((it.flag)&flag)&BF_SKILLMASK))
										continue;
									ele_fix_lh += it.rate;
								}
								cardfix_ = cardfix_ * (100 + ele_fix_lh) / 100;
							}
							cardfix_ = cardfix_ * (100 + sd->left_weapon.addsize[tstatus->size] + sd->left_weapon.addsize[SZ_ALL]) / 100;
							for (const auto &raceit : t_race2)
								cardfix_ = cardfix_ * (100 + sd->left_weapon.addrace2[raceit]) / 100;
							cardfix_ = cardfix_ * (100 + sd->left_weapon.addclass[tstatus->class_] + sd->left_weapon.addclass[CLASS_ALL]) / 100;
						}
					}
					// Calculates right & left hand weapon as unity
					else {
						//! CHECKME: If 'left_cardfix_to_right' is yes, doesn't need to check NK_IGNOREELEMENT?
						//if( !nk[&]K_IGNOREELEMENT) ) { // Affected by Element modifier bonuses
							int ele_fix = sd->right_weapon.addele[tstatus->def_ele] + sd->left_weapon.addele[tstatus->def_ele]
										+ sd->right_weapon.addele[ELE_ALL] + sd->left_weapon.addele[ELE_ALL];

							for (const auto &it : sd->right_weapon.addele2) {
								if (it.ele != tstatus->def_ele)
									continue;
								if (!(((it.flag)&flag)&BF_WEAPONMASK &&
									((it.flag)&flag)&BF_RANGEMASK &&
									((it.flag)&flag)&BF_SKILLMASK))
									continue;
								ele_fix += it.rate;
							}
							for (const auto &it : sd->left_weapon.addele2) {
								if (it.ele != tstatus->def_ele)
									continue;
								if (!(((it.flag)&flag)&BF_WEAPONMASK &&
									((it.flag)&flag)&BF_RANGEMASK &&
									((it.flag)&flag)&BF_SKILLMASK))
									continue;
								ele_fix += it.rate;
							}
							cardfix = cardfix * (100 + ele_fix) / 100;
						//}
						cardfix = cardfix * (100 + sd->right_weapon.addrace[tstatus->race] + sd->left_weapon.addrace[tstatus->race] +
							sd->right_weapon.addrace[RC_ALL] + sd->left_weapon.addrace[RC_ALL]) / 100;
						cardfix = cardfix * (100 + sd->right_weapon.addsize[tstatus->size] + sd->left_weapon.addsize[tstatus->size] +
							sd->right_weapon.addsize[SZ_ALL] + sd->left_weapon.addsize[SZ_ALL]) / 100;
						for (const auto &raceit : t_race2)
							cardfix = cardfix * (100 + sd->right_weapon.addrace2[raceit] + sd->left_weapon.addrace2[raceit]) / 100;
						cardfix = cardfix * (100 + sd->right_weapon.addclass[tstatus->class_] + sd->left_weapon.addclass[tstatus->class_] +
							sd->right_weapon.addclass[CLASS_ALL] + sd->left_weapon.addclass[CLASS_ALL]) / 100;
					}
					if( sd->status.weapon == W_KATAR ) // Adv. Katar Mastery functions similar to a +%ATK card on official [helvetica]
						cardfix = cardfix * (100 + (10 + 2 * skill)) / 100;
				}

				//! CHECKME: These right & left hand weapon ignores 'left_cardfix_to_right'?
				for (const auto &it : sd->right_weapon.add_dmg) {
					if (it.id == t_class) {
						cardfix = cardfix * (100 + it.val) / 100;
						break;
					}
				}
				if( left&1 ) {
					for (const auto &it : sd->left_weapon.add_dmg) {
						if (it.id == t_class) {
							cardfix_ = cardfix_ * (100 + it.val) / 100;
							break;
						}
					}
				}
#ifndef RENEWAL
				if (flag & BF_SHORT)
					cardfix = cardfix * (100 + sd->bonus.short_attack_atk_rate) / 100;
				if( flag&BF_LONG )
					cardfix = cardfix * (100 + sd->bonus.long_attack_atk_rate) / 100;
#endif
				if (left&1) {
					APPLY_CARDFIX(damage, cardfix_);
				} else {
					APPLY_CARDFIX(damage, cardfix);
				}
			}
			// Affected by target DEF bonuses
			else if( tsd && !nk[NK_IGNOREDEFCARD] && !(left&2) ) {
				if( !nk[NK_IGNOREELEMENT] ) { // Affected by Element modifier bonuses
					int ele_fix = tsd->indexed_bonus.subele[rh_ele] + tsd->indexed_bonus.subele[ELE_ALL] + tsd->indexed_bonus.subele_script[rh_ele] + tsd->indexed_bonus.subele_script[ELE_ALL];

					for (const auto &it : tsd->subele2) {
						if (it.ele != rh_ele)
							continue;
						if (!(((it.flag)&flag)&BF_WEAPONMASK &&
							((it.flag)&flag)&BF_RANGEMASK &&
							((it.flag)&flag)&BF_SKILLMASK))
							continue;
						ele_fix += it.rate;
					}
					cardfix = cardfix * (100 - ele_fix) / 100;

					if( left&1 && lh_ele != rh_ele ) {
						int ele_fix_lh = tsd->indexed_bonus.subele[lh_ele] + tsd->indexed_bonus.subele[ELE_ALL] + tsd->indexed_bonus.subele_script[lh_ele] + tsd->indexed_bonus.subele_script[ELE_ALL];

						for (const auto &it : tsd->subele2) {
							if (it.ele != lh_ele)
								continue;
							if (!(((it.flag)&flag)&BF_WEAPONMASK &&
								((it.flag)&flag)&BF_RANGEMASK &&
								((it.flag)&flag)&BF_SKILLMASK))
								continue;
							ele_fix_lh += it.rate;
						}
						cardfix = cardfix * (100 - ele_fix_lh) / 100;
					}

					cardfix = cardfix * (100 - tsd->indexed_bonus.subdefele[s_defele] - tsd->indexed_bonus.subdefele[ELE_ALL]) / 100;
				}

				int32 race_fix = 0;

				cardfix = cardfix * (100 - tsd->indexed_bonus.subsize[sstatus->size] - tsd->indexed_bonus.subsize[SZ_ALL]) / 100;
				for (const auto &raceit : s_race2)
					race_fix += tsd->indexed_bonus.subrace2[raceit];
				cardfix = cardfix * (100 - race_fix) / 100;
				race_fix = tsd->indexed_bonus.subrace[sstatus->race] + tsd->indexed_bonus.subrace[RC_ALL];
				for (const auto &it : tsd->subrace3) {
					if (it.race != sstatus->race)
						continue;
					if (!(((it.flag)&flag)&BF_WEAPONMASK &&
						((it.flag)&flag)&BF_RANGEMASK &&
						((it.flag)&flag)&BF_SKILLMASK))
						continue;
					race_fix += it.rate;
				}
				cardfix = cardfix * (100 - race_fix) / 100;
				cardfix = cardfix * (100 - tsd->indexed_bonus.subclass[sstatus->class_] - tsd->indexed_bonus.subclass[CLASS_ALL]) / 100;
				for (const auto &it : tsd->add_def) {
					if (it.id == s_class) {
						cardfix = cardfix * (100 - it.val) / 100;
						break;
					}
				}
				if( flag&BF_SHORT )
					cardfix = cardfix * (100 - tsd->bonus.near_attack_def_rate) / 100;
				else if (!nk[NK_IGNORELONGCARD])	// BF_LONG (there's no other choice)
					cardfix = cardfix * (100 - tsd->bonus.long_attack_def_rate) / 100;
				if( tsd->sc.data[STATUS_DEF_RATE] )
					cardfix = cardfix * (100 - tsd->sc.data[STATUS_DEF_RATE]->val1) / 100;
				APPLY_CARDFIX(damage, cardfix);
			}
			break;

		case BF_MISC:
			// Affected by target DEF bonuses
			if( tsd && !nk[NK_IGNOREDEFCARD] ) {
				if( !nk[NK_IGNOREELEMENT] ) { // Affected by Element modifier bonuses
					int ele_fix = tsd->indexed_bonus.subele[rh_ele] + tsd->indexed_bonus.subele[ELE_ALL] + tsd->indexed_bonus.subele_script[rh_ele] + tsd->indexed_bonus.subele_script[ELE_ALL];

					for (const auto &it : tsd->subele2) {
						if (it.ele != rh_ele)
							continue;
						if (!(((it.flag)&flag)&BF_WEAPONMASK &&
							((it.flag)&flag)&BF_RANGEMASK &&
							((it.flag)&flag)&BF_SKILLMASK))
							continue;
						ele_fix += it.rate;
					}
					if (s_defele != ELE_NONE)
						ele_fix += tsd->indexed_bonus.subdefele[s_defele] + tsd->indexed_bonus.subdefele[ELE_ALL];
					cardfix = cardfix * (100 - ele_fix) / 100;
				}
				int race_fix = tsd->indexed_bonus.subrace[sstatus->race] + tsd->indexed_bonus.subrace[RC_ALL];
				for (const auto &it : tsd->subrace3) {
					if (it.race != sstatus->race)
						continue;
					if (!(((it.flag)&flag)&BF_WEAPONMASK &&
						((it.flag)&flag)&BF_RANGEMASK &&
						((it.flag)&flag)&BF_SKILLMASK))
						continue;
					race_fix += it.rate;
				}
				cardfix = cardfix * (100 - race_fix) / 100;
				cardfix = cardfix * (100 - tsd->indexed_bonus.subsize[sstatus->size] - tsd->indexed_bonus.subsize[SZ_ALL]) / 100;
				race_fix = 0;
				for (const auto &raceit : s_race2)
					race_fix += tsd->indexed_bonus.subrace2[raceit];
				cardfix = cardfix * (100 - race_fix) / 100;
				cardfix = cardfix * (100 - tsd->indexed_bonus.subclass[sstatus->class_] - tsd->indexed_bonus.subclass[CLASS_ALL]) / 100;
				cardfix = cardfix * (100 - tsd->bonus.miSTATUS_DEF_RATE) / 100;
				if( flag&BF_SHORT )
					cardfix = cardfix * (100 - tsd->bonus.near_attack_def_rate) / 100;
				else if (!nk[NK_IGNORELONGCARD])	// BF_LONG (there's no other choice)
					cardfix = cardfix * (100 - tsd->bonus.long_attack_def_rate) / 100;
				APPLY_CARDFIX(damage, cardfix);
			}
			break;
	}

#undef APPLY_CARDFIX

	return (int)cap_value(damage - original_damage, INT_MIN, INT_MAX);
}

/**
* Absorb damage based on criteria
* @param bl
* @param d Damage
**/
static void battle_absorb_damage(struct block_list *bl, struct Damage *d) {
	int64 dmg_ori = 0, dmg_new = 0;

	nullpo_retv(bl);
	nullpo_retv(d);

	if (!d->damage && !d->damage2)
		return;

	switch (bl->type) {
		case BL_PC:
			{
				struct map_session_data *sd = BL_CAST(BL_PC, bl);
				if (!sd)
					return;
				if (sd->bonus.absorb_dmg_maxhp) {
					int hp = sd->bonus.absorb_dmg_maxhp * status_get_max_hp(bl) / 100;
					dmg_ori = dmg_new = d->damage + d->damage2;
					if (dmg_ori > hp)
						dmg_new = dmg_ori - hp;
				}
			}
			break;
	}

	if (dmg_ori == dmg_new)
		return;

	if (!d->damage2)
		d->damage = dmg_new;
	else if (!d->damage)
		d->damage2 = dmg_new;
	else {
		d->damage = dmg_new;
		d->damage2 = dmg_new * d->damage2 / dmg_ori / 100;
		if (d->damage2 < 1)
			d->damage2 = 1;
		d->damage = d->damage - d->damage2;
	}
}

/**
 * Check for active statuses that block damage
 * @param src: Attacker
 * @param target: Target of attack
 * @param sc: Status Change data
 * @param d: Damage data
 * @param damage: Damage received as a reference
 * @param skill_id: Skill ID
 * @param skill_lv: Skill level
 * @return True: Damage inflicted, False: Missed
 **/
bool battle_status_block_damage(struct block_list *src, struct block_list *target, struct status_change *sc, struct Damage *d, int64 &damage, uint16 skill_id, uint16 skill_lv) {
	if (!src || !target || !sc || !d)
		return true;

	status_change_entry *sce;
	int flag = d->flag;

	// SC Types that must be first because they may or may not block damage
	if ((sce = sc->data[STATUS_KYRIE]) && damage > 0) {
		sce->val2 -= static_cast<int>(cap_value(damage, INT_MIN, INT_MAX));
		if (flag & BF_WEAPON ) {
			if (sce->val2 >= 0)
				damage = 0;
			else
				damage = -sce->val2;
		}
		if ((--sce->val3) <= 0 || (sce->val2 <= 0))
			status_change_end(target, STATUS_KYRIE, INVALID_TIMER);
	}

	

	

	if (damage == 0)
		return false;

	

	/*if ((sc->data[SC_PNEUMA] && (flag&(BF_MAGIC | BF_LONG)) == BF_LONG) ||
		(sc->data[SC_BASILICA_CELL]
		&& !status_bl_has_mode(src, MD_STATUSIMMUNE)) ||
		(sc->data[SC_ZEPHYR] && !(flag&BF_MAGIC && skill_id) && !(skill_get_inf(skill_id)&(INF_GROUND_SKILL | INF_SELF_SKILL))) ||
		sc->data[SC__MANHOLE] ||
		sc->data[STATUS_MILLENIUMSHIELDS] ||
		sc->data[SC_GRAVITYCONTROL]
		)
	{
		d->dmg_lv = ATK_BLOCK;
		return false;
	}*/

	if (sc->data[STATUS_MAGICCELL]) { // Gravitation and Pressure do damage without removing the effect
		if (skill_id == SK_AL_SACREDWAVE ||
			skill_id == SK_MG_SOULSTRIKE ||
			skill_id == SK_MG_VOIDEXPANSION ||
			(skill_id && skill_get_ele(skill_id, skill_lv) == ELE_GHOST) ||
			(skill_id == 0 && (status_get_status_data(src))->rhw.ele == ELE_GHOST))
		{
			if (skill_id == SK_MG_VOIDEXPANSION)
				damage <<= 1; // If used against a player in White Imprison, the skill deals double damage.
			status_change_end(target, STATUS_MAGICCELL, INVALID_TIMER); // Those skills do damage and removes effect
		} else {
			d->dmg_lv = ATK_BLOCK;
			return false;
		}
	}



	// ATK_MISS Type
	if ((sce = sc->data[STATUS_AUTOGUARD]) && flag&BF_WEAPON && rnd() % 100 < (sce->val2 * 2) && !skill_get_inf2(skill_id, INF2_IGNOREAUTOGUARD)) {
		status_change_entry *sce_d = sc->data[STATUS_SWORNPROTECTOR];
		block_list *d_bl;
		int delay;

		// different delay depending on skill level [celest]
		// if (sce->val1 <= 5)
		// 	delay = 300;
		// else if (sce->val1 > 5 && sce->val1 <= 9)
		// 	delay = 200;
		// else
		delay = 100;

		map_session_data *sd = map_id2sd(target->id);

		if (sd && pc_issit(sd))
			pc_setstand(sd, true);
		if (sce_d && (d_bl = map_id2bl(sce_d->val1)) &&
			((d_bl->type == BL_PC && ((TBL_PC*)d_bl)->devotion[sce_d->val2] == target->id)) &&
			check_distance_bl(target, d_bl, sce_d->val3))
		{ //If player is target of devotion, show guard effect on the devotion caster rather than the target
			clif_skill_nodamage(d_bl, d_bl, SK_CR_AUTOGUARD, sce->val1, 1);
			unit_set_walkdelay(d_bl, gettick(), delay, 1);
			d->dmg_lv = ATK_MISS;
			return false;
		} else {
			clif_skill_nodamage(target, target, SK_CR_AUTOGUARD, sce->val1, 1);
			unit_set_walkdelay(target, gettick(), delay, 1);
			d->dmg_lv = ATK_MISS;
			return false;
		}
	}
	

	// Other
	

	if ((sce = sc->data[STATUS_PARRY]) && flag&BF_WEAPON && rnd() % 100 < sce->val2) {
		clif_skill_nodamage(target, target, SK_CM_PARRY, sce->val1, 1);

		if (skill_id == SK_CM_PARRY) {
			unit_data *ud = unit_bl2ud(target);

			if (ud != nullptr) // Delay the next attack
				ud->attackabletime = gettick() + status_get_adelay(target);
		}
		return false;
	}


	if (flag&BF_MAGIC && (sce = sc->data[STATUS_VANGUARDFORCE]) && rnd() % 100 < sce->val2) {
		clif_specialeffect(target, EF_STORMKICK4, AREA); // Still need confirm it.
		return false;
	}

	if (sce = sc->data[STATUS_MILLENIUMSHIELDS]) {
		clif_specialeffect(target, EF_MAXPAIN, AREA);
		return false;
	}

	return true;
}

/**
 * Check damage through status.
 * ATK may be MISS, BLOCKED FAIL, reduc, increase, end status.
 * After this we apply bg/gvg reduction
 * @param src
 * @param bl
 * @param d
 * @param damage
 * @param skill_id
 * @param skill_lv
 * @return damage
 */
int64 battle_calc_damage(struct block_list *src,struct block_list *bl,struct Damage *d,int64 damage,uint16 skill_id,uint16 skill_lv)
{
	struct map_session_data *sd = NULL, *tsd = BL_CAST(BL_PC, src);
	struct status_change *sc;
	struct status_change_entry *sce;
	int div_ = d->div_, flag = d->flag;

	nullpo_ret(bl);

	if( !damage )
		return 0;
	if( battle_config.ksprotection && mob_ksprotected(src, bl) )
		return 0;

	if( map_getcell(bl->m, bl->x, bl->y, CELL_CHKMAELSTROM) && skill_id && skill_get_type(skill_id) != BF_MISC
		&& skill_get_casttype(skill_id) == CAST_GROUND )
		return 0;

	if (bl->type == BL_PC) {
		sd=(struct map_session_data *)bl;
		//Special no damage states
		if(flag&BF_WEAPON && sd->special_state.no_weapon_damage)
			damage -= damage * sd->special_state.no_weapon_damage / 100;

		if(flag&BF_MAGIC && sd->special_state.no_magic_damage)
			damage -= damage * sd->special_state.no_magic_damage / 100;

		if(flag&BF_MISC && sd->special_state.no_misc_damage)
			damage -= damage * sd->special_state.no_misc_damage / 100;

		if(!damage)
			return 0;
		
		sc = status_get_sc(bl);
		if( sc && sc->data[STATUS_ZEPHYRSNIPING]){
			status_change_end(&sd->bl,STATUS_ZEPHYRSNIPING,INVALID_TIMER);
		}
	}

	sc = status_get_sc(bl); //check target status

	


	
	if( sc && sc->count ) {
		if (!battle_status_block_damage(src, bl, sc, d, damage, skill_id, skill_lv)) // Statuses that reduce damage to 0.
			return 0;



		if (sc->data[STATUS_LEXAETERNA] && skill_id != SK_SA_SOULBURN) {
			if (src->type != BL_MER || !skill_id)
				damage <<= 1; // Lex Aeterna only doubles damage of regular attacks from mercenaries


				status_change_end(bl, STATUS_LEXAETERNA, INVALID_TIMER); //Shouldn't end until Breaker's non-weapon part connects.
		}


		

		
		if (sc->data[STATUS_DARKCLAW] && (flag&(BF_SHORT|BF_MAGIC)) == BF_SHORT)
			damage += damage * sc->data[STATUS_DARKCLAW]->val2 / 100;

		// Damage reductions
		// Assumptio increases DEF on RE mode, otherwise gives a reduction on the final damage. [Igniz]

		if( sc->data[STATUS_ASSUMPTIO] ) {
			// if( map_flag_vs(bl->m) )
			float ratio = 0.95 - 0.05*sc->data[STATUS_ASSUMPTIO]->val1;
			damage = (int64)damage*ratio; //Receive ratio% of damage
		}
		if( sc->data[STATUS_ARMORREINFORCEMENT] ) {
			float ratio = 0.95 - sc->data[STATUS_ARMORREINFORCEMENT]->val2*sc->data[STATUS_ARMORREINFORCEMENT]->val1;
			damage = (int64)damage*ratio; //Receive ratio% of damage
		}

		if(sd) {
			short armor_index;
			armor_index = sd->equip_index[EQI_ARMOR];
			if (sd->inventory.u.items_inventory[armor_index].card[0] == CARD0_FORGE) { // Forged armor
				float ratio = 1;
				float reduction = 0;
				reduction = ((sd->inventory.u.items_inventory[armor_index].card[1] >> 8) * 0.005); //each star cru, gives 2.5% d,g reduction

				if (reduction >= 0.075) reduction = 0.1; // 3 Star Crumbs now give 10% dmg reduction
				ratio -= reduction;

				damage = (int64)damage * ratio; //Receive ratio% of damage
			}
		}
		
		if (sc->data[STATUS_DEFENDINGAURA] && (flag&(BF_LONG|BF_WEAPON)) == (BF_LONG|BF_WEAPON))
			damage -= damage * sc->data[STATUS_DEFENDINGAURA]->val2 / 100;

		



	
		if( sc->data[STATUS_ENERGYCOAT] && (

			((flag&BF_WEAPON || flag&BF_MAGIC))

			) )
		{
			struct status_data *status = status_get_status_data(bl);
			int per = 100*status->sp / status->max_sp -1; //100% should be counted as the 80~99% interval
			per /=20; //Uses 20% SP intervals.
			//SP Cost: 1% + 0.5% per every 20% SP
			if (!status_charge(bl, 0, (10+5*per)*status->max_sp/1000))
				status_change_end(bl, STATUS_ENERGYCOAT, INVALID_TIMER);
			damage -= damage * 6 * (1 + per) / 100; //Reduction: 6% + 6% every 20%
		}

	
	
		if( damage > 0 && (sce = sc->data[STATUS_STONESKIN]) ) {
			if( src->type == BL_MOB ) //using explicit call instead break_equip for duration
				sc_start(src,src, STATUS_STRIPWEAPON, 30, 0, skill_get_time2(SK_KN_STONESKIN, sce->val1));
			else if (flag&(BF_WEAPON|BF_SHORT))
				skill_break_equip(src,src, EQP_WEAPON, 3000, BCT_SELF);
		}

		
		// Renewal: steel body reduces all incoming damage to 1/10 [helvetica]
		if( sc->data[STATUS_MENTALSTRENGTH] ){
			int reduction_percentage_to = 60 - (pc_checkskill(sd, SK_SH_MENTALSTRENGTH)*6);
			damage = damage > reduction_percentage_to ? damage / reduction_percentage_to : 1;
		}

		if (!damage)
			return 0;

		

	} //End of target SC_ check

	//SC effects from caster side.
	sc = status_get_sc(src);

	if (sc && sc->count) {
		

		/*if (flag & BF_WEAPON && (sce = sc->data[SC_ADD_ATK_DAMAGE]))
			damage += damage * sce->val1 / 100;
		if (flag & BF_MAGIC && (sce = sc->data[SC_ADD_MATK_DAMAGE]))
			damage += damage * sce->val1 / 100;*/
		
	} //End of caster SC_ check

	//PK damage rates
	if (battle_config.pk_mode && sd && bl->type == BL_PC && damage && map_getmapflag(bl->m, MF_PVP)) {
		if (flag & BF_SKILL) { //Skills get a different reduction than non-skills. [Skotlex]
			if (flag&BF_WEAPON)
				damage = damage * battle_config.pk_weapon_damage_rate / 100;
			if (flag&BF_MAGIC)
				damage = damage * battle_config.pk_magic_damage_rate / 100;
			if (flag&BF_MISC)
				damage = damage * battle_config.pk_misc_damage_rate / 100;
		} else { //Normal attacks get reductions based on range.
			if (flag & BF_SHORT)
				damage = damage * battle_config.pk_short_damage_rate / 100;
			if (flag & BF_LONG)
				damage = damage * battle_config.pk_long_damage_rate / 100;
		}
		damage = i64max(damage,1);
	}

	if(battle_config.skill_min_damage && damage > 0 && damage < div_) {
		if ((flag&BF_WEAPON && battle_config.skill_min_damage&1)
			|| (flag&BF_MAGIC && battle_config.skill_min_damage&2)
			|| (flag&BF_MISC && battle_config.skill_min_damage&4)
		)
			damage = div_;
	}

	if (tsd && pc_ismadogear(tsd)) {
		short element = skill_get_ele(skill_id, skill_lv);

		if( !skill_id || element == ELE_WEAPON ) { //Take weapon's element
			struct status_data *sstatus = NULL;
			if( src->type == BL_PC && ((TBL_PC*)src)->bonus.arrow_ele )
				element = ((TBL_PC*)src)->bonus.arrow_ele;
			else if( (sstatus = status_get_status_data(src)) ) {
				element = sstatus->rhw.ele;
			}
		} else if( element == ELE_ENDOWED ) //Use enchantment's element
			element = status_get_attack_sc_element(src,status_get_sc(src));
		else if( element == ELE_RANDOM ) //Use random element
			element = rnd()%ELE_ALL;
		pc_overheat(tsd, (element == ELE_FIRE ? 3 : 1));
	}

	if (bl->type == BL_MOB) { // Reduces damage received for Green Aura MVP
		mob_data *md = BL_CAST(BL_MOB, bl);

		if (md && md->db->damagetaken != 100)
			damage = i64max(damage * md->db->damagetaken / 100, 1);
	}

	return damage;
}

/**
 * Determines whether battleground target can be hit
 * @param src: Source of attack
 * @param bl: Target of attack
 * @param skill_id: Skill ID used
 * @param flag: Special flags
 * @return Can be hit (true) or can't be hit (false)
 */
bool battle_can_hit_bg_target(struct block_list *src, struct block_list *bl, uint16 skill_id, int flag)
{
	struct mob_data* md = BL_CAST(BL_MOB, bl);
	struct unit_data *ud = unit_bl2ud(bl);

	if (ud && ud->immune_attack)
		return false;
	if (md && md->bg_id) {
		if (status_bl_has_mode(bl, MD_SKILLIMMUNE) && flag&BF_SKILL) //Skill immunity.
			return false;
		if (src->type == BL_PC) {
			struct map_session_data *sd = map_id2sd(src->id);

			if (sd && sd->bg_id == md->bg_id)
				return false;
		}
	}
	return true;
}

/**
 * Calculates BG related damage adjustments.
 * @param src
 * @param bl
 * @param damage
 * @param skill_id
 * @param flag
 * @return damage
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
int64 battle_calc_bg_damage(struct block_list *src, struct block_list *bl, int64 damage, uint16 skill_id, int flag)
{
	if( !damage )
		return 0;

	if (!battle_can_hit_bg_target(src, bl, skill_id, flag))
		return 0;

	if(skill_get_inf2(skill_id, INF2_IGNOREBGREDUCTION))
		return damage; //skill that ignore bg map reduction

	if( flag&BF_SKILL ) { //Skills get a different reduction than non-skills. [Skotlex]
		if( flag&BF_WEAPON )
			damage = damage * battle_config.bg_weapon_damage_rate / 100;
		if( flag&BF_MAGIC )
			damage = damage * battle_config.bg_magic_damage_rate / 100;
		if(	flag&BF_MISC )
			damage = damage * battle_config.bg_misc_damage_rate / 100;
	} else { //Normal attacks get reductions based on range.
		if( flag&BF_SHORT )
			damage = damage * battle_config.bg_short_damage_rate / 100;
		if( flag&BF_LONG )
			damage = damage * battle_config.bg_long_damage_rate / 100;
	}

	damage = i64max(damage,1); //min 1 damage
	return damage;
}

/**
 * Determines whether target can be hit
 * @param src
 * @param bl
 * @param skill_id
 * @param flag
 * @return Can be hit (true) or can't be hit (false)
 */
bool battle_can_hit_gvg_target(struct block_list *src,struct block_list *bl,uint16 skill_id,int flag)
{
	struct mob_data* md = BL_CAST(BL_MOB, bl);
	struct unit_data *ud = unit_bl2ud(bl);
	int class_ = status_get_class(bl);

	if (ud && ud->immune_attack)
		return false;
	if(md && (md->guardian_data || md->special_state.ai == AI_GUILD)) {
		if ((status_bl_has_mode(bl,MD_SKILLIMMUNE) || (class_ == MOBID_EMPERIUM && !skill_get_inf2(skill_id, INF2_TARGETEMPERIUM))) && flag&BF_SKILL) //Skill immunity.
			return false;
		if( src->type != BL_MOB || mob_is_clone( ((struct mob_data*)src)->mob_id ) ){
			struct guild *g = src->type == BL_PC ? ((TBL_PC *)src)->guild : guild_search(status_get_guild_id(src));

			if (class_ == MOBID_EMPERIUM && (!g || guild_checkskill(g,GD_APPROVAL) <= 0 ))
				return false;

			if (g != nullptr) {
				if (battle_config.guild_max_castles && guild_checkcastles(g)>=battle_config.guild_max_castles)
					return false; // [MouseJstr]

				if (md->special_state.ai == AI_GUILD && g->guild_id == md->master_id)
					return false;
			}
		}
	}
	return true;
}

/**
 * Calculates GVG related damage adjustments.
 * @param src
 * @param bl
 * @param damage
 * @param skill_id
 * @param flag
 * @return damage
 */
int64 battle_calc_gvg_damage(struct block_list *src,struct block_list *bl,int64 damage,uint16 skill_id,int flag)
{
	if (!damage) //No reductions to make.
		return 0;

	if (!battle_can_hit_gvg_target(src,bl,skill_id,flag))
		return 0;

	if (skill_get_inf2(skill_id, INF2_IGNOREGVGREDUCTION)) //Skills with no gvg damage reduction.
		return damage;

	if (flag & BF_SKILL) { //Skills get a different reduction than non-skills. [Skotlex]
		if (flag&BF_WEAPON)
			damage = damage * battle_config.gvg_weapon_damage_rate / 100;
		if (flag&BF_MAGIC)
			damage = damage * battle_config.gvg_magic_damage_rate / 100;
		if (flag&BF_MISC)
			damage = damage * battle_config.gvg_misc_damage_rate / 100;
	} else { //Normal attacks get reductions based on range.
		if (flag & BF_SHORT)
			damage = damage * battle_config.gvg_short_damage_rate / 100;
		if (flag & BF_LONG)
			damage = damage * battle_config.gvg_long_damage_rate / 100;
	}
	damage = i64max(damage,1);
	return damage;
}

/**
 * HP/SP drain calculation
 * @param damage Damage inflicted to the enemy
 * @param rate Success chance 1000 = 100%
 * @param per HP/SP drained
 * @return diff
 */
static int battle_calc_drain(int64 damage, int rate, int per)
{
	int64 diff = 0;

	if (per && (rate > 1000 || rnd()%1000 < rate)) {
		diff = (damage * per) / 100;
		if (diff == 0) {
			if (per > 0)
				diff = 1;
			else
				diff = -1;
		}
	}
	return (int)cap_value(diff, INT_MIN, INT_MAX);
}

/*==========================================
 * Consumes ammo for the given skill.
 *------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
void battle_consume_ammo(struct map_session_data*sd, int skill, int lv)
{
	int qty = 1;

	if (!battle_config.arrow_decrement)
		return;

	if (skill) {
		qty = skill_get_ammo_qty(skill, lv);
		if (!qty) qty = 1;
	}

	if (sd->equip_index[EQI_AMMO] >= 0) //Qty check should have been done in skill_check_condition
		pc_delitem(sd,sd->equip_index[EQI_AMMO],qty,0,1,LOG_TYPE_CONSUME);

	sd->state.arrow_atk = 0;
}

static int battle_range_type(struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv)
{
	// [Akinari] , [Xynvaroth]: Traps are always short range.
	if (skill_get_inf2(skill_id, INF2_ISTRAP))
		return BF_SHORT;

	switch (skill_id) {
		case SK_AC_ARROWSHOWER:
		case SK_AC_ARROWRAIN:
			// When monsters use Arrow Shower or Bomb, it is always short range
			if (src->type == BL_MOB)
				return BF_SHORT;
			break;
		case SK_KN_BRANDISHSPEAR:
			// Renewal changes to ranged physical damage
			return BF_LONG;
		case SK_EX_DUMMY_CROSSIMPACT:
			// Cast range is 7 cells and player jumps to target but skill is considered melee
			return BF_SHORT;
	}

	//Skill Range Criteria
	if (battle_config.skillrange_by_distance &&
		(src->type&battle_config.skillrange_by_distance)
	) { //based on distance between src/target [Skotlex]
		if (check_distance_bl(src, target, 3))
			return BF_SHORT;
		return BF_LONG;
	}

	//based on used skill's range
	if (skill_get_range2(src, skill_id, skill_lv, true) < 4)
		return BF_SHORT;
	return BF_LONG;
}

static int battle_blewcount_bonus(struct map_session_data *sd, uint16 skill_id)
{
	if (sd->skillblown.empty())
		return 0;
	//Apply the bonus blewcount. [Skotlex]
	for (const auto &it : sd->skillblown) {
		if (it.id == skill_id)
			return it.val;
	}
	return 0;
}

static enum e_skill_damage_type battle_skill_damage_type( struct block_list* bl ){
	switch( bl->type ){
		case BL_PC:
			return SKILLDMG_PC;
		case BL_MOB:
			if( status_get_class_(bl) == CLASS_BOSS ){
				return SKILLDMG_BOSS;
			}else{
				return SKILLDMG_MOB;
			}
		default:
			return SKILLDMG_OTHER;
	}
}

/**
 * Gets skill damage rate from a skill (based on skill_damage_db.txt)
 * @param src
 * @param target
 * @param skill_id
 * @return Skill damage rate
 */
static int battle_skill_damage_skill(struct block_list *src, struct block_list *target, uint16 skill_id) {
	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);

	if (!skill || !skill->damage.map)
		return 0;

	s_skill_damage *damage = &skill->damage;

	//check the adjustment works for specified type
	if (!(damage->caster&src->type))
		return 0;

	map_data *mapdata = map_getmapdata(src->m);

	if ((damage->map&1 && (!mapdata->flag[MF_PVP] && !mapdata_flag_gvg2(mapdata) && !mapdata->flag[MF_BATTLEGROUND] && !mapdata->flag[MF_SKILL_DAMAGE] && !mapdata->flag[MF_RESTRICTED])) ||
		(damage->map&2 && mapdata->flag[MF_PVP]) ||
		(damage->map&4 && mapdata_flag_gvg2(mapdata)) ||
		(damage->map&8 && mapdata->flag[MF_BATTLEGROUND]) ||
		(damage->map&16 && mapdata->flag[MF_SKILL_DAMAGE]) ||
		(damage->map&mapdata->zone && mapdata->flag[MF_RESTRICTED]))
	{
		return damage->rate[battle_skill_damage_type(target)];
	}

	return 0;
}

/**
 * Gets skill damage rate from a skill (based on 'skill_damage' mapflag)
 * @param src
 * @param target
 * @param skill_id
 * @return Skill damage rate
 */
static int battle_skill_damage_map(struct block_list *src, struct block_list *target, uint16 skill_id) {
	map_data *mapdata = map_getmapdata(src->m);

	if (!mapdata || !mapdata->flag[MF_SKILL_DAMAGE])
		return 0;

	int rate = 0;

	// Damage rate for all skills at this map
	if (mapdata->damage_adjust.caster&src->type)
		rate = mapdata->damage_adjust.rate[battle_skill_damage_type(target)];

	if (mapdata->skill_damage.empty())
		return rate;

	// Damage rate for specified skill at this map
	if (mapdata->skill_damage.find(skill_id) != mapdata->skill_damage.end() && mapdata->skill_damage[skill_id].caster&src->type) {
		rate += mapdata->skill_damage[skill_id].rate[battle_skill_damage_type(target)];
	}
	return rate;
}

/**
 * Check skill damage adjustment based on mapflags and skill_damage_db.txt for specified skill
 * @param src
 * @param target
 * @param skill_id
 * @return Total damage rate
 */
static int battle_skill_damage(struct block_list *src, struct block_list *target, uint16 skill_id) {
	nullpo_ret(src);
	if (!target || !skill_id)
		return 0;
	skill_id = skill_dummy2skill_id(skill_id);
	return battle_skill_damage_skill(src, target, skill_id) + battle_skill_damage_map(src, target, skill_id);
}

/**
 * Calculates Minstrel/Wanderer bonus for Chorus skills.
 * @param sd: Player who has Chorus skill active
 * @return Bonus value based on party count
 */
int battle_calc_chorusbonus(struct map_session_data *sd) {
#ifdef RENEWAL // No bonus in renewal
	return 0;
#endif

	int members = 0;

	if (!sd || !sd->status.party_id)
		return 0;

	members = party_foreachsamemap(party_sub_count_class, sd, 0, MAPID_THIRDMASK, MAPID_MINSTRELWANDERER);

	if (members < 3)
		return 0; // Bonus remains 0 unless 3 or more Minstrels/Wanderers are in the party.
	if (members > 7)
		return 5; // Maximum effect possible from 7 or more Minstrels/Wanderers.
	return members - 2; // Effect bonus from additional Minstrels/Wanderers if not above the max possible.
}

struct Damage battle_calc_magic_attack(struct block_list *src,struct block_list *target,uint16 skill_id,uint16 skill_lv,int mflag);
struct Damage battle_calc_misc_attack(struct block_list *src,struct block_list *target,uint16 skill_id,uint16 skill_lv,int mflag);

/*=======================================================
 * Should infinite defense be applied on target? (plant)
 *-------------------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
  *	flag - see e_battle_flag
 */
bool is_infinite_defense(struct block_list *target, int flag)
{
	struct status_data *tstatus = status_get_status_data(target);

	if(target->type == BL_SKILL) {
		TBL_SKILL *su = ((TBL_SKILL*)target);
	}

	if(status_has_mode(tstatus,MD_IGNOREMELEE) && (flag&(BF_WEAPON|BF_SHORT)) == (BF_WEAPON|BF_SHORT) )
		return true;
	if(status_has_mode(tstatus,MD_IGNOREMAGIC) && flag&(BF_MAGIC) )
		return true;
	if(status_has_mode(tstatus,MD_IGNORERANGED) && (flag&(BF_WEAPON|BF_LONG)) == (BF_WEAPON|BF_LONG) )
		return true;
	if(status_has_mode(tstatus,MD_IGNOREMISC) && flag&(BF_MISC) )
		return true;

	return false;
}

/*========================
 * Is attack arrow based?
 *------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static bool is_skill_using_arrow(struct block_list *src, int skill_id)
{
	if(src != NULL) {
		struct status_data *sstatus = status_get_status_data(src);
		struct map_session_data *sd = BL_CAST(BL_PC, src);

		return ((sd && sd->state.arrow_atk) || (!sd && ((skill_id && skill_get_ammotype(skill_id)) || sstatus->rhw.range>3)));
	} else
		return false;
}

/*=========================================
 * Is attack right handed? By default yes.
 *-----------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static bool is_attack_right_handed(struct block_list *src, int skill_id)
{
	if(src != NULL) {
		struct map_session_data *sd = BL_CAST(BL_PC, src);

		//Skills ALWAYS use ONLY your right-hand weapon (tested on Aegis 10.2)
		if(!skill_id && sd && sd->weapontype1 == W_FIST && sd->weapontype2 != W_FIST)
			return false;
	}
	return true;
}

/*==========================================================
 * Is the attack piercing? (Investigate/Ice Pick in pre-re)
 *----------------------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static int is_attack_piercing(struct Damage* wd, struct block_list *src, struct block_list *target, int skill_id, int skill_lv, short weapon_position)
{
	if (skill_id == SK_MO_OCCULTIMPACT)
		return 2;

	if(src != NULL) {
		struct map_session_data *sd = BL_CAST(BL_PC, src);
		struct status_data *tstatus = status_get_status_data(target);

		if( skill_id != SK_KN_RECKONING && skill_id != SK_HT_LIVINGTORNADO && skill_id != SK_RA_ZEPHYRSNIPING)
		{ //Elemental/Racial adjustments
			if( sd && (sd->right_weapon.def_ratio_atk_ele & (1<<tstatus->def_ele) || sd->right_weapon.def_ratio_atk_ele & (1<<ELE_ALL) ||
				sd->right_weapon.def_ratio_atk_race & (1<<tstatus->race) || sd->right_weapon.def_ratio_atk_race & (1<<RC_ALL) ||
				sd->right_weapon.def_ratio_atk_class & (1<<tstatus->class_) || sd->right_weapon.def_ratio_atk_class & (1<<CLASS_ALL))
			)
				if (weapon_position == EQI_HAND_R)
					return 1;

			if(sd && (sd->left_weapon.def_ratio_atk_ele & (1<<tstatus->def_ele) || sd->left_weapon.def_ratio_atk_ele & (1<<ELE_ALL) ||
				sd->left_weapon.def_ratio_atk_race & (1<<tstatus->race) || sd->left_weapon.def_ratio_atk_race & (1<<RC_ALL) ||
				sd->left_weapon.def_ratio_atk_class & (1<<tstatus->class_) || sd->left_weapon.def_ratio_atk_class & (1<<CLASS_ALL))
			)
			{ //Pass effect onto right hand if configured so. [Skotlex]
				if (battle_config.left_cardfix_to_right && is_attack_right_handed(src, skill_id)){
					if (weapon_position == EQI_HAND_R)
						return 1;
				}
				else if (weapon_position == EQI_HAND_L)
					return 1;
			}
		}
	}
	return 0;
}

static std::bitset<NK_MAX> battle_skill_get_damage_properties(uint16 skill_id, int is_splash)
{
	if (skill_id == 0) {
		if (is_splash) {
			std::bitset<NK_MAX> tmp_nk;

			tmp_nk.set(NK_IGNOREATKCARD);
			tmp_nk.set(NK_IGNOREFLEE);

			return tmp_nk;
		} else
			return 0;
	} else
		return skill_db.find(skill_id)->nk;
}

/*=============================
 * Checks if attack is hitting
 *-----------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static bool is_attack_hitting(struct Damage* wd, struct block_list *src, struct block_list *target, int skill_id, int skill_lv, bool first_call)
{
	struct status_data *sstatus = status_get_status_data(src);
	struct status_data *tstatus = status_get_status_data(target);
	struct status_change *sc = status_get_sc(src);
	struct status_change *tsc = status_get_sc(target);
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	std::bitset<NK_MAX> nk = battle_skill_get_damage_properties(skill_id, wd->miscflag);
	short flee, hitrate;

	if (!first_call)
		return (wd->dmg_lv != ATK_FLEE);
	if (CriticalHitCalculator::is_attack_critical(wd, src, target, skill_id, skill_lv, false))
		return true;
	else if(sd && sd->bonus.perfect_hit > 0 && rnd()%100 < sd->bonus.perfect_hit)
		return true;
	else if ((skill_id == SK_AS_VENOMSPLASHER || skill_id == SK_AM_BOMB) && !wd->miscflag)
		return true;
	else if (tsc && tsc->opt1 && tsc->opt1 != OPT1_STONEWAIT && tsc->opt1 != OPT1_BURNING)
		return true;
	else if (nk[NK_IGNOREFLEE])
		return true;

	// if( tsc && tsc->data[STATUS_NEUTRALBARRIER] && (wd->flag&(BF_LONG|BF_MAGIC)) == BF_LONG )
	// 	return false;

	flee = tstatus->flee;
#ifdef RENEWAL
	hitrate = 0; //Default hitrate
#else
	hitrate = 80; //Default hitrate
#endif

	if(battle_config.agi_penalty_type && battle_config.agi_penalty_target&target->type) {
		unsigned char attacker_count = unit_counttargeted(target); //256 max targets should be a sane max

		if(attacker_count >= battle_config.agi_penalty_count) {
			if (battle_config.agi_penalty_type == 1)
				flee = (flee * (100 - (attacker_count - (battle_config.agi_penalty_count - 1)) * battle_config.agi_penalty_num)) / 100;
			else //assume type 2: absolute reduction
				flee -= (attacker_count - (battle_config.agi_penalty_count - 1)) * battle_config.agi_penalty_num;
			if(flee < 1)
				flee = 1;
		}
	}

	hitrate += sstatus->hit - flee;

	

	if(sd && is_skill_using_arrow(src, skill_id))
		hitrate += sd->bonus.arrow_hit;

#ifdef RENEWAL
	if (sd) //in Renewal hit bonus from Vultures Eye is not anymore shown in status window
		hitrate += pc_checkskill(sd,SK_AC_VULTURE) * 2;
#endif

	if(skill_id) {
		switch(skill_id) { //Hit skill modifiers
			//It is proven that bonus is applied on final hitrate, not hit.
			case SK_SM_BASH:
				hitrate += hitrate * 5 * skill_lv / 100;
				break;
			case SK_SM_MAGNUM:
				hitrate += hitrate * 10 * skill_lv / 100;
				break;
			case SK_KN_COUNTERATTACK:
				hitrate += hitrate * 20 / 100;
				break;
			case SK_KN_PIERCE:
				hitrate += hitrate * 5 * skill_lv / 100;
				break;
			case SK_KN_SONICWAVE:
				hitrate += hitrate * 3 * skill_lv / 100; // !TODO: Confirm the hitrate bonus
				break;
		}
	} else if (sd && wd->type&DMG_MULTI_HIT && wd->div_ == 2) // +1 hit per level of Double Attack on a successful double attack (making sure other multi attack skills do not trigger this) [helvetica]
		hitrate += (pc_checkskill(sd,SK_AS_DOUBLEATTACK) *2);

	if (sd) {
		int skill = 0;
	}

	

	hitrate = cap_value(hitrate, battle_config.min_hitrate, battle_config.max_hitrate);
	return (rnd()%100 < hitrate);
}

/*==========================================
 * If attack ignores def.
 *------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static bool attack_ignores_def(struct Damage* wd, struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv, short weapon_position)
{
	struct status_data *tstatus = status_get_status_data(target);
	struct status_change *sc = status_get_sc(src);
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	std::bitset<NK_MAX> nk = battle_skill_get_damage_properties(skill_id, wd->miscflag);

	if (true)
	{	//Ignore Defense?
		if (sd && (sd->right_weapon.ignore_def_ele & (1<<tstatus->def_ele) || sd->right_weapon.ignore_def_ele & (1<<ELE_ALL) ||
			sd->right_weapon.ignore_def_race & (1<<tstatus->race) || sd->right_weapon.ignore_def_race & (1<<RC_ALL) ||
			sd->right_weapon.ignore_def_class & (1<<tstatus->class_) || sd->right_weapon.ignore_def_class & (1<<CLASS_ALL)))
			if (weapon_position == EQI_HAND_R)
				return true;

		if (sd && (sd->left_weapon.ignore_def_ele & (1<<tstatus->def_ele) || sd->left_weapon.ignore_def_ele & (1<<ELE_ALL) ||
			sd->left_weapon.ignore_def_race & (1<<tstatus->race) || sd->left_weapon.ignore_def_race & (1<<RC_ALL) ||
			sd->left_weapon.ignore_def_class & (1<<tstatus->class_) || sd->left_weapon.ignore_def_class & (1<<CLASS_ALL)))
		{
			if(battle_config.left_cardfix_to_right && is_attack_right_handed(src, skill_id)) {//Move effect to right hand. [Skotlex]
				if (weapon_position == EQI_HAND_R)
					return true;
			} else if (weapon_position == EQI_HAND_L)
				return true;
		}
	} else if (skill_id == SK_KN_WINDCUTTER && sd && sd->status.weapon == W_2HSWORD)
		return true;

	return nk[NK_IGNOREDEFENSE] != 0;
}


/*========================================
 * Do element damage modifier calculation
 *----------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static void battle_calc_element_damage(struct Damage* wd, struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv)
{
	std::bitset<NK_MAX> nk = battle_skill_get_damage_properties(skill_id, wd->miscflag);

	// Elemental attribute fix
	if(!nk[NK_IGNOREELEMENT] && (wd->damage > 0 || wd->damage2 > 0)) {
		struct map_session_data *sd = BL_CAST(BL_PC, src);
		struct status_change *sc = status_get_sc(src);
		struct status_data *sstatus = status_get_status_data(src);
		struct status_data *tstatus = status_get_status_data(target);
		int left_element = EquipmentAttackCalculator::battle_get_weapon_element(wd, src, target, skill_id, skill_lv, EQI_HAND_L, true);
		int right_element = EquipmentAttackCalculator::battle_get_weapon_element(wd, src, target, skill_id, skill_lv, EQI_HAND_R, true);

		switch (skill_id) {
			case SK_RA_ZEPHYRSNIPING:
			case SK_HT_LIVINGTORNADO:
			case SK_KN_RECKONING:
				wd->damage = battle_attr_fix(src, target, wd->damage, right_element, tstatus->def_ele, tstatus->ele_lv);
				if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id))
					wd->damage2 = battle_attr_fix(src, target, wd->damage2, left_element, tstatus->def_ele, tstatus->ele_lv);
				break;
			default:
				if (skill_id == 0 && (battle_config.attack_attr_none & src->type))
					return; // Non-player basic attacks are non-elemental, they deal 100% against all defense elements
#ifdef RENEWAL
				if (sd == nullptr) { // Renewal player's elemental damage calculation is already done before this point, only calculate for everything else
#endif
					wd->damage = battle_attr_fix(src, target, wd->damage, right_element, tstatus->def_ele, tstatus->ele_lv);
					if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id))
						wd->damage2 = battle_attr_fix(src, target, wd->damage2, left_element, tstatus->def_ele, tstatus->ele_lv);
#ifdef RENEWAL
				}
#endif
				break;
		}


#ifdef RENEWAL
		if (sd == nullptr) { // Only monsters have a single ATK for element, in pre-renewal we also apply element to entire ATK on players [helvetica]
#endif
			if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id) && wd->damage2 > 0)
				wd->damage2 = battle_attr_fix(src, target, wd->damage2, left_element, tstatus->def_ele, tstatus->ele_lv);
			if (sc && sc->data[STATUS_WATK_ELEMENT] && (wd->damage || wd->damage2)) {
				// Descriptions indicate this means adding a percent of a normal attack in another element. [Skotlex]
				int64 damage = SkillBaseDamageCalculator::battle_calc_base_damage(src, sstatus, &sstatus->rhw, sc, tstatus->size, (is_skill_using_arrow(src, skill_id) ? 2 : 0)) * sc->data[STATUS_WATK_ELEMENT]->val2 / 100;

				wd->damage += battle_attr_fix(src, target, damage, sc->data[STATUS_WATK_ELEMENT]->val1, tstatus->def_ele, tstatus->ele_lv);
				if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) {
					damage = SkillBaseDamageCalculator::battle_calc_base_damage(src, sstatus, &sstatus->lhw, sc, tstatus->size, (is_skill_using_arrow(src, skill_id) ? 2 : 0)) * sc->data[STATUS_WATK_ELEMENT]->val2 / 100;
					wd->damage2 += battle_attr_fix(src, target, damage, sc->data[STATUS_WATK_ELEMENT]->val1, tstatus->def_ele, tstatus->ele_lv);
				}
			}
#ifdef RENEWAL
		}
#endif
	}
}

#define ATK_RATE(damage, damage2, a) do { int64 rate_ = (a); (damage) = (damage) * rate_ / 100; if(EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) (damage2) = (damage2) * rate_ / 100; } while(0);
#define ATK_RATE2(damage, damage2, a , b) do { int64 rate_ = (a), rate2_ = (b); (damage) = (damage) *rate_ / 100; if(EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) (damage2) = (damage2) * rate2_ / 100; } while(0);
#define ATK_RATER(damage, a) { (damage) = (damage) * (a) / 100; }
#define ATK_RATEL(damage2, a) { (damage2) = (damage2) * (a) / 100; }
//Adds dmg%. 100 = +100% (double) damage. 10 = +10% damage
#define ATK_ADDRATE(damage, damage2, a) do { int64 rate_ = (a); (damage) += (damage) * rate_ / 100; if(EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) (damage2) += (damage2) *rate_ / 100; } while(0);
#define ATK_ADDRATE2(damage, damage2, a , b) do { int64 rate_ = (a), rate2_ = (b); (damage) += (damage) * rate_ / 100; if(EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) (damage2) += (damage2) * rate2_ / 100; } while(0);
//Adds an absolute value to damage. 100 = +100 damage
#define ATK_ADD(damage, damage2, a) do { int64 rate_ = (a); (damage) += rate_; if(EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) (damage2) += rate_; } while(0);
#define ATK_ADD2(damage, damage2, a , b) do { int64 rate_ = (a), rate2_ = (b); (damage) += rate_; if(EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) (damage2) += rate2_; } while(0);

#ifdef RENEWAL
	#define RE_ALLATK_ADD(wd, a) do { int64 a_ = (a); ATK_ADD((wd)->statusAtk, (wd)->statusAtk2, a_); ATK_ADD((wd)->weaponAtk, (wd)->weaponAtk2, a_); ATK_ADD((wd)->equipAtk, (wd)->equipAtk2, a_); ATK_ADD((wd)->masteryAtk, (wd)->masteryAtk2, a_); } while(0);
	#define RE_ALLATK_RATE(wd, a) do { int64 a_ = (a); ATK_RATE((wd)->statusAtk, (wd)->statusAtk2, a_); ATK_RATE((wd)->weaponAtk, (wd)->weaponAtk2, a_); ATK_RATE((wd)->equipAtk, (wd)->equipAtk2, a_); ATK_RATE((wd)->masteryAtk, (wd)->masteryAtk2, a_); } while(0);
	#define RE_ALLATK_ADDRATE(wd, a) do { int64 a_ = (a); ATK_ADDRATE((wd)->statusAtk, (wd)->statusAtk2, a_); ATK_ADDRATE((wd)->weaponAtk, (wd)->weaponAtk2, a_); ATK_ADDRATE((wd)->equipAtk, (wd)->equipAtk2, a_); ATK_ADDRATE((wd)->masteryAtk, (wd)->masteryAtk2, a_); } while(0);
#else
	#define RE_ALLATK_ADD(wd, a) {;}
	#define RE_ALLATK_RATE(wd, a) {;}
	#define RE_ALLATK_ADDRATE(wd, a) {;}
#endif


//For quick div adjustment.
#define DAMAGE_DIV_FIX(dmg, div) { if ((div) < 0) { (div) *= -1; (dmg) /= (div); } (dmg) *= (div); }
#define DAMAGE_DIV_FIX2(dmg, div) { if ((div) > 1) (dmg) *= div; }

/*================================================= [Playtester]
 * Applies DAMAGE_DIV_FIX and checks for min damage
 * @param d: Damage struct to apply DAMAGE_DIV_FIX to
 * @param skill_id: ID of the skill that deals damage
 * @return Modified damage struct
 *------------------------------------------------*/
static void battle_apply_div_fix(struct Damage* d, uint16 skill_id)
{
	if(d->damage) {
		DAMAGE_DIV_FIX(d->damage, d->div_);
		//Min damage
		if(d->damage < d->div_ && ((battle_config.skill_min_damage&d->flag)))
			d->damage = d->div_;
	} else if (d->div_ < 0) {
		d->div_ *= -1;
	}
}

/*=======================================
 * Check for and calculate multi attacks
 *---------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static void battle_calc_multi_attack(struct Damage* wd, struct block_list *src,struct block_list *target, uint16 skill_id, uint16 skill_lv)
{
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	struct status_change *sc = status_get_sc(src);
	struct status_change *tsc = status_get_sc(target);
	struct status_data *tstatus = status_get_status_data(target);

	if( sd && !skill_id ) {	// if no skill_id passed, check for double attack [helvetica]
		short i;
		if (is_attack_right_handed(src, skill_id) 
			&& EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)
			&& sd->weapontype1 != W_KATAR) {
			if( ( ( skill_lv = (pc_checkskill(sd,SK_AS_DOUBLEATTACK)) ) > 0 )) // Will fail bare-handed
			{	//Success chance is not added, the higher one is used [Skotlex]
				int max_rate = 0;
				max_rate = max(10 * skill_lv, sd->bonus.double_rate);
				if( rnd()%100 < max_rate ) {
					wd->div_ = skill_get_num(SK_AS_DOUBLEATTACK,skill_lv?skill_lv:1);
					wd->type = DMG_MULTI_HIT;
				}
			}
		
		}
		else if(sc && sc->data[STATUS_TYPHOONFLOW] && sd->weapontype1==W_BOW
			&& (i = sd->equip_index[EQI_AMMO]) >= 0 && sd->inventory_data[i] && sd->inventory.u.items_inventory[i].amount > 1)
		{
			int chance =sc->data[STATUS_TYPHOONFLOW]->val1 * 6;
			if(rnd()%100 <= chance){
				wd->div_ = 3;
			}
			wd->div_ = min(wd->div_,sd->inventory.u.items_inventory[i].amount);
			sc->data[STATUS_TYPHOONFLOW]->val4 = wd->div_-1;
			if (wd->div_ > 1)
				wd->type = DMG_MULTI_HIT;
		}
	}

	switch (skill_id) {
		case SK_TF_POISONREACT:
			skill_lv = pc_checkskill(sd, SK_AS_DOUBLEATTACK);
			if (skill_lv > 0) {
				if(rnd()%100 < (7 * skill_lv * 2)) {
					wd->div_++;
				}
			}
		break;
	}
}

/*======================================================
 * Calculate skill level ratios for weapon-based skills
 *------------------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static int battle_calc_attack_skill_ratio(struct Damage* wd, struct block_list *src,struct block_list *target,uint16 skill_id,uint16 skill_lv)
{
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	struct map_session_data *tsd = BL_CAST(BL_PC, target);
	struct status_change *sc = status_get_sc(src);
	struct status_change *tsc = status_get_sc(target);
	struct status_data *sstatus = status_get_status_data(src);
	struct status_data *tstatus = status_get_status_data(target);
	int skillratio = 100;
	int i;

	//Skill damage modifiers that stack linearly
	if(sc && skill_id != SK_KN_RECKONING && skill_id != SK_HT_LIVINGTORNADO && skill_id != SK_RA_ZEPHYRSNIPING) {

		if( skill_id == SK_CR_SMITE || skill_id == SK_CR_SHIELDBOOMERANG || skill_id == SK_PA_RAPIDSMITING || skill_id == SK_PA_SHIELDSLAM ) { //Refine bonus applies after cards and elements.
			short index = sd->equip_index[EQI_HAND_L];

			if( index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == IT_ARMOR )
				skillratio += sd->inventory.u.items_inventory[index].refine * 50;
		}


		if(sc->data[STATUS_POWERTHRUST])
			skillratio += sc->data[STATUS_POWERTHRUST]->val3;
		if(sc->data[STATUS_MAXPOWERTHRUST])
			skillratio += sc->data[STATUS_MAXPOWERTHRUST]->val2;
		if(sc->data[STATUS_FRENZY])
			skillratio += sc->data[STATUS_FRENZY]->val1*40;
		if(sc->data[STATUS_UNLIMIT])
			skillratio += sc->data[STATUS_UNLIMIT]->val1*50;
		
		if (sc->data[STATUS_SPEARDYNAMO]){
			skillratio += sc->data[STATUS_SPEARDYNAMO]->val2;
		}
		if (pc_checkskill(sd,SK_CM_TWOHAND) && sd->status.weapon == W_2HSWORD){
			skillratio += pc_checkskill(sd,SK_CM_TWOHAND)*10;
		}
		
	}

	switch(skill_id) {
		case SK_SM_BASH:
		case SK_SM_MAGNUM:
		case SK_SM_TRAUMATIC_STRIKE:
		case SK_SM_LIGHTNING_STRIKE:
		case SK_SM_SPEARSTAB:
			skillratio += SwordsmanSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
			break;
		case SK_TF_THROWSTONE:
		case SK_TF_POISONSLASH:
		case SK_TF_SNATCH:
		case SK_TF_SANDATTACK:
		case SK_TF_VENOMKNIFE:
		case SK_TF_POISONREACT:
			skillratio += ThiefSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv);
			break;
		case SK_MC_CARTQUAKE:
		case SK_MC_MAMMONITE:
		case SK_MC_CARTCYCLONE:
			skillratio += MerchntSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus, sd->status.zeny);
			break;
		case SK_AC_ARROWSHOWER:
		case SK_AC_ARROWRAIN:
		case SK_AC_DOUBLESTRAFE:
		case SK_AC_SPIRITUALSTRAFE:
		case SK_AC_CHARGEDARROW:
		case SK_AC_TRANQUILIZINGDART:
		case SK_AC_PARALIZINGDART:
			skillratio += ArcherSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv);
			break;
		case SK_KN_SPEARCANNON:
		case SK_KN_BOWLINGBASH:
		case SK_KN_COUNTERATTACK:
		case SK_KN_BRANDISHSPEAR:
		case SK_KN_PIERCE:
		case SK_KN_WINDCUTTER:
		case SK_KN_SONICWAVE:
		case SK_CR_SMITE:
		case SK_CR_SHIELDBOOMERANG:
		case SK_KN_RECKONING:
		case SK_CM_HUNDREDSPEAR:
		case SK_CM_CLASHINGSPIRAL:
		case SK_CM_IGNITIONBREAK:
			skillratio += KnightSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
			break;
		case SK_CR_HOLYCROSS:
		case SK_CR_GRANDCROSS:
		case SK_PA_RAPIDSMITING:
		case SK_PA_SHIELDSLAM:
			skillratio += CrusaderSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
			break;
		case SK_AS_PHANTOMMENACE:
		case SK_RG_BACKSTAB:
		case SK_AS_GRIMTOOTH:
		case SK_RG_HACKANDSLASH:
		case SK_RG_SHADYSLASH:
		case SK_RG_QUICKSHOT:
		case SK_RG_TRIANGLESHOT:
		case SK_RG_ARROWSTORM:
		case SK_ST_FATALMENACE:
			skillratio += RogueSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus, sd->sc.data[STATUS_HIDING]);
			break;
		case SK_AS_SONICBLOW:
		case SK_AS_ROLLINGCUTTER:
		case SK_AS_POISONOUSCLOUD:
		case SK_AS_CROSSRIPPERSLASHER:
		case SK_AS_VENOMSPLASHER:
		case SK_EX_METEORASSAULT:
		case SK_AS_KUNAI:
		case SK_EX_DARKCLAW:
		case SK_EX_DUMMY_CROSSIMPACT:
		case SK_EX_CROSSIMPACT:
		case SK_AS_SHURIKEN:
		case SK_EX_SOULDESTROYER:
		case SK_EX_POISONBUSTER:
			if (sc && sc->data[STATUS_ROLLINGCUTTER])
				skillratio += AssassinSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus, sc->data[STATUS_ROLLINGCUTTER]->val1);
			skillratio += AssassinSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus, 0);
			break;
		case SK_PR_DUPLELUX_MELEE:
		case SK_PR_UNHOLYCROSS:
			skillratio += PriestSkillAttackRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
			break;
		case SK_MC_CARTBRUME:
		case SK_MC_CARTFIREWORKS:
		case SK_MS_CARTTERMINATION:
		case SK_BS_CARTSHRAPNEL:
		case SK_BS_CARTCANNON:
		case SK_BS_AXEBOOMERANG:
		case SK_BS_AXETORNADO:
		case SK_BS_HAMMERFALL:
		case SK_MS_HAMMERDOWNPROTOCOL:
		case SK_MS_POWERSWING:
		case SK_BS_CROWDCONTROLSHOT:
		case SK_MS_TRIGGERHAPPYCART:
			skillratio += BlacksmithSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
			break;
		case SK_BA_MELODYSTRIKE:
		case SK_BA_GREATECHO:
		case SK_CL_ARROWVULCAN:
		case SK_CL_SEVERERAINSTORM:
		case SK_CL_SEVERERAINSTORM_MELEE:
			skillratio += BardSkillAttackRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
			break;
		case SK_HT_MAGICTOMAHAWK:
		case SK_RA_ZEPHYRSNIPING:
		case SK_HT_LIVINGTORNADO:
		case SK_HT_SLASH:
		case SK_HT_BLITZBEAT:
		case SK_ST_SHARPSHOOTING:
			skillratio += HunterSkillAttackRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
			break;
		case SK_AM_BASILISK1:
		case SK_AM_BASILISK2:
		case SK_CR_BASILISK3:
		case SK_AM_ACIDTERROR:
		case SK_AM_BOMB:
		case SK_CR_ACIDBOMB:
			skillratio += AlchemistSkillAttackRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus, tstatus);
			break;
		case SK_MO_OCCULTIMPACT:
		case SK_MO_TRHOWSPIRITSPHERE:
		case SK_MO_KIBLAST:
		case SK_MO_CIRCULARFISTS:
		case SK_MO_PALMSTRIKE:
		case SK_MO_FALINGFIST:
		case SK_SH_GUILLOTINEFISTS:
		case SK_MO_TIGERFIST:
		case SK_SH_GATEOFHELL:
		case SK_SH_DRAGONDARKNESSFLAME:
		case SK_MO_TRIPLEARMCANNON:
		case SK_SH_ASURASTRIKE:
			{
				bool revealed_hidden_enemy = false;
				if (tsc && ((tsc->option&(OPTION_HIDE|OPTION_CLOAK|OPTION_CHASEWALK|OPTION_INVISIBLE)) || tsc->data[STATUS_CAMOUFLAGE])) {
					revealed_hidden_enemy = true;
				}
				skillratio += MonkSkillAttackRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus, revealed_hidden_enemy, sd, sc);
			}
			break;
	}
	return skillratio;
}

/*==================================================================================================
 * Constant skill damage additions are added before SC modifiers and after skill base ATK calculation
 *--------------------------------------------------------------------------------------------------*
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static int64 battle_calc_skill_constant_addition(struct Damage* wd, struct block_list *src,struct block_list *target,uint16 skill_id,uint16 skill_lv)
{
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	struct map_session_data *tsd = BL_CAST(BL_PC, target);
	struct status_data *sstatus = status_get_status_data(src);
	struct status_data *tstatus = status_get_status_data(target);
	int64 atk = 0;
	return atk;
}

/*==============================================================
 * Stackable SC bonuses added on top of calculated skill damage
 *--------------------------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static void battle_attack_sc_bonus(struct Damage* wd, struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv)
{
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	struct status_change *sc = status_get_sc(src);
	struct status_data *sstatus = status_get_status_data(src);
	struct status_data *tstatus = status_get_status_data(target);
	uint8 anger_id = 0; // SLS Anger

	// Kagerou/Oboro Earth Charm effect +15% wATK
	if(sd && sd->spiritcharm_type == CHARM_TYPE_LAND && sd->spiritcharm > 0) {
		ATK_ADDRATE(wd->damage, wd->damage2, 15 * sd->spiritcharm);
		ATK_ADDRATE(wd->weaponAtk, wd->weaponAtk2, 15 * sd->spiritcharm);
	}

	//The following are applied on top of current damage and are stackable.
	if (sc) {

		if (sc->data[STATUS_WATK_ELEMENT] && skill_id != SK_EX_METEORASSAULT)
			ATK_ADDRATE(wd->weaponAtk, wd->weaponAtk2, sc->data[STATUS_WATK_ELEMENT]->val2);
		
		
	
		if (sc->data[STATUS_ENCHANTDEADLYPOISON]) {
			switch(skill_id) {
				default: // fall through to apply EDP bonuses
					ATK_RATE(wd->weaponAtk, wd->weaponAtk2, sc->data[STATUS_ENCHANTDEADLYPOISON]->val1 * 100);
					// ATK_RATE(wd->equipAtk, wd->equipAtk2, 100 + (sc->data[STATUS_ENCHANTDEADLYPOISON]->val1 * 60));
					break;
			}
		}
		
		
		
		
		// if(sc->data[STATUS_UNLIMIT] && (wd->flag&(BF_LONG|BF_MAGIC)) == BF_LONG) {
		// 	switch(skill_id) {
		// 		case RA_WUGDASH:
		// 		case RA_WUGBITE:
		// 			break;
		// 		default:
		// 			ATK_ADDRATE(wd->damage, wd->damage2, sc->data[STATUS_UNLIMIT]->val2);
		// 			RE_ALLATK_ADDRATE(wd, sc->data[STATUS_UNLIMIT]->val2);
		// 			break;
		// 	}
		// }
		
		

		
	}

	if (sd != nullptr && !anger_id)
		ARR_FIND(0, MAX_PC_FEELHATE, anger_id, status_get_class(target) == sd->hate_mob[anger_id]);

	uint16 anger_level;
	if (sd != nullptr && anger_id < MAX_PC_FEELHATE && (anger_level = pc_checkskill(sd, sg_info[anger_id].anger_id))) {
		int skillratio = sd->status.base_level + sstatus->dex + sstatus->luk;

		if (anger_id == 2)
			skillratio += sstatus->str; // SG_STAR_ANGER additionally has STR added in its formula.
		if (anger_level < 4)
			skillratio /= 12 - 3 * anger_level;
		ATK_ADDRATE(wd->damage, wd->damage2, skillratio);
#ifdef RENEWAL
		RE_ALLATK_ADDRATE(wd, skillratio);
#endif
	}
}

/*====================================
 * Calc defense damage reduction
 *------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static void battle_calc_defense_reduction(struct Damage* wd, struct block_list *src,struct block_list *target, uint16 skill_id, uint16 skill_lv)
{
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	struct map_session_data *tsd = BL_CAST(BL_PC, target);
	struct status_change *sc = status_get_sc(src);
	struct status_change *tsc = status_get_sc(target);
	struct status_data *sstatus = status_get_status_data(src);
	struct status_data *tstatus = status_get_status_data(target);

	//Defense reduction
	short vit_def;
	defType def1 = status_get_def(target); //Don't use tstatus->def1 due to skill timer reductions.
	short def2 = tstatus->def2;

	if (sd) {
		int i = sd->indexed_bonus.ignore_def_by_race[tstatus->race] + sd->indexed_bonus.ignore_def_by_race[RC_ALL];
		i += sd->indexed_bonus.ignore_def_by_class[tstatus->class_] + sd->indexed_bonus.ignore_def_by_class[CLASS_ALL];
		if (i) {
			i = min(i,100); //cap it to 100 for 0 def min
			def1 -= def1 * i / 100;
			def2 -= def2 * i / 100;
		}

		//Kagerou/Oboro Earth Charm effect +10% eDEF
		if(sd->spiritcharm_type == CHARM_TYPE_LAND && sd->spiritcharm > 0) {
			short si = 10 * sd->spiritcharm;
			def1 = (def1 * (100 + si)) / 100;
		}
	}

	if (sc && sc->data[STATUS_EXPIATIO]) {
		short i = 6 * sc->data[STATUS_EXPIATIO]->val1; // 6% per level

		i = min(i,100); //cap it to 100 for 0 def min
		def1 = (def1*(100-i))/100;
		def2 = (def2*(100-i))/100;
	}

	if (tsc) {
		
		if( tsc->data[STATUS_CAMOUFLAGE] ){
			short i = 5 * tsc->data[STATUS_CAMOUFLAGE]->val3; //5% per second

			i = min(i,100); //cap it to 100 for 0 def min
			def1 = (def1*(100-i))/100;
			def2 = (def2*(100-i))/100;
		}

	}

	if( battle_config.vit_penalty_type && battle_config.vit_penalty_target&target->type ) {
		unsigned char target_count; //256 max targets should be a sane max

		//Official servers limit the count to 22 targets
		target_count = min(unit_counttargeted(target), (100 / battle_config.vit_penalty_num) + (battle_config.vit_penalty_count - 1));
		if(target_count >= battle_config.vit_penalty_count) {
			if(battle_config.vit_penalty_type == 1) {
				if( !tsc || !tsc->data[STATUS_MENTALSTRENGTH] )
					def1 = (def1 * (100 - (target_count - (battle_config.vit_penalty_count - 1))*battle_config.vit_penalty_num))/100;
				def2 = (def2 * (100 - (target_count - (battle_config.vit_penalty_count - 1))*battle_config.vit_penalty_num))/100;
			} else { //Assume type 2
				if( !tsc || !tsc->data[STATUS_MENTALSTRENGTH] )
					def1 -= (target_count - (battle_config.vit_penalty_count - 1))*battle_config.vit_penalty_num;
				def2 -= (target_count - (battle_config.vit_penalty_count - 1))*battle_config.vit_penalty_num;
			}
		}
		// if (skill_id == SK_AM_ACIDTERROR)
		// 	def2 = 0; //Ignore only status defense. [FatalEror]
		// 	def1 = 0; //Ignores only armor defense. [Skotlex]

		if(def2 < 1)
			def2 = 1;
	}

	//Vitality reduction from rodatazone: http://rodatazone.simgaming.net/mechanics/substats.php#def
	if (tsd) {	//Sd vit-eq
		int skill;
#ifndef RENEWAL
		//[VIT*0.5] + rnd([VIT*0.3], max([VIT*0.3],[VIT^2/150]-1))
		vit_def = def2*(def2-15)/150;
		vit_def = def2/2 + (vit_def>0?rnd()%vit_def:0);
#else
		vit_def = def2;
#endif
		

		
	} else { //Mob-Pet vit-eq
#ifndef RENEWAL
		//VIT + rnd(0,[VIT/20]^2-1)
		vit_def = (def2/20)*(def2/20);
		if (tsc && tsc->data[SC_SKA])
			vit_def += 100; //Eska increases the random part of the formula by 100
		vit_def = def2 + (vit_def>0?rnd()%vit_def:0);
#else
		//SoftDEF of monsters is floor((BaseLevel+Vit)/2)
		vit_def = def2;
#endif
	}

	if (battle_config.weapon_defense_type) {
		vit_def += def1*battle_config.weapon_defense_type;
		def1 = 0;
	}

#ifdef RENEWAL
	/**
	 * RE DEF Reduction
	 * Damage = Attack * (4000+eDEF)/(4000+eDEF*10) - sDEF
	 * Pierce defence gains 1 atk per def/2
	 */
	if( def1 == -400 ) /* -400 creates a division by 0 and subsequently crashes */
		def1 = -399;
	ATK_ADD2(wd->damage, wd->damage2,
		is_attack_piercing(wd, src, target, skill_id, skill_lv, EQI_HAND_R) ? (def1*battle_calc_attack_skill_ratio(wd, src, target, skill_id, skill_lv))/200 : 0,
		is_attack_piercing(wd, src, target, skill_id, skill_lv, EQI_HAND_L) ? (def1*battle_calc_attack_skill_ratio(wd, src, target, skill_id, skill_lv))/200 : 0
	);
	if( !attack_ignores_def(wd, src, target, skill_id, skill_lv, EQI_HAND_R) && !is_attack_piercing(wd, src, target, skill_id, skill_lv, EQI_HAND_R) )
		wd->damage = wd->damage * (4000+def1) / (4000+10*def1) - vit_def;
	if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id) && !attack_ignores_def(wd, src, target, skill_id, skill_lv, EQI_HAND_L) && !is_attack_piercing(wd, src, target, skill_id, skill_lv, EQI_HAND_L))
		wd->damage2 = wd->damage2 * (4000+def1) / (4000+10*def1) - vit_def;

#else
		if (def1 > 100) def1 = 100;
		ATK_RATE2(wd->damage, wd->damage2,
			attack_ignores_def(wd, src, target, skill_id, skill_lv, EQI_HAND_R) ?100:(is_attack_piercing(wd, src, target, skill_id, skill_lv, EQI_HAND_R) ? (int64)is_attack_piercing(wd, src, target, skill_id, skill_lv, EQI_HAND_R)*(def1+vit_def) : (100-def1)),
			attack_ignores_def(wd, src, target, skill_id, skill_lv, EQI_HAND_L) ?100:(is_attack_piercing(wd, src, target, skill_id, skill_lv, EQI_HAND_L) ? (int64)is_attack_piercing(wd, src, target, skill_id, skill_lv, EQI_HAND_L)*(def1+vit_def) : (100-def1))
		);
		ATK_ADD2(wd->damage, wd->damage2,
			attack_ignores_def(wd, src, target, skill_id, skill_lv, EQI_HAND_R) || is_attack_piercing(wd, src, target, skill_id, skill_lv, EQI_HAND_R) ?0:-vit_def,
			attack_ignores_def(wd, src, target, skill_id, skill_lv, EQI_HAND_L) || is_attack_piercing(wd, src, target, skill_id, skill_lv, EQI_HAND_L) ?0:-vit_def
		);
#endif
}

/*====================================
 * Modifiers ignoring DEF
 *------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static void battle_calc_attack_post_defense(struct Damage* wd, struct block_list *src,struct block_list *target,uint16 skill_id,uint16 skill_lv)
{
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	struct status_change *sc = status_get_sc(src);
	struct status_data *sstatus = status_get_status_data(src);

	// Post skill/vit reduction damage increases
	if( sc ) { // SC skill damages
		if(sc->data[STATUS_AURABLADE]) {
			ATK_ADD(wd->damage, wd->damage2, sc->data[STATUS_AURABLADE]->val1  * (status_get_lv(src)/2) );
		}
		if(sc->data[STATUS_AURABLADE]) {
			ATK_ADD(wd->damage, wd->damage2, sc->data[STATUS_AURABLADE]->val1  * (status_get_lv(src)/2) );
		}

		if (sc->data[STATUS_ENCHANTBLADE]) {
			int64 enchant_dmg = sstatus->matk_min;
			if (sstatus->matk_max > sstatus->matk_min) {
				enchant_dmg = rand()%(sstatus->matk_max-sstatus->matk_min + 1) + sstatus->matk_min;
			}
			if (enchant_dmg > 0)
				ATK_ADD(wd->damage, wd->damage2, enchant_dmg);
		}
	}

	//Set to min of 1
	if (is_attack_right_handed(src, skill_id) && wd->damage < 1) wd->damage = 1;
	if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id) && wd->damage2 < 1) wd->damage2 = 1;
}

/*=================================================================================
 * "Plant"-type (mobs that only take 1 damage from all sources) damage calculation
 *---------------------------------------------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static void battle_calc_attack_plant(struct Damage* wd, struct block_list *src,struct block_list *target, uint16 skill_id, uint16 skill_lv)
{
	struct status_data *tstatus = status_get_status_data(target);
	bool attack_hits = is_attack_hitting(wd, src, target, skill_id, skill_lv, false);

	if (skill_id != SK_ST_SHARPSHOOTING && skill_id != SK_RG_ARROWSTORM)
		status_change_end(src, STATUS_CAMOUFLAGE, INVALID_TIMER);

	//Plants receive 1 damage when hit
	if( attack_hits || wd->damage > 0 )
		wd->damage = 1; //In some cases, right hand no need to have a weapon to deal a damage
	if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id) && (attack_hits || wd->damage2 > 0)) {
		struct map_session_data *sd = BL_CAST(BL_PC, src);

		if (sd && sd->status.weapon == W_KATAR)
			wd->damage2 = 0; //No backhand damage against plants
		else
			wd->damage2 = 1; //Deal 1 HP damage as long as there is a weapon in the left hand
	}

	if (attack_hits && target->type == BL_MOB) {
		struct status_change *sc = status_get_sc(target);
		int64 damage_dummy = 1;

		if (sc && !battle_status_block_damage(src, target, sc, wd, damage_dummy, skill_id, skill_lv)) { // Statuses that reduce damage to 0.
			wd->damage = wd->damage2 = 0;
			return;
		}
	}

	if( attack_hits && status_get_class(target) == MOBID_EMPERIUM ) {
		if(target && !battle_can_hit_gvg_target(src,target,skill_id,(skill_id)?BF_SKILL:0) && map_flag_gvg2(target->m)) {
			wd->damage = wd->damage2 = 0;
			return;
		}

		const int right_element = EquipmentAttackCalculator::battle_get_weapon_element(wd, src, target, skill_id, skill_lv, EQI_HAND_R, false);
		const int left_element = EquipmentAttackCalculator::battle_get_weapon_element(wd, src, target, skill_id, skill_lv, EQI_HAND_L, false);

		if (wd->damage > 0) {
			wd->damage = battle_attr_fix(src, target, wd->damage, right_element, tstatus->def_ele, tstatus->ele_lv);
			wd->damage = battle_calc_gvg_damage(src, target, wd->damage, skill_id, wd->flag);
		} else if (wd->damage2 > 0) {
			wd->damage2 = battle_attr_fix(src, target, wd->damage2, left_element, tstatus->def_ele, tstatus->ele_lv);
			wd->damage2 = battle_calc_gvg_damage(src, target, wd->damage2, skill_id, wd->flag);
		}
		return;
	}

	//For plants we don't continue with the weapon attack code, so we have to apply DAMAGE_DIV_FIX here
	battle_apply_div_fix(wd, skill_id);

	//If there is left hand damage, total damage can never exceed 2, even on multiple hits
	if(wd->damage > 1 && wd->damage2 > 0) {
		wd->damage = 1;
		wd->damage2 = 1;
	}
}

/*========================================================================================
 * Perform left/right hand weapon damage calculation based on previously calculated damage
 *----------------------------------------------------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static void battle_calc_attack_left_right_hands(struct Damage* wd, struct block_list *src,struct block_list *target,uint16 skill_id,uint16 skill_lv)
{
	struct map_session_data *sd = BL_CAST(BL_PC, src);

	if (sd) {
		int skill;

		if (!is_attack_right_handed(src, skill_id) && EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) {
			wd->damage = wd->damage2;
			wd->damage2 = 0;
		} else if(sd->status.weapon == W_KATAR && !skill_id) { //Katars (offhand damage only applies to normal attacks, tested on Aegis 10.2)
			skill = pc_checkskill(sd,SK_AS_DOUBLEATTACK);
			wd->damage2 = (int64)wd->damage * (1 + (skill * 4))/100;
		} else if (is_attack_right_handed(src, skill_id) && EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) {	//Dual-wield
			if (wd->damage) {
				if( (sd->class_&MAPID_BASEMASK) == MAPID_THIEF ) {
					skill = pc_checkskill(sd,SK_AS_DUALWIELDING);
					ATK_RATER(wd->damage, 50 + (skill * 10))
				}
				if(wd->damage < 1)
					wd->damage = 1;
			}
			if (wd->damage2) {
				if( (sd->class_&MAPID_BASEMASK) == MAPID_THIEF) {
					skill = pc_checkskill(sd,SK_AS_DUALWIELDING);
					ATK_RATEL(wd->damage2, 50 + (skill * 10))
				}
				if(wd->damage2 < 1)
					wd->damage2 = 1;
			}
		}
	}

	if (!is_attack_right_handed(src, skill_id) && !EquipmentAttackCalculator::is_attack_left_handed(src, skill_id) && wd->damage)
		wd->damage=0;

	if (!EquipmentAttackCalculator::is_attack_left_handed(src, skill_id) && wd->damage2)
		wd->damage2=0;
}

/**
* Check if bl is devoted by someone
* @param bl
* @return 'd_bl' if devoted or NULL if not devoted
*/
struct block_list *battle_check_devotion(struct block_list *bl) {
	struct block_list *d_bl = NULL;

	if (battle_config.devotion_rdamage && battle_config.devotion_rdamage > rnd() % 100) {
		struct status_change *sc = status_get_sc(bl);
		if (sc && sc->data[STATUS_SWORNPROTECTOR])
			d_bl = map_id2bl(sc->data[STATUS_SWORNPROTECTOR]->val1);
	}
	return d_bl;
}

/*==========================================
 * BG/GvG attack modifiers
 *------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static void battle_calc_attack_gvg_bg(struct Damage* wd, struct block_list *src,struct block_list *target,uint16 skill_id,uint16 skill_lv)
{
	if( wd->damage + wd->damage2 ) { //There is a total damage value
		if( src != target && //Don't reflect your own damage (Grand Cross)
			(!skill_id || skill_id) ) {
				int64 damage = wd->damage + wd->damage2, rdamage = 0;
				struct map_session_data *tsd = BL_CAST(BL_PC, target);
				struct status_data *sstatus = status_get_status_data(src);
				t_tick tick = gettick(), rdelay = 0;

				rdamage = battle_calc_return_damage(target, src, &damage, wd->flag, skill_id, false);
				if( rdamage > 0 ) { //Item reflect gets calculated before any mapflag reducing is applicated
					struct block_list *d_bl = battle_check_devotion(src);
					 
					rdelay = clif_damage(src, (!d_bl) ? src : d_bl, tick, wd->amotion, sstatus->dmotion, rdamage, 1, DMG_ENDURE, 0, false);
					if( tsd )
						battle_drain(tsd, src, rdamage, rdamage, sstatus->race, sstatus->class_);
					//Use Reflect Shield to signal this kind of skill trigger [Skotlex]
					battle_delay_damage(tick, wd->amotion, target, (!d_bl) ? src : d_bl, 0, SK_CR_REFLECTSHIELD, 0, rdamage, ATK_DEF, rdelay, true, false);
					SkillAdditionalEffects::skill_additional_effect(target, (!d_bl) ? src : d_bl, SK_CR_REFLECTSHIELD, 1, BF_WEAPON|BF_SHORT|BF_NORMAL, ATK_DEF, tick);
				}
		}

		struct map_data *mapdata = map_getmapdata(target->m);

		if(!wd->damage2) {
			wd->damage = battle_calc_damage(src,target,wd,wd->damage,skill_id,skill_lv);
			if( mapdata_flag_gvg2(mapdata) )
				wd->damage=battle_calc_gvg_damage(src,target,wd->damage,skill_id,wd->flag);
			else if( mapdata->flag[MF_BATTLEGROUND] )
				wd->damage=battle_calc_bg_damage(src,target,wd->damage,skill_id,wd->flag);
		}
		else if(!wd->damage) {
			wd->damage2 = battle_calc_damage(src,target,wd,wd->damage2,skill_id,skill_lv);
			if( mapdata_flag_gvg2(mapdata) )
				wd->damage2 = battle_calc_gvg_damage(src,target,wd->damage2,skill_id,wd->flag);
			else if( mapdata->flag[MF_BATTLEGROUND] )
				wd->damage2 = battle_calc_bg_damage(src,target,wd->damage2,skill_id,wd->flag);
		}
		else {
			int64 d1 = wd->damage + wd->damage2,d2 = wd->damage2;
			wd->damage = battle_calc_damage(src,target,wd,d1,skill_id,skill_lv);
			if( mapdata_flag_gvg2(mapdata) )
				wd->damage = battle_calc_gvg_damage(src,target,wd->damage,skill_id,wd->flag);
			else if( mapdata->flag[MF_BATTLEGROUND] )
				wd->damage = battle_calc_bg_damage(src,target,wd->damage,skill_id,wd->flag);
			wd->damage2 = (int64)d2*100/d1 * wd->damage/100;
			if(wd->damage > 1 && wd->damage2 < 1) wd->damage2 = 1;
			wd->damage-=wd->damage2;
		}
	}
}

/*==========================================
 * final ATK modifiers - after BG/GvG calc
 *------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static void battle_calc_weapon_final_atk_modifiers(struct Damage* wd, struct block_list *src,struct block_list *target,uint16 skill_id,uint16 skill_lv)
{
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	struct map_session_data *tsd = BL_CAST(BL_PC, target);
	struct status_change *sc = status_get_sc(src);
	struct status_change *tsc = status_get_sc(target);
	struct status_data *sstatus = status_get_status_data(src);
	struct status_data *tstatus = status_get_status_data(target);
	int skill_damage = 0;

	//Reject Sword bugreport:4493 by Daegaladh
	// if(wd->damage && tsc && tsc->data[STATUS_KILLERINSTINCT] &&
	// 	(src->type!=BL_PC || (
	// 		((TBL_PC *)src)->weapontype1 == W_DAGGER ||
	// 		((TBL_PC *)src)->weapontype1 == W_1HSWORD ||
	// 		((TBL_PC *)src)->status.weapon == W_2HSWORD
	// 	)) &&
	// 	rnd()%100 < tsc->data[STATUS_KILLERINSTINCT]->val2
	// 	)
	// {
	// 	ATK_RATER(wd->damage, 50)
		// status_fix_damage(target,src,wd->damage,clif_damage(target,src,gettick(),0,0,wd->damage,0,DMG_NORMAL,0,false),SK_ST_KILLERINSTIINCT);
		// clif_skill_nodamage(target,target,SK_ST_KILLERINSTIINCT,tsc->data[STATUS_KILLERINSTINCT]->val1,1);
		// if( --(tsc->data[STATUS_KILLERINSTINCT]->val3) <= 0 )
		// 	status_change_end(target, STATUS_KILLERINSTINCT, INVALID_TIMER);
	// }

	

	if( sc ) {
		
		// Only affecting non-skills
		// if (!skill_id && wd->dmg_lv > ATK_BLOCK) {
			
		// }
		if (skill_id != SK_ST_SHARPSHOOTING && skill_id != SK_RG_ARROWSTORM)
			status_change_end(src, STATUS_CAMOUFLAGE, INVALID_TIMER);
	}

	// Skill damage adjustment
	if ((skill_damage = battle_skill_damage(src, target, skill_id)) != 0)
		ATK_ADDRATE(wd->damage, wd->damage2, skill_damage);
}

/*====================================================
 * Basic wd init - not influenced by HIT/MISS/DEF/etc.
 *----------------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static struct Damage initialize_weapon_data(struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv, int wflag)
{
	struct status_data *sstatus = status_get_status_data(src);
	struct status_data *tstatus = status_get_status_data(target);
	struct status_change *sc = status_get_sc(src);
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	struct Damage wd;

	wd.type = DMG_NORMAL; //Normal attack
	wd.div_ = skill_id?skill_get_num(skill_id,skill_lv):1;
	wd.amotion = (skill_id && skill_get_inf(skill_id)&INF_GROUND_SKILL)?0:sstatus->amotion; //Amotion should be 0 for ground skills.
	// counter attack DOES obey ASPD delay on official, uncomment if you want the old (bad) behavior [helvetica]
	/*if(skill_id == SK_KN_COUNTERATTACK)
		wd.amotion >>= 1; */
	wd.dmotion = tstatus->dmotion;
	wd.blewcount =skill_get_blewcount(skill_id,skill_lv);
	wd.miscflag = wflag;
	wd.flag = BF_WEAPON; //Initial Flag
	wd.flag |= (skill_id||wd.miscflag)?BF_SKILL:BF_NORMAL; // Baphomet card's splash damage is counted as a skill. [Inkfish]
	wd.isspdamage = false;
	wd.damage = wd.damage2 =
#ifdef RENEWAL
	wd.statusAtk = wd.statusAtk2 = wd.equipAtk = wd.equipAtk2 = wd.weaponAtk = wd.weaponAtk2 = wd.masteryAtk = wd.masteryAtk2 =
#endif
	0;

	wd.dmg_lv=ATK_DEF;	//This assumption simplifies the assignation later

	if(sd)
		wd.blewcount += battle_blewcount_bonus(sd, skill_id);

	if (skill_id) {
		wd.flag |= battle_range_type(src, target, skill_id, skill_lv);
		switch(skill_id)
		{
			case SK_MO_TRHOWSPIRITSPHERE:
				if (sd) {
					if (battle_config.finger_offensive_type)
						wd.div_ = 1;
					else if ((sd->spiritball + sd->spiritball_old) < wd.div_)
						wd.div_ = sd->spiritball + sd->spiritball_old;
				}
				break;

			case SK_KN_PIERCE:
				wd.div_= (wd.div_>0?tstatus->size+1:-(tstatus->size+1));
				break;

			case SK_AS_DOUBLEATTACK: //For NPC used skill.
				wd.type = DMG_MULTI_HIT;
				break;

				//Fall through
			case SK_SM_SPEARSTAB:
				wd.blewcount = 0;
				break;
			// case SK_KN_BOWLINGBASH:
			// 	if (sd && sd->status.weapon == W_2HSWORD) {
			// 		if (wd.miscflag >= 2 && wd.miscflag <= 3)
			// 			wd.div_ = 3;
			// 		else if (wd.miscflag >= 4)
			// 			wd.div_ = 4;
			// 	}
			// 	break;
			case SK_KN_COUNTERATTACK:
				wd.flag = (wd.flag&~BF_SKILLMASK)|BF_NORMAL;
				break;
			// The number of hits is set to 3 by default for use in Inspiration status.
			// When in Banding, the number of hits is equal to the number of Royal Guards in Banding.
		}
	} else {
		wd.flag |= is_skill_using_arrow(src, skill_id)?BF_LONG:BF_SHORT;
	}

	return wd;
}

/**
 * Check if we should reflect the damage and calculate it if so
 * @param attack_type : BL_WEAPON,BL_MAGIC or BL_MISC
 * @param wd : weapon damage
 * @param src : bl who did the attack
 * @param target : target of the attack
 * @param skill_id : id of casted skill, 0 = basic atk
 * @param skill_lv : lvl of skill casted
 */
void battle_do_reflect(int attack_type, struct Damage *wd, struct block_list* src, struct block_list* target, uint16 skill_id, uint16 skill_lv)
{
	// Don't reflect your own damage (Grand Cross)
	if ((wd->damage + wd->damage2) && src && target && src != target && (src->type != BL_SKILL))
	{
		int64 damage = wd->damage + wd->damage2, rdamage = 0;
		struct map_session_data *tsd = BL_CAST(BL_PC, target);
		struct status_change *tsc = status_get_sc(target);
		struct status_data *sstatus = status_get_status_data(src);
		struct unit_data *ud = unit_bl2ud(target);
		t_tick tick = gettick(), rdelay = 0;

		if (!tsc)
			return;

		// Calculate skill reflect damage separately
		if ((ud && !ud->immune_attack) || !status_bl_has_mode(target, MD_SKILLIMMUNE))
			rdamage = battle_calc_return_damage(target, src, &damage, wd->flag, skill_id,true);
		if( rdamage > 0 ) {
			struct block_list *d_bl = battle_check_devotion(src);
			status_change *sc = status_get_sc(src);

			if( attack_type == BF_WEAPON || attack_type == BF_MISC) {
				rdelay = clif_damage(src, (!d_bl) ? src : d_bl, tick, wd->amotion, sstatus->dmotion, rdamage, 1, DMG_ENDURE, 0, false);
				if( tsd )
					battle_drain(tsd, src, rdamage, rdamage, sstatus->race, sstatus->class_);
				// It appears that official servers give skill reflect damage a longer delay
				battle_delay_damage(tick, wd->amotion, target, (!d_bl) ? src : d_bl, 0, SK_CR_REFLECTSHIELD, 0, rdamage, ATK_DEF, rdelay ,true, false);
				SkillAdditionalEffects::skill_additional_effect(target, (!d_bl) ? src : d_bl, SK_CR_REFLECTSHIELD, 1, BF_WEAPON|BF_SHORT|BF_NORMAL, ATK_DEF, tick);
			}
		}
	}
}

/*============================================
 * Calculate "weapon"-type attacks and skills
 *--------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
static struct Damage battle_calc_weapon_attack(struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv, int wflag)
{
	struct map_session_data *sd, *tsd;
	struct Damage wd;
	struct status_change *sc = status_get_sc(src);
	struct status_change *tsc = status_get_sc(target);
	struct status_data *tstatus = status_get_status_data(target);
	int right_element, left_element;
	bool infdef = false;

	memset(&wd,0,sizeof(wd));

	if (src == NULL || target == NULL) {
		nullpo_info(NLP_MARK);
		return wd;
	}

	wd = initialize_weapon_data(src, target, skill_id, skill_lv, wflag);

	right_element = EquipmentAttackCalculator::battle_get_weapon_element(&wd, src, target, skill_id, skill_lv, EQI_HAND_R, false);
	left_element = EquipmentAttackCalculator::battle_get_weapon_element(&wd, src, target, skill_id, skill_lv, EQI_HAND_L, false);

	if (sc && !sc->count)
		sc = NULL; //Skip checking as there are no status changes active.
	if (tsc && !tsc->count)
		tsc = NULL; //Skip checking as there are no status changes active.

	sd = BL_CAST(BL_PC, src);
	tsd = BL_CAST(BL_PC, target);

	//Check for Lucky Dodge
	if ((!skill_id || skill_id == SK_KN_RECKONING) && tstatus->flee2 && rnd()%1000 < tstatus->flee2) {
		wd.type = DMG_LUCY_DODGE;
		wd.dmg_lv = ATK_LUCKY;
		if(wd.div_ < 0)
			wd.div_ *= -1;
		return wd;
	}

	// on official check for multi hit first so we can override crit on double attack [helvetica]
	battle_calc_multi_attack(&wd, src, target, skill_id, skill_lv);

	// crit check is next since crits always hit on official [helvetica]
	if (CriticalHitCalculator::is_attack_critical(&wd, src, target, skill_id, skill_lv, true)) {
		if (wd.type&DMG_MULTI_HIT)
			wd.type = DMG_MULTI_HIT_CRITICAL;
		else
			wd.type = DMG_CRITICAL;
	}

	std::bitset<NK_MAX> nk = battle_skill_get_damage_properties(skill_id, wd.miscflag);

	// check if we're landing a hit
	if(!is_attack_hitting(&wd, src, target, skill_id, skill_lv, true))
		wd.dmg_lv = ATK_FLEE;
	else if(!(infdef = is_infinite_defense(target, wd.flag))) { //no need for math against plants
		int64 ratio = 0;
		int i = 0;
		SkillBaseDamageCalculator::battle_calc_skill_base_damage(&wd, src, target, skill_id, skill_lv); // base skill damage
		ratio = battle_calc_attack_skill_ratio(&wd, src, target, skill_id, skill_lv); // skill level ratios

		ATK_RATE(wd.damage, wd.damage2, ratio);
		RE_ALLATK_RATE(&wd, ratio);

		int64 bonus_damage = battle_calc_skill_constant_addition(&wd, src, target, skill_id, skill_lv); // other skill bonuses

		ATK_ADD(wd.damage, wd.damage2, bonus_damage);

		if(skill_id == SK_WZ_MAGICCRASHER) { // Add weapon attack for MATK onto Magic Crasher
			struct status_data *sstatus = status_get_status_data(src);

			if (sstatus->matk_max > sstatus->matk_min) {
				ATK_ADD(wd.weaponAtk, wd.weaponAtk2, sstatus->matk_min+rnd()%(sstatus->matk_max-sstatus->matk_min));
			} else
				ATK_ADD(wd.weaponAtk, wd.weaponAtk2, sstatus->matk_min);
		}

		// add any miscellaneous player ATK bonuses
		if( sd && skill_id && (i = pc_skillatk_bonus(sd, skill_id))) {
			ATK_ADDRATE(wd.damage, wd.damage2, i);
			RE_ALLATK_ADDRATE(&wd, i);
		}
		if (tsd && (i = pc_sub_skillatk_bonus(tsd, skill_id))) {
			ATK_ADDRATE(wd.damage, wd.damage2, -i);
			RE_ALLATK_ADDRATE(&wd, -i);
		}
		// In Renewal we only cardfix to the weapon and equip ATK
		//Card Fix for attacker (sd), 2 is added to the "left" flag meaning "attacker cards only"
		if (sd) {
			wd.weaponAtk += battle_calc_cardfix(BF_WEAPON, src, target, nk, right_element, left_element, wd.weaponAtk, 2, wd.flag);
			wd.equipAtk += battle_calc_cardfix(BF_WEAPON, src, target, nk, right_element, left_element, wd.equipAtk, 2, wd.flag);
			if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) {
				wd.weaponAtk2 += battle_calc_cardfix(BF_WEAPON, src, target, nk, right_element, left_element, wd.weaponAtk2, 3, wd.flag);
				wd.equipAtk2 += battle_calc_cardfix(BF_WEAPON, src, target, nk, right_element, left_element, wd.equipAtk2, 3, wd.flag);
			}

			//Card Fix for target (tsd), 2 is not added to the "left" flag meaning "target cards only"
			if (tsd) {
				std::bitset<NK_MAX> ignoreele_nk = nk;

				ignoreele_nk.set(NK_IGNOREELEMENT);
				wd.statusAtk += battle_calc_cardfix(BF_WEAPON, src, target, ignoreele_nk, right_element, left_element, wd.statusAtk, 0, wd.flag);
				wd.weaponAtk += battle_calc_cardfix(BF_WEAPON, src, target, nk, right_element, left_element, wd.weaponAtk, 0, wd.flag);
				wd.equipAtk += battle_calc_cardfix(BF_WEAPON, src, target, nk, right_element, left_element, wd.equipAtk, 0, wd.flag);
				wd.masteryAtk += battle_calc_cardfix(BF_WEAPON, src, target, ignoreele_nk, right_element, left_element, wd.masteryAtk, 0, wd.flag);
				if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) {
					wd.statusAtk2 += battle_calc_cardfix(BF_WEAPON, src, target, ignoreele_nk, right_element, left_element, wd.statusAtk2, 1, wd.flag);
					wd.weaponAtk2 += battle_calc_cardfix(BF_WEAPON, src, target, nk, right_element, left_element, wd.weaponAtk2, 1, wd.flag);
					wd.equipAtk2 += battle_calc_cardfix(BF_WEAPON, src, target, nk, right_element, left_element, wd.equipAtk2, 1, wd.flag);
					wd.masteryAtk2 += battle_calc_cardfix(BF_WEAPON, src, target, ignoreele_nk, right_element, left_element, wd.masteryAtk2, 1, wd.flag);
				}
			}
		}

		// final attack bonuses that aren't affected by cards
		battle_attack_sc_bonus(&wd, src, target, skill_id, skill_lv);

		if (sd) { //monsters, homuns and pets have their damage computed directly
			wd.damage = wd.statusAtk + wd.weaponAtk + wd.equipAtk + wd.masteryAtk + bonus_damage;
			wd.damage2 = wd.statusAtk2 + wd.weaponAtk2 + wd.equipAtk2 + wd.masteryAtk2 + bonus_damage;
			if (wd.flag & BF_SHORT)
				ATK_ADDRATE(wd.damage, wd.damage2, sd->bonus.short_attack_atk_rate);
			if(wd.flag&BF_LONG && (skill_id != SK_WG_SLASH)) //Long damage rate addition doesn't use weapon + equip attack
				ATK_ADDRATE(wd.damage, wd.damage2, sd->bonus.long_attack_atk_rate);
		}

		if (wd.damage + wd.damage2) { //Check if attack ignores DEF
			if(!attack_ignores_def(&wd, src, target, skill_id, skill_lv, EQI_HAND_L) || !attack_ignores_def(&wd, src, target, skill_id, skill_lv, EQI_HAND_R))
				battle_calc_defense_reduction(&wd, src, target, skill_id, skill_lv);

			battle_calc_attack_post_defense(&wd, src, target, skill_id, skill_lv);
		}
	}

	battle_calc_element_damage(&wd, src, target, skill_id, skill_lv);


	if (CriticalHitCalculator::is_attack_critical(&wd, src, target, skill_id, skill_lv, false)) {
		if (sd) { //Check for player so we don't crash out, monsters don't have bonus crit rates [helvetica]
			wd.damage = (int)floor((float)((wd.damage * 140) / 100 * (100 + sd->bonus.crit_atk_rate)) / 100);
			if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id))
				wd.damage2 = (int)floor((float)((wd.damage2 * 140) / 100 * (100 + sd->bonus.crit_atk_rate)) / 100);
		} else {
			wd.damage = (int)floor((float)(wd.damage * 140) / 100);
		}
	}


	if(sd) {

		// if( skill_id == SK_CR_SMITE || skill_id == SK_CR_SHIELDBOOMERANG || skill_id == SK_PA_RAPIDSMITING || skill_id == SK_PA_SHIELDSLAM ) { //Refine bonus applies after cards and elements.
		// 	short index = sd->equip_index[EQI_HAND_L];

		// 	if( index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == IT_ARMOR )
		// 		ATK_ADD(wd.damage, wd.damage2, 50*sd->inventory.u.items_inventory[index].refine);
		// }
	}

	if(tsd) { // Card Fix for target (tsd), 2 is not added to the "left" flag meaning "target cards only"

		switch(skill_id) {
			default:

				wd.damage += battle_calc_cardfix(BF_WEAPON, src, target, nk, right_element, left_element, wd.damage, 0, wd.flag);
				if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id))
					wd.damage2 += battle_calc_cardfix(BF_WEAPON, src, target, nk, right_element, left_element, wd.damage2, 1, wd.flag);

				break;
		}

	}

	// only do 1 dmg to plant, no need to calculate rest
	if(infdef){
		battle_calc_attack_plant(&wd, src, target, skill_id, skill_lv);
		return wd;
	}

	//Apply DAMAGE_DIV_FIX and check for min damage
	battle_apply_div_fix(&wd, skill_id);

	battle_calc_attack_left_right_hands(&wd, src, target, skill_id, skill_lv);


	switch (skill_id) {
		
		default:

			battle_calc_attack_gvg_bg(&wd, src, target, skill_id, skill_lv);

			break;
	}


	battle_calc_weapon_final_atk_modifiers(&wd, src, target, skill_id, skill_lv);

	battle_absorb_damage(target, &wd);

	battle_do_reflect(BF_WEAPON,&wd, src, target, skill_id, skill_lv); //WIP [lighta]

	return wd;
}

/*==========================================
 * Calculate "magic"-type attacks and skills
 *------------------------------------------
 * Credits:
 *	Original coder DracoRPG
 *	Refined and optimized by helvetica
 */
struct Damage battle_calc_magic_attack(struct block_list *src,struct block_list *target,uint16 skill_id,uint16 skill_lv,int mflag)
{
	int i, skill_damage = 0;
	short s_ele = 0;

	TBL_PC *sd;
	TBL_PC *tsd;
	struct status_change *sc, *tsc;
	struct Damage ad;
	struct status_data *sstatus = status_get_status_data(src);
	struct status_data *tstatus = status_get_status_data(target);
	struct {
		unsigned imdef : 1;
		unsigned infdef : 1;
	} flag;

	memset(&ad,0,sizeof(ad));
	memset(&flag,0,sizeof(flag));

	if (src == NULL || target == NULL) {
		nullpo_info(NLP_MARK);
		return ad;
	}
	//Initial Values
	ad.damage = 1;
	ad.div_ = skill_get_num(skill_id,skill_lv);
	ad.amotion = (skill_get_inf(skill_id)&INF_GROUND_SKILL ? 0 : sstatus->amotion); //Amotion should be 0 for ground skills.
	ad.dmotion = tstatus->dmotion;
	ad.blewcount = skill_get_blewcount(skill_id, skill_lv);
	ad.flag = BF_MAGIC|BF_SKILL;
	ad.dmg_lv = ATK_DEF;

	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);
	std::bitset<NK_MAX> nk;

	if (skill)
		nk = skill->nk;

	flag.imdef = nk[NK_IGNOREDEFENSE] ? 1 : 0;

	sd = BL_CAST(BL_PC, src);
	tsd = BL_CAST(BL_PC, target);
	sc = status_get_sc(src);
	tsc = status_get_sc(target);

	//Initialize variables that will be used afterwards
	s_ele = skill_get_ele(skill_id, skill_lv);

	if (s_ele == ELE_WEAPON) { // pl=-1 : the skill takes the weapon's element
		s_ele = sstatus->rhw.ele;
		if(sd && sd->spiritcharm_type != CHARM_TYPE_NONE && sd->spiritcharm >= MAX_SPIRITCHARM)
			s_ele = sd->spiritcharm_type; // Summoning 10 spiritcharm will endow your weapon
	} else if (s_ele == ELE_ENDOWED) //Use status element
		s_ele = status_get_attack_sc_element(src,status_get_sc(src));
	else if (s_ele == ELE_RANDOM) //Use random element
		s_ele = rnd()%ELE_ALL;


	//Set miscellaneous data that needs be filled
	if(sd) {
		sd->state.arrow_atk = 0;
		ad.blewcount += battle_blewcount_bonus(sd, skill_id);
	}

	//Skill Range Criteria
	ad.flag |= battle_range_type(src, target, skill_id, skill_lv);

	//Infinite defense (plant mode)
	flag.infdef = is_infinite_defense(target, ad.flag)?1:0;

	if (!flag.infdef) { //No need to do the math for plants
		unsigned int skillratio = 100; //Skill dmg modifiers.
#ifdef RENEWAL
		ad.damage = 0; //reinitialize..
#endif
//MATK_RATE scales the damage. 100 = no change. 50 is halved, 200 is doubled, etc
#define MATK_RATE(a) { ad.damage = ad.damage * (a) / 100; }
//Adds dmg%. 100 = +100% (double) damage. 10 = +10% damage
#define MATK_ADDRATE(a) { ad.damage += ad.damage * (a) / 100; }
//Adds an absolute value to damage. 100 = +100 damage
#define MATK_ADD(a) { ad.damage += a; }

		//Calc base damage according to skill
		switch (skill_id) {
			// case SK_CR_SWORDSTOPLOWSHARES:
			case SK_AL_HEAL:
				ad.damage = skill_calc_heal(src, target, skill_id, skill_lv, false);
				break;
			case SK_PR_ASPERSIO:
				ad.damage = 40;
				break;
			case SK_PR_RESURRECTIO:
				//Undead check is on skill_castend_damageid code.
				i = 10 * skill_lv + sstatus->luk + sstatus->int_ + status_get_lv(src)
				  	+ 300 - 300 * tstatus->hp / tstatus->max_hp;
				if(i > 700)
					i = 700;
				if(rnd()%1000 < i && !status_has_mode(tstatus,MD_STATUSIMMUNE))
					ad.damage = tstatus->hp;
				else {
					if (sstatus->matk_max > sstatus->matk_min) {
						MATK_ADD(sstatus->matk_min+rnd()%(sstatus->matk_max-sstatus->matk_min));
					} else {
						MATK_ADD(sstatus->matk_min);
					}
					MATK_RATE(skill_lv);
				}
				break;

			case SK_AL_RENEW:
				ad.damage = status_get_lv(src) * 10 + sstatus->int_;
				break;
			default: {
				int matk_max = sstatus->matk_max;
				int matk_min = sstatus->matk_min;
				if(skill_id == SK_SA_FIREBALL ||
				skill_id == SK_PF_JUPITELTHUNDER ||
				skill_id == SK_PF_INFERNO ||
				skill_id == SK_PF_ROCKTOMB ||
				skill_id == SK_PF_HYDROPUMP ||
				skill_id == SK_SA_WINDSLASH ||
				skill_id == SK_SA_ICICLE ||
				skill_id == SK_SA_EARTHSPIKE ||
				skill_id == SK_SA_FIREINSIGNIA ||
				skill_id == SK_SA_WATERINSIGNIA ||
				skill_id == SK_SA_WINDINSIGNIA ||
				skill_id == SK_SA_EARTHINSIGNIA ||
				skill_id == SK_AM_BEHOLDER1 ||
				skill_id == SK_CR_BEHOLDER3 ||
				skill_id == SK_SM_PROVOKE ||
				skill_id == SK_AM_BASILISK2 ||
				skill_id == SK_AM_BEHOLDER2
				){
					if (src->type == BL_ELEM) {
						struct map_session_data* sd2 = BL_CAST(BL_PC, battle_get_master(src));
						matk_max = sd2->ed->elemental.matk;
						matk_min = sd2->ed->elemental.matk_min;
					}
				}
				if (matk_max > matk_min) {
					MATK_ADD(matk_max+rnd()%(matk_max-matk_min));
				} else {
					MATK_ADD(matk_min);
				}

				if (sd) { // Soul energy spheres adds MATK.
					MATK_ADD(3*sd->soulball);
				}

				if (nk[NK_SPLASHSPLIT]) { // Divide MATK in case of multiple targets skill
					if (mflag>0)                                 
						ad.damage /= mflag;
					else
						ShowError("0 enemies targeted by %d:%s, divide per 0 avoided!\n", skill_id, skill_get_name(skill_id));
				}
				switch(skill_id) {
					case SK_AL_HOLYGHOST:
					case SK_AL_SACREDWAVE:
					case SK_AL_RUWACH:
						skillratio += AcolyteSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv);
						break;
					case SK_MG_EARTHBOLT:
					case SK_MG_FIREBOLT:
					case SK_MG_LIGHTNINGBOLT:
					case SK_MG_COLDBOLT:
					case SK_MG_DARKSTRIKE:
					case SK_MG_CORRUPT:
					case SK_MG_SOULSTRIKE:
						skillratio += MageSkillAtkRatioCalculator::calculate_skill_atk_ratio(status_get_lv(src), skill_id, skill_lv, target, sc && sc->data[STATUS_SPELLFIST] && mflag & BF_SHORT);
						if (sc && sc->data[STATUS_SPELLFIST] && mflag&BF_SHORT)  {
							skillratio += MageSkillAtkRatioCalculator::calculate_skill_atk_ratio(status_get_lv(src), skill_id, skill_lv, target, true);
							ad.div_ = 1; // ad mods, to make it work similar to regular hits [Xazax]
							ad.flag = BF_WEAPON|BF_SHORT;
							ad.type = DMG_NORMAL;
						}
						break;
					case SK_PR_JUDEX:
					case SK_PR_SPIRITUSANCTI:
					case SK_PR_MAGNUSEXORCISMUS:
					case SK_PR_DUPLELUX_MAGIC:
					case SK_BI_SENTENTIA:
					case SK_BI_BENEDICTIO:
					case SK_BI_DIABOLICRUCIATUS:
					case SK_BI_PENITENTIA:
						skillratio += PriestSkillAttackRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
						break;
					case SK_BA_METALLICSOUND:
					case SK_BA_REVERBERATION:
					case SK_CL_METALLICFURY:
						skillratio += BardSkillAttackRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
						break;
					case SK_CR_GRANDCROSS:
					case SK_PA_GENESISRAY:
					case SK_PA_GLORIADOMINI:
						skillratio += CrusaderSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
						break;
					case SK_WZ_LORDOFVERMILION:
					case SK_WZ_STORMGUST:
					case SK_WZ_METEORSTORM:
					case SK_WZ_EARTHSTRAIN:
					case SK_SA_SOULBURN:
					case SK_SA_DRAINLIFE:
					case SK_SA_ICICLE:
					case SK_SA_WINDSLASH:
					case SK_SA_EARTHSPIKE:
					case SK_SA_FIREBALL:
					case SK_PF_INFERNO:
					case SK_PF_ROCKTOMB:
					case SK_PF_HYDROPUMP:
					case SK_PF_JUPITELTHUNDER:
					case SK_SA_ELEMENTALACTION1:
					case SK_PF_ELEMENTALACTION3:
						skillratio += SageSkillAttackRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
						break;
					case SK_WZ_CRIMSONROCK:
					case SK_SO_SHADOWBOMB:
					case SK_WZ_THUNDERSTORM:
					case SK_SO_PHANTOMSPEAR:
					case SK_WZ_STALAGMITE:
					case SK_WZ_ICEBERG:
					case SK_WZ_ILLUSIONARYBLADES:
					case SK_WZ_ICONOFSIN:
					case SK_WZ_EXTREMEVACUUM:
					case SK_WZ_REALITYBREAKER:
					case SK_SO_ASTRALSTRIKE:
					case SK_SO_DOOM:
					case SK_SO_DOOM_GHOST:
					case SK_SO_DIAMONDDUST:
					case SK_WZ_MAGICCRASHER:
					case SK_WZ_PSYCHICWAVE:
					case SK_MG_VOIDEXPANSION:
					case SK_SO_TETRAVORTEX_FIRE:
					case SK_SO_TETRAVORTEX_WATER:
					case SK_SO_TETRAVORTEX_WIND:
					case SK_SO_TETRAVORTEX_GROUND:
					case SK_SO_TETRAVORTEX_NEUTRAL:
						skillratio += WizardSkillAttackRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
						break;
					case SK_CM_DRAGONBREATH:
						skillratio +=  KnightSkillAtkRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
						break;
					case SK_AM_BEHOLDER1:
					case SK_CR_BEHOLDER3:
					case SK_AM_BEHOLDER2:
					case SK_AM_BASILISK2:
					case SK_AM_WILDTHORNS:
					case SK_AM_FIREDEMONSTRATION:
					case SK_CR_INCENDIARYBOMB:
					case SK_CR_GEOGRAFIELD_ATK:
					case SK_CR_MANDRAKERAID_ATK:
						skillratio += AlchemistSkillAttackRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus, tstatus);
						break;
					case SK_RA_ULLREAGLETOTEM_ATK:
						skillratio += HunterSkillAttackRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
						break;
				}

			

				MATK_RATE(skillratio);
				break;
			}
		}

		ad.damage += battle_calc_cardfix(BF_MAGIC, src, target, nk, s_ele, 0, ad.damage, 0, ad.flag);


		if(sd) {
			//Damage bonuses
			if ((i = pc_skillatk_bonus(sd, skill_id)))
				ad.damage += (int64)ad.damage*i/100;

			//Ignore Defense?
			if (!flag.imdef && (
				sd->bonus.ignore_mdef_ele & ( 1 << tstatus->def_ele ) || sd->bonus.ignore_mdef_ele & ( 1 << ELE_ALL ) ||
				sd->bonus.ignore_mdef_race & ( 1 << tstatus->race ) || sd->bonus.ignore_mdef_race & ( 1 << RC_ALL ) ||
				sd->bonus.ignore_mdef_class & ( 1 << tstatus->class_ ) || sd->bonus.ignore_mdef_class & ( 1 << CLASS_ALL )
			))
				flag.imdef = 1;
		}

		if (tsd && (i = pc_sub_skillatk_bonus(tsd, skill_id)))
			ad.damage -= (int64)ad.damage*i/100;

		if(!flag.imdef){
			defType mdef = tstatus->mdef;
			int mdef2= tstatus->mdef2;

			if (sc && sc->data[STATUS_EXPIATIO]) {
				i = 6 * sc->data[STATUS_EXPIATIO]->val1; // 6% per level

				i = min(i, 100); //cap it to 100 for 5 mdef min
				mdef -= mdef * i / 100;
				//mdef2 -= mdef2 * i / 100;
			}

			if(sd) {
				i = sd->indexed_bonus.ignore_mdef_by_race[tstatus->race] + sd->indexed_bonus.ignore_mdef_by_race[RC_ALL];
				i += sd->indexed_bonus.ignore_mdef_by_class[tstatus->class_] + sd->indexed_bonus.ignore_mdef_by_class[CLASS_ALL];

				std::vector<e_race2> race2 = status_get_race2(target);

				for (const auto &raceit : race2)
					i += sd->indexed_bonus.ignore_mdef_by_race2[raceit];
				if (i)
				{
					if (i > 100) i = 100;
					mdef -= mdef * i/100;
					//mdef2-= mdef2* i/100;
				}
			}
#ifdef RENEWAL
			/**
			 * RE MDEF Reduction
			 * Damage = Magic Attack * (1000+eMDEF)/(1000+eMDEF) - sMDEF
			 */
			if (mdef < 0)
				mdef = 0; // Negative eMDEF is treated as 0 on official

			ad.damage = ad.damage * (1000 + mdef) / (1000 + mdef * 10) - mdef2;
#else
			if(battle_config.magic_defense_type)
				ad.damage = ad.damage - mdef*battle_config.magic_defense_type - mdef2;
			else
				ad.damage = ad.damage * (100-mdef)/100 - mdef2;
#endif
		}
		if(ad.damage<1)
			ad.damage=1;
		else if(sc) { //only applies when hit
			// switch(skill_id) {
			// 	case SK_MG_LIGHTNINGBOLT:
			// 	case MG_THUNDERSTORM:
			// 		if(sc->data[SC_GUST_OPTION])
			// 			ad.damage += (6 + sstatus->int_ / 4) + max(sstatus->dex - 10, 0) / 30;
			// 		break;
			// 	case SK_MG_FIREBOLT:
			// 	case MG_FIREWALL:
			// 		if(sc->data[SC_PYROTECHNIC_OPTION])
			// 			ad.damage += (6 + sstatus->int_ / 4) + max(sstatus->dex - 10, 0) / 30;
			// 		break;
			// 	case SK_MG_COLDBOLT:
			// 	case SK_MG_FROSTDIVER:
			// 		if(sc->data[SC_AQUAPLAY_OPTION])
			// 			ad.damage += (6 + sstatus->int_ / 4) + max(sstatus->dex - 10, 0) / 30;
			// 		break;
			// 	case SK_WZ_STALAGMITE:
			// 	case WZ_HEAVENDRIVE:
			// 		if(sc->data[SC_PETROLOGY_OPTION])
			// 			ad.damage += (6 + sstatus->int_ / 4) + max(sstatus->dex - 10, 0) / 30;
			// 		break;
			// }
		}

		if (!nk[NK_IGNOREELEMENT])
			ad.damage = battle_attr_fix(src, target, ad.damage, s_ele, tstatus->def_ele, tstatus->ele_lv);

		//Apply the physical part of the skill's damage. [Skotlex]
		switch(skill_id) {
			case SK_CR_GRANDCROSS:
			{
					struct Damage wd = battle_calc_weapon_attack(src,target,skill_id,skill_lv,mflag);
					ad.damage = battle_attr_fix(src, target, wd.damage + ad.damage, s_ele, tstatus->def_ele, tstatus->ele_lv) * (20 * skill_lv) / 100;
					if (src == target)
						ad.damage = 0;
				}
				break;
		}

#ifndef RENEWAL
		ad.damage += battle_calc_cardfix(BF_MAGIC, src, target, nk, s_ele, 0, ad.damage, 0, ad.flag);
#endif
	} //Hint: Against plants damage will still be 1 at this point

	//Apply DAMAGE_DIV_FIX and check for min damage
	battle_apply_div_fix(&ad, skill_id);

	struct map_data *mapdata = map_getmapdata(target->m);

	ad.damage = battle_calc_damage(src,target,&ad,ad.damage,skill_id,skill_lv);
	if (mapdata_flag_gvg2(mapdata))
		ad.damage = battle_calc_gvg_damage(src,target,ad.damage,skill_id,ad.flag);
	else if (mapdata->flag[MF_BATTLEGROUND])
		ad.damage = battle_calc_bg_damage(src,target,ad.damage,skill_id,ad.flag);

	// Skill damage adjustment
	if ((skill_damage = battle_skill_damage(src,target,skill_id)) != 0)
		MATK_ADDRATE(skill_damage);

	battle_absorb_damage(target, &ad);
	status_change_end(src, STATUS_MYSTICALAMPLIFICATION, INVALID_TIMER);
	//battle_do_reflect(BF_MAGIC,&ad, src, target, skill_id, skill_lv); //WIP [lighta] Magic skill has own handler at skill_attack
	return ad;
}

/*==========================================
 * Calculate "misc"-type attacks and skills
 *------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Refined and optimized by helvetica
 */
struct Damage battle_calc_misc_attack(struct block_list *src,struct block_list *target,uint16 skill_id,uint16 skill_lv,int mflag)
{
	int skill_damage = 0;
	short i, s_ele;

	struct map_session_data *sd, *tsd;
	struct Damage md; //DO NOT CONFUSE with md of mob_data!
	struct status_data *sstatus = status_get_status_data(src);
	struct status_data *tstatus = status_get_status_data(target);
	struct status_change *ssc = status_get_sc(src);

	memset(&md,0,sizeof(md));

	if (src == NULL || target == NULL) {
		nullpo_info(NLP_MARK);
		return md;
	}

	//Some initial values
	md.amotion = (skill_get_inf(skill_id)&INF_GROUND_SKILL ? 0 : sstatus->amotion);
	md.dmotion = tstatus->dmotion;
	md.div_ = skill_get_num(skill_id,skill_lv);
	md.blewcount = skill_get_blewcount(skill_id,skill_lv);
	md.dmg_lv = ATK_DEF;
	md.flag = BF_MISC|BF_SKILL;

	std::shared_ptr<s_skill_db> skill = skill_db.find(skill_id);
	std::bitset<NK_MAX> nk;

	if (skill)
		nk = skill->nk;

	sd = BL_CAST(BL_PC, src);
	tsd = BL_CAST(BL_PC, target);

	if(sd) {
		sd->state.arrow_atk = 0;
		md.blewcount += battle_blewcount_bonus(sd, skill_id);
	}

	s_ele = skill_get_ele(skill_id, skill_lv);
	if (s_ele == ELE_WEAPON || s_ele == ELE_ENDOWED) //Attack that takes weapon's element for misc attacks? Make it neutral [Skotlex]
		s_ele = ELE_NEUTRAL;
	else if (s_ele == ELE_RANDOM) //Use random element
		s_ele = rnd()%ELE_ALL;

	//Skill Range Criteria
	md.flag |= battle_range_type(src, target, skill_id, skill_lv);

	switch (skill_id) {
		case SK_TF_THROWSTONE:
			md.damage = 50;
			md.flag |= BF_WEAPON;
			break;
		case SK_WG_SLASH:
			md.damage = (sstatus->dex / 5 + sstatus->int_ / 2 + skill_lv * 30) * 10;
			RE_LVL_MDMOD(100);
			HunterSkillAttackRatioCalculator::calculate_skill_atk_ratio(src, target, status_get_lv(src), skill_id, skill_lv, sstatus);
			break;
		case SK_FC_BLITZBEAT:
		case SK_RA_FALCONASSAULT:
			{
				uint16 skill;
				md.damage = (sstatus->dex / 5 + sstatus->agi / 2 + skill_lv * 30) * 2;
				RE_LVL_MDMOD(100);

				if (skill_id == SK_RA_FALCONASSAULT) {
					//Div fix of Blitzbeat
					DAMAGE_DIV_FIX2(md.damage, skill_get_num(SK_FC_BLITZBEAT, 5));
					//Falcon Assault Modifier
					md.damage = md.damage * (150 + 70 * skill_lv) / 100;
				}
			}
			break;
	}

	if (nk[NK_SPLASHSPLIT]) { // Divide ATK among targets
		if(mflag > 0)
			md.damage /= mflag;
		else
			ShowError("0 enemies targeted by %d:%s, divide per 0 avoided!\n", skill_id, skill_get_name(skill_id));
	}

	if (!nk[NK_IGNOREFLEE]) {
		struct status_change *sc = status_get_sc(target);

		i = 0; //Temp for "hit or no hit"
		if(sc && sc->opt1 && sc->opt1 != OPT1_STONEWAIT && sc->opt1 != OPT1_BURNING)
			i = 1;
		else {
			short
				flee = tstatus->flee,
#ifdef RENEWAL
				hitrate = 0; //Default hitrate
#else
				hitrate = 80; //Default hitrate
#endif

			if(battle_config.agi_penalty_type && battle_config.agi_penalty_target&target->type) {
				unsigned char attacker_count = unit_counttargeted(target); //256 max targets should be a sane max

				if(attacker_count >= battle_config.agi_penalty_count) {
					if (battle_config.agi_penalty_type == 1)
						flee = (flee * (100 - (attacker_count - (battle_config.agi_penalty_count - 1))*battle_config.agi_penalty_num))/100;
					else //assume type 2: absolute reduction
						flee -= (attacker_count - (battle_config.agi_penalty_count - 1))*battle_config.agi_penalty_num;
					if(flee < 1)
						flee = 1;
				}
			}

			hitrate += sstatus->hit - flee;
#ifdef RENEWAL
			if( sd ) //in Renewal hit bonus from Vultures Eye is not shown anymore in status window
				hitrate += pc_checkskill(sd,SK_AC_VULTURE) * 2;
#endif
			hitrate = cap_value(hitrate, battle_config.min_hitrate, battle_config.max_hitrate);

			if(rnd()%100 < hitrate)
				i = 1;
		}
		if (!i) {
			md.damage = 0;
			md.dmg_lv = ATK_FLEE;
		}
	}

	md.damage += battle_calc_cardfix(BF_MISC, src, target, nk, s_ele, 0, md.damage, 0, md.flag);

	if (sd && (i = pc_skillatk_bonus(sd, skill_id)))
		md.damage += (int64)md.damage*i/100;

	if (tsd && (i = pc_sub_skillatk_bonus(tsd, skill_id)))
		md.damage -= (int64)md.damage*i/100;

	if(!nk[NK_IGNOREELEMENT])
		md.damage=battle_attr_fix(src, target, md.damage, s_ele, tstatus->def_ele, tstatus->ele_lv);

	//Plant damage
	if(md.damage < 0)
		md.damage = 0;
	else if(md.damage && is_infinite_defense(target, md.flag)) {
		md.damage = 1;
	}

	//Apply DAMAGE_DIV_FIX and check for min damage
	battle_apply_div_fix(&md, skill_id);

	

	struct map_data *mapdata = map_getmapdata(target->m);

	md.damage = battle_calc_damage(src,target,&md,md.damage,skill_id,skill_lv);
	if(mapdata_flag_gvg2(mapdata))
		md.damage = battle_calc_gvg_damage(src,target,md.damage,skill_id,md.flag);
	else if(mapdata->flag[MF_BATTLEGROUND])
		md.damage = battle_calc_bg_damage(src,target,md.damage,skill_id,md.flag);

	// Skill damage adjustment
	if ((skill_damage = battle_skill_damage(src,target,skill_id)) != 0)
		md.damage += (int64)md.damage * skill_damage / 100;

	battle_absorb_damage(target, &md);

	battle_do_reflect(BF_MISC,&md, src, target, skill_id, skill_lv); //WIP [lighta]

	return md;
}

/**
 * Calculate vanish damage on a target
 * @param sd: Player with vanish item
 * @param target: Target to vanish HP/SP
 * @param flag: Damage struct battle flag
 */
void battle_vanish_damage(struct map_session_data *sd, struct block_list *target, int flag)
{
	nullpo_retv(sd);
	nullpo_retv(target);

	// bHPVanishRate
	int16 vanish_hp = 0;
	if (!sd->hp_vanish.empty()) {
		for (auto &it : sd->hp_vanish) {
			if (!(((it.flag)&flag)&BF_WEAPONMASK &&
				((it.flag)&flag)&BF_RANGEMASK &&
				((it.flag)&flag)&BF_SKILLMASK))
				continue;
			if (it.rate && (it.rate >= 1000 || rnd() % 1000 < it.rate))
				vanish_hp += it.per;
		}
	}

	// bSPVanishRate
	int16 vanish_sp = 0;
	if (!sd->sp_vanish.empty()) {
		for (auto &it : sd->sp_vanish) {
			if (!(((it.flag)&flag)&BF_WEAPONMASK &&
				((it.flag)&flag)&BF_RANGEMASK &&
				((it.flag)&flag)&BF_SKILLMASK))
				continue;
			if (it.rate && (it.rate >= 1000 || rnd() % 1000 < it.rate))
				vanish_sp += it.per;
		}
	}

	if (vanish_hp > 0 || vanish_sp > 0)
		status_percent_damage(&sd->bl, target, -vanish_hp, -vanish_sp, false); // Damage HP/SP applied once
}

/*==========================================
 * Battle main entry, from skill_attack
 *------------------------------------------
 * Credits:
 *	Original coder unknown
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
struct Damage battle_calc_attack(int attack_type,struct block_list *bl,struct block_list *target,uint16 skill_id,uint16 skill_lv,int flag)
{
	struct Damage d;
	if(skill_id == SK_RA_ULLREAGLETOTEM_ATK) {
		attack_type = BF_MAGIC;
	}
	switch(attack_type) {
		case BF_WEAPON: d = battle_calc_weapon_attack(bl,target,skill_id,skill_lv,flag); break;
		case BF_MAGIC:  d = battle_calc_magic_attack(bl,target,skill_id,skill_lv,flag);  break;
		case BF_MISC:   d = battle_calc_misc_attack(bl,target,skill_id,skill_lv,flag);   break;
		default:
			ShowError("battle_calc_attack: unknown attack type! %d (skill_id=%d, skill_lv=%d)\n", attack_type, skill_id, skill_lv);
			memset(&d,0,sizeof(d));
			break;
		}
	if( d.damage + d.damage2 < 1 )
	{	//Miss/Absorbed
		//Weapon attacks should go through to cause additional effects.
		if (d.dmg_lv == ATK_DEF /*&& attack_type&(BF_MAGIC|BF_MISC)*/) // Isn't it that additional effects don't apply if miss?
			d.dmg_lv = ATK_MISS;
		d.dmotion = 0;
	}
	else // Some skills like Weaponry Research will cause damage even if attack is dodged
		d.dmg_lv = ATK_DEF;

	struct map_session_data *sd = BL_CAST(BL_PC, bl);

	if (sd && d.damage + d.damage2 > 1)
		battle_vanish_damage(sd, target, d.flag);

	return d;
}

/*==========================================
 * Final damage return function
 *------------------------------------------
 * Credits:
 *	Original coder unknown
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
int64 battle_calc_return_damage(struct block_list* bl, struct block_list *src, int64 *dmg, int flag, uint16 skill_id, bool status_reflect){
	struct map_session_data* sd;
	int64 rdamage = 0, damage = *dmg;
	int max_damage = status_get_max_hp(bl);
	struct status_change *sc, *ssc;

	sd = BL_CAST(BL_PC, bl);
	sc = status_get_sc(bl);
	ssc = status_get_sc(src);

	if (sc) { // These statuses do not reflect any damage (off the target)
		if (sc->data[STATUS_MAGICCELL] || sc->data[STATUS_DARKCLAW] && (!ssc)) // Nullify reflecting ability except for Shield Spell - Def
				return 0;
	}

	if (ssc) {
		if (ssc->data[STATUS_ULLREAGLETOTEM])
			return 0;
	}

	if (flag & BF_SHORT) {//Bounces back part of the damage.
		if ( (skill_get_inf2(skill_id, INF2_ISTRAP) || !status_reflect) && sd && sd->bonus.short_weapon_damage_return ) {
			rdamage += damage * sd->bonus.short_weapon_damage_return / 100;
			rdamage = i64max(rdamage, 1);
		} else if( status_reflect && sc && sc->count ) {
			if( sc->data[STATUS_REFLECTSHIELD] ) {
				struct status_change_entry *sce_d;
				struct block_list *d_bl = NULL;

				if( (sce_d = sc->data[STATUS_SWORNPROTECTOR]) && (d_bl = map_id2bl(sce_d->val1)) &&
					(
					(d_bl->type == BL_PC && ((TBL_PC*)d_bl)->devotion[sce_d->val2] == bl->id)) )
				{ //Don't reflect non-skill attack if has STATUS_REFLECTSHIELD from Devotion bonus inheritance
					if( (!skill_id && battle_config.devotion_rdamage_skill_only && sc->data[STATUS_REFLECTSHIELD]->val4) ||
						!check_distance_bl(bl,d_bl,sce_d->val3) )
						return 0;
				}
			}
			{
				if ( sc->data[STATUS_REFLECTSHIELD] ) {
					// Don't reflect non-skill attack if has STATUS_REFLECTSHIELD from Devotion bonus inheritance
					if (!skill_id && battle_config.devotion_rdamage_skill_only && sc->data[STATUS_REFLECTSHIELD]->val4)
						rdamage = 0;
					else {
						rdamage += damage * ((sc->data[STATUS_REFLECTSHIELD]->val2 *2) + 20) / 100;
						rdamage = i64max(rdamage, 1);
					}
				}
			}
		}
	} else {
		if (!status_reflect && sd && sd->bonus.long_weapon_damage_return) {
			rdamage += damage * sd->bonus.long_weapon_damage_return / 100;
			rdamage = i64max(rdamage, 1);
		}
	}

	if (rdamage > 0) {
		map_session_data* ssd = BL_CAST(BL_PC, src);
		if (ssd && ssd->bonus.reduce_damage_return != 0) {
			rdamage -= rdamage * ssd->bonus.reduce_damage_return / 100;
			rdamage = i64max(rdamage, 1);
		}
	}



	return cap_value(min(rdamage,max_damage),INT_MIN,INT_MAX);
}

/**
 * Calculate Vellum damage on a target
 * @param sd: Player with vanish item
 * @param target: Target to vanish HP/SP
 * @param wd: Damage struct reference
 * @return True on damage done or false if not
 */
bool battle_vellum_damage(struct map_session_data *sd, struct block_list *target, struct Damage *wd)
{
	nullpo_retr(false, sd);
	nullpo_retr(false, target);
	nullpo_retr(false, wd);

	struct status_data *tstatus = status_get_status_data(target);
	// bHPVanishRaceRate
	int16 vellum_rate_hp = cap_value(sd->hp_vanish_race[tstatus->race].rate + sd->hp_vanish_race[RC_ALL].rate, 0, INT16_MAX);
	int8 vellum_hp = cap_value(sd->hp_vanish_race[tstatus->race].per + sd->hp_vanish_race[RC_ALL].per, INT8_MIN, INT8_MAX);
	// bSPVanishRaceRate
	int16 vellum_rate_sp = cap_value(sd->sp_vanish_race[tstatus->race].rate + sd->sp_vanish_race[RC_ALL].rate, 0, INT16_MAX);
	int8 vellum_sp = cap_value(sd->sp_vanish_race[tstatus->race].per + sd->sp_vanish_race[RC_ALL].per, INT8_MIN, INT8_MAX);

	// The HP and SP damage bonus from these items don't stack because of the special damage display for SP.
	// Vellum damage overrides any other damage done as well.
	if (vellum_hp && vellum_rate_hp && (vellum_rate_hp >= 1000 || rnd() % 1000 < vellum_rate_hp)) {
		wd->damage = apply_rate(tstatus->max_hp, vellum_hp);
		wd->damage2 = 0;
	} else if (vellum_sp && vellum_rate_sp && (vellum_rate_sp >= 1000 || rnd() % 1000 < vellum_rate_sp)) {
		wd->damage = apply_rate(tstatus->max_sp, vellum_sp);
		wd->damage2 = 0;
		wd->isspdamage = true;
	} else
		return false;

	return true;
}

/*===========================================
 * Perform battle drain effects (HP/SP loss)
 *-------------------------------------------*/
void battle_drain(struct map_session_data *sd, struct block_list *tbl, int64 rdamage, int64 ldamage, int race, int class_)
{
	struct weapon_data *wd;
	int64 *damage;
	int thp = 0, // HP gained
		tsp = 0, // SP gained
		//rhp = 0, // HP reduced from target
		//rsp = 0, // SP reduced from target
		hp = 0, sp = 0;

	if (!CHK_RACE(race) && !CHK_CLASS(class_))
		return;

	for (int i = 0; i < 4; i++) {
		//First two iterations: Right hand
		if (i < 2) {
			wd = &sd->right_weapon;
			damage = &rdamage;
		} else {
			wd = &sd->left_weapon;
			damage = &ldamage;
		}

		if (*damage <= 0)
			continue;

		if (i == 1 || i == 3) {
			hp = wd->hp_drain_class[class_] + wd->hp_drain_class[CLASS_ALL];
			hp += battle_calc_drain(*damage, wd->hp_drain_rate.rate, wd->hp_drain_rate.per);

			sp = wd->sp_drain_class[class_] + wd->sp_drain_class[CLASS_ALL];
			sp += battle_calc_drain(*damage, wd->sp_drain_rate.rate, wd->sp_drain_rate.per);

			if( hp ) {
				//rhp += hp;
				thp += hp;
			}

			if( sp ) {
				//rsp += sp;
				tsp += sp;
			}
		} else {
			hp = wd->hp_drain_race[race] + wd->hp_drain_race[RC_ALL];
			sp = wd->sp_drain_race[race] + wd->sp_drain_race[RC_ALL];

			if( hp ) {
				//rhp += hp;
				thp += hp;
			}

			if( sp ) {
				//rsp += sp;
				tsp += sp;
			}
		}
	}

	if (!thp && !tsp)
		return;

	status_heal(&sd->bl, thp, tsp, battle_config.show_hp_sp_drain?3:1);

	//if (rhp || rsp)
	//	status_zap(tbl, rhp, rsp);
}
/*===========================================
 * Deals the same damage to targets in area.
 *-------------------------------------------
 * Credits:
 *	Original coder pakpil
 */
int battle_damage_area(struct block_list *bl, va_list ap) {
	t_tick tick;
	int64 damage;
	int amotion, dmotion;
	struct block_list *src;

	nullpo_ret(bl);

	tick = va_arg(ap, t_tick);
	src = va_arg(ap,struct block_list *);
	amotion = va_arg(ap,int);
	dmotion = va_arg(ap,int);
	damage = va_arg(ap,int);

	if (status_bl_has_mode(bl, MD_SKILLIMMUNE) || status_get_class(bl) == MOBID_EMPERIUM)
		return 0;
	if( bl != src && battle_check_target(src,bl,BCT_ENEMY) > 0 ) {
		map_freeblock_lock();
		if( src->type == BL_PC )
			battle_drain((TBL_PC*)src, bl, damage, damage, status_get_race(bl), status_get_class_(bl));
		if( amotion )
			battle_delay_damage(tick, amotion,src,bl,0,SK_CR_REFLECTSHIELD,0,damage,ATK_DEF,0,true,false);
		/*else
			status_fix_damage(src,bl,damage,0,LG_REFLECTDAMAGE);*/
		clif_damage(bl,bl,tick,amotion,dmotion,damage,1,DMG_ENDURE,0,false);
		SkillAdditionalEffects::skill_additional_effect(src, bl, SK_CR_REFLECTSHIELD, 1, BF_WEAPON|BF_SHORT|BF_NORMAL,ATK_DEF,tick);
		map_freeblock_unlock();
	}

	return 0;
}
/*==========================================
 * Do a basic physical attack (call through unit_attack_timer)
 *------------------------------------------*/
enum damage_lv battle_weapon_attack(struct block_list* src, struct block_list* target, t_tick tick, int flag) {
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct status_data *sstatus, *tstatus;
	struct status_change *sc, *tsc;
	int64 damage;
	int skillv;
	struct Damage wd;
	bool vellum_damage = false;

	nullpo_retr(ATK_NONE, src);
	nullpo_retr(ATK_NONE, target);

	if (src->prev == NULL || target->prev == NULL)
		return ATK_NONE;

	sd = BL_CAST(BL_PC, src);
	tsd = BL_CAST(BL_PC, target);

	sstatus = status_get_status_data(src);
	tstatus = status_get_status_data(target);

	sc = status_get_sc(src);
	tsc = status_get_sc(target);

	if (sc && !sc->count) //Avoid sc checks when there's none to check for. [Skotlex]
		sc = NULL;
	if (tsc && !tsc->count)
		tsc = NULL;

	if (sd)
	{
		sd->state.arrow_atk = (sd->status.weapon == W_BOW || (sd->status.weapon >= W_REVOLVER && sd->status.weapon <= W_GRENADE));
		if (sd->state.arrow_atk)
		{
			short index = sd->equip_index[EQI_AMMO];
			if (index < 0) {
				if (sd->weapontype1 > W_KATAR && sd->weapontype1 < W_HUUMA)
					clif_skill_fail(sd,0,USESKILL_FAIL_NEED_MORE_BULLET,0);
				else
					clif_arrow_fail(sd,0);
				return ATK_NONE;
			}
			//Ammo check by Ishizu-chan
			if (sd->inventory_data[index]) {
				switch (sd->status.weapon) {
					case W_BOW:
						if (sd->inventory_data[index]->subtype != AMMO_ARROW) {
							clif_arrow_fail(sd,0);
							return ATK_NONE;
						}
						break;
					case W_REVOLVER:
					case W_RIFLE:
					case W_GATLING:
					case W_SHOTGUN:
						if (sd->inventory_data[index]->subtype != AMMO_BULLET) {
							clif_skill_fail(sd,0,USESKILL_FAIL_NEED_MORE_BULLET,0);
							return ATK_NONE;
						}
						break;
					case W_GRENADE:
						if (sd->inventory_data[index]->subtype !=
#ifdef RENEWAL
							AMMO_BULLET) {
#else
							AMMO_GRENADE) {
#endif
							clif_skill_fail(sd,0,USESKILL_FAIL_NEED_MORE_BULLET,0);
							return ATK_NONE;
						}
						break;
				}
			}
		}
	}
	if (sc && sc->count) {
		if (sc->data[STATUS_CLOAKING] && !(sc->data[STATUS_CLOAKING]->val4 & 2))
			status_change_end(src, STATUS_CLOAKING, INVALID_TIMER);
	}
	if (tsc && tsc->data[STATUS_AUTOCOUNTER] && status_check_skilluse(target, src, SK_KN_COUNTERATTACK, 1)) {
		uint8 dir = map_calc_dir(target,src->x,src->y);
		int t_dir = unit_getdir(target);
		int dist = distance_bl(src, target);

		if (dist <= 0 || ( dist <= tstatus->rhw.range+1)) {
			uint16 skill_lv = tsc->data[STATUS_AUTOCOUNTER]->val1;

			clif_skillcastcancel(target); //Remove the casting bar. [Skotlex]
			clif_damage(src, target, tick, sstatus->amotion, 1, 0, 1, DMG_NORMAL, 0, false); //Display MISS.
			status_change_end(target, STATUS_AUTOCOUNTER, INVALID_TIMER);
			skill_attack(BF_WEAPON,target,target,src,SK_KN_COUNTERATTACK,skill_lv,tick,0);
			return ATK_BLOCK;
		}
	}

	if( tsc && tsc->data[STATUS_GRAPPLE_WAIT] &&
		(src->type == BL_PC || tsd == NULL || distance_bl(src, target) <= (tsd->status.weapon == W_FIST ? 1 : 2)) )
	{
		uint16 skill_lv = tsc->data[STATUS_GRAPPLE_WAIT]->val1;
		int duration = skill_get_time2(SK_SH_GRAPPLE,skill_lv);
		// if (status_get_class_(src) == CLASS_BOSS)
		// 	duration = 2000; // Only lasts 2 seconds for Boss monsters

		status_change_end(target, STATUS_GRAPPLE_WAIT, INVALID_TIMER);
		if(sc_start4(src,src, STATUS_GRAPPLE, 100, sd?pc_checkskill(sd, SK_SH_GRAPPLE):5, 0, 0, target->id, duration))
		{	//Target locked.
			clif_damage(src, target, tick, sstatus->amotion, 1, 0, 1, DMG_NORMAL, 0, false); //Display MISS.
			clif_bladestop(target, src->id, 1);
			sc_start4(src,target, STATUS_GRAPPLE, 100, skill_lv, 0, 0, src->id, duration);
			return ATK_BLOCK;
		}
	}

	if(sd && (skillv = pc_checkskill(sd,SK_MO_TRIPLEARMCANNON)) > 0) {
		int triple_rate = 6 * skillv;
		if (sd && sd->status.weapon == W_KNUCKLE) {
			triple_rate += 15;
		}
		if (rnd()%100 < triple_rate) {
			//Need to apply canact_tick here because it doesn't go through skill_castend_id
			sd->ud.canact_tick = i64max(tick + skill_delayfix(src, SK_MO_TRIPLEARMCANNON, skillv), sd->ud.canact_tick);
			if( skill_attack(BF_WEAPON,src,src,target,SK_MO_TRIPLEARMCANNON,skillv,tick,0) )
				return ATK_DEF;
			return ATK_MISS;
		}
	}

	if (sc) {
		if (sc->data[STATUS_RECKONING]) {
			uint16 skill_lv = sc->data[STATUS_RECKONING]->val1;
			damage_lv ret_val;

			if( --sc->data[STATUS_RECKONING]->val2 <= 0 )
				status_change_end(src, STATUS_RECKONING, INVALID_TIMER);

			/**
			 * We need to calculate the DMG before the hp reduction, because it can kill the source.
			 * For further information: bugreport:4950
			 */
			ret_val = (damage_lv)skill_attack(BF_WEAPON,src,src,target,SK_KN_RECKONING,skill_lv,tick,0);

			// status_zap(src, sstatus->max_hp*9/100, 0);//Damage to self is always 9%
			if( ret_val == ATK_NONE )
				return ATK_MISS;
			return ret_val;
		}

		if (sc->data[STATUS_LIVINGTORNADO]) {
			uint16 skill_lv = sc->data[STATUS_LIVINGTORNADO]->val1;
			damage_lv ret_val;

			if( --sc->data[STATUS_LIVINGTORNADO]->val2 <= 0 )
				status_change_end(src, STATUS_LIVINGTORNADO, INVALID_TIMER);

			/**
			 * We need to calculate the DMG before the hp reduction, because it can kill the source.
			 * For further information: bugreport:4950
			 */
			ret_val = (damage_lv)skill_attack(BF_WEAPON,src,src,target,SK_HT_LIVINGTORNADO,skill_lv,tick,0);

			// status_zap(src, sstatus->max_hp*9/100, 0);//Damage to self is always 9%
			if( ret_val == ATK_NONE )
				return ATK_MISS;
			return ret_val;
		}

		if (sc->data[STATUS_ZEPHYRSNIPING]) {
			uint16 skill_lv = sc->data[STATUS_ZEPHYRSNIPING]->val1;
			damage_lv ret_val;

			if( --sc->data[STATUS_ZEPHYRSNIPING]->val2 <= 0 )
				status_change_end(src, STATUS_ZEPHYRSNIPING, INVALID_TIMER);

			/**
			 * We need to calculate the DMG before the hp reduction, because it can kill the source.
			 * For further information: bugreport:4950
			 */
			ret_val = (damage_lv)skill_attack(BF_WEAPON,src,src,target,SK_RA_ZEPHYRSNIPING,skill_lv,tick,0);

			// status_zap(src, sstatus->max_hp*9/100, 0);//Damage to self is always 9%
			if( ret_val == ATK_NONE )
				return ATK_MISS;
			return ret_val;
		}


		
		
		
	}

	

	
	

	wd = battle_calc_attack(BF_WEAPON, src, target, 0, 0, flag);

	if (sd && wd.damage + wd.damage2 > 0 && battle_vellum_damage(sd, target, &wd))
		vellum_damage = true;

	if( sc && sc->count ) {
		
		if( sc->data[STATUS_SPELLFIST] ) {
			if( --(sc->data[STATUS_SPELLFIST]->val1) >= 0 && !vellum_damage ){
				if (!is_infinite_defense(target, wd.flag)) {
					struct Damage ad = battle_calc_attack(BF_MAGIC, src, target, sc->data[STATUS_SPELLFIST]->val3, sc->data[STATUS_SPELLFIST]->val4, flag | BF_SHORT);

					wd.damage = ad.damage;
					DAMAGE_DIV_FIX(wd.damage, wd.div_); // Double the damage for multiple hits.
				} else {
					wd.damage = 1;
					DAMAGE_DIV_FIX(wd.damage, wd.div_);
				}
			} else
				status_change_end(src,STATUS_SPELLFIST,INVALID_TIMER);
		}
		
		if( sd && battle_config.arrow_decrement && sc->data[STATUS_TYPHOONFLOW] && sc->data[STATUS_TYPHOONFLOW]->val4 > 0) {
			short idx = sd->equip_index[EQI_AMMO];
			if (idx >= 0 && sd->inventory.u.items_inventory[idx].amount >= sc->data[STATUS_TYPHOONFLOW]->val4) {
				pc_delitem(sd,idx,sc->data[STATUS_TYPHOONFLOW]->val4,0,1,LOG_TYPE_CONSUME);
				sc->data[STATUS_TYPHOONFLOW]->val4 = 0;
			}
		}
	}
	if (sd && sd->state.arrow_atk) //Consume arrow.
		battle_consume_ammo(sd, 0, 0);

	damage = wd.damage + wd.damage2;
	if( damage > 0 && src != target )
	{
		if( sc && sc->data[STATUS_DUPLELUX] && (wd.flag&BF_SHORT) && rnd()%100 <= sc->data[STATUS_DUPLELUX]->val1*8 )
		{	// Activates it only from melee damage
			uint16 skill_id;
			if( rnd()%2 == 1 )
				skill_id = SK_PR_DUPLELUX_MELEE;
			else
				skill_id = SK_PR_DUPLELUX_MAGIC;
			skill_attack(skill_get_type(skill_id), src, src, target, skill_id, sc->data[STATUS_DUPLELUX]->val1, tick, SD_LEVEL);
		}
	}

	wd.dmotion = clif_damage(src, target, tick, wd.amotion, wd.dmotion, wd.damage, wd.div_ , wd.type, wd.damage2, wd.isspdamage);

	if (sd && sd->bonus.splash_range > 0 && damage > 0)
		skill_castend_damage_id(src, target, 0, 1, tick, 0);
	if ( target->type == BL_SKILL && damage > 0 ) {
		TBL_SKILL *su = (TBL_SKILL*)target;

		if (su && su->group) {
			if (su->group->skill_id == SK_AM_WILDTHORNS) {
				if (--su->val2 <= 0)
					skill_delunit(su);
			}
		}
	}

	map_freeblock_lock();

	if( !(tsc && tsc->data[STATUS_SWORNPROTECTOR]) && !vellum_damage && skill_check_shadowform(target, damage, wd.div_) ) {
		if( !status_isdead(target) )
			SkillAdditionalEffects::skill_additional_effect(src, target, 0, 0, wd.flag, wd.dmg_lv, tick);
		if( wd.dmg_lv > ATK_BLOCK )
			skill_counter_additional_effect(src, target, 0, 0, wd.flag, tick);
	} else
		battle_delay_damage(tick, wd.amotion, src, target, wd.flag, 0, 0, damage, wd.dmg_lv, wd.dmotion, true, wd.isspdamage);
	if( tsc ) {
		if( tsc->data[STATUS_SWORNPROTECTOR] ) {
			struct status_change_entry *sce = tsc->data[STATUS_SWORNPROTECTOR];
			struct block_list *d_bl = map_id2bl(sce->val1);

			if( d_bl && (
				(d_bl->type == BL_PC && ((TBL_PC*)d_bl)->devotion[sce->val2] == target->id)
				) && check_distance_bl(target, d_bl, sce->val3) )
			{
				// Only trigger if the devoted player was hit
				if( damage > 0 ){
					struct map_session_data* dsd = BL_CAST( BL_PC, d_bl );

					// The devoting player needs to stand up
					if( dsd && pc_issit( dsd ) ){
						pc_setstand( dsd, true );
						skill_sit( dsd, 0 );
					}

					clif_damage(d_bl, d_bl, gettick(), wd.amotion, wd.dmotion, damage, 1, DMG_NORMAL, 0, false);
					status_fix_damage(NULL, d_bl, damage, 0, SK_CR_SWORNPROTECTOR);
				}
			}
			else
				status_change_end(target, STATUS_SWORNPROTECTOR, INVALID_TIMER);
		}
	}
	if (sc && sc->data[STATUS_AUTOSPELL] && rnd()%100 < sc->data[STATUS_AUTOSPELL]->val4) {
		int sp = 0;
		uint16 skill_id = sc->data[STATUS_AUTOSPELL]->val2;
		uint16 skill_lv = pc_checkskill(sd, skill_id);
		
		sp = skill_get_sp(skill_id,skill_lv) * 2 / 3;
		if (status_charge(src, 0, sp)) {
			struct unit_data *ud = unit_bl2ud(src);
			switch (skill_get_casttype(skill_id)) {
				case CAST_GROUND:
					skill_castend_pos2(src, target->x, target->y, skill_id, skill_lv, tick, flag);
					break;
				case CAST_NODAMAGE:
					skill_castend_nodamage_id(src, target, skill_id, skill_lv, tick, flag);
					break;
				case CAST_DAMAGE:
					skill_castend_damage_id(src, target, skill_id, skill_lv, tick, flag);
					break;
			}
			if (ud) {
				int autospell_tick = skill_delayfix(src, skill_id, skill_lv);

				if (DIFF_TICK(ud->canact_tick, tick + autospell_tick) < 0) {
					ud->canact_tick = i64max(tick + autospell_tick, ud->canact_tick);
					if (battle_config.display_status_timers && sd)
						clif_status_change(src, EFST_POSTDELAY, 1, autospell_tick, 0, 0, 0);
				}
			}
		}
	}
	if (sd) {
		uint16 r_skill = 0, sk_idx = 0;
		if( wd.flag&BF_WEAPON && sc && sc->data[STATUS_AUTOSHADOWSPELL] && rnd()%100 < sc->data[STATUS_AUTOSHADOWSPELL]->val3 &&
			(r_skill = (uint16)sc->data[STATUS_AUTOSHADOWSPELL]->val1) && (sk_idx = skill_get_index(r_skill)) &&
			sd->status.skill[sk_idx].id != 0 && sd->status.skill[sk_idx].flag == SKILL_FLAG_PLAGIARIZED )
		{
			int r_lv = sc->data[STATUS_AUTOSHADOWSPELL]->val2;
			std::shared_ptr<s_skill_db> skill = skill_db.find(r_skill);
			bool abort_autoshadowspell = false;
			if (skill->require.state == ST_SHIELD){
				if(!sd->status.shield){
					abort_autoshadowspell = true;
				}
			}
			if (!status_charge(&sd->bl, 0, 20)){
				abort_autoshadowspell = true;
			}
			if (skill->require.weapon == 8192 || skill->require.weapon == 2048){
				if(sd->status.weapon != W_BOW){
					abort_autoshadowspell = true;
				}
			}
	
			
			struct s_skill_condition require;
			require = skill_get_requirement(sd,r_skill,r_lv);
			int i;
			for( i = 0; i < MAX_SKILL_ITEM_REQUIRE; ++i )
			{
				if( !require.itemid[i] )
					continue;
				if( (pc_search_inventory(sd,require.itemid[i])) < 0 ){
					abort_autoshadowspell = true;
				}
			}
			
			if (!skill_check_condition_castend(sd, r_skill, r_lv)){
				abort_autoshadowspell = true;
			}
			battle_consume_ammo(sd, r_skill, r_lv);
			int type;
			if (!abort_autoshadowspell){
				if( (type = skill_get_casttype(r_skill)) == CAST_GROUND ) {
					int maxcount = 0;

					if( !(BL_PC&battle_config.skill_reiteration) && skill->unit_flag[UF_NOREITERATION] )
							type = -1;

					if( BL_PC&battle_config.skill_nofootset && skill->unit_flag[UF_NOFOOTSET] )
							type = -1;

					if( BL_PC&battle_config.land_skill_limit &&
						(maxcount = skill_get_maxcount(r_skill, r_lv)) > 0
					) {
						int v;
						for(v=0;v<MAX_SKILLUNITGROUP && sd->ud.skillunit[v] && maxcount;v++) {
							if(sd->ud.skillunit[v]->skill_id == r_skill)
								maxcount--;
						}
						if( maxcount == 0 )
							type = -1;
					}

					if( type != CAST_GROUND ){
						clif_skill_fail(sd,r_skill,USESKILL_FAIL_LEVEL,0);
						map_freeblock_unlock();
						return wd.dmg_lv;
					}
				}

				if (sd->state.autocast == 0) {
					sd->state.autocast = 1;
					switch (type) {
						case CAST_GROUND:
							skill_castend_pos2(src, target->x, target->y, r_skill, r_lv, tick, flag);
							break;
						case CAST_NODAMAGE:
							skill_castend_nodamage_id(src, target, r_skill, r_lv, tick, flag);
							break;
						case CAST_DAMAGE:
							skill_castend_damage_id(src, target, r_skill, r_lv, tick, flag);
							break;
					}
				}
				sd->state.autocast = 0;

				sd->ud.canact_tick = i64max(tick + skill_delayfix(src, r_skill, r_lv), sd->ud.canact_tick);
				clif_status_change(src, EFST_POSTDELAY, 1, skill_delayfix(src, r_skill, r_lv), 0, 0, 1);
			}

		}
		if (wd.flag & BF_WEAPON && src != target && damage > 0) {
			if (battle_config.left_cardfix_to_right)
				battle_drain(sd, target, wd.damage, wd.damage, tstatus->race, tstatus->class_);
			else
				battle_drain(sd, target, wd.damage, wd.damage2, tstatus->race, tstatus->class_);
		}
	}

	if (tsc) {
		if (damage > 0 && tsc->data[STATUS_POISONREACT] &&
			(rnd()%100 < tsc->data[STATUS_POISONREACT]->val3
			|| sstatus->def_ele == ELE_POISON) &&
//			check_distance_bl(src, target, tstatus->rhw.range+1) && Doesn't checks range! o.O;
			status_check_skilluse(target, src, SK_TF_POISONSLASH, 0)
		) {	//Poison React
			struct status_change_entry *sce = tsc->data[STATUS_POISONREACT];
			if (sstatus->def_ele == ELE_POISON) {
				sce->val2 = 0;
				skill_attack(BF_WEAPON,target,target,src,SK_TF_POISONREACT,sce->val1,tick,0);
			} else {
				skill_attack(BF_WEAPON,target,target,src,SK_TF_POISONSLASH, 5, tick, 0);
				--sce->val2;
			}
			if (sce->val2 <= 0)
				status_change_end(target, STATUS_POISONREACT, INVALID_TIMER);
		}
	}
	map_freeblock_unlock();
	return wd.dmg_lv;
}

/*=========================
 * Check for undead status
 *-------------------------
 * Credits:
 *	Original coder Skotlex
 *  Refactored by Baalberith
 */
int battle_check_undead(int race,int element)
{
	if(element == ELE_UNDEAD)
		return 1;
	if(race == RC_UNDEAD)
		return 1;

	return 0;
}

/*================================================================
 * Returns the upmost level master starting with the given object
 *----------------------------------------------------------------*/
struct block_list* battle_get_master(struct block_list *src)
{
	struct block_list *prev; //Used for infinite loop check (master of yourself?)
	do {
		prev = src;
		switch (src->type) {
			case BL_PET:
				if (((TBL_PET*)src)->master)
					src = (struct block_list*)((TBL_PET*)src)->master;
				break;
			case BL_MOB:
				if (((TBL_MOB*)src)->master_id)
					src = map_id2bl(((TBL_MOB*)src)->master_id);
				break;
			
			case BL_ELEM:
				if (((TBL_ELEM*)src)->master)
					src = (struct block_list*)((TBL_ELEM*)src)->master;
				break;
			case BL_SKILL:
				if (((TBL_SKILL*)src)->group && ((TBL_SKILL*)src)->group->src_id)
					src = map_id2bl(((TBL_SKILL*)src)->group->src_id);
				break;
		}
	} while (src && src != prev);
	return prev;
}

/*==========================================
 * Checks the state between two targets
 * (enemy, friend, party, guild, etc)
 *------------------------------------------
 * Usage:
 * See battle.hpp for possible values/combinations
 * to be used here (BCT_* constants)
 * Return value is:
 * 1: flag holds true (is enemy, party, etc)
 * -1: flag fails
 * 0: Invalid target (non-targetable ever)
 *
 * Credits:
 *	Original coder unknown
 *	Rewritten by Skotlex
*/
int battle_check_target( struct block_list *src, struct block_list *target,int flag)
{
	int16 m; //map
	int state = 0; //Initial state none
	int strip_enemy = 1; //Flag which marks whether to remove the BCT_ENEMY status if it's also friend/ally.
	struct block_list *s_bl = src, *t_bl = target;
	struct unit_data *ud = NULL;

	nullpo_ret(src);
	nullpo_ret(target);

	ud = unit_bl2ud(target);
	m = target->m;

	//t_bl/s_bl hold the 'master' of the attack, while src/target are the actual
	//objects involved.
	if( (t_bl = battle_get_master(target)) == NULL )
		t_bl = target;

	if( (s_bl = battle_get_master(src)) == NULL )
		s_bl = src;

	if ( s_bl->type == BL_PC ) {
		switch( t_bl->type ) {
			case BL_MOB: // Source => PC, Target => MOB
				if ( pc_has_permission((TBL_PC*)s_bl, PC_PERM_DISABLE_PVM) )
					return 0;
				break;
			case BL_PC:
				if (pc_has_permission((TBL_PC*)s_bl, PC_PERM_DISABLE_PVP))
					return 0;
				break;
			default:/* anything else goes */
				break;
		}
	}

	struct map_data *mapdata = map_getmapdata(m);

	switch( target->type ) { // Checks on actual target
		case BL_PC: {
				struct status_change* sc = status_get_sc(src);

				if (((TBL_PC*)target)->invincible_timer != INVALID_TIMER || pc_isinvisible((TBL_PC*)target))
					return -1; //Cannot be targeted yet.
				
			}
			break;
		case BL_MOB:
		{
			struct mob_data *md = ((TBL_MOB*)target);

			if (ud && ud->immune_attack)
				return 0;
			if(((md->special_state.ai == AI_SPHERE || //Marine Spheres
				(md->special_state.ai == AI_FLORA && battle_config.summon_flora&1)) && s_bl->type == BL_PC && src->type != BL_MOB) || //Floras
				(md->special_state.ai == AI_ZANZOU && t_bl->id != s_bl->id) || //Zanzou
				(md->special_state.ai == AI_FAW && (t_bl->id != s_bl->id || (s_bl->type == BL_PC && src->type != BL_MOB)))
			){	//Targettable by players
				state |= BCT_ENEMY;
				strip_enemy = 0;
			}
			break;
		}
		case BL_SKILL:
		{
			TBL_SKILL *su = (TBL_SKILL*)target;
			uint16 skill_id = battle_getcurrentskill(src);
			if( !su || !su->group)
				return 0;
			if( skill_get_inf2(su->group->skill_id, INF2_ISTRAP) && su->group->unit_id != UNT_USED_TRAPS) {
				if (!skill_id) {
					;
				}
				else if (skill_get_inf2(skill_id, INF2_TARGETTRAP)) { // Only a few skills can target traps
					switch (skill_id) {
						case SK_CM_DRAGONBREATH:
						case SK_BS_AXETORNADO:
							// Can only hit traps in PVP/GVG maps
							if (!mapdata->flag[MF_PVP] && !mapdata->flag[MF_GVG])
								return 0;
							break;
					}
				}
				else
					return 0;
				state |= BCT_ENEMY;
				strip_enemy = 0;
			} else if (su->group->skill_id == SK_MG_ICEWALL || (su->group->skill_id == SK_AM_WILDTHORNS && skill_id != SK_BS_CARTCANNON)) {
				switch (skill_id) {
		
					case SK_CM_DRAGONBREATH:
					case SK_BS_AXETORNADO:
						// Can only hit icewall in PVP/GVG maps
						if (!mapdata->flag[MF_PVP] && !mapdata->flag[MF_GVG])
							return 0;
						break;
					
					default:
						// Usually BCT_ALL stands for only hitting chars, but skills specifically set to hit traps also hit icewall
						if ((flag&BCT_ALL) == BCT_ALL && !skill_get_inf2(skill_id, INF2_TARGETTRAP))
							return -1;
				}
				state |= BCT_ENEMY;
				strip_enemy = 0;
			} else	//Excepting traps, Icewall, and Wall of Thorns, you should not be able to target skills.
				return 0;
		}
			break;
		case BL_MER:
		case BL_HOM:
		case BL_ELEM:
			if (ud && ud->immune_attack)
				return 0;
			break;
		//All else not specified is an invalid target.
		default:
			return 0;
	} //end switch actual target

	switch( t_bl->type ) { //Checks on target master
		case BL_PC: {
			struct map_session_data *sd;
			struct status_change *sc = NULL;

			if( t_bl == s_bl )
				break;

			sd = BL_CAST(BL_PC, t_bl);
			sc = status_get_sc(t_bl);

			if( ((sd->state.block_action & PCBLOCK_IMMUNE) || (sc->data[STATUS_MILLENIUMSHIELDS] && s_bl->type != BL_PC)) && flag&BCT_ENEMY )
				return 0; // Global immunity only to Attacks
			if( sd->status.karma && s_bl->type == BL_PC && ((TBL_PC*)s_bl)->status.karma )
				state |= BCT_ENEMY; // Characters with bad karma may fight amongst them
			if( sd->state.killable ) {
				state |= BCT_ENEMY; // Everything can kill it
				strip_enemy = 0;
			}
			break;
		}
		case BL_MOB:
		{
			struct mob_data *md = BL_CAST(BL_MOB, t_bl);

			if( md->guardian_data && md->guardian_data->guild_id && !mapdata_flag_gvg(mapdata) )
				return 0; // Disable guardians/emperiums owned by Guilds on non-woe times.
			break;
		}
		default: break; //other type doesn't have slave yet
    } //end switch master target

	switch( src->type ) { //Checks on actual src type
		case BL_PET:
			if (t_bl->type != BL_MOB && flag&BCT_ENEMY)
				return 0; //Pet may not attack non-mobs.
			if (t_bl->type == BL_MOB && flag & BCT_ENEMY) {
				mob_data *md = BL_CAST(BL_MOB, t_bl);

				if (md->guardian_data || md->special_state.ai == AI_GUILD)
					return 0; //pet may not attack Guardians/Emperium
			}
			break;
		case BL_SKILL: {
				struct skill_unit *su = (struct skill_unit *)src;
				struct status_change* sc = status_get_sc(target);
				if (!su || !su->group)
					return 0;

				std::bitset<INF2_MAX> inf2 = skill_db.find(su->group->skill_id)->inf2;

				if (su->group->src_id == target->id) {
					if (inf2[INF2_NOTARGETSELF])
						return -1;
					if (inf2[INF2_TARGETSELF])
						return 1;
				}
			}
			break;
		case BL_MER:
			if (t_bl->type == BL_MOB && ((TBL_MOB*)t_bl)->mob_id == MOBID_EMPERIUM && flag&BCT_ENEMY)
				return 0; //mercenary may not attack Emperium
			break;
    } //end switch actual src

	switch( s_bl->type )
	{	//Checks on source master
		case BL_PC:
		{
			struct map_session_data *sd = BL_CAST(BL_PC, s_bl);
			if( s_bl != t_bl )
			{
				if( sd->state.killer )
				{
					state |= BCT_ENEMY; // Can kill anything
					strip_enemy = 0;
				}
				else if( sd->duel_group && !((!battle_config.duel_allow_pvp && mapdata->flag[MF_PVP]) || (!battle_config.duel_allow_gvg && mapdata_flag_gvg(mapdata))) )
				{
					if( t_bl->type == BL_PC && (sd->duel_group == ((TBL_PC*)t_bl)->duel_group) )
						return (BCT_ENEMY&flag)?1:-1; // Duel targets can ONLY be your enemy, nothing else.
					else
						return 0; // You can't target anything out of your duel
				}
			}
			if( !sd->status.guild_id && t_bl->type == BL_MOB && ((TBL_MOB*)t_bl)->mob_id == MOBID_EMPERIUM && mapdata_flag_gvg(mapdata) )
				return 0; //If you don't belong to a guild, can't target emperium.
			if( t_bl->type != BL_PC )
				state |= BCT_ENEMY; //Natural enemy.
			break;
		}
		case BL_MOB:
		{
			struct mob_data *md = BL_CAST(BL_MOB, s_bl);
			if( md->guardian_data && md->guardian_data->guild_id && !mapdata_flag_gvg(mapdata) )
				return 0; // Disable guardians/emperium owned by Guilds on non-woe times.

			if( !md->special_state.ai )
			{ //Normal mobs
				if(
					( target->type == BL_MOB && t_bl->type == BL_PC && ( ((TBL_MOB*)target)->special_state.ai != AI_ZANZOU && ((TBL_MOB*)target)->special_state.ai != AI_ATTACK ) ) ||
					( t_bl->type == BL_MOB && !((TBL_MOB*)t_bl)->special_state.ai )
				  )
					state |= BCT_PARTY; //Normal mobs with no ai are friends.
				else
					state |= BCT_ENEMY; //However, all else are enemies.
			}
			else
			{
				if( t_bl->type == BL_MOB && !((TBL_MOB*)t_bl)->special_state.ai )
					state |= BCT_ENEMY; //Natural enemy for AI mobs are normal mobs.
			}
			break;
		}
		default:
		//Need some sort of default behaviour for unhandled types.
			if (t_bl->type != s_bl->type)
				state |= BCT_ENEMY;
			break;
    } //end switch on src master

	if( (flag&BCT_ALL) == BCT_ALL )
	{ //All actually stands for all attackable chars, icewall and traps
		if(target->type&(BL_CHAR|BL_SKILL))
			return 1;
		else
			return -1;
	}
	if( flag == BCT_NOONE ) //Why would someone use this? no clue.
		return -1;

	if( t_bl == s_bl )
	{ //No need for further testing.
		state |= BCT_SELF|BCT_PARTY|BCT_GUILD;
		if( state&BCT_ENEMY && strip_enemy )
			state&=~BCT_ENEMY;
		return (flag&state)?1:-1;
	}

	if( mapdata_flag_vs(mapdata) )
	{ //Check rivalry settings.
		int sbg_id = 0, tbg_id = 0;
		
		if( flag&(BCT_PARTY|BCT_ENEMY) )
		{
			int s_party = status_get_party_id(s_bl);
			if( s_party && s_party == status_get_party_id(t_bl) && !(mapdata->flag[MF_PVP] && mapdata->flag[MF_PVP_NOPARTY]) && !(mapdata_flag_gvg(mapdata) && mapdata->flag[MF_GVG_NOPARTY]) && (!mapdata->flag[MF_BATTLEGROUND] || sbg_id == tbg_id) )
				state |= BCT_PARTY;
			else
				state |= BCT_ENEMY;
		}
		if( flag&(BCT_GUILD|BCT_ENEMY) )
		{
			int s_guild = status_get_guild_id(s_bl);
			int t_guild = status_get_guild_id(t_bl);
			if( !(mapdata->flag[MF_PVP] && mapdata->flag[MF_PVP_NOGUILD]) && s_guild && t_guild && (s_guild == t_guild || (!(flag&BCT_SAMEGUILD) && guild_isallied(s_guild, t_guild))) && (!mapdata->flag[MF_BATTLEGROUND] || sbg_id == tbg_id) )
				state |= BCT_GUILD;
			else
				state |= BCT_ENEMY;
		}
		if( state&BCT_ENEMY && mapdata->flag[MF_BATTLEGROUND] && sbg_id && sbg_id == tbg_id )
			state &= ~BCT_ENEMY;

		if( state&BCT_ENEMY && battle_config.pk_mode && !mapdata_flag_gvg(mapdata) && s_bl->type == BL_PC && t_bl->type == BL_PC )
		{ // Prevent novice engagement on pk_mode (feature by Valaris)
			TBL_PC *sd = (TBL_PC*)s_bl, *sd2 = (TBL_PC*)t_bl;
			if (
				(sd->class_&MAPID_UPPERMASK) == MAPID_NOVICE ||
				(sd2->class_&MAPID_UPPERMASK) == MAPID_NOVICE ||
				(int)sd->status.base_level < battle_config.pk_min_level ||
			  	(int)sd2->status.base_level < battle_config.pk_min_level ||
				(battle_config.pk_level_range && abs((int)sd->status.base_level - (int)sd2->status.base_level) > battle_config.pk_level_range)
			)
				state &= ~BCT_ENEMY;
		}
	}//end map_flag_vs chk rivality
	else
	{ //Non pvp/gvg, check party/guild settings.
		if( flag&BCT_PARTY || state&BCT_ENEMY )
		{
			int s_party = status_get_party_id(s_bl);
			if(s_party && s_party == status_get_party_id(t_bl))
				state |= BCT_PARTY;
		}
		if( flag&BCT_GUILD || state&BCT_ENEMY )
		{
			int s_guild = status_get_guild_id(s_bl);
			int t_guild = status_get_guild_id(t_bl);
			if(s_guild && t_guild && (s_guild == t_guild || (!(flag&BCT_SAMEGUILD) && guild_isallied(s_guild, t_guild))))
				state |= BCT_GUILD;
		}
	} //end non pvp/gvg chk rivality

	if( !state ) //If not an enemy, nor a guild, nor party, nor yourself, it's neutral.
		state = BCT_NEUTRAL;
	//Alliance state takes precedence over enemy one.
	else if( state&BCT_ENEMY && strip_enemy && state&(BCT_SELF|BCT_PARTY|BCT_GUILD) )
		state&=~BCT_ENEMY;

	return (flag&state)?1:-1;
}
/*==========================================
 * Check if can attack from this range
 * Basic check then calling path_search for obstacle etc..
 *------------------------------------------
 */
bool battle_check_range(struct block_list *src, struct block_list *bl, int range)
{
	int d;
	nullpo_retr(false, src);
	nullpo_retr(false, bl);

	if( src->m != bl->m )
		return false;

#ifndef CIRCULAR_AREA
	if( src->type == BL_PC ) { // Range for players' attacks and skills should always have a circular check. [Angezerus]
		if ( !check_distance_client_bl(src, bl, range) )
			return false;
	} else
#endif
	if( !check_distance_bl(src, bl, range) )
		return false;

	if( (d = distance_bl(src, bl)) < 2 )
		return true;  // No need for path checking.

	if( d > AREA_SIZE )
		return false; // Avoid targetting objects beyond your range of sight.

	return path_search_long(NULL,src->m,src->x,src->y,bl->x,bl->y,CELL_CHKWALL);
}

/*=============================================
 * Battle.conf settings and default/max values
 *---------------------------------------------
 */
static const struct _battle_data {
	const char* str;
	int* val;
	int defval;
	int min;
	int max;
} battle_data[] = {
	{ "warp_point_debug",                   &battle_config.warp_point_debug,                0,      0,      1,              },
	{ "enable_critical",                    &battle_config.enable_critical,                 BL_PC,  BL_NUL, BL_ALL,         },
	{ "mob_critical_rate",                  &battle_config.mob_critical_rate,               100,    0,      INT_MAX,        },
	{ "critical_rate",                      &battle_config.critical_rate,                   100,    0,      INT_MAX,        },
	{ "enable_baseatk",                     &battle_config.enable_baseatk,                  BL_CHAR|BL_HOM, BL_NUL, BL_ALL, },
	{ "enable_baseatk_renewal",             &battle_config.enable_baseatk_renewal,          BL_ALL, BL_NUL, BL_ALL,         },
	{ "enable_perfect_flee",                &battle_config.enable_perfect_flee,             BL_PC|BL_PET, BL_NUL, BL_ALL,   },
	{ "casting_rate",                       &battle_config.cast_rate,                       100,    0,      INT_MAX,        },
	{ "delay_rate",                         &battle_config.delay_rate,                      100,    0,      INT_MAX,        },
	{ "delay_dependon_dex",                 &battle_config.delay_dependon_dex,              0,      0,      1,              },
	{ "delay_dependon_agi",                 &battle_config.delay_dependon_agi,              0,      0,      1,              },
	{ "skill_delay_attack_enable",          &battle_config.sdelay_attack_enable,            0,      0,      1,              },
	{ "left_cardfix_to_right",              &battle_config.left_cardfix_to_right,           0,      0,      1,              },
	{ "skill_add_range",                    &battle_config.skill_add_range,                 0,      0,      INT_MAX,        },
	{ "skill_out_range_consume",            &battle_config.skill_out_range_consume,         1,      0,      1,              },
	{ "skillrange_by_distance",             &battle_config.skillrange_by_distance,          ~BL_PC, BL_NUL, BL_ALL,         },
	{ "skillrange_from_weapon",             &battle_config.use_weapon_skill_range,          BL_NUL, BL_NUL, BL_ALL,         },
	{ "player_damage_delay_rate",           &battle_config.pc_damage_delay_rate,            100,    0,      INT_MAX,        },
	{ "defunit_not_enemy",                  &battle_config.defnotenemy,                     0,      0,      1,              },
	{ "gvg_traps_target_all",               &battle_config.vs_traps_bctall,                 BL_PC,  BL_NUL, BL_ALL,         },
#ifdef RENEWAL
	{ "traps_setting",                      &battle_config.traps_setting,                   2,      0,      2,              },
#else
	{ "traps_setting",                      &battle_config.traps_setting,                   0,      0,      2,              },
#endif
	{ "summon_flora_setting",               &battle_config.summon_flora,                    1|2,    0,      1|2,            },
	{ "clear_skills_on_death",              &battle_config.clear_unit_ondeath,              BL_NUL, BL_NUL, BL_ALL,         },
	{ "clear_skills_on_warp",               &battle_config.clear_unit_onwarp,               BL_ALL, BL_NUL, BL_ALL,         },
	{ "random_monster_checklv",             &battle_config.random_monster_checklv,          0,      0,      1,              },
	{ "attribute_recover",                  &battle_config.attr_recover,                    1,      0,      1,              },
	{ "flooritem_lifetime",                 &battle_config.flooritem_lifetime,              60000,  1000,   INT_MAX,        },
	{ "item_auto_get",                      &battle_config.item_auto_get,                   0,      0,      1,              },
	{ "item_first_get_time",                &battle_config.item_first_get_time,             3000,   0,      INT_MAX,        },
	{ "item_second_get_time",               &battle_config.item_second_get_time,            1000,   0,      INT_MAX,        },
	{ "item_third_get_time",                &battle_config.item_third_get_time,             1000,   0,      INT_MAX,        },
	{ "mvp_item_first_get_time",            &battle_config.mvp_item_first_get_time,         10000,  0,      INT_MAX,        },
	{ "mvp_item_second_get_time",           &battle_config.mvp_item_second_get_time,        10000,  0,      INT_MAX,        },
	{ "mvp_item_third_get_time",            &battle_config.mvp_item_third_get_time,         2000,   0,      INT_MAX,        },
	{ "drop_rate0item",                     &battle_config.drop_rate0item,                  0,      0,      1,              },
	{ "base_exp_rate",                      &battle_config.base_exp_rate,                   100,    0,      INT_MAX,        },
	{ "job_exp_rate",                       &battle_config.job_exp_rate,                    100,    0,      INT_MAX,        },
	{ "pvp_exp",                            &battle_config.pvp_exp,                         1,      0,      1,              },
	{ "death_penalty_type",                 &battle_config.death_penalty_type,              0,      0,      2,              },
	{ "death_penalty_base",                 &battle_config.death_penalty_base,              0,      0,      INT_MAX,        },
	{ "death_penalty_job",                  &battle_config.death_penalty_job,               0,      0,      INT_MAX,        },
	{ "zeny_penalty",                       &battle_config.zeny_penalty,                    0,      0,      INT_MAX,        },
	{ "hp_rate",                            &battle_config.hp_rate,                         100,    1,      INT_MAX,        },
	{ "sp_rate",                            &battle_config.sp_rate,                         100,    1,      INT_MAX,        },
	{ "restart_hp_rate",                    &battle_config.restart_hp_rate,                 0,      0,      100,            },
	{ "restart_sp_rate",                    &battle_config.restart_sp_rate,                 0,      0,      100,            },
	{ "guild_aura",                         &battle_config.guild_aura,                      31,     0,      31,             },
	{ "mvp_hp_rate",                        &battle_config.mvp_hp_rate,                     100,    1,      INT_MAX,        },
	{ "mvp_exp_rate",                       &battle_config.mvp_exp_rate,                    100,    0,      INT_MAX,        },
	{ "monster_hp_rate",                    &battle_config.monster_hp_rate,                 100,    1,      INT_MAX,        },
	{ "monster_max_aspd",                   &battle_config.monster_max_aspd,                199,    100,    199,            },
	{ "view_range_rate",                    &battle_config.view_range_rate,                 100,    0,      INT_MAX,        },
	{ "chase_range_rate",                   &battle_config.chase_range_rate,                100,    0,      INT_MAX,        },
	{ "gtb_sc_immunity",                    &battle_config.gtb_sc_immunity,                 50,     0,      INT_MAX,        },
	{ "guild_max_castles",                  &battle_config.guild_max_castles,               0,      0,      INT_MAX,        },
	{ "guild_skill_relog_delay",            &battle_config.guild_skill_relog_delay,         300000, 0,      INT_MAX,        },
	{ "emergency_call",                     &battle_config.emergency_call,                  11,     0,      31,             },
	{ "atcommand_spawn_quantity_limit",     &battle_config.atc_spawn_quantity_limit,        100,    0,      INT_MAX,        },
	{ "atcommand_slave_clone_limit",        &battle_config.atc_slave_clone_limit,           25,     0,      INT_MAX,        },
	{ "partial_name_scan",                  &battle_config.partial_name_scan,               0,      0,      1,              },
	{ "player_skillfree",                   &battle_config.skillfree,                       0,      0,      1,              },
	{ "player_skillup_limit",               &battle_config.skillup_limit,                   1,      0,      1,              },
	{ "weapon_produce_rate",                &battle_config.wp_rate,                         100,    0,      INT_MAX,        },
	{ "potion_produce_rate",                &battle_config.pp_rate,                         100,    0,      INT_MAX,        },
	{ "monster_active_enable",              &battle_config.monster_active_enable,           1,      0,      1,              },
	{ "monster_damage_delay_rate",          &battle_config.monster_damage_delay_rate,       100,    0,      INT_MAX,        },
	{ "monster_loot_type",                  &battle_config.monster_loot_type,               0,      0,      1,              },
//	{ "mob_skill_use",                      &battle_config.mob_skill_use,                   1,      0,      1,              }, //Deprecated
	{ "mob_skill_rate",                     &battle_config.mob_skill_rate,                  100,    0,      INT_MAX,        },
	{ "mob_skill_delay",                    &battle_config.mob_skill_delay,                 100,    0,      INT_MAX,        },
	{ "mob_count_rate",                     &battle_config.mob_count_rate,                  100,    0,      INT_MAX,        },
	{ "mob_spawn_delay",                    &battle_config.mob_spawn_delay,                 100,    0,      INT_MAX,        },
	{ "plant_spawn_delay",                  &battle_config.plant_spawn_delay,               100,    0,      INT_MAX,        },
	{ "boss_spawn_delay",                   &battle_config.boss_spawn_delay,                100,    0,      INT_MAX,        },
	{ "no_spawn_on_player",                 &battle_config.no_spawn_on_player,              0,      0,      100,            },
	{ "force_random_spawn",                 &battle_config.force_random_spawn,              0,      0,      1,              },
	{ "slaves_inherit_mode",                &battle_config.slaves_inherit_mode,             4,      0,      4,              },
	{ "slaves_inherit_speed",               &battle_config.slaves_inherit_speed,            3,      0,      3,              },
	{ "summons_trigger_autospells",         &battle_config.summons_trigger_autospells,      1,      0,      1,              },
	{ "pc_damage_walk_delay_rate",          &battle_config.pc_walk_delay_rate,              20,     0,      INT_MAX,        },
	{ "damage_walk_delay_rate",             &battle_config.walk_delay_rate,                 100,    0,      INT_MAX,        },
	{ "multihit_delay",                     &battle_config.multihit_delay,                  80,     0,      INT_MAX,        },
	{ "quest_skill_learn",                  &battle_config.quest_skill_learn,               0,      0,      1,              },
	{ "quest_skill_reset",                  &battle_config.quest_skill_reset,               0,      0,      1,              },
	{ "basic_skill_check",                  &battle_config.basic_skill_check,               1,      0,      1,              },
	{ "guild_emperium_check",               &battle_config.guild_emperium_check,            1,      0,      1,              },
	{ "guild_exp_limit",                    &battle_config.guild_exp_limit,                 50,     0,      99,             },
	{ "player_invincible_time",             &battle_config.pc_invincible_time,              5000,   0,      INT_MAX,        },
	{ "pet_catch_rate",                     &battle_config.pet_catch_rate,                  100,    0,      INT_MAX,        },
	{ "pet_rename",                         &battle_config.pet_rename,                      0,      0,      1,              },
	{ "pet_friendly_rate",                  &battle_config.pet_friendly_rate,               100,    0,      INT_MAX,        },
	{ "pet_hungry_delay_rate",              &battle_config.pet_hungry_delay_rate,           100,    10,     INT_MAX,        },
	{ "pet_hungry_friendly_decrease",       &battle_config.pet_hungry_friendly_decrease,    5,      0,      INT_MAX,        },
	{ "pet_status_support",                 &battle_config.pet_status_support,              0,      0,      1,              },
	{ "pet_attack_support",                 &battle_config.pet_attack_support,              0,      0,      1,              },
	{ "pet_damage_support",                 &battle_config.pet_damage_support,              0,      0,      1,              },
	{ "pet_support_min_friendly",           &battle_config.pet_support_min_friendly,        900,    0,      950,            },
	{ "pet_support_rate",                   &battle_config.pet_support_rate,                100,    0,      INT_MAX,        },
	{ "pet_attack_exp_to_master",           &battle_config.pet_attack_exp_to_master,        0,      0,      1,              },
	{ "pet_attack_exp_rate",                &battle_config.pet_attack_exp_rate,             100,    0,      INT_MAX,        },
	{ "pet_lv_rate",                        &battle_config.pet_lv_rate,                     0,      0,      INT_MAX,        },
	{ "pet_max_stats",                      &battle_config.pet_max_stats,                   99,     0,      INT_MAX,        },
	{ "pet_max_atk1",                       &battle_config.pet_max_atk1,                    750,    0,      INT_MAX,        },
	{ "pet_max_atk2",                       &battle_config.pet_max_atk2,                    1000,   0,      INT_MAX,        },
	{ "pet_disable_in_gvg",                 &battle_config.pet_no_gvg,                      0,      0,      1,              },
	{ "pet_master_dead",                    &battle_config.pet_master_dead,                 0,      0,      1,              },
	{ "skill_min_damage",                   &battle_config.skill_min_damage,                2|4,    0,      1|2|4,          },
	{ "finger_offensive_type",              &battle_config.finger_offensive_type,           0,      0,      1,              },
	{ "heal_exp",                           &battle_config.heal_exp,                        0,      0,      INT_MAX,        },
	{ "resurrection_exp",                   &battle_config.resurrection_exp,                0,      0,      INT_MAX,        },
	{ "shop_exp",                           &battle_config.shop_exp,                        0,      0,      INT_MAX,        },
	{ "max_heal_lv",                        &battle_config.max_heal_lv,                     11,     1,      INT_MAX,        },
	{ "max_heal",                           &battle_config.max_heal,                        9999,   0,      INT_MAX,        },
	{ "combo_delay_rate",                   &battle_config.combo_delay_rate,                100,    0,      INT_MAX,        },
	{ "item_check",                         &battle_config.item_check,                      0x0,    0x0,    0x7,            },
	{ "item_use_interval",                  &battle_config.item_use_interval,               100,    0,      INT_MAX,        },
	{ "cashfood_use_interval",              &battle_config.cashfood_use_interval,           60000,  0,      INT_MAX,        },
	{ "wedding_modifydisplay",              &battle_config.wedding_modifydisplay,           0,      0,      1,              },
	{ "wedding_ignorepalette",              &battle_config.wedding_ignorepalette,           0,      0,      1,              },
	{ "xmas_ignorepalette",                 &battle_config.xmas_ignorepalette,              0,      0,      1,              },
	{ "summer_ignorepalette",               &battle_config.summer_ignorepalette,            0,      0,      1,              },
	{ "hanbok_ignorepalette",               &battle_config.hanbok_ignorepalette,            0,      0,      1,              },
	{ "oktoberfest_ignorepalette",          &battle_config.oktoberfest_ignorepalette,       0,      0,      1,              },
	{ "natural_heal_hp_interval",            &battle_config.natural_heal_hp_interval,         6000,   NATURSK_AL_HEAL_INTERVAL, INT_MAX, },
	{ "natural_heal_sp_interval",            &battle_config.natural_heal_sp_interval,         8000,   NATURSK_AL_HEAL_INTERVAL, INT_MAX, },
	{ "natural_heal_skill_interval",        &battle_config.natural_heal_skill_interval,     10000,  NATURSK_AL_HEAL_INTERVAL, INT_MAX, },
	{ "natural_heal_weight_rate",           &battle_config.natural_heal_weight_rate,        50,     0,      100             },
	{ "natural_heal_weight_rate_renewal",   &battle_config.natural_heal_weight_rate_renewal,70,     0,      100             },
	{ "arrow_decrement",                    &battle_config.arrow_decrement,                 1,      0,      2,              },
	{ "ammo_unequip",                       &battle_config.ammo_unequip,                    1,      0,      1,              },
	{ "ammo_check_weapon",                  &battle_config.ammo_check_weapon,               1,      0,      1,              },
	{ "max_aspd",                           &battle_config.max_aspd,                        190,    100,    199,            },
	{ "max_third_aspd",                     &battle_config.max_third_aspd,                  193,    100,    199,            },
	{ "max_summoner_aspd",                  &battle_config.max_summoner_aspd,               193,    100,    199,            },
	{ "max_walk_speed",                     &battle_config.max_walk_speed,                  300,    100,    100*DEFAULT_WALK_SPEED, },
	{ "max_lv",                             &battle_config.max_lv,                          99,     0,      MAX_LEVEL,      },
	{ "aura_lv",                            &battle_config.aura_lv,                         99,     0,      INT_MAX,        },
	{ "max_hp_lv99",                        &battle_config.max_hp_lv99,                    330000,  100,    1000000000,     },
	{ "max_hp_lv150",                       &battle_config.max_hp_lv150,                   660000,  100,    1000000000,     },
	{ "max_hp",                             &battle_config.max_hp,                        1100000,  100,    1000000000,     },
	{ "max_sp",                             &battle_config.max_sp,                          32500,  100,    1000000000,     },
	{ "max_cart_weight",                    &battle_config.max_cart_weight,                 8000,   100,    1000000,        },
	{ "max_parameter",                      &battle_config.max_parameter,                   99,     10,     SHRT_MAX,       },
	{ "max_baby_parameter",                 &battle_config.max_baby_parameter,              80,     10,     SHRT_MAX,       },
	{ "max_def",                            &battle_config.max_def,                         99,     0,      INT_MAX,        },
	{ "over_def_bonus",                     &battle_config.over_def_bonus,                  0,      0,      1000,           },
	{ "skill_log",                          &battle_config.skill_log,                       BL_NUL, BL_NUL, BL_ALL,         },
	{ "battle_log",                         &battle_config.battle_log,                      0,      0,      1,              },
	{ "etc_log",                            &battle_config.etc_log,                         1,      0,      1,              },
	{ "save_clothcolor",                    &battle_config.save_clothcolor,                 1,      0,      1,              },
	{ "undead_detect_type",                 &battle_config.undead_detect_type,              0,      0,      2,              },
	{ "auto_counter_type",                  &battle_config.auto_counter_type,               BL_ALL, BL_NUL, BL_ALL,         },
	{ "min_hitrate",                        &battle_config.min_hitrate,                     5,      0,      100,            },
	{ "max_hitrate",                        &battle_config.max_hitrate,                     100,    0,      100,            },
	{ "agi_penalty_target",                 &battle_config.agi_penalty_target,              BL_PC,  BL_NUL, BL_ALL,         },
	{ "agi_penalty_type",                   &battle_config.agi_penalty_type,                1,      0,      2,              },
	{ "agi_penalty_count",                  &battle_config.agi_penalty_count,               3,      2,      INT_MAX,        },
	{ "agi_penalty_num",                    &battle_config.agi_penalty_num,                 10,     0,      INT_MAX,        },
	{ "vit_penalty_target",                 &battle_config.vit_penalty_target,              BL_PC,  BL_NUL, BL_ALL,         },
	{ "vit_penalty_type",                   &battle_config.vit_penalty_type,                1,      0,      2,              },
	{ "vit_penalty_count",                  &battle_config.vit_penalty_count,               3,      2,      INT_MAX,        },
	{ "vit_penalty_num",                    &battle_config.vit_penalty_num,                 5,      1,      INT_MAX,        },
	{ "weapon_defense_type",                &battle_config.weapon_defense_type,             0,      0,      INT_MAX,        },
	{ "magic_defense_type",                 &battle_config.magic_defense_type,              0,      0,      INT_MAX,        },
	{ "skill_reiteration",                  &battle_config.skill_reiteration,               BL_NUL, BL_NUL, BL_ALL,         },
	{ "skill_nofootset",                    &battle_config.skill_nofootset,                 BL_PC,  BL_NUL, BL_ALL,         },
	{ "player_cloak_check_type",            &battle_config.pc_cloak_check_type,             1,      0,      1|2|4,          },
	{ "monster_cloak_check_type",           &battle_config.monster_cloak_check_type,        4,      0,      1|2|4,          },
	{ "sense_type",                         &battle_config.estimation_type,                 1|2,    0,      1|2,            },
	{ "gvg_short_attack_damage_rate",       &battle_config.gvg_short_damage_rate,           80,     0,      INT_MAX,        },
	{ "gvg_long_attack_damage_rate",        &battle_config.gvg_long_damage_rate,            80,     0,      INT_MAX,        },
	{ "gvg_weapon_attack_damage_rate",      &battle_config.gvg_weapon_damage_rate,          60,     0,      INT_MAX,        },
	{ "gvg_magic_attack_damage_rate",       &battle_config.gvg_magic_damage_rate,           60,     0,      INT_MAX,        },
	{ "gvg_misc_attack_damage_rate",        &battle_config.gvg_misc_damage_rate,            60,     0,      INT_MAX,        },
	{ "gvg_flee_penalty",                   &battle_config.gvg_flee_penalty,                20,     0,      INT_MAX,        },
	{ "pk_short_attack_damage_rate",        &battle_config.pk_short_damage_rate,            80,     0,      INT_MAX,        },
	{ "pk_long_attack_damage_rate",         &battle_config.pk_long_damage_rate,             70,     0,      INT_MAX,        },
	{ "pk_weapon_attack_damage_rate",       &battle_config.pk_weapon_damage_rate,           60,     0,      INT_MAX,        },
	{ "pk_magic_attack_damage_rate",        &battle_config.pk_magic_damage_rate,            60,     0,      INT_MAX,        },
	{ "pk_misc_attack_damage_rate",         &battle_config.pk_misc_damage_rate,             60,     0,      INT_MAX,        },
	{ "mob_changetarget_byskill",           &battle_config.mob_changetarget_byskill,        0,      0,      1,              },
	{ "attack_direction_change",            &battle_config.attack_direction_change,         BL_ALL, BL_NUL, BL_ALL,         },
	{ "land_skill_limit",                   &battle_config.land_skill_limit,                BL_ALL, BL_NUL, BL_ALL,         },
	{ "monster_class_change_full_recover",  &battle_config.monster_class_change_recover,    1,      0,      1,              },
	{ "produce_item_name_input",            &battle_config.produce_item_name_input,         0x1|0x2, 0,     0x9F,           },
	{ "display_skill_fail",                 &battle_config.display_skill_fail,              2,      0,      1|2|4|8,        },
	{ "chat_warpportal",                    &battle_config.chat_warpportal,                 0,      0,      1,              },
	{ "mob_warp",                           &battle_config.mob_warp,                        0,      0,      1|2|4|8,          },
	{ "dead_branch_active",                 &battle_config.dead_branch_active,              1,      0,      1,              },
	{ "vending_max_value",                  &battle_config.vending_max_value,               10000000, 1,    MAX_ZENY,       },
	{ "vending_over_max",                   &battle_config.vending_over_max,                1,      0,      1,              },
	{ "show_steal_in_same_party",           &battle_config.show_steal_in_same_party,        0,      0,      1,              },
	{ "party_hp_mode",                      &battle_config.party_hp_mode,                   0,      0,      1,              },
	{ "show_party_share_picker",            &battle_config.party_show_share_picker,         1,      0,      1,              },
	{ "show_picker.item_type",              &battle_config.show_picker_item_type,           112,    0,      INT_MAX,        },
	{ "party_update_interval",              &battle_config.party_update_interval,           1000,   100,    INT_MAX,        },
	{ "party_item_share_type",              &battle_config.party_share_type,                0,      0,      1|2|3,          },
	{ "attack_attr_none",                   &battle_config.attack_attr_none,                ~BL_PC, BL_NUL, BL_ALL,         },
	{ "gx_allhit",                          &battle_config.gx_allhit,                       0,      0,      1,              },
	{ "gx_disptype",                        &battle_config.gx_disptype,                     1,      0,      1,              },
	{ "devotion_level_difference",          &battle_config.devotion_level_difference,       10,     0,      INT_MAX,        },
	{ "player_skill_partner_check",         &battle_config.player_skill_partner_check,      1,      0,      1,              },
	{ "invite_request_check",               &battle_config.invite_request_check,            1,      0,      1,              },
	{ "skill_removetrap_type",              &battle_config.skill_removetrap_type,           0,      0,      1,              },
	{ "disp_experience",                    &battle_config.disp_experience,                 0,      0,      1,              },
	{ "disp_zeny",                          &battle_config.disp_zeny,                       0,      0,      1,              },
	{ "bone_drop",                          &battle_config.bone_drop,                       0,      0,      2,              },
	{ "buyer_name",                         &battle_config.buyer_name,                      1,      0,      1,              },
	{ "skill_wall_check",                   &battle_config.skill_wall_check,                1,      0,      1,              },
	{ "official_cell_stack_limit",          &battle_config.official_cell_stack_limit,       1,      0,      255,            },
	{ "custom_cell_stack_limit",            &battle_config.custom_cell_stack_limit,         1,      1,      255,            },
	{ "dancing_weaponswitch_fix",           &battle_config.dancing_weaponswitch_fix,        1,      0,      1,              },

	// eAthena additions
	{ "item_logarithmic_drops",             &battle_config.logarithmic_drops,               0,      0,      1,              },
	{ "item_drop_common_min",               &battle_config.item_drop_common_min,            1,      0,      10000,          },
	{ "item_drop_common_max",               &battle_config.item_drop_common_max,            10000,  1,      10000,          },
	{ "item_drop_equip_min",                &battle_config.item_drop_equip_min,             1,      0,      10000,          },
	{ "item_drop_equip_max",                &battle_config.item_drop_equip_max,             10000,  1,      10000,          },
	{ "item_drop_card_min",                 &battle_config.item_drop_card_min,              1,      0,      10000,          },
	{ "item_drop_card_max",                 &battle_config.item_drop_card_max,              10000,  1,      10000,          },
	{ "item_drop_mvp_min",                  &battle_config.item_drop_mvp_min,               1,      0,      10000,          },
	{ "item_drop_mvp_max",                  &battle_config.item_drop_mvp_max,               10000,  1,      10000,          },
	{ "item_drop_mvp_mode",                 &battle_config.item_drop_mvp_mode,              0,      0,      2,              },
	{ "item_drop_heal_min",                 &battle_config.item_drop_heal_min,              1,      0,      10000,          },
	{ "item_drop_heal_max",                 &battle_config.item_drop_heal_max,              10000,  1,      10000,          },
	{ "item_drop_use_min",                  &battle_config.item_drop_use_min,               1,      0,      10000,          },
	{ "item_drop_use_max",                  &battle_config.item_drop_use_max,               10000,  1,      10000,          },
	{ "item_drop_add_min",                  &battle_config.item_drop_adddrop_min,           1,      0,      10000,          },
	{ "item_drop_add_max",                  &battle_config.item_drop_adddrop_max,           10000,  1,      10000,          },
	{ "item_drop_treasure_min",             &battle_config.item_drop_treasure_min,          1,      0,      10000,          },
	{ "item_drop_treasure_max",             &battle_config.item_drop_treasure_max,          10000,  1,      10000,          },
	{ "item_rate_mvp",                      &battle_config.item_rate_mvp,                   100,    0,      1000000,        },
	{ "item_rate_common",                   &battle_config.item_rate_common,                100,    0,      1000000,        },
	{ "item_rate_common_boss",              &battle_config.item_rate_common_boss,           100,    0,      1000000,        },
	{ "item_rate_common_mvp",               &battle_config.item_rate_common_mvp,            100,    0,      1000000,        },
	{ "item_rate_equip",                    &battle_config.item_rate_equip,                 100,    0,      1000000,        },
	{ "item_rate_equip_boss",               &battle_config.item_rate_equip_boss,            100,    0,      1000000,        },
	{ "item_rate_equip_mvp",                &battle_config.item_rate_equip_mvp,             100,    0,      1000000,        },
	{ "item_rate_card",                     &battle_config.item_rate_card,                  100,    0,      1000000,        },
	{ "item_rate_card_boss",                &battle_config.item_rate_card_boss,             100,    0,      1000000,        },
	{ "item_rate_card_mvp",                 &battle_config.item_rate_card_mvp,              100,    0,      1000000,        },
	{ "item_rate_heal",                     &battle_config.item_rate_heal,                  100,    0,      1000000,        },
	{ "item_rate_heal_boss",                &battle_config.item_rate_heal_boss,             100,    0,      1000000,        },
	{ "item_rate_heal_mvp",                 &battle_config.item_rate_heal_mvp,              100,    0,      1000000,        },
	{ "item_rate_use",                      &battle_config.item_rate_use,                   100,    0,      1000000,        },
	{ "item_rate_use_boss",                 &battle_config.item_rate_use_boss,              100,    0,      1000000,        },
	{ "item_rate_use_mvp",                  &battle_config.item_rate_use_mvp,               100,    0,      1000000,        },
	{ "item_rate_adddrop",                  &battle_config.item_rate_adddrop,               100,    0,      1000000,        },
	{ "item_rate_treasure",                 &battle_config.item_rate_treasure,              100,    0,      1000000,        },
	{ "prevent_logout",                     &battle_config.prevent_logout,                  10000,  0,      60000,          },
	{ "prevent_logout_trigger",             &battle_config.prevent_logout_trigger,          0xE,    0,      0xF,            },
	{ "alchemist_summon_reward",            &battle_config.alchemist_summon_reward,         1,      0,      2,              },
	{ "drops_by_luk",                       &battle_config.drops_by_luk,                    0,      0,      INT_MAX,        },
	{ "drops_by_luk2",                      &battle_config.drops_by_luk2,                   0,      0,      INT_MAX,        },
	{ "equip_natural_break_rate",           &battle_config.equip_natural_break_rate,        0,      0,      INT_MAX,        },
	{ "equip_self_break_rate",              &battle_config.equip_self_break_rate,           100,    0,      INT_MAX,        },
	{ "equip_skill_break_rate",             &battle_config.equip_skill_break_rate,          100,    0,      INT_MAX,        },
	{ "pk_mode",                            &battle_config.pk_mode,                         0,      0,      2,              },
	{ "pk_mode_mes",                        &battle_config.pk_mode_mes,                     1,      0,      1,              },
	{ "pk_level_range",                     &battle_config.pk_level_range,                  0,      0,      INT_MAX,        },
	{ "manner_system",                      &battle_config.manner_system,                   0xFFF,  0,      0xFFF,          },
	{ "pet_equip_required",                 &battle_config.pet_equip_required,              0,      0,      1,              },
	{ "multi_level_up",                     &battle_config.multi_level_up,                  0,      0,      1,              },
	{ "multi_level_up_base",                &battle_config.multi_level_up_base,             0,      0,      MAX_LEVEL,      },
	{ "multi_level_up_job",                 &battle_config.multi_level_up_job,              0,      0,      MAX_LEVEL,      },
	{ "max_exp_gain_rate",                  &battle_config.max_exp_gain_rate,               0,      0,      INT_MAX,        },
	{ "backstab_bow_penalty",               &battle_config.backstab_bow_penalty,            0,      0,      1,              },
	{ "night_at_start",                     &battle_config.night_at_start,                  0,      0,      1,              },
	{ "show_mob_info",                      &battle_config.show_mob_info,                   0,      0,      1|2|4,          },
	{ "ban_hack_trade",                     &battle_config.ban_hack_trade,                  0,      0,      INT_MAX,        },
	{ "min_hair_style",                     &battle_config.min_hair_style,                  0,      0,      INT_MAX,        },
	{ "max_hair_style",                     &battle_config.max_hair_style,                  23,     0,      INT_MAX,        },
	{ "min_hair_color",                     &battle_config.min_hair_color,                  0,      0,      INT_MAX,        },
	{ "max_hair_color",                     &battle_config.max_hair_color,                  9,      0,      INT_MAX,        },
	{ "min_cloth_color",                    &battle_config.min_cloth_color,                 0,      0,      INT_MAX,        },
	{ "max_cloth_color",                    &battle_config.max_cloth_color,                 4,      0,      INT_MAX,        },
	{ "pet_hair_style",                     &battle_config.pet_hair_style,                  100,    0,      INT_MAX,        },
	{ "castrate_dex_scale",                 &battle_config.castrate_dex_scale,              150,    1,      INT_MAX,        },
	{ "vcast_stat_scale",                   &battle_config.vcast_stat_scale,                530,    1,      INT_MAX,        },
	{ "area_size",                          &battle_config.area_size,                       14,     0,      INT_MAX,        },
	{ "zeny_from_mobs",                     &battle_config.zeny_from_mobs,                  0,      0,      1,              },
	{ "mobs_level_up",                      &battle_config.mobs_level_up,                   0,      0,      1,              },
	{ "mobs_level_up_exp_rate",             &battle_config.mobs_level_up_exp_rate,          1,      1,      INT_MAX,        },
	{ "pk_min_level",                       &battle_config.pk_min_level,                    55,     1,      INT_MAX,        },
	{ "skill_steal_max_tries",              &battle_config.skill_steal_max_tries,           0,      0,      UCHAR_MAX,      },
	{ "motd_type",                          &battle_config.motd_type,                       0,      0,      1,              },
	{ "finding_ore_rate",                   &battle_config.finding_ore_rate,                100,    0,      INT_MAX,        },
	{ "exp_calc_type",                      &battle_config.exp_calc_type,                   0,      0,      1,              },
	{ "exp_bonus_attacker",                 &battle_config.exp_bonus_attacker,              25,     0,      INT_MAX,        },
	{ "exp_bonus_max_attacker",             &battle_config.exp_bonus_max_attacker,          12,     2,      INT_MAX,        },
	{ "min_skill_delay_limit",              &battle_config.min_skill_delay_limit,           100,    10,     INT_MAX,        },
	{ "default_walk_delay",                 &battle_config.default_walk_delay,              300,    0,      INT_MAX,        },
	{ "no_skill_delay",                     &battle_config.no_skill_delay,                  BL_MOB, BL_NUL, BL_ALL,         },
	{ "attack_walk_delay",                  &battle_config.attack_walk_delay,               BL_ALL, BL_NUL, BL_ALL,         },
	{ "require_glory_guild",                &battle_config.require_glory_guild,             0,      0,      1,              },
	{ "idle_no_share",                      &battle_config.idle_no_share,                   0,      0,      INT_MAX,        },
	{ "party_even_share_bonus",             &battle_config.party_even_share_bonus,          0,      0,      INT_MAX,        },
	{ "delay_battle_damage",                &battle_config.delay_battle_damage,             1,      0,      1,              },
	{ "hide_woe_damage",                    &battle_config.hide_woe_damage,                 0,      0,      1,              },
	{ "display_version",                    &battle_config.display_version,                 1,      0,      1,              },
	{ "display_hallucination",              &battle_config.display_hallucination,           1,      0,      1,              },
	{ "use_statpoint_table",                &battle_config.use_statpoint_table,             1,      0,      1,              },
	{ "berserk_cancels_buffs",              &battle_config.berserk_cancels_buffs,           0,      0,      1,              },
	{ "debuff_on_logout",                   &battle_config.debuff_on_logout,                1|2,    0,      1|2,            },
	{ "monster_ai",                         &battle_config.mob_ai,                          0x000,  0x000,  0xFFF,          },
	{ "hom_setting",                        &battle_config.hom_setting,                     0xFFFF, 0x0000, 0xFFFF,         },
	{ "dynamic_mobs",                       &battle_config.dynamic_mobs,                    1,      0,      1,              },
	{ "mob_remove_damaged",                 &battle_config.mob_remove_damaged,              1,      0,      1,              },
	{ "show_hp_sp_drain",                   &battle_config.show_hp_sp_drain,                0,      0,      1,              },
	{ "show_hp_sp_gain",                    &battle_config.show_hp_sp_gain,                 1,      0,      1,              },
	{ "mob_npc_event_type",                 &battle_config.mob_npc_event_type,              1,      0,      1,              },
	{ "character_size",                     &battle_config.character_size,                  1|2,    0,      1|2,            },
	{ "mob_max_skilllvl",                   &battle_config.mob_max_skilllvl,                MAX_MOBSKILL_LEVEL, 1, MAX_MOBSKILL_LEVEL, },
	{ "retaliate_to_master",                &battle_config.retaliate_to_master,             1,      0,      1,              },
	{ "rare_drop_announce",                 &battle_config.rare_drop_announce,              0,      0,      10000,          },
	{ "duel_allow_pvp",                     &battle_config.duel_allow_pvp,                  0,      0,      1,              },
	{ "duel_allow_gvg",                     &battle_config.duel_allow_gvg,                  0,      0,      1,              },
	{ "duel_allow_teleport",                &battle_config.duel_allow_teleport,             0,      0,      1,              },
	{ "duel_autoleave_when_die",            &battle_config.duel_autoleave_when_die,         1,      0,      1,              },
	{ "duel_time_interval",                 &battle_config.duel_time_interval,              60,     0,      INT_MAX,        },
	{ "duel_only_on_same_map",              &battle_config.duel_only_on_same_map,           0,      0,      1,              },
	{ "skip_teleport_lv1_menu",             &battle_config.skip_teleport_lv1_menu,          0,      0,      1,              },
	{ "allow_skill_without_day",            &battle_config.allow_skill_without_day,         0,      0,      1,              },
	{ "allow_es_magic_player",              &battle_config.allow_es_magic_pc,               0,      0,      1,              },
	{ "skill_caster_check",                 &battle_config.skill_caster_check,              1,      0,      1,              },
	{ "status_cast_cancel",                 &battle_config.sc_castcancel,                   BL_NUL, BL_NUL, BL_ALL,         },
	{ "pc_status_def_rate",                 &battle_config.pc_STATUS_DEF_RATE,                  100,    0,      INT_MAX,        },
	{ "mob_status_def_rate",                &battle_config.mob_STATUS_DEF_RATE,                 100,    0,      INT_MAX,        },
	{ "pc_max_status_def",                  &battle_config.pc_max_sc_def,                   100,    0,      INT_MAX,        },
	{ "mob_max_status_def",                 &battle_config.mob_max_sc_def,                  100,    0,      INT_MAX,        },
	{ "sg_miracle_skill_ratio",             &battle_config.sg_miracle_skill_ratio,          1,      0,      10000,          },
	{ "sg_angel_skill_ratio",               &battle_config.sg_angel_skill_ratio,            10,     0,      10000,          },
	{ "autospell_stacking",                 &battle_config.autospell_stacking,              0,      0,      1,              },
	{ "override_mob_names",                 &battle_config.override_mob_names,              0,      0,      2,              },
	{ "min_chat_delay",                     &battle_config.min_chat_delay,                  0,      0,      INT_MAX,        },
	{ "friend_auto_add",                    &battle_config.friend_auto_add,                 1,      0,      1,              },
	{ "hom_rename",                         &battle_config.hom_rename,                      0,      0,      1,              },
	{ "homunculus_show_growth",             &battle_config.homunculus_show_growth,          0,      0,      1,              },
	{ "homunculus_friendly_rate",           &battle_config.homunculus_friendly_rate,        100,    0,      INT_MAX,        },
	{ "vending_tax",                        &battle_config.vending_tax,                     0,      0,      10000,          },
	{ "vending_tax_min",                    &battle_config.vending_tax_min,                 0,      0,      MAX_ZENY,       },
	{ "day_duration",                       &battle_config.day_duration,                    0,      0,      INT_MAX,        },
	{ "night_duration",                     &battle_config.night_duration,                  0,      0,      INT_MAX,        },
	{ "mob_remove_delay",                   &battle_config.mob_remove_delay,                60000,  1000,   INT_MAX,        },
	{ "mob_active_time",                    &battle_config.mob_active_time,                 0,      0,      INT_MAX,        },
	{ "boss_active_time",                   &battle_config.boss_active_time,                0,      0,      INT_MAX,        },
	{ "sg_miracle_skill_duration",          &battle_config.sg_miracle_skill_duration,       3600000, 0,     INT_MAX,        },
	{ "hvan_explosion_intimate",            &battle_config.hvan_explosion_intimate,         45000,  0,      100000,         },
	{ "quest_exp_rate",                     &battle_config.quest_exp_rate,                  100,    0,      INT_MAX,        },
	{ "at_mapflag",                         &battle_config.autotrade_mapflag,               0,      0,      1,              },
	{ "at_timeout",                         &battle_config.at_timeout,                      0,      0,      INT_MAX,        },
	{ "homunculus_autoloot",                &battle_config.homunculus_autoloot,             0,      0,      1,              },
	{ "idle_no_autoloot",                   &battle_config.idle_no_autoloot,                0,      0,      INT_MAX,        },
	{ "max_guild_alliance",                 &battle_config.max_guild_alliance,              3,      0,      3,              },
	{ "ksprotection",                       &battle_config.ksprotection,                    5000,   0,      INT_MAX,        },
	{ "auction_feeperhour",                 &battle_config.auction_feeperhour,              12000,  0,      INT_MAX,        },
	{ "auction_maximumprice",               &battle_config.auction_maximumprice,            500000000, 0,   MAX_ZENY,       },
	{ "homunculus_auto_vapor",              &battle_config.homunculus_auto_vapor,           80,     0,      100,            },
	{ "display_status_timers",              &battle_config.display_status_timers,           1,      0,      1,              },
	{ "skill_add_heal_rate",                &battle_config.skill_add_heal_rate,             7,      0,      INT_MAX,        },
	{ "eq_single_target_reflectable",       &battle_config.eq_single_target_reflectable,    1,      0,      1,              },
	{ "invincible.nodamage",                &battle_config.invincible_nodamage,             0,      0,      1,              },
	{ "mob_slave_keep_target",              &battle_config.mob_slave_keep_target,           0,      0,      1,              },
	{ "autospell_check_range",              &battle_config.autospell_check_range,           0,      0,      1,              },
	{ "knockback_left",                     &battle_config.knockback_left,                  1,      0,      1,              },
	{ "client_reshuffle_dice",              &battle_config.client_reshuffle_dice,           0,      0,      1,              },
	{ "client_sort_storage",                &battle_config.client_sort_storage,             0,      0,      1,              },
	{ "feature.buying_store",               &battle_config.feature_buying_store,            1,      0,      1,              },
	{ "feature.search_stores",              &battle_config.feature_search_stores,           1,      0,      1,              },
	{ "searchstore_querydelay",             &battle_config.searchstore_querydelay,         10,      0,      INT_MAX,        },
	{ "searchstore_maxresults",             &battle_config.searchstore_maxresults,         30,      1,      INT_MAX,        },
	{ "display_party_name",                 &battle_config.display_party_name,              0,      0,      1,              },
	{ "cashshop_show_points",               &battle_config.cashshop_show_points,            0,      0,      1,              },
	{ "mail_show_status",                   &battle_config.mail_show_status,                0,      0,      2,              },
	{ "client_limit_unit_lv",               &battle_config.client_limit_unit_lv,            0,      0,      BL_ALL,         },
	{ "land_protector_behavior",            &battle_config.land_protector_behavior,         0,      0,      1,              },
	{ "npc_emotion_behavior",               &battle_config.npc_emotion_behavior,            0,      0,      1,              },
// BattleGround Settings
	{ "bg_update_interval",                 &battle_config.bg_update_interval,              1000,   100,    INT_MAX,        },
	{ "bg_short_attack_damage_rate",        &battle_config.bg_short_damage_rate,            80,     0,      INT_MAX,        },
	{ "bg_long_attack_damage_rate",         &battle_config.bg_long_damage_rate,             80,     0,      INT_MAX,        },
	{ "bg_weapon_attack_damage_rate",       &battle_config.bg_weapon_damage_rate,           60,     0,      INT_MAX,        },
	{ "bg_magic_attack_damage_rate",        &battle_config.bg_magic_damage_rate,            60,     0,      INT_MAX,        },
	{ "bg_misc_attack_damage_rate",         &battle_config.bg_misc_damage_rate,             60,     0,      INT_MAX,        },
	{ "bg_flee_penalty",                    &battle_config.bg_flee_penalty,                 20,     0,      INT_MAX,        },
// rAthena
	{ "max_third_parameter",				&battle_config.max_third_parameter,				135,	10,		SHRT_MAX,		},
	{ "max_baby_third_parameter",			&battle_config.max_baby_third_parameter,		108,	10,		SHRT_MAX,		},
	{ "max_trans_parameter",				&battle_config.max_trans_parameter,				99,		10,		SHRT_MAX,		},
	{ "max_third_trans_parameter",			&battle_config.max_third_trans_parameter,		135,	10,		SHRT_MAX,		},
	{ "max_extended_parameter",				&battle_config.max_extended_parameter,			125,	10,		SHRT_MAX,		},
	{ "max_summoner_parameter",				&battle_config.max_summoner_parameter,			120,	10,		SHRT_MAX,		},
	{ "skill_amotion_leniency",             &battle_config.skill_amotion_leniency,          0,      0,      300             },
	{ "mvp_tomb_enabled",                   &battle_config.mvp_tomb_enabled,                1,      0,      1               },
	{ "mvp_tomb_delay",                     &battle_config.mvp_tomb_delay,                  9000,   0,      INT_MAX,        },
	{ "feature.atcommand_suggestions",      &battle_config.atcommand_suggestions_enabled,   0,      0,      1               },
	{ "min_npc_vendchat_distance",          &battle_config.min_npc_vendchat_distance,       3,      0,      100             },
	{ "atcommand_mobinfo_type",             &battle_config.atcommand_mobinfo_type,          0,      0,      1               },
	{ "homunculus_max_level",               &battle_config.hom_max_level,                   99,     0,      MAX_LEVEL,      },
	{ "homunculus_S_max_level",             &battle_config.hom_S_max_level,                 150,    0,      MAX_LEVEL,      },
	{ "mob_size_influence",                 &battle_config.mob_size_influence,              0,      0,      1,              },
	{ "skill_trap_type",                    &battle_config.skill_trap_type,                 0,      0,      3,              },
	{ "allow_consume_restricted_item",      &battle_config.allow_consume_restricted_item,   1,      0,      1,              },
	{ "allow_equip_restricted_item",        &battle_config.allow_equip_restricted_item,     1,      0,      1,              },
	{ "max_walk_path",                      &battle_config.max_walk_path,                   17,     1,      MAX_WALKPATH,   },
	{ "item_enabled_npc",                   &battle_config.item_enabled_npc,                1,      0,      1,              },
	{ "item_flooritem_check",               &battle_config.item_onfloor,                    1,      0,      1,              },
	{ "bowling_bash_area",                  &battle_config.bowling_bash_area,               0,      0,      20,             },
	{ "drop_rateincrease",                  &battle_config.drop_rateincrease,               0,      0,      1,              },
	{ "feature.auction",                    &battle_config.feature_auction,                 0,      0,      2,              },
	{ "feature.banking",                    &battle_config.feature_banking,                 1,      0,      1,              },
#ifdef VIP_ENABLE
	{ "vip_storage_increase",               &battle_config.vip_storage_increase,          300,      0,      MAX_STORAGE-MIN_STORAGE, },
#else
	{ "vip_storage_increase",               &battle_config.vip_storage_increase,          300,      0,      MAX_STORAGE, },
#endif
	{ "vip_base_exp_increase",              &battle_config.vip_base_exp_increase,          50,      0,      INT_MAX,        },
	{ "vip_job_exp_increase",               &battle_config.vip_job_exp_increase,           50,      0,      INT_MAX,        },
	{ "vip_exp_penalty_base",               &battle_config.vip_exp_penalty_base,          100,      0,      INT_MAX,        },
	{ "vip_exp_penalty_job",                &battle_config.vip_exp_penalty_job,           100,      0,      INT_MAX,        },
	{ "vip_zeny_penalty",                   &battle_config.vip_zeny_penalty,                0,      0,      INT_MAX,        },
	{ "vip_bm_increase",                    &battle_config.vip_bm_increase,                 2,      0,      INT_MAX,        },
	{ "vip_drop_increase",                  &battle_config.vip_drop_increase,              50,      0,      INT_MAX,        },
	{ "vip_gemstone",                       &battle_config.vip_gemstone,                    2,      0,      2,              },
	{ "vip_disp_rate",                      &battle_config.vip_disp_rate,                   1,      0,      1,              },
	{ "mon_trans_disable_in_gvg",           &battle_config.mon_trans_disable_in_gvg,        0,      0,      1,              },
	{ "homunculus_S_growth_level",          &battle_config.hom_S_growth_level,             99,      0,      MAX_LEVEL,      },
	{ "emblem_woe_change",                  &battle_config.emblem_woe_change,               0,      0,      1,              },
	{ "emblem_transparency_limit",          &battle_config.emblem_transparency_limit,      80,      0,      100,            },
	{ "discount_item_point_shop",			&battle_config.discount_item_point_shop,		0,		0,		3,				},
	{ "update_enemy_position",				&battle_config.update_enemy_position,			0,		0,		1,				},
	{ "devotion_rdamage",					&battle_config.devotion_rdamage,				0,		0,		100,			},
	{ "feature.autotrade",					&battle_config.feature_autotrade,				1,		0,		1,				},
	{ "feature.autotrade_direction",		&battle_config.feature_autotrade_direction,		4,		-1,		7,				},
	{ "feature.autotrade_head_direction",	&battle_config.feature_autotrade_head_direction,0,		-1,		2,				},
	{ "feature.autotrade_sit",				&battle_config.feature_autotrade_sit,			1,		-1,		1,				},
	{ "feature.autotrade_open_delay",		&battle_config.feature_autotrade_open_delay,	5000,	1000,	INT_MAX,		},
	{ "disp_servervip_msg",					&battle_config.disp_servervip_msg,				0,		0,		1,				},
	{ "warg_can_falcon",                    &battle_config.warg_can_falcon,                 0,      0,      1,              },
	{ "path_blown_halt",                    &battle_config.path_blown_halt,                 1,      0,      1,              },
	{ "rental_mount_speed_boost",           &battle_config.rental_mount_speed_boost,        25,     0,      100,        	},
	{ "feature.warp_suggestions",           &battle_config.warp_suggestions_enabled,        0,      0,      1,              },
	{ "taekwon_mission_mobname",            &battle_config.taekwon_mission_mobname,         0,      0,      2,              },
	{ "teleport_on_portal",                 &battle_config.teleport_on_portal,              0,      0,      1,              },
	{ "cart_revo_knockback",                &battle_config.cart_revo_knockback,             1,      0,      1,              },
	{ "guild_notice_changemap",             &battle_config.guild_notice_changemap,          2,      0,      2,              },
	{ "transcendent_status_points",         &battle_config.transcendent_status_points,     52,      1,      INT_MAX,        },
	{ "taekwon_ranker_min_lv",              &battle_config.taekwon_ranker_min_lv,          90,      1,      MAX_LEVEL,      },
	{ "revive_onwarp",                      &battle_config.revive_onwarp,                   1,      0,      1,              },
	{ "fame_taekwon_mission",               &battle_config.fame_taekwon_mission,            1,      0,      INT_MAX,        },
	{ "fame_refine_lv1",                    &battle_config.fame_refine_lv1,                 1,      0,      INT_MAX,        },
	{ "fame_refine_lv1",                    &battle_config.fame_refine_lv1,                 1,      0,      INT_MAX,        },
	{ "fame_refine_lv2",                    &battle_config.fame_refine_lv2,                 25,     0,      INT_MAX,        },
	{ "fame_refine_lv3",                    &battle_config.fame_refine_lv3,                 1000,   0,      INT_MAX,        },
	{ "fame_forge",                         &battle_config.fame_forge,                      10,     0,      INT_MAX,        },
	{ "fame_pharmacy_3",                    &battle_config.fame_pharmacy_3,                 1,      0,      INT_MAX,        },
	{ "fame_pharmacy_5",                    &battle_config.fame_pharmacy_5,                 3,      0,      INT_MAX,        },
	{ "fame_pharmacy_7",                    &battle_config.fame_pharmacy_7,                 10,     0,      INT_MAX,        },
	{ "fame_pharmacy_10",                   &battle_config.fame_pharmacy_10,                50,     0,      INT_MAX,        },
	{ "mail_delay",                         &battle_config.mail_delay,                      1000,   1000,   INT_MAX,        },
	{ "at_monsterignore",                   &battle_config.autotrade_monsterignore,         0,      0,      1,              },
	{ "idletime_option",                    &battle_config.idletime_option,                 0x7C1F, 1,      0xFFFF,         },
	{ "spawn_direction",                    &battle_config.spawn_direction,                 0,      0,      1,              },
	{ "arrow_shower_knockback",             &battle_config.arrow_shower_knockback,          1,      0,      1,              },
	{ "devotion_rdamage_skill_only",        &battle_config.devotion_rdamage_skill_only,     1,      0,      1,              },
	{ "max_extended_aspd",                  &battle_config.max_extended_aspd,               193,    100,    199,            },
	{ "monster_chase_refresh",              &battle_config.mob_chase_refresh,               3,      0,      30,             },
	{ "mob_icewall_walk_block",             &battle_config.mob_icewall_walk_block,          75,     0,      255,            },
	{ "boss_icewall_walk_block",            &battle_config.boss_icewall_walk_block,         0,      0,      255,            },
	{ "snap_dodge",                         &battle_config.snap_dodge,                      0,      0,      1,              },
	{ "stormgust_knockback",                &battle_config.stormgust_knockback,             1,      0,      1,              },
	{ "default_fixed_castrate",             &battle_config.default_fixed_castrate,          20,     0,      100,            },
	{ "default_bind_on_equip",              &battle_config.default_bind_on_equip,           BOUND_CHAR, BOUND_NONE, BOUND_MAX-1, },
	{ "pet_ignore_infinite_def",            &battle_config.pet_ignore_infinite_def,         0,      0,      1,              },
	{ "homunculus_evo_intimacy_need",       &battle_config.homunculus_evo_intimacy_need,    91100,  0,      INT_MAX,        },
	{ "homunculus_evo_intimacy_reset",      &battle_config.homunculus_evo_intimacy_reset,   1000,   0,      INT_MAX,        },
	{ "monster_loot_search_type",           &battle_config.monster_loot_search_type,        1,      0,      1,              },
	{ "feature.roulette",                   &battle_config.feature_roulette,                1,      0,      1,              },
	{ "monster_hp_bars_info",               &battle_config.monster_hp_bars_info,            1,      0,      1,              },
	{ "min_body_style",                     &battle_config.min_body_style,                  0,      0,      SHRT_MAX,       },
	{ "max_body_style",                     &battle_config.max_body_style,                  1,      0,      SHRT_MAX,       },
	{ "save_body_style",                    &battle_config.save_body_style,                 1,      0,      1,              },
	{ "monster_eye_range_bonus",            &battle_config.mob_eye_range_bonus,             0,      0,      10,             },
	{ "monster_stuck_warning",              &battle_config.mob_stuck_warning,               0,      0,      1,              },
	{ "skill_eightpath_algorithm",          &battle_config.skill_eightpath_algorithm,       1,      0,      1,              },
	{ "skill_eightpath_same_cell",          &battle_config.skill_eightpath_same_cell,       1,      0,      1,              },
	{ "death_penalty_maxlv",                &battle_config.death_penalty_maxlv,             0,      0,      3,              },
	{ "exp_cost_redemptio",                 &battle_config.exp_cost_redemptio,              1,      0,      100,            },
	{ "exp_cost_redemptio_limit",           &battle_config.exp_cost_redemptio_limit,        5,      0,      MAX_PARTY,      },
	{ "exp_cost_inspiration",               &battle_config.exp_cost_inspiration,            1,      0,      100,            },
	{ "mvp_exp_reward_message",             &battle_config.mvp_exp_reward_message,          0,      0,      1,              },
	{ "can_damage_skill",                   &battle_config.can_damage_skill,                1,      0,      BL_ALL,         },
	{ "atcommand_levelup_events",			&battle_config.atcommand_levelup_events,		0,		0,		1,				},
	{ "atcommand_disable_npc",				&battle_config.atcommand_disable_npc,			1,		0,		1,				},
	{ "block_account_in_same_party",		&battle_config.block_account_in_same_party,		1,		0,		1,				},
	{ "tarotcard_equal_chance",             &battle_config.tarotcard_equal_chance,          0,      0,      1,              },
	{ "change_party_leader_samemap",        &battle_config.change_party_leader_samemap,     1,      0,      1,              },
	{ "dispel_song",                        &battle_config.dispel_song,                     0,      0,      1,              },
	{ "guild_maprespawn_clones",			&battle_config.guild_maprespawn_clones,			0,		0,		1,				},
	{ "hide_fav_sell", 			&battle_config.hide_fav_sell,			0,      0,      1,              },
	{ "mail_daily_count",					&battle_config.mail_daily_count,				100,	0,		INT32_MAX,		},
	{ "mail_zeny_fee",						&battle_config.mail_zeny_fee,					2,		0,		100,			},
	{ "mail_attachment_price",				&battle_config.mail_attachment_price,			2500,	0,		INT32_MAX,		},
	{ "mail_attachment_weight",				&battle_config.mail_attachment_weight,			2000,	0,		INT32_MAX,		},
	{ "banana_bomb_duration",				&battle_config.banana_bomb_duration,			0,		0,		UINT16_MAX,		},
	{ "guild_leaderchange_delay",			&battle_config.guild_leaderchange_delay,		1440,	0,		INT32_MAX,		},
	{ "guild_leaderchange_woe",				&battle_config.guild_leaderchange_woe,			0,		0,		1,				},
	{ "guild_alliance_onlygm",              &battle_config.guild_alliance_onlygm,           0,      0,      1, },
	{ "feature.achievement",                &battle_config.feature_achievement,             1,      0,      1,              },
	{ "allow_bound_sell",                   &battle_config.allow_bound_sell,                0,      0,      0xF,            },
	{ "event_refine_chance",                &battle_config.event_refine_chance,             0,      0,      1,              },
	{ "autoloot_adjust",                    &battle_config.autoloot_adjust,                 0,      0,      1,              },
	{ "feature.petevolution",               &battle_config.feature_petevolution,            1,      0,      1,              },
	{ "feature.petautofeed",                &battle_config.feature_pet_autofeed,            1,      0,      1,              },
	{ "feature.pet_autofeed_rate",          &battle_config.feature_pet_autofeed_rate,      89,      0,    100,              },
	{ "pet_autofeed_always",                &battle_config.pet_autofeed_always,             1,      0,      1,              },
	{ "broadcast_hide_name",                &battle_config.broadcast_hide_name,             2,      0,      NAME_LENGTH,    },
	{ "skill_drop_items_full",              &battle_config.skill_drop_items_full,           0,      0,      1,              },
	{ "switch_remove_edp",                  &battle_config.switch_remove_edp,               2,      0,      3,              },
	{ "feature.homunculus_autofeed",        &battle_config.feature_homunculus_autofeed,     1,      0,      1,              },
	{ "feature.homunculus_autofeed_rate",   &battle_config.feature_homunculus_autofeed_rate,30,     0,    100,              },
	{ "summoner_race",                      &battle_config.summoner_race,                   RC_PLAYER_DORAM,      RC_FORMLESS,      RC_PLAYER_DORAM,              },
	{ "summoner_size",                      &battle_config.summoner_size,                   SZ_SMALL,                SZ_SMALL,               SZ_BIG,              },
	{ "homunculus_autofeed_always",         &battle_config.homunculus_autofeed_always,      1,      0,      1,              },
	{ "feature.attendance",                 &battle_config.feature_attendance,              1,      0,      1,              },
	{ "feature.privateairship",             &battle_config.feature_privateairship,          1,      0,      1,              },
	{ "rental_transaction",                 &battle_config.rental_transaction,              1,      0,      1,              },
	{ "min_shop_buy",                       &battle_config.min_shop_buy,                    1,      0,      INT_MAX,        },
	{ "min_shop_sell",                      &battle_config.min_shop_sell,                   0,      0,      INT_MAX,        },
	{ "feature.equipswitch",                &battle_config.feature_equipswitch,             1,      0,      1,              },
	{ "pet_walk_speed",                     &battle_config.pet_walk_speed,                  1,      1,      3,              },
	{ "blacksmith_fame_refine_threshold",   &battle_config.blacksmith_fame_refine_threshold,10,     1,      MAX_REFINE,     },
	{ "mob_nopc_idleskill_rate",            &battle_config.mob_nopc_idleskill_rate,         100,    0,    100,              },
	{ "mob_nopc_move_rate",                 &battle_config.mob_nopc_move_rate,              100,    0,    100,              },
	{ "boss_nopc_idleskill_rate",           &battle_config.boss_nopc_idleskill_rate,        100,    0,    100,              },
	{ "boss_nopc_move_rate",                &battle_config.boss_nopc_move_rate,             100,    0,    100,              },
	{ "hom_idle_no_share",                  &battle_config.hom_idle_no_share,               0,      0,      INT_MAX,        },
	{ "idletime_hom_option",                &battle_config.idletime_hom_option,             0x1F,   0x1,    0xFFF,          },
	{ "devotion_standup_fix",               &battle_config.devotion_standup_fix,            1,      0,      1,              },
	{ "feature.bgqueue",                    &battle_config.feature_bgqueue,                 1,      0,      1,              },
	{ "bgqueue_nowarp_mapflag",             &battle_config.bgqueue_nowarp_mapflag,          0,      0,      1,              },
	{ "homunculus_exp_gain",                &battle_config.homunculus_exp_gain,             10,     0,      100,            },
	{ "rental_item_novalue",                &battle_config.rental_item_novalue,             1,      0,      1,              },
	{ "ping_timer_inverval",                &battle_config.ping_timer_interval,             30,     0,      99999999,       },
	{ "ping_time",                          &battle_config.ping_time,                       20,     0,      99999999,       },
	{ "show_skill_scale",                   &battle_config.show_skill_scale,                1,      0,      1,              },
	{ "achievement_mob_share",              &battle_config.achievement_mob_share,           0,      0,      1,              },
	{ "slave_stick_with_master",            &battle_config.slave_stick_with_master,         0,      0,      1,              },
	{ "at_logout_event",                    &battle_config.at_logout_event,                 1,      0,      1,              },
	{ "homunculus_starving_rate",           &battle_config.homunculus_starving_rate,        10,     0,      100,            },
	{ "homunculus_starving_delay",          &battle_config.homunculus_starving_delay,       20000,  0,      INT_MAX,        },
	{ "drop_connection_on_quit",            &battle_config.drop_connection_on_quit,         0,      0,      1,              },
	{ "mob_spawn_variance",                 &battle_config.mob_spawn_variance,              1,      0,      3,              },
	{ "mercenary_autoloot",                 &battle_config.mercenary_autoloot,              0,      0,      1,              },
	{ "mer_idle_no_share" ,                 &battle_config.mer_idle_no_share,               0,      0,      INT_MAX,        },
	{ "idletime_mer_option",                &battle_config.idletime_mer_option,             0x1F,   0x1,    0xFFF,          },

#include "../custom/battle_config_init.inc"
};

/*==========================
 * Set battle settings
 *--------------------------*/
int battle_set_value(const char* w1, const char* w2)
{
	int val = config_switch(w2);

	int i;
	ARR_FIND(0, ARRAYLENGTH(battle_data), i, strcmpi(w1, battle_data[i].str) == 0);
	if (i == ARRAYLENGTH(battle_data))
		return 0; // not found

	if (val < battle_data[i].min || val > battle_data[i].max) {
		ShowWarning("Value for setting '%s': %s is invalid (min:%i max:%i)! Defaulting to %i...\n", w1, w2, battle_data[i].min, battle_data[i].max, battle_data[i].defval);
		val = battle_data[i].defval;
	}

	*battle_data[i].val = val;
	return 1;
}

/*===========================
 * Get battle settings
 *---------------------------*/
int battle_get_value(const char* w1)
{
	int i;
	ARR_FIND(0, ARRAYLENGTH(battle_data), i, strcmpi(w1, battle_data[i].str) == 0);
	if (i == ARRAYLENGTH(battle_data))
		return 0; // not found
	else
		return *battle_data[i].val;
}

/*======================
 * Set default settings
 *----------------------*/
void battle_set_defaults()
{
	int i;
	for (i = 0; i < ARRAYLENGTH(battle_data); i++)
		*battle_data[i].val = battle_data[i].defval;
}

/*==================================
 * Cap certain battle.conf settings
 *----------------------------------*/
void battle_adjust_conf()
{
	battle_config.monster_max_aspd = 2000 - battle_config.monster_max_aspd * 10;
	battle_config.max_aspd = 2000 - battle_config.max_aspd * 10;
	battle_config.max_third_aspd = 2000 - battle_config.max_third_aspd * 10;
	battle_config.max_summoner_aspd = 2000 - battle_config.max_summoner_aspd * 10;
	battle_config.max_extended_aspd = 2000 - battle_config.max_extended_aspd * 10;
	battle_config.max_walk_speed = 100 * DEFAULT_WALK_SPEED / battle_config.max_walk_speed;
	battle_config.max_cart_weight *= 10;

	if (battle_config.max_def > 100 && !battle_config.weapon_defense_type) // added by [Skotlex]
		battle_config.max_def = 100;

	if (battle_config.min_hitrate > battle_config.max_hitrate)
		battle_config.min_hitrate = battle_config.max_hitrate;

	if (battle_config.pet_max_atk1 > battle_config.pet_max_atk2) //Skotlex
		battle_config.pet_max_atk1 = battle_config.pet_max_atk2;

	if (battle_config.day_duration && battle_config.day_duration < 60000) // added by [Yor]
		battle_config.day_duration = 60000;
	if (battle_config.night_duration && battle_config.night_duration < 60000) // added by [Yor]
		battle_config.night_duration = 60000;

#if PACKETVER < 20100427
	if (battle_config.feature_buying_store) {
		ShowWarning("conf/battle/feature.conf:buying_store is enabled but it requires PACKETVER 2010-04-27 or newer, disabling...\n");
		battle_config.feature_buying_store = 0;
	}
#endif

#if PACKETVER < 20100803
	if (battle_config.feature_search_stores) {
		ShowWarning("conf/battle/feature.conf:search_stores is enabled but it requires PACKETVER 2010-08-03 or newer, disabling...\n");
		battle_config.feature_search_stores = 0;
	}
#endif

#if PACKETVER < 20120101
	if (battle_config.feature_bgqueue) {
		ShowWarning("conf/battle/feature.conf:bgqueue is enabled but it requires PACKETVER 2012-01-01 or newer, disabling...\n");
		battle_config.feature_bgqueue = 0;
	}
#endif

#if PACKETVER > 20120000 && PACKETVER < 20130515 /* Exact date (when it started) not known */
	if (battle_config.feature_auction) {
		ShowWarning("conf/battle/feature.conf:feature.auction is enabled but it is not stable on PACKETVER " EXPAND_AND_QUOTE(PACKETVER) ", disabling...\n");
		ShowWarning("conf/battle/feature.conf:feature.auction change value to '2' to silence this warning and maintain it enabled\n");
		battle_config.feature_auction = 0;
	}
#elif PACKETVER >= 20141112
	if (battle_config.feature_auction) {
		ShowWarning("conf/battle/feature.conf:feature.auction is enabled but it is not available for clients from 2014-11-12 on, disabling...\n");
		ShowWarning("conf/battle/feature.conf:feature.auction change value to '2' to silence this warning and maintain it enabled\n");
		battle_config.feature_auction = 0;
	}
#endif

#if PACKETVER < 20130724
	if (battle_config.feature_banking) {
		ShowWarning("conf/battle/feature.conf banking is enabled but it requires PACKETVER 2013-07-24 or newer, disabling...\n");
		battle_config.feature_banking = 0;
	}
#endif

#if PACKETVER < 20131223
	if (battle_config.mvp_exp_reward_message) {
		ShowWarning("conf/battle/client.conf MVP EXP reward message is enabled but it requires PACKETVER 2013-12-23 or newer, disabling...\n");
		battle_config.mvp_exp_reward_message = 0;
	}
#endif

#if PACKETVER < 20141022
	if (battle_config.feature_roulette) {
		ShowWarning("conf/battle/feature.conf roulette is enabled but it requires PACKETVER 2014-10-22 or newer, disabling...\n");
		battle_config.feature_roulette = 0;
	}
#endif

#if PACKETVER < 20150513
	if (battle_config.feature_achievement) {
		ShowWarning("conf/battle/feature.conf achievement is enabled but it requires PACKETVER 2015-05-13 or newer, disabling...\n");
		battle_config.feature_achievement = 0;
	}
#endif

#if PACKETVER < 20141008
	if (battle_config.feature_petevolution) {
		ShowWarning("conf/battle/feature.conf petevolution is enabled but it requires PACKETVER 2014-10-08 or newer, disabling...\n");
		battle_config.feature_petevolution = 0;
	}
	if (battle_config.feature_pet_autofeed) {
		ShowWarning("conf/battle/feature.conf pet auto feed is enabled but it requires PACKETVER 2014-10-08 or newer, disabling...\n");
		battle_config.feature_pet_autofeed = 0;
	}
#endif

#if PACKETVER < 20170208
	if (battle_config.feature_equipswitch) {
		ShowWarning("conf/battle/feature.conf equip switch is enabled but it requires PACKETVER 2017-02-08 or newer, disabling...\n");
		battle_config.feature_equipswitch = 0;
	}
#endif

#if PACKETVER < 20170920
	if( battle_config.feature_homunculus_autofeed ){
		ShowWarning("conf/battle/feature.conf homunculus autofeeding is enabled but it requires PACKETVER 2017-09-20 or newer, disabling...\n");
		battle_config.feature_homunculus_autofeed = 0;
	}
#endif

#if PACKETVER < 20180307
	if( battle_config.feature_attendance ){
		ShowWarning("conf/battle/feature.conf attendance system is enabled but it requires PACKETVER 2018-03-07 or newer, disabling...\n");
		battle_config.feature_attendance = 0;
	}
#endif

#if PACKETVER < 20180321
	if( battle_config.feature_privateairship ){
		ShowWarning("conf/battle/feature.conf private airship system is enabled but it requires PACKETVER 2018-03-21 or newer, disabling...\n");
		battle_config.feature_privateairship = 0;
	}
#endif

#ifndef CELL_NOSTACK
	if (battle_config.custom_cell_stack_limit != 1)
		ShowWarning("Battle setting 'custom_cell_stack_limit' takes no effect as this server was compiled without Cell Stack Limit support.\n");
#endif
}

/*=====================================
 * Read battle.conf settings from file
 *-------------------------------------*/
int battle_config_read(const char* cfgName)
{
	FILE* fp;
	static int count = 0;

	if (count == 0)
		battle_set_defaults();

	count++;

	fp = fopen(cfgName,"r");
	if (fp == NULL)
		ShowError("File not found: %s\n", cfgName);
	else {
		char line[1024], w1[1024], w2[1024];

		while(fgets(line, sizeof(line), fp)) {
			if (line[0] == '/' && line[1] == '/')
				continue;
			if (sscanf(line, "%1023[^:]:%1023s", w1, w2) != 2)
				continue;
			if (strcmpi(w1, "import") == 0)
				battle_config_read(w2);
			else if( strcmpi( w1, "atcommand_symbol" ) == 0 ){
				const char* symbol = &w2[0];

				if (ISPRINT(*symbol) && // no control characters
					*symbol != '/' && // symbol of client commands
					*symbol != '%' && // symbol of party chat
					*symbol != '$' && // symbol of guild chat
					*symbol != charcommand_symbol)
					atcommand_symbol = *symbol;
			}else if( strcmpi( w1, "charcommand_symbol" ) == 0 ){
				const char* symbol = &w2[0];

				if (ISPRINT(*symbol) && // no control characters
					*symbol != '/' && // symbol of client commands
					*symbol != '%' && // symbol of party chat
					*symbol != '$' && // symbol of guild chat
					*symbol != atcommand_symbol)
					charcommand_symbol = *symbol;
			}else if( battle_set_value(w1, w2) == 0 )
				ShowWarning("Unknown setting '%s' in file %s\n", w1, cfgName);
		}

		fclose(fp);
	}

	count--;

	if (count == 0)
		battle_adjust_conf();

	return 0;
}

/*==========================
 * initialize battle timer
 *--------------------------*/
void do_init_battle(void)
{
	delay_damage_ers = ers_new(sizeof(struct delay_damage),"battle.cpp::delay_damage_ers",ERS_OPT_CLEAR);
	add_timer_func_list(battle_delay_damage_sub, "battle_delay_damage_sub");
}

/*==================
 * end battle timer
 *------------------*/
void do_final_battle(void)
{
	ers_destroy(delay_damage_ers);
}
