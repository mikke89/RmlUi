static const char* data_models_rcss = R"RCSS(
body {
	width: 320dp;
	min-width: 320dp;
	margin-top: 52dp;
	margin-right: 30dp;
	margin-left: auto;
}
div#content {
	height: auto;
	min-height: 200dp;
	max-height: 650dp;
}
div#content div h2 {
	padding-left: 5dp;
}
div#content div div
{
	font-size: 12dp;
	padding-left: 10dp;
}
div#content .name {
	color: #610;
}
scrollbarvertical {
	scrollbar-margin: 0px;
}
)RCSS";

static const char* data_models_rml = R"RML(
<h1>
	<handle id="position_handle" move_target="#document"/>
	<div id="close_button">X</div>
	<div id="title-content">Data Models</div>
</h1>
<div id="content">
	<div id="models">
		<h2>Data models</h2>
		<div id="models-content"></div>
	</div>
	<div id="variables">
		<h2>Data variables</h2>
		<div id="variables-content"></div>
	</div>
</div>
)RML";
