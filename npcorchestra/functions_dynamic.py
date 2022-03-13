import random
import uuid
import os
import re
from lists import * 

iterations = 2
initial_mob_id = 20021

def buildDynamic():
    buildMobsfile()
    buildMobsAvailfile()
    buildCityMobs()
    

def buildCityMobs():
    fout = open("../npc/mobs/fields/izlude_npcbuilder.txt", "wt")

    mob_names = mobs_novice + mobs_first_job + mobs_second_job + mobs_advanced_job
    for _ in range(iterations):
        mob_names += mob_names.copy()

    mob_id = initial_mob_id
    for mob_name in mob_names:
        mob_title = mob_name.lower().replace("job_", "").replace("_", "").title()
        mob_line = "izlude,0,0	monster	" + mob_title + "	" + str(mob_id) + ",1,5000\n"
        mob_id += 1
        fout.write(mob_line)
    
    fout.close()
    

def buildMobsfile():  
    mobs = buildMobs()
    fin = open("./templates/dynamic/mob_db_template.yml", "rt")
    fout = open("../db/re/mob_db.yml", "wt")
    for line in fin:
        if "#END_NPCBUILDER_MOBS" in line:
            for mob in mobs:
                fout.write(mob)
        fout.write(line)
    fin.close()
    fout.close()

def buildMobsAvailfile():
    mobs_avail = buildMobsAvail()
    fin = open("./templates/dynamic/mob_avail_template.yml", "rt")
    fout = open("../db/import/mob_avail.yml", "wt")
    for line in fin:
        if "#END_NPCBUILDER_MOBS" in line:
            for mob_avail in mobs_avail:
                fout.write(mob_avail)
        fout.write(line)
    fin.close()
    fout.close()

def buildMobs():
    mob_names = mobs_novice + mobs_first_job + mobs_second_job + mobs_advanced_job
    for _ in range(iterations):
        mob_names += mob_names.copy()

    mobs = []
    mob_id = initial_mob_id
    for mob_name in mob_names:
        mob = buildMob(mob_id, mob_name)
        mob_id += 1
        mobs.append(mob)
    return mobs

def buildMobsAvail():
    mob_names = mobs_novice + mobs_first_job + mobs_second_job + mobs_advanced_job
    for _ in range(iterations):
        mob_names += mob_names.copy()

    mobs_avail = []
    mob_id = initial_mob_id
    for mob_name in mob_names:    
        mob_avail = buildMobAvail(mob_id, mob_name)
        mob_id += 1
        mobs_avail.append(mob_avail)
    return mobs_avail

def buildMob(mob_id, mob_name):
    mob = "  - Id: " + str(mob_id) + "\n"
    mob += "    AegisName: " + mob_name + "_" + str(mob_id) + "\n"
    mob += "    Name: " + mob_name.lower().replace("job_", "").replace("_", "").title() + "\n"
    mob += "    Level: 1\n"
    mob += "    Hp: 60\n"
    mob += "    WalkSpeed: 200\n"
    mob += "    Ai: 01\n"
    return mob

def buildMobAvail(mob_id, mob_name):
    theseksInt = str(random.randint(0, 1))
    if(theseksInt == 0):
        theseks = "Male"
    else:
        theseks = "Female"
    mobAvail = "  - Mob: " + mob_name + "_" + str(mob_id) + "\n"
    mobAvail += "    Sprite: " + mob_name + "\n"
    mobAvail += "    Sex: " + theseks + "\n"
    mobAvail += "    HairStyle: " + str(random.randint(0, 8)) + "\n"
    mobAvail += "    HairColor: " + str(random.randint(0, 8)) + "\n"
    mobAvail += "    ClothColor: " + str(random.randint(0, 3)) + "\n"
    mobAvail += "    Weapon: Gladius\n"
    mobAvail += "    Shield: Guard\n"
    return mobAvail

    # 1000-3999 or 20020-31999
