import random
import uuid
import os
import re
from lists import *

iterations = 4 #16 of each job
initial_mob_id = 20021

# 1000-3999 or 20020-31999

def buildDynamic():
    buildMobsfile()
    buildMobsAvailfile()
    buildMobsCities()

def buildMobsCities():
    end_mob_id = calculateEndMobId()
    city_id = 0
    for city in all_cities:
        city_density = cities_density[city_id]
        buildMobsCitiy(city,city_density,end_mob_id)
        city_id += 1

def buildMobsCitiy(city, city_density, end_mob_id):   
    fout = open("../npc/mobs/npcorchestra/" + city + "_npcorchestra.txt", "wt")
    city_mobs = []
    for _ in range(city_density):
        city_mobs.append(random.randint(initial_mob_id, end_mob_id))
    
    for city_mob_id in city_mobs:
        mob_line = city + ",0,0	monster	 Adventurer	" + str(city_mob_id) + ",1,5000\n"
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
    mob_names = buildMobArrayIterations()
    mobs = []
    mob_id = initial_mob_id
    for mob_name in mob_names:
        mob = buildMob(mob_id, mob_name)
        mob_id += 1
        mobs.append(mob)
    return mobs

def buildMobsAvail():
    mob_names = buildMobArrayIterations()
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
    theseksInt = random.randint(1, 2)
    if(theseksInt == 1):
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
    mobAvail += "    HeadTop: " + random.choice(top_headgears) + "\n"
    mobAvail += "    HeadMid: " + random.choice(mid_headgears) + "\n"
    mobAvail += "    HeadLow: " + random.choice(low_headgears) + "\n"

    jobs_falcon = ["JOB_HUNTER", "JOB_SNIPER"]
    if mob_name in jobs_falcon:
        mobAvail += "    Options:\n"
        mobAvail += "      Falcon: true\n"

    jobs_cart = ["JOB_MERCHANT", "JOB_BLACKSMITH", "JOB_ALCHEMIST", "JOB_WHITESMITH", "JOB_CREATOR"]
    if mob_name in jobs_cart:
        mobAvail += "    PushCart: true\n"
        # mobAvail += "      Cart1: true\n"

    jobs_riding = ["JOB_KNIGHT", "JOB_CRUSADER", "JOB_LORD_KNIGHT", "JOB_PALAIN"]
    mob_riding = random.randint(1, 2)
    if mob_name in jobs_riding:
        if(mob_riding == 1):
            mobAvail += "    Options:\n"
            mobAvail += "      Riding: true\n"

    return mobAvail

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