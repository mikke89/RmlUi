/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "precompiled.h"
#include "../../Include/RmlUi/Core/DataModel.h"
#include "DataParser.h"
#include <stack>

#ifdef _MSC_VER
#pragma warning(default : 4061)
#pragma warning(default : 4062)
#endif

namespace Rml {
namespace Core {

class Interpreter;
class DataParser;

/*
	The abstract machine for RmlUi data scripts.

	The machine can execute a program which contains a list of instructions listed below.

	The abstract machine has three registers:
		R  Typically results and right-hand side arguments.
		L  Typically left-hand side arguments.
		C  Typically center arguments (eg. in ternary operator).

	And two stacks:
		S  The main program stack.
		A  The arguments stack, only used to pass arguments to an external transform function.

	In addition, each instruction has an optional payload:
		D  Instruction data (payload).

	Notation used in the instruction list below:
		S+  Push to stack S.
		S-  Pop stack S (returns the popped value).
*/
enum class Instruction { // Assignment (register/stack) = Read (register R/L/C, instruction data D, or stack)
	Push      = 'P',     //      S+ = R
	Pop       = 'o',     // <R/L/C> = S-  (D determines R/L/C)
	Literal   = 'D',     //       R = D
	Variable  = 'V',     //       R = DataModel.GetVariable(D)  (D is an index into the variable address list)
	Add       = '+',     //       R = L + R
	Subtract  = '-',     //       R = L - R
	Multiply  = '*',     //       R = L * R
	Divide    = '/',     //       R = L / R
	Not       = '!',     //       R = !R
	And       = '&',     //       R = L && R
	Or        = '|',     //       R = L || R
	Less      = '<',     //       R = L < R
	LessEq    = 'L',     //       R = L <= R
	Greater   = '>',     //       R = L > R
	GreaterEq = 'G',     //       R = L >= R
	Equal     = '=',     //       R = L == R
	NotEqual  = 'N',     //       R = L != R
	Ternary   = '?',     //       R = L ? C : R
	Arguments = 'a',     //      A+ = S-  (Repeated D times, where D gives the num. arguments)
	Function  = 'F',     //       R = DataModel.Execute( D, R, A ); A.Clear();  (D determines function name, R the input value, A the arguments)
};
enum class Register {
	R,
	L,
	C
};

struct InstructionData {
	Instruction instruction;
	Variant data;
};

namespace Parse {
	static void Expression(DataParser& parser);
};


class DataParser {
public:
	DataParser(String expression, DataVariableInterface variable_interface) : expression(std::move(expression)), variable_interface(variable_interface) {}

	char Look() {
		if (reached_end)
			return '\0';
		return expression[index];
	}

	bool Match(char c, bool skip_whitespace = true) {
		if (c == Look()) {
			Next();
			if (skip_whitespace)
				SkipWhitespace();
			return true;
		}
		Expected(c);
		return false;
	}

	char Next() {
		++index;
		if (index >= expression.size())
			reached_end = true;
		return Look();
	}

	void SkipWhitespace() {
		char c = Look();
		while (StringUtilities::IsWhitespace(c))
			c = Next();
	}

	void Error(const String message)
	{
		parse_error = true;
		Log::Message(Log::LT_WARNING, "Error in data expression at %d. %s", index, message.c_str());
		Log::Message(Log::LT_WARNING, "  \"%s\"", expression.c_str());
		
		const size_t cursor_offset = size_t(index) + 3;
		const String cursor_string = String(cursor_offset, ' ') + '^';
		Log::Message(Log::LT_WARNING, cursor_string.c_str());
	}
	void Expected(String expected_symbols) {
		const char c = Look();
		if (c == '\0')
			Error(CreateString(expected_symbols.size() + 50, "Expected %s but found end of string.", expected_symbols.c_str()));
		else
			Error(CreateString(expected_symbols.size() + 50, "Expected %s but found character '%c'.", expected_symbols.c_str(), c));
	}
	void Expected(char expected) {
		Expected(String(1, '\'') + expected + '\'');
	}

