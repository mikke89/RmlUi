#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Plugin.h>
#include <algorithm>
#include <doctest.h>

using namespace Rml;

namespace {

class DataModelTrackingPlugin : public Plugin {
public:
	int GetEventClasses() override { return EVT_DATA_MODEL; }

	void OnDataModelCreate(Context* context, const String& name) override { created.push_back({context, name}); }

	void OnDataModelDestroy(Context* context, const String& name) override
	{
		destroyed.push_back({context, name});
		// The data model should still be resolvable at this point.
		model_resolvable_on_destroy = static_cast<bool>(context->GetDataModel(name));
	}

	struct Entry {
		Context* context;
		String name;
	};

	Vector<Entry> created;
	Vector<Entry> destroyed;
	bool model_resolvable_on_destroy = false;
};

} // namespace

TEST_CASE("plugin.data_model_notifications")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	DataModelTrackingPlugin plugin;
	Rml::RegisterPlugin(&plugin);

	// Creation notification.
	{
		DataModelConstructor constructor = context->CreateDataModel("plugin_test_model");
		REQUIRE(constructor);
		REQUIRE(plugin.created.size() == 1);
		CHECK(plugin.created[0].context == context);
		CHECK(plugin.created[0].name == "plugin_test_model");
		CHECK(plugin.destroyed.empty());
	}

	// Duplicate create should not fire another notification.
	{
		TestsShell::SetNumExpectedWarnings(1);
		DataModelConstructor duplicate = context->CreateDataModel("plugin_test_model");
		TestsShell::SetNumExpectedWarnings(0);
		CHECK(!duplicate);
		CHECK(plugin.created.size() == 1);
	}

	// Explicit removal fires destroy notification, and the model is still resolvable during the callback.
	{
		const bool removed = context->RemoveDataModel("plugin_test_model");
		CHECK(removed);
		REQUIRE(plugin.destroyed.size() == 1);
		CHECK(plugin.destroyed[0].context == context);
		CHECK(plugin.destroyed[0].name == "plugin_test_model");
		CHECK(plugin.model_resolvable_on_destroy);
	}

	// Removing a nonexistent data model should not fire a notification.
	{
		const bool removed = context->RemoveDataModel("plugin_test_model");
		CHECK_FALSE(removed);
		CHECK(plugin.destroyed.size() == 1);
	}

	// Context destruction should fire destroy notifications for any remaining data models.
	{
		DataModelConstructor a = context->CreateDataModel("plugin_model_a");
		DataModelConstructor b = context->CreateDataModel("plugin_model_b");
		REQUIRE(a);
		REQUIRE(b);
		CHECK(plugin.created.size() == 3);

		const size_t destroyed_before = plugin.destroyed.size();

		TestsShell::ShutdownShell();

		CHECK(plugin.destroyed.size() == destroyed_before + 2);
		Vector<String> names_destroyed_on_shutdown;
		for (size_t i = destroyed_before; i < plugin.destroyed.size(); ++i)
			names_destroyed_on_shutdown.push_back(plugin.destroyed[i].name);
		CHECK(
			std::find(names_destroyed_on_shutdown.begin(), names_destroyed_on_shutdown.end(), "plugin_model_a") != names_destroyed_on_shutdown.end());
		CHECK(
			std::find(names_destroyed_on_shutdown.begin(), names_destroyed_on_shutdown.end(), "plugin_model_b") != names_destroyed_on_shutdown.end());
	}

	Rml::UnregisterPlugin(&plugin);
}
