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

#include "DataExpression.h"
#include "../../Include/RmlUi/Core/DataModelHandle.h"
#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/Variant.h"
#include "DataModel.h"
#include <stack>

#ifdef _MSC_VER
	#pragma warning(default : 4061)
	#pragma warning(default : 4062)
#endif

namespace Rml {

class DataParser;

/*
    The abstract machine for RmlUi data expressions.

    The machine can execute a program which contains a list of instructions listed below.

    The abstract machine has three registers:
        R  Typically results and right-hand side arguments.
        L  Typically left-hand side arguments.
        C  Typically center arguments (eg. in ternary operator).

    And a stack:
        S  The program stack.

    In addition, each instruction has an optional payload:
        D  Instruction data (payload).

    Notation used in the instruction list below:
        S+  Push to stack S.
        S-  Pop stack S (returns the popped value).
*/
enum class Instruction {
	// clang-format off
	// Assignment (register/stack) = Read (register R/L/C, instruction data D, or stack)
	Push            = 'P',     //      S+ = R
	Pop             = 'o',     // <R/L/C> = S-  (D determines R/L/C)
	Literal         = 'D',     //       R = D
	Variable        = 'V',     //       R = DataModel.GetVariable(D)  (D is an index into the variable address list)
	Add             = '+',     //       R = L + R
	Subtract        = '-',     //       R = L - R
	Multiply        = '*',     //       R = L * R
	Divide          = '/',     //       R = L / R
	Not             = '!',     //       R = !R
	And             = '&',     //       R = L && R
	Or              = '|',     //       R = L || R
	Less            = '<',     //       R = L < R
	LessEq          = 'L',     //       R = L <= R
	Greater         = '>',     //       R = L > R
	GreaterEq       = 'G',     //       R = L >= R
	Equal           = '=',     //       R = L == R
	NotEqual        = 'N',     //       R = L != R
	Ternary         = '?',     //       R = L ? C : R
	NumArguments    = '#',     //       R = D  (Contains the num. arguments currently on the stack, immediately followed by a 'T' or 'E' instruction)
	TransformFnc    = 'T',     //       R = DataModel.Execute(D, A) where A = S[TOP - R, TOP]; S -= R;  (D determines function name, input R the num. arguments, A the arguments)
	EventFnc        = 'E',     //       DataModel.EventCallback(D, A); S -= R;
	Assign          = 'A',     //       DataModel.SetVariable(D, R)
	DynamicVariable = 'Y',     //       DataModel.GetVariable(DataModel.ParseAddress(R)) (Looks up a variable by path in R)
	CastToInt       = 'I'      //       R = (int)R
	// clang-format on
};

enum class Register {
	R,
	L,
	C,
};

struct InstructionData {
	Instruction instruction;
	Variant data;
};

struct ProgramState {
	size_t program_length;
	int stack_size;
};

namespace Parse {
	static void Assignment(DataParser& parser);
	static void Expression(DataParser& parser);
} // namespace Parse

class DataParser {
public:
	DataParser(String expression, DataExpressionInterface expression_interface) :
		expression(std::move(expression)), expression_interface(expression_interface)
	{}

	char Look()
	{
		if (reached_end)
			return '\0';
		return expression[index];
	}

	bool Match(char c, bool skip_whitespace = true)
	{
		if (c == Look())
		{
			Next();
			if (skip_whitespace)
				SkipWhitespace();
			return true;
		}
		Expected(c);
		return false;
	}

	char Next()
	{
		++index;
		if (index >= expression.size())
			reached_end = true;
		return Look();
	}

	void SkipWhitespace()
	{
		char c = Look();
		while (StringUtilities::IsWhitespace(c))
			c = Next();
	}

	void Error(const String& message)
	{
		parse_error = true;
		Log::Message(Log::LT_WARNING, "Error in data expression at %zu. %s", index, message.c_str());
		Log::Message(Log::LT_WARNING, "  \"%s\"", expression.c_str());

		const size_t cursor_offset = size_t(index) + 3;
		const String cursor_string = String(cursor_offset, ' ') + '^';
		Log::Message(Log::LT_WARNING, "%s", cursor_string.c_str());
	}
	void Expected(const String& expected_symbols)
	{
		const char c = Look();
		if (c == '\0')
			Error(CreateString("Expected %s but found end of string.", expected_symbols.c_str()));
		else
			Error(CreateString("Expected %s but found character '%c'.", expected_symbols.c_str(), c));
	}
	void Expected(char expected) { Expected(String(1, '\'') + expected + '\''); }

