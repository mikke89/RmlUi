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
