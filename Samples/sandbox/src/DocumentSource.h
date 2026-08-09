#pragma once
#include "RmlUi/Config/Config.h"

static Rml::String external_document_source = R"RML(


<rml>
<head>
<title>Effects Sample</title>
<meta name="source" content="basic/effects/data/effects.rml" />

<style path="assets/rml.rcss" inline="0">
/*
* Default styles for all the basic elements.
*/

div
{
	display: block;
}

p
{
	display: block;
}

h1
{
	display: block;
}

em
{
	font-style: italic;
}

strong
{
	font-weight: bold;
}

select
{
	text-align: left;
}

tabset tabs
{
	display: block;
}

table {
	box-sizing: border-box;
	display: table;
}
tr {
	box-sizing: border-box;
	display: table-row;
}
td {
	box-sizing: border-box;
	display: table-cell;
}
col {
	box-sizing: border-box;
	display: table-column;
}
colgroup {
	display: table-column-group;
}
thead, tbody, tfoot {
	display: table-row-group;
}

</style>

<style path="basic/effects/data/effects_style.rcss" inline="0">
body {
	font-family: LatoLatin;
	font-weight: normal;
	font-style: normal;
	font-size: 15dp;

	left: 80dp;
	right: 80dp;
	top: 50dp;
	bottom: 50dp;
	min-width: 400dp;
	min-height: 60dp;
	background-color: #a4b6b7;
	border: 3dp #d3e9ea;
	border-radius: 30dp 8dp;
	padding-top: 75dp;
	overflow: hidden auto;
}
h1 {
	margin: 0em 0 0.7em;
	font-size: 22dp;
	font-effect: glow(2dp #354c2e);
	color: #fed;
	padding: 1em 0 1em 40dp;
	border-bottom: 3dp #d3e9ea;
	background-color: #619158;
	z-index: 1;
	position: fixed;
	top: 0;
	right: 0;
	left: 0;
}
handle.size {
	position: fixed;
	z-index: 100;
	bottom: 0;
	right: 0;
	width: 18dp;
	height: 18dp;
	background-color: #d3e9ea66;
	border-top-left-radius: 5dp;
	cursor: resize;
}
handle.size:hover, handle.size:active {
	background-color: #d3e9ea;
}

#menu_button {
	position: fixed;
	z-index: 2;
	top: 15dp;
	right: 25dp;
	box-sizing: border-box;
	width: 36dp;
	height: 36dp;

	background: #fffc;
	border: 2dp #555a;
	border-radius: 5dp;
	color: #333;
	padding-top: 5dp;
	text-align: center;
	line-height: 7dp;
	font-size: 28dp;
	cursor: pointer;
}
#menu_button.open {
	background-color: #4bdc;
	border-color: transparent;
	border-top-right-radius: 15dp;
}
#menu_button:hover { background: #bcbc; }
#menu_button:active { background: #abac; }
#menu_button.open:hover { background: #5cec; }
#menu_button.open:active { background: #4bdc; }

#menu {
	position: fixed;
	z-index: 1;
	top: 15dp;
	right: 25dp;
	box-sizing: border-box;
	width: 400dp;
	height: 480dp;
	overflow: auto;
	overscroll-behavior: contain;

	background: #fffc;
	border: 2dp #555a;
	border-radius: 15dp;
	color: #222;
	padding: 20dp 40dp 0dp;
}
#menu table {
	margin-bottom: 10dp;
}
#menu td {
	vertical-align: middle;
	height: 36dp;
	line-height: 16dp;
}
#menu td:nth-child(3) {
	text-align: right;
	white-space: nowrap;
	font-size: 0.92em;
}

#submenu {
	display: flex;
	text-align: center;
	margin-bottom: 20dp;
	justify-content: space-around;
}
#submenu div {
	flex: 0.35;
	height: 25dp;
	cursor: pointer;
	border-bottom: 1dp #aaa;
	box-sizing: border-box;
}
#submenu div:hover {
	color: #000;
	border-bottom-color: #555;
}
#submenu div.selected {
	font-weight: bold;
	color: #37a;
	border-bottom-color: #4bd;
	border-bottom: 2dp #37a;
}

