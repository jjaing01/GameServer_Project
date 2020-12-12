#include "stdafx.h"
#include "npc.h"

void initialize_NPC()
{
	cout << "Initialize NPC" << endl;
	for (int i = MAX_USER; i < MAX_USER + NUM_NPC; ++i)
	{
		g_clients[i].x = rand() % WORLD_WIDTH;
		g_clients[i].y = rand() % WORLD_HEIGHT;
		g_clients[i].m_bIsBye = false;
		g_clients[i].m_iMoveDir = 0;
		g_clients[i].hp = 100;
		g_clients[i].lev = 1;
		g_clients[i].exp = 0;

		char npc_name[50];
		sprintf_s(npc_name, "N%d", i);
		strcpy_s(g_clients[i].name, npc_name);
		g_clients[i].m_status = ST_STOP;

		/* Create Lua Virtaul Machine */
		lua_State* L = g_clients[i].L = luaL_newstate();
		luaL_openlibs(L);

		/* file load�� NPC �������� �޶���Ѵ�. ������ monster ���̹Ƿ� �̷��� �Ѵ�.*/
		int error = luaL_loadfile(L, "monster.lua");
		error = lua_pcall(L, 0, 0, 0);

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 1, 0);

		/* ��ư� ����� �� �ִ� API �Լ��� �����ش�. */
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
		add_timer(id, OP_RANDOM_MOVE, system_clock::now() + 1s);
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

	/* ��� ��ũ��Ʈ -> ī��Ʈ ���� ���� */
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
		//cout << "BYE ���� �� �����̴� ����" << g_clients[id].m_iMoveDir << endl;
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

	/* �̵��� �ߴٸ� ������ �÷��̾�� �˷����� */
	unordered_set <int> new_viewlist;
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (id == i) continue;
		if (false == g_clients[i].in_use) continue;
		if (true == is_near(id, i)) new_viewlist.insert(i);
	}

	/* [�����̱� �� �þ� �˻�]*/
	for (auto& pl : old_viewlist)
	{
		// NPC�� ������ �� ������ �þ� ���� �ִ� ���
		if (0 < new_viewlist.count(pl))
		{
			if (g_clients[pl].view_list.count(id))		/* ������ �þ߿� ���� ��(NPC)�� ���� ��� -> NPC�� Move Packet Send */
				send_move_packet(pl, id);
			else										/* ������ �þ߿� ���� ��(NPC)�� ���� ��� -> NPC�� Enter Packet Send */
			{
				g_clients[pl].view_list.insert(id);
				send_enter_packet(pl, id);
			}
		}
		/* NPC�� ������ �� ������ �þ� ���� ���� ��� */
		else
		{
			if (g_clients[pl].view_list.count(id) > 0)	/* ������ �þ߿� ���� ��(NPC)�� ���� ��� -> NPC�� Leave Packet Send */
			{
				g_clients[pl].view_list.erase(id);
				send_leave_packet(pl, id);
			}
		}
	}
	
	/* [������ �� �þ� �˻�]*/
	for (auto& pl : new_viewlist)
	{		
		if (0 == g_clients[pl].view_list.count(id))		/* �ش� ������ �þ߿� ��(NPC)�� ���� ��� -> �þ߿� ���, Enter ��Ŷ ���� */
		{
			g_clients[pl].view_list.insert(id);
			send_enter_packet(pl, id);
		}
		else											/* �ش� ������ �þ߿� ��(NPC)�� �ִ� ��� -> Move ��Ŷ ���� */
			send_move_packet(pl, id);	
	}

	/* NPC �ֺ��� �������� ���� ��� -> ������ �ֱ� */
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

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 3);

	send_chat_packet(user_id, my_id, mess);

	/* ���� �Ķ���� ������ 0 �̹Ƿ� return 0 */
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

	/* ���� �Ķ���� ������ 0 �̹Ƿ� return 0 */
	return 0;
}
