#include "LuaProcessor/LuaInstance.h"
#include "LuaProcessor/LuaProcessor.h"
#include "LuaProcessor/LuaProcessorStateStorage.h"
#include "LuaProcessor/LuaRef.h"
#include "Network/FINNetworkComponent.h"
#include "Network/FINNetworkUtils.h"
#include "Reflection/FINClass.h"
#include "Reflection/FINReflection.h"
#include <map>

#define INSTANCE_TYPE "InstanceType"

#define OffsetParam(type, off) (type*)((std::uint64_t)param + off)

namespace FINLua {
	std::map<UObject*, FCriticalSection> objectLocks;
	TMap<UFINClass*, FString> ClassToMetaName;
	TMap<FString, UFINClass*> MetaNameToClass;
	FCriticalSection ClassMetaNameLock;
	
	void luaInstanceType(lua_State* L, LuaInstanceType&& instanceType);
	int luaInstanceTypeUnpersist(lua_State* L) {
		UFINLuaProcessor* Processor = UFINLuaProcessor::luaGetProcessor(L);
		
		// get persist storage
		FFINLuaProcessorStateStorage& storage = Processor->StateStorage;
		
		luaInstanceType(L, LuaInstanceType{Cast<UClass>(storage.GetRef(luaL_checkinteger(L, lua_upvalueindex(1))))});

		return 1;
	}

	int luaInstanceTypePersist(lua_State* L) {
		LuaInstanceType* type = static_cast<LuaInstanceType*>(luaL_checkudata(L, 1, INSTANCE_TYPE));
		
		UFINLuaProcessor* Processor = UFINLuaProcessor::luaGetProcessor(L);
		
		// get persist storage
		FFINLuaProcessorStateStorage& storage = Processor->StateStorage;

		lua_pushinteger(L, storage.Add(type->type));
		
		// create & return closure
		lua_pushcclosure(L, &luaInstanceTypeUnpersist, 1);
		return 1;
	}

	int luaInstanceTypeGC(lua_State* L) {
		LuaInstanceType* type = static_cast<LuaInstanceType*>(luaL_checkudata(L, 1, INSTANCE_TYPE));
		type->~LuaInstanceType();
		return 0;
	}

	static const luaL_Reg luaInstanceTypeLib[] = {
		{"__persist", luaInstanceTypePersist},
		{"__gc", luaInstanceTypeGC},
		{nullptr, nullptr}
	};

	void luaInstanceType(lua_State* L, LuaInstanceType&& instanceType) {
		LuaInstanceType* type = static_cast<LuaInstanceType*>(lua_newuserdata(L, sizeof(LuaInstanceType)));
		new (type) LuaInstanceType(std::move(instanceType));
		luaL_setmetatable(L, INSTANCE_TYPE);
	}

	LuaInstance* GetInstance(lua_State* L, int Index, UFINClass** OutClass = nullptr) {
		Index = lua_absindex(L, Index);
		FString TypeName;
		if (luaL_getmetafield(L, Index, "__name") == LUA_TSTRING) {
			TypeName = lua_tostring(L, -1);
			lua_pop(L, 1);
		} else if (lua_type(L, Index) == LUA_TLIGHTUSERDATA) {
			TypeName = "light userdata";
		} else {
			TypeName = luaL_typename(L, Index);
		}
		if (OutClass) {
			ClassMetaNameLock.Lock();
			UFINClass** Class = MetaNameToClass.Find(TypeName);
			if (!Class) {
				ClassMetaNameLock.Unlock();
				luaL_argerror(L, Index, "Instance is invalid type");
			}
			*OutClass = *Class;
			ClassMetaNameLock.Unlock();
		}
		return static_cast<LuaInstance*>(lua_touserdata(L, Index));
	}

	LuaInstance* CheckAndGetInstance(lua_State* L, int Index, UFINClass** OutClass = nullptr) {
		LuaInstance* Instance = GetInstance(L, Index, OutClass);
		if (!Instance) luaL_argerror(L, Index, "Instance is invalid");
		return Instance;
	}

	int luaInstanceFuncCall(lua_State* L) {		// Instance, args..., up: UFINFunction
		// get function
		UFINFunction* Function = luaFIN_checkReflectionFunction(L, lua_upvalueindex(1));
		UFINStruct* Type = Function->GetTypedOuter<UFINStruct>();
		
		// get and check instance
		ClassMetaNameLock.Lock();
		FString* MetaNamePtr = ClassToMetaName.Find(Cast<UFINClass>(Type));
		if (!MetaNamePtr) {
			ClassMetaNameLock.Unlock();
			return luaL_error(L, "Function name is invalid (internal error)");
		}
		const FString MetaName = *MetaNamePtr;
		ClassMetaNameLock.Unlock();
		LuaInstance* Instance = static_cast<LuaInstance*>(luaL_checkudata(L, 1, TCHAR_TO_UTF8(*MetaName)));
		UObject* Obj = *Instance->Trace;
		if (!Obj) return luaL_argerror(L, 1, "Instance is invalid");
		
		// call func
		//return luaCallFINFunc(L, Function, FFINExecutionContext(Instance->Trace), "Instance");
		return 0;
	}

