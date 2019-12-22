/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2018 Michael R. P. Ragazzon
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#include <RmlUi/Core.h>
#include <RmlUi/Controls.h>
#include <RmlUi/Debugger.h>
#include <Input.h>
#include <Shell.h>
#include <ShellRenderInterfaceOpenGL.h>


class DemoWindow : public Rml::Core::EventListener
{
public:
	DemoWindow(const Rml::Core::String &title, const Rml::Core::Vector2f &position, Rml::Core::Context *context)
	{
		using namespace Rml::Core;
		document = context->LoadDocument("basic/databinding/data/databinding.rml");
		if (document)
		{
			document->GetElementById("title")->SetInnerRML(title);
			document->SetProperty(PropertyId::Left, Property(position.x, Property::PX));
			document->SetProperty(PropertyId::Top, Property(position.y, Property::PX));

			document->Show();
		}
	}

	void Update() 
	{

	}

	void Shutdown() 
	{
		if (document)
		{
			document->Close();
			document = nullptr;
		}
	}

	void ProcessEvent(Rml::Core::Event& event) override
	{
		using namespace Rml::Core;

		switch (event.GetId())
		{
		case EventId::Keydown:
		{
			Rml::Core::Input::KeyIdentifier key_identifier = (Rml::Core::Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);
			bool ctrl_key = event.GetParameter< bool >("ctrl_key", false);

			if (key_identifier == Rml::Core::Input::KI_ESCAPE)
			{
				Shell::RequestExit();
			}
			else if (key_identifier == Rml::Core::Input::KI_F8)
			{
				Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
			}
		}
		break;

		default:
			break;
		}
	}

	Rml::Core::ElementDocument * GetDocument() {
		return document;
	}


private:
	Rml::Core::ElementDocument *document = nullptr;
};



struct Invader {
	Rml::Core::String name;
	Rml::Core::String sprite;
	Rml::Core::String color;
};


struct MyData {
	Rml::Core::String hello_world = "Hello World!";
	int rating = 99;
	bool good_rating = true;

	Invader invader{ "Delightful invader", "icon-invader", "red" };

	std::vector<Invader> invaders;

	std::vector<int> indices = { 1, 2, 3, 4, 5 };
} my_data;

Rml::Core::DataModelHandle my_model;












namespace Data {
	using namespace Rml::Core;


	template<typename T>
	struct is_valid_scalar {
		static constexpr bool value = std::is_convertible<T, byte>::value
			|| std::is_convertible<T, char>::value
			|| std::is_convertible<T, float>::value
			|| std::is_convertible<T, int>::value
			|| std::is_convertible<T, String>::value
			|| std::is_convertible<T, Vector2f>::value
			|| std::is_convertible<T, Vector3f>::value
			|| std::is_convertible<T, Vector4f>::value
			|| std::is_convertible<T, Colourb>::value
			|| std::is_convertible<T, char*>::value
			|| std::is_convertible<T, void*>::value;
	};


	enum class VariableType { Scalar, Array, Struct };

	class Variable {
	public:
		virtual ~Variable() = default;
		VariableType Type() const { return type; }

	protected:
		Variable(VariableType type) : type(type) {}

	private:
		VariableType type;
	};


	class VariableInstancer {
	public:
		virtual ~VariableInstancer() = default;
		virtual UniquePtr<Variable> Instance(void* ptr) = 0;
	};



	class Scalar : public Variable {
	public:
		virtual bool Get(Variant& variant) = 0;
		virtual bool Set(const Variant& variant) = 0;

	protected:
		Scalar() : Variable(VariableType::Scalar) {}
	};

	template<typename T>
	class ScalarDefault final : public Scalar {
	public:
		ScalarDefault(T* ptr) : ptr(ptr) { RMLUI_ASSERT(ptr); }
		bool Get(Variant& variant) override {
			variant = *ptr;
			return true;
		}
		bool Set(const Variant& variant) override {
			return variant.GetInto<T>(*ptr);
		}
	private:
		T* ptr;
	};

