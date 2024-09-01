#pragma once

#include "FINLua/LuaUtil.h"
#include "Reflection/FIRClass.h"

class UFINKernelSystem;

namespace FINLua {
	/**
	 * Structure used in the userdata representing a instance.
	 */
	struct FLuaObject {
		UFIRClass* Type;
		FFIRTrace Object;
		UFINKernelSystem* Kernel;
		FLuaObject(const FFIRTrace& Trace, UFINKernelSystem* Kernel);
		FLuaObject(const FLuaObject& Other);
		~FLuaObject();
		static void CollectReferences(void* Obj, FReferenceCollector& Collector);
	};

	/**
	 * @brief Pushes the given trace/object on-top of the lua stack.
	 * @param L the lua state
	 * @param Object the object you want to push
	 */
	void luaFIN_pushObject(lua_State* L, const FFIRTrace& Object);

	/**
	 * @brief Tries to retrieve a Lua Object from the lua value at the given index in the lua stack.
	 * @param L the lua state
	 * @param Index the index of the lua value you try to get as Lua Object
	 * @param ParentType if not nullptr, used to check the type of the Lua Object, causes nullptr return if mismatch
	 * @return the pointer to the lua object in the lua stack (attention to GC!), nullptr if type check failed
	 */
	FLuaObject* luaFIN_toLuaObject(lua_State* L, int Index, UFIRClass* ParentType);

	/**
	 * @brief Tries to retrieve a Lua Object from the lua value at the given index in the lua stack. Causes a lua error if failed to get from lua value or type mismatch
	 * @param L the lua state
	 * @param Index the index of the lua value you try to get as Lua Object
	 * @param ParentType if not nullptr, used to check the type of the Lua Object, causes lua error if mismatch
	 * @return the pointer to the lua object in the lua stack (attention to GC!)
	 */
	FLuaObject* luaFIN_checkLuaObject(lua_State* L, int Index, UFIRClass* ParentType);

	/**
	 * @brief Tries to retrieve a Object from the lua value at the given index in the lua stack.
	 * @param L the lua state
	 * @param Index the index of the lua value you try to get as Object
	 * @param ParentType if not nullptr, used to check the type of the Object, causes None return if mismatch
	 * @return the trace/object of the lua object in the lua stack, None if type check failed
	 */
	TOptional<FFIRTrace> luaFIN_toObject(lua_State* L, int Index, UFIRClass* ParentType);
	
	/**
	 * @brief Tries to retrieve a Object from the lua value at the given index in the lua stack.
	 * @param L the lua state
	 * @param Index the index of the lua value you try to get as Object
	 * @param ParentType if not nullptr, used to check the type of the Object, causes None return if mismatch
	 * @return the trace/object of the lua object in the lua stack, None if type check failed
	 */
	FFIRTrace luaFIN_checkObject(lua_State* L, int Index, UFIRClass* ParentType);

	/**
	 * @return The Lua Metatable/Type-Name of Object
	 */
	FString luaFIN_getLuaObjectTypeName();
}