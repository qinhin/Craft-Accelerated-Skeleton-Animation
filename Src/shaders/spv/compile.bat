cd %~dp0

glslangValidator.exe -V debug_skinning.frag -o ./debug_skinning.frag.spv
glslangValidator.exe -V debug_skinning.vert -o ./debug_skinning.vert.spv

glslangValidator.exe -V animation_pass1.comp -o ./animation_pass1.comp.spv
glslangValidator.exe -V animation_pass2.comp -o ./animation_pass2.comp.spv

echo ±‡“ÎÕÍ±œ
pause

# glslangValidator.exe -i -q -V animation_pass1.comp -o ./animation_pass1.comp.spv