#include "MageAdditionalEffectsCalculator.h"



void MageAdditionalEffectsCalculator::apply_frost_diver_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_FREEZE, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
}

void MageAdditionalEffectsCalculator::apply_stone_curse_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STONE, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
	MageAdditionalEffectsCalculator::add_stone_curse_special_effects(bl);
}


void MageAdditionalEffectsCalculator::add_stone_curse_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_STONECURSE, AREA);
}