	int luaInstanceIndex(lua_State* L) {																			// Instance, FuncName
		// get instance
		UFINClass* Class;
		LuaInstance* Instance = CheckAndGetInstance(L, 1, &Class);
		check(Instance != nullptr);
		
		// get member name
		const FString MemberName = lua_tostring(L, 2);
		
		UObject* Obj = *Instance->Trace;
		UObject* NetworkHandler = UFINNetworkUtils::FindNetworkComponentFromObject(Obj);

		if (!IsValid(Obj)) {
			return luaL_error(L, "Instance is invalid");
		}
		
		// check for network component stuff
		if (NetworkHandler) {
			if (MemberName == "id") {
				lua_pushstring(L, TCHAR_TO_UTF8(*IFINNetworkComponent::Execute_GetID(NetworkHandler).ToString()));
				return UFINLuaProcessor::luaAPIReturn(L, 1);
			}
			if (MemberName == "nick") {
				lua_pushstring(L, TCHAR_TO_UTF8(*IFINNetworkComponent::Execute_GetNick(NetworkHandler)));
				return UFINLuaProcessor::luaAPIReturn(L, 1);
			}
		}

		ClassMetaNameLock.Lock();
		const FString MetaName = ClassToMetaName[Class];
		ClassMetaNameLock.Unlock();
		//return luaFindGetMember(L, Class, FFINExecutionContext(Instance->Trace), MemberName, MetaName + "_" + MemberName, &luaInstanceFuncCall, false);
		return 0;
	}

	int luaInstanceNewIndex(lua_State* L) {
		// get instance
		UFINClass* Class;
		LuaInstance* Instance = CheckAndGetInstance(L, 1, &Class);
			
		// get member name
		const FString MemberName = lua_tostring(L, 2);
		
		UObject* Obj = *Instance->Trace;
		UObject* NetworkHandler = UFINNetworkUtils::FindNetworkComponentFromObject(Obj);

		if (!IsValid(Obj)) {
			return luaL_error(L, "Instance is invalid");
		}
		
		// check for network component stuff
		if (NetworkHandler) {
			if (MemberName == "id") {
				return luaL_error(L, TCHAR_TO_UTF8(*("Property '" + MemberName + "' is read-only")));
			}
			if (MemberName == "nick") {
				const FString Nick = luaL_checkstring(L, 3);
				IFINNetworkComponent::Execute_SetNick(NetworkHandler, Nick);
				return UFINLuaProcessor::luaAPIReturn(L, 1);
			}
		}

		//return luaFindSetMember(L, Class, FFINExecutionContext(Instance->Trace), MemberName, false);
		return 0;
	}

	int luaInstanceEQ(lua_State* L) {
		LuaInstance* inst1 = CheckAndGetInstance(L, 1);
		LuaInstance* inst2 = GetInstance(L, 2);
		if (!inst1 || !inst2) {
			lua_pushboolean(L, false);
			return UFINLuaProcessor::luaAPIReturn(L, 1);
		}
		
		lua_pushboolean(L, inst1->Trace.IsEqualObj(inst2->Trace));
		return UFINLuaProcessor::luaAPIReturn(L, 1);
	}

	int luaInstanceLt(lua_State* L) {
		LuaInstance* inst1 = CheckAndGetInstance(L, 1);
		LuaInstance* inst2 = GetInstance(L, 2);
		if (!inst1 || !inst2) {
			lua_pushboolean(L, false);
			return UFINLuaProcessor::luaAPIReturn(L, 1);
		}
		
		lua_pushboolean(L, GetTypeHash(inst1->Trace.GetUnderlyingPtr()) < GetTypeHash(inst2->Trace.GetUnderlyingPtr()));
		return UFINLuaProcessor::luaAPIReturn(L, 1);
	}

	int luaInstanceLe(lua_State* L) {
		LuaInstance* inst1 = CheckAndGetInstance(L, 1);
		LuaInstance* inst2 = GetInstance(L, 2);
		if (!inst1 || !inst2) {
			lua_pushboolean(L, false);
			return UFINLuaProcessor::luaAPIReturn(L, 1);
		}
		
		lua_pushboolean(L, GetTypeHash(inst1->Trace.GetUnderlyingPtr()) <= GetTypeHash(inst2->Trace.GetUnderlyingPtr()));
		return UFINLuaProcessor::luaAPIReturn(L, 1);
	}

