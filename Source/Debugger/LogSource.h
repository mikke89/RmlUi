static const char* log_rcss = R"RCSS(body
{
	width: 400dp;
	height: 300dp;
	min-width: 250dp;
	min-height: 150dp;
	top: 42dp;
	left: 20dp;
}
div#tools
{
	float: right;
	width: 200dp;
}
div.log-entry
{
	margin: 3dp 2dp;
}
div.log-entry div.icon
{
	float: left;
	display: block;
	width: 18dp;
	height: 18dp;
	text-align: center;
	border-width: 1px;
	margin-right: 5dp;
}
div.button
{
	display: inline-block;
	width: 32dp;
	font-size: 13dp;
	line-height: 20dp;
	text-align: center;
	border-width: 1px;
	margin-right: 3dp;
}
div.button.clear
{
	border-color: #666;
	background-color: #aaa;
	color: #111;
}
div.button:hover
{
	border-color: #ddd;
}
div.button:active
{
	border-color: #fff;
}
div.button.last
{
	margin-right: 0px;
}
div.log-entry p.message
{
	display: block;
	white-space: pre-wrap;
	margin-left: 20dp;
}
)RCSS";

static const char* log_rml = R"RML(
<h1>
	<handle id="position_handle" move_target="#document"/>
	<div id="close_button">X</div>
	<div id="tools">
		<div id="clear_button" class="button clear" style="width: 45dp;">Clear</div>
		<div id="error_button" class="button error">On</div>
		<div id="warning_button" class="button warning">On</div>
		<div id="info_button" class="button info">Off</div>
		<div id="debug_button" class="button debug last">On</div>
	</div>
	<div style="width: 100dp;">Event Log</div>
</h1>
<div id="content">
	No messages in log.
</div>
<handle id="size_handle" size_target="#document" />
)RML";
