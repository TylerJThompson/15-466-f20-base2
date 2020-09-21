#This makefile for Windows was originally made by Dominic Calkosz and made available in the class Discord.
#I did not write this makefile originally, I only edited it for scenes besides city.

all : \
    ../dist/city.pnct \
    ../dist/city.scene \

../dist/city.scene : city.blend export-scene.py
    "E:\Program Files\Blender Foundation\Blender 2.90\blender.exe" --background --python export-scene.py -- "city.blend:Main" "../dist/city.scene"

../dist/city.pnct : city.blend export-meshes.py
    "E:\Program Files\Blender Foundation\Blender 2.90\blender.exe" --background --python export-meshes.py -- "city.blend:Main" "../dist/city.pnct" 