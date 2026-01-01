#pragma once

namespace Rml {

class TextInputContext;

/**
    Handler of changes to text editable areas. Implement this interface to pick up these events, and pass
    the custom implementation to a context (via its constructor) or globally (via SetTextInputHandler).

    Be aware that backends might provide their custom handler to, for example, handle the IME.

    The lifetime of a text input context is ended with the call of OnDestroy().

    @see Rml::TextInputContext
    @see Rml::SetTextInputHandler()
 */
class RMLUICORE_API TextInputHandler : public NonCopyMoveable {
public:
	virtual ~TextInputHandler() {}

	/// Called when a text input area is activated (e.g., focused).
	/// @param[in] input_context The input context to be activated.
	virtual void OnActivate(TextInputContext* /*input_context*/) {}

	/// Called when a text input area is deactivated (e.g., by losing focus).
	/// @param[in] input_context The input context to be deactivated.
	virtual void OnDeactivate(TextInputContext* /*input_context*/) {}

	/// Invoked when the context of a text input area is destroyed (e.g., when the element is being removed).
	/// @param[in] input_context The input context to be destroyed.
	virtual void OnDestroy(TextInputContext* /*input_context*/) {}
};

} // namespace Rml
