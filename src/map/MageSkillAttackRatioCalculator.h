#pragma once
class MageSkillAtkRatioCalculator
{
private:

public:
	static int calculate_skill_atk_ratio(int base_lv, int skill_id, int skill_lv);

private:
	static int calculate_cold_bolt_attack(int skill_lv);
	/*static int calculate_magnum_break_atk_ratio(int skill_lv);
	static int calculate_spear_stab_atk_ratio(int skill_lv);*/
};