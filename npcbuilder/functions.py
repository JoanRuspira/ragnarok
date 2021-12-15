import random
import uuid
import os
from lists import * 

def buildCity(city):

    #randomize sprite ids
	fin = open("./templates/cities/" + city + "_template.txt", "rt")
	fout = open("./" + city + "_tmp.txt", "wt")

	for line in fin:
		fout.write(line.replace('spriteid', str(random.choice(sprites))))
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
        cities = ["umbala", "comodo", "morroc", "payon", "alberta", "izlude", "geffen", "aldebaran", "hugel"]
    
    for city in cities:
        buildCity(city)