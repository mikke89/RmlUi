static const char* info_rcss = R"RCSS(
body
{
	width: 320dp;
	min-width: 320dp;
	margin-top: 42dp;
	margin-right: 20dp;
	margin-left: auto;
}
div#content
{
	height: auto;
	min-height: 200dp;
	max-height: 650dp;
}
div#content div h2
{
	padding-left: 5dp;
}
div#content div div
{
	font-size: 12dp;
	padding-left: 10dp;
}
div#content .name
{
	color: #610;
}
div#position p:hover,
div#ancestors p:hover,
div#children p:hover
{
	background-color: #ddd;
}
scrollbarvertical
{
	scrollbar-margin: 0px;
}
h3.strong
{
	margin-top: 1.0em;
	padding: 2dp;
	color: #900;
	background-color: #eee;
}
#pseudo pseudo
{
	padding: 0 8dp 0 3dp;
	background-color: #ddd;
	border: 2px #aaa;
	display: inline-block;
}
#pseudo pseudo.active
{
	border-color: #8af;
	background-color: #eef;
}
div.header_button
{
	font-size: 0.9em;
	margin-left: 3dp;
	z-index: 1;
	float: right;
	width: 18dp;
	color: #999;
	background-color: #666;
	border-width: 1px;
	border-color: #666;
	text-align: center;
}
div.header_button.active
{
	border-color: #ccc;
	color: #fff;
}
div.header_button:hover
{
	background-color: #555;
}
div.header_button:active
{
	background-color: #444;
}
div#title-content {
	width: 220dp;
}
div#title-content em {
	font-size: 14dp;
}
p.non_dom {
	font-style: italic;
}
.break-all {
	word-break: break-all;
}
)RCSS";

static const char* info_rml = R"RML(
<h1>
	<handle id="position_handle" move_target="#document"/>
	<div id="close_button">X</div>
	<div id="update_source" class="header_button active">U</div>
	<div id="show_source" class="header_button active">D</div>
	<div id="enable_element_select" class="header_button active">*</div>
	<div id="title-content">Element Information</div>
</h1>
<div id="content">
	<div id="pseudo">
		<pseudo name="hover" class="active">:hover</pseudo>
		<pseudo name="active">:active</pseudo>
		<pseudo name="focus">:focus</pseudo>
		<pseudo name="checked">:checked</pseudo>
		<span id="extra"></span>
	</div>
	<div id="attributes">
		<h2>Attributes</h2>
		<div id="attributes-content">
		</div>
	</div>
	<div id="properties">
		<h2>Properties</h2>
		<div id="properties-content">
		</div>
	</div>
	<div id="events">
		<h2>Events</h2>
		<div id="events-content">
		</div>
	</div>
	<div id="position">
		<h2>Position</h2>
		<div id="position-content">
		</div>
	</div>
	<div id="ancestors">
		<h2>Ancestors</h2>
		<div id="ancestors-content">
		</div>
	</div>
	<div id="children">
		<h2>Children</h2>
		<div id="children-content">
		</div>
	</div>
</div>
)RML";