	class Array : public Variable {
	public:
		Variable* operator[](const int index) const 
		{
			if (index < 0 || index >= (int)variables.size())
			{
				Log::Message(Log::LT_WARNING, "Data array index out of bounds.");
				return nullptr;
			}
			return variables[index].get();
		}

		int Size() const {
			return (int)variables.size();
		}

		virtual void Update() = 0;

	protected:
		Array() : Variable(VariableType::Array) {}
		std::vector<UniquePtr<Variable>> variables;
	};

	template<typename Container>
	class ArrayDefault final : public Array {
	public:
		ArrayDefault(Container* ptr, VariableInstancer* item_instancer) : ptr(ptr), item_instancer(item_instancer) { Update(); }

		void Update() override
		{
			variables.resize(ptr->size());

			for (size_t i = 0; i < variables.size(); i++)
				variables[i] = item_instancer->Instance(&((*ptr)[i]));
		}
	private:
		Container* ptr;
		VariableInstancer* item_instancer;
	};



	class Struct final : public Variable {
	public:
		Struct(SmallUnorderedMap<String, UniquePtr<Variable>>&& members) : Variable(VariableType::Struct), members(std::move(members))
		{}

		Variable* operator[](const String& name) const {
			auto it = members.find(name);
			if (it == members.end())
				return nullptr;
			return it->second.get();
		}

	private:
		SmallUnorderedMap<String, UniquePtr<Variable>> members;
	};



	template<typename T>
	class ScalarInstancer final : public VariableInstancer {
	public:
		UniquePtr<Variable> Instance(void* ptr) override {
			return std::make_unique<ScalarDefault<T>>( static_cast<T*>(ptr) );
		}
	};


	template<typename Container>
	class ArrayInstancer final : public VariableInstancer {
	public:
		ArrayInstancer(VariableInstancer* item_instancer) : item_instancer(item_instancer) {}

		UniquePtr<Variable> Instance(void* ptr) override {
			return std::make_unique<ArrayDefault<Container>>(static_cast<Container*>(ptr), item_instancer);
		}
	private:
		VariableInstancer* item_instancer;
	};


	class StructInstancer final : public VariableInstancer {
	public:
		StructInstancer() {}

		template <typename Object, typename MemberType>
		void AddMember(const String& name, MemberType Object::* member_ptr, VariableInstancer* instancer) {
			RMLUI_ASSERT(instancer && instancer != this);
			members.emplace(name, std::make_unique< MemberInstancer<Object, MemberType> >(member_ptr, instancer));
		}

		UniquePtr<Variable> Instance(void* ptr_to_struct) override {
			SmallUnorderedMap<String, UniquePtr<Variable>> variable_members;
			variable_members.reserve(members.size());
			for (auto& member : members)
			{
				auto variable = member.second->Instance(ptr_to_struct);
				bool inserted = variable_members.emplace(member.first, std::move(variable)).second;
				RMLUI_ASSERT(inserted);
			}
			return std::make_unique< Struct >(std::move(variable_members));
		}
	private:
		class MemberInstancerBase {
		public:
			virtual ~MemberInstancerBase() = default;
			virtual UniquePtr<Variable> Instance(void* base_ptr) = 0;
		};

		template <typename Object, typename MemberType>
		class MemberInstancer final : public MemberInstancerBase {
		public:
			MemberInstancer(MemberType Object::* member_ptr, VariableInstancer* instancer) : member_ptr(member_ptr), instancer(instancer) {}

			UniquePtr<Variable> Instance(void* base_ptr) override {
				return instancer->Instance(&(static_cast<Object*>(base_ptr)->*member_ptr));
			}

		private:
			MemberType Object::* member_ptr;
			VariableInstancer* instancer;
		};

		SmallUnorderedMap<String, UniquePtr<MemberInstancerBase>> members;
	};


	class Instancers;

	class InstancerBuilderBase {
	public:
		operator bool() const { return instancers && GetInstancer(); }

		virtual VariableInstancer* GetInstancer() const = 0;

	protected:
		InstancerBuilderBase(Instancers* instancers) : instancers(instancers) {}
		Instancers* instancers;
	};

