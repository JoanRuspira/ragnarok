#include "SkillAdditionalEffects.h"
//
///*==========================================
// * Add effect to skill when hit succesfully target
// *------------------------------------------*/
int SkillAdditionalEffects::skill_additional_effect(struct block_list* src, struct block_list *bl, uint16 skill_id, uint16 skill_lv, int attack_type, enum damage_lv dmg_lv, t_tick tick)
{
	struct map_session_data *sd, *dstsd;
	struct mob_data *md, *dstmd;
	struct status_data *sstatus, *tstatus;
	struct status_change *sc, *tsc;
	int skill;
	int rate;

	nullpo_ret(src);
	nullpo_ret(bl);

	if (skill_id > 0 && !skill_lv) return 0;	// don't forget auto attacks! - celest

	if (dmg_lv < ATK_BLOCK) // Don't apply effect if miss.
		return 0;

	sd = BL_CAST(BL_PC, src);
	md = BL_CAST(BL_MOB, src);
	dstsd = BL_CAST(BL_PC, bl);
	dstmd = BL_CAST(BL_MOB, bl);

	sc = status_get_sc(src);
	tsc = status_get_sc(bl);
	sstatus = status_get_status_data(src);
	tstatus = status_get_status_data(bl);

	if (!tsc) //skill additional effect is about adding effects to the target...
		//So if the target can't be inflicted with statuses, this is pointless.
		return 0;

	skill_trigger_status_even_by_blocked_damage(src, bl, sd, skill_id, skill_lv, rate, attack_type, dmg_lv);

	if (dmg_lv < ATK_DEF) // no damage, return;
		return 0;

	player_skill_additional_effect(src, bl, sd, sstatus, sc, dstmd, skill, skill_id, skill_lv, rate, attack_type, tick);
	monster_skill_additional_effect(src, bl, sstatus, skill_id, skill_lv, rate);
	
	if (md && battle_config.summons_trigger_autospells && md->master_id && md->special_state.ai)
	{	//Pass heritage to Master for status causing effects. [Skotlex]
		sd = map_id2sd(md->master_id);
		src = sd ? &sd->bl : src;
	}

	// Coma
	//if (sd && sd->special_state.bonus_coma && (!md || util::vector_exists(status_get_race2(&md->bl), RC2_GVG) || status_get_class(&md->bl) != CLASS_BATTLEFIELD)) {
	//	rate = 0;
	//	//! TODO: Filter the skills that shouldn't inflict coma bonus, to avoid some non-damage skills inflict coma. [Cydh]
	//	if (!skill_id || !skill_get_nk(skill_id, NK_NODAMAGE)) {
	//		rate += sd->indexed_bonus.coma_class[tstatus->class_] + sd->indexed_bonus.coma_class[CLASS_ALL];
	//		rate += sd->indexed_bonus.coma_race[tstatus->race] + sd->indexed_bonus.coma_race[RC_ALL];
	//	}
	//	if (attack_type&BF_WEAPON) {
	//		rate += sd->indexed_bonus.weapon_coma_ele[tstatus->def_ele] + sd->indexed_bonus.weapon_coma_ele[ELE_ALL];
	//		rate += sd->indexed_bonus.weapon_coma_race[tstatus->race] + sd->indexed_bonus.weapon_coma_race[RC_ALL];
	//		rate += sd->indexed_bonus.weapon_coma_class[tstatus->class_] + sd->indexed_bonus.weapon_coma_class[CLASS_ALL];
	//	}
	//	if (rate > 0)
	//		status_change_start(src, bl, SC_COMA, rate, 0, 0, src->id, 0, 0, SCSTART_NONE);
	//}

	if (attack_type&BF_WEAPON)
	{ // Breaking Equipment
		if (sd && battle_config.equip_self_break_rate)
		{	// Self weapon breaking
			rate = battle_config.equip_natural_break_rate;
			if (rate)
				skill_break_equip(src, src, EQP_WEAPON, rate, BCT_SELF);
		}
		if (battle_config.equip_skill_break_rate && skill_id != WS_CARTTERMINATION && skill_id != ITM_TOMAHAWK)
		{	// Cart Termination/Tomahawk won't trigger breaking data. Why? No idea, go ask Gravity.
			// Target weapon breaking
			rate = 0;
			if (sd)
				rate += sd->bonus.break_weapon_rate;
			if (sc && sc->data[SC_MELTDOWN])
				rate += sc->data[SC_MELTDOWN]->val2;
			if (rate)
				skill_break_equip(src, bl, EQP_WEAPON, rate, BCT_ENEMY);

			// Target armor breaking
			rate = 0;
			if (sd)
				rate += sd->bonus.break_armor_rate;
			if (sc && sc->data[SC_MELTDOWN])
				rate += sc->data[SC_MELTDOWN]->val3;
			if (rate)
				skill_break_equip(src, bl, EQP_ARMOR, rate, BCT_ENEMY);
		}
		if (sd && !skill_id && bl->type == BL_PC) { // This effect does not work with skills.
			if (sd->def_set_race[tstatus->race].rate)
				status_change_start(src, bl, SC_DEFSET, sd->def_set_race[tstatus->race].rate, sd->def_set_race[tstatus->race].value,
					0, 0, 0, sd->def_set_race[tstatus->race].tick, SCSTART_NOTICKDEF);
			if (sd->mdef_set_race[tstatus->race].rate)
				status_change_start(src, bl, SC_MDEFSET, sd->mdef_set_race[tstatus->race].rate, sd->mdef_set_race[tstatus->race].value,
					0, 0, 0, sd->mdef_set_race[tstatus->race].tick, SCSTART_NOTICKDEF);
			if (sd->norecover_state_race[tstatus->race].rate)
				status_change_start(src, bl, SC_NORECOVER_STATE, sd->norecover_state_race[tstatus->race].rate,
					0, 0, 0, 0, sd->norecover_state_race[tstatus->race].tick, SCSTART_NONE);
		}
	}

	if (sd && sd->ed && sc && !status_isdead(bl) && !skill_id) {
		struct unit_data *ud = unit_bl2ud(src);

		if (sc->data[SC_WILD_STORM_OPTION])
			skill = sc->data[SC_WILD_STORM_OPTION]->val2;
		else if (sc->data[SC_UPHEAVAL_OPTION])
			skill = sc->data[SC_UPHEAVAL_OPTION]->val3;
		else if (sc->data[SC_TROPIC_OPTION])
			skill = sc->data[SC_TROPIC_OPTION]->val3;
		else if (sc->data[SC_CHILLY_AIR_OPTION])
			skill = sc->data[SC_CHILLY_AIR_OPTION]->val3;
		else
			skill = 0;

		if (rnd() % 100 < 25 && skill) {
			skill_castend_damage_id(src, bl, skill, 5, tick, 0);

			if (ud) {
				rate = skill_delayfix(src, skill, skill_lv);
				if (DIFF_TICK(ud->canact_tick, tick + rate) < 0) {
					ud->canact_tick = i64max(tick + rate, ud->canact_tick);
					if (battle_config.display_status_timers)
						clif_status_change(src, EFST_POSTDELAY, 1, rate, 0, 0, 0);
				}
			}
		}
	}

	// Autospell when attacking
	if (sd && !status_isdead(bl) && !sd->autospell.empty())
	{
		struct block_list *tbl;
		struct unit_data *ud;
		int autospl_skill_lv, type;

		for (const auto &it : sd->autospell) {
			if (!(((it.flag)&attack_type)&BF_WEAPONMASK &&
				((it.flag)&attack_type)&BF_RANGEMASK &&
				((it.flag)&attack_type)&BF_SKILLMASK))
				continue; // one or more trigger conditions were not fulfilled

			skill = (it.id > 0) ? it.id : -it.id;

			sd->state.autocast = 1;
			if (skill_isNotOk(skill, sd)) {
				sd->state.autocast = 0;
				continue;
			}
			sd->state.autocast = 0;

			autospl_skill_lv = it.lv ? it.lv : 1;
			if (autospl_skill_lv < 0) autospl_skill_lv = 1 + rnd() % (-autospl_skill_lv);

			rate = (!sd->state.arrow_atk) ? it.rate : it.rate / 2;

			if (rnd() % 1000 >= rate)
				continue;

			tbl = (it.id < 0) ? src : bl;

			if ((type = skill_get_casttype(skill)) == CAST_GROUND) {
				if (!skill_pos_maxcount_check(src, tbl->x, tbl->y, skill, autospl_skill_lv, BL_PC, false))
					continue;
			}
			if (battle_config.autospell_check_range &&
				!battle_check_range(bl, tbl, skill_get_range2(src, skill, autospl_skill_lv, true)))
				continue;

			if (skill == PF_SPIDERWEB) //Special case, due to its nature of coding.
				type = CAST_GROUND;

			sd->state.autocast = 1;
			skill_consume_requirement(sd, skill, autospl_skill_lv, 1);
			skill_toggle_magicpower(src, skill);
			switch (type) {
			case CAST_GROUND:
				skill_castend_pos2(src, tbl->x, tbl->y, skill, autospl_skill_lv, tick, 0);
				break;
			case CAST_NODAMAGE:
				skill_castend_nodamage_id(src, tbl, skill, autospl_skill_lv, tick, 0);
				break;
			case CAST_DAMAGE:
				skill_castend_damage_id(src, tbl, skill, autospl_skill_lv, tick, 0);
				break;
			}
			sd->state.autocast = 0;
			//Set canact delay. [Skotlex]
			ud = unit_bl2ud(src);
			if (ud) {
				rate = skill_delayfix(src, skill, autospl_skill_lv);
				if (DIFF_TICK(ud->canact_tick, tick + rate) < 0) {
					ud->canact_tick = i64max(tick + rate, ud->canact_tick);
					if (battle_config.display_status_timers && sd)
						clif_status_change(src, EFST_POSTDELAY, 1, rate, 0, 0, 0);
				}
			}
		}
	}

	//Autobonus when attacking
	if (sd && !sd->autobonus.empty())
	{
		for (auto &it : sd->autobonus) {
			if (rnd() % 1000 >= it.rate)
				continue;
			if (!(((it.atk_type)&attack_type)&BF_WEAPONMASK &&
				((it.atk_type)&attack_type)&BF_RANGEMASK &&
				((it.atk_type)&attack_type)&BF_SKILLMASK))
				continue; // one or more trigger conditions were not fulfilled
			pc_exeautobonus(sd, &sd->autobonus, &it);
		}
	}

	//Polymorph
	if (sd && sd->bonus.classchange && attack_type&BF_WEAPON &&
		dstmd && !status_has_mode(tstatus, MD_STATUSIMMUNE) &&
		(rnd() % 10000 < sd->bonus.classchange))
	{
		int class_ = mob_get_random_id(MOBG_Branch_Of_Dead_Tree, RMF_DB_RATE, 0);
		if (class_ != 0 && mobdb_checkid(class_))
			mob_class_change(dstmd, class_);
	}

return 0;

}