	bool Parse() 
	{
		program.clear();
		variable_addresses.clear();
		index = 0;
		reached_end = false;
		parse_error = false;
		if (expression.empty())
			reached_end = true;

		SkipWhitespace();
		Parse::Expression(*this);
		
		if (!reached_end) {
			parse_error = true;
			Error(CreateString(50, "Unexpected character '%c' encountered.", Look()));
		}
		if (!parse_error && program_stack_size != 0) {
			parse_error = true;
			Error(CreateString(120, "Internal parser error, inconsistent stack operations. Stack size is %d at parse end.", program_stack_size));
		}

		return !parse_error;
	}

	Program ReleaseProgram() {
		RMLUI_ASSERT(!parse_error);
		return std::move(program);
	}
	AddressList ReleaseAddresses() {
		RMLUI_ASSERT(!parse_error);
		return std::move(variable_addresses);
	}

	void Emit(Instruction instruction, Variant data = Variant())
	{
		RMLUI_ASSERTMSG(instruction != Instruction::Push && instruction != Instruction::Pop &&
			instruction != Instruction::Arguments && instruction != Instruction::Variable,
			"Use the Push(), Pop(), Arguments(), and Variable() procedures for stack manipulation and variable instructions.");
		program.push_back(InstructionData{ instruction, std::move(data) });
	}
	void Push() {
		program_stack_size += 1;
		program.push_back(InstructionData{ Instruction::Push, Variant() });
	}
	void Pop(Register destination) {
		if (program_stack_size <= 0) {
			Error("Internal parser error: Tried to pop an empty stack.");
			return;
		}
		program_stack_size -= 1;
		program.push_back(InstructionData{ Instruction::Pop, Variant(int(destination)) });
	}
	void Arguments(int num_arguments) {
		if (program_stack_size < num_arguments) {
			Error(CreateString(128, "Internal parser error: Popping %d arguments, but the stack contains only %d elements.", num_arguments, program_stack_size));
			return;
		}
		program_stack_size -= num_arguments;
		program.push_back(InstructionData{ Instruction::Arguments, Variant(int(num_arguments)) });
	}
	void Variable(const String& name) {
		DataAddress address = variable_interface.ParseAddress(name);
		if (address.empty()) {
			Error(CreateString(name.size() + 50, "Could not find data variable with name '%s'.", name.c_str()));
			return;
		}
		int index = int(variable_addresses.size());
		variable_addresses.push_back(std::move(address));
		program.push_back(InstructionData{ Instruction::Variable, Variant(int(index)) });
	}

private:
	const String expression;
	DataVariableInterface variable_interface;

	size_t index = 0;
	bool reached_end = false;
	bool parse_error = true;
	int program_stack_size = 0;

	Program program;
	
	AddressList variable_addresses;
};


namespace Parse {

	// Forward declare all parse functions.
	static void Expression(DataParser& parser);
	static void Factor(DataParser& parser);
	static void Term(DataParser& parser);

	static void NumberLiteral(DataParser& parser);
	static void StringLiteral(DataParser& parser);
	static void Variable(DataParser& parser);

	static void Add(DataParser& parser);
	static void Subtract(DataParser& parser);
	static void Multiply(DataParser& parser);
	static void Divide(DataParser& parser);

	static void Not(DataParser& parser);
	static void And(DataParser& parser);
	static void Or(DataParser& parser);
	static void Less(DataParser& parser);
	static void Greater(DataParser& parser);
	static void Equal(DataParser& parser);
	static void NotEqual(DataParser& parser);

	static void Ternary(DataParser& parser);
	static void Function(DataParser& parser);

	// Helper functions
	static bool IsVariableCharacter(char c, bool is_first_character)
	{
		const bool is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');

		if (is_first_character)
			return is_alpha;

		if (is_alpha || (c >= '0' && c <= '9'))
			return true;

		for (char valid_char : "_.[] ")
		{
			if (c == valid_char && valid_char != '\0')
				return true;
		}

		return false;
	}
	static String VariableName(DataParser& parser)
	{
		String name;

		bool is_first_character = true;
		char c = parser.Look();

		while (IsVariableCharacter(c, is_first_character))
		{
			name += c;
			c = parser.Next();
			is_first_character = false;
		}

		// Right trim spaces in name
		size_t new_size = String::npos;
		for (int i = int(name.size()) - 1; i >= 1; i--)
		{
			if (name[i] == ' ')
				new_size = size_t(i);
			else
				break;
		}
		if (new_size != String::npos)
			name.resize(new_size);

		return name;
	}

