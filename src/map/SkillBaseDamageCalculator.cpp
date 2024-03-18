
#include <math.h>
#include <stdlib.h>
#include <cstring>
#include <cstdint>
#include "skill.hpp"
#include "status.hpp"
#include "Damage.h"
#include "map.hpp"
#include "battle.hpp"
#include "pc.hpp"
#include "party.hpp"
#include "CriticalHitCalculator.h"
#include "SkillBaseDamageCalculator.h"
#include "EquipmentAttackCalculator.h"
#include "elemental.hpp"
#include "battle.hpp"



#define ATK_ADDRATE(damage, damage2, a) do { int64 rate_ = (a); (damage) += (damage) * rate_ / 100; if(EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) (damage2) += (damage2) *rate_ / 100; } while(0);
#define RE_ALLATK_ADDRATE(wd, a) do { int64 a_ = (a); ATK_ADDRATE((wd)->statusAtk, (wd)->statusAtk2, a_); ATK_ADDRATE((wd)->weaponAtk, (wd)->weaponAtk2, a_); ATK_ADDRATE((wd)->equipAtk, (wd)->equipAtk2, a_); ATK_ADDRATE((wd)->masteryAtk, (wd)->masteryAtk2, a_); } while(0);
#define ATK_ADD(damage, damage2, a) do { int64 rate_ = (a); (damage) += rate_; if(EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) (damage2) += rate_; } while(0);
#define ATK_ADD2(damage, damage2, a , b) do { int64 rate_ = (a), rate2_ = (b); (damage) += rate_; if(EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) (damage2) += rate2_; } while(0);




/**
 * BASE ATK CALCULATOR FOR SKILLS AND NORMAL ATTACKS
 */


 /*========================================================== battle_calc_skill_base_damage
  * Calculate basic ATK that goes into the skill ATK formula    called by battle_calc_weapon_attack along with the ratio calc
  *----------------------------------------------------------
  */
void SkillBaseDamageCalculator::battle_calc_skill_base_damage(Damage * wd, block_list * src, block_list * target, uint16 skill_id, uint16 skill_lv)
{
	struct status_change *sc = status_get_sc(src);
	struct status_data *sstatus = status_get_status_data(src);
	struct status_data *tstatus = status_get_status_data(target);
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	uint16 i;
	//std::bitset<NK_MAX> nk = battle_skill_get_damage_properties(skill_id, wd->miscflag);
	std::bitset<NK_MAX> nk;

	if (skill_id == 0) {
		if (wd->miscflag) {
			std::bitset<NK_MAX> tmp_nk;

			tmp_nk.set(NK_IGNOREATKCARD);
			tmp_nk.set(NK_IGNOREFLEE);

			nk = tmp_nk;
		}
		else
			nk = 0;
	}
	else
		nk = skill_db.find(skill_id)->nk;


	//joan
	if (skill_id == SK_KN_RECKONING || skill_id == SK_HT_LIVINGTORNADO || skill_id == SK_RA_ZEPHYRSNIPING) {
		wd->damage = sstatus->max_hp * 1 / 100;
		wd->damage2 = 0;
		wd->weaponAtk = wd->damage;
		wd->weaponAtk2 = wd->damage2;
	}



	if (sd) {
		battle_calc_damage_parts(wd, src, target, skill_id, skill_lv);
	} else {
		i = (CriticalHitCalculator::is_attack_critical(wd, src, target, skill_id, skill_lv, false) ? 1 : 0) |
			(!skill_id && sc && false ? 4 : 0);

		wd->damage = battle_calc_base_damage(src, sstatus, &sstatus->rhw, sc, tstatus->size, i);
		if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) {
			wd->damage2 = battle_calc_base_damage(src, sstatus, &sstatus->lhw, sc, tstatus->size, i);
		}
	}

	if (nk[NK_SPLASHSPLIT]) { // Divide ATK among targets
		if (wd->miscflag > 0) {
			wd->damage /= wd->miscflag;
			wd->statusAtk /= wd->miscflag;
			wd->weaponAtk /= wd->miscflag;
			wd->equipAtk /= wd->miscflag;
			wd->masteryAtk /= wd->miscflag;
		}
		else
			ShowError("0 enemies targeted by %d:%s, divide per 0 avoided!\n", skill_id, skill_get_name(skill_id));
	}

	//Add any bonuses that modify the base atk (pre-skills)
	if (sd) {
		int skill;

		if (sd->bonus.atk_rate) {
			ATK_ADDRATE(wd->damage, wd->damage2, sd->bonus.atk_rate);
			RE_ALLATK_ADDRATE(wd, sd->bonus.atk_rate);
		}
	}
}


