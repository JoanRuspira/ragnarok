#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"
class SwordsmanAdditionalEffectsCalculator
{
public:
	static void apply_bash_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
	static void apply_traumatic_blow_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);

private:
	static void add_bash_special_effects(struct block_list *target);
};
