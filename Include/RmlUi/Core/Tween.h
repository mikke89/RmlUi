#pragma once

#include "Header.h"
#include "Types.h"

namespace Rml {

class RMLUICORE_API Tween {
public:
	enum Type { None, Back, Bounce, Circular, Cubic, Elastic, Exponential, Linear, Quadratic, Quartic, Quintic, Sine, Callback, Count };
	enum Direction { In = 1, Out = 2, InOut = 3 };
	using CallbackFnc = float (*)(float);

	Tween(Type type = Linear, Direction direction = Out);
	Tween(Type type_in, Type type_out);
	Tween(CallbackFnc callback, Direction direction = In);

	// Evaluate the Tweening function at point t in [0, 1].
	float operator()(float t) const;

	// Reverse direction of the tweening function.
	void reverse();

	bool operator==(const Tween& other) const;
	bool operator!=(const Tween& other) const;

	String to_string() const;

private:
	float tween(Type type, float t) const;
	float in(float t) const;
	float out(float t) const;
	float in_out(float t) const;

	Type type_in = None;
	Type type_out = None;
	CallbackFnc callback = nullptr;
};

} // namespace Rml
