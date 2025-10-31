#include "precompiled.h"

CMatchVoteMenu gMatchVoteMenu;

// Server Activate
void CMatchVoteMenu::SetState(int State)
{
	// Switch State
	if (State == STATE_DEAD)
	{
		// Loop max clients id
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			// Clear all votes
			this->m_Votes[i].Reset();
		}

		// Clear Map List
		this->m_MapList = gMatchUtil.GetMapList(false);
	}
	else
	{
		// Loop max clients id
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			// Clear votes for pause
			this->m_Votes[i].VotePause.fill(false);

			// Clear votes for restart on that period
			this->m_Votes[i].VoteRestart[State] = false;
		}
	}
}

// Player Disconnect
void CMatchVoteMenu::PlayerDisconnect(edict_t* pEntity)
{
	// If is not null
	if (!FNullEnt(pEntity))
	{
		// Get Player entity Index
		auto EntityIndex = ENTINDEX(pEntity);

		// Reset Player Votes
		this->m_Votes[EntityIndex].Reset();

		// Loop Clients to reset Vote Kick
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			// Reset Vote Kick Count from other players to player
			this->m_Votes[i].VoteKick[EntityIndex] = false;
		}
	}
}

// Main Vote Menu
bool CMatchVoteMenu::Menu(CBasePlayer* Player)
{
	// If is in a valid team
	if (Player->m_iTeam == TERRORIST || Player->m_iTeam == CT)
	{
		// If vote is not started
		if (gMatchBot.GetState() != STATE_START)
		{
			// Get player entity index
			auto EntityIndex = Player->entindex();

			// Menu title
			gMatchMenu[EntityIndex].Create(_T("Player Vote Menu:"), true, (void*)this->MenuHandle);

			// Vote Kick
			gMatchMenu[EntityIndex].AddItem(0, _T("Vote Kick"));

			// Vote Map
			gMatchMenu[EntityIndex].AddItem(1, _T("Vote Map"));

			// Vote Timeout
			gMatchMenu[EntityIndex].AddItem(2, _T("Vote Tactical Timeout"));

			// Vote Restart
			gMatchMenu[EntityIndex].AddItem(3, _T("Vote Restart Match"));

			// Vote Surrender
			gMatchMenu[EntityIndex].AddItem(4, _T("Vote Surrender"));

			// Vote Stop
			gMatchMenu[EntityIndex].AddItem(5, _T("Vote to Cancel The Match"));

			// Show menu
			gMatchMenu[EntityIndex].Show(EntityIndex);

			// Return result
			return true;
		}
	}

	// Show error message
	gMatchUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("Unable to use this command now."));

	// Return result
	return false;
}

// Main Vote Menu Handle
void CMatchVoteMenu::MenuHandle(int EntityIndex, P_MENU_ITEM Item)
{
	// Get player by entity index
	auto Player = UTIL_PlayerByIndexSafe(EntityIndex);

	// If found
	if (Player)
	{
		// Switch info
		switch(Item.Info)
		{
			case 0: // Vote Kick
			{
				gMatchVoteMenu.VoteKick(Player);
				break;
			}
			case 1: // Vote Map
			{
				gMatchVoteMenu.VoteMap(Player);
				break;
			}
			case 2: // Vote Pause
			{
				gMatchVoteMenu.VotePause(Player);
				break;
			}
			case 3: // Vote Restart
			{
				gMatchVoteMenu.VoteRestart(Player);
				break;
			}
			case 4: // Vote Surrender
			{
				gMatchVoteMenu.VoteSurrender(Player);
				break;
			}
			case 5: // Vote Stop
			{
				gMatchVoteMenu.VoteStop(Player);
				break;
			}
		}
	}
}