void SkillAdditionalEffects::player_skill_additional_effect(struct block_list* src, struct block_list *bl, struct map_session_data *sd,
	struct status_data *sstatus, struct status_change *sc, struct mob_data *dstmd, int &skill, uint16 skill_id, uint16 skill_lv, int &rate, int &attack_type, t_tick tick) {
	switch (skill_id) {
		case 0:
			{ // Normal attacks (no skill used)
				if (attack_type&BF_SKILL)
					break; // If a normal attack is a skill, it's splash damage. [Inkfish]
				if (sd) {
					// Automatic trigger of Blitz Beat
					if( sd->ed ) {
						if( sd->ed->elemental.class_ == ELEMENTALID_VENTUS_S) {
							if ((skill = pc_checkskill(sd, SO_SUMMON_VENTUS)) > 0 &&
								rnd() % 1000 <= sstatus->luk * (2*skill) / 5 + 1) {
								skill_castend_nodamage_id(src, bl, HT_FALCON_1, skill, tick, 0);
							}
						}
					}
					// Automatic trigger of Warg Strike
					if( sd->ed ) {
						if( sd->ed->elemental.class_ == ELEMENTALID_AQUA_S) {
							if ((skill = pc_checkskill(sd, SO_SUMMON_AQUA)) > 0 &&
								rnd() % 1000 <= sstatus->luk * (2*skill) / 5 + 1) {
								skill_castend_nodamage_id(src, bl, HT_WARG_1, skill, tick, 0);
							}
						}
					}
					//Mug
					if (dstmd && sd->status.weapon != W_BOW && 
						(skill = pc_checkskill(sd, RG_SNATCHER)) > 0 
						&& rnd() % 100 <= skill * 4)
						skill_castend_damage_id(src, bl, TF_SNATCH, skill, tick, 0, true);
					
					if (sc && sc->data[SC_PYROCLASTIC] && ((rnd() % 100) <= sc->data[SC_PYROCLASTIC]->val3))
						skill_castend_pos2(src, bl->x, bl->y, BS_HAMMERFALL, sc->data[SC_PYROCLASTIC]->val1, tick, 0);
				}

				if (sc) {
					struct status_change_entry *sce;
					// Enchant Poison gives a chance to poison attacked enemies
					if ((sce = sc->data[SC_ENCPOISON])) //Don't use sc_start since chance comes in 1/10000 rate.
						status_change_start(src, bl, SC_POISON, sce->val2, sce->val1, src->id, 0, 0,
							skill_get_time2(AS_ENCHANTPOISON, sce->val1), SCSTART_NONE);
					// Enchant Deadly Poison gives a chance to deadly poison attacked enemies
					if ((sce = sc->data[SC_EDP]))
						sc_start4(src, bl, SC_DPOISON, sce->val2, sce->val1, src->id, 0, 0,
							skill_get_time2(ASC_EDP, sce->val1));

					
				}
			}
			break;

		case SM_BASH:
			SwordsmanAdditionalEffectsCalculator::apply_bash_additional_effect(src, bl, skill_lv);
			break;
		case LK_HEADCRUSH:
			SwordsmanAdditionalEffectsCalculator::apply_traumatic_blow_additional_effect(src, bl, skill_lv);
			break;
		case AC_TRANQUILIZING:
			ArcherAdditionalEffectsCalculator::apply_tranquilizer_shot_additional_effect(src, bl, skill_lv);
			break;
		case AC_PARALIZING:
			ArcherAdditionalEffectsCalculator::apply_paralyzing_shot_additional_effect(src, bl, skill_lv);
			break;
		case AL_HOLYLIGHT:
			AcolyteAdditionalEffectsCalculator::apply_holy_light_additional_effect(src, bl, skill_lv);
			break;
		case AS_VENOMKNIFE:
		case TF_POISON:
			ThiefAdditionalEffectsCalculator::apply_envenom_additional_effect(src, bl, skill_lv);
			break;
		case TF_THROWSTONE:
			ThiefAdditionalEffectsCalculator::apply_throw_stone_additional_effect(src, bl, skill_lv);
			break;
		case TF_SNATCH:
			ThiefAdditionalEffectsCalculator::apply_snatch_additional_effect(src, bl, sd, skill_lv, skill);
			break;
		case TF_SANDATTACK:
			ThiefAdditionalEffectsCalculator::apply_sand_attack_additional_effect(src, bl, skill_lv);
			break;
		case MG_FROSTDIVER:
			MageAdditionalEffectsCalculator::apply_frost_diver_additional_effect(src, bl, skill_lv);
			break;
		case LK_SPIRALPIERCE:
		case KN_AUTOCOUNTER:
			KnightAdditionalEffectsCalculator::apply_auto_counter_additional_effect(src, bl, skill_lv);
			break;
		case PA_SHIELDSLAM:
			KnightAdditionalEffectsCalculator::apply_smite_additional_effect(src, bl, skill_lv);
			break;
		case CR_HOLYCROSS:
		case LG_RAYOFGENESIS:
			CrusaderAdditionalEffectsCalculator::apply_holy_cross_additional_effect(src, bl, skill_lv);
			break;
		case PR_UNHOLYCROSS:
		case HP_PENITENTIA:
			PriestAdditionalEffectsCalculator::apply_unholy_cross_additional_effect(src, bl, skill_lv);
			break;
		case RG_BACKSTAP:
		case GC_COUNTERSLASH:
			RogueAdditionalEffectsCalculator::apply_back_stab_additional_effect(src, bl, skill_lv);
			break;
		case WM_GREAT_ECHO:
			BardAdditionalEffectsCalculator::apply_great_echo_additional_effect(src, bl, skill_lv);
			break;
		case WM_SEVERE_RAINSTORM:
		case WM_SEVERE_RAINSTORM_MELEE:
			BardAdditionalEffectsCalculator::apply_severe_rainstorm_additional_effect(src, bl, skill_lv);
			break;
		case JG_TAROTCARD:
			BardAdditionalEffectsCalculator::apply_tarot_cards_additional_effect(src, bl, skill_lv);
			break;
		case WZ_EARTHSPIKE:
			WizardAdditionalEffectsCalculator::apply_stalagmite_additional_effect(src, bl, skill_lv);
			break;
		case WZ_ICEBERG:
			WizardAdditionalEffectsCalculator::apply_iceberg_additional_effect(src, bl, skill_lv);
			break;
		case WL_CRIMSONROCK:
			WizardAdditionalEffectsCalculator::apply_crimson_rock_additional_effect(src, bl, skill_lv);
			break;
		case AM_ACIDTERROR:
			AlchemistAdditionalEffectsCalculator::apply_acid_terror_additional_effect(src, bl, skill_lv);
			break;
		case GN_SPORE_EXPLOSION:
			AlchemistAdditionalEffectsCalculator::apply_bomb_additional_effect(src, bl, skill_lv);
			break;
		case MO_BALKYOUNG:
			MonkAdditionalEffectsCalculator::apply_excruciating_fist_additional_effect(src, bl, skill_lv);
			break;
		case RK_HUNDREDSPEAR:
			KnightAdditionalEffectsCalculator::apply_a_hundred_spears_additional_effect(src, bl, skill_lv, tick);
			break;
		case CG_ARROWVULCAN:
			clif_skill_damage(src,bl,tick, status_get_amotion(src), 0, -30000, 1, DUMMY_ARROWVULCAN, skill_lv, DMG_SINGLE);
			break;
		case AS_SONICBLOW:
			clif_skill_damage(src,bl,tick, status_get_amotion(src), 0, -30000, 1, DUMMY_SONICBLOW, skill_lv, DMG_SINGLE);
			break;
		case GC_CROSSIMPACT:
			clif_skill_damage(src,bl,tick, status_get_amotion(src), 0, -30000, 1, DUMMY_CROSSIMPACT, skill_lv, DMG_SINGLE);
			break;
		case CH_PALMSTRIKE:
			clif_specialeffect(bl, EF_DECAGILITY, AREA);
			sc_start(src, bl, SC_DECREASEAGI, 100, -50, skill_get_time(skill_id, skill_lv));
			break;
		case HW_SHADOWBOMB:
			WizardAdditionalEffectsCalculator::apply_shadow_bomb_additional_effect(src, bl, skill_lv);
			break;
		case HW_PHANTOMSPEAR:
			WizardAdditionalEffectsCalculator::apply_phantom_spear_additional_effect(src, bl, skill_lv);
			break;
		case WZ_LIGHTNINGROD:
			WizardAdditionalEffectsCalculator::apply_lightning_rod_additional_effect(src, bl, skill_lv);
			break;
		case SR_DRAGONCOMBO:
		{
			int percentage = rand()%(100) + 1;
			int margin = skill_lv*2;
			if (percentage <= margin) {
				status_change_start(src, bl, SC_COMA, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
				clif_specialeffect(bl, EF_BLEEDING, AREA);
			}
			break;
		}
	}
}


void SkillAdditionalEffects::monster_skill_additional_effect(struct block_list* src, struct block_list *bl, 
	struct status_data *sstatus, uint16 skill_id, uint16 skill_lv, int &rate) {
	switch (skill_id) {
		case NPC_PETRIFYATTACK:
			sc_start4(src, bl, status_skill2sc(skill_id), (20 * skill_lv),
				skill_lv, 0, 0, skill_get_time(skill_id, skill_lv),
				skill_get_time2(skill_id, skill_lv));
			break;
		case NPC_CURSEATTACK:
		case NPC_SLEEPATTACK:
		case NPC_BLINDATTACK:
		case NPC_POISON:
		case NPC_SILENCEATTACK:
		case NPC_STUNATTACK:
		case NPC_BLEEDING:
			sc_start(src, bl, status_skill2sc(skill_id), (20 * skill_lv), skill_lv, skill_get_time2(skill_id, skill_lv));
			break;
		case NPC_ACIDBREATH:
		case NPC_ICEBREATH:
			sc_start(src, bl, status_skill2sc(skill_id), 70, skill_lv, skill_get_time2(skill_id, skill_lv));
			break;
		case NPC_MENTALBREAKER:
		{	
			rate = sstatus->matk_min;
			if (rate < sstatus->matk_max)
				rate += rnd() % (sstatus->matk_max - sstatus->matk_min);
			rate *= skill_lv;
			status_zap(bl, 0, rate);
			break;
		}
		// Equipment breaking monster skills [Celest]
		case NPC_WEAPONBRAKER:
			skill_break_equip(src, bl, EQP_WEAPON, 150 * skill_lv, BCT_ENEMY);
			break;
		case NPC_ARMORBRAKE:
			skill_break_equip(src, bl, EQP_ARMOR, 150 * skill_lv, BCT_ENEMY);
			break;
		case NPC_HELMBRAKE:
			skill_break_equip(src, bl, EQP_HELM, 150 * skill_lv, BCT_ENEMY);
			break;
		case NPC_SHIELDBRAKE:
			skill_break_equip(src, bl, EQP_SHIELD, 150 * skill_lv, BCT_ENEMY);
			break;

		case NPC_COMET:
			sc_start4(src, bl, SC_BURNING, 100, skill_lv, 1000, src->id, 0, skill_get_time(skill_id, skill_lv));
			break;
		case NPC_JACKFROST:
			sc_start(src, bl, SC_FREEZE, 200, skill_lv, skill_get_time(skill_id, skill_lv));
			break;
	}
}


void SkillAdditionalEffects::skill_trigger_status_even_by_blocked_damage(struct block_list* src, struct block_list *bl, struct map_session_data *sd,
	uint16 skill_id, uint16 skill_lv, int &rate, int &attack_type, enum damage_lv dmg_lv) {

	if (sd)
	{ // These statuses would be applied anyway even if the damage was blocked by some skills. [Inkfish]
		if (skill_id != WS_CARTTERMINATION && skill_id != AM_DEMONSTRATION && skill_id != CR_REFLECTSHIELD && skill_id != MS_REFLECTSHIELD && skill_id != GN_HELLS_PLANT_ATK
			) {
			// Trigger status effects
			enum sc_type type;
			unsigned int time;

			for (const auto &it : sd->addeff) {
				rate = it.rate;
				if (attack_type&BF_LONG) // Any ranged physical attack takes status arrows into account (Grimtooth...) [DracoRPG]
					rate += it.arrow_rate;
				if (!rate)
					continue;

				if ((it.flag&(ATF_WEAPON | ATF_MAGIC | ATF_MISC)) != (ATF_WEAPON | ATF_MAGIC | ATF_MISC)) {
					// Trigger has attack type consideration.
					if ((it.flag&ATF_WEAPON && attack_type&BF_WEAPON) ||
						(it.flag&ATF_MAGIC && attack_type&BF_MAGIC) ||
						(it.flag&ATF_MISC && attack_type&BF_MISC))
						;
					else
						continue;
				}

				if ((it.flag&(ATF_LONG | ATF_SHORT)) != (ATF_LONG | ATF_SHORT)) {
					// Trigger has range consideration.
					if ((it.flag&ATF_LONG && !(attack_type&BF_LONG)) ||
						(it.flag&ATF_SHORT && !(attack_type&BF_SHORT)))
						continue; //Range Failed.
				}

				type = it.sc;
				time = it.duration;

				if (it.flag&ATF_TARGET)
					status_change_start(src, bl, type, rate, 7, 0, (type == SC_BURNING) ? src->id : 0, 0, time, SCSTART_NONE);

				if (it.flag&ATF_SELF)
					status_change_start(src, src, type, rate, 7, 0, (type == SC_BURNING) ? src->id : 0, 0, time, SCSTART_NONE);
			}
		}

		if (skill_id) {
			// Trigger status effects on skills
			enum sc_type type;
			unsigned int time;

			for (const auto &it : sd->addeff_onskill) {
				if (skill_id != it.skill_id || !it.rate)
					continue;
				type = it.sc;
				time = it.duration;

				if (it.target&ATF_TARGET)
					status_change_start(src, bl, type, it.rate, 7, 0, 0, 0, time, SCSTART_NONE);
				if (it.target&ATF_SELF)
					status_change_start(src, src, type, it.rate, 7, 0, 0, 0, time, SCSTART_NONE);
			}
		}
	}

}

bool skill_strip_equip2(struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv)
{
	nullpo_retr(false, src);
	nullpo_retr(false, target);

	struct status_change *tsc = status_get_sc(target);

	if (!tsc || tsc->option&OPTION_MADOGEAR) // Mado Gear cannot be divested [Ind]
		return false;

	const int pos[5] = { EQP_WEAPON, EQP_SHIELD, EQP_ARMOR, EQP_HELM, EQP_ACC };
	const enum sc_type sc_atk[5] = { SC_STRIPWEAPON, SC_STRIPSHIELD, SC_STRIPARMOR, SC_STRIPHELM, SC__STRIPACCESSORY };
	const enum sc_type sc_def[5] = { SC_CP_WEAPON, SC_CP_SHIELD, SC_CP_ARMOR, SC_CP_HELM, SC_NONE };
	struct status_data *sstatus = status_get_status_data(src), *tstatus = status_get_status_data(target);
	int rate, time, location, mod = 100;

	// switch (skill_id) { // Rate
	// case RG_STRIPWEAPON:
	// case RG_STRIPARMOR:
	// case RG_STRIPSHIELD:
	// case RG_STRIPHELM:
	// case GC_WEAPONCRUSH:
	// 	rate = 50 * (skill_lv + 1) + 2 * (sstatus->dex - tstatus->dex);
	// 	mod = 1000;
	// 	break;
	// case ST_FULLSTRIP: {
		
	// 	rate = 100;
	
	// 	break;
	// }
	// case GS_DISARM:
	// 	rate = sstatus->dex / (4 * (7 - skill_lv)) + sstatus->luk / (4 * (6 - skill_lv));
	// 	rate = rate + status_get_lv(src) - (tstatus->agi * rate / 100) - tstatus->luk - status_get_lv(target);
	// 	break;
	// case WL_EARTHSTRAIN: {
	// 	int job_lv = 0;

	// 	if (src->type == BL_PC)
	// 		job_lv = ((TBL_PC*)src)->status.job_level;
	// 	rate = 6 * skill_lv + job_lv / 4 + sstatus->dex / 10;
	// 	break;
	// }
	// case SC_STRIPACCESSORY:
	// case SC_STRIPACCESSARY:
	// 	// rate = 12 + 2 * skill_lv;
	// 	rate = 100;
	// 	break;
	// default:
	// 	return false;
	// }

	// if (rnd() % mod >= rate)
	// 	return false;

	switch (skill_id) { // Duration
		case SC_STRIPACCESSORY:
		case SC_STRIPACCESSARY:
			time = skill_get_time(skill_id, skill_lv);
			break;
		case ST_FULLSTRIP:
			if (skill_id == WL_EARTHSTRAIN)
				time = skill_get_time2(skill_id, skill_lv);
			else
				time = skill_get_time(skill_id, skill_lv);
			break;
	}

	switch (skill_id) { // Location
		case ST_FULLSTRIP:
			location = EQP_WEAPON | EQP_SHIELD | EQP_ARMOR | EQP_HELM;
			break;
		case SC_STRIPACCESSORY:
		case SC_STRIPACCESSARY:
			location = EQP_ACC;
			break;
	}

	for (uint8 i = 0; i < ARRAYLENGTH(pos); i++) {
		if (location&pos[i] && sc_def[i] > SC_NONE && tsc->data[sc_def[i]])
			location &= ~pos[i];
	}
	if (!location)
		return false;

	for (uint8 i = 0; i < ARRAYLENGTH(pos); i++) {
		if (location&pos[i] && !sc_start(src, target, sc_atk[i], 100, skill_lv, time))
			location &= ~pos[i];
	}
	return location ? true : false;

}

