import random
import uuid
import os
from lists import * 

def getCommonSprites():
	return [x for x in allsprites if x not in morroc + geffen + izlude + comodo + hugel]

def getCityNPCS(city):
    match city:
        case "izlude" | "payon" | "alberta" | "aldebaran":
            return getCommonSprites() + izlude
        case "geffen":
            return getCommonSprites() + geffen
        case "morroc":
            return getCommonSprites() + morroc
        case "comodo":
            return getCommonSprites() + comodo
        case "hugel":
            return getCommonSprites() + hugel
        case _:
            return allsprites


def buildCity(city):

    #randomize sprite ids
	fin = open("./templates/cities/" + city + "_template.txt", "rt")
	fout = open("./" + city + "_tmp.txt", "wt")

	#citySprites = getCityNPCS(city)
	for line in fin:
		fout.write(line.replace('spriteid', str(random.choice(allsprites))))
	fout.close()
	fin.close()


    #generate npc ids
	fin2 = open("./" + city + "_tmp.txt", "rt")
	fout2 = open("../npc/joan/" + city + ".txt", "wt")

	for line in fin2:
		fout2.write(line.replace('npcid', str(uuid.uuid4())[:8]))
	fout2.close()
	fin2.close()
	os.remove("./" + city + "_tmp.txt")


def build(inputParameter):

    cities = [inputParameter]
    if inputParameter == 'all':
        cities = ["umbala", "comodo", "morroc", "payon", "alberta", "izlude", "geffen", "aldebaran", "hugel", "einbroch", "einbech"]
    
    for city in cities:
        buildCity(city)