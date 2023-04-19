import random
import uuid
import os
import re
from lists import * 
from cities import all_cities


def buildStatic(inputParameter):
    cities = [inputParameter]
    if inputParameter == 'all':
        cities = all_cities
    for city in cities:
        buildCity(city)

def getCommonSprites():
	return [x for x in allsprites if x not in morroc + geffen + izlude + comodo + hugel]

def getCityNPCS(city):
    if city.get("Name") == "umbala":
        return generic + umbala
    if city.get("Name") == "umbala_natives":
        return umbala_natives
    if city.get("Name") == "amatsu" or city.get("Name") == "louyang":
        return generic + amatsu
    if city.get("Name") == "comodo" or city.get("Name") == "cmd_fild07":
        return generic + comodo
    if city.get("Name") == "morocc" or city.get("Name") == "veins":
        return generic + morroc
    if city.get("Name") == "alberta":
        return generic + alberta
    if city.get("Name") == "ayothaya":
        return generic + ayothaya
    if city.get("Name") == "payon" or city.get("Name") == "pay_arche":
        return generic + payon
    if city.get("Name") == "izlude" or city.get("Name") == "prontera":
        return generic + prontera
    if city.get("Name") == "geffen" or city.get("Name") == "yuno":
        return generic + geffen
    if city.get("Name") == "aldebaran":
        return generic + aldebaran
    if city.get("Name") == "yuno":
        return generic + geffen + aldebaran + prontera
    if city.get("Name") == "hugel":
        return generic + hugel
    if city.get("Name") == "einbech":
        return generic + einbech
    if city.get("Name") == "einbroch":
        return generic + einbroch
    if city.get("Name") == "lighthalzen":
        return generic + lighthalzen
    if city.get("Name") == "lighthalzen_slums":
        return lighthalzen_slums
    if city.get("Name") == "lighthalzen_rekenber":
        return lighthalzen_rekenber
    if city.get("Name") == "dicastes01":
        return generic + dicastes
    if city.get("Name") == "rachel" or city.get("Name") == "ra_temple":
        return generic + morroc + rachel
    if city.get("Name") ==  "niflheim":
        return generic + nifflheim
    if city.get("Name") == "monk":
        return monk
    if city.get("Name") == "mid_camp":
        return generic
    if city.get("Name") == "nameless_n":
        return nameless_n
    if city.get("Name") == "airplane" or city.get("Name") == "airplane_01":
        return generic + prontera + aldebaran
    return generic

def buildCity(city):
    #randomize sprite ids
	fin = open("./templates/static/" + city.get("Name") + "_template.txt", "rt")
	fout = open("./" + city.get("Name") + "_tmp.txt", "wt")

	citySprites = getCityNPCS(city)
	for line in fin:
		fout.write(line.replace('spriteid', str(random.choice(citySprites))))
	fout.close()
	fin.close()

    #generate npc ids
	fin2 = open("./" + city.get("Name") + "_tmp.txt", "rt")
	fout2 = open("./generated/static/" + city.get("Name") + ".txt", "wt")

	for line in fin2:
		fout2.write(line.replace('npcid', str(uuid.uuid4())[:8]))
	fout2.close()
	fin2.close()
	os.remove("./" + city.get("Name") + "_tmp.txt")

