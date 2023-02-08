import bpy
import sys
import os
import re
import shutil
import json
with open("export.json", 'r') as file:
	j = file.read()
	config = json.loads(j)
name=config['name']
blendfn = sys.argv[-1]
basename= os.path.splitext(blendfn)[0]
objfn = basename+".obj"
content_dir = os.environ["ALVAHSHI_CONTENT"]
basealv = os.environ["ALVAHSHI_BASEQ3"]

model_path = basealv+'/models/'+name
if os.path.isdir(model_path)==False:
	os.makedirs(model_path, exist_ok=True)

model_content_path = content_dir+'/models/'+name
if os.path.isdir(model_content_path)==False:
	os.makedirs(model_content_path, exist_ok=True)
print("Blender export scene in obj Format in file "+objfn)
# Doc can be found here: https://docs.blender.org/api/current/bpy.ops.export_scene.html
bpy.ops.export_scene.obj(filepath=objfn)
matfn = basename+".mtl"
data = ''
with open(matfn, 'r') as file:
    data = file.read().replace('\\\\', '/')

with open(matfn, 'w') as file:
	file.write(data)

#h3d export, blend->fbx->dae->h3d
#fbxfn = basename+".fbx"
#bpy.ops.export_scene.fbx(filepath=fbxfn)

daefn = basename+".dae"
bpy.ops.wm.collada_export(filepath=daefn)
#cmd = "noesis ?cmode %s %s"%(fn,daefn)
#os.system(cmd)

cmd = "khcol %s -type model "%(daefn)
os.system(cmd)

#fix material names
#copy files to contnet and basealv
scenefn = basename+".scene.xml"
geofn = basename+".geo"
shutil.copyfile(geofn, model_content_path+"/"+geofn)

all_mat_files = []
with open(scenefn, 'r') as file:
	data = file.read()
	data = data.replace('name="%s"'%(basename),'name="%s"'%(name))
	data = data.replace('geometry="%s.geo"'%(basename),'geometry="%s"'%('models/'+name+'/'+geofn))
	for m in re.finditer('material="',data):
		s = m.start()+10;
		mname = data[s:s+data[s:].find('"')]
		expmatname = ''
		with open(mname, 'r') as matfile:
			mdata = matfile.read();
			m = mdata.find('name="albedoMap" map="')
			s = m+22
			albedo_map = mdata[s:s+mdata[s:].find('"')]
			expmatname= os.path.splitext(albedo_map)[0]
			expmatname = expmatname+".material.xml"
			all_mat_files.append(expmatname)
			mdata = mdata.replace(albedo_map,'models/'+name+'/'+albedo_map)
			mdata = mdata.replace("model.shader","model_org.shader")
			with open(expmatname, 'w') as file:
				file.write(mdata)
			if os.path.exists(model_content_path+"/"+expmatname)==False:
				shutil.copyfile(expmatname, model_content_path+"/"+expmatname)
			if os.path.exists(model_content_path+"/"+albedo_map)==False:
				shutil.copyfile(albedo_map, model_content_path+"/"+albedo_map)
		os.remove(mname)

		data = data.replace(mname,'models/'+name+'/'+expmatname)
with open(scenefn, 'w') as file:
	file.write(data)
shutil.copyfile(scenefn, model_content_path+"/"+scenefn)
#copy files to target dirs

LL = []
with open(matfn, 'r') as file:
	lines = file.readlines()
	for l in lines:
		if l[:6] == 'map_Kd':
			ffn = l[7:].strip()
			asetname = '/models/'+name+"/"+os.path.basename(ffn)
			shutil.copyfile(ffn, basealv+asetname)
			r = l.replace(ffn,asetname)
			LL.append(r)
			print(ffn)
		else:
			LL.append(l)
with open(matfn, 'w') as file:
	file.writelines(LL)

	
shutil.copyfile(objfn, basealv+'/models/'+name+"/"+objfn)
shutil.copyfile(matfn, basealv+'/models/'+name+"/"+matfn)

#quit()
os.remove(objfn)
os.remove(daefn)
#os.remove(fbxfn)
os.remove(matfn)
os.remove(scenefn)
os.remove(geofn)
for f in all_mat_files:
	os.remove(f)




