#include "ArcherSkillAtkRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int ArcherSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list* target, int base_lv, int skill_id, int skill_lv)
{
	switch (skill_id) {
		case AC_DOUBLE:
			return calculate_double_strafe_atk_ratio(base_lv, skill_lv);
			break;
		case AC_SHOWER:
			add_arrow_shower_special_effects(target);
			return calculate_arrow_shower_atk_ratio(base_lv, skill_lv);
			break;
		case HT_POWER:
		    add_spiritual_strafe_special_effects(target);
			return calculate_spiritual_strafe_atk_ratio(skill_lv, src);
			break;
		case AC_PARALIZING:
			add_paralyzing_shot_special_effects(target);
			return -99;
			break;
		case AC_TRANQUILIZING:
			add_tranquilizer_shot_special_effects(target);
			return -99;
			break;
		case AC_CHARGEARROW:
			add_charge_arrow_special_effects(target);
			return calculate_charge_arrow_atk_ratio(base_lv, skill_lv);
			break;
		case HT_PHANTASMIC:
			add_arrow_rain_special_effects(target);
			return calculate_arrow_rain_atk_ratio(base_lv, skill_lv, src);
			break;
		default:
			return 0;
			break;
	}
}

int ArcherSkillAtkRatioCalculator::calculate_double_strafe_atk_ratio(int base_lv, int skill_lv)
{
	// 100+ratio x hit
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = -62;
			break;
		case 2:
			ratio = -24;
			break;
		case 3:
			ratio = 14;
			break;
		case 4:
			ratio = 52;
			break;
		case 5:
			ratio = 110;
			break;
	}
	return ratio;
}

int ArcherSkillAtkRatioCalculator::calculate_arrow_shower_atk_ratio(int base_lv, int skill_lv)
{
	int ratio = 0;
	
	switch (skill_lv) {
		case 1:
			ratio = -71;
			break;
		case 2:
			ratio = -42;
			break;
		case 3:
			ratio = -13;
			break;
		case 4:
			ratio = 16;
			break;
		case 5:
			ratio = 45;
			break;
	}
	return ratio;
	
}


int ArcherSkillAtkRatioCalculator::calculate_arrow_rain_atk_ratio(int base_lv, int skill_lv, struct block_list* src)
{
	int ratio = 0;
	struct status_data* status;
	status = status_get_status_data(src);
	switch (skill_lv) {
		case 1:
			ratio = -71;
			break;
		case 2:
			ratio = -42;
			break;
		case 3:
			ratio = -13;
			break;
		case 4:
			ratio = 16;
			break;
		case 5:
			ratio = 45;
			break;
	}
	return ratio + (status->int_/3);
	
}

int ArcherSkillAtkRatioCalculator::calculate_charge_arrow_atk_ratio(int base_lv, int skill_lv)
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

int ArcherSkillAtkRatioCalculator::calculate_spiritual_strafe_atk_ratio(int skill_lv, struct block_list* src)
{
	int ratio = 0;
	struct status_data *status;
	status = status_get_status_data(src);
	switch (skill_lv) {
		case 1:
			ratio = 30 + (status->int_/2);
			break;
		case 2:
			ratio = 100 + (status->int_/2);
			break;
		case 3:
			ratio = 180 + (status->int_/2);
			break;
		case 4:
			ratio = 240 + (status->int_/2);
			break;
		case 5:
			ratio = 300 + (status->int_/2);
			break;
	}
	return ratio;
}

void ArcherSkillAtkRatioCalculator::add_tranquilizer_shot_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_SLEEPATTACK, AREA);
}

void ArcherSkillAtkRatioCalculator::add_paralyzing_shot_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_NPC_SLOWCAST, AREA);
	// clif_specialeffect(target, EF_MAXPAIN, AREA);
}

void ArcherSkillAtkRatioCalculator::add_arrow_shower_special_effects(struct block_list *target)
{
	int effect = rnd() % 2 + 1;
	if( effect == 1) {
		clif_specialeffect(target, 1278, AREA);
		return;
	}
	clif_specialeffect(target, 1281, AREA);
}

void ArcherSkillAtkRatioCalculator::add_arrow_rain_special_effects(struct block_list *target)
{
	int effect = rnd() % 2 + 1;
	if( effect == 1) {
		clif_specialeffect(target, 1279, AREA);
		return;
	}
	clif_specialeffect(target, 1280, AREA);
}

void ArcherSkillAtkRatioCalculator::add_charge_arrow_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_KASUMIKIRI, AREA);
}


void ArcherSkillAtkRatioCalculator::add_spiritual_strafe_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_DELUGE, AREA);
}
