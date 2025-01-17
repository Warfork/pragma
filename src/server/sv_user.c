/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sv_user.c -- server code for moving users

#include "server.h"

gentity_t	*sv_player;

void Scr_ClientBegin(gentity_t* self);
void Scr_ClientThink(gentity_t* ent, usercmd_t* ucmd);
void Scr_ClientEndServerFrame(gentity_t* ent);
/*
============================================================

USER STRINGCMD EXECUTION

sv_client and sv_player will be valid.
============================================================
*/

/*
==================
SV_BeginDemoServer
==================
*/
void SV_BeginDemoserver (void)
{
	char		name[MAX_OSPATH];

	Com_sprintf (name, sizeof(name), "demos/%s", sv.name);
	FS_FOpenFile (name, &sv.demofile);
	if (!sv.demofile)
		Com_Error (ERR_DROP, "Couldn't open %s\n", name);
}

/*
================
SV_New_f

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_New_f (void)
{
	char		*gamedir;
	int			playernum;
	gentity_t		*ent;

	Com_DPrintf (DP_SV, "New() from %s\n", sv_client->name);

	if (sv_client->state != cs_connected)
	{
		Com_Printf ("New not valid -- already spawned\n");
		return;
	}

	// demo servers just dump the file message
	if (sv.state == ss_demo)
	{
		SV_BeginDemoserver ();
		return;
	}

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	gamedir = Cvar_VariableString ("gamedir");

	// send the serverdata
	MSG_WriteByte (&sv_client->netchan.message, SVC_SERVERDATA);
	MSG_WriteLong (&sv_client->netchan.message, PROTOCOL_VERSION);
	MSG_WriteLong (&sv_client->netchan.message, svs.spawncount);
	MSG_WriteByte (&sv_client->netchan.message, sv.attractloop);
	MSG_WriteString (&sv_client->netchan.message, gamedir);

	if (sv.state == ss_cinematic || sv.state == ss_pic)
		playernum = -1;
	else
		playernum = sv_client - svs.clients;
	MSG_WriteShort (&sv_client->netchan.message, playernum);

	// send full levelname
	MSG_WriteString (&sv_client->netchan.message, sv.configstrings[CS_NAME]);

	//
	// game server
	// 
	if (sv.state == ss_game)
	{
		// set up the entity for the client
		ent = EDICT_NUM(playernum+1);
		ent->s.number = playernum+1;
		sv_client->edict = ent;
		memset (&sv_client->lastcmd, 0, sizeof(sv_client->lastcmd));

		// begin fetching configstrings
		MSG_WriteByte (&sv_client->netchan.message, SVC_STUFFTEXT);
		MSG_WriteString (&sv_client->netchan.message, va("cmd configstrings %i 0\n",svs.spawncount) );
	}

}

/*
==================
SV_Configstrings_f
==================
*/
void SV_Configstrings_f (void)
{
	int			start;

	Com_DPrintf (DP_SV, "Configstrings() from %s\n", sv_client->name);

	if (sv_client->state != cs_connected)
	{
		Com_Printf ("configstrings not valid -- already spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Com_Printf ("SV_Configstrings_f from different level\n");
		SV_New_f ();
		return;
	}
	
	start = atoi(Cmd_Argv(2));

	// write a packet full of data

	while ( sv_client->netchan.message.cursize < MAX_MSGLEN/2 
		&& start < MAX_CONFIGSTRINGS)
	{
		if (sv.configstrings[start][0])
		{
			MSG_WriteByte (&sv_client->netchan.message, SVC_CONFIGSTRING);
			MSG_WriteShort (&sv_client->netchan.message, start);
			MSG_WriteString (&sv_client->netchan.message, sv.configstrings[start]);
		}
		start++;
	}

	// send next command

	if (start == MAX_CONFIGSTRINGS)
	{
		MSG_WriteByte (&sv_client->netchan.message, SVC_STUFFTEXT);
		MSG_WriteString (&sv_client->netchan.message, va("cmd baselines %i 0\n",svs.spawncount) );
	}
	else
	{
		MSG_WriteByte (&sv_client->netchan.message, SVC_STUFFTEXT);
		MSG_WriteString (&sv_client->netchan.message, va("cmd configstrings %i %i\n",svs.spawncount, start) );
	}
}

/*
==================
SV_Baselines_f
==================
*/
void SV_Baselines_f (void)
{
	int		start;
	entity_state_t	nullstate;
	entity_state_t	*base;

	Com_DPrintf (DP_SV, "Baselines() from %s\n", sv_client->name);

	if (sv_client->state != cs_connected)
	{
		Com_Printf ("baselines not valid -- already spawned\n");
		return;
	}
	
	// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Com_Printf ("SV_Baselines_f from different level\n");
		SV_New_f ();
		return;
	}
	
	start = atoi(Cmd_Argv(2));

	memset (&nullstate, 0, sizeof(nullstate));

	// write a packet full of data

	while ( sv_client->netchan.message.cursize <  MAX_MSGLEN/2
		&& start < MAX_GENTITIES)
	{
		base = &sv.baselines[start];
		if (base->modelindex || base->loopingSound || base->effects)
		{
			MSG_WriteByte (&sv_client->netchan.message, SVC_SPAWNBASELINE);
			MSG_WriteDeltaEntity (&nullstate, base, &sv_client->netchan.message, true, true);
		}
		start++;
	}

	// send next command

	if (start == MAX_GENTITIES)
	{
		MSG_WriteByte (&sv_client->netchan.message, SVC_STUFFTEXT);
		MSG_WriteString (&sv_client->netchan.message, va("precache %i\n", svs.spawncount) );
	}
	else
	{
		MSG_WriteByte (&sv_client->netchan.message, SVC_STUFFTEXT);
		MSG_WriteString (&sv_client->netchan.message, va("cmd baselines %i %i\n",svs.spawncount, start) );
	}
}

