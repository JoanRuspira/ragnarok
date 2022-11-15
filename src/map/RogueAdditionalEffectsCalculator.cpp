#include "RogueAdditionalEffectsCalculator.h"


void RogueAdditionalEffectsCalculator::apply_back_stab_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 500, SCSTART_NONE);
}
