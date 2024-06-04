/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2019-2024 The RmlUi Team, and contributors
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

#ifndef DEMOWINDOW_H
#define DEMOWINDOW_H

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/EventListener.h>

struct TweeningParameters {
	Rml::Tween::Type type = Rml::Tween::Linear;
	Rml::Tween::Direction direction = Rml::Tween::Out;
	float duration = 0.5f;
};

class DemoWindow : public Rml::EventListener {
public:
	bool Initialize(const Rml::String& title, Rml::Context* context);
	void Shutdown();

	void Update();

	void ProcessEvent(Rml::Event& event) override;

	Rml::ElementDocument* GetDocument();

	void SubmitForm(Rml::String in_submit_message);
	void SetSandboxStylesheet(const Rml::String& string);
	void SetSandboxBody(const Rml::String& string);

	TweeningParameters GetTweeningParameters() const;
	void SetTweeningParameters(TweeningParameters tweening_parameters);

private:
	Rml::ElementDocument* document = nullptr;
	Rml::ElementDocument* iframe = nullptr;
	Rml::Element* gauge = nullptr;
	Rml::Element* progress_horizontal = nullptr;
	Rml::SharedPtr<Rml::StyleSheetContainer> rml_basic_style_sheet;

	bool submitting = false;
	double submitting_start_time = 0;
	Rml::String submit_message;

	TweeningParameters tweening_parameters;
};

#endif
