import random
import uuid
import os
import re
from lists import *
from cities import all_cities
from fields import all_fields
from interiors import all_interiors

iterations = 4
initial_mob_id = 20021
excluded_ids = [20365, 20357, 20358, 20363,20364, 20367, 20368, 20370, 20379, 20380, 20420, 20932]

adventurers_novice = []
adventurers_swordsman = []
adventurers_mage = []
adventurers_archer = []
adventurers_acolyte = []
adventurers_merchant = []
adventurers_thief = []

# 1000-3999 or 20020-31999

def buildDynamic():
    buildMobsfile()
    buildMobsAvailfile()
    buildMobsCities()
    buildMobsFields()

def buildMobsfile():  
    mobs = buildMobs()
    fin = open("./templates/dynamic/mob_db_template.yml", "rt")
    fout = open("./generated/db/mob_db.yml", "wt")
    for line in fin:
        if "#END_NPCBUILDER_MOBS" in line:
            for mob in mobs:
                fout.write(mob)
        fout.write(line)   
    fin.close()
    fout.close()

def buildMobs():
    mob_names = buildMobArrayIterations()
    mobs = []
    mob_id = initial_mob_id
    for mob_name in mob_names:
        mob = buildMob(mob_id, mob_name, "01")
        populate_job_lists(mob_id, mob_name)
        mob_id += 1
        mobs.append(mob)
    
    # mob = buildMob(mob_id, "MORROC_GUARD", "10")
    mobs.append(mob)
    return mobs


def buildMob(mob_id, mob_name, ai):
    mob = "  - Id: " + str(mob_id) + "\n"
    mob += "    AegisName: " + mob_name + "_" + str(mob_id) + "\n"
    mob += "    Name: " + mob_name.lower().replace("job_", "").replace("_", "").title() + "\n"
    mob += "    Level: 1\n"
    mob += "    Hp: 60\n"
    mob += "    WalkSpeed: 200\n"
    mob += "    Ai: " + ai + "\n"
    mob += "    AttackRange: 1\n"
    mob += "    SkillRange: 10\n"
    mob += "    ChaseRange: 12\n"
    mob += "    BaseExp: 164\n"        
    mob += "    JobExp: 195\n"        
    mob += "    Attack: 33\n"        
    mob += "    Attack2: 8\n"        
    mob += "    Defense: 13\n"        
    mob += "    Str: 10\n"        
    mob += "    Agi: 12\n"        
    mob += "    Vit: 8\n"        
    mob += "    Int: 5\n"        
    mob += "    Dex: 17\n"        
    mob += "    Luk: 7\n"        
    mob += "    Size: Small\n"        
    mob += "    Race: Brute\n"        
    mob += "    Element: Fire\n"        
    mob += "    ElementLevel: 1\n"        
    mob += "    AttackDelay: 1600\n"        
    mob += "    AttackMotion: 900\n"        
    mob += "    DamageMotion: 240\n"        
    
    DamageMotion: 240

    return mob

def populate_job_lists(mob_id, mob_name):
    if mob_name == "JOB_NOVICE":
        adventurers_novice.append(str(mob_id))
    if mob_name == "JOB_SWORDMAN":
        adventurers_swordsman.append(str(mob_id))
    if mob_name == "JOB_MAGE":
        adventurers_mage.append(str(mob_id))
    if mob_name == "JOB_ARCHER":
        adventurers_archer.append(str(mob_id))
    if mob_name == "JOB_ACOLYTE":
        adventurers_acolyte.append(str(mob_id))
    if mob_name == "JOB_MERCHANT":
        adventurers_merchant.append(str(mob_id))
    if mob_name == "JOB_THIEF":
        adventurers_thief.append(str(mob_id))

def buildMobsCities():
    end_mob_id = calculateEndMobId()
    for city in all_cities + all_interiors: 
        buildMobsMapCity(city, end_mob_id)

def buildMobsFields():
    for field in all_fields:
        buildMobsMapField(field)

def buildMobsMapCity(city, end_mob_id):   
    fout = open("./generated/dynamic/" + city.get("Name") + "_npcorchestra.txt", "wt")
    city_mobs = []
    for _ in range(city.get("Density")):
        city_mobs.append(random.choice([i for i in range(initial_mob_id,end_mob_id) if i not in excluded_ids]))
    
    for city_mob_id in city_mobs:
        mob_line = city.get("Name") + ",0,0	monster	 Adventurer	" + str(city_mob_id) + ",1,5000\n"
        fout.write(mob_line)
    fout.close()


def buildMobsMapField(field):   
    fout = open("./generated/dynamic/" + field.get("Name") + "_npcorchestra.txt", "wt")
    field_mobs = []
    for _ in range(field.get("Density")):
        if field.get("Adventurer_Types") == "Novices":
            field_mobs.append(random.choice(adventurers_novice))
        if field.get("Adventurer_Types") == "First Jobs":
            field_mobs.append(random.choice(adventurers_swordsman + 
                adventurers_mage + adventurers_archer + adventurers_acolyte + 
                adventurers_merchant + adventurers_thief))
    
    for field_mob_id in field_mobs:
        mob_line = field.get("Name") + ",0,0	monster	 Adventurer	" + str(field_mob_id) + ",1,5000\n"
        fout.write(mob_line)
    fout.close()


def buildMobsAvailfile():
    mobs_avail = buildMobsAvail()
    fin = open("./templates/dynamic/mob_avail_template.yml", "rt")
    fout = open("./generated/db/import/mob_avail.yml", "wt")
    for line in fin:
        if "#END_NPCBUILDER_MOBS" in line:
            for mob_avail in mobs_avail:
                fout.write(mob_avail)
        fout.write(line)
    fin.close()
    fout.close()