scrollbarvertical {
	z-index: 100;
	margin-top: 75dp;
	margin-bottom: 20dp;
	margin-right: 0dp;
	width: 0dp;
}
scrollbarvertical sliderbar {
	margin-left: -14dp;
	width: 12dp;
	min-height: 25dp;
	background: #d3e9ea66;
	border-radius: 4dp;
}
scrollbarvertical sliderbar:hover, scrollbarvertical sliderbar:active {
	background: #d3e9eaaa;
}

input.range {
	width: 100%;
	height: 15dp;
	transition: opacity 0.2s cubic-out;
}
input.range:disabled { opacity: 0.3; }
input.range slidertrack {
	background-color: #fff;
}
input.range sliderbar {
	width: 15dp;
	border-radius: 3dp;
}
input.range:hover sliderbar { background-color: #333; }
input.range sliderbar:active { background-color: #111; }
input.range sliderbar, input.range sliderbar:disabled { background-color: #555; }
input.range sliderarrowdec, input.range sliderarrowinc {
	width: 12dp;
	height: 15dp;
}
input.range sliderarrowdec { border-radius: 2dp 0 0 2dp; }
input.range sliderarrowinc { border-radius: 0 2dp 2dp 0; }
input.range sliderarrowdec:hover,    input.range sliderarrowinc:hover    { background-color: #ddd; }
input.range sliderarrowdec:active,   input.range sliderarrowinc:active   { background-color: #eee; }
input.range sliderarrowdec,          input.range sliderarrowinc,
input.range sliderarrowdec:disabled, input.range sliderarrowinc:disabled { background-color: #ccc; }

input.radio, input.checkbox {
	width: 15dp;
	height: 15dp;
	border: 1dp #ccc;
	background: #fff;
	border-radius: 2dp;
}
input.radio {
	border-radius: 8dp;
}
input.radio:hover,   input.checkbox:hover   { background-color: #ff3; }
input.radio:active,  input.checkbox:active  { background-color: #ddd; }
input.radio:checked, input.checkbox:checked { background-color: #555; }

button {
	border: 1dp #555;
	border-radius: 7dp;
	padding: 6dp 13dp;
	background-color: #fffa;
	cursor: pointer;
}
button:hover { background-color: #ccca; }
button:active { background-color: #bbba; }

</style>

<style path="basic/effects/data/effects.rml" inline="1">

@spritesheet effects-sheet
{
	src: /assets/invader.tga;
	icon-invader: 179px 152px 51px 39px;
}

.filter {
	transform-origin: 50% 0;
}

.box {
	color: black;
	font-size: 18dp;
	width: 280dp;
	height: 70dp;
	background: #fff8;
	border: 2dp #def6f7;
	margin: 10dp auto;
	padding: 15dp;
	border-radius: 30dp 8dp;
	box-sizing: border-box;
	position: relative;
}
.box img, .box .placeholder {
	float: left;
	margin-right: 8dp;
}
.box .placeholder {
	width: 51dp;
	height: 39dp;
}
.box .label {
	color: #bbba;
	position: absolute;
	font-size: 0.75em;
	bottom: 0.1em;
	right: 1.5em;
}

.box.window {
	position: absolute;
	left: 30dp;
	margin: 0;
}
.box.window handle {
	position: absolute;
	top: 0; right: 0; bottom: 0; left: 0;
	display: block;
	cursor: move;
}
.box.big {
	width: 500dp;
	height: 260dp;
	max-width: 100%;
	border-color: #333;
}

.transform, .filter.transform_all > .box { transform: rotate3d(0.2, 0.4, 0.1, 15deg); }

.mask {
	decorator: horizontal-gradient(#f00 #ff0);
	mask-image: image(icon-invader scale-none 15px 50%), horizontal-gradient(#0000 #000f);
}
.shader { decorator: shader("creation"); }
.gradient { decorator: linear-gradient(110deg, #fff3, #fff 10%, #c33 250dp, #3c3, #33c, #000 90%, #0003) border-box; }

.brightness { filter: brightness(0.5); }
.contrast { filter: contrast(0.5); }
.sepia { filter: sepia(80%); }
.grayscale { filter: grayscale(0.9); }

.saturate { filter: saturate(200%); }
.hue_rotate { filter: hue-rotate(260deg); }
.invert { filter: invert(100%); }
.opacity_low { filter: opacity(0.2); }

.blur { filter: blur(10px); }
.back_blur { backdrop-filter: blur(5px);  }

.dropshadow { filter: drop-shadow(#f33f 30px 20px 5px); }

.boxshadow_blur {
	box-shadow:
		#f00f  40px  30px 25px 0px,
		#00ff -40px -30px 45px 0px,
		#0f08 -60px  70px 60px 0px,
		#333a  0px  0px 30px 15px inset
		;
	margin-top: 100px;
	margin-bottom: 100px;
}
.boxshadow_trail {
	box-shadow:
		#f66 30dp 30dp 0 0,
		#c88 60dp 60dp 0 0,
		#baa 90dp 90dp 0 0,
		#ffac 0 0 .8em 8dp inset
		;
	margin-bottom: 100px;
	filter: opacity(1); /* Tests filter clipping behavior when element has ink overflow due to box-shadow. */
}
.boxshadow_inset { box-shadow: #f4fd 10px 5px 5px 3px inset; }

@keyframes animate-filter {
	from { filter: drop-shadow(#f00) opacity(1.0) sepia(1.0); }
	to   { filter: drop-shadow(#000 30px 20px 5px) opacity(0.2) sepia(0.2); }
}
.animate {
	animation: animate-filter 1.5s cubic-in-out infinite alternate;
}


</style>
</head>
<body data-model="effects" data-style-perspective="perspective &gt;= 3000 ? 'none' : perspective + 'dp'" data-style-perspective-origin-x="perspective_origin_x + '%'" data-style-perspective-origin-y="perspective_origin_y + '%'" style="perspective: none; perspective-origin-x: 50%; perspective-origin-y: 50%; position: absolute; visibility: visible;"><handle move_target="#document" style="drag: drag;"><h1>Effects Sample</h1>
</handle>
<handle class="size" size_target="#document" style="drag: drag;" />
<div data-class-open="show_menu" data-event-click="show_menu = !show_menu" id="menu_button">—<br />
—<br />
—</div>
<div data-if="show_menu" id="menu"><div id="submenu"><div data-class-selected="submenu == 'filter'" data-event-click="submenu = 'filter'">Filter</div>
<div data-class-selected="submenu == 'transform'" data-event-click="submenu = 'transform'">Transform</div>
</div>
<table><col style="width: 200%;" />
<col style="width: 300%;" />
<col style="width: 100%;" />
<tbody data-if="submenu == 'filter'"><tr><td>Opacity</td>
<td><input data-value="opacity" max="1" min="0" step="0.01" type="range" value="1" style="tab-index: auto;" />
</td>
<td>1</td>
</tr>
<tr><td>Sepia</td>
<td><input data-value="sepia" max="1" min="0" step="0.01" type="range" value="0" style="tab-index: auto;" />
</td>
<td>0 %</td>
</tr>
<tr><td>Grayscale</td>
<td><input data-value="grayscale" max="1" min="0" step="0.01" type="range" value="0" style="tab-index: auto;" />
</td>
<td>0 %</td>
</tr>
<tr><td>Saturate</td>
<td><input data-value="saturate" max="2" min="0" step="0.01" type="range" value="1" style="tab-index: auto;" />
</td>
<td>100 %</td>
</tr>
<tr><td>Brightness</td>
<td><input data-value="brightness" max="2" min="0" step="0.02" type="range" value="1" style="tab-index: auto;" />
</td>
<td>100 %</td>
</tr>
<tr><td>Contrast</td>
<td><input data-value="contrast" max="2" min="0" step="0.02" type="range" value="1" style="tab-index: auto;" />
</td>
<td>100 %</td>
</tr>
<tr><td>Hue</td>
<td><input data-value="hue_rotate" max="360" min="0" step="1" type="range" value="0" style="tab-index: auto;" />
</td>
<td>0 deg</td>
</tr>
<tr><td>Invert</td>
<td><input data-value="invert" max="1" min="0" step="0.01" type="range" value="0" style="tab-index: auto;" />
</td>
<td>0 %</td>
</tr>
<tr><td>Blur</td>
<td><input data-value="blur" max="50" min="0" step="1" type="range" value="0" style="tab-index: auto;" />
</td>
<td>0 px</td>
</tr>
<tr><td><label for="drop_shadow">Drop shadow</label>
</td>
<td colspan="2"><input data-checked="drop_shadow" id="drop_shadow" type="checkbox" style="tab-index: auto;" />
</td>
</tr>
</tbody>
<tbody data-if="submenu == 'transform'" style="display: none;"><tr><td>Scale</td>
<td><input data-value="scale" max="2.0" min="0.1" step="0.1" type="range" value="1" style="tab-index: auto;" />
</td>
<td>1.0x</td>
</tr>
<tr><td>Rotate X</td>
<td><input data-value="rotate_x" max="90" min="-90" step="5" type="range" value="0" style="tab-index: auto;" />
</td>
<td>0 deg</td>
</tr>
<tr><td>Rotate Y</td>
<td><input data-value="rotate_y" max="90" min="-90" step="5" type="range" value="0" style="tab-index: auto;" />
</td>
<td>0 deg</td>
</tr>
<tr><td>Rotate Z</td>
<td><input data-value="rotate_z" max="90" min="-90" step="5" type="range" value="0" style="tab-index: auto;" />
</td>
<td>0 deg</td>
</tr>
<tr><td>Perspective</td>
<td><input data-value="perspective" max="3000" min="100" step="25" type="range" value="3000" style="tab-index: auto;" />
</td>
<td>none</td>
</tr>
<tr><td>Perspective X</td>
<td><input data-attrif-disabled="perspective &gt;= 3000" data-value="perspective_origin_x" disabled="" max="200" min="-100" step="5" type="range" value="50" style="focus: none; tab-index: auto;" />
</td>
<td>50 %</td>
</tr>
<tr><td>Perspective Y</td>
<td><input data-attrif-disabled="perspective &gt;= 3000" data-value="perspective_origin_y" disabled="" max="200" min="-100" step="5" type="range" value="50" style="focus: none; tab-index: auto;" />
</td>
<td>50 %</td>
</tr>
<tr><td><label for="transform_all">Transform all</label>
</td>
<td colspan="2"><input data-checked="transform_all" id="transform_all" type="checkbox" style="tab-index: auto;" />
</td>
</tr>
</tbody>
</table>
<button data-event-click="reset()">Reset</button>
</div>
<div class="filter" data-class-transform_all="transform_all" data-style-filter="'opacity(' + opacity + ') sepia(' + sepia + ') grayscale(' + grayscale + ') saturate(' + saturate + ') brightness(' + brightness + ') contrast(' + contrast + ') hue-rotate(' + hue_rotate + 'deg) invert(' + invert + ') blur(' + blur + 'px)' + (drop_shadow ? ' drop-shadow(#f11b 10px 10px 8px)' : '')" data-style-transform="'scale(' + scale + ') rotateX(' + rotate_x + 'deg) rotateY(' + rotate_y + 'deg) rotateZ(' + rotate_z + 'deg)'" style="transform: scale(1, 1) rotateX(0deg) rotateY(0deg) rotateZ(0deg); filter: opacity(1) sepia(0) grayscale(0) saturate(1) brightness(1) contrast(1) hue-rotate(0deg) invert(0) blur(0px);"><div class="box boxshadow_blur transform"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box boxshadow_trail"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box boxshadow_inset"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box big shader"><div class="label">&quot;Creation&quot; (Danilo Guanabara)</div>
</div>
<div class="box big gradient"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box mask"><div class="placeholder" />
Hello, do you feel the funk?</div>
<div class="box hue_rotate"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box animate" style="filter: drop-shadow(#ac0000 16.332px 10.888px 2.722px) opacity(0.564) sepia(0.564);"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box saturate"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box invert"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box blur"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box brightness"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box contrast"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box dropshadow"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box grayscale"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box opacity_low"><img sprite="icon-invader" />
Hello, do you feel the funk?</div>
</div>
<div class="box window sepia" style="top: 375dp;"><handle move_target="#parent" style="drag: drag;" />
<img sprite="icon-invader" />
Hello, do you feel the funk?</div>
<div class="box window back_blur" style="top: 475dp;"><handle move_target="#parent" style="drag: drag;" />
<img sprite="icon-invader" />
Hello, do you feel the funk?
</div>
</body>

</rml>

)RML";
