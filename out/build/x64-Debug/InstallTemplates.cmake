macro( INSTALL_TEMPLATE _SRC _DST )
  set( SRC "F:/ragnarok/server/${_SRC}" )
  set( DST "${CMAKE_INSTALL_PREFIX}/${_DST}" )
  if( EXISTS "${DST}" )
    message( "-- Already exists: ${DST}" )
  else()
    message( "-- Installing template: ${DST}" )
    execute_process( COMMAND "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" -E copy "${SRC}" "${DST}" )
  endif()
endmacro()
INSTALL_TEMPLATE( "conf/import-tmpl/atcommands.yml" "conf/import/atcommands.yml" )
INSTALL_TEMPLATE( "conf/import-tmpl/battle_conf.txt" "conf/import/battle_conf.txt" )
INSTALL_TEMPLATE( "conf/import-tmpl/char_conf.txt" "conf/import/char_conf.txt" )
INSTALL_TEMPLATE( "conf/import-tmpl/inter_conf.txt" "conf/import/inter_conf.txt" )
INSTALL_TEMPLATE( "conf/import-tmpl/inter_server.yml" "conf/import/inter_server.yml" )
INSTALL_TEMPLATE( "conf/import-tmpl/log_conf.txt" "conf/import/log_conf.txt" )
INSTALL_TEMPLATE( "conf/import-tmpl/login_conf.txt" "conf/import/login_conf.txt" )
INSTALL_TEMPLATE( "conf/import-tmpl/map_conf.txt" "conf/import/map_conf.txt" )
INSTALL_TEMPLATE( "conf/import-tmpl/packet_conf.txt" "conf/import/packet_conf.txt" )
INSTALL_TEMPLATE( "conf/import-tmpl/script_conf.txt" "conf/import/script_conf.txt" )
INSTALL_TEMPLATE( "conf/msg_conf/import-tmpl/map_msg_chn_conf.txt" "conf/msg_conf/import/map_msg_chn_conf.txt" )
INSTALL_TEMPLATE( "conf/msg_conf/import-tmpl/map_msg_eng_conf.txt" "conf/msg_conf/import/map_msg_eng_conf.txt" )
INSTALL_TEMPLATE( "conf/msg_conf/import-tmpl/map_msg_frn_conf.txt" "conf/msg_conf/import/map_msg_frn_conf.txt" )
INSTALL_TEMPLATE( "conf/msg_conf/import-tmpl/map_msg_grm_conf.txt" "conf/msg_conf/import/map_msg_grm_conf.txt" )
INSTALL_TEMPLATE( "conf/msg_conf/import-tmpl/map_msg_idn_conf.txt" "conf/msg_conf/import/map_msg_idn_conf.txt" )
INSTALL_TEMPLATE( "conf/msg_conf/import-tmpl/map_msg_mal_conf.txt" "conf/msg_conf/import/map_msg_mal_conf.txt" )
INSTALL_TEMPLATE( "conf/msg_conf/import-tmpl/map_msg_por_conf.txt" "conf/msg_conf/import/map_msg_por_conf.txt" )
INSTALL_TEMPLATE( "conf/msg_conf/import-tmpl/map_msg_rus_conf.txt" "conf/msg_conf/import/map_msg_rus_conf.txt" )
INSTALL_TEMPLATE( "conf/msg_conf/import-tmpl/map_msg_spn_conf.txt" "conf/msg_conf/import/map_msg_spn_conf.txt" )
INSTALL_TEMPLATE( "conf/msg_conf/import-tmpl/map_msg_tha_conf.txt" "conf/msg_conf/import/map_msg_tha_conf.txt" )
INSTALL_TEMPLATE( "db/import/abra_db.yml" "db/import/abra_db.yml" )
INSTALL_TEMPLATE( "db/import/achievement_db.yml" "db/import/achievement_db.yml" )
INSTALL_TEMPLATE( "db/import/achievement_level_db.yml" "db/import/achievement_level_db.yml" )
INSTALL_TEMPLATE( "db/import/attendance.yml" "db/import/attendance.yml" )
INSTALL_TEMPLATE( "db/import/attr_fix.txt" "db/import/attr_fix.txt" )
INSTALL_TEMPLATE( "db/import/battleground_db.yml" "db/import/battleground_db.yml" )
INSTALL_TEMPLATE( "db/import/castle_db.txt" "db/import/castle_db.txt" )
INSTALL_TEMPLATE( "db/import/const.txt" "db/import/const.txt" )
INSTALL_TEMPLATE( "db/import/create_arrow_db.txt" "db/import/create_arrow_db.txt" )
INSTALL_TEMPLATE( "db/import/create_arrow_db.yml" "db/import/create_arrow_db.yml" )
INSTALL_TEMPLATE( "db/import/elemental_db.txt" "db/import/elemental_db.txt" )
INSTALL_TEMPLATE( "db/import/elemental_skill_db.txt" "db/import/elemental_skill_db.txt" )
INSTALL_TEMPLATE( "db/import/exp_guild.txt" "db/import/exp_guild.txt" )
INSTALL_TEMPLATE( "db/import/exp_homun.txt" "db/import/exp_homun.txt" )
INSTALL_TEMPLATE( "db/import/exp_homun.yml" "db/import/exp_homun.yml" )
INSTALL_TEMPLATE( "db/import/guild_skill_tree.yml" "db/import/guild_skill_tree.yml" )
INSTALL_TEMPLATE( "db/import/homun_skill_tree.txt" "db/import/homun_skill_tree.txt" )
INSTALL_TEMPLATE( "db/import/homunculus_db.txt" "db/import/homunculus_db.txt" )
INSTALL_TEMPLATE( "db/import/instance_db.yml" "db/import/instance_db.yml" )
INSTALL_TEMPLATE( "db/import/item_avail.txt" "db/import/item_avail.txt" )
INSTALL_TEMPLATE( "db/import/item_bluebox.txt" "db/import/item_bluebox.txt" )
INSTALL_TEMPLATE( "db/import/item_buyingstore.txt" "db/import/item_buyingstore.txt" )
INSTALL_TEMPLATE( "db/import/item_cardalbum.txt" "db/import/item_cardalbum.txt" )
INSTALL_TEMPLATE( "db/import/item_cash_db.txt" "db/import/item_cash_db.txt" )
INSTALL_TEMPLATE( "db/import/item_combo_db.txt" "db/import/item_combo_db.txt" )
INSTALL_TEMPLATE( "db/import/item_db.txt" "db/import/item_db.txt" )
INSTALL_TEMPLATE( "db/import/item_db.yml" "db/import/item_db.yml" )
INSTALL_TEMPLATE( "db/import/item_delay.txt" "db/import/item_delay.txt" )
INSTALL_TEMPLATE( "db/import/item_flag.txt" "db/import/item_flag.txt" )
INSTALL_TEMPLATE( "db/import/item_giftbox.txt" "db/import/item_giftbox.txt" )
INSTALL_TEMPLATE( "db/import/item_group_db.txt" "db/import/item_group_db.txt" )
INSTALL_TEMPLATE( "db/import/item_misc.txt" "db/import/item_misc.txt" )
INSTALL_TEMPLATE( "db/import/item_noequip.txt" "db/import/item_noequip.txt" )
INSTALL_TEMPLATE( "db/import/item_nouse.txt" "db/import/item_nouse.txt" )
INSTALL_TEMPLATE( "db/import/item_package.txt" "db/import/item_package.txt" )
INSTALL_TEMPLATE( "db/import/item_randomopt_db.txt" "db/import/item_randomopt_db.txt" )
INSTALL_TEMPLATE( "db/import/item_randomopt_db.yml" "db/import/item_randomopt_db.yml" )
INSTALL_TEMPLATE( "db/import/item_randomopt_group.txt" "db/import/item_randomopt_group.txt" )
INSTALL_TEMPLATE( "db/import/item_randomopt_group.yml" "db/import/item_randomopt_group.yml" )
INSTALL_TEMPLATE( "db/import/item_stack.txt" "db/import/item_stack.txt" )
INSTALL_TEMPLATE( "db/import/item_trade.txt" "db/import/item_trade.txt" )
INSTALL_TEMPLATE( "db/import/item_violetbox.txt" "db/import/item_violetbox.txt" )
INSTALL_TEMPLATE( "db/import/job_basehpsp_db.txt" "db/import/job_basehpsp_db.txt" )
INSTALL_TEMPLATE( "db/import/job_db1.txt" "db/import/job_db1.txt" )
INSTALL_TEMPLATE( "db/import/job_db2.txt" "db/import/job_db2.txt" )
INSTALL_TEMPLATE( "db/import/job_exp.txt" "db/import/job_exp.txt" )
INSTALL_TEMPLATE( "db/import/job_noenter_map.txt" "db/import/job_noenter_map.txt" )
INSTALL_TEMPLATE( "db/import/job_param_db.txt" "db/import/job_param_db.txt" )
INSTALL_TEMPLATE( "db/import/level_penalty.txt" "db/import/level_penalty.txt" )
INSTALL_TEMPLATE( "db/import/level_penalty.yml" "db/import/level_penalty.yml" )
INSTALL_TEMPLATE( "db/import/magicmushroom_db.yml" "db/import/magicmushroom_db.yml" )
INSTALL_TEMPLATE( "db/import/map_cache.dat" "db/import/map_cache.dat" )
INSTALL_TEMPLATE( "db/import/map_index.txt" "db/import/map_index.txt" )
INSTALL_TEMPLATE( "db/import/mercenary_db.txt" "db/import/mercenary_db.txt" )
INSTALL_TEMPLATE( "db/import/mercenary_skill_db.txt" "db/import/mercenary_skill_db.txt" )
INSTALL_TEMPLATE( "db/import/mob_avail.yml" "db/import/mob_avail.yml" )
INSTALL_TEMPLATE( "db/import/mob_boss.txt" "db/import/mob_boss.txt" )
INSTALL_TEMPLATE( "db/import/mob_branch.txt" "db/import/mob_branch.txt" )
INSTALL_TEMPLATE( "db/import/mob_chat_db.txt" "db/import/mob_chat_db.txt" )
INSTALL_TEMPLATE( "db/import/mob_chat_db.yml" "db/import/mob_chat_db.yml" )
INSTALL_TEMPLATE( "db/import/mob_classchange.txt" "db/import/mob_classchange.txt" )
INSTALL_TEMPLATE( "db/import/mob_db.txt" "db/import/mob_db.txt" )
INSTALL_TEMPLATE( "db/import/mob_db.yml" "db/import/mob_db.yml" )
INSTALL_TEMPLATE( "db/import/mob_drop.txt" "db/import/mob_drop.txt" )
INSTALL_TEMPLATE( "db/import/mob_item_ratio.txt" "db/import/mob_item_ratio.txt" )
INSTALL_TEMPLATE( "db/import/mob_mission.txt" "db/import/mob_mission.txt" )
INSTALL_TEMPLATE( "db/import/mob_poring.txt" "db/import/mob_poring.txt" )
INSTALL_TEMPLATE( "db/import/mob_pouch.txt" "db/import/mob_pouch.txt" )
INSTALL_TEMPLATE( "db/import/mob_race2_db.txt" "db/import/mob_race2_db.txt" )
INSTALL_TEMPLATE( "db/import/mob_random_db.txt" "db/import/mob_random_db.txt" )
INSTALL_TEMPLATE( "db/import/mob_skill_db.txt" "db/import/mob_skill_db.txt" )
INSTALL_TEMPLATE( "db/import/mob_summon.yml" "db/import/mob_summon.yml" )
INSTALL_TEMPLATE( "db/import/pet_db.yml" "db/import/pet_db.yml" )
INSTALL_TEMPLATE( "db/import/produce_db.txt" "db/import/produce_db.txt" )
INSTALL_TEMPLATE( "db/import/quest_db.yml" "db/import/quest_db.yml" )
INSTALL_TEMPLATE( "db/import/refine.yml" "db/import/refine.yml" )
INSTALL_TEMPLATE( "db/import/refine_db.yml" "db/import/refine_db.yml" )
INSTALL_TEMPLATE( "db/import/size_fix.yml" "db/import/size_fix.yml" )
INSTALL_TEMPLATE( "db/import/skill_changematerial_db.txt" "db/import/skill_changematerial_db.txt" )
INSTALL_TEMPLATE( "db/import/skill_damage_db.txt" "db/import/skill_damage_db.txt" )
INSTALL_TEMPLATE( "db/import/skill_db.yml" "db/import/skill_db.yml" )
INSTALL_TEMPLATE( "db/import/skill_nocast_db.txt" "db/import/skill_nocast_db.txt" )
INSTALL_TEMPLATE( "db/import/skill_tree.txt" "db/import/skill_tree.txt" )
INSTALL_TEMPLATE( "db/import/spellbook_db.yml" "db/import/spellbook_db.yml" )
INSTALL_TEMPLATE( "db/import/statpoint.txt" "db/import/statpoint.txt" )
INSTALL_TEMPLATE( "db/import/status_disabled.txt" "db/import/status_disabled.txt" )