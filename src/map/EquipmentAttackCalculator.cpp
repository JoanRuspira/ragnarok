
#include <math.h>
#include <stdlib.h>
#include <cstring>
#include <cstdint>
#include "skill.hpp"
#include "status.hpp"
#include "Damage.h"
#include "map.hpp"
#include "homunculus.hpp"
#include "battle.hpp"
#include "pc.hpp"
#include "party.hpp"
#include "CriticalHitCalculator.h"
#include "SkillBaseDamageCalculator.h"
#include "EquipmentAttackCalculator.h"
#include "elemental.hpp"
#include "battle.hpp"


/** Calculates overrefine damage bonus and weapon related bonuses (unofficial)
* @param sd Player
* @param damage Current damage
* @param lr_type EQI_HAND_L:left-hand weapon, EQI_HAND_R:right-hand weapon
*/
void EquipmentAttackCalculator::battle_add_weapon_damage(map_session_data * sd, int64 * damage, int lr_type)
{
	if (!sd)
		return;
	//rodatazone says that Overrefine bonuses are part of baseatk
	//Here we also apply the weapon_damage_rate bonus so it is correctly applied on left/right hands.
	if (lr_type == EQI_HAND_L) {
		if (sd->left_weapon.overrefine)
			(*damage) = (*damage) + rnd() % sd->left_weapon.overrefine + 1;
		if (sd->indexed_bonus.weapon_damage_rate[sd->weapontype2])
			(*damage) += (*damage) * sd->indexed_bonus.weapon_damage_rate[sd->weapontype2] / 100;
	}
	else if (lr_type == EQI_HAND_R) {
		if (sd->right_weapon.overrefine)
			(*damage) = (*damage) + rnd() % sd->right_weapon.overrefine + 1;
		if (sd->indexed_bonus.weapon_damage_rate[sd->weapontype1])
			(*damage) += (*damage) * sd->indexed_bonus.weapon_damage_rate[sd->weapontype1] / 100;
	}
}


/**
 * Calculates renewal Variance, OverUpgradeBonus, and SizePenaltyMultiplier of weapon damage parts for player
 * @param src Block list of attacker
 * @param tstatus Target's status data
 * @param wa Weapon attack data
 * @param sd Player
 * @return Base weapon damage
 */
int EquipmentAttackCalculator::battle_calc_base_weapon_attack(block_list * src, status_data * tstatus, weapon_atk * wa, map_session_data * sd)
{
	struct status_data *status = status_get_status_data(src);
	uint8 type = (wa == &status->lhw) ? EQI_HAND_L : EQI_HAND_R;
	uint16 atkmin = (type == EQI_HAND_L) ? status->watk2 : status->watk;
	uint16 atkmax = atkmin;
	int64 damage = atkmin;
	uint16 weapon_perfection = 0;
	struct status_change *sc = status_get_sc(src);

	if (sd->equip_index[type] >= 0 && sd->inventory_data[sd->equip_index[type]]) {
		int variance = wa->atk * (sd->inventory_data[sd->equip_index[type]]->wlv * 5) / 100;

		atkmin = max(0, (int)(atkmin - variance));
		atkmax = min(UINT16_MAX, (int)(atkmax + variance));

		if (sc && sc->data[SC_MAXIMIZEPOWER])
			damage = atkmax;
		else
			damage = rnd_value(atkmin, atkmax);
	}

	if (sc && sc->data[SC_WEAPONPERFECTION])
		weapon_perfection = 1;

	battle_add_weapon_damage(sd, &damage, type);

	damage = battle_calc_sizefix(damage, sd, tstatus->size, type, weapon_perfection);

	return (int)cap_value(damage, INT_MIN, INT_MAX);
}


/**
 * Equipment Attack
 */
int EquipmentAttackCalculator::battle_calc_equip_attack(block_list * src, int skill_id)
{
	if (src != NULL) {
		int eatk = 0;
		struct status_data *status = status_get_status_data(src);
		struct map_session_data *sd = BL_CAST(BL_PC, src);

		if (sd) // add arrow atk if using an applicable skill
			eatk += (CriticalHitCalculator::is_skill_using_arrow(src, skill_id) ? sd->bonus.arrow_atk : 0);

		return eatk + status->eatk;
	}
	return 0; // shouldn't happen but just in case
}


