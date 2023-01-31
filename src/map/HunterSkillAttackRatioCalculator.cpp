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
		case RA_WUGSTRIKE:
			add_slash_special_effects(target);
			break;
		default:
			return 0;
			break;
	}
}

void HunterSkillAttackRatioCalculator::add_slash_special_effects(struct block_list *target)
{
    // clif_specialeffect(target, EF_PEONG, AREA);
	// clif_specialeffect(target, EF_BANISHING_BUSTER, AREA);
    // clif_specialeffect(target, 1242, AREA);
}

