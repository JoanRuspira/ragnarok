import sys
from functions import * 



if len(sys.argv) != 2:
	print("Provide 1 argument pls")
else:
	build(sys.argv[1])












# #input file
# fin = open("hugel_tmp.txt", "rt")
# #output file to write the result to
# fout = open("../npc/joan/hugel.txt", "wt")
# #for each line in the input file

# for line in fin:
# 	#read replace the string and write to output file
# 	fout.write(line.replace('spriteid', str(random.choice(sprites))))








# fin = open("../npc/joan/archer_village.txt", "rt")
# replacement = ""

# for line in fin:

# 	if "#" in line and "Archer Village Guard" not in line and "Palace Guard" not in line: 
# 		fromindex = line.find("#")
# 		toindex = line.find(",{")
# 		line = line[:fromindex] + "#arch::npcid	spriteid," + line[toindex + 1:]
# 	if line != '':
# 		if "npctalk" in line: 
# 			if "Welcome to the Archer Village" in line:
# 				replacement = replacement + 'npctalk "Welcome to Payon";\n'
# 			else:
# 				replacement = replacement + 'npctalk "npcdialog";\n'
# 		else:
# 			replacement = replacement + line

# fin.close()
# fout = open("motivation.txt", "w")
# fout.write(replacement)
# fout.close()