/*
==================
SV_Begin_f
==================
*/
void SV_Begin_f (void)
{
	int i;
	gentity_t* ent = NULL;

//	Com_DPrintf (DP_SV, "Begin() from %s\n", sv_client->name);

	// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Com_Printf ("SV_Begin_f from different level\n");
		SV_New_f ();
		return;
	}

	sv_client->state = cs_spawned;

	ent = sv_player;
	ent->client = &svs.gclients[NUM_FOR_EDICT(ent) - 1];

	// if there is already a body waiting for us (a loadgame), just take it, otherwise spawn one from scratch
	if (ent->inuse == true)
	{
		// the client has cleared the client side viewangles upon connecting to the server, which is different than the
		// state when the game is saved, so we need to compensate with deltaangles
		for (i = 0; i < 3; i++)
		{
#if PROTOCOL_FLOAT_PLAYERANGLES == 1
			ent->client->ps.pmove.delta_angles[i] = ent->client->ps.viewangles[i];
#else
			ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->ps.viewangles[i]);
			ent->v.pm_delta_angles[i] = ANGLE2SHORT(ent->client->ps.viewangles[i]);
#endif
		}
			
	}
	else
	{
		// a spawn point will completely reinitialize the entity except for 
		// the persistant data that was initialized at ClientConnect() time
		SV_InitEntity(ent);
		ent->v.classname = Scr_SetString("player");
		Scr_ClientBegin(ent);
		SV_LinkEdict(ent);
	}
//	SV_InitEntity(ent);
//	Scr_ClientBegin(ent);
	// make sure all view stuff is valid
	Scr_ClientEndServerFrame(ent);

	Cbuf_InsertFromDefer ();
}

//=============================================================================

/*
==================
SV_NextDownload_f
==================
*/
void SV_NextDownload_f (void)
{
	int		r;
	int		percent;
	int		size;

	if (!sv_client->download)
		return;

	r = sv_client->downloadsize - sv_client->downloadcount;
	if (r > 1024)
		r = 1024;

	MSG_WriteByte (&sv_client->netchan.message, SVC_DOWNLOAD);
	MSG_WriteShort (&sv_client->netchan.message, r);

	sv_client->downloadcount += r;
	size = sv_client->downloadsize;
	if (!size)
		size = 1;
	percent = sv_client->downloadcount*100/size;

	MSG_WriteByte (&sv_client->netchan.message, percent);
	SZ_Write (&sv_client->netchan.message, sv_client->download + sv_client->downloadcount - r, r);

	if (sv_client->downloadcount != sv_client->downloadsize)
		return;

	FS_FreeFile (sv_client->download);
	sv_client->download = NULL;
}

