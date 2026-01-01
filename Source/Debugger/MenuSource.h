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
button.open {
	border-color: #6cf;
	color: #22a;
	background: #cee;
}
button:hover { background: #eee; }
button:active { background: #fff; }
button.open:hover { background: #dff; }
button.open:active { background: #eff; }
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
	<button id="data-models-button">Data Models</button>
</div>
)RML";