// Vote Kick Menu
bool CMatchVoteMenu::VoteKick(CBasePlayer* Player)
{
	// If player is in game
	if (Player->m_iTeam == TERRORIST || Player->m_iTeam == CT)
	{
		// If vote state is not running
		if (gMatchBot.GetState() != STATE_START)
		{
			// Create menu
			gMatchMenu[Player->entindex()].Create(_T("Vote Kick:"), true, (void*)this->VoteKickHandle);

			// Get players from player team
			auto Players = gMatchUtil.GetPlayers(Player->m_iTeam, true);

			// If we have the needed players to vote kick
			if (gMatchBot.m_PlayerVoteKick->value && (Players.size() >= (size_t)round(gMatchBot.m_PlayerVoteKick->value)))
			{
				// Get the total of votes needed to kick (Subtract the player itself vote)
				auto VotesNeed = static_cast<float>(Players.size() - 1);

				// Loop players
				for (auto Target : Players)
				{
					// If is not equal the player
					if (Target->entindex() != Player->entindex())
					{
						// If is not admin immunity
						if (!gMatchAdmin.Access(Target->entindex(), ADMIN_IMMUNITY))
						{
							// Get percentage of vote count
							auto VoteCount = 0.0f;

							// Loop client votes
							for (int i = 1; i <= gpGlobals->maxClients; ++i)
							{
								// Check if voted
								if (this->m_Votes[i].VoteKick[Target->entindex()])
								{
									VoteCount += 1.0f;
								}
							}

							// Add to menu
							gMatchMenu[Player->entindex()].AddItem(Target->entindex(), gMatchUtil.FormatString("%s \\y\\R%2.0f%%", STRING(Target->edict()->v.netname), ((VoteCount * 100.0f) / VotesNeed)), this->m_Votes[Player->entindex()].VoteKick[Target->entindex()]);
						}
					}
				}
				
				// Show Menu
				gMatchMenu[Player->entindex()].Show(Player->entindex());
			}
			else
			{
				// Message
				gMatchUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("We need ^3%d^1 players in ^3%s^1 team to Vote Kick."), (int)(gMatchBot.m_PlayerVoteKick->value), gMatchBot.GetTeam(Player->m_iTeam, false));
			}

			return true;
		}
	}
	
	// Failed command
	gMatchUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("Unable to use this command now."));

	return false;
}

void CMatchVoteMenu::VoteKickHandle(int EntityIndex, P_MENU_ITEM Item)
{
	auto Player = UTIL_PlayerByIndexSafe(EntityIndex);

	if (Player)
	{
		auto Target = UTIL_PlayerByIndexSafe(Item.Info);

		if (Target)
		{
			auto PlayerIndex = Player->entindex();
			auto TargetIndex = Target->entindex();

			if (!Item.Disabled)
			{
				gMatchVoteMenu.m_Votes[PlayerIndex].VoteKick[TargetIndex] = true;

				// Get percentage of vote count
				auto VoteCount = 0.0f;

				// Loop clients
				for (int i = 1; i <= gpGlobals->maxClients; ++i)
				{
					// Check if voted
					if (gMatchVoteMenu.m_Votes[i].VoteKick[Target->entindex()])
					{
						VoteCount += 1.0f;
					}
				}

				// Get votes needed
				auto VotesNeed = static_cast<float>(gMatchUtil.GetCount(Player->m_iTeam) - 1);

				// Vote Map Progress to change map
				auto VoteProgress = ((VoteCount * 100.0f) / VotesNeed);

				// If reached count
				if (VoteProgress < 100.0f)
				{
					// Send vote kick messages
					gMatchUtil.SayText(nullptr, PlayerIndex, _T("^3%s^1 voted to kick ^3%s^1: %2.0f%% of votes to kick."), STRING(Player->edict()->v.netname), STRING(Target->edict()->v.netname), VoteProgress);

					// Get command info
					auto lpCommand = gMatchCommand.GetInfo(CMD_PLAYER_VOTE_KICK);

					// If is not null
					if (lpCommand)
					{
						// Show message
						gMatchUtil.SayText(nullptr, PlayerIndex, _T("Say ^3%s^1 to open vote kick."), lpCommand->Name.c_str());
					}
				}
				else
				{
					// Kick Player
					gMatchUtil.ServerCommand("kick #%d %s", GETPLAYERUSERID(Target->edict()), _T("Kicked by Vote Kick."));

					// Send Mesasge
					gMatchUtil.SayText(nullptr, TargetIndex, _T("^3%s^1 Kicked: ^4%d^1 votes reached."), STRING(Target->edict()->v.netname), VotesNeed);
				}
			}
			else
			{
				gMatchUtil.SayText(Player->edict(), TargetIndex, _T("Already voted to kick: ^3%s^1..."), STRING(Target->edict()->v.netname));
			}
		}
	}
}