	bool Parse(bool is_assignment_expression)
	{
		program.clear();
		variable_addresses.clear();
		index = 0;
		reached_end = false;
		parse_error = false;
		if (expression.empty())
			reached_end = true;

		SkipWhitespace();

		if (is_assignment_expression)
			Parse::Assignment(*this);
		else
			Parse::Expression(*this);

		if (!reached_end)
		{
			parse_error = true;
			Error(CreateString("Unexpected character '%c' encountered.", Look()));
		}
		if (!parse_error && program_stack_size != 0)
		{
			parse_error = true;
			Error(CreateString("Internal parser error, inconsistent stack operations. Stack size is %d at parse end.", program_stack_size));
		}

		return !parse_error;
	}

	Program ReleaseProgram()
	{
		RMLUI_ASSERT(!parse_error);
		return std::move(program);
	}
	AddressList ReleaseAddresses()
	{
		RMLUI_ASSERT(!parse_error);
		return std::move(variable_addresses);
	}

	void Emit(Instruction instruction, Variant data = Variant())
	{
		RMLUI_ASSERTMSG(instruction != Instruction::Push && instruction != Instruction::Pop && instruction != Instruction::NumArguments &&
				instruction != Instruction::TransformFnc && instruction != Instruction::EventFnc && instruction != Instruction::Variable &&
				instruction != Instruction::Assign,
			"Use Push(), Pop(), Function(), Variable(), and Assign() procedures for stack manipulation and variable instructions.");

		program.push_back(InstructionData{instruction, std::move(data)});
	}
	void Push()
	{
		program_stack_size += 1;
		program.push_back(InstructionData{Instruction::Push, Variant()});
	}
	void Pop(Register destination)
	{
		if (program_stack_size <= 0)
		{
			Error("Internal parser error: Tried to pop an empty stack.");
			return;
		}
		program_stack_size -= 1;
		program.push_back(InstructionData{Instruction::Pop, Variant(int(destination))});
	}
	void Function(Instruction instruction, int num_arguments, String&& name)
	{
		RMLUI_ASSERT(instruction == Instruction::TransformFnc || instruction == Instruction::EventFnc);
		RMLUI_ASSERT(num_arguments >= 0);
		if (program_stack_size < num_arguments)
		{
			Error(CreateString("Internal parser error: Popping %d arguments, but the stack contains only %d elements.", num_arguments,
				program_stack_size));
			return;
		}
		program_stack_size -= num_arguments;
		program.push_back(InstructionData{Instruction::NumArguments, Variant(int(num_arguments))});
		program.push_back(InstructionData{instruction, Variant(std::move(name))});
	}
	void Variable(const String& data_address) { VariableGetSet(data_address, false); }
	void Assign(const String& data_address) { VariableGetSet(data_address, true); }

	ProgramState GetProgramState() { return ProgramState{program.size(), program_stack_size}; }

	void SetProgramState(const ProgramState& state)
	{
		RMLUI_ASSERT(state.program_length <= program.size());
		program.resize(state.program_length);
		program_stack_size = state.stack_size;
	}

	bool AddVariableAddress(const String& name)
	{
		DataAddress address = expression_interface.ParseAddress(name);
		if (address.empty())
		{
			return false;
		}

		variable_addresses.push_back(std::move(address));
		return true;
	}

private:
	void VariableGetSet(const String& name, bool is_assignment)
	{
		DataAddress address = expression_interface.ParseAddress(name);
		if (address.empty())
		{
			Error(CreateString("Could not find data variable with name '%s'.", name.c_str()));
			return;
		}
		int index = int(variable_addresses.size());
		variable_addresses.push_back(std::move(address));
		program.push_back(InstructionData{is_assignment ? Instruction::Assign : Instruction::Variable, Variant(int(index))});
	}

	const String expression;
	DataExpressionInterface expression_interface;

	size_t index = 0;
	bool reached_end = false;
	bool parse_error = true;
	int program_stack_size = 0;

	Program program;

	AddressList variable_addresses;
};

namespace Parse {

	// Forward declare all parse functions.
	static void Assignment(DataParser& parser);

	// The following in order of precedence.
	static void Expression(DataParser& parser);
	static void Relational(DataParser& parser);
	static void Additive(DataParser& parser);
	static void Term(DataParser& parser);
	static void Factor(DataParser& parser);

