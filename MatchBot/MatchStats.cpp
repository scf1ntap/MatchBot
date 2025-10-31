#include "precompiled.h"

CMatchStats gMatchStats;

// On match bot new state
void CMatchStats::SetState(int State, bool KnifeRound)
{
	// Set current state
	this->m_State = State;

	// Switch states
	switch (this->m_State)
	{
		case STATE_DEAD:
		{
			// Clear all player data
			this->m_Player.clear();
			break;
		}
		case STATE_WARMUP:
		{
			// Reset match data
			this->m_Match.Reset();

			// Reset round event data
			this->m_Event.clear();

			// Loop each player
			for (auto& Player : this->m_Player)
			{
				// Clear Chat Log
				Player.second.Chat.clear();

				// Clear player round stats
				Player.second.Round.Reset();
			}
			break;
		}
		case STATE_START:
		{
			// Reset match data
			this->m_Match.Reset();

			// Reset round event data
			this->m_Event.clear();
			break;
		}
		case STATE_FIRST_HALF:
		{
			// Set Hostname
			this->m_Match.HostName = (gMatchBot.m_Hostname) ? gMatchBot.m_Hostname->string : "";

			// Set Map
			this->m_Match.Map = STRING(gpGlobals->mapname);

			// Set address
			this->m_Match.Address = (gMatchBot.m_NetAddress) ? gMatchBot.m_NetAddress->string : "";

			// Set match start time
			this->m_Match.Time[0] = time(nullptr);

			// Set match end time
			this->m_Match.Time[1] = 0;

			// Reset game mode
			this->m_Match.GameMode = gMatchVoteTeam.GetMode();

			// Match has Knife Round
			this->m_Match.KnifeRound = KnifeRound;

			// Reset round event data
			this->m_Event.clear();

			// Loop each player
			for (auto& Player : this->m_Player)
			{
				// Clear player stats
				Player.second.Stats[State].Reset();

				// Clear Chat Log
				Player.second.Chat.clear();

				// Clear player round stats
				Player.second.Round.Reset();
			}
			break;
		}
		case STATE_HALFTIME:
		case STATE_SECOND_HALF:
		case STATE_OVERTIME:
		{
			// Loop each player
			for (auto& Player : this->m_Player)
			{
				// Clear player stats
				Player.second.Stats[State].Reset();

				// Clear player round stats
				Player.second.Round.Reset();
			}
			break;
		}
		case STATE_END:
		{
			// Set match end time
			this->m_Match.Time[1] = time(nullptr);

			// Loop player list
			for (auto& Player : this->m_Player)
			{
				// Clear winner of match
				Player.second.Winner = 0;

				// If is in winner team
				if (Player.second.Team == static_cast<int>(gMatchBot.GetWinner()))
				{
					// Set player as winner team
					Player.second.Winner = 1;
				}
			}
			break;
		}
	}
}

// On player enter in game
void CMatchStats::PlayerGetIntoGame(CBasePlayer* Player)
{
	// Get Steam ID
	auto Auth = gMatchUtil.GetPlayerAuthId(Player->edict());

	// If is not null
	if (Auth)
	{
		// Set joined time
		this->m_Player[Auth].JoinGameTime = time(0);

		// Set name
		this->m_Player[Auth].Name = STRING(Player->edict()->v.netname);

		// Set team
		this->m_Player[Auth].Team = static_cast<int>(Player->m_iTeam);
	}
}

// On player disconnect
void CMatchStats::PlayerDisconnect(edict_t* pEntity)
{
	// Get Steam ID
	auto Auth = gMatchUtil.GetPlayerAuthId(pEntity);

	// If is not null
	if (Auth)
	{
		// Set disconnect time
		this->m_Player[Auth].DisconnectTime = time(0);
	}
}

