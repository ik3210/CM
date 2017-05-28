
#include "CatchMe.h" 
#include "TableUtil.h"
#include "DelegateLuaProxy.h"
#include "UObject/UObjectThreadContext.h"
#include "luaload.h"
#include "lualoadgame.h"
// #include "GeneratedScriptLibraries.inl"
#include "allenum.script.h"
DEFINE_LOG_CATEGORY(LuaLog);

lua_State* UTableUtil::L = nullptr;
int32 UTableUtil::ManualInitCount = 0;
bool UTableUtil::HasManualInit = false;


TMap<FString, TMap<FString, UProperty*>> UTableUtil::propertyMap;
TMap<FString, TMap<FString, UFunction*>> UTableUtil::functionMap;
// FLuaGcObj UTableUtil::gcobjs;
#ifdef LuaDebug
TMap<FString, int> UTableUtil::countforgc;
#endif
// FLuaGcObj UTableUtil::gcobjs;
void UTableUtil::InitClassMap()
{
	static bool HasInit = false;
	if (!HasInit) {
		HasInit = true;
		for (TObjectIterator<UClass> uClass; uClass; ++uClass)
		{
			UClass* theClass = *uClass;
			FString className = FString::Printf(TEXT("%s%s"), theClass->GetPrefixCPP(), *theClass->GetName());
			TMap<FString, UProperty*> m;
			for (TFieldIterator<UProperty> PropertyIt(theClass); PropertyIt; ++PropertyIt)
			{
				UProperty* Property = *PropertyIt;
				m.Add(Property->GetName(), Property);
			}
			propertyMap.Add(className, m);
			TMap<FString, UFunction*> funcs;
			for (TFieldIterator<UFunction> FuncIt(theClass); FuncIt; ++FuncIt)
			{
				UFunction* func = *FuncIt;
				funcs.Add(func->GetName(), func);
			}
			functionMap.Add(className, funcs);
		}
		for (TObjectIterator<UScriptStruct> uStruct; uStruct; ++uStruct)
		{
			auto theStruct = *uStruct;
			FString className = FString::Printf(TEXT("%s%s"), theStruct->GetPrefixCPP(), *theStruct->GetName());
			TMap<FString, UProperty*> m;
			for (TFieldIterator<UProperty> PropertyIt(theStruct); PropertyIt; ++PropertyIt)
			{
				UProperty* Property = *PropertyIt;
				m.Add(Property->GetName(), Property);
			}
			propertyMap.Add(className, m);
		}
	}
}

UProperty* UTableUtil::GetPropertyByName(FString classname, FString propertyname)
{
	return propertyMap[classname][propertyname];
}

UFunction* UTableUtil::GetFunctionByName(FString classname, FString funcname)
{
	return functionMap[classname][funcname];
}

UProperty* UTableUtil::GetPropertyByName(UClass *Class, FString propertyname)
{
	FString namecpp = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
	return GetPropertyByName(namecpp, propertyname);
}

static void* LuaAlloc(void *Ud, void *Ptr, size_t OldSize, size_t NewSize)
{
	if (NewSize != 0)
	{
		return FMemory::Realloc(Ptr, NewSize);
	}
	else
	{
		FMemory::Free(Ptr);
		return NULL;
	}
}

static int32 LuaPanic(lua_State *L)
{
	UTableUtil::log(FString::Printf(TEXT("PANIC: unprotected error in call to Lua API(%s)"), ANSI_TO_TCHAR(lua_tostring(L, -1))));
	return 0;
}

