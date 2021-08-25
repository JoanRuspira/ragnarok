#pragma once
class SwordsmanSkillAtkRatioCalculator
{
private:
	int _base_lv;
	int _skill_id;
	int _skill_lv;
	int _int;
	int _weapon_element;

public:
	SwordsmanSkillAtkRatioCalculator(int base_lv, int skill_id, int skill_lv, int int_, int _weapon_element);
	int calculate_skill_atk_ratio();

private:
	int calculate_bash_atk_ratio();
	int calculate_magnum_break_atk_ratio();
};
