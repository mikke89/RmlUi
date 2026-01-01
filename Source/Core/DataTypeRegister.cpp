#include "../../Include/RmlUi/Core/DataTypeRegister.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/Variant.h"

namespace Rml {

DataTypeRegister::DataTypeRegister()
{
	// Add default transform functions.

	transform_register.Register("to_lower", [](const VariantList& arguments) -> Variant {
		if (arguments.size() != 1)
			return {};
		String value;
		if (!arguments[0].GetInto(value))
			return {};
		return Variant(StringUtilities::ToLower(std::move(value)));
	});

	transform_register.Register("to_upper", [](const VariantList& arguments) -> Variant {
		if (arguments.size() != 1)
			return {};
		String value;
		if (!arguments[0].GetInto(value))
			return {};
		return Variant(StringUtilities::ToUpper(value));
	});

	transform_register.Register("format", [](const VariantList& arguments) -> Variant {
		// Arguments in:
		//   0 : number     Number to format.
		//   1 : int[0,32]  Precision. Number of digits after the decimal point.
		//  [2]: bool       True to remove trailing zeros (default = false).
		if (arguments.empty() || arguments.size() > 3)
		{
			Log::Message(Log::LT_WARNING, "Transform function 'format' requires at least two arguments, at most three arguments.");
			return {};
		}
		int precision = 0;
		if (!arguments[1].GetInto(precision) || precision < 0 || precision > 32)
		{
			Log::Message(Log::LT_WARNING, "Transform function 'format': Second argument must be an integer in [0, 32].");
			return {};
		}
		bool remove_trailing_zeros = false;
		if (arguments.size() >= 3)
		{
			if (!arguments[2].GetInto(remove_trailing_zeros))
				return {};
		}

		double value = 0;
		if (!arguments[0].GetInto(value))
			return {};

		String format_specifier = String(remove_trailing_zeros ? "%#." : "%.") + ToString(precision) + 'f';
		String result;
		if (FormatString(result, format_specifier.c_str(), value) == 0)
			return {};

		if (remove_trailing_zeros)
			StringUtilities::TrimTrailingDotZeros(result);

		return Variant(std::move(result));
	});

	transform_register.Register("round", [](const VariantList& arguments) -> Variant {
		if (arguments.size() != 1)
			return {};
		double value;
		if (!arguments[0].GetInto(value))
			return {};
		return Variant(Math::Round(value));
	});
}

DataTypeRegister::~DataTypeRegister() {}

void TransformFuncRegister::Register(const String& name, DataTransformFunc transform_func)
{
	RMLUI_ASSERT(transform_func);
	bool inserted = transform_functions.emplace(name, std::move(transform_func)).second;
	if (!inserted)
	{
		Log::Message(Log::LT_ERROR, "Transform function '%s' already exists.", name.c_str());
		RMLUI_ERROR;
	}
}

bool TransformFuncRegister::Call(const String& name, const VariantList& arguments, Variant& out_result) const
{
	auto it = transform_functions.find(name);
	if (it == transform_functions.end())
		return false;

	const DataTransformFunc& transform_func = it->second;
	RMLUI_ASSERT(transform_func);

	out_result = transform_func(arguments);
	return true;
}

} // namespace Rml
