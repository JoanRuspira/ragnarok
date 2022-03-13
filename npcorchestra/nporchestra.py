import sys
from functions_static import buildStatic
from functions_dynamic import buildDynamic


if len(sys.argv) != 2:
	print("Provide 1 argument pls")
else:
	buildStatic(sys.argv[1])
	buildDynamic()