	template<typename Object>
	class InstancerBuilderStruct final : public InstancerBuilderBase {
	public:
		InstancerBuilderStruct(Instancers* instancers, StructInstancer* instancer) : InstancerBuilderBase(instancers), instancer(instancer) {}

		template <typename MemberType>
		InstancerBuilderStruct<Object>& AddMember(const String& name, MemberType Object::* member_ptr) {
			static_assert(typename is_valid_scalar<MemberType>::value, "Not a valid scalar member type. Did you mean to add a struct member?");
			VariableInstancer* member_instancer = instancers->GetOrAddScalar<MemberType>();
			instancer->AddMember(name, member_ptr, member_instancer);
			return *this;
		}

		template <typename MemberType>
		InstancerBuilderStruct<Object>& AddMember(const String& name, MemberType Object::* member_ptr, const InstancerBuilderBase& member_instancer) {
			RMLUI_ASSERTMSG(instancers->Get<MemberType>() == member_instancer.GetInstancer(), "Mismatch between member type and provided struct instancer.");
			instancer->AddMember(name, member_ptr, member_instancer.GetInstancer());
			return *this;
		}

		VariableInstancer* GetInstancer() const override {
			return instancer;
		}
	private:
		StructInstancer* instancer;
	};

	template<typename Container>
	class InstancerBuilderArray final : public InstancerBuilderBase {
	public:
		InstancerBuilderArray(Instancers* instancers, ArrayInstancer<Container>* instancer) : InstancerBuilderBase(instancers), instancer(instancer) {}

		VariableInstancer* GetInstancer() const override {
			return instancer;
		}
	private:
		ArrayInstancer<Container>* instancer;
	};




	class Instancers {
	public:

		template<typename T>
		InstancerBuilderStruct<T> RegisterStruct() 
		{
			int family_id = Family<T>::Id();

			auto result = instancers.emplace(family_id, std::make_unique<StructInstancer>());
			auto& it = result.first;
			bool inserted = result.second;
			if (!inserted)
			{
				RMLUI_ERRORMSG("Type already declared");
				return  InstancerBuilderStruct<T>(nullptr, nullptr);
			}
			
			return InstancerBuilderStruct<T>(this, static_cast<StructInstancer*>(it->second.get()));
		}

		template<typename Container>
		InstancerBuilderArray<Container> RegisterArray()
		{
			using value_type = typename Container::value_type;
			static_assert(typename is_valid_scalar<value_type>::value, "Underlying value type of array is not a valid scalar type. Did you mean to add a struct or array?");

			VariableInstancer* value_instancer = GetOrAddScalar<value_type>();

			return RegisterArray<Container>(value_instancer);
		}

		template<typename Container>
		InstancerBuilderArray<Container> RegisterArray(const InstancerBuilderBase& member_instancer)
		{
			using value_type = typename Container::value_type;

			VariableInstancer* value_instancer = Get<value_type>();
			if (!value_instancer)
			{
				RMLUI_ERRORMSG("Underlying value type of array has not been registered.");
				return  InstancerBuilderArray<Container>(nullptr, nullptr);
			}

			return RegisterArray<Container>(value_instancer);
		}

		template<typename T>
		VariableInstancer* GetOrAddScalar()
		{
			int id = Family<T>::Id();

			auto result = instancers.emplace(id, nullptr);
			auto& it = result.first;
			bool inserted = result.second;

			if (inserted)
			{
				it->second = std::make_unique<ScalarInstancer<T>>();
			}

			return it->second.get();
		}

		template<typename T>
		VariableInstancer* Get() const
		{
			int id = Family<T>::Id();
			auto it = instancers.find(id);
			if (it == instancers.end())
				return nullptr;

			return it->second.get();
		}

		template<typename T>
		UniquePtr<Variable> Instance(T* ptr) const
		{
			if (auto instancer = Get<T>())
				return instancer->Instance(ptr);
			return nullptr;
		}

