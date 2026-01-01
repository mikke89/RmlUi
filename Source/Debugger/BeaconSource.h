static const char* beacon_rcss = R"RCSS(
body
{
	position: absolute;
	top: 5px;
	right: 33dp;
	z-index: 1000000;
	width: 20px;
	font-family: rmlui-debugger-font;
	font-size: 12dp;
	color: black;
	visibility: hidden;
}
button
{
	display: block;
	width: 18dp;
	height: 18dp;
	text-align: center;
	border-width: 1px;
	font-weight: bold;
}
)RCSS";

static const char* beacon_rml = R"RML(
<button class="error">!</button>
)RML";