// On player switch team
void CMatchStats::PlayerSwitchTeam(CBasePlayer* Player)
{
	// Get Steam ID
	auto Auth = gMatchUtil.GetPlayerAuthId(Player->edict());

	// If is not null
	if (Auth)
	{
		// Set name
		this->m_Player[Auth].Name = STRING(Player->edict()->v.netname);

		// Set team
		this->m_Player[Auth].Team = static_cast<int>(Player->m_iTeam);
	}
}

// On player respawn
void CMatchStats::PlayerRespawn(CBasePlayer* Player)
{
	// If match is not dead
	if (this->m_State != STATE_DEAD)
	{
		// Get Steam ID
		auto Auth = gMatchUtil.GetPlayerAuthId(Player->edict());

		// If is not null
		if (Auth)
		{
			// Reset round stats
			this->m_Player[Auth].Round.Reset();
		}
	}
}

// On Round Restart
void CMatchStats::RoundRestart()
{
	// If match is not dead
	if (this->m_State != STATE_DEAD)
	{
		// If has CSGameRules
		if (g_pGameRules)
		{
			// If is complete reset
			if (CSGameRules()->m_bCompleteReset)
			{
				// Loop all saved players
				for (auto& Player : this->m_Player)
				{
					// Reset player stats of this state
					Player.second.Stats[this->m_State].Reset();

					// Reset round stats
					Player.second.Round.Reset();
				}
			}
		}
	}
}

