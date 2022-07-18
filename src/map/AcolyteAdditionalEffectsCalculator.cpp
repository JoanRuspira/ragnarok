#include "AcolyteAdditionalEffectsCalculator.h"



void AcolyteAdditionalEffectsCalculator::apply_holy_light_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_BLIND, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
}
