#include "WizardAdditionalEffectsCalculator.h"




void WizardAdditionalEffectsCalculator::apply_crimson_rock_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}

void WizardAdditionalEffectsCalculator::apply_iceberg_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_FREEZE, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}

void WizardAdditionalEffectsCalculator::apply_stalagmite_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STONE, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}
