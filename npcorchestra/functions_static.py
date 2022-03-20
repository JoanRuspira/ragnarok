import random
import uuid
import os
import re
from lists import * 


def buildStatic(inputParameter):
    cities = [inputParameter]
    if inputParameter == 'all':
        cities = all_cities
    for city in cities:
        buildCity(city)

def getCommonSprites():
	return [x for x in allsprites if x not in morroc + geffen + izlude + comodo + hugel]

def getCityNPCS(city):
    match city:
        case "umbala":
            return generic + umbala
        case "umbala_natives":
            return umbala_natives
        case "amatsu" | "louyang":
            return generic + amatsu
        case "comodo", "cmd_fild07":
            return generic + comodo
        case "morocc" | "veins":
            return generic + morroc
        case "alberta":
            return generic + alberta
        case "ayothaya":
            return generic + ayothaya
        case "payon" | "pay_arche":
            return generic + payon
        case "izlude" | "prontera":
            return generic + prontera
        case "geffen" | "yuno":
            return generic + geffen
        case "aldebaran":
            return generic + aldebaran
        case "hugel":
            return generic + hugel
        case "einbech":
            return generic + einbech
        case "einbroch":
            return generic + einbroch
        case "lighthalzen":
            return generic + lighthalzen
        case "lighthalzen_slums":
            return lighthalzen_slums
        case "lighthalzen_rekenber":
            return lighthalzen_rekenber
        case "dicastes01":
            return generic + dicastes
        case "rachel" | "ra_temple":
            return generic + morroc + rachel
        case "niflheim":
            return generic + nifflheim
        case "monk":
            return monk
        case "mid_camp":
            return generic
        case "nameless_n":
            return nameless_n
        case _:
            return generic

def buildCity(city):
    #randomize sprite ids
	fin = open("./templates/static/" + city + "_template.txt", "rt")
	fout = open("./" + city + "_tmp.txt", "wt")

	citySprites = getCityNPCS(city)
	for line in fin:
		fout.write(line.replace('spriteid', str(random.choice(citySprites))))
	fout.close()
	fin.close()

    #generate npc ids
	fin2 = open("./" + city + "_tmp.txt", "rt")
	fout2 = open("../npc/npcorchestra/" + city + ".txt", "wt")

	for line in fin2:
		fout2.write(line.replace('npcid', str(uuid.uuid4())[:8]))
	fout2.close()
	fin2.close()
	os.remove("./" + city + "_tmp.txt")

