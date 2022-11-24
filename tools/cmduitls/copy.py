import sys
import shutil
if len(sys.argv) < 3:
	print("Invalid syntax, copy.py source dest \n source is a file dest can be directory");

src = sys.argv[1]
dst = sys.argv[2]
shutil.copy2(src, dst)