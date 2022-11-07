#include "KnightAdditionalEffectsCalculator.h"



void KnightAdditionalEffectsCalculator::apply_auto_counter_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_BLEEDING, 10000, skill_lv, 0, 0, 0, 360000, SCSTART_NONE);
}

void KnightAdditionalEffectsCalculator::apply_smite_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}
