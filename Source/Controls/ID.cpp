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

#include "../../Include/Rocket/Controls/ID.h"

namespace Rocket {
namespace Controls {


// All ID values are undefined until Rocket::Controls::Initialise

namespace PropertyId {

Core::PropertyId MinRows;
Core::PropertyId TabIndex;

}

namespace EventId {

Core::EventId Columnadd;
Core::EventId Rowadd;
Core::EventId Rowchange;
Core::EventId Rowremove;
Core::EventId Rowupdate;

Core::EventId Submit;
Core::EventId Change;

Core::EventId Tabchange;

}

// Called from Rocket::Controls::Initialise (not exposed to header file)
void InitialiseIDs()
{
	{
		using namespace PropertyId;
		MinRows = Rocket::Core::CreateOrGetPropertyId("min-rows");
		TabIndex = Rocket::Core::CreateOrGetPropertyId("tab-index");
	}

	{	
		using namespace EventId;
		Columnadd = Rocket::Core::CreateOrGetEventId("columnadd");
		Rowadd = Rocket::Core::CreateOrGetEventId("rowadd");
		Rowchange = Rocket::Core::CreateOrGetEventId("rowchange");
		Rowremove = Rocket::Core::CreateOrGetEventId("rowremove");
		Rowupdate = Rocket::Core::CreateOrGetEventId("rowupdate");

		Submit = Rocket::Core::CreateOrGetEventId("submit");
		Change = Rocket::Core::CreateOrGetEventId("change");

		Tabchange = Rocket::Core::CreateOrGetEventId("tabchange");
	}
}

}
}
