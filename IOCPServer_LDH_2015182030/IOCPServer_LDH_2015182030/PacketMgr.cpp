#include "stdafx.h"
#include "PacketMgr.h"

void send_packet(int id, void* p)
{
	unsigned char* packet = reinterpret_cast<unsigned char*>(p);

	OVER_EX* send_over = new OVER_EX;

	memcpy(send_over->iocp_buf, packet, packet[0]);
	send_over->op_mode = OPMODE::OP_MODE_SEND;
	send_over->wsa_buf.buf = reinterpret_cast<CHAR*>(send_over->iocp_buf);
	send_over->wsa_buf.len = packet[0];
	ZeroMemory(&send_over->wsa_over, sizeof(send_over->wsa_over));

	g_clients[id].c_lock.lock();
	if (true == g_clients[id].in_use)
		WSASend(g_clients[id].m_sock, &send_over->wsa_buf, 1, NULL, 0, &send_over->wsa_over, NULL);
	g_clients[id].c_lock.unlock();
}

void send_login_ok(int id)
{
	sc_packet_login_ok p;

	p.exp = 0;
	p.hp = 100;
	p.id = id;
	p.level = 1;
	p.size = sizeof(p);
	p.type = SC_PACKET_LOGIN_OK;
	p.x = g_clients[id].x;
	p.y = g_clients[id].y;

	send_packet(id, &p);
}

void send_chat_packet(int to, int id, char* message)
{
	sc_packet_chat p;

	p.size = sizeof(p);
	p.type = SC_PACKET_CHAT;
	p.id = id;
	strcpy_s(p.message, message);

	send_packet(to, &p);
}

void send_move_packet(int to_client, int id)
{
	sc_packet_move p;

	p.id = id;
	p.size = sizeof(p);
	p.type = SC_PACKET_MOVE;
	p.x = g_clients[id].x;
	p.y = g_clients[id].y;
	p.move_time = g_clients[id].move_time;

	send_packet(to_client, &p);
}

void send_enter_packet(int to_client, int new_id)
{
	sc_packet_enter p;

	p.id = new_id;
	p.size = sizeof(p);
	p.type = SC_PACKET_ENTER;
	p.x = g_clients[new_id].x;
	p.y = g_clients[new_id].y;
	g_clients[new_id].c_lock.lock();
	strcpy_s(p.name, g_clients[new_id].name);
	g_clients[new_id].c_lock.unlock();
	p.o_type = g_clients[new_id].type;

	send_packet(to_client, &p);
}

void send_leave_packet(int to_client, int new_id)
{
	sc_packet_leave p;

	p.id = new_id;
	p.size = sizeof(p);
	p.type = SC_PACKET_LEAVE;

	send_packet(to_client, &p);
}

void send_stat_change_packet(int to, int id)
{
	sc_packet_stat_change p;

	p.size = sizeof(p);
	p.type = SC_PACKET_STAT_CHANGE;

	p.id = id;
	p.level = g_clients[id].lev;
	p.exp = g_clients[id].exp;
	p.hp = g_clients[id].hp;
	
	send_packet(to, &p);
}

void process_move(int id, char dir)
{
	short x = g_clients[id].x;
	short y = g_clients[id].y;

	switch (dir)
	{
	case MV_UP: if (y > 0) y--; break;
	case MV_DOWN: if (y < (WORLD_HEIGHT - 1)) y++; break;
	case MV_LEFT: if (x > 0) x--; break;
	case MV_RIGHT: if (x < (WORLD_WIDTH - 1)) x++; break;
	default:
#ifdef CHECKING
		cout << "Unknown Direction in CS_MOVE packet.\n";
		while (true);
#endif // CHECKING
		break;
	}

	/* 현재 유저의 기존 시야 리스트 */
	g_clients[id].vl.lock();
	unordered_set <int> old_viewlist = g_clients[id].view_list;
	g_clients[id].vl.unlock();

	g_clients[id].x = x;
	g_clients[id].y = y;

	/* 현재 유저에게만 자신의 움직임 정보를 전송한다. */
	send_move_packet(id, id);

	/* 현재 유저의 새로운 시야 리스트 */
	unordered_set <int> new_viewlist;
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (id == i) continue;
		if (false == g_clients[i].in_use) continue;
		if (true == is_near(id, i)) new_viewlist.insert(i);
	}

#ifdef NPC
	/* NPC 처리 */
	for (int i = MAX_USER; i < MAX_USER + NUM_NPC; ++i)
	{
		if (true == is_near(id, i))
		{
			new_viewlist.insert(i);
			wake_up_npc(i);
		}
	}