/*
==================
SV_BeginDownload_f
==================
*/
void SV_BeginDownload_f(void)
{
	char	*name;
	extern	cvar_t *allow_download;
	extern	cvar_t *allow_download_models;
	extern	cvar_t *allow_download_sounds;
	extern	cvar_t *allow_download_maps;
	extern	int		file_from_pak; // ZOID did file come from pak?
	int offset = 0;

	name = Cmd_Argv(1);

	if (Cmd_Argc() > 2)
		offset = atoi(Cmd_Argv(2)); // downloaded offset

	// hacked by zoid to allow more conrol over download
	// first off, no .. or global allow check
	if (strstr (name, "..") || !allow_download->value
		// leading dot is no good
		|| *name == '.' 
		// leading slash bad as well, must be in subdir
		|| *name == '/'
		// now models
		|| (strncmp(name, "models/", 6) == 0 && !allow_download_models->value)
		// now sounds
		|| (strncmp(name, "sound/", 6) == 0 && !allow_download_sounds->value)
		// now maps (note special case for maps, must not be in pak)
		|| (strncmp(name, "maps/", 6) == 0 && !allow_download_maps->value)
		// MUST be in a subdirectory	
		|| !strstr (name, "/") )	
	{	// don't allow anything with .. path
		MSG_WriteByte (&sv_client->netchan.message, SVC_DOWNLOAD);
		MSG_WriteShort (&sv_client->netchan.message, -1);
		MSG_WriteByte (&sv_client->netchan.message, 0);
		return;
	}


	if (sv_client->download)
		FS_FreeFile (sv_client->download);

	sv_client->downloadsize = FS_LoadFile (name, (void **)&sv_client->download);
	sv_client->downloadcount = offset;

	if (offset > sv_client->downloadsize)
		sv_client->downloadcount = sv_client->downloadsize;

	// special check for maps, if it came from a pak file, don't allow download  ZOID
	if (!sv_client->download || (strncmp(name, "maps/", 5) == 0 && file_from_pak))
	{
		Com_DPrintf (DP_SV, "Couldn't download %s to %s\n", name, sv_client->name);
		if (sv_client->download) 
		{
			FS_FreeFile (sv_client->download);
			sv_client->download = NULL;
		}

		MSG_WriteByte (&sv_client->netchan.message, SVC_DOWNLOAD);
		MSG_WriteShort (&sv_client->netchan.message, -1);
		MSG_WriteByte (&sv_client->netchan.message, 0);
		return;
	}

	SV_NextDownload_f ();
	Com_DPrintf (DP_SV, "Downloading %s to %s\n", name, sv_client->name);
}



//============================================================================


/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately
=================
*/
void SV_Disconnect_f (void)
{
//	SV_EndRedirect ();
	SV_DropClient (sv_client);	
}


/*
==================
SV_ShowServerinfo_f

Dumps the serverinfo info string
==================
*/
void SV_ShowServerinfo_f (void)
{
	Info_Print (Cvar_Serverinfo());
}


void SV_Nextserver (void)
{
	char	*v;

	//ZOID, ss_pic can be nextserver'd in coop mode
	if (sv.state == ss_game || (sv.state == ss_pic && !Cvar_VariableValue("coop")))
		return;		// can't nextserver while playing a normal game

	svs.spawncount++;	// make sure another doesn't sneak in
	v = Cvar_VariableString ("nextserver");
	if (!v[0])
		Cbuf_AddText ("killserver\n");
	else
	{
		Cbuf_AddText (v);
		Cbuf_AddText ("\n");
	}
	Cvar_Set ("nextserver","");
}

/*
==================
SV_Nextserver_f

A cinematic has completed or been aborted by a client, so move
to the next server
==================
*/
void SV_Nextserver_f (void)
{
	if ( atoi(Cmd_Argv(1)) != svs.spawncount ) 
	{
		Com_DPrintf (DP_SV, "Nextserver() from wrong level, from %s\n", sv_client->name);
		return;		// leftover from last server
	}

	Com_DPrintf (DP_SV, "Nextserver() from %s\n", sv_client->name);

	SV_Nextserver ();
}

/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/

typedef struct
{
	char	*name;
	void	(*func) (void);
} ucmd_t;

ucmd_t ucmds[] =
{
	// auto issued
	{"new", SV_New_f},
	{"configstrings", SV_Configstrings_f},
	{"baselines", SV_Baselines_f},
	{"begin", SV_Begin_f},

	{"nextserver", SV_Nextserver_f},

	{"disconnect", SV_Disconnect_f},

	// issued by hand at client consoles	
	{"info", SV_ShowServerinfo_f},

	{"download", SV_BeginDownload_f},
	{"nextdl", SV_NextDownload_f},

	{NULL, NULL}
};