void UTableUtil::init(bool IsManual)
{
	if (IsManual)
	{
		++ManualInitCount;
	}
	if (L != nullptr)
	{
		if (IsManual && !HasManualInit)
		{
			Call_void("Shutdown");
			lua_close(L);
		}
		else
			return;
	}
	InitClassMap();
	auto l = lua_newstate(LuaAlloc, nullptr);
	if (l) lua_atpanic(l, &LuaPanic);
	luaL_openlibs(l);
	L = l;
	FString gameDir = FPaths::GameDir();
	FString luaDir = gameDir /TEXT("LuaSource");
	FString mainFilePath = luaDir / TEXT("main.lua");
	if (luaL_dofile(l, TCHAR_TO_ANSI(*mainFilePath)))
	{
		UTableUtil::log(FString(lua_tostring(L, -1)));
	}
	else
	{
		//set table for index exist userdata
		lua_newtable(L);
		lua_newtable(L);
		lua_pushstring(L, "v");
		lua_setfield(L, -2, "__mode");
		lua_setmetatable(L, -2);
		lua_setfield(L, LUA_REGISTRYINDEX, "_existuserdata");

		//set table for need Destroy data
		lua_newtable(L);
		lua_setfield(L, LUA_REGISTRYINDEX, "_needgcdata");

		lua_newtable(L);
		lua_setfield(L, LUA_REGISTRYINDEX, "_luacallback");

		//when lua has correspond table of the ins, push the table
		lua_newtable(L);
		lua_newtable(L);
		lua_pushstring(L, "kv");
		lua_setfield(L, -2, "__mode");
		lua_setmetatable(L, -2);
		lua_setfield(L, LUA_GLOBALSINDEX, "_objectins2luatable");

		push(luaDir);
		lua_setfield(L, LUA_GLOBALSINDEX, "_luadir");
		push(gameDir);
		lua_setfield(L, LUA_GLOBALSINDEX, "_gamedir");

		//register all function
		ULuaLoad::LoadAll(L);
		ULuaLoadGame::LoadAll(L);
		lua_pushboolean(L, IsManual);
		executeFunc("Init", 0, 1);
#ifdef LuaDebug
		testtemplate();
#endif // LuaDebug

	}
	HasManualInit = IsManual;
}

void UTableUtil::shutdown()
{
	--ManualInitCount;
	if (L != nullptr && ManualInitCount == 0)
	{
		Call_void("Shutdown");
		lua_close(L);
		L = nullptr;
	}
}

int32 indexFunc(lua_State* L)
{
	lua_getmetatable(L, 1);
	lua_pushvalue(L, 2);
	lua_rawget(L,-2);
	if (lua_isnil(L, -1))
	{
		FString property = FString(lua_tostring(L, 2));
		lua_pop(L, 1);
		FString propertyKey = FString::Printf(TEXT("Get_%s"), *property);
		UTableUtil::push(propertyKey);
		lua_rawget(L, -2);
		if (!lua_isnil(L, -1))
		{
			lua_pushvalue(L, 1);
			lua_pcall(L, 1, 1, 0);
		}
	}
	return 1;
}

int32 newindexFunc(lua_State* L)
{
	if (lua_isuserdata(L, 1))
	{
		lua_getmetatable(L, 1);
		FString property = FString(lua_tostring(L, 2));
		FString propertyKey = FString::Printf(TEXT("Set_%s"), *property);
		UTableUtil::push(propertyKey);
		lua_rawget(L, -2);
		if (!lua_isnil(L, -1))
		{
			lua_pushvalue(L, 1);
			lua_pushvalue(L, 3);
			lua_call(L, 2, 1);
		}
	}
	else if (lua_istable(L, 1))
	{
		lua_rawset(L, -3);
	}
	return 0;
}

int32 cast(lua_State* L)
{
	if (lua_isnil(L, 2))
	{
		UTableUtil::log(FString("cast error, nil"));
		return 1;
	}
	lua_pushstring(L, "classname");
	lua_rawget(L, 1);
	luaL_getmetatable(L, lua_tostring(L, -1));
	lua_setmetatable(L, 2);
	lua_pushvalue(L, 2);
	return 1;
}

int32 gcfunc(lua_State *L)
{
	auto u = (void**)lua_touserdata(L, -1);
	lua_getfield(L, LUA_REGISTRYINDEX, "_needgcdata");
	lua_pushlightuserdata(L, *u);
	lua_rawget(L, -2);
	if ( !lua_isnil(L, -1) )
	{
		lua_pop(L, 1);
		lua_pushlightuserdata(L, *u);
		lua_pushnil(L);
		lua_rawset(L, -3);
		lua_getmetatable(L, 1);
		lua_getfield(L, -1, "Destroy");
		if (lua_iscfunction(L, -1))
		{
			lua_getmetatable(L, 1);
			lua_getfield(L, -1, "classname");
			FString n = lua_tostring(L, -1);
			lua_pop(L, 2);
#ifdef LuaDebug
			UTableUtil::countforgc[n]--;
#endif
			lua_pushvalue(L, 1);
			if (lua_pcall(L, 1, 0, 0))
			{
				UTableUtil::log(FString(lua_tostring(L, -1)));
			}
		}
		else
		{
			lua_pop(L, 1);
			lua_getmetatable(L, 1);
			lua_getfield(L, -1, "classname");
			FString classname = lua_tostring(L, -1);
			if (classname == "UClass")
			{
#ifdef LuaDebug
				UTableUtil::countforgc[classname]--;
#endif			
				UTableUtil::rmgcref(static_cast<UObject*>(UTableUtil::tousertype(TCHAR_TO_ANSI(*classname), 1)));
			}
		}
	}
	return 0;
}

