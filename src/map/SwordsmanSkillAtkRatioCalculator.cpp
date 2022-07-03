#include "SwordsmanSkillAtkRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int SwordsmanSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv)
{
	switch (skill_id) {
		case SM_BASH:
			add_bash_special_effects(target);
			return calculate_bash_atk_ratio(skill_lv);
			break;
		case SM_MAGNUM:
			return calculate_magnum_break_atk_ratio(skill_lv);
			break;
		case KN_SPEARSTAB:
			add_spear_stab_special_effects(src, target);
			return calculate_spear_stab_atk_ratio(skill_lv, target);
			break;
		case LK_HEADCRUSH:
			add_traumatic_blow_special_effects(src, target);
			calculate_traumatic_blow_atk_ratio(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

void SwordsmanSkillAtkRatioCalculator::add_spear_stab_special_effects(struct block_list* src, struct block_list *target)
{
	clif_specialeffect(src, EF_BASH3D, AREA);
}


void SwordsmanSkillAtkRatioCalculator::add_traumatic_blow_special_effects(struct block_list* src, struct block_list *target)
{
	clif_specialeffect(target, EF_MADNESS_RED, AREA);
	clif_specialeffect(target, EF_CRITICALWOUND, AREA);
}

void SwordsmanSkillAtkRatioCalculator::add_bash_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_STUNATTACK, AREA);
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

int SwordsmanSkillAtkRatioCalculator::calculate_spear_stab_atk_ratio(int skill_lv, struct block_list *target)
{
	int ratio = 0;
	struct status_change *target_status;

	target_status = status_get_sc(target);
	switch (skill_lv) {
		case 1:
			ratio = 30;
			break;
		case 2:
			ratio = 100;
			break;
		case 3:
			ratio = 180;
			if (target_status->data[SC_BLEEDING]){
				ratio = 200;
			}
			break;
		case 4:
			ratio = 240;
			if (target_status->data[SC_BLEEDING]){
				ratio = 300;
			}
			break;
		case 5:
			ratio = 300;
			if (target_status->data[SC_BLEEDING]){
				ratio = 400;
			}
			break;
	}
	return ratio;
}

int SwordsmanSkillAtkRatioCalculator::calculate_traumatic_blow_atk_ratio(int skill_lv)
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