	static void NumberLiteral(DataParser& parser);
	static void StringLiteral(DataParser& parser);
	static String VariableExpression(DataParser& parser, const String& address_prefix);
	static void VariableOrFunction(DataParser& parser);

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
	static void Function(DataParser& parser, Instruction function_type, String&& name, bool first_argument_piped);

	// Helper functions
	static bool IsVariableCharacter(char c, bool is_first_character)
	{
		const bool is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');

		if (is_first_character)
			return is_alpha;

		if (is_alpha || (c >= '0' && c <= '9'))
			return true;

		for (char valid_char : "_.")
		{
			if (c == valid_char && valid_char != '\0')
				return true;
		}

		return false;
	}
	static String VariableOrFunctionName(DataParser& parser, bool* out_valid_function_name)
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

		if (out_valid_function_name)
			*out_valid_function_name = (name.find_first_of(". ") == String::npos);

		return name;
	}

	static String FindNumberLiteral(DataParser& parser, bool silent)
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
			if (!silent)
				parser.Error(CreateString("Invalid number literal. Expected '0-9' or '.' but found '%c'.", c));

			return String();
		}

		return str;
	}

	// Parser functions
	static void Assignment(DataParser& parser)
	{
		bool looping = true;
		while (looping)
		{
			if (parser.Look() != '\0')
			{
				String variable_name = VariableOrFunctionName(parser, nullptr);
				if (variable_name.empty())
				{
					parser.Error("Expected a variable for assignment but got an empty name.");
					return;
				}

				parser.SkipWhitespace();

				const char c = parser.Look();
				if (c == '=')
				{
					parser.Match('=');
					Expression(parser);
					parser.Assign(variable_name);
				}
				else if (c == '(' || c == ';' || c == '\0')
				{
					Function(parser, Instruction::EventFnc, std::move(variable_name), false);
				}
				else
				{
					parser.Expected("one of  = ; (  or end of string");
					return;
				}
			}

			const char c = parser.Look();
			if (c == ';')
				parser.Match(';');
			else if (c == '\0')
				looping = false;
			else
			{
				parser.Expected("';' or end of string");
				looping = false;
			}
		}
	}
	static void Expression(DataParser& parser)
	{
		Relational(parser);

		bool looping = true;
		while (looping)
		{
			switch (parser.Look())
			{
			case '&': And(parser); break;
			case '|':
			{
				parser.Match('|', false);
				if (parser.Look() == '|')
					Or(parser);
				else
				{
					parser.Push();
					parser.SkipWhitespace();
					bool valid_function_name = true;
					String name = VariableOrFunctionName(parser, &valid_function_name);
					if (name.empty())
					{
						parser.Error("Expected a transform function name but got an empty name.");
						return;
					}
					if (!valid_function_name)
					{
						parser.Error("Expected a transform function name but got an invalid name '" + name + "'.");
						return;
					}

					Function(parser, Instruction::TransformFnc, std::move(name), true);
				}
			}
			break;
			case '?': Ternary(parser); break;
			default: looping = false;
			}
		}
	}

	static void Relational(DataParser& parser)
	{
		Additive(parser);

		bool looping = true;
		while (looping)
		{
			switch (parser.Look())
			{
			case '=': Equal(parser); break;
			case '!': NotEqual(parser); break;
			case '<': Less(parser); break;
			case '>': Greater(parser); break;
			default: looping = false;
			}
		}
	}

	static void Additive(DataParser& parser)
	{
		Term(parser);

		bool looping = true;
		while (looping)
		{
			switch (parser.Look())
			{
			case '+': Add(parser); break;
			case '-': Subtract(parser); break;
			default: looping = false;
			}
		}
	}

	static void Term(DataParser& parser)
	{
		Factor(parser);

		bool looping = true;
		while (looping)
		{
			switch (parser.Look())
			{
			case '*': Multiply(parser); break;
			case '/': Divide(parser); break;
			default: looping = false;
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
			VariableOrFunction(parser);
			parser.SkipWhitespace();
		}
		else
			parser.Expected("literal, variable name, function name, parenthesis, or '!'");
	}

	static void NumberLiteral(DataParser& parser)
	{
		String str = FindNumberLiteral(parser, false);

		if (str.empty())
			return;

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
			if (c_prev == '\\' && (c == '\\' || c == '\''))
			{
				str.pop_back();
				c_prev = '\0';
			}
			else
			{
				c_prev = c;
			}

			str += c;
			c = parser.Next();
		}

		parser.Emit(Instruction::Literal, Variant(str));
	}

	static String VariableExpression(DataParser& parser, const String& address_prefix)
	{
		if (parser.Look() == '[')
		{
			parser.Next();
			String prefix = address_prefix;

			prefix.push_back('[');

			// Backup program state before trying to parse number inside brackets.
			// Could turn out to be expression and needs reparsing
			auto backup_state = parser.GetProgramState();

			String index = FindNumberLiteral(parser, true);
			if (!index.empty() && parser.Look() == ']')
			{
				parser.Next();
				prefix.append(index);
				prefix.push_back(']');
				return VariableExpression(parser, prefix);
			}
			else
			{
				parser.SetProgramState(backup_state);

				parser.Emit(Instruction::Literal, Variant(prefix));
				parser.Push();

				Expression(parser);
				parser.Emit(Instruction::CastToInt);
				parser.Push();

				parser.Match(']');
				VariableExpression(parser, "]");

				parser.Pop(Register::L);
				parser.Emit(Instruction::Add, Variant());
				parser.Pop(Register::L);
				parser.Emit(Instruction::Add, Variant());

				return "";
			}
		}
		else if (parser.Look() == '.')
		{
			parser.Next();
			return VariableExpression(parser, address_prefix + ".");
		}
		else
		{
			String next = VariableOrFunctionName(parser, nullptr);
			if (next.empty())
			{
				if (!address_prefix.empty())
					parser.Emit(Instruction::Literal, Variant(address_prefix));
				return address_prefix;
			}
			return VariableExpression(parser, address_prefix + next);
		}
	}

	static void VariableOrFunction(DataParser& parser)
	{
		bool valid_function_name = true;
		String name = VariableOrFunctionName(parser, &valid_function_name);
		if (name.empty())
		{
			parser.Error("Expected a variable or function name but got an empty name.");
			return;
		}

		// Keywords are parsed like variables, but are really literals. Check for them here.
		if (name == "true")
			parser.Emit(Instruction::Literal, Variant(true));
		else if (name == "false")
			parser.Emit(Instruction::Literal, Variant(false));
		else if (parser.Look() == '(')
		{
			if (!valid_function_name)
			{
				parser.Error("Invalid function name '" + name + "'.");
				return;
			}
			Function(parser, Instruction::TransformFnc, std::move(name), false);
		}
		else if (parser.Look() == '[')
		{
			// Backup program state before trying to parse part inside brackets.
			// Could turn out to be expression and needs reparsing
			auto backup_state = parser.GetProgramState();

			String full_address = VariableExpression(parser, name);

			if (!full_address.empty())
			{
				parser.SetProgramState(backup_state);
				parser.Variable(full_address);
			}
			else
			{
				// add the root of a variable expression as dependency into the address list
				parser.AddVariableAddress(name);

				parser.Emit(Instruction::DynamicVariable, Variant());
			}
		}
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
		Relational(parser);
		parser.Pop(Register::L);
		parser.Emit(Instruction::Or);
	}
	static void And(DataParser& parser)
	{
		parser.Match('&', false);
		parser.Match('&');
		parser.Push();
		Relational(parser);
		parser.Pop(Register::L);
		parser.Emit(Instruction::And);
	}
	static void Less(DataParser& parser)
	{
		Instruction instruction = Instruction::Less;
		parser.Match('<', false);
		if (parser.Look() == '=')
		{
			parser.Match('=');
			instruction = Instruction::LessEq;
		}
		else
		{
			parser.SkipWhitespace();
		}
		parser.Push();
		Additive(parser);
		parser.Pop(Register::L);
		parser.Emit(instruction);
	}
	static void Greater(DataParser& parser)
	{
		Instruction instruction = Instruction::Greater;
		parser.Match('>', false);
		if (parser.Look() == '=')
		{
			parser.Match('=');
			instruction = Instruction::GreaterEq;
		}
		else
		{
			parser.SkipWhitespace();
		}
		parser.Push();
		Additive(parser);
		parser.Pop(Register::L);
		parser.Emit(instruction);
	}
	static void Equal(DataParser& parser)
	{
		parser.Match('=', false);
		parser.Match('=');
		parser.Push();
		Additive(parser);
		parser.Pop(Register::L);
		parser.Emit(Instruction::Equal);
	}
	static void NotEqual(DataParser& parser)
	{
		parser.Match('!', false);
		parser.Match('=');
		parser.Push();
		Additive(parser);
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
	static void Function(DataParser& parser, Instruction function_type, String&& func_name, bool first_argument_piped)
	{
		RMLUI_ASSERT(function_type == Instruction::TransformFnc || function_type == Instruction::EventFnc);

		// We already matched the variable name, and also pushed the first argument to the stack if it was piped using '|'.
		int num_arguments = first_argument_piped ? 1 : 0;
		if (parser.Look() == '(')
		{
			bool looping = true;

			parser.Match('(');
			if (parser.Look() == ')')
			{
				parser.Match(')');
				looping = false;
			}

			while (looping)
			{
				num_arguments += 1;
				Expression(parser);
				parser.Push();

				switch (parser.Look())
				{
				case ')':
					parser.Match(')');
					looping = false;
					break;
				case ',': parser.Match(','); break;
				default:
					parser.Expected("one of ')' or ','");
					looping = false;
					break;
				}
			}
		}
		else
		{
			parser.SkipWhitespace();
		}

		parser.Function(function_type, num_arguments, std::move(func_name));
	}

} // namespace Parse

