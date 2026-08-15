slangc -lang slang avg.slang.h -target glsl -profile glsl_450 -force-glsl-scalar-layout -entry main -o spv_c/a_vg.vert.glsl
slangc -lang slang avg.slang.h -target glsl -profile glsl_450 -force-glsl-scalar-layout -entry fragMain -o spv_c/a_vg.frag.glsl 

slangc base3d.slang -target glsl -profile glsl_450 -force-glsl-scalar-layout -entry main -o spv_c/a_base3d.vert.glsl  
slangc base3d.slang -target glsl -profile glsl_450 -force-glsl-scalar-layout -entry fragMain -o spv_c/a_base3d.frag.glsl 

slangc base3d.slang -target glsl -profile glsl_450 -force-glsl-scalar-layout -entry main -D COLOR_MASK=1 -o spv_c/a_base3d_mask.vert.glsl  
slangc base3d.slang -target glsl -profile glsl_450 -force-glsl-scalar-layout -entry fragMain -D COLOR_MASK=1 -o spv_c/a_base3d_mask.frag.glsl 

slangc base3d.slang -target glsl -profile glsl_450 -force-glsl-scalar-layout -entry main -D DOUBLESIDEDCOLOR -o spv_c/a_base3d_dsc.vert.glsl  
slangc base3d.slang -target glsl -profile glsl_450 -force-glsl-scalar-layout -entry fragMain -D DOUBLESIDEDCOLOR -o spv_c/a_base3d_dsc.frag.glsl 

slangc base3d.slang -target glsl -profile glsl_450 -force-glsl-scalar-layout -entry main -D ID_INSTANCING=1 -o spv_c/a_base3d_inst.vert.glsl  
slangc base3d.slang -target glsl -profile glsl_450 -force-glsl-scalar-layout -entry fragMain -D ID_INSTANCING=1 -o spv_c/a_base3d_inst.frag.glsl 

slangc base3d.slang -target glsl -profile glsl_450 -force-glsl-scalar-layout -entry main -D DOUBLESIDEDCOLOR -D ID_INSTANCING=1 -o spv_c/a_base3d_dsc_inst.vert.glsl  
slangc base3d.slang -target glsl -profile glsl_450 -force-glsl-scalar-layout -entry fragMain -D DOUBLESIDEDCOLOR -D ID_INSTANCING=1 -o spv_c/a_base3d_dsc_inst.frag.glsl 

glslangValidator avg.vert.glsl.h -V -S vert --vn vg_vert -o spv_c/a_vg0.vert.h
glslangValidator avg.frag.glsl.h -V -S frag --vn vg_frag -o spv_c/a_vg0.frag.h

glslangValidator spv_c/a_vg.vert.glsl -V --vn vg_vert -o spv_c/a_vg.vert.h
glslangValidator spv_c/a_vg.frag.glsl -V --vn vg_frag -o spv_c/a_vg.frag.h

glslangValidator spv_c/a_base3d.vert.glsl -V --vn a_base3d_vert -o spv_c/a_base3d.vert.h
glslangValidator spv_c/a_base3d.frag.glsl -V --vn a_base3d_frag -o spv_c/a_base3d.frag.h

glslangValidator spv_c/a_base3d_mask.vert.glsl -V --vn a_base3d_mask_vert -o spv_c/a_base3d_mask.vert.h
glslangValidator spv_c/a_base3d_mask.frag.glsl -V --vn a_base3d_mask_frag -o spv_c/a_base3d_mask.frag.h

glslangValidator spv_c/a_base3d_dsc.vert.glsl -V --vn a_base3d_dsc_vert -o spv_c/a_base3d_dsc.vert.h
glslangValidator spv_c/a_base3d_dsc.frag.glsl -V --vn a_base3d_dsc_frag -o spv_c/a_base3d_dsc.frag.h

glslangValidator spv_c/a_base3d_inst.vert.glsl -V --vn a_base3d_inst_vert -o spv_c/a_base3d_inst.vert.h
glslangValidator spv_c/a_base3d_inst.frag.glsl -V --vn a_base3d_inst_frag -o spv_c/a_base3d_inst.frag.h

glslangValidator spv_c/a_base3d_dsc_inst.vert.glsl -V --vn a_base3d_dsc_inst_vert -o spv_c/a_base3d_dsc_inst.vert.h
glslangValidator spv_c/a_base3d_dsc_inst.frag.glsl -V --vn a_base3d_dsc_inst_frag -o spv_c/a_base3d_dsc_inst.frag.h