	// Parser functions
	static void Expression(DataParser& parser)
	{
		Term(parser);

		bool looping = true;
		while (looping)
		{
			switch (char c = parser.Look())
			{
			case '+': Add(parser); break;
			case '-': Subtract(parser); break;
			case '?': Ternary(parser); break;
			case '|':
			{
				parser.Match('|', false);
				if (parser.Look() == '|')
					Or(parser);
				else
				{
					parser.SkipWhitespace();
					Function(parser);
				}
			}
			break;
			case '&': And(parser); break;
			case '=': Equal(parser); break;
			case '!': NotEqual(parser); break;
			case '<': Less(parser); break;
			case '>': Greater(parser); break;
			case '\0':
				looping = false;
				break;
			default:
				looping = false;
			}
		}
	}
	static void Term(DataParser& parser)
	{
		Factor(parser);

		bool looping = true;
		while (looping)
		{
			switch (const char c = parser.Look())
			{
			case '*': Multiply(parser); break;
			case '/': Divide(parser); break;
			case '\0': looping = false; break;
			default:
				looping = false;
			}
		}
	}
	static void Factor(DataParser& parser)
	{
		const char c = parser.Look();

		if (c == '(')
		{
			parser.Match('(');
			Expression(parser);
			parser.Match(')');
		}
		else if (c == '\'')
		{
			parser.Match('\'', false);
			StringLiteral(parser);
			parser.Match('\'');
		}
		else if (c == '!')
		{
			Not(parser);
			parser.SkipWhitespace();
		}
		else if (c == '-' || (c >= '0' && c <= '9'))
		{
			NumberLiteral(parser);
			parser.SkipWhitespace();
		}
		else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		{
			Variable(parser);
			parser.SkipWhitespace();
		}
		else
			parser.Expected("literal, variable name, parenthesis, or '!'");
	}

	static void NumberLiteral(DataParser& parser)
	{
		String str;

		bool first_match = false;
		bool has_dot = false;
		char c = parser.Look();
		if (c == '-')
		{
			str += c;
			c = parser.Next();
		}

		while ((c >= '0' && c <= '9') || (c == '.' && !has_dot))
		{
			first_match = true;
			str += c;
			if (c == '.')
				has_dot = true;
			c = parser.Next();
		}

		if (!first_match)
		{
			parser.Error(CreateString(100, "Invalid number literal. Expected '0-9' or '.' but found '%c'.", c));
			return;
		}

		const double number = FromString(str, 0.0);

		parser.Emit(Instruction::Literal, Variant(number));
	}
	static void StringLiteral(DataParser& parser)
	{
		String str;

		char c = parser.Look();
		char c_prev = '\0';

		while (c != '\0' && (c != '\'' || c_prev == '\\'))
		{
			if (c_prev == '\\' && (c == '\\' || c == '\'')) {
				str.pop_back();
				c_prev = '\0';
			}
			else {
				c_prev = c;
			}

			str += c;
			c = parser.Next();
		}

		parser.Emit(Instruction::Literal, Variant(str));
	}
	static void Variable(DataParser& parser)
	{
		String name = VariableName(parser);
		if (name.empty()) {
			parser.Error("Expected a variable but got an empty name.");
			return;
		}

		// Keywords are parsed like variables, but are really literals.
		// Check for them here.
		if (name == "true")
			parser.Emit(Instruction::Literal, Variant(true));
		else if (name == "false")
			parser.Emit(Instruction::Literal, Variant(false));
		else
			parser.Variable(name);
	}

	static void Add(DataParser& parser)
	{
		parser.Match('+');
		parser.Push();
		Term(parser);
		parser.Pop(Register::L);
		parser.Emit(Instruction::Add);
	}
	static void Subtract(DataParser& parser)
	{
		parser.Match('-');
		parser.Push();
		Term(parser);
		parser.Pop(Register::L);
		parser.Emit(Instruction::Subtract);
	}
	static void Multiply(DataParser& parser)
	{
		parser.Match('*');
		parser.Push();
		Factor(parser);
		parser.Pop(Register::L);
		parser.Emit(Instruction::Multiply);
	}
	static void Divide(DataParser& parser)
	{
		parser.Match('/');
		parser.Push();
		Factor(parser);
		parser.Pop(Register::L);
		parser.Emit(Instruction::Divide);
	}

