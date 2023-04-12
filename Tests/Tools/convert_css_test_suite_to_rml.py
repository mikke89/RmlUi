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

import os
import re
import sys
import argparse

parser = argparse.ArgumentParser(description=\
'''Convert the W3C CSS 2.1 test suite to RML documents for testing in RmlUi.

Fetch the CSS tests archive from here: https://www.w3.org/Style/CSS/Test/CSS2.1/
Extract the 'xhtml1' folder and point the 'in_dir' argument to this directory.''')

parser.add_argument('in_dir',
                    help="Input directory which contains the 'xhtml1' (.xht) files to be converted.")
parser.add_argument('out_dir',
                    help="Output directory for the converted RML files.")
parser.add_argument('--clean', action='store_true',
                    help='Will *delete* all existing *.rml files in the output directory.')
parser.add_argument('--match',
                    help="Only process file names containing the given string.")

args = parser.parse_args()

in_dir = args.in_dir
out_dir = args.out_dir
out_ref_dir = os.path.join(out_dir, r'reference')
match_files = args.match

if not os.path.isdir(in_dir):
	print("Error: Specified input directory '{}' does not exist.".format(out_dir))
	exit()

if not os.path.exists(out_dir):
	try: 
		os.mkdir(out_dir)
	except Exception as e:
		print('Error: Failed to create output directory {}'.format(out_dir))

if not os.path.exists(out_ref_dir):
	try: 
		os.mkdir(out_ref_dir)
	except Exception as e:
		print('Error: Failed to create reference output directory {}'.format(out_ref_dir))

if not os.path.isdir(out_dir) or not os.path.isdir(out_ref_dir):
	print("Error: Specified output directory '{}' or reference '{}' are not directories.".format(out_dir, out_ref_dir))
	exit()

if args.clean:
	print("Deleting all *.rml files in output directory '{}' and reference directory '{}'".format(out_dir, out_ref_dir))

	for del_dir in [out_dir, out_ref_dir]:
		for file in os.listdir(del_dir):
			path = os.path.join(del_dir, file)
			try:
				if os.path.isfile(path) and file.endswith('.rml'):
					os.unlink(path)
			except Exception as e:
				print('Failed to delete {}. Reason: {}'.format(path, e))

html_color_mapping = {
	"lightblue": "#add8e6",
	"lightgrey": "#d3d3d3",
	"lightgray": "#d3d3d3",
	"lightgreen": "#90ee90",
	"pink": "#ffc0cb",
	"coral": "#ff7f50",
	"slateblue": "#6a5acd",
	"steelblue": "#4682b4",
	"tan": "#d2b48c",
	"violet": "#ee82ee",
}

def border_format(side: str, type: str, content: str):
	# Side: (empty)/-top/-right/-bottom/-left
	# Type: (empty)/-width/-style/-color
	
	content = content.replace("thick", "5px")
	content = content.replace("medium", "3px")
	content = content.replace("thin", "1px")

	if type == "-width":
		return "border" + side + type + ": " + content
	if type == "-color":
		color = content.strip()
		if color in html_color_mapping:
			color = html_color_mapping[color]
		return "border" + side + type + ": " + color

	# Convert style to width. This is not perfect, but seems to be the most used case.
	if type == "-style":
		content = content.replace("none", "0px").replace("hidden", "0px")
		# We may want to only match "solid" here, and cancel the test if it contains any other styles which are unsupported.
		content = re.sub(r'\b[a-z]+\b', '3px', content, flags = re.IGNORECASE)
		return "border" + side + "-width: " + content

	# Next are the shorthand properties, they should contain max a single size, a single style, and a single color.
	width = re.search(r'\b([0-9]+(\.[0-9]+)?[a-z]+|0)\b', content, flags = re.IGNORECASE)
	if width:
		width = width.group(1)

	style_pattern = r'none|solid|hidden|dotted|dashed|double|groove|ridge|inset|outset|sold'
	style = re.search(style_pattern, content, flags = re.IGNORECASE)
	if style:
		style = style.group(0)
		if style == "none" or style == "hidden":
			width = "0px"

	content = re.sub(style_pattern, "", content)
	color = re.search(r'\b([a-z]+|#[0-9a-f]+)\b', content)
	if color:
		color = color.group(1)
		if color in html_color_mapping:
			color = html_color_mapping[color]
	else:
		color = "black"

	width = width or "3px"

	return "border" + side + ": " + width + " " + color