/*
==================
SV_ExecuteUserCommand
==================
*/
void SV_ExecuteUserCommand (char *s)
{
	ucmd_t	*u;
	
	Cmd_TokenizeString (s, true);
	sv_player = sv_client->edict;

	for (u = ucmds; u->name; u++)
	{
		if (!strcmp(Cmd_Argv(0), u->name))
		{
			u->func();
			break;
		}
	}
	// handle incomming command from client in progs
	if (!u->name && sv.state == ss_game)
		ClientCommand (sv_player);
}

void SV_ClientThink (client_t *cl, usercmd_t *cmd)

{
	cl->commandMsec -= cmd->msec;
	if (cl->commandMsec < 0 && sv_enforcetime->value )
	{
		Com_DPrintf (DP_SV, "commandMsec underflow from %s\n", cl->name);
		return;
	}

	sv_entity = cl->edict;
	Scr_ClientThink(cl->edict, cmd);
}


/*
===================
SV_ExecuteClientMessage

The current net_message is parsed for the given client
===================
*/
void SV_ExecuteClientMessage (client_t *cl)
{
	int		c;
	char	*s;

	usercmd_t	nullcmd;
	usercmd_t	oldest, oldcmd, newcmd;
	int		net_drop;
	int		stringCmdCount;
	int		checksum, calculatedChecksum;
	int		checksumIndex;
	qboolean	move_issued;
	int		lastframe;

	sv_client = cl;
	sv_player = sv_client->edict;

	// only allow one move command
	move_issued = false;
	stringCmdCount = 0;

	while (1)
	{
		if (net_message.readcount > net_message.cursize)
		{
			Com_Printf ("SV_ReadClientMessage: badread\n");
			SV_DropClient (cl);
			return;
		}	

		c = MSG_ReadByte (&net_message);
		if (c == -1)
			break;
				
		switch (c)
		{
		default:
			Com_Printf ("SV_ReadClientMessage: unknown command char\n");
			SV_DropClient (cl);
			return;
						
		case clc_nop:
			break;

		case clc_userinfo:
			strncpy (cl->userinfo, MSG_ReadString (&net_message), sizeof(cl->userinfo)-1);
			SV_UserinfoChanged (cl);
			break;

		case clc_move:
			if (move_issued)
				return;		// someone is trying to cheat...

			move_issued = true;
			checksumIndex = net_message.readcount;
			checksum = MSG_ReadByte (&net_message);
			lastframe = MSG_ReadLong (&net_message);
			if (lastframe != cl->lastframe) 
			{
				cl->lastframe = lastframe;
				if (cl->lastframe > 0) 
				{
					cl->frame_latency[cl->lastframe&(LATENCY_COUNTS-1)] = svs.realtime - cl->frames[cl->lastframe & UPDATE_MASK].senttime;
				}
			}

			memset (&nullcmd, 0, sizeof(nullcmd));
			MSG_ReadDeltaUsercmd (&net_message, &nullcmd, &oldest);
			MSG_ReadDeltaUsercmd (&net_message, &oldest, &oldcmd);
			MSG_ReadDeltaUsercmd (&net_message, &oldcmd, &newcmd);

			if ( cl->state != cs_spawned )
			{
				cl->lastframe = -1;
				break;
			}

			// if the checksum fails, ignore the rest of the packet
			calculatedChecksum = COM_BlockSequenceCRCByte (
				net_message.data + checksumIndex + 1,
				net_message.readcount - checksumIndex - 1,
				cl->netchan.incoming_sequence);

			if (calculatedChecksum != checksum)
			{
				Com_DPrintf (DP_SV, "Failed command checksum for %s (%d != %d)/%d\n",
					cl->name, calculatedChecksum, checksum, 
					cl->netchan.incoming_sequence);
				return;
			}

			if (!sv_paused->value)
			{
				net_drop = cl->netchan.dropped;
				if (net_drop < 20)
				{

				//	if (net_drop > 2)
				//		Com_Printf ("drop %i\n", net_drop);
					while (net_drop > 2)
					{
						SV_ClientThink (cl, &cl->lastcmd);

						net_drop--;
					}
					if (net_drop > 1)
						SV_ClientThink (cl, &oldest);

					if (net_drop > 0)
						SV_ClientThink (cl, &oldcmd);

				}
				SV_ClientThink (cl, &newcmd);
			}

			cl->lastcmd = newcmd;
			break;

		case clc_stringcmd:	
			s = MSG_ReadString (&net_message);

			// malicious users may try using too many string commands
			if (++stringCmdCount < MAX_STRINGCMDS)
				SV_ExecuteUserCommand (s);

			if (cl->state == cs_zombie)
				return;	// disconnect command
			break;
		}
	}
}

