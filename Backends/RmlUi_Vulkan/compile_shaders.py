import sys
import os
import shutil
import subprocess
from datetime import datetime

now = datetime.now()

# Format: Year-Month-Day Hour:Minute:Second
formatted_time = now.strftime("%Y-%m-%d %H:%M:%S")

# Compiles all .frag and .vert files in this directory to SPIR-V binary C character arrays.
# Requires 'glslc' (shaderc) or 'glslang'/'glslangValidator' (glslang) installed and available system-wide.

out_spv_filename = "ShadersCompiledSPV.h"
out_list_filename = "ShadersList.h"

current_dir = os.path.dirname(os.path.realpath(__file__));
temp_spirv_path = os.path.join(current_dir, ".temp.spv")

def find_glsl_compiler():
	if shutil.which("glslc"):
		return ("glslc", lambda shader_path, out_path: ["glslc", shader_path, "-o", out_path])
	if shutil.which("glslang"):
		return ("glslang", lambda shader_path, out_path: ["glslang", "-V", shader_path, "-o", out_path])
	if shutil.which("glslangValidator"):
		return ("glslangValidator", lambda shader_path, out_path: ["glslangValidator", "-V", shader_path, "-o", out_path])
	return (None, None)

compiler_name, compiler_command = find_glsl_compiler()

if compiler_command is None:
	raise RuntimeError("No GLSL compiler found on PATH: install 'glslc' (shaderc) or 'glslang' (glslangValidator).")

variable_names = []

common_header = f'''#pragma once

// RmlUi SPIR-V Vulkan shaders compiled using command: \'python compile_shaders.py\' with \'{compiler_name}\'. Do not edit manually.
// Compilation date: {formatted_time}
'''

with open(os.path.join(current_dir, out_spv_filename), 'w') as result_file:
	result_file.write(common_header)
	result_file.write('\n#include <stdint.h>\n')

	for file in sorted(os.listdir(current_dir)):
		if file.endswith(".vert") or file.endswith(".frag"):
			shader_path = os.path.join(current_dir, file)
			variable_name = os.path.splitext(file)[0]

			print("Compiling '{}' to SPIR-V using {}.".format(file, compiler_name))

			subprocess.run(compiler_command(shader_path, temp_spirv_path), check = True)

			print("Success, writing output to variable '{}' in {}".format(variable_name, out_spv_filename))

			i = 0
			result_file.write('\nalignas(uint32_t) static constexpr unsigned char {}[] = {{'.format(variable_name))
			for b in open(temp_spirv_path, 'rb').read():
				if i % 20 == 0:
					result_file.write('\n\t')
				result_file.write('0x%02X,' % b)
				i += 1

			result_file.write('\n};\n')

			variable_names.append(variable_name)

			os.remove(temp_spirv_path)


with open(os.path.join(current_dir, out_list_filename), 'w') as result_file:
	result_file.write(common_header)
	result_file.write('\nenum class eVKShaderID : int {\n')
	total_size = len(variable_names)
	for variable_name in variable_names:
		result_file.write(f'\t{variable_name},\n')

	result_file.write('\tcount,\n')
	result_file.write('};\n')