static String DumpProgram(const Program& program)
{
	String str;
	for (size_t i = 0; i < program.size(); i++)
	{
		String instruction_str = program[i].data.Get<String>();
		str += CreateString("  %4zu  '%c'  %s\n", i, char(program[i].instruction), instruction_str.c_str());
	}
	return str;
}

class DataInterpreter {
public:
	DataInterpreter(const Program& program, const AddressList& addresses, DataExpressionInterface expression_interface) :
		program(program), addresses(addresses), expression_interface(expression_interface)
	{}

	bool Error(const String& message) const
	{
		Log::Message(Log::LT_WARNING, "Error during execution. %s", message.c_str());
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

		if (success && !stack.empty())
			Log::Message(Log::LT_WARNING, "Possible data interpreter stack corruption. Stack size is %zu at end of execution (should be zero).",
				stack.size());

		if (!success)
		{
			String program_str = DumpProgram(program);
			Log::Message(Log::LT_WARNING, "Failed to execute program with %zu instructions:", program.size());
			Log::Message(Log::LT_WARNING, "%s", program_str.c_str());
		}

		return success;
	}

	Variant Result() const { return R; }

private:
	Variant R, L, C;
	Vector<Variant> stack;

	const Program& program;
	const AddressList& addresses;
	DataExpressionInterface expression_interface;

