/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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
#include <Rocket/Core/StringUtilities.h>
#include <ctype.h>
#include <stdio.h>

namespace Rocket {
namespace Core {

StringUtilities::ArgumentState::ArgumentState()
{
	index = 1;
	display_errors = true;
}

// Expands character-delimited list of values in a single string to a whitespace-trimmed list of values.
void StringUtilities::ExpandString(StringList& string_list, const String& string, const char delimiter)
{	
	char quote = 0;
	bool last_char_delimiter = true;
	const char* ptr = string.CString();
	const char* start_ptr = NULL;
	const char* end_ptr = ptr;

	while (*ptr)
	{
		// Switch into quote mode if the last char was a delimeter ( excluding whitespace )
		// and we're not already in quote mode
		if (last_char_delimiter && !quote && (*ptr == '"' || *ptr == '\''))
		{			
			quote = *ptr;
		}
		// Switch out of quote mode if we encounter a quote that hasn't been escaped
		else if (*ptr == quote && *(ptr-1) != '\\')
		{
			quote = 0;
		}
		// If we encouter a delimiter while not in quote mode, add the item to the list
		else if (*ptr == delimiter && !quote)
		{
			if (start_ptr)
				string_list.push_back(String(start_ptr, end_ptr + 1));
			else
				string_list.push_back("");
			last_char_delimiter = true;
			start_ptr = NULL;
		}
		// Otherwise if its not white space or we're in quote mode, advance the pointers
		else if (!isspace(*ptr) || quote)
		{
			if (!start_ptr)
				start_ptr = ptr;
			end_ptr = ptr;
			last_char_delimiter = false;
		}

		ptr++;
	}

	// If there's data pending, add it.
	if (start_ptr)
		string_list.push_back(String(start_ptr, end_ptr + 1));
}

// Joins a list of string values into a single string separated by a character delimiter.
void StringUtilities::JoinString(String& string, const StringList& string_list, const char delimiter)
{
	for (size_t i = 0; i < string_list.size(); i++)
	{
		string += string_list[i];
		if (delimiter != '\0' && i < string_list.size() - 1)
			string.Append(delimiter);
	}
}

// Forward declare the MD5 function
String MD5String(const char* string, int length);

// Hashes a string of data to an 32-character MD5 value.
String StringUtilities::MD5Hash(const char* data, int length)
{
	return MD5String(data, length);
}

// Hashes a string of data to an integer value using the FNV algorithm.
Hash StringUtilities::FNVHash(const char *string, int length)
{
	// FNV-1 hash algorithm
	Hash hval = 0;
	unsigned char *bp = (unsigned char *)string;	// start of buffer
	unsigned char *be = bp + ( length < 0 ? strlen(string) : length );	// beyond end of buffer
    
	// FNV-1 hash each octet in the buffer
	while (bp < be) 
	{
		// multiply by the 32 bit FNV magic prime mod 2^32 
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);

		// xor the bottom with the current octet
		hval ^= *bp++;
	}

	return hval;
}

static unsigned char hexchars[] = "0123456789ABCDEF";

// Encodes a string with URL-encoding.
bool StringUtilities::URLEncode(const char* input, size_t input_length, String& output)
{	
	for (size_t i = 0; i < input_length; i++) 
	{		
		if (input[i] == ' ') 
		{
			output += '+';
		} 
		else if (isalnum((unsigned char)input[i]) || input[i] == '_' || input[i] == '-' || input[i] == '.')
		{
			/* Allow only alphanumeric chars and '_', '-', '.'; escape the rest */
			output += input[i];
		}
		else
		{
			output += '%';
			output += hexchars[(unsigned char) input[i] >> 4];
			output += hexchars[(unsigned char) input[i] & 0x0F];
		}
	}

	return true;
}

// Decodes a URL-encoded string.
bool StringUtilities::URLDecode(const String& input, char* output, size_t output_length)
{
	char* dest = output;
	const char* data = input.CString();
	int len = (int) input.Length();
	size_t used = 0;

	while (len-- && used < output_length) 
	{
		if (*data == '+')
		{
			*dest = ' ';
		}
		else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) && isxdigit((int) *(data + 2))) 
		{
			int value;
			int c;

			c = ((unsigned char*) data)[1];
			if (isupper(c))
				c = tolower(c);
			value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

			c = ((unsigned char*) data)[2];
			if (isupper(c))
				c = tolower(c);
			value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

			data += 2;
			len -= 2;

			*dest = (char) value;
		} 
		else
		{
			*dest = *data;
		}

