#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/ModuleLoader.hpp>

#if IS_SERVERSIDE 
#include <eiface.h>
#define SERVER_CMD_INDEX 36
#define SERVER_CLIENT_CMD_INDEX 5
#else
#include <cdll_int.h>
#define CLIENT_CMD_INDEX 6
#define CLIENT_CMD_UNRESTRICTED_INDEX 106
#endif

#include <vtable.h>

GarrysMod::Lua::ILuaBase* LUA = nullptr;
VTable* hooker = nullptr;

typedef bool(__thiscall* RunCommandFn)(void*, const char*);
typedef void* (__cdecl* CreateInterfaceFn)(const char* name, int* found);

#if IS_SERVERSIDE
VTable* server_client_hooker = nullptr;
const SourceSDK::ModuleLoader engine_loader("engine_srv");
#else
const SourceSDK::ModuleLoader engine_loader("engine");
#endif

bool AllowStringCommand(const char* cmdStr, bool isUnrestricted) 
{
	if (LUA == nullptr) return true;

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "hook");

	LUA->GetField(-1, "Run");
	LUA->PushString("AllowStringCommand");
	LUA->PushString(cmdStr);
	LUA->PushBool(isUnrestricted);

	LUA->Call(3, 1);

	bool ret = true;
	if (LUA->GetType(-1) == (int)GarrysMod::Lua::Type::BOOL) 
	{
		ret = LUA->GetBool(-1);
	}

	LUA->Pop(3);

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

// TODO: Figure out how to edict_t to ent index
/*void ClientIssuedCommand(edict_t* pEntity, const CCommand& args)
{
	const char* cmdStr = args.GetCommandString();
	CBaseEntity* ent = pEntity->GetIServerEntity()->GetBaseEntity();
	
}*/
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
	LUA = LUA;

#if IS_SERVERSIDE
	IVEngineServer* serverInterface = reinterpret_cast<IVEngineServer*>(CreateInterfaceFn(engine_loader.GetSymbol("CreateInterface"))(INTERFACEVERSION_VENGINESERVER, 0));
	IServerGameClients* serverClientInterface = reinterpret_cast<IServerGameClients*>(CreateInterfaceFn(engine_loader.GetSymbol("CreateInterface"))(INTERFACEVERSION_SERVERGAMECLIENTS, 0));

	hooker = new VTable(serverInterface);
	hooker->hook(SERVER_CMD_INDEX, (void*)&ServerCommand);

	/*server_client_hooker = new VTable(serverClientInterface);
	server_client_hooker->hook(SERVER_CLIENT_CMD_INDEX, (void*)&ClientIssuedCommand);*/
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
	//server_client_hooker->unhook(SERVER_CLIENT_CMD_INDEX);

	//delete server_client_hooker;
#else
	hooker->unhook(CLIENT_CMD_INDEX);
	hooker->unhook(CLIENT_CMD_UNRESTRICTED_INDEX);
#endif

	delete hooker;

	LUA = nullptr;

	return 0;
}