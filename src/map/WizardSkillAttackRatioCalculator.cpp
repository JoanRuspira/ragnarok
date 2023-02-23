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

void WizardSkillAttackRatioCalculator::add_iceberg_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_HYOUSYOURAKU, AREA);
}

void WizardSkillAttackRatioCalculator::add_stalagmite_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_KEEPING, AREA);
}