	private:
		template<typename Container>
		InstancerBuilderArray<Container> RegisterArray(VariableInstancer* value_instancer)
		{
			int container_id = Family<Container>::Id();

			auto result = instancers.emplace(container_id, std::make_unique<ArrayInstancer<Container>>(value_instancer));
			auto& it = result.first;
			bool inserted = result.second;
			if (!inserted)
			{
				RMLUI_ERRORMSG("Type already declared");
				return  InstancerBuilderArray<Container>(nullptr, nullptr);
			}

			return InstancerBuilderArray<Container>(this, static_cast<ArrayInstancer<Container>*>(it->second.get()));
		}

		UnorderedMap<int, UniquePtr<VariableInstancer>> instancers;
	};




}



void TestDataVariable()
{
	using namespace Rml::Core;
	using namespace Data;

	using IntVector = std::vector<int>;

	struct FunData {
		int i = 99;
		String x = "hello";
		IntVector magic = { 3, 5, 7, 11, 13 };
	};

	using FunArray = std::array<FunData, 3>;

	struct SmartData {
		bool valid = true;
		FunData fun;
		FunArray more_fun;
	};

	Instancers instancers;

	auto int_vector_instancer = instancers.RegisterArray<IntVector>();

	auto fun_instancer = instancers.RegisterStruct<FunData>();
	if (fun_instancer)
	{
		fun_instancer.AddMember("i", &FunData::i);
		fun_instancer.AddMember("x", &FunData::x);
		fun_instancer.AddMember("magic", &FunData::magic, int_vector_instancer);
	}

	auto fun_array_instancer = instancers.RegisterArray<FunArray>(fun_instancer);

	auto smart_instancer = instancers.RegisterStruct<SmartData>();
	if (smart_instancer)
	{
		smart_instancer.AddMember("valid", &SmartData::valid);
		smart_instancer.AddMember("fun", &SmartData::fun, fun_instancer);
		smart_instancer.AddMember("more_fun", &SmartData::more_fun, fun_array_instancer);
	}

	{
		SmartData data;
		data.fun.x = "Hello, we're in SmartData!";
		UniquePtr<Variable> variable_struct = instancers.Instance(&data);

		struct AddressEntry {
			AddressEntry(String name) : name(name), index(-1) { }
			AddressEntry(int index) : index(index) { }
			String name;
			int index;
		};
		std::vector<AddressEntry> address = { AddressEntry("more_fun"), AddressEntry(1), AddressEntry("magic"), AddressEntry(3) };


		Variable* var = variable_struct.get();
		for (int i = 0; i < (int)address.size(); i++)
		{
			if (!var)
			{
				RMLUI_ERROR;
				break;
			}
			const AddressEntry& entry = address[i];

			switch (var->Type()) {
			case VariableType::Struct:
			{
				Struct& the_struct = static_cast<Struct&>(*var);
				var = the_struct[entry.name];
				break;
			}
			case VariableType::Array:
			{
				Array& the_array = static_cast<Array&>(*var);
				var = the_array[entry.index];
				break;
			}
			case VariableType::Scalar:
			{
				Log::Message(Log::LT_WARNING, "Invalid data variable address. The scalar variable '%s' was encountered, but it needs to be the final entry of an address.", (i > 0 ? address[i - 1].name.c_str() : ""));
				var = nullptr;
				break;
			}
			default:
				RMLUI_ERROR;
				var = nullptr;
			}
		}

		bool success = false;

		if (var && var->Type() == VariableType::Scalar)
		{
			Variant variant;
			if (static_cast<Scalar*>(var)->Get(variant))
			{
				String result = variant.Get<String>();
				success = true;
			}
		}

		if (!success)
		{
			RMLUI_ERRORMSG("Could not get value");
		}
	}

}






bool SetupDataBinding(Rml::Core::Context* context)
{
	my_model = context->CreateDataModel("my_model");
	if (!my_model)
		return false;

	my_model.BindValue("hello_world", &my_data.hello_world);
	my_model.BindValue("rating", &my_data.rating);
	my_model.BindValue("good_rating", &my_data.good_rating);

	auto invader_type = my_model.RegisterType<Invader>();
	invader_type.RegisterMember("name", &Invader::name);
	invader_type.RegisterMember("sprite", &Invader::sprite);
	invader_type.RegisterMember("color", &Invader::color);

	my_model.BindTypeValue("invader", &my_data.invader);

	my_model.BindContainer("indices", &my_data.indices);

	TestDataVariable();

	return true;
}



