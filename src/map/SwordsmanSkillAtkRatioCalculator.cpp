#include "SwordsmanSkillAtkRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int SwordsmanSkillAtkRatioCalculator::calculate_skill_atk_ratio(int base_lv, int skill_id, int skill_lv)
{
	switch (skill_id) {
		case SM_BASH:
			return calculate_bash_atk_ratio(skill_lv);
			break;
		case SM_MAGNUM:
			return calculate_magnum_break_atk_ratio(skill_lv);
			break;
		case KN_SPEARSTAB:
			return calculate_spear_stab_atk_ratio(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

void SwordsmanSkillAtkRatioCalculator::add_skill_special_effect(struct block_list* src, struct block_list *target, int skill_id)
{
	switch (skill_id) {
		case KN_SPEARSTAB:
			clif_specialeffect(src, EF_SPINEDBODY, AREA);
			clif_specialeffect(target, EF_BASH3D, AREA);
			break;
		}
}

int SwordsmanSkillAtkRatioCalculator::calculate_bash_atk_ratio(int skill_lv)
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

int SwordsmanSkillAtkRatioCalculator::calculate_magnum_break_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 0;
			break;
		case 2:
			ratio = 15;
			break;
		case 3:
			ratio = 35;
			break;
		case 4:
			ratio = 55;
			break;
		case 5:
			ratio = 75;
			break;
		}
	return ratio;
}

int SwordsmanSkillAtkRatioCalculator::calculate_spear_stab_atk_ratio(int skill_lv)
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

