#include "stdafx.h"
#include "npc.h"

void initialize_NPC()
{
	cout << "Initialize NPC" << endl;
	for (int i = MAX_USER; i < MAX_USER + NUM_NPC; ++i)
	{
		g_clients[i].x = rand() % WORLD_WIDTH;
		g_clients[i].y = rand() % WORLD_HEIGHT;
		g_clients[i].ori_x = g_clients[i].x;
		g_clients[i].ori_y = g_clients[i].y;

		g_clients[i].m_bIsBye = false;
		g_clients[i].m_iMoveDir = 0;
		g_clients[i].lev = (abs(g_clients[i].x - (WORLD_WIDTH / 2)) + abs(g_clients[i].y - (WORLD_WIDTH / 2))) / 100 + 1;	// 거리에 따른 레벨 조정 (맵 중심부- 저렙 구간)
		g_clients[i].exp = (g_clients[i].lev * g_clients[i].lev) * 2;														// Term Project 요구 사항

		/* NPC TYPE: ORC */
		if (i < MAX_USER + NUM_NPC / 2)
		{
			g_clients[i].type = TYPE_ORC;
			g_clients[i].att = ORC_ATT;
			g_clients[i].hp = ORC_HP;
			g_clients[i].maxhp = ORC_HP;			
		}
		/* NPC TYPE: ELF */
		else
		{
			g_clients[i].type = TYPE_ELF;
			g_clients[i].att = ELF_ATT;
			g_clients[i].hp = ELF_HP;
			g_clients[i].maxhp = ELF_HP;
		}

		char npc_name[50];
		sprintf_s(npc_name, "N%d", i);
		strcpy_s(g_clients[i].name, npc_name);
		g_clients[i].m_status = ST_STOP;

		/* Create Lua Virtaul Machine */
		lua_State* L = g_clients[i].L = luaL_newstate();
		luaL_openlibs(L);

		/* file load는 NPC 종류마다 달라야한다. 지금은 monster 뿐이므로 이렇게 한다.*/
		int error = luaL_loadfile(L, "monster.lua");
		error = lua_pcall(L, 0, 0, 0);

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 1, 0);

		/* 루아가 사용할 수 있는 API 함수를 정해준다. */
		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
		lua_register(L, "API_IsBye", API_IsBye);
	}
	cout << "NPC Initialize Finish" << endl;
}

void wake_up_npc(int id)
{
	STATUS prev_state = ST_STOP;

	if (true == atomic_compare_exchange_strong(&g_clients[id].m_status, &prev_state, ST_ACTIVE))
	{
		if (g_clients[id].type == TYPE_ELF)
		{
			//opmode = OP_ELF_MOVE;
			add_timer(id, OP_RANDOM_MOVE, system_clock::now() + 1s);
		}
		else if (g_clients[id].type == TYPE_ORC)
		{
			add_timer(id, OP_ORC_MOVE, system_clock::now() + 1s);
		}
		else if (g_clients[id].type == TYPE_BOSS)
		{
			add_timer(id, OP_BOSS_MOVE, system_clock::now() + 1s);
		}	
		else if (g_clients[id].type == TYPE_NORMAL)
		{
			add_timer(id, OP_RANDOM_MOVE, system_clock::now() + 1s);
		}
	}
}

void add_timer(int obj_id, OPMODE ev_type, system_clock::time_point t)
{
	event_type ev{ obj_id,t,ev_type };
	g_timer_lock.lock();
	g_timer_queue.push(event_type{ ev });
	g_timer_lock.unlock();
}