def border_find_replace(line: str):
	new_line = ""
	prev_end = 0

	pattern = r"border(-(top|right|bottom|left))?(-(width|style|color))?:([^;}\"]+)([;}\"])"
	for match in re.finditer(pattern, line, flags = re.IGNORECASE):
		side = match.group(1) or ""
		type = match.group(3) or ""
		content = match.group(5)
		suffix = match.group(6)

		replacement = border_format(side, type, content) + suffix

		new_line += line[prev_end:match.start()] + replacement
		prev_end = match.end()

	new_line += line[prev_end:]

	return new_line

assert( border_find_replace("margin:10px; border:20px solid black; padding:30px;") == 'margin:10px; border: 20px black; padding:30px;' )
assert( border_find_replace(" border-left: 7px solid navy; border-right: 17px solid navy; } ") == ' border-left: 7px navy; border-right: 17px navy; } ' )
assert( border_find_replace(" border: blue solid 3px; ") == ' border: 3px blue; ' )
assert( border_find_replace(" border: solid lime; ") == ' border: 3px lime; ' )
assert( border_find_replace(" border: 1px pink; ") == ' border: 1px #ffc0cb; ' )
assert( border_find_replace(" border-color: pink; ") == ' border-color: #ffc0cb; ' )
assert( border_find_replace(" border: 0; ") == ' border: 0 black; ' )
assert( border_find_replace(" border-bottom: 0.25em solid green; ") == ' border-bottom: 0.25em green; ' )
assert( border_find_replace(" border-width: 0; ") == ' border-width:  0; ' )
assert( border_find_replace(" border-left: orange solid 1em; ") == ' border-left: 1em orange; ' )
assert( border_find_replace(" border-style: solid none solid solid; ") == ' border-width:  3px 0px 3px 3px; ' )
assert( border_find_replace("   border: solid; border-style: solid none solid solid; border-style: solid solid solid none; ") == '   border: 3px black; border-width:  3px 0px 3px 3px; border-width:  3px 3px 3px 0px; ' )
assert( border_find_replace(" p + .set {border-top: solid orange} ") == ' p + .set {border-top: 3px orange} ' )
assert( border_find_replace(r'<span style="border-right: none; border-left: none" class="outer">') == '<span style="border-right: 0px black; border-left: 0px black" class="outer">' )


reference_links = []

