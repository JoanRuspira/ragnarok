#include "AlchemistSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int AlchemistSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case HM_BASILISK_1:
		case AM_EL_ACTION:
			calculate_basilisk_1_special_effects(target);
			return calculate_basilisk_1_attack_ratio(skill_lv);
			break;
		case HM_BEHOLDER_1:
			calculate_beholder_1_special_effects(target);
			return calculate_basilisk_1_attack_ratio(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

int AlchemistSkillAttackRatioCalculator::calculate_basilisk_1_attack_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 110;
			break;
		case 2:
			ratio = 220;
			break;
		case 3:
			ratio = 330;
			break;
		case 4:
			ratio = 440;
			break;
		case 5:
			ratio = 550;
			break;
		}
	return ratio;
}

void AlchemistSkillAttackRatioCalculator::calculate_basilisk_1_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_HFLIMOON3, AREA);
}

void AlchemistSkillAttackRatioCalculator::calculate_beholder_1_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_REDLINE, AREA);
}