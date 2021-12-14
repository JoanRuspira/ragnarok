import random
import uuid
from lists import * 

#input file
fin = open("hugel_template.txt", "rt")
#output file to write the result to
fout = open("hugel_tmp.txt", "wt")
#for each line in the input file
for line in fin:
	#read replace the string and write to output file
	fout.write(line.replace('npcid', str(uuid.uuid4())[:8]))
#close input and output files
fin.close()
fout.close()



#input file
fin = open("hugel_tmp.txt", "rt")
#output file to write the result to
fout = open("../npc/joan/hugel.txt", "wt")
#for each line in the input file

for line in fin:
	#read replace the string and write to output file
	fout.write(line.replace('spriteid', str(random.choice(sprites))))

