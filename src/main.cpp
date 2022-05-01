#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/ModuleLoader.hpp>

#if IS_SERVERSIDE 
#include <eiface.h>
#ifdef _WIN32
#define SERVER_CMD_INDEX 36
#define SERVER_CLIENT_CMD_INDEX 5
#else
#define SERVER_CMD_INDEX 37
#define SERVER_CLIENT_CMD_INDEX 6
#define __thiscall
#define __cdecl
#endif
#else
#include <cdll_int.h>
#ifdef _WIN32
#define CLIENT_CMD_INDEX 6
#define CLIENT_CMD_UNRESTRICTED_INDEX 106
#else
#define CLIENT_CMD_INDEX 7
#define CLIENT_CMD_UNRESTRICTED_INDEX 106
#define __thiscall
#define __cdecl
#endif
#endif

#include <vtable.h>

GarrysMod::Lua::ILuaBase* _LUA = nullptr;
VTable* hooker = nullptr;

typedef void(__thiscall* RunCommandFn)(void*, const char*);
typedef void* (__cdecl* CreateInterfaceFn)(const char* name, int* found);

#if IS_SERVERSIDE
VTable* server_client_hooker = nullptr;
IVEngineServer* serverInterface = nullptr;
IServerGameClients* serverClientInterface = nullptr;
const SourceSDK::ModuleLoader engine_loader("engine_srv");
#else
const SourceSDK::ModuleLoader engine_loader("engine");
#endif

bool AllowStringCommand(const char* cmdStr, bool isUnrestricted) 
{
	if (_LUA == nullptr) return true;

	_LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	_LUA->GetField(-1, "hook");

	_LUA->GetField(-1, "Run");
	_LUA->PushString("AllowStringCommand");
	_LUA->PushString(cmdStr);
	_LUA->PushBool(isUnrestricted);

	_LUA->Call(3, 1);

	bool ret = true;
	if (_LUA->GetType(-1) == (int)GarrysMod::Lua::Type::BOOL) 
	{
		ret = _LUA->GetBool(-1);
	}

	_LUA->Pop(3);

	return ret;
}

#if IS_SERVERSIDE
void ServerCommand(void* inst, const char* cmdStr)
{
	bool shouldRun = AllowStringCommand(cmdStr, true);
	if (shouldRun)
	{
		RunCommandFn(hooker->getold(SERVER_CMD_INDEX))(inst, cmdStr);
	}
}

typedef void(__thiscall* ClientIssuedCommandFn)(void*, edict_t*, const CCommand&);
void ClientIssuedCommand(void* inst, edict_t* pEntity, const CCommand& args)
{
	const char* cmdStr = args.GetCommandString();
	int index = serverInterface->IndexOfEdict(pEntity);

	_LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	_LUA->GetField(-1, "hook");

	_LUA->GetField(-1, "Run");
	_LUA->PushString("AllowClientStringCommand");

	_LUA->GetField(-4, "Entity");
	_LUA->PushNumber(index);
	_LUA->Call(1, 1); // push the player on the stack, to pass it to the hook

	_LUA->PushString(cmdStr);

	_LUA->Call(3, 1);

	bool ret = true;
	if (_LUA->GetType(-1) == (int)GarrysMod::Lua::Type::BOOL)
	{
		ret = _LUA->GetBool(-1);
	}

	_LUA->Pop(3);

	if (!ret) return; // deny user the command

	ClientIssuedCommandFn(server_client_hooker->getold(SERVER_CLIENT_CMD_INDEX))(inst, pEntity, args);
}
#else
void ClientCommand(void* inst, const char* cmdStr)
{
	bool shouldRun = AllowStringCommand(cmdStr, false);
	if (shouldRun)
	{
		RunCommandFn(hooker->getold(CLIENT_CMD_INDEX))(inst, cmdStr);
	}
}

void ClientCommandUnrestricted(void* inst, const char* cmdStr)
{
	bool shouldRun = AllowStringCommand(cmdStr, true);
	if (shouldRun)
	{
		RunCommandFn(hooker->getold(CLIENT_CMD_UNRESTRICTED_INDEX))(inst, cmdStr);
	}
}
#endif

GMOD_MODULE_OPEN() 
{
	_LUA = LUA;

#if IS_SERVERSIDE
	serverInterface = reinterpret_cast<IVEngineServer*>(CreateInterfaceFn(engine_loader.GetSymbol("CreateInterface"))(INTERFACEVERSION_VENGINESERVER, 0));
	serverClientInterface = reinterpret_cast<IServerGameClients*>(CreateInterfaceFn(engine_loader.GetSymbol("CreateInterface"))(INTERFACEVERSION_SERVERGAMECLIENTS, 0));

	hooker = new VTable(serverInterface);
	hooker->hook(SERVER_CMD_INDEX, (void*)&ServerCommand);

	server_client_hooker = new VTable(serverClientInterface);
	server_client_hooker->hook(SERVER_CLIENT_CMD_INDEX, (void*)&ClientIssuedCommand);
#else
	IVEngineClient013* clientInterface = reinterpret_cast<IVEngineClient013*>(CreateInterfaceFn(engine_loader.GetSymbol("CreateInterface"))(VENGINE_CLIENT_INTERFACE_VERSION, 0));
	hooker = new VTable(clientInterface);

	hooker->hook(CLIENT_CMD_INDEX, (void*)&ClientCommand);
	hooker->hook(CLIENT_CMD_UNRESTRICTED_INDEX, (void*)&ClientCommandUnrestricted);
#endif

	return 0;
}

GMOD_MODULE_CLOSE() 
{
#if IS_SERVERSIDE
	hooker->unhook(SERVER_CMD_INDEX);
	server_client_hooker->unhook(SERVER_CLIENT_CMD_INDEX);

	delete server_client_hooker;
#else
	hooker->unhook(CLIENT_CMD_INDEX);
	hooker->unhook(CLIENT_CMD_UNRESTRICTED_INDEX);
#endif

	delete hooker;

	LUA = nullptr;

	return 0;
}