// On send death message
void CMatchStats::SendDeathMessage(CBaseEntity* KillerEntity, CBasePlayer* Victim, CBasePlayer* Assister, entvars_t* pevInflictor, const char* killerWeaponName, int iDeathMessageFlags, int iRarityOfKill)
{
	// If match is live
	if ((this->m_State == STATE_FIRST_HALF) || (this->m_State == STATE_SECOND_HALF) || (this->m_State == STATE_OVERTIME))
	{
		// Item Index
		auto ItemIndex = WEAPON_NONE;

		// If has killer entity
		if (KillerEntity)
		{
			// If is player
			if (KillerEntity->IsPlayer())
			{
				// Get CBasePlayer
				auto Killer = static_cast<CBasePlayer*>(KillerEntity);

				// Get auth id
				auto KillerAuth = gMatchUtil.GetPlayerAuthId(Killer->edict());

				// If is not null
				if (KillerAuth)
				{
					// If is not killed self
					if (Killer->entindex() != Victim->entindex())
					{
						// Set Item Index
						ItemIndex = static_cast<WeaponIdType>((Victim->m_bKilledByGrenade) ? WEAPON_HEGRENADE : ((Killer->m_pActiveItem) ? Killer->m_pActiveItem->m_iId : WEAPON_NONE));

						// Set frags
						this->m_Player[KillerAuth].Stats[this->m_State].Frags++;

						// Set weapon frags
						this->m_Player[KillerAuth].Stats[this->m_State].Weapon[ItemIndex].Frags++;

						// Set round frags
						this->m_Player[KillerAuth].Round.Frags++;

						// If has player assistant
						if (iDeathMessageFlags & PLAYERDEATH_ASSISTANT)
						{
							// If is not null
							if (Assister)
							{
								// If is player
								if (Assister->IsPlayer())
								{
									// Get auth id
									auto AssisterAuth = gMatchUtil.GetPlayerAuthId(Assister->edict());

									// If is not null
									if (AssisterAuth)
									{
										// Set assists
										this->m_Player[AssisterAuth].Stats[this->m_State].Assists++;
									}
								}

							}
						}

						// Is Headshot
						if (iRarityOfKill & KILLRARITY_HEADSHOT)
						{
							// Set headshots
							this->m_Player[KillerAuth].Stats[this->m_State].Headshots++;

							// Set weapon headshots
							this->m_Player[KillerAuth].Stats[this->m_State].Weapon[ItemIndex].Headshots++;

							// Set round headshots
							this->m_Player[KillerAuth].Round.Headshots++;
						}

						// Is blind
						if (iRarityOfKill & KILLRARITY_KILLER_BLIND)
						{
							// Set blind frags
							this->m_Player[KillerAuth].Stats[this->m_State].BlindFrags++;
						}

						// Is no scoped
						if (iRarityOfKill & KILLRARITY_NOSCOPE)
						{
							// Set no scoped frags
							this->m_Player[KillerAuth].Stats[this->m_State].NoScope++;
						}

						// Is penetrated frag
						if (iRarityOfKill & KILLRARITY_PENETRATED)
						{
							// Set wall frags
							this->m_Player[KillerAuth].Stats[this->m_State].WallFrags++;
						}

						// Is through smoke
						if (iRarityOfKill & KILLRARITY_THRUSMOKE)
						{
							// Set smoke frags
							this->m_Player[KillerAuth].Stats[this->m_State].SmokeFrags++;
						}

						// Is assisted flash
						if (iRarityOfKill & KILLRARITY_ASSISTEDFLASH)
						{
							// Set assisted flash frags
							this->m_Player[KillerAuth].Stats[this->m_State].AssistedFlash++;
						}

						// Is domination
						if (iRarityOfKill & KILLRARITY_DOMINATION_BEGAN)
						{
							// Set domination
							this->m_Player[KillerAuth].Stats[this->m_State].Dominations++;
						}

						// Is domination
						if (iRarityOfKill & KILLRARITY_DOMINATION)
						{
							// Set domination
							this->m_Player[KillerAuth].Stats[this->m_State].DominationRepeats++;
						}

						// Is revenge
						if (iRarityOfKill & KILLRARITY_REVENGE)
						{
							// Set revenge
							this->m_Player[KillerAuth].Stats[this->m_State].Revenges++;
						}

						// TEST
						if (!Killer->IsBot())
						{
							LOG_CONSOLE
							(
								PLID,
								"[%s][%s] F %d A %d AF %d D %d HS %d",
								__func__,
								KillerAuth,
								this->m_Player[KillerAuth].Stats[this->m_State].Frags,
								this->m_Player[KillerAuth].Stats[this->m_State].Assists,
								this->m_Player[KillerAuth].Stats[this->m_State].AssistedFlash,
								this->m_Player[KillerAuth].Stats[this->m_State].Deaths,
								this->m_Player[KillerAuth].Stats[this->m_State].Headshots
							);
						}
					}
					else
					{
						// Set suicides
						this->m_Player[KillerAuth].Stats[this->m_State].Suicides++;
					}
				}
			}
		}

		// If victim is player
		if (Victim->IsPlayer())
		{
			// Get auth id
			auto VictimAuth = gMatchUtil.GetPlayerAuthId(Victim->edict());

			// If is not null
			if (VictimAuth)
			{
				// Set deaths
				this->m_Player[VictimAuth].Stats[this->m_State].Deaths++;

				// Set weapon deaths
				this->m_Player[VictimAuth].Stats[this->m_State].Weapon[ItemIndex].Deaths++;

				// Set round deaths
				this->m_Player[VictimAuth].Round.Deaths++;
			}
		}
	}
}

