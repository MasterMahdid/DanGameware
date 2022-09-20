'''
scans 'materials' folder in conent. makes coresponidng materials in basealve
also scans models fodler

bps gives this:
d:/alvahshi/newstructure/editor/gamedist/basealv/textures/walls/stonewall1.jpg
we need to convert it to:
materials/walls/stonewall1/mat.material.xml
'''
import os
import xml.etree.ElementTree as ET
import shutil
from PIL import Image
import dbm
import hashlib
db = dbm.open("data",'c')

def find_materials():
	content_dir = os.environ["ALVAHSHI_CONTENT"];
	mat_dir =  os.path.join(content_dir,"materials")
	mat_xmls = []
	for root, dirs, files in os.walk(content_dir, topdown=False):
		for name in files:
			if name == "mat.material.xml":
				pp = str(os.path.relpath(root,mat_dir))
				pp = pp.replace("\\","/")
				mat_xmls+=[pp]
	return mat_xmls
def do_mat(mat_name):
	content_dir = os.environ["ALVAHSHI_CONTENT"]
	basealv = os.environ["ALVAHSHI_BASEQ3"]
	mat_dir =  os.path.join(content_dir,"materials",mat_name)
	xml_path = os.path.join(mat_dir,"mat.material.xml")
	tree = ET.parse(xml_path)
	root = tree.getroot()
	samplers = root.findall("./Sampler")
	albedomap = ""
	for smp in samplers:
		nm=smp.attrib["name"]
		if nm == "albedoMap":
			albedomap = smp.attrib["map"]
			break
	sourcepath = os.path.join(content_dir,albedomap)
	
	filemd5= hashlib.md5(open(sourcepath,'rb').read()).hexdigest()
	if sourcepath in db and db[sourcepath].decode('ascii')==filemd5:
		return

	target_path = os.path.join(basealv,"textures",mat_name+'.png')
	dir_tomake = os.path.dirname(target_path)
	if os.path.exists(dir_tomake)==False:
		os.makedirs(dir_tomake)
	
	im = Image.open(sourcepath)
	sz = im.size
	if sz[0]>1024:
		h = int(1024*(sz[1]/sz[0]))
		im = im.resize((1024,h))
	im.convert('RGBA').save(target_path,"PNG")

	db[sourcepath] = filemd5


mats = find_materials()

for m in mats:
	print(m)
	do_mat(m)

print("Done");