// Vote Map Menu
bool CMatchVoteMenu::VoteMap(CBasePlayer* Player)
{
	// If player is in game
	if (Player->m_iTeam == TERRORIST || Player->m_iTeam == CT)
	{
		// If vote state is not running
		if (gMatchBot.GetState() != STATE_START)
		{
			// Get Players
			auto Players = gMatchUtil.GetPlayers(true, true);

			// If player count match
			if (gMatchBot.m_PlayerVoteMap->value && (Players.size() >= (size_t)round(gMatchBot.m_PlayerVoteMap->value)))
			{
				// Create menu
				gMatchMenu[Player->entindex()].Create(_T("Vote Map:"), true, (void*)this->VoteMapHandle);

				// Needed votes
				auto VotesNeed = (static_cast<float>(Players.size()) * gMatchBot.m_VotePercent->value);

				// Loop Map List
				for (auto const& Map : this->m_MapList)
				{
					// Get Vote Count
					auto VoteCount = 0.0f;

					// Loop in game players
					for (const auto& Temp : Players)
					{
						// If has voted on that map
						if (this->m_Votes[Temp->entindex()].VoteMap[Map.first])
						{
							// Count vote
							VoteCount += 1.0f;
						}
					}

					// Add Menu Item
					gMatchMenu[Player->entindex()].AddItem(Map.first, gMatchUtil.FormatString("%s \\y\\R%2.0f%%", Map.second.c_str(), ((VoteCount * 100.0f) / VotesNeed)), this->m_Votes[Player->entindex()].VoteMap[Map.first]);
				}

				// Show Menu to player
				gMatchMenu[Player->entindex()].Show(Player->entindex());

				return true;
			}
			else
			{
				gMatchUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("%d players is needed to enable that command."), (int)(gMatchBot.m_PlayerVoteMap->value));
			}
		}
	}

	// Failed command
	gMatchUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("Unable to use this command now."));

	return false;
}

// Vote Map Menu Handle
void CMatchVoteMenu::VoteMapHandle(int EntityIndex, P_MENU_ITEM Item)
{
	// Get player by entity index
	auto Player = UTIL_PlayerByIndexSafe(EntityIndex);

	// If found
	if (Player)
	{
		// Get Voted Map
		auto MapName = gMatchVoteMenu.m_MapList[Item.Info];

		// If can vote in that map
		if (!Item.Disabled)
		{
			// Add Vote
			gMatchVoteMenu.m_Votes[Player->entindex()].VoteMap[Item.Info] = true;

			// Get Players
			auto Players = gMatchUtil.GetPlayers(true, true);

			// Needed votes
			auto VotesNeed = (static_cast<float>(Players.size()) * gMatchBot.m_VotePercent->value);

			// Get Vote Count
			auto VoteCount = 0.0f;

			// Loop in game players
			for (const auto& Temp : Players)
			{
				// If has voted on that map
				if (gMatchVoteMenu.m_Votes[Temp->entindex()].VoteMap[Item.Info])
				{
					// Count vote
					VoteCount += 1.0f;
				}
			}

			// Vote Map Progress to change map
			auto VoteProgress = ((VoteCount * 100.0f) / VotesNeed);

			// If need more votes to change map
			if (VoteProgress < 100.0f)
			{
				// Send messages
				gMatchUtil.SayText(nullptr, Player->entindex(), _T("^3%s^1 nomitated ^4%s^1: %2.0f%% of votes to change map."), STRING(Player->edict()->v.netname), MapName.c_str(), VoteProgress);

				// Get command info
				auto lpCommand = gMatchCommand.GetInfo(CMD_PLAYER_VOTE_MAP);

				// If is not null
				if (lpCommand)
				{
					// Show message
					gMatchUtil.SayText(nullptr, Player->entindex(), _T("Say ^3%s^1 to nominate a map."), lpCommand->Name.c_str());
				}
			}
			else
			{
				// Remove Vote Map Variable
				g_engfuncs.pfnCvar_DirectSet(gMatchBot.m_VoteMap, "0");

				// Change map to new level
				gMatchChangeMap.ChangeMap(MapName, 5.0f, true);
				
				// Send message
				gMatchUtil.SayText(nullptr, Player->entindex(), _T("Changing map to ^4%s^1..."), MapName.c_str());
			}
		}
		else
		{
			// Send error message
			gMatchUtil.SayText(Player->edict(), Player->entindex(), _T("Already nominated ^3%s^1..."), MapName.c_str());
		}
	}
}

