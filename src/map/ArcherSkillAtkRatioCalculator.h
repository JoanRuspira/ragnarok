#pragma once
class ArcherSkillAtkRatioCalculator
{
private:

public:
	static int calculate_skill_atk_ratio(int base_lv, int skill_id, int skill_lv);

private:
	static int calculate_double_strafe_atk_ratio(int base_lv, int skill_lv);
	static int calculate_arrow_shower_atk_ratio(int base_lv, int skill_lv);
};
