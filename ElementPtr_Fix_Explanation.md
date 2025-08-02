# ElementPtr Method Calling Fix for RmlUi Lua Bindings

## Issue Summary

**GitHub Issue:** [#390 - Cannot access Element methods using ElementPtr values returned by CreateElement() and other functions](https://github.com/mikke89/RmlUi/issues/390)

**Problem:** In RmlUi's Lua bindings, `Document:CreateElement()` returns an `ElementPtr` object, but you couldn't call Element methods like `SetAttribute()`, `SetClass()`, etc. on it. This made dynamic UI creation in Lua very cumbersome.

## The Problem (Before Fix)

### What Didn't Work:
```lua
-- This was the desired workflow, but it FAILED:
local dropdown = document:GetElementById("my_select")

-- CreateElement returns ElementPtr, but we couldn't call methods on it
local option = document:CreateElement("option")

-- ❌ THESE WOULD FAIL WITH LUA ERRORS:
option:SetAttribute("value", "option1")    -- ERROR: ElementPtr has no SetAttribute method
option:SetClass("my-class", true)          -- ERROR: ElementPtr has no SetClass method  
option.inner_rml = "Option 1"              -- ERROR: ElementPtr has no inner_rml property

dropdown:AppendChild(option)
```

### Workaround Required:
```lua
-- Users had to use this cumbersome workaround:
local dropdown = document:GetElementById("my_select")

local option = document:CreateElement("option")
-- AppendChild returns Element*, not ElementPtr, so this worked:
option = dropdown:AppendChild(option)      -- Convert ElementPtr to Element*

-- ✅ NOW these worked because option is Element*, not ElementPtr:
option:SetAttribute("value", "option1")    -- Works
option:SetClass("my-class", true)          -- Works
option.inner_rml = "Option 1"              -- Works
```

### Real-World Impact:
This limitation made dynamic UI creation very difficult. For example, populating a `<select>` element with options generated from game data required the awkward AppendChild workaround for every single option.

## The Solution (After Fix)

### What Now Works:
```lua
-- ✅ The desired workflow now works perfectly:
local dropdown = document:GetElementById("my_select")

-- CreateElement returns ElementPtr, but now we CAN call methods on it!
local option = document:CreateElement("option")

-- ✅ ALL OF THESE NOW WORK:
option:SetAttribute("value", "option1")    -- Works! 
option:SetClass("my-class", true)          -- Works!
option.inner_rml = "Option 1"              -- Works!

dropdown:AppendChild(option)
```

### Advanced Example - Populating a Select Element:
```lua
-- Before: Required cumbersome workarounds
-- After: Clean, intuitive code

local dropdown = document:GetElementById("difficulty_select")

local difficulties = {"Easy", "Normal", "Hard", "Expert"}

for i, difficulty in ipairs(difficulties) do
    -- Create option (returns ElementPtr)
    local option = document:CreateElement("option")
    
    -- Set attributes and content directly on ElementPtr - this now works!
    option:SetAttribute("value", difficulty:lower())
    option:SetClass("difficulty-option", true)
    option.inner_rml = difficulty
    
    -- Add specific styling for expert mode
    if difficulty == "Expert" then
        option:SetClass("expert-mode", true)
        option:SetAttribute("data-warning", "Very difficult!")
    end
    
    dropdown:AppendChild(option)
end
```

## Technical Implementation

### The Problem at the Code Level

In RmlUi's Lua bindings, there were two separate userdata types:
- **`Element`**: Had full access to all methods (`SetAttribute`, `SetClass`, etc.) and properties
- **`ElementPtr`**: Was a separate type with NO methods or properties exposed

When `Document:CreateElement()` returned an `ElementPtr`, Lua couldn't find any methods on it because they were only registered on the `Element` metatable.

### Original Attempt (First Commit - 937df939)

The first fix attempt added basic `__index` forwarding:

```cpp
// Added ElementPtr__indexForward() - INCOMPLETE VERSION
static int ElementPtr__indexForward(lua_State* L)
{
    // stack: userdata (ElementPtr), key
    // 1) resolve Element metatable (cached in registry at "RmlUi.Element")
    luaL_getmetatable(L, "RmlUi.Element");     // +1
    if (lua_isnil(L, -1))
        return 0;                              // should never happen
    // 2) push key again & rawget – fetch method or property getter
    lua_pushvalue(L, 2);                       // duplicate key
    lua_rawget(L, -2);                         // look-up
    return 1;                                  // leave result on stack
}
```

**Why This Didn't Work:**
- ✅ **Property access worked**: `elementPtr.tag_name` returned the correct value
- ❌ **Method calls failed**: `elementPtr:SetAttribute(...)` crashed with Lua errors

**The Core Issue:** When the original function looked up a method like `SetAttribute`, it returned the raw C function. But when Lua called that function, it passed the `ElementPtr` userdata as the `self` argument. The C function expected an `Element` userdata, not an `ElementPtr` userdata, causing a type mismatch error.

### Enhanced Fix (Second Commit - 279628b4) 

The enhanced fix added **method wrapping** to solve the type mismatch:

#### 1. Added ElementPtr_methodWrapper()

```cpp
static int ElementPtr_methodWrapper(lua_State* L)
{
    // Number of arguments passed to the method (excluding self)
    int num_args = lua_gettop(L) - 1;
    
    // Extract the original self (ElementPtr userdata)
    ElementPtr* element_ptr = LuaType<ElementPtr>::check(L, 1);
    if (!element_ptr || !*element_ptr)
    {
        return luaL_error(L, "Invalid element pointer");
    }
    
    // Get the raw Element* and create a temporary Element userdata
    Element* element = element_ptr->get();
    LuaType<Element>::push(L, element, false); // Don't manage memory
    
    // Replace the original self (ElementPtr) with the temp Element userdata
    lua_replace(L, 1);
    
    // Push the original function (stored as upvalue)
    lua_pushvalue(L, lua_upvalueindex(1));
    
    // Insert the function at position 1 (before the args)
    lua_insert(L, 1);
    
    // Call the original function with the adjusted self and original args
    lua_call(L, num_args + 1, LUA_MULTRET);
    
    // Return the number of results from the call
    return lua_gettop(L);
}
```

**What this does:**
1. **Receives the call**: When `elementPtr:SetAttribute(key, value)` is called
2. **Extracts ElementPtr**: Gets the ElementPtr from position 1 (self)
3. **Creates Element userdata**: Makes a temporary Element userdata pointing to the same object
4. **Replaces self**: Swaps the ElementPtr with the Element userdata
5. **Calls original**: Invokes the real `SetAttribute` function with correct types

#### 2. Enhanced ElementPtr__indexForward()

```cpp
static int ElementPtr__indexForward(lua_State* L)
{
    // ... [previous forwarding logic using LuaType<Element>::index] ...
    
    // NEW: If the result is a C function (method), wrap it in a closure
    if (result == 1 && lua_iscfunction(L, -1))
    {
        // Create a closure that wraps the original function
        lua_pushcclosure(L, ElementPtr_methodWrapper, 1);
    }
    
    return result;
}
```

**What this does:**
1. **Forwards lookup**: Uses the proper RmlUi lookup mechanism via `LuaType<Element>::index(L)`
2. **Detects methods**: Checks if the returned value is a C function 
3. **Wraps methods**: If it's a method, wraps it in `ElementPtr_methodWrapper` closure
4. **Preserves properties**: Non-function values (like `tag_name`) pass through unchanged

### Why The Original Fix Wasn't Enough

The original fix only handled **property lookups** but failed on **method calls** because:

1. **Property Access** (`elementPtr.tag_name`):
   - Lookup returns a value directly
   - No function call involved
   - Type doesn't matter
   - ✅ **Worked with original fix**

2. **Method Calls** (`elementPtr:SetAttribute(...)`):
   - Lookup returns a C function
   - Lua then calls that function with `elementPtr` as `self`
   - C function expects `Element*` userdata, gets `ElementPtr` userdata
   - Type mismatch causes runtime error
   - ❌ **Failed with original fix**
   - ✅ **Fixed with method wrapping**

### The Complete Solution

The enhanced fix creates a **transparent proxy layer**:

```
Lua Call: elementPtr:SetAttribute("id", "test")
    ↓
1. Lua looks up "SetAttribute" on ElementPtr
    ↓
2. ElementPtr__indexForward() forwards to Element metatable
    ↓
3. Gets SetAttribute C function
    ↓
4. Wraps it in ElementPtr_methodWrapper closure
    ↓
5. Returns wrapped function to Lua
    ↓
6. Lua calls wrapped function with ElementPtr as self
    ↓
7. ElementPtr_methodWrapper converts ElementPtr → Element
    ↓
8. Calls original SetAttribute with correct Element userdata
    ↓
9. Method executes successfully!
```

This solution is:
- ✅ **Complete**: Handles both properties and methods
- ✅ **Transparent**: No visible difference from Element usage
- ✅ **Compatible**: All existing code continues working
- ✅ **Efficient**: Minimal overhead, wrapping only when needed

## Benefits

✅ **Full Backward Compatibility**: All existing Lua scripts continue to work unchanged

✅ **Transparent Operation**: ElementPtr objects now work exactly like Element objects from Lua's perspective  

✅ **Complete API Coverage**: All Element methods and properties are now available on ElementPtr

✅ **Intuitive Workflow**: Dynamic UI creation now follows the natural, expected pattern

✅ **Performance**: Minimal overhead - method wrapping only happens when needed

## Testing

The fix was tested using RmlUi's lua_invaders sample with this test case:

```lua
-- Test the fix: CreateElement returns ElementPtr, but we should be able to call methods
local element = document:CreateElement("div")
element:SetAttribute("id", "test")          -- This used to fail, now works
element:SetClass("dynamic", true)           -- This used to fail, now works

print("✓ ElementPtr method calls working! Tag:", element.tag_name, "HasAttribute:", element:HasAttribute("id"))
-- Output: ✓ ElementPtr method calls working! Tag: div HasAttribute: true
```

## Impact

This fix resolves a major usability issue that has existed since the Lua bindings were created. It enables developers to write clean, intuitive Lua code for dynamic UI creation without workarounds, making RmlUi much more developer-friendly for Lua-based projects.

The issue affected any Lua developer using RmlUi who wanted to:
- Dynamically create UI elements
- Populate select/dropdown elements  
- Generate UI from game data
- Create complex UI hierarchies programmatically

**Status**: ✅ **FIXED** - Committed and ready for merge into master branch.
