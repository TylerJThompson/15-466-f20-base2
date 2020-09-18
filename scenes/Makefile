#This makefile for Windows was originally made by Dominic Calkosz and made available in the class Discord.
#I did not write this makefile originally, I only edited it for scenes besides hexapod.

all : \
    ../dist/hexapod.pnct \
    ../dist/hexapod.scene \

../dist/hexapod.scene : hexapod.blend export-scene.py
    "E:\Program Files\Blender Foundation\Blender 2.90\blender.exe" --background --python export-scene.py -- "hexapod.blend:Main" "../dist/hexapod.scene"

../dist/hexapod.pnct : hexapod.blend export-meshes.py
    "E:\Program Files\Blender Foundation\Blender 2.90\blender.exe" --background --python export-meshes.py -- "hexapod.blend:Main" "../dist/hexapod.pnct" 