		data++;
		dest++;
		used++;
	}

	return true;
}



static const char base64digits[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define BAD	-1
static const char base64val[] = {
    BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,
    BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,
    BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD, 62, BAD,BAD,BAD, 63,
     52, 53, 54, 55,  56, 57, 58, 59,  60, 61,BAD,BAD, BAD,BAD,BAD,BAD,
    BAD,  0,  1,  2,   3,  4,  5,  6,   7,  8,  9, 10,  11, 12, 13, 14,
     15, 16, 17, 18,  19, 20, 21, 22,  23, 24, 25,BAD, BAD,BAD,BAD,BAD,
    BAD, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, 36,  37, 38, 39, 40,
     41, 42, 43, 44,  45, 46, 47, 48,  49, 50, 51,BAD, BAD,BAD,BAD,BAD
};
#define DECODE64(c)  (isascii(c) ? base64val[c] : BAD)

// Encodes a string with base64-encoding.
bool StringUtilities::Base64Encode( const char* input, size_t input_length, String& output )
{
	output.Clear();

	size_t i = input_length;
	for (; i >= 3; i -= 3)
	{
		output += base64digits[input[0] >> 2];
		output += base64digits[((input[0] << 4) & 0x30) | (input[1] >> 4)];
		output += base64digits[((input[1] << 2) & 0x3c) | (input[2] >> 6)];
		output += base64digits[input[2] & 0x3f];
		input += 3;
	}

    if (i > 0)
    {
		unsigned char fragment;

		output += base64digits[input[0] >> 2];
		fragment = (input[0] << 4) & 0x30;
		if (i > 1)
			fragment |= input[1] >> 4;
		output += base64digits[fragment];
		output += (i < 2) ? '=' : base64digits[(input[1] << 2) & 0x3c];
		output += '=';
	}

	return true;
}

// Decodes a base64-encoded string.
bool StringUtilities::Base64Decode( const String& input, char* output, size_t output_length )
{
	const char* in = input.CString();

	size_t len = 0;
	unsigned char digit1, digit2, digit3, digit4;

    if (in[0] == '+' && in[1] == ' ')
		in += 2;
    if (*in == '\r')
		return false;

    do
	{
		digit1 = in[0];
		if (DECODE64(digit1) == BAD)
			return false;
		digit2 = in[1];
		if (DECODE64(digit2) == BAD)
			return false;
		digit3 = in[2];
		if (digit3 != '=' && DECODE64(digit3) == BAD)
			return false;
		digit4 = in[3];
		if (digit4 != '=' && DECODE64(digit4) == BAD)
			return false;
		in += 4;
		++len;
		if (output_length && len > output_length)
			return false;

		*output++ = (DECODE64(digit1) << 2) | (DECODE64(digit2) >> 4);
		if (digit3 != '=')
		{
			++len;
			if (output_length && len > output_length)
				return false;

			*output++ = ((DECODE64(digit2) << 4) & 0xf0) | (DECODE64(digit3) >> 2);
		    if (digit4 != '=')
		    {
			    ++len;
				if (output_length && len > output_length)
					return false;

				*output++ = ((DECODE64(digit3) << 6) & 0xc0) | DECODE64(digit4);
			}
		}
    }
	while (*in && *in != '\r' && digit4 != '=');

    return true;
}



// Defines, helper functions for the UTF8 / UCS2 conversion functions.
#define _NXT	0x80
#define _SEQ2	0xc0
#define _SEQ3	0xe0
#define _SEQ4	0xf0
#define _SEQ5	0xf8
#define _SEQ6	0xfc

#define _BOM	0xfeff

static int __wchar_forbidden(unsigned int sym)
{
	// Surrogate pairs
	if (sym >= 0xd800 && sym <= 0xdfff)
		return -1;

	return 0;
}

static int __utf8_forbidden(unsigned char octet)
{
	switch (octet)
	{
		case 0xc0:
		case 0xc1:
		case 0xf5:
		case 0xff:
			return -1;

		default:
			return 0;
	}
}



// Converts a character array in UTF-8 encoding to a vector of words.
bool StringUtilities::UTF8toUCS2(const String& input, std::vector< word >& output)
{
	if (input.Empty())
		return true;

	unsigned char* p = (unsigned char*) input.CString();
	unsigned char* lim = p + input.Length();

	// Skip the UTF-8 byte order marker if it exists.
	if (input.Substring(0, 3) == "\xEF\xBB\xBF")
		p += 3;

	int num_bytes;
	for (; p < lim; p += num_bytes)
	{
		if (__utf8_forbidden(*p) != 0)
			return false;

		// Get number of bytes for one wide character.
		word high;
		num_bytes = 1;

		if ((*p & 0x80) == 0)
		{
			high = (wchar_t)*p;
		}
		else if ((*p & 0xe0) == _SEQ2)
		{
			num_bytes = 2;
			high = (wchar_t)(*p & 0x1f);
		}
		else if ((*p & 0xf0) == _SEQ3)
		{
			num_bytes = 3;
			high = (wchar_t)(*p & 0x0f);
		}
		else if ((*p & 0xf8) == _SEQ4)
		{
			num_bytes = 4;
			high = (wchar_t)(*p & 0x07);
		}
		else if ((*p & 0xfc) == _SEQ5)
		{
			num_bytes = 5;
			high = (wchar_t)(*p & 0x03);
		}
		else if ((*p & 0xfe) == _SEQ6)
		{
			num_bytes = 6;
			high = (wchar_t)(*p & 0x01);
		}
		else
		{
			return false;
		}

		// Does the sequence header tell us the truth about length?
		if (lim - p <= num_bytes - 1)
		{
			return false;
		}

		// Validate the sequence. All symbols must have higher bits set to 10xxxxxx.
		if (num_bytes > 1)
		{
			int i;
			for (i = 1; i < num_bytes; i++)
			{
				if ((p[i] & 0xc0) != _NXT)
					break;
			}

			if (i != num_bytes)
			{
				return false;
			}
		}

		// Make up a single UCS-4 (32-bit) character from the required number of UTF-8 tokens. The first byte has
		// been determined earlier, the second and subsequent bytes contribute the first six of their bits into the
		// final character code.
		unsigned int ucs4_char = 0;
		int num_bits = 0;
		for (int i = 1; i < num_bytes; i++)
		{
			ucs4_char |= (word)(p[num_bytes - i] & 0x3f) << num_bits;
			num_bits += 6;
		}
		ucs4_char |= high << num_bits;

		// Check for surrogate pairs.
		if (__wchar_forbidden(ucs4_char) != 0)
		{
			return false;
		}

		// Only add the character to the output if it exists in the Basic Multilingual Plane (ie, fits in a single
		// word).
		if (ucs4_char <= 0xffff)
			output.push_back((word) ucs4_char);
	}

	output.push_back(0);
	return true;
}

// Converts a vector of words in UCS-2 encoding a character array in UTF-8 encoding.
bool StringUtilities::UCS2toUTF8(const std::vector< word >& input, String& output)
{
	return UCS2toUTF8(&input[0], input.size(), output);
}

// Converts an array of words in UCS-2 encoding into a character array in UTF-8 encoding.
bool StringUtilities::UCS2toUTF8(const word* input, size_t input_size, String& output)
{
	unsigned char *oc;
	size_t n;

	word* w = (word*) input;
	word* wlim = w + input_size;

	//Log::Message(LC_CORE, Log::LT_ALWAYS, "UCS2TOUTF8 size: %d", input_size);
	for (; w < wlim; w++)
	{
		if (__wchar_forbidden(*w) != 0)
			return false;

		if (*w == _BOM)
			continue;

		//if (*w < 0)
		//	return false;
		if (*w <= 0x007f)
			n = 1;
		else if (*w <= 0x07ff)
			n = 2;
		else //if (*w <= 0x0000ffff)
			n = 3;
		/*else if (*w <= 0x001fffff)
			n = 4;
		else if (*w <= 0x03ffffff)
			n = 5;
		else // if (*w <= 0x7fffffff)
			n = 6;*/

		// Convert to little endian.
		word ch = (*w >> 8) & 0x00FF;
		ch |= (*w << 8) & 0xFF00;
//		word ch = EMPConvertEndian(*w, ROCKET_ENDIAN_BIG);

		oc = (unsigned char *)&ch;
		switch (n)
		{
			case 1:
				output += oc[1];
				break;

			case 2:
				output += (_SEQ2 | (oc[1] >> 6) | ((oc[0] & 0x07) << 2));
				output += (_NXT | oc[1] & 0x3f);
				break;

			case 3:
				output += (_SEQ3 | ((oc[0] & 0xf0) >> 4));
				output += (_NXT | (oc[1] >> 6) | ((oc[0] & 0x0f) << 2));
				output += (_NXT | oc[1] & 0x3f);
				break;

			case 4:
				break;

			case 5:
				break;

			case 6:
				break;
		}

		//Log::Message(LC_CORE, Log::LT_ALWAYS, "Converting...%c(%d) %d -> %d", *w, *w, w - input, output.Length());
	}

	return true;
}

// Strip whitespace characters from the beginning and end of a string.
String StringUtilities::StripWhitespace(const String& string)
{
	const char* start = string.CString();
	const char* end = start + string.Length();

	while (start < end && IsWhitespace(*start))
		start++;

	while (end > start && IsWhitespace(*(end - 1)))
		end--;

	if (start < end)
		return String(start, end);

	return String();
}



////////////////////////////////////////////////////////////////////////////
// GetOpt - Public Domain Software
////////////////////////////////////////////////////////////////////////////

/* transcript/src/getopt.c
 *
 * public domain getopt from mod.sources
 * RCSID: $Header: getopt.c,v 2.1 85/11/24 11:49:10 shore Rel $
 */

/*
**  This is a public domain version of getopt(3).
**  Bugs, fixes to:
**		Keith Bostic
**			ARPA: keith@seismo
**			UUCP: seismo!keith
**  Added NO_STDIO, opterr handling, Rich $alz (mirror!rs).
*/

/*
**  Error macro.  Maybe we want stdio, maybe we don't.
**  The (undocumented?) variable opterr tells us whether or not
**  to print errors.
*/

#define tell(s) \
	if (arg_state.display_errors) \
    { \
	    (void)fputs(*nargv, stderr); \
	    (void)fputs(s,stderr); \
	    (void)fputc(arg_state.option, stderr); \
	    (void)fputc('\n', stderr); \
	}

/* Global variables. */
static char	 EMSG[] = "";

int StringUtilities::GetOpt( int nargc, char* nargv[], char* ostr, ArgumentState& arg_state )
{
    static char		 *place = EMSG;	/* option letter processing	*/
    register char	 *oli;		/* option letter list index	*/

    if (!*place)			/* update scanning pointer	*/
    {
		if (arg_state.index >= nargc || *(place = nargv[arg_state.index]) != '-' || !*++place)
			return(EOF);
		if (*place == '-')		/* found "--"			*/
		{
			arg_state.index++;
			return(EOF);
		}
    }
    /* option letter okay? */
    if ((arg_state.option = *place++) == ':' || (oli = strchr(ostr, arg_state.option)) == NULL)
    {
		if (!*place)
			arg_state.index++;
		tell(": illegal option -- ");
		goto Bad;
    }
	if (*++oli != ':')			/* don't need argument		*/
	{
		arg_state.argument = NULL;
		if (!*place)
			arg_state.index++;
    }
    else				/* need an argument		*/
    {
		if (*place)
			arg_state.argument = place;		/* no white space		*/
		else
			if (nargc <= ++arg_state.index)
			{
				place = EMSG;
				tell(": option requires an argument -- ");
				goto Bad;
			}
			else
				arg_state.argument = nargv[arg_state.index];	/* white space			*/
		place = EMSG;
		arg_state.index++;
    }
    return(arg_state.option);			/* dump back option letter	*/
Bad:
    return('?');
}


////////////////////////////////////////////////////////////////////////////
// MD5 Algorithm
////////////////////////////////////////////////////////////////////////////


/* This function is the RSA Data Security, Inc. MD5 Message-Digest Algorithm

BE WARNED: This code is ripped straight from the RFC, and is very ugly. Read at your own peril.
The only function that is human written is the last one, right at the bottom of this file */


// POINTER defines a generic pointer type
typedef unsigned char *POINTER;

// UINT2 defines a two byte word
typedef unsigned short int UINT2;

// UINT4 defines a four byte word
typedef unsigned long int UINT4;

#define PROTO_LIST(list) list

typedef struct
{
  UINT4 state[4];                                   // state (ABCD)
  UINT4 count[2];        // number of bits, modulo 2^64 (lsb first)
  unsigned char buffer[64];                         // input buffer
} MD5_CTX;

void MD5Init PROTO_LIST ((MD5_CTX *));
void MD5Update PROTO_LIST ((MD5_CTX *, unsigned char *, unsigned int));
void MD5Final PROTO_LIST ((unsigned char [16], MD5_CTX *));


// Constants for MD5Transform routine.

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

void MD5Transform PROTO_LIST ((UINT4 [4], unsigned char [64]));
void Encode PROTO_LIST ((unsigned char *, UINT4 *, unsigned int));
void Decode PROTO_LIST ((UINT4 *, unsigned char *, unsigned int));
void MD5_memcpy PROTO_LIST ((POINTER, POINTER, unsigned int));
void MD5_memset PROTO_LIST ((POINTER, int, unsigned int));

static unsigned char MD5_PADDING[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// F, G, H and I are basic MD5 functions.
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

// ROTATE_LEFT rotates x left n bits.
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
Rotation is separate from addition to prevent recomputation. */
#define FF(a, b, c, d, x, s, ac) \
{ \
	(a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
}
#define GG(a, b, c, d, x, s, ac) \
{ \
	(a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
}
#define HH(a, b, c, d, x, s, ac) \
{ \
	(a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
}
#define II(a, b, c, d, x, s, ac) \
{ \
	(a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
}

// MD5 initialization. Begins an MD5 operation, writing a new context.
void MD5Init (MD5_CTX *context)
{
	context->count[0] = context->count[1] = 0;
	/* Load magic initialization constants.*/
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
}

/* MD5 block update operation. Continues an MD5 message-digest
  operation, processing another message block, and updating the
  context. */
void MD5Update (MD5_CTX *context, unsigned char *input, unsigned int inputLen)
{
	unsigned int i, index, partLen;

	// Compute number of bytes mod 64
	index = (unsigned int)((context->count[0] >> 3) & 0x3F);

	// Update number of bits
	if ((context->count[0] += ((UINT4)inputLen << 3)) < ((UINT4)inputLen << 3))
		context->count[1]++;

	context->count[1] += ((UINT4)inputLen >> 29);

	partLen = 64 - index;

	// Transform as many times as possible.
	if (inputLen >= partLen)
	{
		MD5_memcpy ((POINTER)&context->buffer[index], (POINTER)input, partLen);
		MD5Transform (context->state, context->buffer);

		for (i = partLen; i + 63 < inputLen; i += 64)
			MD5Transform (context->state, &input[i]);

		index = 0;
	}
	else
		i = 0;

	// Buffer remaining input
	MD5_memcpy ((POINTER)&context->buffer[index], (POINTER)&input[i],
	inputLen-i);
}

/* MD5 finalization. Ends an MD5 message-digest operation, writing the
  the message digest and zeroizing the context. */
void MD5Final (unsigned char digest[16], MD5_CTX *context)
{
	unsigned char bits[8];
	unsigned int index, padLen;

	// Save number of bits
	Encode (bits, context->count, 8);

	// Pad out to 56 mod 64.
	index = (unsigned int)((context->count[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	MD5Update (context, MD5_PADDING, padLen);

	// Append length (before padding)
	MD5Update (context, bits, 8);

	// Store state in digest
	Encode (digest, context->state, 16);

	// Zeroize sensitive information.
	MD5_memset ((POINTER)context, 0, sizeof (*context));
}

// MD5 basic transformation. Transforms state based on block.
void MD5Transform (UINT4 state[4], unsigned char block[64])
{
	UINT4 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

	Decode (x, block, 64);

	// Round 1
	FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
	FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
	FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
	FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
	FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
	FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
	FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
	FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
	FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
	FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
	FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
	FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
	FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
	FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
	FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
	FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

	// Round 2
	GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
	GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
	GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
	GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
	GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
	GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
	GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
	GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
	GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
	GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
	GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
	GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
	GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
	GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
	GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
	GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

	// Round 3
	HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
	HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
	HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
	HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
	HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
	HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
	HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
	HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
	HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
	HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
	HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
	HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
	HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
	HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
	HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
	HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

	// Round 4
	II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
	II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
	II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
	II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
	II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
	II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
	II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
	II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
	II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
	II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
	II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
	II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
	II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
	II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
	II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
	II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	// Zeroize sensitive information.
	MD5_memset ((POINTER)x, 0, sizeof (x));
}

// Encodes input (UINT4) into output (unsigned char). Assumes len is a multiple of 4.
void Encode (unsigned char *output, UINT4 *input, unsigned int len)
{
	unsigned int i, j;

	for (i = 0, j = 0; j < len; i++, j += 4)
	{
		output[j] = (unsigned char)(input[i] & 0xff);
		output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
		output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
		output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
	}
}

// Decodes input (unsigned char) into output (UINT4). Assumes len is a multiple of 4.
void Decode (UINT4 *output, unsigned char *input, unsigned int len)
{
	unsigned int i, j;

	for (i = 0, j = 0; j < len; i++, j += 4)
		output[i] = ((UINT4)input[j]) | (((UINT4)input[j+1]) << 8) | (((UINT4)input[j+2]) << 16) | (((UINT4)input[j+3]) << 24);
}

// Note: Replace "for loop" with standard memcpy if possible.

void MD5_memcpy (POINTER output, POINTER input, unsigned int len)
{
	unsigned int i;

	for (i = 0; i < len; i++)
		output[i] = input[i];
}

// Note: Replace "for loop" with standard memset if possible.
void MD5_memset (POINTER output, int value, unsigned int len)
{
	unsigned int i;

	for (i = 0; i < len; i++)
		((char *)output)[i] = (char)value;
}

// Length of test block, number of test blocks.
#define TEST_BLOCK_LEN 1000
#define TEST_BLOCK_COUNT 1000

void MDString PROTO_LIST ((char *));
void MDTimeTrial PROTO_LIST ((void));
void MDTestSuite PROTO_LIST ((void));
void MDFile PROTO_LIST ((char *));
void MDFilter PROTO_LIST ((void));
String MDPrint PROTO_LIST ((unsigned char [16]));

// Digests a string and returns the result.
String MD5String (const char *string, int length)
{
	MD5_CTX context;
	unsigned char digest[16];
	unsigned int len;
	if (length < 0)
		len = (unsigned int) strlen (string);
	else
		len = (unsigned int) length;

	MD5Init (&context);
	MD5Update (&context, (unsigned char*)string, len);
	MD5Final (digest, &context);

	return MDPrint(digest);
}

// Prints a message digest in hexadecimal.
String MDPrint (unsigned char digest[16])
{
	char hex_digest[33];

	for (unsigned int i = 0; i < 16; i++)
		sprintf(&(hex_digest[i * 2]), "%02x", digest[i]);

	return String(hex_digest);
}



// Operators for STL containers using strings.
bool StringUtilities::StringComparei::operator()(const String& lhs, const String& rhs) const
{
	return strcasecmp(lhs.CString(), rhs.CString()) < 0;
}

}
}
