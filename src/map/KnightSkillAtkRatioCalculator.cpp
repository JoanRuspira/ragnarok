#include "KnightSkillAtkRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"


int KnightSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case KN_AUTOCOUNTER:
			add_auto_counter_special_effects(src, target);
			return calculate_auto_counter_atk_ratio(skill_lv);
			break;
		case KN_BOWLINGBASH:
			return calculate_bowling_bash_atk_ratio(skill_lv, target);
			break;
		case CR_SHIELDBOOMERANG:
		case KN_SPEARBOOMERANG:
			return calculate_spear_boomerang_atk_ratio(skill_lv);
			break;
		case KN_PIERCE:
			return calculate_pierce_atk_ratio(skill_lv);
			break;
		case KN_BRANDISHSPEAR:
			return calculate_brandish_spear_atk_ratio(skill_lv, sstatus->vit);
			break;
		case RK_WINDCUTTER:
			return calculate_wind_cutter_atk_ratio(skill_lv, sstatus->int_);
			break;
		case RK_SONICWAVE:
			return calculate_sonic_wave_atk_ratio(skill_lv,  sstatus->int_);
		case CR_SHIELDCHARGE:
			add_smite_special_effects(target);
			return calculate_smite_atk_ratio(skill_lv);
		case PA_SACRIFICE:
			return calculate_reckoning_atk_ratio(skill_lv);
		case RK_HUNDREDSPEAR:
			return calculate_a_hundred_spears_atk_ratio(skill_lv);
		case LK_SPIRALPIERCE:
			return calculate_bowling_bash_atk_ratio(skill_lv, target);
		default:
			return 0;
			break;
	}
}

int KnightSkillAtkRatioCalculator::calculate_clashing_spiral_atk_ratio(int skill_lv, struct block_list *target)
{
	int ratio = 0;
	struct status_change *target_status;
	target_status = status_get_sc(target);
	switch (skill_lv) {
		case 1:
			ratio = 150;
			break;
		case 2:
			ratio = 250;
			break;
		case 3:
			ratio = 350;
			break;
		case 4:
			ratio = 450;
			break;
		case 5:
			ratio = 550;
			break;
		}

	if (target_status->data[SC_BLEEDING]) {
		ratio += 50;
	}
	return ratio;
}


void KnightSkillAtkRatioCalculator::add_auto_counter_special_effects(struct block_list* src, struct block_list *target)
{
	clif_specialeffect(target, EF_MADNESS_RED, AREA);
}

int KnightSkillAtkRatioCalculator::calculate_reckoning_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 0;
			break;
		case 2:
			ratio = 25;
			break;
		case 3:
			ratio = 50;
			break;
		case 4:
			ratio = 75;
			break;
		case 5:
			ratio = 100;
			break;
		}
	return ratio;
}


int KnightSkillAtkRatioCalculator::calculate_smite_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 55;
			break;
		case 2:
			ratio = 125;
			break;
		case 3:
			ratio = 205;
			break;
		case 4:
			ratio = 265;
			break;
		case 5:
			ratio = 325;
			break;
		}
	return ratio;
}

int KnightSkillAtkRatioCalculator::calculate_sonic_wave_atk_ratio(int skill_lv, int intelligence)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 0;
			break;
		case 2:
			ratio = 10;
			break;
		case 3:
			ratio = 20;
			break;
		case 4:
			ratio = 30;
			break;
		case 5:
			ratio = 40;
			break;
		}
	return ratio + (intelligence/3);
}

int KnightSkillAtkRatioCalculator::calculate_wind_cutter_atk_ratio(int skill_lv, int intelligence)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 100;
			break;
		case 2:
			ratio = 150;
			break;
		case 3:
			ratio = 200;
			break;
		case 4:
			ratio = 250;
			break;
		case 5:
			ratio = 300;
			break;
		}
	return ratio + (intelligence/3);
}

int KnightSkillAtkRatioCalculator::calculate_a_hundred_spears_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 150;
			break;
		case 2:
			ratio = 250;
			break;
		case 3:
			ratio = 350;
			break;
		case 4:
			ratio = 450;
			break;
		case 5:
			ratio = 550;
			break;
		}
	return ratio;
}

int KnightSkillAtkRatioCalculator::calculate_brandish_spear_atk_ratio(int skill_lv, int vit)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 100;
			break;
		case 2:
			ratio = 150;
			break;
		case 3:
			ratio = 200;
			break;
		case 4:
			ratio = 250;
			break;
		case 5:
			ratio = 300;
			break;
		}
	return ratio + vit/3;
}

int KnightSkillAtkRatioCalculator::calculate_pierce_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 40;
			break;
		case 2:
			ratio = 60;
			break;
		case 3:
			ratio = 80;
			break;
		case 4:
			ratio = 100;
			break;
		case 5:
			ratio = 120;
			break;
		}
	return ratio;
}