#endif // NPC

	/* [시야에 들어온 객체 처리] */
	for (int ob : new_viewlist)
	{
		// <시야에 새로 들어왔을 경우>
		if (0 == old_viewlist.count(ob))
		{
			g_clients[id].view_list.insert(ob);
			send_enter_packet(id, ob);
			// <NPC처리>
			if (false == is_npc(ob))									// NPC가 아닌 다른 유저일 경우에는 패킷 Send!
			{
				if (0 == g_clients[ob].view_list.count(id))
				{
					g_clients[ob].vl.lock();
					g_clients[ob].view_list.insert(id);
					g_clients[ob].vl.unlock();
					send_enter_packet(ob, id);							// 나를 처음 발견한 다른 유저라면 나의 등장을 알리는 패킷 전송
				}
				else													// 내가 타인의 시야에 기존에 계속 있었던 경우엔 move 패킷만 전송 
					send_move_packet(ob, id);
			}
		}
		// <이전에도 시야에 있었고, 이동 후에도 시야에 있는 객체>
		else
		{
			if (false == is_npc(ob))
			{
				if (0 != g_clients[ob].view_list.count(id))
					send_move_packet(ob, id);
				else
				{
					g_clients[ob].vl.lock();
					g_clients[ob].view_list.insert(id);
					g_clients[ob].vl.unlock();
					send_enter_packet(ob, id);
				}
			}
		}
	}


	/* [이전 ViewList 내에 있는데 현재 ViewList 없으면 삭제] */
	for (int ob : old_viewlist)
	{
		if (0 == new_viewlist.count(ob))
		{
			g_clients[id].vl.lock();
			g_clients[id].view_list.erase(ob);
			g_clients[id].vl.unlock();

			send_leave_packet(id, ob);

			// NPC 처리 
			if (false == is_npc(ob))
			{
				if (0 != g_clients[ob].view_list.count(id))
				{
					g_clients[ob].vl.lock();
					g_clients[ob].view_list.erase(id);
					g_clients[ob].vl.unlock();
					send_leave_packet(ob, id);
				}
			}
		}
	}

#ifdef NPC
	/* NPC AI - lua */
	if (false == is_npc(id))
	{
		if (new_viewlist.count(MAX_USER + NUM_NPC - 1) > 0)
		{
			OVER_EX* ex_over = new OVER_EX;
			ex_over->object_id = id;
			ex_over->op_mode = OPMODE::OP_PLAYER_MOVE_NOTIFY;
			PostQueuedCompletionStatus(g_hIocp, 1, MAX_USER + NUM_NPC - 1, &ex_over->wsa_over);
		}
	}
#endif // NPC
}

void process_attck(int id)
{
	/* 현재 유저의 공격 리스트 */
	unordered_set <int> new_attcklist;
	for (int i = MAX_USER; i < MAX_USER + NUM_NPC; ++i)
	{
		if (true == is_attack(id, i)) new_attcklist.insert(i);
	}

	/* 공격 리스트 내의 NPC들 공격 */
	for (auto& monster : new_attcklist)
	{	
		/* 죽어있는 몬스터는 공격 불가 */
		if (g_clients[monster].m_bIsDead)
			continue;

		/* MONSTER HP 깎기 */
		g_clients[monster].hp -= g_clients[id].att;

		char buf[MAX_STR_LEN];
		sprintf_s(buf, "[User ID_%d]님이 [Monster_%d]를 공격하였습니다!", id, monster);
		send_chat_packet(id, id, buf);
		
		/* Monster DEAD */
		if (g_clients[monster].hp <= ZERO_HP)
		{	
			g_clients[monster].hp = ZERO_HP;

			/* MONSTER DIE */
			dead_npc(monster);
			g_clients[monster].m_bIsDead = true;

			/* 플레이어의 시야에서도 삭제한다 */
			update_view_leave(monster);

			/* PLAYER EXP 획득 */
			g_clients[id].exp += g_clients[monster].exp;

			memset(buf, 0, sizeof(buf));
			sprintf_s(buf, "[User ID_%d]님이 [Monster_%d]를 사냥하여 경험치 %d을 획득하였습니다!", id, monster, g_clients[monster].exp);
			send_chat_packet(id, id, buf);

			/* PLAYER LEVEL UP 가능 여부 */
			int isUP = g_clients[id].exp;
			if (isUP >= LEVEL_UP_EXP * g_clients[id].lev)
			{
				++g_clients[id].lev;
				g_clients[id].exp = ZERO_EXP;

				memset(buf, 0, sizeof(buf));
				sprintf_s(buf, "[User ID_%d]님 Level UP 성공! [현재 레벨: %d]입니다.", id, g_clients[id].lev);
				send_chat_packet(id, id, buf);
			}		
		}
		else
		{
			/* MONSTER 전투 모드 ON */
			g_clients[monster].m_target_id = id;

			attack_start_npc(monster);
		}
		send_stat_change_packet(id, monster);
	}

	send_stat_change_packet(id, id);
}

void update_view_leave(int id)
{
	unordered_set <int> old_viewlist;
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (false == g_clients[i].in_use) continue;
		if (true == is_near(id, i)) old_viewlist.insert(i);
	}


	/* 부활 위치 초기화 & HP 초기화 */
	g_clients[id].x = g_clients[id].ori_x;
	g_clients[id].y = g_clients[id].ori_y;
	g_clients[id].hp = g_clients[id].maxhp;

	if (!is_npc(id))
		send_move_packet(id, id);

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
}

