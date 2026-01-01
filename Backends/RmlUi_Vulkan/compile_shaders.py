import sys
import os
import subprocess

# Compiles all .frag and .vert files in this directory to SPIR-V binary C character arrays. Requires 'glslc' installed and available system-wide.

out_file = "ShadersCompiledSPV.h"

current_dir = os.path.dirname(os.path.realpath(__file__));

temp_spirv_path = os.path.join(current_dir, ".temp.spv")
out_path = os.path.join(current_dir, out_file)

with open(out_path,'w') as result_file:
	result_file.write('// RmlUi SPIR-V Vulkan shaders compiled using command: \'python compile_shaders.py\'. Do not edit manually.\n\n#include <stdint.h>\n')

	for file in os.listdir(current_dir):
		if file.endswith(".vert") or file.endswith(".frag"):
			shader_path = os.path.join(current_dir, file)
			variable_name = os.path.splitext(file)[0]

			print("Compiling '{}' to SPIR-V using glslc.".format(file))

			subprocess.run(["glslc", shader_path, "-o", temp_spirv_path], check = True)

			print("Success, writing output to variable '{}' in {}".format(variable_name, out_file))

			i = 0
			result_file.write('\nalignas(uint32_t) static const unsigned char {}[] = {{'.format(variable_name))
			for b in open(temp_spirv_path, 'rb').read():
				if i % 20 == 0:
					result_file.write('\n\t')
				result_file.write('0x%02X,' % b)
				i += 1

			result_file.write('\n};\n')

			os.remove(temp_spirv_path)