// Vote Pause
bool CMatchVoteMenu::VotePause(CBasePlayer* Player)
{
	// If player is in game
	if (Player->m_iTeam == TERRORIST || Player->m_iTeam == CT)
	{
		// If vote state is not running
		if (gMatchBot.GetState() == STATE_FIRST_HALF || gMatchBot.GetState() == STATE_SECOND_HALF || gMatchBot.GetState() == STATE_OVERTIME)
		{
			// If pause timer is enabled and pause limit have value
			if ((gMatchBot.m_PauseTime->value > 0.0f) && (gMatchBot.m_PlayerVotePause->value > 0.0f))
			{
				// Get Players
				auto Players = gMatchUtil.GetPlayers(Player->m_iTeam, true);

				// If player count match
				if (gMatchBot.m_PlayerVotePause->value && (Players.size() >= (size_t)round(gMatchBot.m_PlayerVotePause->value)))
				{
					// If has CS GameRules
					if (g_pGameRules)
					{
						// If is not in freeze time period
						if (!CSGameRules()->IsFreezePeriod())
						{
							// If player not voted yet
							if (!this->m_Votes[Player->entindex()].VotePause[Player->m_iTeam])
							{
								// Add Vote
								this->m_Votes[Player->entindex()].VotePause[Player->m_iTeam] = true;

								// Needed votes
								auto VotesNeed = (static_cast<float>(Players.size()) * gMatchBot.m_VotePercent->value);

								// Get Vote Count
								auto VoteCount = 0.0f;

								// Loop Players in team
								for (const auto& Temp : Players)
								{
									// Count Vote if is true
									if (this->m_Votes[Temp->entindex()].VotePause[Temp->m_iTeam])
									{
										VoteCount += 1.0f;
									}
								}

								// Vote Map Progress to change map
								auto VoteProgress = ((VoteCount * 100.0f) / VotesNeed);

								// If need more votes to change map
								if (VoteProgress < 100.0f)
								{
									// Send messages
									gMatchUtil.SayText(nullptr, Player->entindex(), _T("^3%s^1 from ^3%s^1 voted for pause match: %2.0f%% of votes to pause match."), STRING(Player->edict()->v.netname), gMatchBot.GetTeam(Player->m_iTeam, false), VoteProgress);

									// Get command info
									auto lpCommand = gMatchCommand.GetInfo(CMD_PLAYER_VOTE_PAUSE);

									// If is not null
									if (lpCommand)
									{
										// Show message
										gMatchUtil.SayText(nullptr, Player->entindex(), _T("Say ^3%s^1 to vote for a pause."), lpCommand->Name.c_str());
									}
								}
								else
								{
									// Pause Match
									gMatchPause.Init(nullptr, Player->m_iTeam);
								}
							}
							else
							{
								// Send error message
								gMatchUtil.SayText(Player->edict(), Player->entindex(), _T("You already voted to pause the game."));
							}

							// Return result
							return true;
						}
					}
				}
				else
				{
					// Send message
					gMatchUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("%d players is needed to enable that command."), static_cast<int>(gMatchBot.m_PlayerVotePause->value));
				}
			}
		}
	}

	// Failed command
	gMatchUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("Unable to use this command now."));

	// Return result
	return false;
}