	static void Not(DataParser& parser)
	{
		parser.Match('!');
		Factor(parser);
		parser.Emit(Instruction::Not);
	}
	static void Or(DataParser& parser)
	{
		// We already skipped the first '|' during expression
		parser.Match('|');
		parser.Push();
		Term(parser);
		parser.Pop(Register::L);
		parser.Emit(Instruction::Or);
	}
	static void And(DataParser& parser)
	{
		parser.Match('&', false);
		parser.Match('&');
		parser.Push();
		Term(parser);
		parser.Pop(Register::L);
		parser.Emit(Instruction::And);
	}
	static void Less(DataParser& parser)
	{
		Instruction instruction = Instruction::Less;
		parser.Match('<', false);
		if (parser.Look() == '=') {
			parser.Match('=');
			instruction = Instruction::LessEq;
		}
		else {
			parser.SkipWhitespace();
		}
		parser.Push();
		Term(parser);
		parser.Pop(Register::L);
		parser.Emit(instruction);
	}
	static void Greater(DataParser& parser)
	{
		Instruction instruction = Instruction::Greater;
		parser.Match('>', false);
		if (parser.Look() == '=') {
			parser.Match('=');
			instruction = Instruction::GreaterEq;
		}
		else {
			parser.SkipWhitespace();
		}
		parser.Push();
		Term(parser);
		parser.Pop(Register::L);
		parser.Emit(instruction);
	}
	static void Equal(DataParser& parser)
	{
		parser.Match('=', false);
		parser.Match('=');
		parser.Push();
		Term(parser);
		parser.Pop(Register::L);
		parser.Emit(Instruction::Equal);
	}
	static void NotEqual(DataParser& parser)
	{
		parser.Match('!', false);
		parser.Match('=');
		parser.Push();
		Term(parser);
		parser.Pop(Register::L);
		parser.Emit(Instruction::NotEqual);
	}

	static void Ternary(DataParser& parser)
	{
		parser.Match('?');
		parser.Push();
		Expression(parser);
		parser.Push();
		parser.Match(':');
		Expression(parser);
		parser.Pop(Register::C);
		parser.Pop(Register::L);
		parser.Emit(Instruction::Ternary);
	}
	static void Function(DataParser& parser)
	{
		// We already matched '|' during expression
		String name = VariableName(parser);
		if (name.empty()) {
			parser.Error("Expected a transform name but got an empty name.");
			return;
		}

		if (parser.Look() == '(')
		{
			int num_arguments = 0;
			bool looping = true;

			parser.Match('(');
			if (parser.Look() == ')') {
				parser.Match(')');
				looping = false;
			}
			else
				parser.Push();

			while (looping)
			{
				num_arguments += 1;
				Expression(parser);
				parser.Push();

				switch (parser.Look()) {
				case ')': parser.Match(')'); looping = false; break;
				case ',': parser.Match(','); break;
				default:
					parser.Expected("one of ')' or ','");
					looping = false;
				}
			}

			if (num_arguments > 0) {
				parser.Arguments(num_arguments);
				parser.Pop(Register::R);
			}
		}
		else {
			parser.SkipWhitespace();
		}

		parser.Emit(Instruction::Function, Variant(name));
	}


} // </namespace Parse>



static String DumpProgram(const Program& program)
{
	String str;
	for (size_t i = 0; i < program.size(); i++)
	{
		String instruction_str = program[i].data.Get<String>();
		str += CreateString(instruction_str.size(), "  %4d  '%c'  %s\n", i, char(program[i].instruction), instruction_str.c_str());
	}
	return str;
}


class DataInterpreter {
public:
	DataInterpreter(const Program& program, const AddressList& addresses, DataVariableInterface variable_interface) : program(program), addresses(addresses), variable_interface(variable_interface) {}

	bool Error(String message) const
	{
		message = "Error during execution. " + message;
		Log::Message(Log::LT_WARNING, message.c_str());
		RMLUI_ERROR;
		return false;
	}

