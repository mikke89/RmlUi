/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#include "ScrollController.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"

namespace Rml {

static constexpr float AUTOSCROLL_SPEED_FACTOR = 0.09f;
static constexpr float AUTOSCROLL_DEADZONE = 10.0f; // [dp]

static constexpr float SMOOTHSCROLL_WINDOW_SIZE = 50.f;        // The window where smoothing is applied, as a distance from scroll start and end. [dp]
static constexpr float SMOOTHSCROLL_MAX_VELOCITY = 10'000.f;   // [dp/s]
static constexpr float SMOOTHSCROLL_VELOCITY_CONSTANT = 800.f; // [dp/s]
static constexpr float SMOOTHSCROLL_VELOCITY_SQUARE_FACTOR = 0.05f;

// Clamp the delta time to some reasonable FPS range, to avoid large steps in case of stuttering or freezing.
static constexpr float DELTA_TIME_CLAMP_LOW = 1.f / 500.f; // [s]
static constexpr float DELTA_TIME_CLAMP_HIGH = 1.f / 15.f; // [s]

// Determines the autoscroll velocity based on the distance from the scroll-start mouse position. [px/s]
static Vector2f CalculateAutoscrollVelocity(Vector2f target_delta, float dp_ratio)
{
	target_delta = target_delta / dp_ratio;
	target_delta = {
		Math::Absolute(target_delta.x) < AUTOSCROLL_DEADZONE ? 0.f : target_delta.x,
		Math::Absolute(target_delta.y) < AUTOSCROLL_DEADZONE ? 0.f : target_delta.y,
	};

	// We use a signed square model for the velocity, which seems to work quite well. This is mostly about feeling and tuning.
	return AUTOSCROLL_SPEED_FACTOR * target_delta * Math::Absolute(target_delta);
}

// Determines the smoothscroll velocity based on the distance to the target, and the distance scrolled so far. [px/s]
static Vector2f CalculateSmoothscrollVelocity(Vector2f target_delta, Vector2f scrolled_distance, float dp_ratio)
{
	scrolled_distance = Math::Absolute(scrolled_distance) / dp_ratio;
	target_delta = target_delta / dp_ratio;

	const Vector2f target_delta_abs = Math::Absolute(target_delta);
	Vector2f target_delta_signum = {
		target_delta.x > 0.f ? 1.f : (target_delta.x < 0.f ? -1.f : 0.f),
		target_delta.y > 0.f ? 1.f : (target_delta.y < 0.f ? -1.f : 0.f),
	};

	// The window provides velocity smoothing near the start and end of the scroll.
	const Tween tween(Tween::Exponential, Tween::Out);
	const Vector2f alpha_in = Math::Min(scrolled_distance / SMOOTHSCROLL_WINDOW_SIZE, Vector2f(1.f));
	const Vector2f alpha_out = Math::Min(target_delta_abs / SMOOTHSCROLL_WINDOW_SIZE, Vector2f(1.f));
	const Vector2f smooth_window = {
		0.2f + 0.8f * tween(alpha_in.x) * tween(alpha_out.x),
		0.2f + 0.8f * tween(alpha_in.y) * tween(alpha_out.y),
	};

	const Vector2f velocity_constant = Vector2f(SMOOTHSCROLL_VELOCITY_CONSTANT);
	const Vector2f velocity_square = SMOOTHSCROLL_VELOCITY_SQUARE_FACTOR * target_delta_abs * target_delta_abs;

	// Short scrolls are dominated by the smoothed constant velocity, while the square term is added for quick longer scrolls.
	return dp_ratio * target_delta_signum * smooth_window * Math::Min(velocity_constant + velocity_square, Vector2f(SMOOTHSCROLL_MAX_VELOCITY));
}

void ScrollController::ActivateAutoscroll(Element* in_target, Vector2i start_position)
{
	Reset();
	if (!in_target)
		return;
	target = in_target;
	mode = Mode::Autoscroll;
	autoscroll_start_position = start_position;
	UpdateTime();
}

void ScrollController::ActivateSmoothscroll(Element* in_target, Vector2f delta_distance, ScrollBehavior scroll_behavior)
{
	Reset();
	if (!in_target)
		return;

	target = in_target;

	// Do instant scroll if preferred.
	if (smoothscroll_prefer_instant && scroll_behavior != ScrollBehavior::Smooth)
	{
		PerformScrollOnTarget(delta_distance);
		target = nullptr;
		return;
	}

	mode = Mode::Smoothscroll;
	UpdateTime();
	IncrementSmoothscrollTarget(delta_distance);

	// If the target is scrolled to its edge already, simply cancel the smoothscroll operation.
	if (HasSmoothscrollReachedTarget())
		Reset();
}

bool ScrollController::Update(Vector2i mouse_position, float dp_ratio)
{
	if (mode == Mode::Autoscroll)
		UpdateAutoscroll(mouse_position, dp_ratio);
	else if (mode == Mode::Smoothscroll)
		UpdateSmoothscroll(dp_ratio);

	return mode != Mode::None;
}

void ScrollController::UpdateAutoscroll(Vector2i mouse_position, float dp_ratio)
{
	RMLUI_ASSERT(mode == Mode::Autoscroll && target);

	const float dt = UpdateTime();

	const Vector2f scroll_delta = Vector2f(mouse_position - autoscroll_start_position);
	const Vector2f scroll_velocity = CalculateAutoscrollVelocity(scroll_delta, dp_ratio);

	autoscroll_accumulated_length += scroll_velocity * dt;

	// Only submit the integer part of the scroll length, accumulate and store fractional parts to enable sub-pixel-per-frame scrolling speeds.
	Vector2f scroll_length_integral = autoscroll_accumulated_length;
	autoscroll_accumulated_length.x = Math::DecomposeFractionalIntegral(autoscroll_accumulated_length.x, &scroll_length_integral.x);
	autoscroll_accumulated_length.y = Math::DecomposeFractionalIntegral(autoscroll_accumulated_length.y, &scroll_length_integral.y);

	if (scroll_velocity != Vector2f(0.f))
		autoscroll_moved = true;

	PerformScrollOnTarget(scroll_length_integral);
}

void ScrollController::UpdateSmoothscroll(float dp_ratio)
{
	RMLUI_ASSERT(mode == Mode::Smoothscroll && target);

	const Vector2f target_delta = Vector2f(smoothscroll_target_distance - smoothscroll_scrolled_distance);
	const Vector2f velocity = CalculateSmoothscrollVelocity(target_delta, smoothscroll_scrolled_distance, dp_ratio);

	const float dt = UpdateTime();
	Vector2f scroll_distance = (smoothscroll_speed_factor * velocity * dt).Round();

	for (int i = 0; i < 2; i++)
	{
		// Ensure minimum scroll speed of 1px/frame, and clamp the distance to the target in case of overshooting
		// integration. As opposed to autoscroll, we don't care about fractional speeds here since we want to be fast.
		if (target_delta[i] > 0.f)
			scroll_distance[i] = Math::Min(Math::Max(scroll_distance[i], 1.f), target_delta[i]);
		else if (target_delta[i] < 0.f)
			scroll_distance[i] = Math::Max(Math::Min(scroll_distance[i], -1.f), target_delta[i]);
		else
			scroll_distance[i] = 0.f;
	}

#if 0
	// Useful debugging output for velocity model tuning.
	Log::Message(Log::LT_INFO, "Scroll  y0 %8.2f   y1 %8.2f   v %8.2f   d %8.2f", smoothscroll_scrolled_distance.y, target_delta.y, velocity.y,
		scroll_distance.y);
#endif

	smoothscroll_scrolled_distance += scroll_distance;
	PerformScrollOnTarget(scroll_distance);

	if (HasSmoothscrollReachedTarget())
		Reset();
}

bool ScrollController::HasSmoothscrollReachedTarget() const
{
	constexpr float epsilon = 0.1f;
	return (smoothscroll_target_distance - smoothscroll_scrolled_distance).SquaredMagnitude() < epsilon;
}

void ScrollController::PerformScrollOnTarget(Vector2f delta_distance)
{
	RMLUI_ASSERT(target);
	auto overflow_is_scrollable = [](Style::Overflow overflow) { return overflow == Style::Overflow::Auto || overflow == Style::Overflow::Scroll; };
	auto& computed_values = target->GetComputedValues();

	if (delta_distance.x != 0.f && overflow_is_scrollable(computed_values.overflow_x()))
		target->SetScrollLeft(target->GetScrollLeft() + delta_distance.x);
	if (delta_distance.y != 0.f && overflow_is_scrollable(computed_values.overflow_y()))
		target->SetScrollTop(target->GetScrollTop() + delta_distance.y);
}

void ScrollController::IncrementSmoothscrollTarget(Vector2f delta_distance)
{
	auto OppositeDirection = [](float a, float b) { return (a < 0.f && b > 0.f) || (a > 0.f && b < 0.f); };
	Vector2f delta = smoothscroll_target_distance - smoothscroll_scrolled_distance;

	// Reset movement state if we start scrolling in the opposite direction.
	for (int i = 0; i < 2; i++)
	{
		if (OppositeDirection(delta_distance[i], delta[i]))
		{
			smoothscroll_target_distance[i] = 0.f;
			smoothscroll_scrolled_distance[i] = 0.f;
		}
	}

	// Clamp the delta distance to the scrollable area.
	const Vector2f scroll_offset = {target->GetScrollLeft(), target->GetScrollTop()};
	const Vector2f max_offset = {target->GetScrollWidth() - target->GetClientWidth(), target->GetScrollHeight() - target->GetClientHeight()};

	const Vector2f target_offset = scroll_offset + smoothscroll_target_distance - smoothscroll_scrolled_distance;
	const Vector2f clamped_delta = Math::Clamp(delta_distance + target_offset, Vector2f(0.f), max_offset) - target_offset;

	smoothscroll_target_distance += clamped_delta;
}

void ScrollController::Reset()
{
	mode = Mode::None;
	target = nullptr;

	autoscroll_start_position = Vector2i{};
	autoscroll_accumulated_length = Vector2f{};
	autoscroll_moved = false;

	smoothscroll_target_distance = Vector2f{};
	smoothscroll_scrolled_distance = Vector2f{};
	// Keep smoothscroll configuration parameters.
}

void ScrollController::SetDefaultScrollBehavior(ScrollBehavior scroll_behavior, float speed_factor)
{
	smoothscroll_prefer_instant = (scroll_behavior == ScrollBehavior::Instant);
	smoothscroll_speed_factor = speed_factor;
}

String ScrollController::GetAutoscrollCursor(Vector2i mouse_position, float dp_ratio) const
{
	RMLUI_ASSERT(mode == Mode::Autoscroll);

	const Vector2f scroll_delta = Vector2f(mouse_position - autoscroll_start_position);
	const Vector2f scroll_velocity = CalculateAutoscrollVelocity(scroll_delta, dp_ratio);

	if (scroll_velocity == Vector2f(0.f))
		return "rmlui-scroll-idle";

	String result = "rmlui-scroll";

	if (scroll_velocity.y > 0.f)
		result += "-up";
	else if (scroll_velocity.y < 0.f)
		result += "-down";

	if (scroll_velocity.x > 0.f)
		result += "-right";
	else if (scroll_velocity.x < 0.f)
		result += "-left";

	return result;
}

bool ScrollController::HasAutoscrollMoved() const
{
	return mode == Mode::Autoscroll && autoscroll_moved;
}

float ScrollController::UpdateTime()
{
	const double previous_tick = previous_update_time;
	previous_update_time = GetSystemInterface()->GetElapsedTime();

	const float dt = float(previous_update_time - previous_tick);
	return Math::Clamp(dt, DELTA_TIME_CLAMP_LOW, DELTA_TIME_CLAMP_HIGH);
}

} // namespace Rml