def buildMobsAvail():
    mob_names = buildMobArrayIterations()
    mobs_avail = []
    mob_id = initial_mob_id
    for mob_name in mob_names:
        if mob_id in excluded_ids:
            mob_id += 1
            continue
        mob_avail = buildMobAvail(mob_id, mob_name)
        mob_id += 1
        mobs_avail.append(mob_avail)
    return mobs_avail



def buildMobAvail(mob_id, mob_name):
    theseksInt = random.randint(1, 2)
    if(theseksInt == 1):
        theseks = "Male"
    else:
        theseks = "Female"
    mob_avail = "  - Mob: " + mob_name + "_" + str(mob_id) + "\n"
    mob_avail += "    Sprite: " + mob_name + "\n"
    mob_avail += "    Sex: " + theseks + "\n"
    mob_avail += "    HairStyle: " + str(random.randint(0, 8)) + "\n"
    mob_avail += "    HairColor: " + str(random.randint(0, 8)) + "\n"
    mob_avail += "    ClothColor: " + str(random.randint(0, 3)) + "\n"
    mob_avail += "    Weapon: Gladius\n"
    mob_avail += "    Shield: Guard\n"
    
    mob_avail = build_headgear_section(mob_avail, mob_name)
    mob_avail = build_special_options_section(mob_avail, mob_name)

    return mob_avail

def build_headgear_section(mob_avail, mob_name):
    has_headgear = random.randint(0, 9)
    pool_top_headgears = []
    pool_mid_headgears = mid_headgears
    pool_low_headgears = low_headgears
    if (mob_name == "JOB_NOVICE"):
        return mob_avail
    if (mob_name == "JOB_SWORDMAN"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_swordsman
    if (mob_name == "JOB_THIEF"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_thief
    if (mob_name == "JOB_MAGE"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_magician
    if (mob_name == "JOB_MERCHANT"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_merchant
    if (mob_name == "JOB_ARCHER"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_archer
    if (mob_name == "JOB_ACOLYTE"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_acolyte
    if (mob_name == "JOB_KNIGHT" or mob_name == "JOB_LORD_KNIGHT"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_2ndJob + top_headgears_swordsman + top_headgears_knight
    if (mob_name == "JOB_CRUSADER" or mob_name == "JOB_PALADIN"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_2ndJob + top_headgears_swordsman + top_headgears_crusader
    if (mob_name == "JOB_ASSASSIN" or mob_name == "JOB_ASSASSIN_CROSS"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_2ndJob + top_headgears_thief + top_headgears_assassin
    if (mob_name == "JOB_ROGUE" or mob_name == "JOB_STALKER"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_2ndJob + top_headgears_thief + top_headgears_rogue
    if (mob_name == "JOB_WIZARD" or mob_name == "JOB_HIGH_WIZARD"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_2ndJob + top_headgears_magician + top_headgears_wizard
    if (mob_name == "JOB_SAGE" or mob_name == "JOB_PROFESSOR"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_2ndJob + top_headgears_magician + top_headgears_sage
    if (mob_name == "JOB_BLACKSMITH" or mob_name == "JOB_WHITESMITH"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_2ndJob + top_headgears_merchant + top_headgears_blacksmith
    if (mob_name == "JOB_ALCHEMIST" or mob_name == "JOB_CREATOR"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_2ndJob + top_headgears_merchant + top_headgears_alchemist
    if (mob_name == "JOB_HUNTER" or mob_name == "JOB_SNIPER"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_2ndJob + top_headgears_archer + top_headgears_hunter
    if (mob_name == "JOB_BARD" or mob_name == "JOB_CLOWN" or mob_name == "JOB_DANCER" or mob_name == "JOB_GYPSY"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_2ndJob + top_headgears_archer + top_headgears_bard
    if (mob_name == "JOB_PRIEST" or mob_name == "JOB_HIGH_PRIEST"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_2ndJob + top_headgears_acolyte + top_headgears_priest
    if (mob_name == "JOB_MONK" or mob_name == "JOB_CHAMPION"):
        pool_top_headgears = top_headgears_1stJob + top_headgears_2ndJob + top_headgears_acolyte + top_headgears_monk

    if(has_headgear not in [9]):
        mob_avail += "    HeadTop: " + random.choice(pool_top_headgears) + "\n"
    if(has_headgear in [0,1,2,3,4]):
        mob_avail += "    HeadMid: " + random.choice(pool_mid_headgears) + "\n"
    if(has_headgear in [5,6,7,8,9]):
        mob_avail += "    HeadLow: " + random.choice(pool_low_headgears) + "\n"
    return mob_avail



def build_special_options_section(mob_avail, mob_name):
    
    if mob_name in jobs_falcon:
        mob_avail += "    Options:\n"
        mob_avail += "      Falcon: true\n"

    # jobs_cart = ["JOB_MERCHANT", "JOB_BLACKSMITH", "JOB_ALCHEMIST", "JOB_WHITESMITH", "JOB_CREATOR"]
    # if mob_name in jobs_cart:
    #     mob_avail += "    Options:\n"
    #     mob_avail += "      Cart2: true\n"

    mob_riding = random.randint(1, 2)
    if mob_name in jobs_riding:
        if(mob_riding == 1):
            mob_avail += "    Options:\n"
            mob_avail += "      Riding: true\n"
    return mob_avail

def buildMobArrayIterations():
    mob_names = mobs_novice + mobs_first_job + mobs_second_job + mobs_advanced_job
    for _ in range(iterations):
        mob_names += mob_names.copy()
    return mob_names

def calculateEndMobId():
    mob_names = buildMobArrayIterations()
    end_mob_id = initial_mob_id
    for mob_name in mob_names:
        end_mob_id += 1
    end_mob_id -= 1
    return end_mob_id