// Vote Restart
bool CMatchVoteMenu::VoteRestart(CBasePlayer* Player)
{
	// If player is in game
	if (Player->m_iTeam == TERRORIST || Player->m_iTeam == CT)
	{
		// Get Players
		auto Players = gMatchUtil.GetPlayers(true, true);

		// If player count match
		if (gMatchBot.m_PlayerVoteRestart->value && (Players.size() >= (size_t)round(gMatchBot.m_PlayerVoteRestart->value)))
		{
			// Get Match BOT State
			auto MatchState = gMatchBot.GetState();

			// If state is valid
			if (MatchState == STATE_FIRST_HALF)
			{
				// If not voted to restart this state
				if (!this->m_Votes[Player->entindex()].VoteRestart[MatchState])
				{
					// Set player vote on this state to true
					this->m_Votes[Player->entindex()].VoteRestart[MatchState] = true;

					// Needed votes
					auto VotesNeed = (static_cast<float>(Players.size()) * gMatchBot.m_VotePercent->value);

					// Get Vote Count
					auto VoteCount = 0.0f;

					// Loop in game Players
					for (const auto& Temp : Players)
					{
						// If has voted to restart this state
						if (this->m_Votes[Temp->entindex()].VoteRestart[MatchState])
						{
							VoteCount += 1.0f;
						}
					}

					// Vote Restart Progress to restart current period
					auto VoteProgress = ((VoteCount * 100.0f) / VotesNeed);

					// If need more votes to restart period
					if (VoteProgress < 100.0f)
					{
						// Send messages
						gMatchUtil.SayText(nullptr, Player->entindex(), _T("^3%s^1 voted to restart ^3%s^1: %2.0f%% of votes to restart match."), STRING(Player->edict()->v.netname), gMatchBot.GetState(MatchState), VoteProgress);

						// Get command info
						auto lpCommand = gMatchCommand.GetInfo(CMD_PLAYER_VOTE_RESTART);

						// If is not null
						if (lpCommand)
						{
							// Show message
							gMatchUtil.SayText(nullptr, Player->entindex(), _T("Say ^3%s^1 to restart match."), lpCommand->Name.c_str());
						}
					}
					else
					{
						// Send Mesasge
						gMatchUtil.SayText(nullptr, PRINT_TEAM_DEFAULT, _T("Restarting match.."));

						// Restart Match
						gMatchBot.RestartMatch(nullptr);
					}
				}
				else
				{
					// Send error message
					gMatchUtil.SayText(Player->edict(), Player->entindex(), _T("You already voted to retart match."), gMatchBot.GetState(MatchState));
				}

				return true;
			}
		}
		else
		{
			// Send message
			gMatchUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("%d players is needed to enable that command."), (int)(gMatchBot.m_PlayerVoteRestart->value));
		}
	}

	// Failed command
	gMatchUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("Unable to use this command now."));

	// Return result
	return false;
}

// Vote Surrander
bool CMatchVoteMenu::VoteSurrender(CBasePlayer* Player)
{
	// If player is in game
	if (Player->m_iTeam == TERRORIST || Player->m_iTeam == CT)
	{
		// If vote state is not running
		if (gMatchBot.GetState() == STATE_FIRST_HALF || gMatchBot.GetState() == STATE_SECOND_HALF || gMatchBot.GetState() == STATE_OVERTIME)
		{
			// If vote surrender is enabled
			if (gMatchBot.m_PlayerVoteSurrender->value > 0.0f)
			{
				// Get Players
				auto Players = gMatchUtil.GetPlayers(Player->m_iTeam, true);

				// If player count match
				if (Players.size() >= static_cast<size_t>(gMatchBot.m_PlayerVoteSurrender->value))
				{
					// If player not voted yet
					if (!this->m_Votes[Player->entindex()].VoteSurrender[Player->m_iTeam])
					{
						// Add Vote
						this->m_Votes[Player->entindex()].VoteSurrender[Player->m_iTeam] = true;

						// Needed votes
						auto VotesNeed = (static_cast<float>(Players.size()) * gMatchBot.m_VotePercent->value);

						// Get Vote Count
						auto VoteCount = 0.0f;

						// Loop Players in team
						for (const auto& Temp : Players)
						{
							// Count Vote if is true
							if (this->m_Votes[Temp->entindex()].VoteSurrender[Temp->m_iTeam])
							{
								VoteCount += 1.0f;
							}
						}

						// Vote Map Progress to change map
						auto VoteProgress = ((VoteCount * 100.0f) / VotesNeed);

						// If need more votes to change map
						if (VoteProgress < 100.0f)
						{
							// Send messages
							gMatchUtil.SayText(nullptr, Player->entindex(), _T("^3%s^1 from ^3%s^1 voted for surrender: %2.0f%% of votes to surrender match."), STRING(Player->edict()->v.netname), gMatchBot.GetTeam(Player->m_iTeam, false), VoteProgress);

							// Get command info
							auto lpCommand = gMatchCommand.GetInfo(CMD_PLAYER_VOTE_SURRENDER);

							// If is not null
							if (lpCommand)
							{
								// Show message
								gMatchUtil.SayText(nullptr, Player->entindex(), _T("Say ^3%s^1 to vote for a surrender."), lpCommand->Name.c_str());
							}
						}
						else
						{
							// Stop Match with a loser and winner
							gMatchBot.EndMatch(Player->m_iTeam, (Player->m_iTeam == TERRORIST) ? CT : TERRORIST);
						}
					}
					else
					{
						// Send error message
						gMatchUtil.SayText(Player->edict(), Player->entindex(), _T("You already voted to surrender the game."));
					}
				}
				else
				{
					// Send message
					gMatchUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("%d players is needed to enable that command."), static_cast<int>(gMatchBot.m_PlayerVoteSurrender->value));
				}

				// Return result
				return true;
			}
		}
	}

	// Failed command
	gMatchUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("Unable to use this command now."));

	// Return result
	return false;
}

