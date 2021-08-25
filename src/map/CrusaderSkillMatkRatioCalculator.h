#pragma once
class CrusaderSkillMatkRatioCalculator
{
private:
	int _base_lv;
	int _skill_id;
	int _skill_lv;
	int _int;

public:
	CrusaderSkillMatkRatioCalculator(int base_lv, int skill_id, int skill_lv, int int_);
	~CrusaderSkillMatkRatioCalculator();
	int calculate_skill_matk_ratio();

private:
	int calculate_pressure_matk_ratio();
	int calculate_genesis_ray_matk_ratio();
};