def process_file(in_file):
	
	in_path = os.path.join(in_dir, in_file)
	out_file = os.path.splitext(in_file)[0] + '.rml'
	out_path = os.path.join(out_dir, out_file)
	
	f = open(in_path, 'r', encoding="utf8")
	lines = f.readlines()
	f.close()

	data = ''
	reference_link = ''
	in_style = False

	for line in lines:
		if re.search(r'<style', line, flags = re.IGNORECASE):
			in_style = True
		if re.search(r'</style', line, flags = re.IGNORECASE):
			in_style = False
		
		if in_style:
			line = re.sub(r'(^|[^<])html', r'\1body', line, flags = re.IGNORECASE)
			line = re.sub(r'<!--', r'/*', line, flags = re.IGNORECASE)
			line = re.sub(r'-->', r'*/', line, flags = re.IGNORECASE)

		reference_link_search_candidates = [
			r'(<link href="(reference/[^"]+))\.xht(" rel="match" ?/>)',
			r'(<link rel="match" href="(reference/[^"]+))\.xht(" ?/>)',
			]

		for reference_link_search in reference_link_search_candidates:
			reference_link_match = re.search(reference_link_search, line, flags = re.IGNORECASE)
			if reference_link_match:
				reference_link = reference_link_match[2] + '.xht'
				line = re.sub(reference_link_search, r'\1.rml\3', line, flags = re.IGNORECASE)
				break

		line = re.sub(r'<!DOCTYPE[^>]*>\s*', '', line, flags = re.IGNORECASE)
		line = re.sub(r' xmlns="[^"]+"', '', line, flags = re.IGNORECASE)
		line = re.sub(r'<(/?)html[^>]*>', r'<\1rml>', line, flags = re.IGNORECASE)
		line = re.sub(r'^(\s*)(.*<head[^>]*>)', r'\1\2\n\1\1<link type="text/rcss" href="/../Tests/Data/style.rcss" />', line, flags = re.IGNORECASE)
		line = re.sub(r'<style[^>]*><!\[CDATA\[\s*', '<style>\n', line, flags = re.IGNORECASE)
		line = re.sub(r'\]\]></style>', '</style>', line, flags = re.IGNORECASE)
		line = re.sub(r'direction:\s*ltr\s*;?', r'', line, flags = re.IGNORECASE)
		line = re.sub(r'list-style(-type)?:\s*none\s*;?', r'', line, flags = re.IGNORECASE)
		line = re.sub(r'(font(-size):[^;}\"]*)xxx-large', r'\1 2.0em', line, flags = re.IGNORECASE)
		line = re.sub(r'(font(-size):[^;}\"]*)xx-large', r'\1 1.7em', line, flags = re.IGNORECASE)
		line = re.sub(r'(font(-size):[^;}\"]*)x-large', r'\1 1.3em', line, flags = re.IGNORECASE)
		line = re.sub(r'(font(-size):[^;}\"]*)large', r'\1 1.15em', line, flags = re.IGNORECASE)
		line = re.sub(r'(font(-size):[^;}\"]*)medium', r'\1 1.0em', line, flags = re.IGNORECASE)
		line = re.sub(r'(font(-size):[^;}\"]*)small', r'\1 0.9em', line, flags = re.IGNORECASE)
		line = re.sub(r'(font(-size):[^;}\"]*)x-small', r'\1 0.7em', line, flags = re.IGNORECASE)
		line = re.sub(r'(font(-size):[^;}\"]*)xx-small', r'\1 0.5em', line, flags = re.IGNORECASE)
		line = re.sub(r'font:[^;}]*\b([0-9]+[a-z]+)\b[^;}]*([;}])', r'font-size: \1 \2', line, flags = re.IGNORECASE)
		line = re.sub(r'font-family:[^;}]*[;}]', r'', line, flags = re.IGNORECASE)
		line = re.sub(r'(line-height:)\s*normal', r'\1 1.2em', line, flags = re.IGNORECASE)
		line = re.sub(r'-moz-box-sizing', r'box-sizing', line, flags = re.IGNORECASE)
		line = re.sub(r'cyan', r'aqua', line, flags = re.IGNORECASE)

		line = re.sub(r'flex: none;', r'flex: 0 0 auto;', line, flags = re.IGNORECASE)
		line = re.sub(r'align-content:\s*(start|end)', r'align-content: flex-\1', line, flags = re.IGNORECASE)
		line = re.sub(r'align-content:\s*space-evenly', r'align-content: space-around', line, flags = re.IGNORECASE)
		line = re.sub(r'justify-content:\s*space-evenly', r'justify-content: space-around', line, flags = re.IGNORECASE)
		line = re.sub(r'justify-content:\s*left', r'justify-content: flex-start', line, flags = re.IGNORECASE)
		line = re.sub(r'justify-content:\s*right', r'justify-content: flex-end', line, flags = re.IGNORECASE)
		line = re.sub(r'table-layout:[^;}]*[;}]', r'', line, flags = re.IGNORECASE)

		if re.search(r'background:[^;}\"]*fixed', line, flags = re.IGNORECASE):
			print("File '{}' skipped since it uses unsupported background.".format(in_file))
			return False
		line = re.sub(r'background:(\s*([a-z]+|#[0-9a-f]+)\s*[;}\"])', r'background-color:\1', line, flags = re.IGNORECASE)
		prev_end = 0
		new_line = ""
		for match in re.finditer(r'background-color:([^;]*)([;"])', line, flags = re.IGNORECASE):
			color = match.group(1).strip()
			delimiter = match.group(2)
			if color in html_color_mapping:
				color = html_color_mapping[color]
			new_line += line[prev_end:match.start()] + 'background-color: ' + color + delimiter
			prev_end = match.end()
			
		new_line += line[prev_end:]
		line = new_line

		prev_end = 0
		new_line = ""
		for match in re.finditer(r'calc\(\s*(\d+)(\w{1,3})\s*/\s*(\d)\s*\)', line, flags = re.IGNORECASE):
			num = match.group(1)
			unit = match.group(2)
			den = match.group(3)
			calc_result = "{}{}".format(float(num) / float(den), unit)
			new_line += line[prev_end:match.start()] + calc_result
			prev_end = match.end()
		new_line += line[prev_end:]
		line = new_line

		line = border_find_replace(line)

		if in_style and not '<' in line:
			line = line.replace('&gt;', '>')
		flags_match = re.search(r'<meta.*name="flags" content="([^"]*)" ?/>', line, flags = re.IGNORECASE) or re.search(r'<meta.*content="([^"]*)".*name="flags".*?/>', line, flags = re.IGNORECASE)
		if flags_match and flags_match[1] != '' and flags_match[1] != 'interactive':
			print("File '{}' skipped due to flags '{}'".format(in_file, flags_match[1]))
			return False
		if re.search(r'display:[^;]*(inline-table|table-caption|table-header-group|table-footer-group|run-in|list-item|inline-flex)', line, flags = re.IGNORECASE):
			print("File '{}' skipped since it uses unsupported display modes.".format(in_file))
			return False
		if re.search(r'visibility:[^;]*collapse|z-index:\s*[0-9\.]+%', line, flags = re.IGNORECASE):
			print("File '{}' skipped since it uses unsupported visibility.".format(in_file))
			return False
		if re.search(r'data:|support/|<img|<iframe', line, flags = re.IGNORECASE):
			print("File '{}' skipped since it uses data or images.".format(in_file))
			return False
		if re.search(r'<script>', line, flags = re.IGNORECASE):
			print("File '{}' skipped since it uses scripts.".format(in_file))
			return False
		if in_style and re.search(r':before|:after|@media|\s\+\s', line, flags = re.IGNORECASE):
			print("File '{}' skipped since it uses unsupported CSS selectors.".format(in_file))
			return False
		if re.search(r'(: ?inherit ?;)|(!\s*important)|[0-9\.]+(ch|ex)[\s;}]', line, flags = re.IGNORECASE):
			print("File '{}' skipped since it uses unsupported CSS values.".format(in_file))
			return False
		if re.search(r'@font-face|font:|ahem', line, flags = re.IGNORECASE):
			print("File '{}' skipped since it uses special fonts.".format(in_file))
			return False
		if re.search(r'\b((direction:[^;]*[;"])|(content:[^;]*[;"])|(outline:[^;]*[;"])|(quote:[^;]*[;"])|(border-spacing:[^;]*[;"])|(border-collapse:[^;]*[;"])|(background:[^;]*[;"]))', line, flags = re.IGNORECASE)\
			or re.search(r'\b((font-variant:[^;]*[;"])|(font-kerning:[^;]*[;"])|(font-feature-settings:[^;]*[;"])|(background-image:[^;]*[;"])|(caption-side:[^;]*[;"])|(clip:[^;]*[;"])|(page-break-inside:[^;]*[;"])|(word-spacing:[^;]*[;"]))', line, flags = re.IGNORECASE)\
			or re.search(r'\b((writing-mode:[^;]*[;"])|(text-orientation:[^;]*[;"])|(text-indent:[^;]*[;"])|(page-break-after:[^;]*[;"])|(page-break-before:[^;]*[;"])|(column[^:]*:[^;]*[;"])|(empty-cells:[^;]*[;"]))', line, flags = re.IGNORECASE)\
			or re.search(r'\b((aspect-ratio:[^;]*[;"])|(flex-basis:[^;]*[;"])|(order:[^;]*[;"]))', line, flags = re.IGNORECASE):
			print("File '{}' skipped since it uses unsupported CSS properties.".format(in_file))
			return False
		data += line

	f = open(out_path, 'w', encoding="utf8")
	f.write(data)
	f.close()

	if reference_link:
		reference_links.append(reference_link)
	
	print("File '{}' processed successfully!".format(in_file))

	return True


