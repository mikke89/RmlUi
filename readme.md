# libRocket - The HTML/CSS User Interface Library

Documentation at https://barotto.github.io/libRocketDoc/

Original website at http://librocket.com


## Fork features

This fork contains some additional features over the [original libRocket branch](https://github.com/libRocket/libRocket), briefly documented here.



## Density-independent pixel (dp)

The `dp` unit behaves like `px` except that its size can be set globally to scale relative to pixels. This makes it easy to achieve a scalable user interface. Set the ratio globally on the context by calling:

```C++
float dp_ratio = 1.5f;
context->SetDensityIndependentPixelRatio(dp_ratio);
```

Usage example in RCSS:
```CSS
div#header 
{
	width: 800dp;
	height: 50dp;
	font-size: 20dp;
}
```


### Transform property

Based on the work of @shoemark, with additional fixes.

Use `perspective`, `perspective-origin`, `transform` and `transform-origin` in RCSS, roughly equivalent to their respective CSS properties.

```CSS
perspective: 1000px;
perspective-origin: 20px 50%;
transform: rotateX(10) skew(-10, 15) translateZ(100dp);
transform-origin: left top 0;
```

All transform properties and their argument types are as follows:
```
perspective,  length1
matrix,       abs_numbers6
matrix3d,     abs_numbers16
translateX,   length1
translateY,   length1
translateZ,   length1
translate,    length2
translate3d,  length3
scaleX,       number1
scaleY,       number1
scaleZ,       number1
scale,        number2
scale,        number1
scale3d,      number3
rotateX,      angle1
rotateY,      angle1
rotateZ,      angle1
rotate,       angle1
rotate3d,     length3angle1
skewX,        angle1
skewY,        angle1
skew,         angle2
```

Angles are in degrees by default.


### Pointer events

Set the element property to disregard mouse input events on this and descending elements.
```CSS
pointer-events: none;
```
Default is `auto`.


### Image-color property

Non-standard RCSS property which multiplies a color with images in `<img>` tags and image decorators. Useful for `:hover`-events and for applying transparency.
```CSS
image-color: rgba(255, 160, 160, 200);
icon-decorator: image;
icon-image: background.png 34px 0px 66px 28px;
```


### Inline-block

Unlike the original branch, elements with
```CSS
display: inline-block;
```
will shrink to the width of their content, like in CSS.



### Border shorthand

Enables the `border` property shorthand.
```CSS
border: 4px #e99;
```


### Various changes

The slider on the `input.range` element can be dragged from anywhere in the element. Additionally, the `:checked` pseudo class can be used to style the selected item in drop-down lists.



# libRocket

libRocket is the C++ user interface package based on the HTML and CSS standards. It is 
designed as a complete solution for any project's interface needs.

libRocket uses the time-tested open standards XHTML1.0 and CSS2.0 (while borrowing features from 
HTML5 and CSS3), and extends them with features suited towards real-time applications. Because of 
this, you don't have to learn a whole new proprietary technology like other libraries in this space.

## Features

- Cross platform architecture: Windows, Mac, Linux, iPhone, etc.
- Dynamic layout system.
- Efficient application-wide styling, with a custom-built templating engine.
- Fully featured control set: buttons, sliders, drop-downs, etc.
- Runtime visual debugging suite.
- Easily integrated and extensible with Python or Lua scripting.

## Extensible
- Abstracted interfaces for plugging in to any game engine.
- Decorator engine allowing custom application-specific effects that can be applied to any element.
- Generic event system that binds seamlessly into existing projects.

## License (MIT)
 
 Copyright (c) 2008-2014 CodePoint Ltd, Shift Technology Ltd, and contributors
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
  
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