	int luaInstanceToString(lua_State* L) {
		UFINClass* Class;
		LuaInstance* Instance = CheckAndGetInstance(L, 1, &Class);
		UObject* Obj = *Instance->Trace;
		if (!IsValid(Obj)) return luaL_argerror(L, 1, "Instance is invalid");
		UClass* Type = Obj->GetClass();
		FString Msg = Class->GetInternalName();
		if (Type->ImplementsInterface(UFINNetworkComponent::StaticClass())) {
			const FString Nick = IFINNetworkComponent::Execute_GetNick(Obj);
			if (Nick.Len() > 0) Msg += " \"" + Nick + "\"";
			Msg += " " + IFINNetworkComponent::Execute_GetID(Obj).ToString();
		}
		lua_pushstring(L,  TCHAR_TO_UTF8(*Msg));
		return 1;
	}

	int luaInstanceUnpersist(lua_State* L) {
		UFINLuaProcessor* Processor = UFINLuaProcessor::luaGetProcessor(L);
		
		// get persist storage
		FFINLuaProcessorStateStorage& Storage = Processor->StateStorage;

		// get trace and typename
		const FFINNetworkTrace Trace = Storage.GetTrace(luaL_checkinteger(L, lua_upvalueindex(1)));
		
		// create instance
		newInstance(L, Trace);
		
		return 1;
	}

	int luaInstancePersist(lua_State* L) {
		// get instance
		LuaInstance* Instance = CheckAndGetInstance(L, 1);
		
		UFINLuaProcessor* Processor = UFINLuaProcessor::luaGetProcessor(L);
		
		// get persist storage
		FFINLuaProcessorStateStorage& Storage = Processor->StateStorage;

		// add trace to storage
		lua_pushinteger(L, Storage.Add(Instance->Trace));
		
		// create & return closure
		lua_pushcclosure(L, &luaInstanceUnpersist, 1);
		return 1;
	}

	int luaInstanceGC(lua_State* L) {
		LuaInstance* instance = CheckAndGetInstance(L, 1);
		instance->~LuaInstance();
		return 0;
	}

	static const luaL_Reg luaInstanceLib[] = {
		{"__index", luaInstanceIndex},
		{"__newindex", luaInstanceNewIndex},
		{"__eq", luaInstanceEQ},
		{"__lt", luaInstanceLt},
		{"__le", luaInstanceLe},
		{"__tostring", luaInstanceToString},
		{"__persist", luaInstancePersist},
		{"__gc", luaInstanceGC},
		{nullptr, nullptr}
	};

	LuaInstance::LuaInstance(const FFINNetworkTrace& Trace, UFINKernelSystem* Kernel) : Trace(Trace), Kernel(Kernel) {
		Kernel->AddReferencer(this, &CollectReferences);
	}
	
	LuaInstance::LuaInstance(const LuaInstance& Other) : Trace(Other.Trace), Kernel(Other.Kernel) {
		Kernel->AddReferencer(this, &CollectReferences);
	}

	LuaInstance::~LuaInstance() {
		Kernel->RemoveReferencer(this);
	}

	void LuaInstance::CollectReferences(void* Obj, FReferenceCollector& Collector) {
		static_cast<LuaInstance*>(Obj)->Trace.AddStructReferencedObjects(Collector);
	}

	bool newInstance(lua_State* L, const FFINNetworkTrace& Trace) {
		// check obj and if type is registered
		UObject* Obj = Trace.GetUnderlyingPtr();
		UFINClass* Class = nullptr;
		if (IsValid(Obj)) Class = FFINReflection::Get()->FindClass(Obj->GetClass());
		if (!Class) {
			lua_pushnil(L);
			return false;
		}
		
		setupMetatable(UFINLuaProcessor::luaGetProcessor(L)->GetLuaState(), Class);
		ClassMetaNameLock.Lock();
		FString* NamePtr = ClassToMetaName.Find(Class);
		if (!NamePtr) {
			ClassMetaNameLock.Unlock();
			lua_pushnil(L);
			return false;
		}
		const FString Name = *NamePtr;
		ClassMetaNameLock.Unlock();
		
		// create instance
		LuaInstance* Instance = static_cast<LuaInstance*>(lua_newuserdata(L, sizeof(LuaInstance)));
		new (Instance) LuaInstance(Trace, UFINLuaProcessor::luaGetProcessor(L)->GetKernel());
		
		luaL_setmetatable(L, TCHAR_TO_UTF8(*Name));
		return true;
	}

