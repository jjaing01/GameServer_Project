#include "stdafx.h"

/* IOCP SERVER ���� ����*/
HANDLE g_hIocp;
SOCKET g_hListenSock;
OVER_EX g_accept_over;

/* OBJECT ���� ���� */
mutex g_id_lock;
client_info g_clients[MAX_USER + NUM_NPC];

/* TIMER ���� ���� */
priority_queue<event_type> g_timer_queue;
mutex g_timer_lock;

/* DATABASE ���� ���� */
SQLHDBC g_hdbc;
SQLHSTMT g_hstmt;
SQLRETURN g_retcode;
SQLHENV g_henv;

/*==============================================================�Լ� �����========================================================================*/
void Init_obj();					// Ŭ���̾�Ʈ ��ü & NPC ��ü �ڷᱸ�� �ʱ�ȭ
void Ready_Server();				// ���� ���� ���� �ʱ�ȭ

void add_new_client(SOCKET ns);		// ���ο� ���� ���� �Լ�
void disconnect_client(int id);		// ���� ���� ���� �Լ�

void time_worker();					// Timer Thread Enter Function
void worker_thread();				// Worker Thread Enter Function

/*================================================================================================================================================*/
int main()
{
	/* Init Server & Connect DB */
	Ready_Server();

	initialize_NPC();

	/* Time Thread ���� */
	thread time_thread{ time_worker };

	/* Worker Threads ���� */
	vector<thread> vecWorker;
	for (int i = 0; i < 6; ++i)
		vecWorker.emplace_back(worker_thread);

	/* Worker Threads �Ҹ� */
	for (auto& th : vecWorker)
		th.join();

	/* Time Thread �Ҹ� */
	time_thread.join();

	/* Disconnect Database */
	disconnect_DB();

	closesocket(g_hListenSock);
	WSACleanup();

	return NO_EVENT;
}
/*==============================================================�Լ� ���Ǻ�========================================================================*/
void Init_obj()
{
	memset(&g_clients, 0, sizeof(g_clients));
	memset(&g_clients, 0, sizeof(g_clients));
}
void Ready_Server()
{
	std::wcout.imbue(std::locale("korean"));

	connect_DB();

	/* 1. Winsock �ʱ�ȭ */
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);

	/* IOCP ���� */
	g_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

	/* Listen Socket ����*/
	g_hListenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	/* Listen Socket�� IOCP�� ��� */
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_hListenSock), g_hIocp, KEY_SERVER, 0);

	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	/* bind */
	::bind(g_hListenSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
	/* listen */
	listen(g_hListenSock, SOMAXCONN);

	/* ��ſ� Client Socket ����*/
	SOCKET cSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_accept_over.op_mode = OP_MODE_ACCEPT;
	g_accept_over.wsa_buf.len = static_cast<int>(cSocket);
	ZeroMemory(&g_accept_over.wsa_over, sizeof(&g_accept_over.wsa_over));

	/* �񵿱� Accept */
	AcceptEx(g_hListenSock, cSocket, g_accept_over.iocp_buf, 0, 32, 32, NULL, &g_accept_over.wsa_over);

}

void add_new_client(SOCKET ns)
{
	// ���ο� Ŭ���̾�Ʈ�� ������ ���� ��ȣ
	int i;								
	
	g_id_lock.lock();	
	for (i = 0; i < MAX_USER; ++i)
	{
		if (!g_clients[i].in_use)
			break;
	}
	g_id_lock.unlock();

	// �ִ� ���� �ο� �ʰ� ����
	if (MAX_USER == i)					
	{
#ifdef CHECKING
		cout << "Max user limit exceeded.\n";
#endif // CHECKING
		closesocket(ns);
	}
	else
	{
#ifdef CHECKING
		cout << "New Client [" << i << "] Accepted" << endl;
#endif // CHECKING

		/* ���� ���� ���� ���� �ʱ�ȭ */
		g_clients[i].c_lock.lock();
		g_clients[i].in_use = true;
		g_clients[i].m_sock = ns;
		g_clients[i].name[0] = 0;
		g_clients[i].c_lock.unlock();

		g_clients[i].m_packet_start = g_clients[i].m_recv_over.iocp_buf;
		g_clients[i].m_recv_over.op_mode = OPMODE::OP_MODE_RECV;
		g_clients[i].m_recv_over.wsa_buf.buf = reinterpret_cast<CHAR*>(g_clients[i].m_recv_over.iocp_buf);
		g_clients[i].m_recv_over.wsa_buf.len = sizeof(g_clients[i].m_recv_over.iocp_buf);
		ZeroMemory(&g_clients[i].m_recv_over.wsa_over, sizeof(g_clients[i].m_recv_over.wsa_over));
		g_clients[i].m_recv_start = g_clients[i].m_recv_over.iocp_buf;

		g_clients[i].x = rand() % WORLD_WIDTH;
		g_clients[i].y = rand() % WORLD_HEIGHT;
		g_clients[i].ori_x = g_clients[i].x;
		g_clients[i].ori_y = g_clients[i].y;
		g_clients[i].hp = INIT_HP;
		g_clients[i].maxhp = INIT_HP;
		g_clients[i].lev = 1;
		g_clients[i].exp = 0;
		g_clients[i].att = INIT_ATT;
		g_clients[i].type = TYPE_PLAYER;

		/* �ش� Ŭ���̾�Ʈ ���� IOCP�� ��� */
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(ns), g_hIocp, i, 0);

		DWORD flags = 0;
		int ret;

		/* �ش� Ŭ���̾�Ʈ�� ���� RECV */
		g_clients[i].c_lock.lock();
		if (g_clients[i].in_use)
		{
			ret = WSARecv(g_clients[i].m_sock, &g_clients[i].m_recv_over.wsa_buf, 1, NULL, &flags, &g_clients[i].m_recv_over.wsa_over, NULL);
		}
		g_clients[i].c_lock.unlock();

		if (SOCKET_ERROR == ret) 
		{
			int err_no = WSAGetLastError();
			if (ERROR_IO_PENDING != err_no)
				error_display("WSARecv : ", err_no);
		}
	}

	/* ���� ���� ������ ���� ó�� �Ϸ� -> �ٽ� �񵿱� ACCEPT */
	SOCKET cSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_accept_over.op_mode = OP_MODE_ACCEPT;
	g_accept_over.wsa_buf.len = static_cast<ULONG> (cSocket);
	ZeroMemory(&g_accept_over.wsa_over, sizeof(&g_accept_over.wsa_over));
	AcceptEx(g_hListenSock, cSocket, g_accept_over.iocp_buf, 0, 32, 32, NULL, &g_accept_over.wsa_over);
}

