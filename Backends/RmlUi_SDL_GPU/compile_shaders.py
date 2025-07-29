# This source file is part of RmlUi, the HTML/CSS Interface Middleware
#
# For the latest information, see http://github.com/mikke89/RmlUi
#
# Copyright (c) 2008-2014 CodePoint Ltd, Shift Technology Ltd, and contributors
# Copyright (c) 2019-2023 The RmlUi Team, and contributors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import sys
import os
import subprocess

# Compiles all .frag and .vert files in this directory to SPIR-V, MSL, and DXIL binary C character arrays.
#
# Requires the SDL_shadercross tool. Binaries can be found here: https://github.com/libsdl-org/SDL_shadercross/actions
# Click on the latest workflow and download them for the platform of your choice.

if len(sys.argv) < 2:
    print("Usage: python compile_shaders.py <shadercross_path>")
    sys.exit(1)

out_file = "ShadersCompiledSPV.h"

current_dir = os.path.dirname(os.path.realpath(__file__));
shadercross_path = os.path.realpath(sys.argv[1])

if not os.path.isfile(shadercross_path):
    print(f"Error: The specified shadercross path '{shadercross_path}' does not exist.")
    sys.exit(1)

out_path = os.path.join(current_dir, out_file)

with open(out_path,'w') as result_file:
	result_file.write('// RmlUi SDL GPU shaders compiled using command: \'python compile_shaders.py\'. Do not edit manually.\n\n#include <stdint.h>\n')

	for file in os.listdir(current_dir):
		if file.endswith(".vert") or file.endswith(".frag"):
			shader_path = os.path.join(current_dir, file)

			def compile(target):
				print("Compiling '{}' to '{}' using SDL_shadercross.".format(file, target))

				variable_name = "{}_{}".format(os.path.splitext(file)[0], target)
				temp_path = os.path.join(current_dir, ".temp.{}".format(target))

				print(temp_path)

				subprocess.run([shadercross_path, "-s", "hlsl", shader_path, "-o", temp_path, "-d", target], check = True)

				print("Success, writing output to variable '{}' in {}".format(variable_name, out_file))

				i = 0
				result_file.write('\nalignas(uint32_t) static const unsigned char {}[] = {{'.format(variable_name))
				for b in open(temp_path, 'rb').read():
					if i % 20 == 0:
						result_file.write('\n\t')
					result_file.write('0x%02X,' % b)
					i += 1

				result_file.write('\n};\n')

				os.remove(temp_path)

			compile("spirv")
			compile("msl")
			compile("dxil")
