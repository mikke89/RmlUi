/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

static const char* menu_rcss = R"RCSS(
body
{
	width: 100%;
	height: 32dp;
	position: absolute;
	z-index: 1000000;
	background: #888;
	font-family: rmlui-debugger-font;
	font-size: 14dp;
	color: black;
}
div
{
	display: block;
}
div#button-group
{
	margin-top: 3dp;
}
button
{
	border-width: 1px;
	border-color: #666;
	background: #ddd;
	margin-left: 6dp;
	display: inline-block;
	width: 130dp;
	line-height: 24dp;
	text-align: center;
}
button:hover
{
	background: #eee;
}
button:active
{
	background: #fff;
}
div#version-info
{
	padding: 0px;
	margin-top: 0px;
	font-size: 20dp;
	float: right;
	margin-right: 20dp;
	width: 200dp;
	text-align: right;
	color: white;
}
span#version-number
{
	font-size: 15dp;
}
)RCSS";

static const char* menu_rml = R"RML(
<div id="version-info">RmlUi <span id="version-number"></span></div>
<div id="button-group">
	<button id="event-log-button">Event Log</button>
	<button id="debug-info-button">Element Info</button>
	<button id="outlines-button">Outlines</button>
</div>
)RML";
