import random
import uuid
import os
import re
from lists import * 


def buildStatic(inputParameter):
    cities = [inputParameter]
    if inputParameter == 'all':
        cities = ["monk", "umbala_natives", "umbala", "comodo", "morroc", "payon", "archer_village",
         "alberta", "ayothaya", "izlude", "geffen", "aldebaran", "hugel", "einbroch", "einbech", "hel_camp",
         "lighthalzen", "lighthalzen_slums", "lighthalzen_rekenber", "dicastes", "pharos", "amatsu",
          "louyang", "veins", "nifflheim", "vanishing", "prontera"]
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
        case "comodo", "pharos":
            return generic + comodo
        case "morroc" | "veins":
            return generic + morroc
        case "alberta":
            return generic + alberta
        case "ayothaya":
            return generic + ayothaya
        case "payon" | "archer_village":
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
        case "dicastes":
            return generic + dicastes
        case "rachel":
            return generic + morroc + rachel
        case "nifflheim":
            return generic + nifflheim
        case "monk":
            return monk
        case "hel_camp":
            return generic
        case "vanishing":
            return vanishing
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
	fout2 = open("../npc/npcbuilder/" + city + ".txt", "wt")

	for line in fin2:
		fout2.write(line.replace('npcid', str(uuid.uuid4())[:8]))
	fout2.close()
	fin2.close()
	os.remove("./" + city + "_tmp.txt")