/*==========================================
 * Calculates the standard damage of a normal attack assuming it hits,
 * it calculates nothing extra fancy, is needed for magnum break's WATK_ELEMENT bonus. [Skotlex]
 * This applies to pre-renewal and non-sd in renewal
 *------------------------------------------
 * Pass damage2 as NULL to not calc it.
 * Flag values:
 * &1 : Critical hit
 * &2 : Arrow attack
 * &4 : Skill is Magic Crasher
 * &8 : Skip target size adjustment (Extremity Fist?)
 * &16: Arrow attack but BOW, REVOLVER, RIFLE, SHOTGUN, GATLING or GRENADE type weapon not equipped (i.e. shuriken, kunai and venom knives not affected by DEX)
 */
int64 SkillBaseDamageCalculator::battle_calc_base_damage(block_list * src, status_data * status, weapon_atk * wa, status_change * sc, unsigned short t_size, int flag)
{
	unsigned int atkmin = 0, atkmax = 0;
	short type = 0;
	int64 damage = 0;
	struct map_session_data *sd = NULL;
	// nullpo_ret(damage, src);

	sd = BL_CAST(BL_PC, src);

	if (!sd) { //Mobs/Pets
		if (flag & 4) {
			atkmin = status->matk_min;
			atkmax = status->matk_max;
		}
		else {
			atkmin = wa->atk;
			atkmax = wa->atk2;
		}
		if (atkmin > atkmax)
			atkmin = atkmax;
	}
	else { //PCs
		atkmax = wa->atk;
		type = (wa == &status->lhw) ? EQI_HAND_L : EQI_HAND_R;

		if (!(flag & 1) || (flag & 2)) { //Normal attacks
			atkmin = status->dex;

			if (sd->equip_index[type] >= 0 && sd->inventory_data[sd->equip_index[type]])
				atkmin = atkmin * (80 + sd->inventory_data[sd->equip_index[type]]->wlv * 20) / 100;

			if (atkmin > atkmax)
				atkmin = atkmax;

			if (flag & 2 && !(flag & 16)) { //Bows
				atkmin = atkmin * atkmax / 100;
				if (atkmin > atkmax)
					atkmax = atkmin;
			}
		}
	}

	if (sc && sc->data[STATUS_MAXIMIZEPOWER])
		atkmin = atkmax;

	//Weapon Damage calculation
	if (!(flag & 1))
		damage = (atkmax > atkmin ? rnd() % (atkmax - atkmin) : 0) + atkmin;
	else
		damage = atkmax;

	if (sd) {
		//rodatazone says the range is 0~arrow_atk-1 for non crit
		if (flag & 2 && sd->bonus.arrow_atk)
			damage += ((flag & 1) ? sd->bonus.arrow_atk : rnd() % sd->bonus.arrow_atk);

		// Size fix only for players
		if (!(sd->special_state.no_sizefix || (flag & 8)))
			damage = damage * (type == EQI_HAND_L ? sd->left_weapon.atkmods[t_size] : sd->right_weapon.atkmods[t_size]) / 100;
	}
	else if (src->type == BL_ELEM) {
		struct status_change *ele_sc = status_get_sc(src);
		int ele_class = status_get_class(src);

		if (ele_sc) {
			switch (ele_class) {
			case ELEMENTALID_AGNI_S:
			case ELEMENTALID_AGNI_M:
			case ELEMENTALID_AGNI_L:
				break;
			case ELEMENTALID_AQUA_S:
			case ELEMENTALID_AQUA_M:
			case ELEMENTALID_AQUA_L:
				break;
			case ELEMENTALID_VENTUS_S:
			case ELEMENTALID_VENTUS_M:
			case ELEMENTALID_VENTUS_L:
				break;
			case ELEMENTALID_TERA_S:
			case ELEMENTALID_TERA_M:
			case ELEMENTALID_TERA_L:
				break;
			}
		}
	}

	//Finally, add baseatk
	if (flag & 4)
		damage += status->matk_min;
	else
		damage += status->batk;

	if (sd)
		EquipmentAttackCalculator::battle_add_weapon_damage(sd, &damage, type);

	if (flag & 1)
		damage = (damage * 14) / 10;


	return damage;
}


/*=========================================
 * Calculate the various Renewal ATK parts
 *-----------------------------------------
 */