void UTableUtil::initmeta()
{
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, indexFunc);
	lua_rawset(L, -3);
	lua_pushstring(L, "cast");
	lua_pushcfunction(L, cast);
	lua_rawset(L, -3);
	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, newindexFunc);
	lua_rawset(L, -3);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, gcfunc);
	lua_rawset(L, -3);
	lua_pushstring(L, "__iscppclass");
	lua_pushboolean(L, true);
	lua_rawset(L, -3);
}

void UTableUtil::addmodule(const char* name)
{
	lua_getglobal(L, name);
	if (lua_istable(L, -1))
	{
		lua_pop(L, 1);
		return;
	}
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	lua_pushstring(L, name);
	luaL_newmetatable(L, name);
	initmeta();
	lua_pushstring(L, "classname");
	lua_pushstring(L, name);
	lua_rawset(L, -3);
	lua_rawset(L, -3);
	lua_pop(L, 2);
}

void UTableUtil::openmodule(const char* name)
{
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	lua_pushstring(L, name);
	lua_rawget(L, -2);
}


void UTableUtil::addfunc(const char* name, luafunc f)
{
	lua_pushstring(L, name);
	lua_pushcfunction(L, f);
	lua_rawset(L, -3);
}

void UTableUtil::closemodule()
{
	lua_pop(L, 2);
}

void* tousertype(lua_State* L, const char* classname, int i)
{
	if (lua_isnil(L, i))
		return nullptr;
	else if (lua_istable(L, i))
	{
		if (i < 0)
			i = lua_gettop(L) + i+1;
		lua_pushstring(L,  "_cppinstance_");
		lua_rawget(L, i);
		if (lua_isnil(L, -1))
		{
			lua_pushstring(L, "suck in tousertype");
			lua_error(L);
			return nullptr;
		}
		else
		{
			lua_replace(L, i);
			return tousertype(L, classname, i);
		}
	}
	else if (lua_isuserdata(L, i))
	{
		auto u = static_cast<void**>(lua_touserdata(L, i));
		return *u;
	}
	else
		return nullptr;
}

void* UTableUtil::tousertype(const char* classname, int i)
{
	return ::tousertype(L, classname, i);
}

void UTableUtil::setmeta(const char* classname, int index)
{
	luaL_getmetatable(L, classname);
	if (lua_istable(L, -1))
	{
		lua_setmetatable(L, index-1);
	}
	else
	{
		lua_pop(L, 1);
		UTableUtil::addmodule(classname);
		luaL_getmetatable(L, classname);
		lua_setmetatable(L, index-1);
	}
}
int UTableUtil::push(uint8 value)
{
	lua_pushboolean(L, !!value);
	return 1;
}
int UTableUtil::push(int value)
{
	lua_pushinteger(L, value);
	return 1;
}
int UTableUtil::push(float value)
{
	lua_pushnumber(L, value);
	return 1;
}
int UTableUtil::push(double value)
{
	lua_pushnumber(L, value);
	return 1;
}
int UTableUtil::push(bool value)
{
	lua_pushboolean(L, value);
	return 1;
}

int UTableUtil::push(FString value)
{
	lua_pushstring(L, TCHAR_TO_ANSI(*value));
	return 1;
}

int UTableUtil::push(FText value)
{
	lua_pushstring(L, TCHAR_TO_ANSI(*value.ToString()));
	return 1;
}

int UTableUtil::push(FName value)
{
	lua_pushstring(L, TCHAR_TO_ANSI(*value.ToString()));
	return 1;
}

int UTableUtil::push(const char* value)
{
	lua_pushstring(L, value);
	return 1;
}

