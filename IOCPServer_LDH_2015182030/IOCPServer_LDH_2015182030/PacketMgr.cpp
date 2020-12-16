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

void send_stat_change_packet(int id)
{
	sc_packet_stat_change p;

	p.size = sizeof(p);
	p.type = SC_PACKET_STAT_CHANGE;

	p.id = id;
	p.level = g_clients[id].lev;
	p.exp = g_clients[id].exp;
	p.hp = g_clients[id].hp;
	
	send_packet(id, &p);
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

	/* ���� ������ ���� �þ� ����Ʈ */
	g_clients[id].vl.lock();
	unordered_set <int> old_viewlist = g_clients[id].view_list;
	g_clients[id].vl.unlock();

	g_clients[id].x = x;
	g_clients[id].y = y;

	/* ���� �������Ը� �ڽ��� ������ ������ �����Ѵ�. */
	send_move_packet(id, id);

	/* ���� ������ ���ο� �þ� ����Ʈ */
	unordered_set <int> new_viewlist;
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (id == i) continue;
		if (false == g_clients[i].in_use) continue;
		if (true == is_near(id, i)) new_viewlist.insert(i);
	}

#ifdef NPC
	/* NPC ó�� */
	for (int i = MAX_USER; i < MAX_USER + NUM_NPC; ++i)
	{
		if (true == is_near(id, i))
		{
			new_viewlist.insert(i);
			wake_up_npc(i);
		}
	}
#endif // NPC

	/* [�þ߿� ���� ��ü ó��] */
	for (int ob : new_viewlist)
	{
		// <�þ߿� ���� ������ ���>
		if (0 == old_viewlist.count(ob))
		{
			g_clients[id].view_list.insert(ob);
			send_enter_packet(id, ob);
			// <NPCó��>
			if (false == is_npc(ob))									// NPC�� �ƴ� �ٸ� ������ ��쿡�� ��Ŷ Send!
			{
				if (0 == g_clients[ob].view_list.count(id))
				{
					g_clients[ob].vl.lock();
					g_clients[ob].view_list.insert(id);
					g_clients[ob].vl.unlock();
					send_enter_packet(ob, id);							// ���� ó�� �߰��� �ٸ� ������� ���� ������ �˸��� ��Ŷ ����
				}
				else													// ���� Ÿ���� �þ߿� ������ ��� �־��� ��쿣 move ��Ŷ�� ���� 
					send_move_packet(ob, id);
			}
		}
		// <�������� �þ߿� �־���, �̵� �Ŀ��� �þ߿� �ִ� ��ü>
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


	/* [���� ViewList ���� �ִµ� ���� ViewList ������ ����] */
	for (int ob : old_viewlist)
	{
		if (0 == new_viewlist.count(ob))
		{
			g_clients[id].vl.lock();
			g_clients[id].view_list.erase(ob);
			g_clients[id].vl.unlock();

			send_leave_packet(id, ob);

			// NPC ó�� 
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
		for (auto& npc : new_viewlist)
		{
			if (false == is_npc(npc)) continue;

			OVER_EX* ex_over = new OVER_EX;
			ex_over->object_id = id;
			ex_over->op_mode = OPMODE::OP_PLAYER_MOVE_NOTIFY;
			PostQueuedCompletionStatus(g_hIocp, 1, npc, &ex_over->wsa_over);
		}
	}
#endif // NPC
}

void process_attck(int id)
{
	/* ���� ������ ���� ����Ʈ */
	unordered_set <int> new_attcklist;
	for (int i = MAX_USER; i < MAX_USER + NUM_NPC; ++i)
	{
		if (true == is_attack(id, i)) new_attcklist.insert(i);
	}

	/* ���� ����Ʈ ���� NPC�� ���� */
	for (auto& monster : new_attcklist)
	{	
		/* MONSTER HP ��� */
		g_clients[monster].hp -= g_clients[id].att;

		/* Monster DEAD */
		if (g_clients[monster].hp <= ZERO_HP && g_clients[monster].m_status == ST_ATTACK)
		{	
			/* MONSTER DIE */
			dead_npc(monster);
			
			/* PLAYER EXP ȹ�� */
			g_clients[id].exp += g_clients[monster].exp;

			/* PLAYER LEVEL UP ���� ���� */
			int isUP = g_clients[id].exp;
			if (isUP >= LEVEL_UP_EXP)
			{
				++g_clients[id].lev;
				g_clients[id].exp = 0;
			}
		}
		else
		{
			/* MONSTER ���� ��� ON */
			g_clients[monster].m_target_id = id;

			attack_start_npc(monster);
		}
	}

	send_stat_change_packet(id);
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

		send_login_ok(id);											// ������ '���� ����'���� login_ok_pakcet(�α��� ���� ��Ŷ)�� ����

		for (int i = 0; i < MAX_USER; ++i)							// ��ü ���� ��ȸ ����
			if (true == g_clients[i].in_use)						// ������ ������ ���
				if (id != i)										// '���� ����'�� ������ ������ ��� �������� enter_packet ���� 
				{
					if (false == is_near(i, id)) continue;			// '���� ����'�� i ������ �Ÿ��� �ִٸ� ���� X

					if (0 == g_clients[i].view_list.count(id))		// i ������ �þ߿� '���� ����'�� ���ٸ� i���� '���� ����'�� ������ ���� 
					{
						g_clients[i].vl.lock();
						g_clients[i].view_list.insert(id);
						g_clients[i].vl.unlock();
						send_enter_packet(i, id);
					}

					if (0 == g_clients[id].view_list.count(i))		// '���� ����'�� �þ� ���� �ִ� i ������ ������ �����ٸ� '���� ����'���� i�� ������ ����
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
			/* ��ó�� �ֳ� Ȯ�� */
			if (false == is_near(id, j)) continue;
			g_clients[id].view_list.insert(j);
			send_enter_packet(id, j);								// NPC���� ������Ŷ�� �� �ʿ䵵 ����, NPC�� �丮��Ʈ�� ���� �ʿ䵵 ����.
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