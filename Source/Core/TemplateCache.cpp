#include "TemplateCache.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "StreamFile.h"
#include "Template.h"

namespace Rml {

static TemplateCache* instance = nullptr;

TemplateCache::TemplateCache()
{
	RMLUI_ASSERT(instance == nullptr);
	instance = this;
}

TemplateCache::~TemplateCache()
{
	for (Templates::iterator itr = instance->templates.begin(); itr != instance->templates.end(); ++itr)
	{
		delete (*itr).second;
	}

	instance = nullptr;
}

bool TemplateCache::Initialise()
{
	new TemplateCache();

	return true;
}

void TemplateCache::Shutdown()
{
	delete instance;
}

Template* TemplateCache::LoadTemplate(const String& name)
{
	// Check if the template is already loaded
	Templates::iterator itr = instance->templates.find(name);
	if (itr != instance->templates.end())
		return (*itr).second;

	// Nope, we better load it
	Template* new_template = nullptr;
	auto stream = MakeUnique<StreamFile>();
	if (stream->Open(name))
	{
		new_template = new Template();
		if (!new_template->Load(stream.get()))
		{
			Log::Message(Log::LT_ERROR, "Failed to load template %s.", name.c_str());
			delete new_template;
			new_template = nullptr;
		}
		else if (new_template->GetName().empty())
		{
			Log::Message(Log::LT_ERROR, "Failed to load template %s, template is missing its name.", name.c_str());
			delete new_template;
			new_template = nullptr;
		}
		else
		{
			instance->templates[name] = new_template;
			instance->template_ids[new_template->GetName()] = new_template;
		}
	}
	else
	{
		Log::Message(Log::LT_ERROR, "Failed to open template file %s.", name.c_str());
	}

	return new_template;
}

Template* TemplateCache::GetTemplate(const String& name)
{
	// Check if the template is already loaded
	Templates::iterator itr = instance->template_ids.find(name);
	if (itr != instance->template_ids.end())
		return (*itr).second;

	return nullptr;
}

void TemplateCache::Clear()
{
	for (Templates::iterator i = instance->templates.begin(); i != instance->templates.end(); ++i)
		delete (*i).second;

	instance->templates.clear();
	instance->template_ids.clear();
}

} // namespace Rml