void UTableUtil::pushclass(const char* classname, void* p, bool bgcrecord)
{
	if (p == nullptr)
	{
		lua_pushnil(L);
		return;
	}
// should do in glue code , later
	bool bIsActorOrObject = (classname[0] == 'U' || classname[0] == 'A');
	if (!existdata(p))
	{
		*(void**)lua_newuserdata(L, sizeof(void *)) = p;

		lua_getfield(L, LUA_REGISTRYINDEX, "_existuserdata");
		lua_pushlightuserdata(L, p);
		lua_pushvalue(L, -3);
		lua_rawset(L, -3);
		lua_pop(L, 1);
		if (bgcrecord || bIsActorOrObject)
		{
			lua_getfield(L, LUA_REGISTRYINDEX, "_needgcdata");
			lua_pushlightuserdata(L, p);
			lua_pushboolean(L, true);
			lua_rawset(L, -3);
			lua_pop(L, 1);
#ifdef LuaDebug
			if (countforgc.Contains(classname))  
				countforgc[classname]++;
			else
				countforgc.Add(classname, 1);
#endif
		}
		if (bIsActorOrObject)
			addgcref(static_cast<UObject*>(p));
		setmeta(classname, -1);
	}
	else if (bIsActorOrObject)
	{
		lua_getfield(L, LUA_GLOBALSINDEX, "_objectins2luatable");
		lua_pushvalue(L, -2);
		lua_rawget(L, -2);
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 2);
#ifdef LuaDebug
			lua_getfield(L, -1, "classname");
			FString oldname = lua_tostring(L, -1);
			countforgc[oldname]--;
			if (countforgc.Contains(classname))
				countforgc[classname]++;
			else
				countforgc.Add(classname, 1);
			lua_pop(L, 1);

#endif
			setmeta(classname, -1);
		}
		else
		{
			lua_remove(L, -2);
			lua_remove(L, -2);
		}
	}
	else
	{
		setmeta(classname, -1);
	}

}

UTableUtil::UTableUtil()
{
	
}

void UTableUtil::loadlib(const luaL_Reg funclist[], const char* classname)
{
	int i = 0;
	UTableUtil::addmodule(classname);
	UTableUtil::openmodule(classname);
	while (true)
	{
		luaL_Reg temp = funclist[i];
		if (temp.name == nullptr)
		{
			break;
		}
		else if (temp.func != nullptr)
		{
			UTableUtil::addfunc(temp.name, temp.func);
		}
		i++;
	}
	UTableUtil::closemodule();
}

void UTableUtil::addutil(const luaL_Reg funclist[], const char* tablename)
{
	lua_newtable(L);
	int i = 0;
	while (true)
	{
		const luaL_Reg &temp = funclist[i];
		if (temp.name == nullptr)
		{
			break;
		}
		else if (temp.func != nullptr)
		{
			lua_pushcfunction(L, temp.func);
			lua_setfield(L, -2, temp.name);
		}
		i++;
	}
	lua_setfield(L, LUA_GLOBALSINDEX, tablename);
}


void UTableUtil::loadEnum(const EnumItem list[], const char* enumname)
{
	lua_newtable(L);
	int i = 0;
	while (true)
	{
		EnumItem temp = list[i];
		if (temp.key == nullptr)
			break;
		else
		{
			lua_pushstring(L, temp.key);
			lua_pushinteger(L, temp.value);
			lua_rawset(L, -3);
		}
		++i;
	}
	lua_setfield(L, LUA_GLOBALSINDEX, enumname);
}

void UTableUtil::executeFunc(FString funcName, int n, int nargs)
{
	lua_getfield(L, LUA_GLOBALSINDEX, TCHAR_TO_ANSI(*funcName));
	if (nargs > 0)
		lua_insert(L, -nargs-1);
	if (lua_pcall(L, nargs, n, 0))
		log(lua_tostring(L, -1));
}

void UTableUtil::executeFuncid(int32 funcid, int n, int nargs)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, funcid);
	if (nargs > 0)
		lua_insert(L, -nargs-1);
	if (lua_pcall(L, nargs, n, 0))
		log(lua_tostring(L, -1));
}

// api for blueprint start:
void UTableUtil::Push_obj(UObject *p)
{
	auto Class = p->StaticClass();
	FString ClassName = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
	pushclass(TCHAR_TO_ANSI(*ClassName), (void*)p);
}

void UTableUtil::Push_float(float value)
{
	lua_pushnumber(L, value);
}

void UTableUtil::Push_str(FString value)
{
	push(value);
}

void UTableUtil::Push_bool(bool isTrue)
{
	lua_pushboolean(L, isTrue);
}


void UTableUtil::Call_void(FString funcName, int count_param)
{
	if (count_param == -1)
		count_param = lua_gettop(L);
	executeFunc(funcName, 0, count_param);
}

float UTableUtil::Call_float(FString funcName, int count_param)
{
	if (count_param == -1)
		count_param = lua_gettop(L);
	executeFunc(funcName, 0, count_param);
	auto result = lua_tonumber(L, -1);
	return result;
}

UObject* UTableUtil::Call_obj(FString funcName, int count_param)
{
	if (count_param == -1)
		count_param = lua_gettop(L);
	executeFunc(funcName, 0, count_param);
	auto result = (UObject*)tousertype("", -1);
	return result;
}

