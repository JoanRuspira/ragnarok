#include "HunterSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int HunterSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case SK_WG_SLASH:
			add_slash_special_effects(target);
			return 0;
			break;
		// 	return calculate_slash_atk_ratio(skill_lv,sstatus->int_);
		// case SK_FC_BLITZBEAT:
		// 	return calculate_blitz_beat_atk_ratio(skill_lv,sstatus->agi);
		case SK_RA_ZEPHYRSNIPING:
			add_hurricane_fury_special_effects(target);
			return calculate_zephyr_sniping_atk_ratio(skill_lv);
		case SK_HT_LIVINGTORNADO:
    		clif_specialeffect(target, 1402, AREA); //new_cannon_spear_03_clock
			return calculate_cyclonic_charge_atk_ratio(skill_lv);
		case SK_HT_MAGICTOMAHAWK:
			add_magic_tomahawk_special_effects(target);
			return calculate_magic_tomahawk_atk_ratio(skill_lv, sstatus->matk_max);
		case SK_ST_SHARPSHOOTING:
			add_sharp_shooting_special_effects(target);
			return calculate_sharp_shooting_atk_ratio(skill_lv,sstatus->dex);
		case SK_RA_ULLREAGLETOTEM_ATK:
			return calculate_ullrs_magic_shot_atk_ratio(skill_lv);
		default:
			return 0;
			break;
	}
}

int HunterSkillAttackRatioCalculator::calculate_ullrs_magic_shot_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = -95;
			break;
		case 2:
			ratio = -85;
			break;
		case 3:
			ratio = -75;
			break;
		case 4:
			ratio = -65;
			break;
		case 5:
			ratio = -55;
			break;
		}
	return ratio;
}

int HunterSkillAttackRatioCalculator::calculate_sharp_shooting_atk_ratio(int skill_lv, int dex)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 200;
			break;
		case 2:
			ratio = 300;
			break;
		case 3:
			ratio = 400;
			break;
		case 4:
			ratio = 500;
			break;
		case 5:
			ratio = 600;
			break;
		}
	return ratio + dex;
}


int HunterSkillAttackRatioCalculator::calculate_slash_atk_ratio(int skill_lv, int int_)
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
	return ratio + int_;
}

int HunterSkillAttackRatioCalculator::calculate_blitz_beat_atk_ratio(int skill_lv, int agi)
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
	return ratio + agi;
}

void HunterSkillAttackRatioCalculator::add_sharp_shooting_special_effects(struct block_list *target)
{
	
    clif_specialeffect(target, EF_ERASER_CUTTER, AREA);
}

void HunterSkillAttackRatioCalculator::add_magic_tomahawk_special_effects(struct block_list *target)
{
	
    clif_specialeffect(target, EF_SMA_READY, AREA);
    clif_specialeffect(target, 1271, AREA);
}

void HunterSkillAttackRatioCalculator::add_hurricane_fury_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_TEIHIT1T, AREA);
}

void HunterSkillAttackRatioCalculator::add_slash_special_effects(struct block_list *target)
{
    clif_specialeffect(target, 1103, AREA);
}

int HunterSkillAttackRatioCalculator::calculate_zephyr_sniping_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 100;
			break;
		case 2:
			ratio = 125;
			break;
		case 3:
			ratio = 150;
			break;
		case 4:
			ratio = 175;
			break;
		case 5:
			ratio = 200;
			break;
		}
	return ratio;
}

int HunterSkillAttackRatioCalculator::calculate_cyclonic_charge_atk_ratio(int skill_lv)
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

int HunterSkillAttackRatioCalculator::calculate_magic_tomahawk_atk_ratio(int skill_lv, int matk)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 125;
			break;
		case 2:
			ratio = 225;
			break;
		case 3:
			ratio = 325;
			break;
		case 4:
			ratio = 425;
			break;
		case 5:
			ratio = 525;
			break;
		}
	return (ratio + matk) / 1.5;
}
