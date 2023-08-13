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
		case MG_EARTHBOLT:
			add_earth_bolt_special_effects(target);
			return calculate_bolt_attack(skill_lv);
			break;
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

void MageSkillAtkRatioCalculator::add_earth_bolt_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_EARTHHIT, AREA);
	clif_specialeffect(target, EF_EL_UPHEAVAL, AREA);
	clif_specialeffect(target, EF_EL_CURSED_SOIL, AREA);
	clif_specialeffect(target, EF_EL_PETROLOGY, AREA);
}

int MageSkillAtkRatioCalculator::calculate_undead_embrace_attack(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 10;
			break;
		case 2:
			ratio = 120;
			break;
		case 3:
			ratio = 230;
			break;
		case 4:
			ratio = 340;
			break;
		case 5:
			ratio = 450;
			break;
		}
	return ratio;
}

int MageSkillAtkRatioCalculator::calculate_bolt_attack(int skill_lv)
{
	int ratio = 10;
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
	// clif_specialeffect(target, EF_DEVIL, AREA);
	// clif_specialeffect(target, EF_BLACKDEVIL, AREA);
	clif_specialeffect(target, 1238, AREA);
	clif_specialeffect(target, EF_KO_ZENKAI_WIND, AREA);
}