Rml::Core::Context* context = nullptr;
ShellRenderInterfaceExtensions *shell_renderer;
std::unique_ptr<DemoWindow> demo_window;

void GameLoop()
{
	my_model.UpdateControllers();
	my_data.good_rating = (my_data.rating > 50);
	my_model.UpdateViews();

	demo_window->Update();
	context->Update();

	shell_renderer->PrepareRenderBuffer();
	context->Render();
	shell_renderer->PresentRenderBuffer();
}




class DemoEventListener : public Rml::Core::EventListener
{
public:
	DemoEventListener(const Rml::Core::String& value, Rml::Core::Element* element) : value(value), element(element) {}

	void ProcessEvent(Rml::Core::Event& event) override
	{
		using namespace Rml::Core;

		if (value == "exit")
		{
			Shell::RequestExit();
		}
	}

	void OnDetach(Rml::Core::Element* element) override { delete this; }

private:
	Rml::Core::String value;
	Rml::Core::Element* element;
};



class DemoEventListenerInstancer : public Rml::Core::EventListenerInstancer
{
public:
	Rml::Core::EventListener* InstanceEventListener(const Rml::Core::String& value, Rml::Core::Element* element) override
	{
		return new DemoEventListener(value, element);
	}
};


#if defined RMLUI_PLATFORM_WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE RMLUI_UNUSED_PARAMETER(instance_handle), HINSTANCE RMLUI_UNUSED_PARAMETER(previous_instance_handle), char* RMLUI_UNUSED_PARAMETER(command_line), int RMLUI_UNUSED_PARAMETER(command_show))
#else
int main(int RMLUI_UNUSED_PARAMETER(argc), char** RMLUI_UNUSED_PARAMETER(argv))
#endif
{
#ifdef RMLUI_PLATFORM_WIN32
	RMLUI_UNUSED(instance_handle);
	RMLUI_UNUSED(previous_instance_handle);
	RMLUI_UNUSED(command_line);
	RMLUI_UNUSED(command_show);
#else
	RMLUI_UNUSED(argc);
	RMLUI_UNUSED(argv);
#endif

	const int width = 1600;
	const int height = 900;

	ShellRenderInterfaceOpenGL opengl_renderer;
	shell_renderer = &opengl_renderer;

	// Generic OS initialisation, creates a window and attaches OpenGL.
	if (!Shell::Initialise() ||
		!Shell::OpenWindow("Data Binding Sample", shell_renderer, width, height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// RmlUi initialisation.
	Rml::Core::SetRenderInterface(&opengl_renderer);
	opengl_renderer.SetViewport(width, height);

	ShellSystemInterface system_interface;
	Rml::Core::SetSystemInterface(&system_interface);

	Rml::Core::Initialise();

	// Create the main RmlUi context and set it on the shell's input layer.
	context = Rml::Core::CreateContext("main", Rml::Core::Vector2i(width, height));

	if (!context || !SetupDataBinding(context))
	{
		Rml::Core::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Controls::Initialise();
	Rml::Debugger::Initialise(context);
	Input::SetContext(context);
	shell_renderer->SetContext(context);
	
	DemoEventListenerInstancer event_listener_instancer;
	Rml::Core::Factory::RegisterEventListenerInstancer(&event_listener_instancer);

	Shell::LoadFonts("assets/");

	demo_window = std::make_unique<DemoWindow>("Data binding", Rml::Core::Vector2f(150, 50), context);
	demo_window->GetDocument()->AddEventListener(Rml::Core::EventId::Keydown, demo_window.get());
	demo_window->GetDocument()->AddEventListener(Rml::Core::EventId::Keyup, demo_window.get());

	Shell::EventLoop(GameLoop);

	demo_window->Shutdown();

	// Shutdown RmlUi.
	Rml::Core::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	demo_window.reset();

	return 0;
}