void random_move_npc(int id)
{
	unordered_set <int> old_viewlist;
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (false == g_clients[i].in_use) continue;
		if (true == is_near(id, i)) old_viewlist.insert(i);
	}

	int x = g_clients[id].x;
	int y = g_clients[id].y;

	/* 루아 스크립트 -> 카운트 시작 여부 */
	if (!g_clients[id].m_bIsBye)
	{
		switch (rand() % 4)
		{
		case 0: if (x > 0) x--; break;
		case 1: if (x < (WORLD_WIDTH - 1)) x++; break;
		case 2: if (y > 0) y--; break;
		case 3: if (y < (WORLD_HEIGHT - 1)) y++; break;
		}
	}
	else
	{
		//cout << "BYE 시작 전 움직이는 방향" << g_clients[id].m_iMoveDir << endl;
		switch (g_clients[id].m_iMoveDir)
		{
		case MV_UP: if (y > 0) y--; break;
		case MV_DOWN: if (y < (WORLD_HEIGHT - 1)) y++; break;
		case MV_LEFT: if (x > 0) x--; break;
		case MV_RIGHT: if (x < (WORLD_WIDTH - 1)) x++; break;
		default: 
			cout << "Unknown Direction in CS_MOVE packet.\n";
			while (true);
		}
	}

	g_clients[id].x = x;
	g_clients[id].y = y;

	/* 이동을 했다면 주위의 플레이어에게 알려주자 */
	unordered_set <int> new_viewlist;
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (id == i) continue;
		if (false == g_clients[i].in_use) continue;
		if (true == is_near(id, i)) new_viewlist.insert(i);
	}

	/* [움직이기 전 시야 검색]*/
	for (auto& pl : old_viewlist)
	{
		// NPC가 움직인 후 유저가 시야 내에 있는 경우
		if (0 < new_viewlist.count(pl))
		{
			if (g_clients[pl].view_list.count(id))		/* 유저의 시야에 현재 나(NPC)가 있을 경우 -> NPC의 Move Packet Send */
				send_move_packet(pl, id);
			else										/* 유저의 시야에 현재 나(NPC)가 없을 경우 -> NPC의 Enter Packet Send */
			{
				g_clients[pl].view_list.insert(id);
				send_enter_packet(pl, id);
			}
		}
		/* NPC가 움직인 후 유저가 시야 내에 없는 경우 */
		else
		{
			if (g_clients[pl].view_list.count(id) > 0)	/* 유저의 시야에 현재 나(NPC)가 있을 경우 -> NPC의 Leave Packet Send */
			{
				g_clients[pl].view_list.erase(id);
				send_leave_packet(pl, id);
			}
		}
	}
	
	/* [움직인 후 시야 검색]*/
	for (auto& pl : new_viewlist)
	{		
		if (0 == g_clients[pl].view_list.count(id))		/* 해당 유저의 시야에 나(NPC)가 없는 경우 -> 시야에 등록, Enter 패킷 전송 */
		{
			g_clients[pl].view_list.insert(id);
			send_enter_packet(pl, id);
		}
		else											/* 해당 유저의 시야에 나(NPC)가 있는 경우 -> Move 패킷 전송 */
			send_move_packet(pl, id);	
	}

	/* NPC 주변에 유저들이 없는 경우 -> 가만히 있기 */
	if (true == new_viewlist.empty()) 
	{
		g_clients[id].m_status = ST_STOP;
	}
	
	// Lua Script NPC AI
	for (auto pc : new_viewlist)
	{
		OVER_EX* over = new OVER_EX;
		over->object_id = pc;
		over->op_mode = OPMODE::OP_PLAYER_MOVE_NOTIFY;
		PostQueuedCompletionStatus(g_hIocp, 1, id, &over->wsa_over);
	}
}