// On BOT manager event
void CMatchStats::CBotManager_OnEvent(GameEventType GameEvent, CBaseEntity* pEntity, CBaseEntity* pOther)
{
	// If match is live
	if ((this->m_State == STATE_FIRST_HALF) || (this->m_State == STATE_SECOND_HALF) || (this->m_State == STATE_OVERTIME))
	{
		// Switch event
		switch (GameEvent)
		{
			case EVENT_WEAPON_FIRED: // tell bots the player is attack (argumens: 1 = attacker, 2 = NULL)
			{
				// If entity is not null
				if (!FNullEnt(pEntity))
				{
					// Cast to CBasePlayer
					auto Player = static_cast<CBasePlayer*>(pEntity);

					// If is not null
					if (Player)
					{
						// Get Steam ID
						auto Auth = gMatchUtil.GetPlayerAuthId(Player->edict());

						// If is not null
						if (Auth)
						{
							// Player weapon fired count
							this->m_Player[Auth].Stats[this->m_State].WeaponFired++;
						}
					}
				}
				break;
			}
			case EVENT_WEAPON_FIRED_ON_EMPTY: // tell bots the player is attack without clip ammo (argumens: 1 = attacker, 2 = NULL)
			{
				// If entity is not null
				if (!FNullEnt(pEntity))
				{
					// Cast to CBasePlayer
					auto Player = static_cast<CBasePlayer*>(pEntity);

					// If is not null
					if (Player)
					{
						// Get Steam ID
						auto Auth = gMatchUtil.GetPlayerAuthId(Player->edict());

						// If is not null
						if (Auth)
						{
							// Player blind count
							this->m_Player[Auth].Stats[this->m_State].WeaponFiredEmpty++;
						}
					}
				}
				break;
			}
			case EVENT_WEAPON_RELOADED: // tell bots the player is reloading his weapon (argumens: 1 = reloader, 2 = NULL)
			{
				// If entity is not null
				if (!FNullEnt(pEntity))
				{
					// Cast to CBasePlayer
					auto Player = static_cast<CBasePlayer*>(pEntity);

					// If is not null
					if (Player)
					{
						// Get Steam ID
						auto Auth = gMatchUtil.GetPlayerAuthId(Player->edict());

						// If is not null
						if (Auth)
						{
							// Player blind count
							this->m_Player[Auth].Stats[this->m_State].WeaponReloaded++;
						}
					}
				}
				break;
			}
			case EVENT_PLAYER_BLINDED_BY_FLASHBANG:
			{
				// If entity is not null
				if (!FNullEnt(pEntity))
				{
					// Cast to CBasePlayer
					auto Player = static_cast<CBasePlayer*>(pEntity);

					// If is not null
					if (Player)
					{
						// If is blind
						if (Player->IsBlind())
						{
							// If is full blind
							if (Player->m_blindAlpha >= 255)
							{
								// Get Steam ID
								auto Auth = gMatchUtil.GetPlayerAuthId(Player->edict());

								// If is not null
								if (Auth)
								{
									// Player blind count
									this->m_Player[Auth].Stats[this->m_State].Blind++;
								}
							}
						}
					}
				}
				break;
			}
			case EVENT_PLAYER_FOOTSTEP:
			{
				// If entity is not null
				if (!FNullEnt(pEntity))
				{
					// Cast to CBasePlayer
					auto Player = static_cast<CBasePlayer*>(pEntity);

					// If is not null
					if (Player)
					{
						// Get player move vars
						auto pMove = g_ReGameApi->GetPlayerMove();

						// If is not null
						if (pMove)
						{
							// If steps left is equal of next step sound
							if (pMove->iStepLeft == pMove->flTimeStepSound)
							{
								// Get Steam ID
								auto Auth = gMatchUtil.GetPlayerAuthId(Player->edict());

								// If is not null
								if (Auth)
								{
									// Player footsteps count
									this->m_Player[Auth].Stats[this->m_State].Footsteps++;
								}
							}
						}
					}
				}
				break;
			}
			case EVENT_PLAYER_JUMPED:
			{
				// If entity is not null
				if (!FNullEnt(pEntity))
				{
					// Cast to CBasePlayer
					auto Player = static_cast<CBasePlayer*>(pEntity);

					// If is not null
					if (Player)
					{
						// Get Steam ID
						auto Auth = gMatchUtil.GetPlayerAuthId(Player->edict());

						// If is not null
						if (Auth)
						{
							// Player jump count
							this->m_Player[Auth].Stats[this->m_State].Jumps++;
						}
					}
				}
				break;
			}
			case EVENT_PLAYER_LANDED_FROM_HEIGHT:
			{
				// If entity is not null
				if (!FNullEnt(pEntity))
				{
					// Cast to CBasePlayer
					auto Player = static_cast<CBasePlayer*>(pEntity);

					// If is not null
					if (Player)
					{
						// Get Steam ID
						auto Auth = gMatchUtil.GetPlayerAuthId(Player->edict());

						// If is not null
						if (Auth)
						{
							// Player landed from height count
							this->m_Player[Auth].Stats[this->m_State].LandedFromHeight++;
						}
					}
				}
				break;
			}
		}
	}
}