	bool Run()
	{
		bool success = true;
		for (size_t i = 0; i < program.size(); i++)
		{
			if (!Execute(program[i].instruction, program[i].data))
			{
				success = false;
				break;
			}
		}

		if(success && !stack.empty())
			Log::Message(Log::LT_WARNING, "Possible data interpreter stack corruption. Stack size is %d at end of execution (should be zero).", stack.size());

		if(!success)
		{
			String program_str = DumpProgram(program);
			Log::Message(Log::LT_WARNING, "Failed to execute program with %d instructions:", program.size());
			Log::Message(Log::LT_WARNING, program_str.c_str());
		}

		return success;
	}

	Variant Result() const {
		return R;
	}


private:
	Variant R, L, C;
	std::stack<Variant> stack;
	std::vector<Variant> arguments;

	const Program& program;
	const AddressList& addresses;
	DataVariableInterface variable_interface;

	bool Execute(const Instruction instruction, const Variant& data)
	{
		auto AnyString = [](const Variant& v1, const Variant& v2) {
			return v1.GetType() == Variant::STRING || v2.GetType() == Variant::STRING;
		};

		switch (instruction)
		{
		case Instruction::Push:
		{
			stack.push(std::move(R));
			R.Clear();
		}
		break;
		case Instruction::Pop:
		{
			if (stack.empty())
				return Error("Cannot pop stack, it is empty.");

			Register reg = Register(data.Get<int>(-1));
			switch (reg) {
			case Register::R:  R = stack.top(); stack.pop(); break;
			case Register::L:  L = stack.top(); stack.pop(); break;
			case Register::C:  C = stack.top(); stack.pop(); break;
			default:
				return Error(CreateString(50, "Invalid register %d.", int(reg)));
			}
		}
		break;
		case Instruction::Literal:
		{
			R = data;
		}
		break;
		case Instruction::Variable:
		{
			size_t variable_index = size_t(data.Get<int>(-1));
			if (variable_index < addresses.size())
				R = variable_interface.GetValue(addresses[variable_index]);
			else
				return Error("Variable address not found.");
		}
		break;
		case Instruction::Add:
		{
			if (AnyString(L, R))
				R = Variant(L.Get<String>() + R.Get<String>());
			else
				R = Variant(L.Get<double>() + R.Get<double>());
		}
		break;
		case Instruction::Subtract:  R = Variant(L.Get<double>() - R.Get<double>());  break;
		case Instruction::Multiply:  R = Variant(L.Get<double>() * R.Get<double>());  break;
		case Instruction::Divide:    R = Variant(L.Get<double>() / R.Get<double>());  break;
		case Instruction::Not:       R = Variant(!R.Get<bool>());                     break;
		case Instruction::And:       R = Variant(L.Get<bool>() && R.Get<bool>());     break;
		case Instruction::Or:        R = Variant(L.Get<bool>() || R.Get<bool>());     break;
		case Instruction::Less:      R = Variant(L.Get<double>() < R.Get<double>());  break;
		case Instruction::LessEq:    R = Variant(L.Get<double>() <= R.Get<double>()); break;
		case Instruction::Greater:   R = Variant(L.Get<double>() > R.Get<double>());  break;
		case Instruction::GreaterEq: R = Variant(L.Get<double>() >= R.Get<double>()); break;
		case Instruction::Equal:
		{
			if (AnyString(L, R))
				R = Variant(L.Get<String>() == R.Get<String>());
			else
				R = Variant(L.Get<double>() == R.Get<double>());
		}
		break;
		case Instruction::NotEqual:
		{
			if (AnyString(L, R))
				R = Variant(L.Get<String>() != R.Get<String>());
			else
				R = Variant(L.Get<double>() != R.Get<double>());
		}
		break;
		case Instruction::Ternary:
		{
			if (L.Get<bool>())
				R = C;
		}
		break;
		case Instruction::Arguments:
		{
			if (!arguments.empty())
				return Error("Argument stack is not empty.");

			int num_arguments = data.Get<int>(-1);
			if (num_arguments < 0)
				return Error("Invalid number of arguments.");
			if (stack.size() < size_t(num_arguments))
				return Error(CreateString(100, "Cannot pop %d arguments, stack contains only %d elements.", num_arguments, stack.size()));

			arguments.resize(num_arguments);
			for (int i = num_arguments - 1; i >= 0; i--)
			{
				arguments[i] = std::move(stack.top());
				stack.pop();
			}
		}
		break;
		case Instruction::Function:
		{
			const String function_name = data.Get<String>();
			
			if (!variable_interface.CallTransform(function_name, R, arguments))
			{
				String arguments_str;
				for (size_t i = 0; i < arguments.size(); i++)
				{
					arguments_str += arguments[i].Get<String>();
					if (i < arguments.size() - 1)
						arguments_str += ", ";
				}
				Error(CreateString(50 + function_name.size() + arguments_str.size(), "Failed to execute data function: %s(%s)", function_name.c_str(), arguments_str.c_str()));
			}

			arguments.clear();
		}
		break;
		default:
			RMLUI_ERRORMSG("Instruction not implemented."); break;
		}
		return true;
	}
};





struct TestParser {
	TestParser() : model(type_register.GetTransformFuncRegister())
	{
		DataModelHandle handle(&model, &type_register);
		handle.Bind("color_name", &color_name);
		handle.BindFunc("color_value", [this](Rml::Core::Variant& variant) {
			variant = ToString(color_value);
		});

		String result;
		result = TestExpression("!!10 - 1 ? 'hello' : 'world' | to_upper");
		result = TestExpression("(color_name) + (': rgba(' + color_value + ')')");
		result = TestExpression("'hello world' | to_upper(5 + 12 == 17 ? 'yes' : 'no', 9*2)");
		result = TestExpression("true == false");
		result = TestExpression("true != false");
		result = TestExpression("true");
		result = TestExpression(R"(true || false ? (true && true ? 'Absolutely!' : 'well..') : 'no')");
		result = TestExpression("2 * 2");
		result = TestExpression("50000 / 1500");
		result = TestExpression("5*1+2");
		result = TestExpression("5*(1+2)");
		result = TestExpression("5.2 + 19 + 'px'");
	}

