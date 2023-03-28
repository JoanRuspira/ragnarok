#include "WizardSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int WizardSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case WZ_ICEBERG:
			add_iceberg_special_effects(target);
			return calculate_bolt2_attack(skill_lv);
			break;
		case WZ_EARTHSPIKE:
			add_stalagmite_special_effects(target);
			return calculate_bolt2_attack(skill_lv);
			break;
		case WL_CRIMSONROCK:
			return calculate_bolt2_attack(skill_lv);
			break;
		case WZ_JUPITEL:
			return calculate_bolt2_attack(skill_lv);
			break;
		case SL_SMA:
			return calculate_bolt2_attack(skill_lv);
			break;
		case WZ_CORRUPT:
			add_corrupt_special_effects(target);
			return calculate_corrupt_atk_ratio(skill_lv);
			break;
		case WZ_EXTREMEVACUUM:
			add_extreme_vacuum_special_effects(target);
			return calculate_extreme_vacuum_atk_ratio(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

int WizardSkillAttackRatioCalculator::calculate_bolt2_attack(int skill_lv)
{
	int ratio = 50;
	return ratio;
}

void WizardSkillAttackRatioCalculator::add_corrupt_special_effects(struct block_list *target)
{
    clif_specialeffect(target, 1277, AREA);
    clif_specialeffect(target, 1238, AREA);
    clif_specialeffect(target, 129, AREA);
}

void WizardSkillAttackRatioCalculator::add_extreme_vacuum_special_effects(struct block_list *target)
{
    clif_specialeffect(target, 1323, AREA);
}

int WizardSkillAttackRatioCalculator::calculate_extreme_vacuum_atk_ratio(int skill_lv)
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
	return ratio;
}

int WizardSkillAttackRatioCalculator::calculate_corrupt_atk_ratio(int skill_lv)
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
	return ratio;
}

void WizardSkillAttackRatioCalculator::add_iceberg_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_HYOUSYOURAKU, AREA);
}

void WizardSkillAttackRatioCalculator::add_stalagmite_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_KEEPING, AREA);
}

