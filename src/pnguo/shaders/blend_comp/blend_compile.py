# coding: utf-8
import os
import sys
import re
path = os.getcwd()
exepath = path
if "win32" == sys.platform:
	exepath = "glslangvalidator.exe"
	pass
elif "linux" == sys.platform:
	exepath = "glslangValidator"
	pass
elif "linux2" == sys.platform:
	exepath = "glslangValidator"
	pass
elif "darwin" == sys.platform:
	exepath = "glslangValidator"
	pass
def readfd(fn):
	data_content = [[],[],[]]
	idx = 0
	with open(fn,'r',encoding='utf-8') as file:
		content_list = file.readlines()
	for c in content_list:
		if "*end" in c:
			idx = idx + 1
		data_content[idx].append(c)
		if "*begin" in c:
			idx = idx + 1
	t = [""]
	i = 0
	for c in data_content[1]:		
		t[i] = t[i] + c
		if "c =" in c:
			t.append("")
			i += 1
	data_content[1] = t
	return data_content
def clear_notes(a):
	t = []
	for c in a:
		if ("//" in c) == False:
			t.append(c)
	return t

files = []
for parentDir, _, fileNames in os.walk(os.getcwd()):
	for fileName in fileNames:
		filepath = os.path.join(parentDir, fileName)
		if 'blend22.comp' in filepath:
			files.append(filepath)
pass
shaders = [".vert", ".frag", ".comp", ".tese", ".tesc", ".geom"]
shaderFiles = []

for file in files:
	_, ext = os.path.splitext(file)
	ext = ext.lower()
	if ext in shaders:
		shaderFiles.append(file.replace("\\", "/"))
	pass

for shader in shaderFiles:
	dct = readfd(shader)
	n = len(dct[1]) - 1
	for i in range(0,n):
		fn = "bd" + str(i) 
		with open(fn + ".comp",'w',encoding='utf-8') as f:
			f.truncate()
			f.writelines(clear_notes(dct[0]))
			f.write(dct[1][i])
			f.writelines(clear_notes(dct[2]))
		os.system(exepath + " -V " + fn + ".comp" + " -o " + fn + ".spv")
	pass