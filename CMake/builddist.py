#!/usr/bin/python
import subprocess
import os
import sys
import getopt
import traceback
import shutil
import re

def Usage(args):
	print(sys.argv[0] + ' [-h] [-s] [-b] [-a] [-v version]')
	print('')
	print(' -h\t: This help screen')
	print(' -s\t: Include full source code and build files')
	print(' -b\t: Include sample binaries')
	print(' -a\t: Create archive using 7z')
	print(' -v\t: Specify RmlUi version')
	print('')
	sys.exit()

def CheckVSVars():
	if 'VCINSTALLDIR' in os.environ:
		return
		
	if not 'VS90COMNTOOLS' in os.environ:
		print("Unable to find VS9 install - check your VS90COMNTOOLS environment variable")
		sys.exit()
		
	path = os.environ['VS90COMNTOOLS']
	subprocess.call('"' + path + 'vsvars32.bat" > NUL && ' + ' '.join(sys.argv))
	sys.exit()
	
def ProcessOptions(args):

	options = {'RMLUI_VERSION': 'custom', 'FULL_SOURCE': False, 'ARCHIVE_NAME': 'RmlUi-sdk', 'SAMPLE_BINARIES': False, 'ARCHIVE': False}
	
	try:
		optlist, args = getopt.getopt(args, 'v:hsba')
	except getopt.GetoptError as e:
		print('\nError: ' + str(e) + '\n')
		Usage(args)
	
	for opt in optlist:
		if opt[0] == '-h':
			Usage(args)
		if opt[0] == '-v':
			options['RMLUI_VERSION'] = opt[1]
		if opt[0] == '-s':
			options['FULL_SOURCE'] = True
			options['ARCHIVE_NAME'] = 'RmlUi-source'
		if opt[0] == '-b':
			options['SAMPLE_BINARIES'] = True
		if opt[0] == '-a':
			options['ARCHIVE'] = True
			
	return options
		
def Build(project, configs, defines = {}):

	old_cl = ''
	if 'CL' in os.environ:
		old_cl = os.environ['CL']
	else:
		os.environ['CL'] = ''

	for name, value in defines.items():
		os.environ['CL'] = os.environ['CL'] + ' /D' + name + '=' + value
		
	for config in configs:
		cmd = '"' + os.environ['VCINSTALLDIR'] + '\\vcpackages\\vcbuild.exe" /rebuild ' + project + '.vcproj "' + config + '|Win32"'
		ret = subprocess.call(cmd)
		if ret != 0:
			print("Failed to build " + project)
			sys.exit()
			
	os.environ['CL'] = old_cl
	
def DelTree(path):
	if not os.path.exists(path):
		return
		
	print('Deleting ' + path + '...')
	for root, dirs, files in os.walk(path, topdown=False):
		for name in files:
			os.remove(os.path.join(root, name))
		for name in dirs:
			os.rmdir(os.path.join(root, name))

def CopyFiles(source_path, destination_path, file_list = [], exclude_directories = [], exclude_files = [], preserve_paths = True):
	working_directory = os.getcwd()
	source_directory = os.path.abspath(os.path.join(working_directory, os.path.normpath(source_path)))
	destination_directory = os.path.abspath(os.path.join(working_directory, os.path.normpath(destination_path)))
	print("Copying " + source_directory + " to " + destination_directory + " ...")
	
	if not os.path.exists(source_directory):
		print("Warning: Source directory " + source_directory + " doesn't exist.")
		return False
	
	for root, directories, files in os.walk(source_directory, topdown=True):
		directories[:] = [d for d in directories if d not in exclude_directories]
		
		for file in files:
			# Skip files not in the include list.
			if len(file_list) > 0:
				included = False
				for include in file_list:
					if re.search(include, file):
						included = True
						break;

				if not included:
					continue
			
			# Determine our subdirectory.
			subdir = root.replace(source_directory, "")
			if subdir[:1] == os.path.normcase('/'):
				subdir = subdir[1:]
			
			# Skip paths in the exclude list
			excluded = False
			for exclude in exclude_files:
				if re.search(exclude, file):
					excluded = True
					break
					
			if excluded:
				continue
			
			# Build up paths
			source_file = os.path.join(root, file)
			destination_subdir = destination_directory
			if preserve_paths:
				destination_subdir = os.path.join(destination_directory, subdir)
			
			if not os.path.exists(destination_subdir):
				os.makedirs(destination_subdir)
			destination_file = os.path.join(destination_subdir, file)
			
			# Copy files
			try:
				shutil.copy(source_file, destination_file)
			except:
				print("Failed copying " + source_file + " to " + destination_file)
				traceback.print_exc()
					
	return True
	
def Archive(archive_name, path):
	cwd = os.getcwd()
	os.chdir(path + '/..')
	file_name = archive_name + '.zip'
	if os.path.exists(file_name):
		os.unlink(file_name)
	os.system('7z a ' + file_name + ' ' + path[path.rfind('/')+1:])
	os.chdir(cwd)
	
def main():
	options = ProcessOptions(sys.argv[1:])
	
	#CheckVSVars()
	#Build('RmlCore', ['Debug', 'Release'], {'RMLUI_VERSION': '\\"' + options['RMLUI_VERSION'] + '\\"'})
	#Build('RmlControls', ['Debug', 'Release'])
	#Build('RmlDebugger', ['Debug', 'Release'])
	
	DelTree('../Distribution/RmlUi')
	CopyFiles('../Include', '../Distribution/RmlUi/Include')
	CopyFiles('../Build', '../Distribution/RmlUi/Build', ['\.dll$', '^Rml.*\.lib$'], ['CMakeFiles'])
	CopyFiles('../CMake', '../Distribution/RmlUi/CMake', ['\.cmake$', '\.in$', '\.plist$', '\.py$', '\.sh$'])
	CopyFiles('../Samples', '../Distribution/RmlUi/Samples', ['\.h$', '\.cpp$', '\.rml$', '\.rcss$', '\.tga$', '\.py$', '\.otf$', '\.ttf$', '\.txt$'])
	if options['FULL_SOURCE']:
		CopyFiles('../Build', '../Distribution/RmlUi/Build', ['\.vcxproj$', '\.sln$', '\.vsprops$', '\.py$'], ['CMakeFiles'])
		CopyFiles('../Source', '../Distribution/RmlUi/Source', ['\.cpp$', '\.h$', '\.inl$'])
	if options['SAMPLE_BINARIES']:
		CopyFiles('../Build', '../Distribution/RmlUi/Build', ['\.exe$'], ['CMakeFiles'])
	shutil.copyfile('../LICENSE', '../Distribution/RmlUi/LICENSE')
	shutil.copyfile('../readme.md', '../Distribution/RmlUi/readme.md')
	if options['ARCHIVE']:
		Archive(options['ARCHIVE_NAME'] + '-' + options['RMLUI_VERSION'], '../Distribution/RmlUi');
	
if __name__ == '__main__':
	main()