slangc vg.slang -target spirv -profile glsl_450 -force-glsl-scalar-layout -entry main -o a_vg.vert.spv  
slangc vg.slang -target spirv -profile glsl_450 -force-glsl-scalar-layout -entry fragMain -o a_vg.frag.spv 

slangc base3d.slang -target spirv -profile glsl_450 -force-glsl-scalar-layout -entry main -o a_base3d.vert.spv  
slangc base3d.slang -target spirv -profile glsl_450 -force-glsl-scalar-layout -entry fragMain -o a_base3d.frag.spv 

slangc base3d.slang -target spirv -profile glsl_450 -force-glsl-scalar-layout -entry main -D DOUBLESIDEDCOLOR -o a_base3d_dsc.vert.spv  
slangc base3d.slang -target spirv -profile glsl_450 -force-glsl-scalar-layout -entry fragMain -D DOUBLESIDEDCOLOR -o a_base3d_dsc.frag.spv 

slangc base3d.slang -target spirv -profile glsl_450 -force-glsl-scalar-layout -entry main -D ID_INSTANCING -o a_base3d_inst.vert.spv  
slangc base3d.slang -target spirv -profile glsl_450 -force-glsl-scalar-layout -entry fragMain -D ID_INSTANCING -o a_base3d_inst.frag.spv 

slangc base3d.slang -target spirv -profile glsl_450 -force-glsl-scalar-layout -entry main -D DOUBLESIDEDCOLOR -D ID_INSTANCING -o a_base3d_dsc_inst.vert.spv  
slangc base3d.slang -target spirv -profile glsl_450 -force-glsl-scalar-layout -entry fragMain -D DOUBLESIDEDCOLOR -D ID_INSTANCING -o a_base3d_dsc_inst.frag.spv 