void agro_move_orc(int id)
{
	unordered_set <int> old_viewlist;
	unordered_set <int> old_targetlist;

	/* AGRO Monster 시야 설정 */
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (false == g_clients[i].in_use) continue;
		if (true == is_near(id, i)) old_viewlist.insert(i);
	}

	for(auto& pl:old_viewlist)
		if (true == is_orc_target(id, pl)) old_targetlist.insert(pl);

	int x = g_clients[id].x;
	int y = g_clients[id].y;

	/* ORC의 시야 내에 플레이어가 존재한다면 TARGET으로 설정 */
	bool is_target = false;
	int target_id = -1;
	int target_posX = 0;
	int target_posY = 0;
	int npc_oriPosX = 0;
	int npc_oriPosY = 0;

	/* TARGET이 존재할 경우 - AGRO MOVE */
	if (!old_targetlist.empty())
	{
		target_id = *(old_targetlist.begin());
		target_posX = g_clients[target_id].x;
		target_posY = g_clients[target_id].y;
		npc_oriPosX = g_clients[id].ori_x;
		npc_oriPosY = g_clients[id].ori_y;

		if (target_posX > x && x < npc_oriPosX + ORC_RAOMING_DIST && target_posX != x + 1) ++x;
		else if (target_posX < x && x > npc_oriPosX - ORC_RAOMING_DIST && target_posX != x - 1) --x;
		else if (target_posY > y && y < npc_oriPosY + ORC_RAOMING_DIST && target_posY != y + 1) ++y;
		else if (target_posY < y && y > npc_oriPosY - ORC_RAOMING_DIST && target_posY != y - 1) --y;
	}
	/* TARGET이 존재하지 않을 경우 - ROAMING MOVE */
	else
	{
		switch (rand() % 4)
		{
		case 0: if (x > npc_oriPosX + ORC_RAOMING_DIST && x > 0) x--; break;
		case 1: if (x < (WORLD_WIDTH - 1)) x++; break;
		case 2: if (y > npc_oriPosY - ORC_RAOMING_DIST && y > 0) y--; break;
		case 3: if (y < (WORLD_HEIGHT - 1)) y++; break;
		}
	}
	
	g_clients[id].x = x;
	g_clients[id].y = y;

	/* 이동을 했다면 주위의 플레이어에게 알려주자 */
	unordered_set <int> new_viewlist;
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (id == i) continue;
		if (false == g_clients[i].in_use) continue;
		if (true == is_orc_target(id, i)) new_viewlist.insert(i);
	}

	/* [움직이기 전 시야 검색]*/
	for (auto& pl : old_viewlist)
	{
		// NPC가 움직인 후 유저가 시야 내에 있는 경우
		if (0 < new_viewlist.count(pl))
		{
			if (g_clients[pl].view_list.count(id))		/* 유저의 시야에 현재 나(NPC)가 있을 경우 -> NPC의 Move Packet Send */
				send_move_packet(pl, id);
			else										/* 유저의 시야에 현재 나(NPC)가 없을 경우 -> NPC의 Enter Packet Send */
			{
				g_clients[pl].view_list.insert(id);
				send_enter_packet(pl, id);
			}
		}
		/* NPC가 움직인 후 유저가 시야 내에 없는 경우 */
		else
		{
			if (g_clients[pl].view_list.count(id) > 0)	/* 유저의 시야에 현재 나(NPC)가 있을 경우 -> NPC의 Leave Packet Send */
			{
				g_clients[pl].view_list.erase(id);
				send_leave_packet(pl, id);
			}
		}
	}

	/* [움직인 후 시야 검색]*/
	for (auto& pl : new_viewlist)
	{
		if (0 == g_clients[pl].view_list.count(id))		/* 해당 유저의 시야에 나(NPC)가 없는 경우 -> 시야에 등록, Enter 패킷 전송 */
		{
			g_clients[pl].view_list.insert(id);
			send_enter_packet(pl, id);
		}
		else											/* 해당 유저의 시야에 나(NPC)가 있는 경우 -> Move 패킷 전송 */
			send_move_packet(pl, id);
	}

	/* NPC 주변에 유저들이 없는 경우 -> 가만히 있기 */
	if (true == new_viewlist.empty())
	{
		g_clients[id].m_status = ST_STOP;
	}

	// Lua Script NPC AI
	for (auto pc : new_viewlist)
	{
		/* 공격 사거리 내에 들어왔을 경우: 플레이어 공격 */
		OVER_EX* over = new OVER_EX;
		over->object_id = pc;
		over->op_mode = OPMODE::OP_PLAYER_MOVE_NOTIFY;
		PostQueuedCompletionStatus(g_hIocp, 1, id, &over->wsa_over);
	}
}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 3);

	send_chat_packet(user_id, my_id, mess);

	/* 리턴 파라미터 개수가 0 이므로 return 0 */
	return 0;
}

int API_get_x(lua_State* L)
{
	int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);

	int x = g_clients[user_id].x;

	lua_pushnumber(L, x);

	return 1;
}

int API_get_y(lua_State* L)
{
	int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);

	int y = g_clients[user_id].y;

	lua_pushnumber(L, y);

	return 1;
}

int API_IsBye(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int my_dir = (int)lua_tointeger(L, -2);
	bool bIsBye = (bool)lua_tointeger(L, -1);

	lua_pop(L, 3);

	g_clients[my_id].m_bIsBye = bIsBye;
	g_clients[my_id].m_iMoveDir = my_dir;

	/* 리턴 파라미터 개수가 0 이므로 return 0 */
	return 0;
}