void disconnect_client(int id)
{
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (true == g_clients[i].in_use)
			if (i != id)
			{
				if (0 != g_clients[i].view_list.count(id))
				{
					g_clients[i].vl.lock();
					g_clients[i].view_list.erase(id);
					g_clients[i].vl.unlock();
					send_leave_packet(i, id);
				}
			}
	}

	g_clients[id].c_lock.lock();
	g_clients[id].in_use = false;
	g_clients[id].view_list.clear();
	closesocket(g_clients[id].m_sock);
	g_clients[id].m_sock = 0;
	g_clients[id].c_lock.unlock();
}

void time_worker()
{
	while (true)
	{
		this_thread::sleep_for(1ms);

		while (true)
		{
			g_timer_lock.lock();

			/* Timer Queue ���� ��� Ȯ�� */
			if (g_timer_queue.empty())
			{
				g_timer_lock.unlock();
				break;
			}

			auto now_t = system_clock::now();
			event_type ev = g_timer_queue.top();

			if (ev.wakeup_time > system_clock::now())
			{
				g_timer_lock.unlock();
				break;
			}

			g_timer_queue.pop();
			g_timer_lock.unlock();

			switch (ev.event_id)
			{
			case OPMODE::OP_RANDOM_MOVE:
			{
				OVER_EX* over = new OVER_EX;
				over->op_mode = ev.event_id;
				PostQueuedCompletionStatus(g_hIocp, 1, ev.obj_id, &over->wsa_over);
			}
			break;

			case OPMODE::OP_ORC_MOVE:
			{
				OVER_EX* over = new OVER_EX;
				over->op_mode = ev.event_id;
				PostQueuedCompletionStatus(g_hIocp, 1, ev.obj_id, &over->wsa_over);
			}
			break;

			case OPMODE::OP_ELF_MOVE:
			{
				OVER_EX* over = new OVER_EX;
				over->op_mode = ev.event_id;
				PostQueuedCompletionStatus(g_hIocp, 1, ev.obj_id, &over->wsa_over);
			}
			break;

			case OPMODE::OP_MONSTER_DIE:
			{
				OVER_EX* over = new OVER_EX;
				over->op_mode = ev.event_id;
				PostQueuedCompletionStatus(g_hIocp, 1, ev.obj_id, &over->wsa_over);
			}
			break;

			case OPMODE::OP_MONSTER_ATTACK:
			{
				OVER_EX* over = new OVER_EX;
				over->op_mode = ev.event_id;
				PostQueuedCompletionStatus(g_hIocp, 1, ev.obj_id, &over->wsa_over);
			}
			break;

			case OPMODE::OP_MONSTER_REGEN:
			{
				OVER_EX* over = new OVER_EX;
				over->op_mode = ev.event_id;
				PostQueuedCompletionStatus(g_hIocp, 1, ev.obj_id, &over->wsa_over);
			}
			break;

			default:
				break;
			}
		}
	}
}
void worker_thread()
{
	// [worker_thread�� �ϴ� �� - MAIN]
	// ���� �����带 IOCP thread pool�� ���  => GQCS
	// iocp�� ó���� �ñ� �Ϸ�� I/O �����͸� ������ => GQCS
	// ���� �����͸� ó��

	while (true)
	{
		DWORD io_size;				// ���� I/O ũ��
		int key;					// �������� KEY
		ULONG_PTR iocp_key;			// �������� KEY
		WSAOVERLAPPED* lpover;		// IOCP�� OVERLAPPED ����� I/O

		/* GQCS => I/O ������ */
		int ret = GetQueuedCompletionStatus(g_hIocp, &io_size, &iocp_key, &lpover, INFINITE);
		key = static_cast<int>(iocp_key);

#ifdef CHECKING
		cout << "Completion Detected" << endl;
#endif // CHECKING

		if (ret == FALSE)
			error_display("GQCS Error : ", WSAGetLastError());

		/* <I/O ó�� �۾�> */
		OVER_EX* over_ex = reinterpret_cast<OVER_EX*>(lpover);

		switch (over_ex->op_mode)
		{
		case OPMODE::OP_MODE_ACCEPT:
			add_new_client(static_cast<SOCKET>(over_ex->wsa_buf.len));
			break;

		case OPMODE::OP_MODE_RECV:
			if (0 == io_size)
				disconnect_client(key);
			else
			{
#ifdef CHECKING
				cout << "Packet from Client [" << key << "]" << endl;
#endif // CHECKING
				
				/* Packet ������ */
				process_recv(key, io_size);
			}
			break;

		case OPMODE::OP_MODE_SEND:
			delete over_ex;
			break;

		case OPMODE::OP_RANDOM_MOVE:
		{
			random_move_npc(key);

			bool alive = false;
			for (int i = 0; i < MAX_USER; ++i)
			{
				if (true == is_near(key, i))
				{
					if (ST_ACTIVE == g_clients[i].m_status)
					{
						alive = true;
						break;
					}
				}
			}

			if (true == alive) 
				add_timer(key, OP_RANDOM_MOVE, system_clock::now() + 1s);
			else 
			{
				if (g_clients[key].m_status != ST_DEAD)
					g_clients[key].m_status = ST_STOP;
			}

			if (over_ex != nullptr)
				delete over_ex;
		}
		break;

		case OPMODE::OP_PLAYER_MOVE_NOTIFY:
		{
			/* Key NPC(monster)���� �ش� ��� �Լ��� �Ѱ��ش�. */
			g_clients[key].Lua_l.lock();
			lua_getglobal(g_clients[key].L, "event_player_move");
			lua_pushnumber(g_clients[key].L, over_ex->object_id);
			lua_pcall(g_clients[key].L, 1, 1, 0);
			g_clients[key].Lua_l.unlock();

			if (over_ex != nullptr)
				delete over_ex;
		}
		break;

		case OPMODE::OP_MODE_ATTACK:
		{
			if (over_ex != nullptr)
				delete over_ex;
		}
		break;

		case OPMODE::OP_ORC_MOVE:
		{
			agro_move_orc(key);

			bool alive = false;
			for (int i = 0; i < MAX_USER; ++i)
			{
				if (true == is_near(key, i))
				{
					if (ST_ACTIVE == g_clients[i].m_status)
					{
						alive = true;
						break;
					}
				}
			}

			if (true == alive) 
				add_timer(key, OP_ORC_MOVE, system_clock::now() + 1s);
			else 
			{
				if (g_clients[key].m_status != ST_DEAD)
					g_clients[key].m_status = ST_STOP;
			}
		}
		break;

		case OPMODE::OP_ELF_MOVE:
		{
			if (g_clients[key].m_status == ST_ATTACK)
				agro_move_orc(key);
			else
				peace_move_elf(key);

			bool alive = false;
			for (int i = 0; i < MAX_USER; ++i)
			{
				if (true == is_near(key, i))
				{
					if (ST_ACTIVE == g_clients[i].m_status)
					{
						alive = true;
						break;
					}
				}
			}

			if (true == alive)
				add_timer(key, OP_ELF_MOVE, system_clock::now() + 1s);
			else
			{
				if (g_clients[key].m_status != ST_DEAD)
					g_clients[key].m_status = ST_STOP;
			}

			if (over_ex != nullptr)
				delete over_ex;
		}
		break;

		case OPMODE::OP_MONSTER_DIE:
		{
			/* MONSTER REGEN MODE */
			regen_npc(key);

			if (over_ex != nullptr)
				delete over_ex;
		}
		break;

		case OPMODE::OP_MONSTER_ATTACK:
		{
			process_attack_npc(key);

			if (g_clients[key].m_status == ST_ATTACK)
				add_timer(key, OP_MONSTER_ATTACK, system_clock::now() + 1s);
			else
			{
				if (g_clients[key].m_status != ST_DEAD)
					g_clients[key].m_status = ST_STOP;
			}

			if (over_ex != nullptr)
				delete over_ex;
		}
		break;

		case OPMODE::OP_MONSTER_REGEN:
		{
			process_regen_npc(key);

			if (over_ex != nullptr)
				delete over_ex;
		}
		break;

		default:
#ifdef CHECKING
			cout << "[ERROR] Unknown Type MODE in Worker Thread!" << endl;
#endif // CHECKING
			break;
		}
	}
}
/*================================================================================================================================================*/