void SkillBaseDamageCalculator::battle_calc_damage_parts(Damage * wd, block_list * src, block_list * target, uint16 skill_id, uint16 skill_lv)
{
	struct status_data *sstatus = status_get_status_data(src);
	struct status_data *tstatus = status_get_status_data(target);
	struct map_session_data *sd = BL_CAST(BL_PC, src);

	int right_element = EquipmentAttackCalculator::battle_get_weapon_element(wd, src, target, skill_id, skill_lv, EQI_HAND_R, false);
	int left_element = EquipmentAttackCalculator::battle_get_weapon_element(wd, src, target, skill_id, skill_lv, EQI_HAND_L, false);

	wd->statusAtk += battle_calc_status_attack(sstatus, EQI_HAND_R);
	wd->statusAtk2 += battle_calc_status_attack(sstatus, EQI_HAND_L);

	// status atk is considered neutral on normal attacks [helvetica]
	wd->statusAtk = battle_attr_fix(src, target, wd->statusAtk, ELE_NEUTRAL, tstatus->def_ele, tstatus->ele_lv);
	wd->statusAtk2 = battle_attr_fix(src, target, wd->statusAtk, ELE_NEUTRAL, tstatus->def_ele, tstatus->ele_lv);
	
	wd->weaponAtk += EquipmentAttackCalculator::battle_calc_base_weapon_attack(src, tstatus, &sstatus->rhw, sd);
	wd->weaponAtk = battle_attr_fix(src, target, wd->weaponAtk, right_element, tstatus->def_ele, tstatus->ele_lv);

	wd->weaponAtk2 += EquipmentAttackCalculator::battle_calc_base_weapon_attack(src, tstatus, &sstatus->lhw, sd);
	wd->weaponAtk2 = battle_attr_fix(src, target, wd->weaponAtk2, left_element, tstatus->def_ele, tstatus->ele_lv);

	wd->equipAtk += EquipmentAttackCalculator::battle_calc_equip_attack(src, skill_id);
	wd->equipAtk = battle_attr_fix(src, target, wd->equipAtk, right_element, tstatus->def_ele, tstatus->ele_lv);

	wd->equipAtk2 += EquipmentAttackCalculator::battle_calc_equip_attack(src, skill_id);
	wd->equipAtk2 = battle_attr_fix(src, target, wd->equipAtk2, left_element, tstatus->def_ele, tstatus->ele_lv);

	//Mastery ATK is a special kind of ATK that has no elemental properties
	//Because masteries are not elemental, they are unaffected by Ghost armors or Raydric Card
	battle_calc_attack_masteries(wd, src, target, skill_id, skill_lv);

	wd->damage = 0;
	wd->damage2 = 0;
}


/*==================================
 * Calculate weapon mastery damages
 *----------------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */
void SkillBaseDamageCalculator::battle_calc_attack_masteries(Damage * wd, block_list * src, block_list * target, uint16 skill_id, uint16 skill_lv)
{
	struct map_session_data *sd = BL_CAST(BL_PC, src);
	struct status_change *sc = status_get_sc(src);
	struct status_data *sstatus = status_get_status_data(src);
	int t_class = status_get_class(target);

	if (sd && EquipmentAttackCalculator::battle_skill_stacks_masteries_vvs(skill_id))
	{	//Add mastery damage
		uint16 skill;

		wd->damage = battle_addmastery(sd, target, wd->damage, 0);
		wd->masteryAtk = battle_addmastery(sd, target, wd->weaponAtk, 0);

		if (EquipmentAttackCalculator::is_attack_left_handed(src, skill_id)) {
			wd->damage2 = battle_addmastery(sd, target, wd->damage2, 1);

			wd->masteryAtk2 = battle_addmastery(sd, target, wd->weaponAtk2, 1);

		}


		//General skill masteries
		if (skill_id == SK_TF_POISONSLASH) //Additional ATK from Envenom is treated as mastery type damage [helvetica]
			ATK_ADD(wd->masteryAtk, wd->masteryAtk2, 15 * skill_lv);
		if (skill_id != SK_CR_SHIELDBOOMERANG)
			ATK_ADD2(wd->masteryAtk, wd->masteryAtk2, ((wd->div_ < 1) ? 1 : wd->div_) * sd->right_weapon.star, ((wd->div_ < 1) ? 1 : wd->div_) * sd->left_weapon.star);
		if (skill_id == SK_MO_TRHOWSPIRITSPHERE) {
			ATK_ADD(wd->masteryAtk, wd->masteryAtk2, ((wd->div_ < 1) ? 1 : wd->div_) * sd->spiritball_old * 3);
		}
		else
			ATK_ADD(wd->masteryAtk, wd->masteryAtk2, ((wd->div_ < 1) ? 1 : wd->div_) * sd->spiritball * 3);

		// switch (skill_id) {
		// case RA_WUGDASH:
		// case RA_WUGBITE:
		// 	if (sd) {
		// 		skill = pc_checkskill(sd, RA_TOOTHOFWUG);
		// 		ATK_ADD(wd->damage, wd->damage2, 30 * skill * 2);
		// 		ATK_ADD(wd->masteryAtk, wd->masteryAtk2, 30 * skill * 2);
		// 	}
		// 	break;
		// }
		if (sc) { // Status change considered as masteries
			
			if (sc->data[STATUS_CAMOUFLAGE]) {
				ATK_ADD(wd->damage, wd->damage2, 30 * min(10, sc->data[STATUS_CAMOUFLAGE]->val3));
				ATK_ADD(wd->masteryAtk, wd->masteryAtk2, 30 * min(10, sc->data[STATUS_CAMOUFLAGE]->val3));
			}
			
			
		}
	}
}