	bool Execute(const Instruction instruction, const Variant& data)
	{
		auto AnyString = [](const Variant& v1, const Variant& v2) { return v1.GetType() == Variant::STRING || v2.GetType() == Variant::STRING; };

		switch (instruction)
		{
		case Instruction::Push:
		{
			stack.push_back(std::move(R));
			R.Clear();
		}
		break;
		case Instruction::Pop:
		{
			if (stack.empty())
				return Error("Cannot pop stack, it is empty.");

			Register reg = Register(data.Get<int>(-1));
			switch (reg)
			{
				// clang-format off
			case Register::R:  R = stack.back(); stack.pop_back(); break;
			case Register::L:  L = stack.back(); stack.pop_back(); break;
			case Register::C:  C = stack.back(); stack.pop_back(); break;
				// clang-format on
			default: return Error(CreateString("Invalid register %d.", int(reg)));
			}
		}
		break;
		case Instruction::Literal:
		{
			R = data;
		}
		break;
		case Instruction::DynamicVariable:
		{
			auto str = R.Get<String>();
			auto address = expression_interface.ParseAddress(str);
			if (address.empty())
				return Error("Variable address not found.");
			R = expression_interface.GetValue(address);
		}
		break;
		case Instruction::Variable:
		{
			size_t variable_index = size_t(data.Get<int>(-1));
			if (variable_index < addresses.size())
				R = expression_interface.GetValue(addresses[variable_index]);
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
			// clang-format off
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
			// clang-format on
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
		case Instruction::NumArguments:
		{
			const int num_arguments = data.Get<int>(-1);
			R = num_arguments;
		}
		break;
		case Instruction::TransformFnc:
		case Instruction::EventFnc:
		{
			Vector<Variant> arguments;
			if (!ExtractArgumentsFromStack(arguments))
				return false;

			const String function_name = data.Get<String>();
			const bool result = (instruction == Instruction::TransformFnc ? expression_interface.CallTransform(function_name, arguments, R)
																		  : expression_interface.EventCallback(function_name, arguments));
			if (!result)
			{
				String arguments_str;
				for (size_t i = 0; i < arguments.size(); i++)
				{
					arguments_str += arguments[i].Get<String>();
					if (i < arguments.size() - 1)
						arguments_str += ", ";
				}
				return Error(
					CreateString("Failed to execute %s: %s(%s)", instruction == Instruction::TransformFnc ? "transform function" : "event callback",
						function_name.c_str(), arguments_str.c_str()));
			}
		}
		break;
		case Instruction::Assign:
		{
			size_t variable_index = size_t(data.Get<int>(-1));
			if (variable_index < addresses.size())
			{
				if (!expression_interface.SetValue(addresses[variable_index], R))
					return Error("Could not assign to variable.");
			}
			else
				return Error("Variable address not found.");
		}
		break;
		case Instruction::CastToInt:
		{
			int tmp;
			if (!R.GetInto(tmp))
				return Error("Could not cast value to int.");
			else
				R = tmp;
		}
		break;
		default: RMLUI_ERRORMSG("Instruction not implemented."); break;
		}
		return true;
	}

	bool ExtractArgumentsFromStack(Vector<Variant>& out_arguments)
	{
		int num_arguments = R.Get<int>(-1);
		if (num_arguments < 0)
			return Error("Invalid number of arguments.");
		if (stack.size() < size_t(num_arguments))
			return Error(CreateString("Cannot pop %d arguments, stack contains only %zu elements.", num_arguments, stack.size()));

		const auto it_stack_begin_arguments = stack.end() - num_arguments;
		out_arguments.insert(out_arguments.end(), std::make_move_iterator(it_stack_begin_arguments), std::make_move_iterator(stack.end()));

		stack.erase(it_stack_begin_arguments, stack.end());
		return true;
	}
};

DataExpression::DataExpression(String expression) : expression(std::move(expression)) {}

DataExpression::~DataExpression() {}

bool DataExpression::Parse(const DataExpressionInterface& expression_interface, bool is_assignment_expression)
{
	DataParser parser(expression, expression_interface);
	if (!parser.Parse(is_assignment_expression))
		return false;

	program = parser.ReleaseProgram();
	addresses = parser.ReleaseAddresses();

	return true;
}

bool DataExpression::Run(const DataExpressionInterface& expression_interface, Variant& out_value)
{
	DataInterpreter interpreter(program, addresses, expression_interface);

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

DataExpressionInterface::DataExpressionInterface(DataModel* data_model, Element* element, Event* event) :
	data_model(data_model), element(element), event(event)
{}

DataAddress DataExpressionInterface::ParseAddress(const String& address_str) const
{
	if (address_str.size() >= 4 && address_str[0] == 'e' && address_str[1] == 'v' && address_str[2] == '.')
		return DataAddress{DataAddressEntry("ev"), DataAddressEntry(address_str.substr(3))};

	return data_model ? data_model->ResolveAddress(address_str, element) : DataAddress();
}
Variant DataExpressionInterface::GetValue(const DataAddress& address) const
{
	Variant result;
	if (event && address.size() == 2 && address.front().name == "ev")
	{
		auto& parameters = event->GetParameters();
		auto it = parameters.find(address.back().name);
		if (it != parameters.end())
			result = it->second;
	}
	else if (data_model)
	{
		data_model->GetVariableInto(address, result);
	}
	return result;
}

bool DataExpressionInterface::SetValue(const DataAddress& address, const Variant& value) const
{
	bool result = false;
	if (data_model && !address.empty())
	{
		if (DataVariable variable = data_model->GetVariable(address))
			result = variable.Set(value);

		if (result)
			data_model->DirtyVariable(address.front().name);
	}
	return result;
}

bool DataExpressionInterface::CallTransform(const String& name, const VariantList& arguments, Variant& out_result)
{
	return data_model ? data_model->CallTransform(name, arguments, out_result) : false;
}

bool DataExpressionInterface::EventCallback(const String& name, const VariantList& arguments)
{
	if (!data_model || !event)
		return false;

	const DataEventFunc* func = data_model->GetEventCallback(name);
	if (!func || !*func)
		return false;

	DataModelHandle handle(data_model);
	func->operator()(handle, *event, arguments);
	return true;
}

} // namespace Rml
