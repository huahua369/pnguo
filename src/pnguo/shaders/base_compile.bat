glslangValidator base3d.frag.glsl -V --vn base3d_frag -o base3d.frag.h
glslangValidator base3d.vert.glsl -V --vn base3d_vert -o base3d.vert.h
glslangValidator base3d2.frag.glsl -V --vn base3d2_frag -o base3d2.frag.h
glslangValidator base3d2.vert.glsl -V --vn base3d2_vert -o base3d2.vert.h
glslangValidator agauss_cs.comp.glsl.h -V -S comp --vn agauss_cs_comp -o agauss_cs.comp.h
aglslc --target-env=vulkan1.3 -fshader-stage=compute -fentry-point=main agauss_cs.comp.glsl -o agauss_cs.spv.h