	FFINNetworkTrace getObjInstance(lua_State* L, int index, UClass* clazz, bool bError) {
		if (lua_isnil(L, index)) return FFINNetworkTrace(nullptr);
		LuaInstance* instance;
		if (bError) instance = CheckAndGetInstance(L, index);
		else instance = GetInstance(L, index);
		if (!instance || !instance->Trace.IsValid() || !instance->Trace->GetClass()->IsChildOf(clazz)) return FFINNetworkTrace(nullptr);
		return instance->Trace;
	}

	int luaFindClass(lua_State* L) {
		const int args = lua_gettop(L);

		for (int i = 1; i <= args; ++i) {
			const bool isT = lua_istable(L, i);

			TArray<FString> ClassNames;
			if (isT) {
				const auto count = lua_rawlen(L, i);
				for (int j = 1; j <= count; ++j) {
					lua_geti(L, i, j);
					if (!lua_isstring(L, -1)) return luaL_argerror(L, i, "array contains non-string");
					ClassNames.Add(lua_tostring(L, -1));
					lua_pop(L, 1);
				}
				lua_newtable(L);
			} else {
				if (!lua_isstring(L, i)) return luaL_argerror(L, i, "is not string");
				ClassNames.Add(lua_tostring(L, i));
			}
			int j = 0;
			TArray<UFINClass*> Classes;
			FFINReflection::Get()->GetClasses().GenerateValueArray(Classes);
			for (const FString& ClassName : ClassNames) {
				UFINClass** Class = Classes.FindByPredicate([ClassName](UFINClass* Class) {
					if (Class->GetInternalName() == ClassName) return true;
					return false;
				});
				if (Class) newInstance(L, FINTrace(*Class));
				else lua_pushnil(L);
				if (isT) lua_seti(L, -2, ++j);
			}
		}
		return UFINLuaProcessor::luaAPIReturn(L, args);
	}

	UFINClass* luaFIN_getObjectType(lua_State* L, int index) {
		if (lua_type(L, index) != LUA_TUSERDATA) return nullptr;
		int type = luaL_getmetafield(L, index, "__name");
		if (type != LUA_TSTRING) {
			if (type != LUA_TNIL) lua_pop(L, 1);
			return nullptr;
		}
		FString TypeName = luaFIN_toFString(L, -1);
		lua_pop(L, 1);
		ClassMetaNameLock.Lock();
		UFINClass** Type = MetaNameToClass.Find(TypeName);
		ClassMetaNameLock.Unlock();
		if (!Type) return nullptr;
		return *Type;
	}

	void setupMetatable(lua_State* L, UFINClass* Class) {
		FScopeLock ScopeLock(&ClassMetaNameLock);
		
		lua_getfield(L, LUA_REGISTRYINDEX, "PersistUperm");
		lua_getfield(L, LUA_REGISTRYINDEX, "PersistPerm");
		PersistSetup("InstanceSystem", -2);

		FString TypeName = Class->GetInternalName();
		if (luaL_getmetatable(L, TCHAR_TO_UTF8(*TypeName)) != LUA_TNIL) {
			lua_pop(L, 3);
			return;
		}
		lua_pop(L, 1);
		luaL_newmetatable(L, TCHAR_TO_UTF8(*TypeName));							// ..., InstanceMeta
		lua_pushboolean(L, true);
		lua_setfield(L, -2, "__metatable");
		luaL_setfuncs(L, luaInstanceLib, 0);
		lua_newtable(L);															// ..., InstanceMeta, InstanceCache
		lua_setfield(L, -2, LUA_REF_CACHE);									// ..., InstanceMeta
		PersistTable(TCHAR_TO_UTF8(*TypeName), -1);
		lua_pop(L, 1);															// ...
		MetaNameToClass.FindOrAdd(TypeName) = Class;
		ClassToMetaName.FindOrAdd(Class) = TypeName;
	}

	void setupInstanceSystem(lua_State* L) {
		PersistSetup("InstanceSystem", -2);
		
		luaL_newmetatable(L, INSTANCE_TYPE);			// ..., InstanceTypeMeta
		lua_pushboolean(L, true);
		lua_setfield(L, -2, "__metatable");
		luaL_setfuncs(L, luaInstanceTypeLib, 0);
		PersistTable(INSTANCE_TYPE, -1);
		lua_pop(L, 1);									// ...

		lua_register(L, "findClass", luaFindClass);
		PersistGlobal("findClass");

		lua_pushcfunction(L, luaInstanceFuncCall);			// ..., InstanceFuncCall
		PersistValue("InstanceFuncCall");					// ...
		lua_pushcfunction(L, luaInstanceUnpersist);			// ..., LuaInstanceUnpersist
		PersistValue("InstanceUnpersist");					// ...
		lua_pushcfunction(L, luaInstanceTypeUnpersist);		// ..., LuaInstanceTypeUnpersist
		PersistValue("InstanceTypeUnpersist");				// ...
	}
}
