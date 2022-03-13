import random
import uuid
import os
import re
from lists import * 

def buildDynamic():
    buildMobsfile()
    buildMobsAvailfile()

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
    mob_names = mobs_novice + mobs_first_job + mobs_second_job + mobs_advanced_job + mobs_third_job
    # for _ in range(3):
    #     mob_names += mob_names.copy()

    mobs = []
    mob_id = 3900
    for mob_name in mob_names:
        mob = buildMob(mob_id, mob_name)
        mob_id += 1
        mobs.append(mob)
    return mobs

def buildMobsAvail():
    mob_names = mobs_novice + mobs_first_job + mobs_second_job + mobs_advanced_job + mobs_third_job
    # for _ in range(3):
    #     mob_names += mob_names.copy()

    mobs_avail = []
    for mob_name in mob_names:    
        mob_avail = buildMobAvail(mob_name)
        mobs_avail.append(mob_avail)
    return mobs_avail

def buildMob(mob_id, mob_name):
    mob = "  - Id: " + str(mob_id) + "\n"
    mob += "    AegisName: " + mob_name + "\n"
    mob += "    Name: " + mob_name.lower().replace("job_", "").replace("_", "").title() + "\n"
    mob += "    Level: 1\n"
    mob += "    Hp: 60\n"
    mob += "    WalkSpeed: 200\n"
    mob += "    Ai: 01\n"
    return mob

def buildMobAvail(mob_name):
    seksInt = str(random.randint(0, 1))
    if(seksInt == 0):
        seks = "Male"
    else:
        seks = "Female"
    mobAvail = "  - Mob: " + mob_name + "\n"
    mobAvail += "    Sprite: " + mob_name + "\n"
    mobAvail += "    Sex: " + seks + "\n"
    mobAvail += "    HairStyle: " + str(random.randint(0, 8)) + "\n"
    mobAvail += "    HairColor: " + str(random.randint(0, 8)) + "\n"
    mobAvail += "    ClothColor: " + str(random.randint(0, 3)) + "\n"
    mobAvail += "    Weapon: Gladius\n"
    mobAvail += "    Shield: Guard\n"
    return mobAvail

    # 1000-3999 or 20020-31999