// On Round event
void CMatchStats::RoundEvent(GameEventType GameEvent, CBasePlayer* Killer, CBasePlayer* Victim)
{
	// Round event data struct
	P_ROUND_EVENT Event = { };

	// Set current round count
	Event.Round = (gMatchBot.GetRound() + 1);

	// If has CSGameRules class
	if (g_pGameRules)
	{
		// Set round remaining time
		Event.Time = CSGameRules()->GetRoundRemainingTimeReal();
	}

	// Set what event is
	Event.GameEvent = GameEvent;

	// Round scenario event
	Event.ScenarioEvent = ROUND_NONE;

	// Switch events
	switch (GameEvent)
	{
		case EVENT_PLAYER_DIED:
		{
			// If is not null
			if (Victim)
			{
				// Set Steam ID
				Event.Victim = gMatchUtil.GetPlayerAuthId(Victim->edict());

				// Set Origin
				Event.VictimOrigin = Victim->edict()->v.origin;

				// Set loser team
				Event.Loser = Victim->m_iTeam;

				// Is headshot
				Event.IsHeadShot = Victim->m_bHeadshotKilled;

				// If is killed by bomb
				if (Victim->m_bKilledByBomb)
				{
					// Set item
					Event.ItemIndex = WEAPON_C4;
				}

				// If is killed by HE
				if (Victim->m_bKilledByGrenade)
				{
					// Set item
					Event.ItemIndex = WEAPON_HEGRENADE;
				}
			}

			// If is not null
			if (Killer)
			{
				// Set Steam ID
				Event.Killer = gMatchUtil.GetPlayerAuthId(Killer->edict());

				// Set origin
				Event.KillerOrigin = Killer->edict()->v.origin;

				// Set winner
				Event.Winner = Killer->m_iTeam;

				// If has active item
				if (Killer->m_pActiveItem)
				{
					// If is not set by last victim
					if (Event.ItemIndex == WEAPON_NONE)
					{
						// Set item index
						Event.ItemIndex = Killer->m_pActiveItem->m_iId;
					}
				}
			}

			break;
		}
		case EVENT_BOMB_PLANTED:
		{
			// If is not null
			if (Killer)
			{
				// Set killer
				Event.Killer = gMatchUtil.GetPlayerAuthId(Killer->edict());

				// Set origin
				Event.KillerOrigin = Killer->edict()->v.origin;
			}

			// Set victim
			Event.Victim.clear();

			// Set origin
			Event.VictimOrigin = { 0.0f, 0.0f, 0.0f };

			// Set winner of event
			Event.Winner = TERRORIST;

			// Set loser of event
			Event.Loser = CT;

			// If is headshot (Implement bomb A or bomb B)
			Event.IsHeadShot = false;

			// Set item
			Event.ItemIndex = WEAPON_C4;

			break;
		}
		case EVENT_BOMB_DROPPED:
		{
			// Set killer
			Event.Killer.clear();

			// Set origin
			Event.KillerOrigin = { 0.0f, 0.0f, 0.0f };

			if (Victim)
			{
				// Set victim
				Event.Victim = gMatchUtil.GetPlayerAuthId(Victim->edict());

				// Set origin
				Event.VictimOrigin = Victim->edict()->v.origin;
			}

			// Set winner of event
			Event.Winner = CT;

			// Set loser of event
			Event.Loser = TERRORIST;

			// Is headshot (Picked bomb)
			Event.IsHeadShot = true;

			// Set item index
			Event.ItemIndex = WEAPON_C4;
			break;
		}
		case EVENT_BOMB_PICKED_UP:
		{
			// If is not null
			if (Killer)
			{
				// Set killer
				Event.Killer = gMatchUtil.GetPlayerAuthId(Killer->edict());

				// Set origin
				Event.KillerOrigin = Killer->edict()->v.origin;
			}

			// Set victim
			Event.Victim.clear();

			// Set origin
			Event.VictimOrigin = { 0.0f, 0.0f, 0.0f };

			// Set winner of event
			Event.Winner = TERRORIST;

			// Set loser of event
			Event.Loser = CT;

			// Is headshot (Picked bomb)
			Event.IsHeadShot = true;

			// Set item index
			Event.ItemIndex = WEAPON_C4;

			break;
		}
		case EVENT_BOMB_DEFUSED:
		{
			// If is not null
			if (Killer)
			{
				// Set killer
				Event.Killer = gMatchUtil.GetPlayerAuthId(Killer->edict());

				// Set origin
				Event.KillerOrigin = Killer->edict()->v.origin;

				// Is headshot (Defuser)
				Event.IsHeadShot = Killer->m_bHasDefuser;
			}

			// Set victim
			Event.Victim.clear();

			// Set origin
			Event.VictimOrigin = { 0.0f, 0.0f, 0.0f };

			// Set scenario event
			Event.ScenarioEvent = ROUND_BOMB_DEFUSED;

			// Set event winner
			Event.Winner = CT;

			// Set event loser
			Event.Loser = TERRORIST;

			// Set item index
			Event.ItemIndex = WEAPON_C4;
			break;
		}
		case EVENT_BOMB_EXPLODED:
		{
			// If is not null
			if (Killer)
			{
				// Set killer
				Event.Killer = gMatchUtil.GetPlayerAuthId(Killer->edict());

				// Set origin
				Event.KillerOrigin = Killer->edict()->v.origin;
			}

			// Set victim
			Event.Victim.clear();

			// Set origin
			Event.VictimOrigin = { 0.0f, 0.0f, 0.0f };

			// Set is headshot
			Event.IsHeadShot = false;

			// Set scenario event
			Event.ScenarioEvent = ROUND_TARGET_BOMB;

			// Set winner
			Event.Winner = TERRORIST;

			// Set loser
			Event.Loser = CT;

			// Set item index
			Event.ItemIndex = WEAPON_C4;
			break;
		}
		case EVENT_TERRORISTS_WIN:
		{
			// Set screnario Event
			Event.ScenarioEvent = ROUND_TERRORISTS_WIN;

			// Set winner
			Event.Winner = TERRORIST;

			// Set loser
			Event.Loser = CT;

			// Set killer
			Event.Killer.clear();

			// Set origin
			Event.KillerOrigin = { 0.0f, 0.0f, 0.0f };

			// Set victim
			Event.Victim.clear();

			// Set origin
			Event.VictimOrigin = { 0.0f, 0.0f, 0.0f };

			// Set is headshot
			Event.IsHeadShot = false;

			// Set item index
			Event.ItemIndex = WEAPON_NONE;
			break;
		}
		case EVENT_CTS_WIN:
		{
			// Set screnario Event
			Event.ScenarioEvent = ROUND_CTS_WIN;

			// Set winner
			Event.Winner = CT;

			// Set loser
			Event.Loser = TERRORIST;

			// Set killer
			Event.Killer.clear();

			// Set origin
			Event.KillerOrigin = { 0.0f, 0.0f, 0.0f };

			// Set victim
			Event.Victim.clear();

			// Set origin
			Event.VictimOrigin = { 0.0f, 0.0f, 0.0f };

			// Set is headshot
			Event.IsHeadShot = false;

			// Set item index
			Event.ItemIndex = WEAPON_NONE;
			break;
		}
	}

	// Push that match event
	this->m_Event.push_back(Event);
}
