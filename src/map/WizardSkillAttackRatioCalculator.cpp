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
			return calculate_iceberg_atk_ratio(skill_lv);
			break;
		case WZ_STALAGMITE:
			add_stalagmite_special_effects(target);
			return calculate_stalagmite_atk_ratio(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

void WizardSkillAttackRatioCalculator::add_iceberg_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_HYOUSYOURAKU, AREA);
}

int WizardSkillAttackRatioCalculator::calculate_iceberg_atk_ratio(int skill_lv)
{
	return skill_lv*100;
}

void WizardSkillAttackRatioCalculator::add_stalagmite_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_KEEPING, AREA);
	clif_specialeffect(target, EF_EARTHSPIKE, AREA);
	clif_specialeffect(target, EF_EARTHHIT, AREA);
	clif_specialeffect(target, EF_EL_CURSED_SOIL, AREA);
}

int WizardSkillAttackRatioCalculator::calculate_stalagmite_atk_ratio(int skill_lv)
{
	return skill_lv*100;
}