int KnightSkillAtkRatioCalculator::calculate_spear_boomerang_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 30;
			break;
		case 2:
			ratio = 100;
			break;
		case 3:
			ratio = 180;
			break;
		case 4:
			ratio = 240;
			break;
		case 5:
			ratio = 300;
			break;
		}
	return ratio;
}


int KnightSkillAtkRatioCalculator::calculate_auto_counter_atk_ratio(int skill_lv)
{
	return (60  * skill_lv);
}


int KnightSkillAtkRatioCalculator::calculate_bowling_bash_atk_ratio(int skill_lv, struct block_list *target)
{
	int ratio = 0;
	struct status_change *target_status;

	target_status = status_get_sc(target);

	if (target_status->data[SC_BLEEDING]) {
		return calculate_bowling_bash_bleeding_atk_ratio(skill_lv);
	}
	return calculate_bowling_bash_normal_atk_ratio(skill_lv);
}

int KnightSkillAtkRatioCalculator::calculate_bowling_bash_normal_atk_ratio(int skill_lv)
{
	int ratio = 0;

	switch (skill_lv) {
		case 1:
			ratio = 10;
			break;
		case 2:
			ratio = 20;
			break;
		case 3:
			ratio = 30;
			break;
		case 4:
			ratio = 40;
			break;
		case 5:
			ratio = 50;
			break;
	}
	return ratio;
}


int KnightSkillAtkRatioCalculator::calculate_bowling_bash_bleeding_atk_ratio(int skill_lv)
{
	int ratio = 0;
	
	switch (skill_lv) {
		case 1:
			ratio = 40;
			break;
		case 2:
			ratio = 50;
			break;
		case 3:
			ratio = 60;
			break;
		case 4:
			ratio = 70;
			break;
		case 5:
			ratio = 80;
			break;
	}
	return ratio;
}


void KnightSkillAtkRatioCalculator::add_smite_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_STUNATTACK, AREA);
}

// int KnightSkillAtkRatioCalculator::calculate_spear_boomerang_atk_ratio()
// {
// 	return (100 + (_base_lv / 5)) * _skill_lv;
// }

// int KnightSkillAtkRatioCalculator::calculate_brandish_spear_atk_ratio()
// {
// 	return (_base_lv / 6) * _skill_lv;
// }

// int KnightSkillAtkRatioCalculator::calculate_pierce_atk_ratio()
// {
// 	return (10 + (_base_lv / 10)) * _skill_lv;
// }

// int KnightSkillAtkRatioCalculator::calculate_hundred_spear_atk_ratio()
// {
// 	return 550 + ((_base_lv + 20) * _skill_lv);
// }



// int KnightSkillAtkRatioCalculator::calculate_charge_attack_atk_ratio()
// {
// 	return (10 * _base_lv) * 3;
// }

// int KnightSkillAtkRatioCalculator::calculate_head_crush_atk_ratio()
// {
// 	return (_base_lv / 4) * _skill_lv;
// }

// int KnightSkillAtkRatioCalculator::calculate_spiral_pierce_atk_ratio()
// {
// 	return 55 + (_base_lv / 3) * _skill_lv;
// }

// int KnightSkillAtkRatioCalculator::calculate_shield_chain_atk_ratio()
// {
// 	return 400 + _base_lv + 40 * _skill_lv;
// }

// int KnightSkillAtkRatioCalculator::calculate_wind_cutter_atk_ratio()
// {
// 	return 100 + ((_base_lv + _int) / 1.5 * _skill_lv);
// }

// int KnightSkillAtkRatioCalculator::calculate_sonic_wave_atk_ratio()
// {
// 	return 200 + ((_base_lv + _int) / 20 * _skill_lv);
// }

// int KnightSkillAtkRatioCalculator::calculate_ignition_break_atk_ratio()
// {
// 	int skillratio = ((_base_lv + _int) + 60)* _skill_lv;
// 	skillratio = skillratio * _base_lv / 100;
// 	// Elemental check, 1.5x damage if your weapon element is fire.
// 	if (_weapon_element == ELE_FIRE) {
// 		skillratio += 100 * _skill_lv;
// 	}
// 	return skillratio;
// }

// //sstatus->rhw.ele
// 		//case RK_DRAGONBREATH:
// 		//	{
// 		//		int damagevalue = ((50 + status_get_lv(src) + sstatus->int_) * 4) * skill_lv; //status_get_max_sp(src)

// 		//		if(status_get_lv(src) > 100)
// 		//			damagevalue = damagevalue * status_get_lv(src) / 150;
// 		//		if(sd)
// 		//			damagevalue = damagevalue * (100 + 5 * (pc_checkskill(sd,RK_DRAGONTRAINING) - 1)) / 100;
// 		//		ATK_ADD(wd->damage, wd->damage2, damagevalue);
// 		//		ATK_ADD(wd->weaponAtk, wd->weaponAtk2, damagevalue);
// 		//		wd->flag |= BF_LONG;
// 		//	}
// 		//	break;