// Vote Stop
bool CMatchVoteMenu::VoteStop(CBasePlayer* Player)
{
	// If player is in game
	if (Player->m_iTeam == TERRORIST || Player->m_iTeam == CT)
	{
		// Get Players
		auto Players = gMatchUtil.GetPlayers(true, true);

		// If player count match
		if (Players.size() > 0)
		{
			// Get Match BOT State
			auto MatchState = gMatchBot.GetState();

			// If state is valid
			if(MatchState >= STATE_FIRST_HALF && MatchState <= STATE_OVERTIME)
			{
				// If not voted to stop this state
				if (!this->m_Votes[Player->entindex()].VoteStop[MatchState])
				{
					// Set player vote on this state to true
					this->m_Votes[Player->entindex()].VoteStop[MatchState] = true;

					// Needed votes
					auto VotesNeed = (static_cast<float>(Players.size()) * gMatchBot.m_VotePercent->value);

					// Get Vote Count
					auto VoteCount = 0.0f;

					// Loop in game Players
					for (const auto& Temp : Players)
					{
						// If has voted to restart this state
						if (this->m_Votes[Temp->entindex()].VoteStop[MatchState])
						{
							VoteCount += 1.0f;
						}
					}

					// Vote Restart Progress to restart current period
					auto VoteProgress = ((VoteCount * 100.0f) / VotesNeed);

					// If need more votes to restart period
					if (VoteProgress < 100.0f)
					{
						// Send messages
						gMatchUtil.SayText(nullptr, Player->entindex(), _T("^3%s^1 voted to cancel the match: %2.0f%% of votes to cancel the match."), STRING(Player->edict()->v.netname), VoteProgress);

						// Get command info
						auto lpCommand = gMatchCommand.GetInfo(CMD_PLAYER_VOTE_STOP);

						// If is not null
						if (lpCommand)
						{
							// Show message
							gMatchUtil.SayText(nullptr, Player->entindex(), _T("Say ^3%s^1 to cancel the match."), lpCommand->Name.c_str());
						}
					}
					else
					{
						// Send Mesasge
						gMatchUtil.SayText(nullptr, PRINT_TEAM_DEFAULT, _T("Canceling match..."));

						// Restart Match
						gMatchBot.StopMatch(nullptr);
					}
				}
				else
				{
					// Send error message
					gMatchUtil.SayText(Player->edict(), Player->entindex(), _T("You already voted to cancel the match."), gMatchBot.GetState(MatchState));
				}

				return true;
			}
		}
	}

	// Failed command
	gMatchUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, _T("Unable to use this command now."));

	// Return result
	return false;
}