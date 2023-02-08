import os
import json
with open("export.json", 'r') as file:
	j = file.read()
	config = json.loads(j)

cmd = "blender %s --background --python D:\\Alvahshi\\game\\sources\\tools\\blendexport\\export.py -- %s"%(config['input'],config['input'])
os.system(cmd)