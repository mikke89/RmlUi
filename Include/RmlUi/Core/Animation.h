/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2018 Michael Ragazzon
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

#ifndef RMLUICOREANIMATION_H
#define RMLUICOREANIMATION_H

#include "Types.h"
#include "Tween.h"
#include "ID.h"

namespace Rml {
namespace Core {

/* Data parsed from the 'animation' property. */
struct Animation {
	float duration = 0.0f;
	Tween tween;
	float delay = 0.0f;
	bool alternate = false;
	bool paused = false;
	int num_iterations = 1;
	String name;
};

/* Data parsed from the 'transition' property. */
struct Transition {
	PropertyId id = PropertyId::Invalid;
	Tween tween;
	float duration = 0.0f;
	float delay = 0.0f;
	float reverse_adjustment_factor = 0.0f;
};

struct TransitionList {
	bool none = true;
	bool all = false;
	std::vector<Transition> transitions;

	TransitionList() {}
	TransitionList(bool none, bool all, std::vector<Transition> transitions) :
		none(none), all(all), transitions(transitions) {}
};

inline bool operator==(const Animation& a, const Animation& b) { return a.duration == b.duration && a.tween == b.tween && a.delay == b.delay && a.alternate == b.alternate && a.paused == b.paused && a.num_iterations == b.num_iterations && a.name == b.name; }
inline bool operator!=(const Animation& a, const Animation& b) { return !(a == b); }
inline bool operator==(const Transition& a, const Transition& b) { return a.id == b.id && a.tween == b.tween && a.duration == b.duration && a.delay == b.delay && a.reverse_adjustment_factor == b.reverse_adjustment_factor; }
inline bool operator!=(const Transition& a, const Transition& b) { return !(a == b); }
inline bool operator==(const TransitionList& a, const TransitionList& b) { return a.none == b.none && a.all == b.all && a.transitions == b.transitions; }
inline bool operator!=(const TransitionList& a, const TransitionList& b) { return !(a == b); }

}
}

#endif