	String TestExpression(String expression)
	{
		DataVariableInterface interface(&model, nullptr);

		DataParser parser(expression, interface);
		if (parser.Parse())
		{
			Program program = parser.ReleaseProgram();
			AddressList addresses = parser.ReleaseAddresses();

			DataInterpreter interpreter(program, addresses, interface);
			if (interpreter.Run())
				return interpreter.Result().Get<String>();
		}
		return "<invalid expression>";
	};

	DataTypeRegister type_register;
	DataModel model;

	String color_name = "color";
	Colourb color_value = Colourb(180, 100, 255);
};




DataExpression::DataExpression(String expression) : expression(expression) {}

DataExpression::~DataExpression()
{
}

bool DataExpression::Parse(const DataVariableInterface& variable_interface)
{
	// @todo: Remove, debugging only
	static TestParser test_parser;

	// TODO:
	//  3. Create a plug-in wrapper for use by scripting languages to replace this parser. Design wrapper as for events.
	//  5. Add tests

	DataParser parser(expression, variable_interface);
	if (!parser.Parse())
		return false;

	program = parser.ReleaseProgram();
	addresses = parser.ReleaseAddresses();

	return true;
}

bool DataExpression::Run(const DataVariableInterface& variable_interface, Variant& out_value)
{
	DataInterpreter interpreter(program, addresses, variable_interface);
	
	if (!interpreter.Run())
		return false;

	out_value = interpreter.Result();
	return true;
}

StringList DataExpression::GetVariableNameList() const
{
	StringList list;
	list.reserve(addresses.size());
	for (const DataAddress& address : addresses)
	{
		if (!address.empty())
			list.push_back(address[0].name);
	}
	return list;
}

DataVariableInterface::DataVariableInterface(DataModel* data_model, Element* element) : data_model(data_model), element(element)
{}

DataAddress DataVariableInterface::ParseAddress(const String& address_str) const {
	return data_model ? data_model->ResolveAddress(address_str, element) : DataAddress();
}
Variant DataVariableInterface::GetValue(const DataAddress& address) const {
	Variant result;
	if (data_model)
		data_model->GetValue(address, result);
	return result;
}

bool DataVariableInterface::CallTransform(const String& name, Variant& inout_variant, const VariantList& arguments)
{
	return data_model ? data_model->CallTransform(name, inout_variant, arguments) : false;
}

}
}