/*========================================
 * Returns the element type of attack
 *----------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
int EquipmentAttackCalculator::battle_get_weapon_element(Damage * wd, block_list * src, block_list * target, uint16 skill_id, uint16 skill_lv, short weapon_position, bool calc_for_damage_only)
{
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	struct status_change *sc = status_get_sc(src);
	struct status_data *sstatus = status_get_status_data(src);
	int element = skill_get_ele(skill_id, skill_lv);

	//Take weapon's element
	if (!skill_id || element == ELE_WEAPON) {
		if (weapon_position == EQI_HAND_R)
			element = sstatus->rhw.ele;
		else
			element = sstatus->lhw.ele;
		if (CriticalHitCalculator::is_skill_using_arrow(src, skill_id) && sd && sd->bonus.arrow_ele && weapon_position == EQI_HAND_R)
			element = sd->bonus.arrow_ele;
		if (sd && sd->spiritcharm_type != CHARM_TYPE_NONE && sd->spiritcharm >= MAX_SPIRITCHARM)
			element = sd->spiritcharm_type; // Summoning 10 spiritcharm will endow your weapon
		// on official endows override all other elements [helvetica]
		if (sc && sc->data[SC_ENCHANTARMS]) // Check for endows
			element = sc->data[SC_ENCHANTARMS]->val1;
	}
	else if (element == ELE_ENDOWED) //Use enchantment's element
		element = status_get_attack_sc_element(src, sc);
	else if (element == ELE_RANDOM) //Use random element
		element = rnd() % ELE_ALL;

	switch (skill_id) {
	case GS_GROUNDDRIFT:
		element = wd->miscflag; //element comes in flag.
		break;
	case LK_SPIRALPIERCE:
		if (!sd)
			element = ELE_NEUTRAL; //forced neutral for monsters
		break;
	case LG_HESPERUSLIT:
		if (sc && sc->data[SC_BANDING] && sc->data[SC_BANDING]->val2 > 4)
			element = ELE_HOLY;
		break;
	case SJ_PROMINENCEKICK:
		element = ELE_FIRE;
		break;
	case RL_H_MINE:
		if (sd && sd->flicker) //Force RL_H_MINE deals fire damage if activated by RL_FLICKER
			element = ELE_FIRE;
		break;
	}

	if (sc && sc->data[SC_GOLDENE_FERSE] && ((!skill_id && (rnd() % 100 < sc->data[SC_GOLDENE_FERSE]->val4)) || skill_id == MH_STAHL_HORN))
		element = ELE_HOLY;

	// calc_flag means the element should be calculated for damage only
	if (calc_for_damage_only)
		return element;

	// if (skill_id == CR_SHIELDBOOMERANG)
	// 	element = ELE_NEUTRAL;

	return element;
}


/*=======================================
 * Is attack left handed? By default no.
 *---------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
bool EquipmentAttackCalculator::is_attack_left_handed(block_list * src, int skill_id)
{
	if (src != NULL) {
		//Skills ALWAYS use ONLY your right-hand weapon (tested on Aegis 10.2)
		if (!skill_id) {
			struct map_session_data *sd = BL_CAST(BL_PC, src);

			if (sd) {
				if (sd->weapontype1 == 0 && sd->weapontype2 > 0)
					return true;
				if (sd->status.weapon == W_KATAR)
					return true;
			}

			struct status_data *sstatus = status_get_status_data(src);

			if (sstatus->lhw.atk)
				return true;
		}
	}
	return false;
}




int EquipmentAttackCalculator::battle_calc_sizefix(int64 damage, map_session_data * sd, unsigned char t_size, unsigned char weapon_type, short flag)
{
	if (sd && !sd->special_state.no_sizefix && !flag) // Size fix only for players
		damage = damage * (weapon_type == EQI_HAND_L ? sd->left_weapon.atkmods[t_size] : sd->right_weapon.atkmods[t_size]) / 100;

	return (int)cap_value(damage, INT_MIN, INT_MAX);
}


/*================================================
 * Should skill attack consider VVS and masteries?
 *------------------------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
bool EquipmentAttackCalculator::battle_skill_stacks_masteries_vvs(uint16 skill_id)
{
	if (skill_id == RK_DRAGONBREATH || skill_id == RK_DRAGONBREATH_WATER || skill_id == NC_SELFDESTRUCTION ||
		skill_id == LG_SHIELDPRESS || skill_id == LG_EARTHDRIVE)
		return false;

	return true;
}

