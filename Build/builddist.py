#!/usr/bin/python
import subprocess
import os
import sys
import getopt
import traceback
import shutil
import re

def Usage(args):
	print sys.argv[0] + ' [-hp] [-r revision]'
	print ''
	print ' -r\t: Specify rocket internal revision number'
	print ' -p\t: Include python libraries'
	print ' -s\t: Include full source code and build files'
	print ' -h\t: This help screen'
	print ''
	sys.exit()

def CheckVSVars():
	if 'VCINSTALLDIR' in os.environ:
		return
		
	if not 'VS90COMNTOOLS' in os.environ:
		print "Unable to find VS9 install - check your VS90COMNTOOLS environment variable"
		sys.exit()
		
	path = os.environ['VS90COMNTOOLS']
	subprocess.call('"' + path + 'vsvars32.bat" > NUL && ' + ' '.join(sys.argv))
	sys.exit()
	
def ProcessOptions(args):

	options = {'ROCKET_VERSION': 'custom', 'BUILD_PYTHON': False, 'FULL_SOURCE': False, 'ARCHIVE_NAME': 'libRocket-sdk'}
	
	try:
		optlist, args = getopt.getopt(args, 'r:phs')
	except getopt.GetoptError, e:
		print '\nError: ' + str(e) + '\n'
		Usage(args)
	
	for opt in optlist:
		if opt[0] == '-h':
			Usage(args)
		if opt[0] == '-r':
			options['ROCKET_VERSION'] = opt[1]
		if opt[0] == '-p':
			options['BUILD_PYTHON'] = True
		if opt[0] == '-s':
			options['FULL_SOURCE'] = True
			options['ARCHIVE_NAME'] = 'libRocket-source'
			
	return options
		
def Build(project, configs, defines = {}):

	old_cl = ''
	if 'CL' in os.environ:
		old_cl = os.environ['CL']
	else:
		os.environ['CL'] = ''

	for name, value in defines.iteritems():
		os.environ['CL'] = os.environ['CL'] + ' /D' + name + '=' + value
		
	for config in configs:
		cmd = '"' + os.environ['VCINSTALLDIR'] + '\\vcpackages\\vcbuild.exe" /rebuild ' + project + '.vcproj "' + config + '|Win32"'
		ret = subprocess.call(cmd)
		if ret != 0:
			print "Failed to build " + project
			sys.exit()
			
	os.environ['CL'] = old_cl
	
def DelTree(path):
	if not os.path.exists(path):
		return
		
	print 'Deleting ' + path + '...'
	for root, dirs, files in os.walk(path, topdown=False):
		for name in files:
			os.remove(os.path.join(root, name))
		for name in dirs:
			os.rmdir(os.path.join(root, name))

def CopyFiles(source_path, destination_path, file_list = [], exclude_list = [], preserve_paths = True):
	working_directory = os.getcwd()
	source_directory = os.path.abspath(os.path.join(working_directory, os.path.normpath(source_path)))
	destination_directory = os.path.abspath(os.path.join(working_directory, os.path.normpath(destination_path)))
	print "Copying " + source_directory + " to " + destination_directory + " ..."
	
	if not os.path.exists(source_directory):
		print "Warning: Source directory " + source_directory + " doesn't exist."
		return False
	
	for root, directories, files in os.walk(source_directory, False):
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
				for exclude in exclude_list:
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
					print "Failed copying " + source_file + " to " + destination_file
					traceback.print_exc()
					
	return True
	
def Archive(archive_name, path):
	cwd = os.getcwd()
	os.chdir(path + '/..')
	file_name = archive_name + '.zip'
	if os.path.exists(file_name):
		os.unlink(file_name)
	os.system('zip -r ' + file_name + ' ' + path[path.rfind('/')+1:])
	os.chdir(cwd)
	
def main():
	CheckVSVars()
	options = ProcessOptions(sys.argv[1:])
	
	Build('RocketCore', ['Debug', 'Release'], {'ROCKET_VERSION': '\\"' + options['ROCKET_VERSION'] + '\\"'})
	Build('RocketControls', ['Debug', 'Release'])
	Build('RocketDebugger', ['Debug', 'Release'])
	if options['BUILD_PYTHON']:
		Build('RocketCorePython', ['Debug', 'Release'])
		Build('RocketControlsPython', ['Debug', 'Release'])
		
	DelTree('../dist/libRocket')
	CopyFiles('../Include', '../dist/libRocket/Include')
	CopyFiles('../bin', '../dist/libRocket/bin', ['\.dll$', '^[^_].*\.lib$', '\.py$', '\.pyd$'])
	CopyFiles('../Samples', '../dist/libRocket/Samples', ['\.h$', '\.cpp$', '\.vcproj$', '\.sln$', '\.vcproj\.user$', '\.rml$', '\.rcss$', '\.tga$', '\.py$', '\.otf$', '\.txt$'])
	if options['FULL_SOURCE']:
		CopyFiles('../Build', '../dist/libRocket/Build', ['\.vcproj$', '\.sln$', '\.vsprops$', '\.py$'])
		CopyFiles('../Source', '../dist/libRocket/Source', ['\.cpp$', '\.h$', '\.inl$'])
	shutil.copyfile('../changelog.txt', '../dist/libRocket/changelog.txt')
	Archive(options['ARCHIVE_NAME'] + '-' + options['ROCKET_VERSION'], '../dist/libRocket');
	
if __name__ == '__main__':
	main()