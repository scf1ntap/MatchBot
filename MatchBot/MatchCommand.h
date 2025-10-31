#pragma once

// Default commands file
#define MB_COMMANDS_FILE "cstrike/addons/matchbot/commands.txt"

// Command info
typedef struct S_COMMAND_INFO
{
	int Index;
	std::string Name;
	int Flag;
}P_COMMAND_INFO, *LP_COMMAND_INFO;

// Command indexes
enum E_COMMAND_INDEX
{
	CMD_PLAYER_STATUS = 1,
	CMD_PLAYER_SCORE,
	CMD_PLAYER_READY,
	CMD_PLAYER_NOTREADY,
	CMD_PLAYER_HP,
	CMD_PLAYER_DMG,
	CMD_PLAYER_RDMG,
	CMD_PLAYER_SUM,
	CMD_PLAYER_HELP,
	CMD_PLAYER_VOTE,
	CMD_PLAYER_VOTE_KICK,
	CMD_PLAYER_VOTE_MAP,
	CMD_PLAYER_VOTE_PAUSE,
	CMD_PLAYER_VOTE_RESTART,
	CMD_PLAYER_VOTE_STOP,
	CMD_PLAYER_MUTE_MENU,
	CMD_PLAYER_VOTE_SURRENDER,
	//
	CMD_ADMIN_MENU = 33,
	CMD_ADMIN_KICK,
	CMD_ADMIN_BAN,
	CMD_ADMIN_KILL,
	CMD_ADMIN_TEAM,
	CMD_ADMIN_MAP,
	CMD_ADMIN_CONTROL,
	CMD_ADMIN_MESSAGE,
	CMD_ADMIN_COMMAND,
	CMD_ADMIN_SWAP,
	CMD_ADMIN_VOTE_MAP,
	CMD_ADMIN_VOTE_TEAM,
	CMD_ADMIN_START_MATCH,
	CMD_ADMIN_STOP_MATCH,
	CMD_ADMIN_RESTART_MATCH,
	CMD_ADMIN_PAUSE_MATCH,
	CMD_ADMIN_HELP,
	CMD_ADMIN_PLAYER_LIST,
	CMD_ADMIN_CVAR_MENU
};

class CMatchCommand
{
public:
	// On Server Activate
	void ServerActivate();

	// Get Command info by index
	LP_COMMAND_INFO GetInfo(E_COMMAND_INDEX CommandIndex);

	// On Client Command
	bool ClientCommand(CBasePlayer* Player, const char* pcmd, const char* parg1);

private:
	std::map<std::string, P_COMMAND_INFO> m_Data;
};

extern CMatchCommand gMatchCommand;
