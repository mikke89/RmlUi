#pragma once

#include "ID.h"
#include "Tween.h"
#include "Types.h"

namespace Rml {

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
	Vector<Transition> transitions;

	TransitionList() {}
	TransitionList(bool none, bool all, Vector<Transition> transitions) : none(none), all(all), transitions(std::move(transitions)) {}
};

inline bool operator==(const Animation& a, const Animation& b)
{
	return a.duration == b.duration && a.tween == b.tween && a.delay == b.delay && a.alternate == b.alternate && a.paused == b.paused &&
		a.num_iterations == b.num_iterations && a.name == b.name;
}
inline bool operator!=(const Animation& a, const Animation& b)
{
	return !(a == b);
}
inline bool operator==(const Transition& a, const Transition& b)
{
	return a.id == b.id && a.tween == b.tween && a.duration == b.duration && a.delay == b.delay &&
		a.reverse_adjustment_factor == b.reverse_adjustment_factor;
}
inline bool operator!=(const Transition& a, const Transition& b)
{
	return !(a == b);
}
inline bool operator==(const TransitionList& a, const TransitionList& b)
{
	return a.none == b.none && a.all == b.all && a.transitions == b.transitions;
}
inline bool operator!=(const TransitionList& a, const TransitionList& b)
{
	return !(a == b);
}

} // namespace Rml
