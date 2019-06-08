/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

static const char* log_rcss =
"body\n"
"{\n"
"	width: 400dp;\n"
"   height: 300dp;\n"
"	min-width: 200dp;\n"
"	min-height: 150dp;\n"
"	top: 42dp;\n"
"	left: 20dp;\n"
"}\n"
"div#tools\n"
"{\n"
"	float: right;\n"
"	width: 137dp;\n"
"}\n"
"div.log-entry\n"
"{\n"
"	margin: 3dp 2dp;\n"
"}\n"
"div.log-entry div.icon\n"
"{\n"
"	float: left;\n"
"	display: block;\n"
"	width: 18dp;\n"
"	height: 18dp;\n"
"	text-align: center;\n"
"	border-width: 1px;\n"
"	margin-right: 5dp;\n"
"	font-weight: bold;\n"
"}\n"
"div.button\n"
"{\n"
"	display: inline-block;"
"	width: 30dp;\n"
"	text-align: center;\n"
"	border-width: 1px;\n"
"	font-weight: bold;\n"
"	margin-right: 3dp;\n"
"}\n"
"div.button.last\n"
"{\n"
"	margin-right: 0px;\n"
"}\n"
"div.log-entry p.message\n"
"{\n"
"	display: block;\n"
"	margin-left: 20dp;\n"
"}\n";

static const char* log_rml =
"<h1>\n"
"	<handle id=\"position_handle\" move_target=\"#document\">\n"
"		<div id=\"close_button\">X</div>\n"
"		<div id=\"tools\">\n"
"			<div id=\"error_button\" class=\"button error\">On</div>\n"
"			<div id=\"warning_button\" class=\"button warning\">On</div>\n"
"			<div id=\"info_button\" class=\"button info\">Off</div>\n"
"			<div id=\"debug_button\" class=\"button debug last\">On</div>\n"
"		</div>\n"
"		<div style=\"width: 100dp;\">Event Log</div>\n"
"	</handle>\n"
"</h1>\n"
"<div id=\"content\">\n"
"	No messages in log.\n"
"</div>\n"
"<handle id=\"size_handle\" size_target=\"#document\" />";
