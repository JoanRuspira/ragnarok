#include "MageSkillAttackRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"

/**
 * ATK ratio calculater for Mage skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int MageSkillAtkRatioCalculator::calculate_skill_atk_ratio(int base_lv, int skill_id, int skill_lv, struct block_list *target)
{
	switch (skill_id) {
		case WZ_EARTHSPIKE:
		case MG_LIGHTNINGBOLT:
		case MG_COLDBOLT:
		case MG_FIREBOLT:
			return calculate_bolt_attack(skill_lv);
			break;
		case MG_UNDEADEMBRACE:
			add_undead_embrace_special_effects(target);
			return calculate_undead_embrace_attack(skill_lv);
			break;
		case NPC_DARKSTRIKE:
		case MG_SOULSTRIKE:
			return calculate_soul_strike_attack(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

int MageSkillAtkRatioCalculator::calculate_undead_embrace_attack(int skill_lv)
{
	int ratio = 0;
	ratio = skill_lv * 100;
	return ratio;
}

int MageSkillAtkRatioCalculator::calculate_bolt_attack(int skill_lv)
{
	int ratio = 40;
	return ratio;
}


int MageSkillAtkRatioCalculator::calculate_soul_strike_attack(int skill_lv)
{
	int ratio = 0;
	ratio = 5 * skill_lv;
	return ratio;
}

void MageSkillAtkRatioCalculator::add_undead_embrace_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_DEVIL, AREA);
	clif_specialeffect(target, EF_BLACKDEVIL, AREA);
	clif_specialeffect(target, EF_KO_ZENKAI_WIND, AREA);
}
