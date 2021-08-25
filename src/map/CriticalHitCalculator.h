#pragma once
#include <cstdint>


class CriticalHitCalculator
{
public:
	static bool is_attack_critical(struct Damage* wd, struct block_list *src, struct block_list *target, int skill_id, int skill_lv, bool first_call);
	static bool is_skill_using_arrow(struct block_list *src, int skill_id);

};