FString UTableUtil::Call_str(FString funcName, int count_param)
{
	if (count_param == -1)
		count_param = lua_gettop(L);
	executeFunc(funcName, 0, count_param);
	FString result = ANSI_TO_TCHAR(luaL_checkstring(L, -1));
	return result;
}

bool UTableUtil::Call_bool(FString funcName, int count_param)
{
	if (count_param == -1)
		count_param = lua_gettop(L);
	executeFunc(funcName, 0, count_param);
	bool result = !!lua_toboolean(L, -1);
	return result;
}

bool UTableUtil::existdata(void * p)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "_existuserdata");
	lua_pushlightuserdata(L, p);
	lua_rawget(L, -2);
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 2);
		return false;
	}
	else
	{
		lua_replace(L, -2);
		return true;
	}
}

void UTableUtil::log(FString content)
{
	UE_LOG(LuaLog, Warning, TEXT("[lua error] %s"), *content);
}

void UTableUtil::CtorCpp(UObject* p, FString classpath)
{
	if (L == nullptr)
		init();
	pushclass("UObject", (void*)p, true);
	push(classpath);
	Call_void(FString("CtorCpp"));
}

UObject* UTableUtil::FObjectFinder( UClass* Class, FString PathName )
{
	auto& ThreadContext = FUObjectThreadContext::Get();
	UE_CLOG(!ThreadContext.IsInConstructor, LogUObjectGlobals, Fatal, TEXT("FObjectFinders can't be used outside of constructors to find %s"), *PathName);
	ConstructorHelpers::StripObjectClass(PathName, true);
	int32 PackageDelimPos = INDEX_NONE;
	PathName.FindChar(TCHAR('.'), PackageDelimPos);
	if (PackageDelimPos == INDEX_NONE)
	{
		int32 ObjectNameStart = INDEX_NONE;
		PathName.FindLastChar(TCHAR('/'), ObjectNameStart);
		if (ObjectNameStart != INDEX_NONE)
		{
			const FString ObjectName = PathName.Mid(ObjectNameStart + 1);
			PathName += TCHAR('.');
			PathName += ObjectName;
		}
	}
	Class->GetDefaultObject(); // force the CDO to be created if it hasn't already
	UObject * result = (UObject*)StaticLoadObject(Class, nullptr, *PathName);
	if (result)
	{
		result->AddToRoot();
	}
	return result;
}

int UTableUtil::pushluafunc(int index)
{
	if (index < 0)
		index = lua_gettop(L) + index + 1;

	lua_pushvalue(L, index);
	int r = luaL_ref(L, LUA_REGISTRYINDEX);

	return r;
}

int UTableUtil::popluafunc(int index)
{
	if (index < 0)
		index = lua_gettop(L) + index + 1;
	lua_getfield(L, LUA_REGISTRYINDEX, "_luacallback");
	lua_pushvalue(L, index);
	lua_rawget(L, -2);
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 3);
		return -1;
	}
	else
	{
		int r = lua_tointeger(L, -1);
		luaL_unref(L, LUA_REGISTRYINDEX, r);
		lua_pushvalue(L, index);
		lua_pushnil(L);
		lua_rawset(L, -4);
		lua_pop(L, 3);
		return r;
	}
}

void UTableUtil::unref(int r)
{
	luaL_unref(L, LUA_REGISTRYINDEX, r);
}

void UTableUtil::addgcref(UObject *p)
{
	FLuaGcObj::Get()->objs.Add(p);
}

void UTableUtil::rmgcref(UObject *p)
{
	FLuaGcObj::Get()->objs.Remove(p);
}

void UTableUtil::testtemplate()
{
	FVector f;
	AActor *p = nullptr;
	TArray<FVector> t;
	call("testtemplate_call", f, 1,"test", p, 2.0, true, t);
	if (callr<int>("testtemplate_r_int") != 1)
		log("int");
	if ( callr<float>("testtemplate_r_float") != 1.0 )
		log("float");
	if ( callr<double>("testtemplate_r_double") != 1.0 )
		log("double");
	if (callr<AActor*>("testtemplate_r_AActor") != nullptr)
		log("AActor");
	f = callr<FVector>("testtemplate_r_FVector");
	if (f.X != 1 || f.Y != 2 || f.Z != 3)
		log("FVector");
	auto t1 = callr<TArray<int>>("testtemplate_r_TArray");
	if (t1[0] != 1 || t1[1] != 2 || t1[2] != 3)
		log("TArray");
}