file_block_filters = ['charset','font','list','text-decoration','text-indent','text-transform','bidi','cursor',
					'uri','stylesheet','word-spacing','table-anonymous','outline','at-rule','at-import','attribute',
					'style','quote','rtl','ltr','first-line','first-letter','first-page','import','border','toc',
					'chapter','character-encoding','escape','media','contain-','grid','case-insensitive',
					'containing-block-initial','multicol','system-colors']

def should_block(name):

	for file_block_filter in file_block_filters:
		if file_block_filter in name:
			print("File '{}' skipped due to unsupported feature '{}'".format(name, file_block_filter))
			return True
	return False

in_dir_list = os.listdir(in_dir)
if match_files:
	in_dir_list = [ name for name in in_dir_list if match_files in name ]
total_files = len(in_dir_list)

in_dir_list = [ name for name in in_dir_list if name.endswith(".xht") and not should_block(name) ]

processed_files = 0
processed_reference_files = 0

for in_file in in_dir_list:
	if process_file(in_file):
		processed_files += 1

final_reference_links = reference_links[:]
total_reference_files = len(final_reference_links)
reference_links.clear()

for in_ref_file in final_reference_links:
	if process_file(in_ref_file):
		processed_reference_files += 1

print('\nDone!\n\nTotal test files: {}\nSkipped test files: {}\nParsed test files: {}\n\nTotal reference files: {}\nSkipped reference files: {}\nIgnored alternate references: {}\nParsed reference files: {}'\
	.format(total_files, total_files - processed_files, processed_files, total_reference_files, total_reference_files - processed_reference_files, len(reference_links), processed_reference_files ))
