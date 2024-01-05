#include "AlchemistAdditionalEffectsCalculator.h"


void AlchemistAdditionalEffectsCalculator::apply_acid_terror_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	if (bl->type == BL_PC) {
		if (skill_break_equip(src, bl, EQP_ARMOR, (200 * skill_lv), BCT_ENEMY)){
			clif_emotion(bl, ET_HUK);
    	}
	}else {
		int rate = rnd() % 100;
		if (rate <= (2*skill_lv)) {
			sc_start(src, bl, SC_INCDEFRATE, 100, -30, 10000);
			clif_emotion(bl, ET_HUK);
		}
	}
}

void AlchemistAdditionalEffectsCalculator::apply_bomb_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	if (bl->type == BL_PC) {
		if (skill_break_equip(src, bl, EQP_WEAPON, (200 * skill_lv), BCT_ENEMY)){
			clif_emotion(bl, ET_HUK);
    	}
	}else {
		int rate = rnd() % 100;
		if (rate <= (2*skill_lv)) {
			sc_start(src, bl, SC_INCATKRATE, 100, -30, 10000);
			clif_emotion(bl, ET_HUK);
		}	
	}
}
