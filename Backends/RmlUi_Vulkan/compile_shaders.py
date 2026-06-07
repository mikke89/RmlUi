import sys
import os
import subprocess
from datetime import datetime

now = datetime.now()

# Format: Year-Month-Day Hour:Minute:Second
formatted_time = now.strftime("%Y-%m-%d %H:%M:%S")

# Compiles all .frag and .vert files in this directory to SPIR-V binary C character arrays. Requires 'glslc' installed and available system-wide.

out_file = "ShadersCompiledSPV.h"

current_dir = os.path.dirname(os.path.realpath(__file__));

temp_spirv_path = os.path.join(current_dir, ".temp.spv")
out_path = os.path.join(current_dir, out_file)

variable_names = []

with open(out_path,'w') as result_file:
	result_file.write(f'// RmlUi SPIR-V Vulkan shaders compiled using command: \'python compile_shaders.py\'. Do not edit manually.\n// Compilation date: {formatted_time}\n\n#include <stdint.h>\n')

	for file in os.listdir(current_dir):
		if file.endswith(".vert") or file.endswith(".frag"):
			shader_path = os.path.join(current_dir, file)
			variable_name = os.path.splitext(file)[0]

			print("Compiling '{}' to SPIR-V using glslc.".format(file))

			subprocess.run(["glslc", shader_path, "-o", temp_spirv_path], check = True)

			print("Success, writing output to variable '{}' in {}".format(variable_name, out_file))

			i = 0
			result_file.write('\ninline alignas(uint32_t) constexpr const unsigned char {}[] = {{'.format(variable_name))
			for b in open(temp_spirv_path, 'rb').read():
				if i % 20 == 0:
					result_file.write('\n\t')
				result_file.write('0x%02X,' % b)
				i += 1

			result_file.write('\n};\n')

			variable_names.append(variable_name)

			os.remove(temp_spirv_path)

	result_file.write('\nenum class eVKShaderID : int\n{\n')
	total_size = len(variable_names)
	for i, variable_name in enumerate(variable_names):
		if i == total_size - 1:
			result_file.write(f'{variable_name},\n')
			result_file.write(f'total_size\n')
		else:
			result_file.write(f'{variable_name},\n')

	result_file.write('};')
