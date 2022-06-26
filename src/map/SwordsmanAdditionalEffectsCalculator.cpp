#include "SwordsmanAdditionalEffectsCalculator.h"



void SwordsmanAdditionalEffectsCalculator::calculate_bash_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}

void SwordsmanAdditionalEffectsCalculator::calculate_spear_stab_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv, int flag, int skill_area_temp[8], t_tick tick)
{
	status_change_start(src, bl, SC_BLEEDING, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
	if (flag & 1) {
		if (bl->id == skill_area_temp[1]) {
			return;
		}

		if (skill_attack(BF_WEAPON, src, src, bl, KN_SPEARSTAB, skill_lv, tick, SD_ANIMATION))
			skill_blown(src, bl, skill_area_temp[2], -1, BLOWN_NONE);
	}
	else {
		int x = bl->x, y = bl->y, i, dir;
		dir = map_calc_dir(bl, src->x, src->y);
		skill_area_temp[1] = bl->id;
		skill_area_temp[2] = skill_get_blewcount(KN_SPEARSTAB, skill_lv);
		// all the enemies between the caster and the target are hit, as well as the target
		if (skill_attack(BF_WEAPON, src, src, bl, KN_SPEARSTAB, skill_lv, tick, 0))
			skill_blown(src, bl, skill_area_temp[2], -1, BLOWN_NONE);
		for (i = 0; i < 4; i++) {
			map_foreachincell(skill_area_sub, bl->m, x, y, BL_CHAR,
				src, KN_SPEARSTAB, skill_lv, tick, flag | BCT_ENEMY | 1, skill_castend_damage_id);
			x += dirx[dir];
			y += diry[dir];
		}
	}
}

