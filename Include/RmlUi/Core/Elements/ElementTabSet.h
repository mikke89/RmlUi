#pragma once

#include "../Element.h"
#include "../Header.h"

namespace Rml {

/**
    A tabulated set of panels.
 */

class RMLUICORE_API ElementTabSet : public Element {
public:
	RMLUI_RTTI_DefineWithParent(ElementTabSet, Element)

	ElementTabSet(const String& tag);
	~ElementTabSet();

	/// Sets the specified tab index's tab title RML.
	/// @param[in] tab_index The tab index to set. If it doesn't already exist, it will be created.
	/// @param[in] rml The RML to set on the tab title.
	void SetTab(int tab_index, const String& rml);
	/// Sets the specified tab index's tab panel RML.
	/// @param[in] tab_index The tab index to set. If it doesn't already exist, it will be created.
	/// @param[in] rml The RML to set on the tab panel.
	void SetPanel(int tab_index, const String& rml);

	/// Set the specified tab index's title element.
	/// @param[in] tab_index The tab index to set. If it doesn't already exist, it will be created.
	/// @param[in] element The root of the element tree to set as the tab title.
	void SetTab(int tab_index, ElementPtr element);
	/// Set the specified tab index's body element.
	/// @param[in] tab_index The tab index to set. If it doesn't already exist, it will be created.
	/// @param[in] element The root of the element tree to set as the window.
	void SetPanel(int tab_index, ElementPtr element);

	/// Remove one of the tab set's panels and its corresponding tab.
	/// @param[in] tab_index The tab index to remove. If no tab matches this index, nothing will be removed.
	void RemoveTab(int tab_index);

	/// Retrieve the number of tabs in the tabset.
	/// @return The number of tabs.
	int GetNumTabs();

	/// Sets the currently active (visible) tab index.
	/// @param[in] tab_index Index of the tab to display.
	void SetActiveTab(int tab_index);

	/// Get the current active tab index.
	/// @return The index of the active tab.
	int GetActiveTab() const;

	/// Capture clicks on our tabs.
	void ProcessDefaultAction(Event& event) override;

protected:
	// Catch child add so we can correctly set up its properties.
	void OnChildAdd(Element* child) override;

private:
	Element* GetChildByTag(const String& tag);

	int active_tab;
};

} // namespace Rml
