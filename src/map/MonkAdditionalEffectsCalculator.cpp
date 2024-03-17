#include "MonkAdditionalEffectsCalculator.h"




void MonkAdditionalEffectsCalculator::apply_excruciating_fist_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, STATUS_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}