/**
 * Passive skill damage increases
 * @param sd
 * @param target
 * @param dmg
 * @param type
 * @return damage
 */
int64 SkillBaseDamageCalculator::battle_addmastery(map_session_data * sd, block_list * target, int64 dmg, int type)
{
	int64 damage;
	struct status_data *status = status_get_status_data(target);
	int weapon, skill;

	damage = 0;
	nullpo_ret(sd);

	
	// if ((skill = pc_checkskill(sd, SK_HT_TRACKINGBREEZE)) > 0 && (status->race == RC_BRUTE || status->race == RC_PLANT || status->race == RC_FISH))
	// 	damage += (skill * 5);
	

	if (type == 0)
		weapon = sd->weapontype1;
	else
		weapon = sd->weapontype2;

	switch (weapon) {
		case W_2HSWORD:
			if ((skill = pc_checkskill(sd, SK_SM_TWOHAND)) > 0)
				damage += (skill * 8);
		case W_1HSWORD:
			if ((skill = pc_checkskill(sd, SK_SM_SWORD)) > 0)
				damage += (skill * 8);
			if ((skill = pc_checkskill(sd, SK_TF_SWORD)) > 0)
				damage += (skill * 8);
		case W_DAGGER:
			if ((skill = pc_checkskill(sd, SK_TF_DAGGER)) > 0)
				damage += (skill * 8);
			break;
		case W_1HSPEAR:
		case W_2HSPEAR:
			if ((skill = pc_checkskill(sd, SK_SM_SPEARMASTERY)) > 0) {
				damage += (skill * 8);
			}
			break;
		case W_BOW:
			if ((skill = pc_checkskill(sd, SK_AC_BOW)) > 0)
				damage += (skill * 8);
			break;
		case W_1HAXE:
		case W_2HAXE:
			if ((skill = pc_checkskill(sd, SK_MC_AXEMASTERY)) > 0)
				damage += (skill * 8);
			break;
		case W_MACE:
		case W_2HMACE:
		case W_KNUCKLE:
			if ((skill = pc_checkskill(sd, SK_AL_MACEMASTERY)) > 0)
				damage += (skill * 8);
			break;
		case W_MUSICAL:
			if ((skill = pc_checkskill(sd, SK_BA_SHOWMANSHIP)) > 0)
				damage += (skill * 8);
			break;
		case W_KATAR:
			if ((skill = pc_checkskill(sd, SK_AS_KATAR)) > 0)
				damage += (skill * 8);
			break;
	}

	return damage;
}


int SkillBaseDamageCalculator::battle_calc_status_attack(status_data * status, short hand)
{
	//left-hand penalty on sATK is always 50% [Baalberith]
	if (hand == EQI_HAND_L)
		return status->batk;
	else
		return 2 * status->batk;
}

//
//std::bitset<NK_MAX> SkillBaseDamageCalculator::battle_skill_get_damage_properties(uint16 skill_id, int is_splash)
//{
//	if (skill_id == 0) {
//		if (is_splash) {
//			std::bitset<NK_MAX> tmp_nk;
//
//			tmp_nk.set(NK_IGNOREATKCARD);
//			tmp_nk.set(NK_IGNOREFLEE);
//
//			return tmp_nk;
//		}
//		else
//			return 0;
//	}
//	else
//		return skill_db.find(skill_id)->nk;
//}
//
//