void process_packet(int id)
{
	char p_type = g_clients[id].m_packet_start[1];

	switch (p_type)
	{
	case CS_LOGIN:
	{
		cs_packet_login* p = reinterpret_cast<cs_packet_login*>(g_clients[id].m_packet_start);

		g_clients[id].c_lock.lock();
		strcpy_s(g_clients[id].name, p->name);
		g_clients[id].c_lock.unlock();

		send_login_ok(id);											// 서버가 '현재 유저'에게 login_ok_pakcet(로그인 수락 패킷)을 전송

		for (int i = 0; i < MAX_USER; ++i)							// 전체 유저 순회 시작
			if (true == g_clients[i].in_use)						// 접속한 유저일 경우
				if (id != i)										// '현재 유저'를 제외한 접속한 모든 유저에게 enter_packet 전송 
				{
					if (false == is_near(i, id)) continue;			// '현재 유저'와 i 유저의 거리가 멀다면 전송 X

					if (0 == g_clients[i].view_list.count(id))		// i 유저의 시야에 '현재 유저'가 없다면 i에게 '현재 유저'의 정보를 전송 
					{
						g_clients[i].vl.lock();
						g_clients[i].view_list.insert(id);
						g_clients[i].vl.unlock();
						send_enter_packet(i, id);
					}

					if (0 == g_clients[id].view_list.count(i))		// '현재 유저'의 시야 내에 있는 i 유저의 정보가 없었다면 '현재 유저'에게 i의 정보를 전송
					{
						g_clients[id].vl.lock();
						g_clients[id].view_list.insert(i);
						g_clients[id].vl.unlock();
						send_enter_packet(id, i);
					}
				}

#ifdef NPC
		for (int j = MAX_USER; j < MAX_USER + NUM_NPC; ++j)
		{
			/* 근처에 있나 확인 */
			if (false == is_near(id, j)) continue;
			g_clients[id].view_list.insert(j);
			send_enter_packet(id, j);								// NPC에게 엔터패킷을 줄 필요도 없고, NPC는 뷰리스트를 가질 필요도 없다.
		}
#endif // NPC
		break;
	}

	case CS_MOVE:
	{
		cs_packet_move* p = reinterpret_cast<cs_packet_move*>(g_clients[id].m_packet_start);
		g_clients[id].move_time = p->move_time;
		process_move(id, p->direction);
		break;
	}

	case CS_ATTACK:
	{
		cs_packet_attack* p = reinterpret_cast<cs_packet_attack*>(g_clients[id].m_packet_start);
		process_attck(id);
	}
	break;

	case CS_CHAT:
	{
		cs_packet_chat* p = reinterpret_cast<cs_packet_chat*>(g_clients[id].m_packet_start);
		for (auto& pl : g_clients[id].view_list)
		{
			send_chat_packet(pl, id, p->message);
		}	
	}
	break;
	case CS_LOGOUT:
		break;
	case CS_TELEORT:
		break;

	default:
#ifdef CHECKING
		cout << "Unknown Packet type [" << p_type << "] from Client [" << id << "]\n";
		while (true);
#endif // CHECKING
		break;
	}
}

void process_recv(int id, DWORD iosize)
{
	unsigned char p_size = g_clients[id].m_packet_start[0];
	unsigned char* next_recv_ptr = g_clients[id].m_recv_start + iosize;
	while (p_size <= next_recv_ptr - g_clients[id].m_packet_start) {
		process_packet(id);
		g_clients[id].m_packet_start += p_size;
		if (g_clients[id].m_packet_start < next_recv_ptr)
			p_size = g_clients[id].m_packet_start[0];
		else break;
	}

	long long left_data = next_recv_ptr - g_clients[id].m_packet_start;

	if ((MAX_BUFFER - (next_recv_ptr - g_clients[id].m_recv_over.iocp_buf))
		< MIN_BUFFER) {
		memcpy(g_clients[id].m_recv_over.iocp_buf,
			g_clients[id].m_packet_start, left_data);
		g_clients[id].m_packet_start = g_clients[id].m_recv_over.iocp_buf;
		next_recv_ptr = g_clients[id].m_packet_start + left_data;
	}
	DWORD recv_flag = 0;
	g_clients[id].m_recv_start = next_recv_ptr;
	g_clients[id].m_recv_over.wsa_buf.buf = reinterpret_cast<CHAR*>(next_recv_ptr);
	g_clients[id].m_recv_over.wsa_buf.len = MAX_BUFFER -
		static_cast<int>(next_recv_ptr - g_clients[id].m_recv_over.iocp_buf);

	g_clients[id].c_lock.lock();
	if (true == g_clients[id].in_use) {
		WSARecv(g_clients[id].m_sock, &g_clients[id].m_recv_over.wsa_buf,
			1, NULL, &recv_flag, &g_clients[id].m_recv_over.wsa_over, NULL);
	}
	g_clients[id].c_lock.unlock();
}