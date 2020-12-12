#include "stdafx.h"

/* IOCP SERVER 관련 변수*/
HANDLE g_hIocp;
SOCKET g_hListenSock;
OVER_EX g_accept_over;

/* OBJECT 관련 변수 */
mutex g_id_lock;
obj_info g_npcs[NUM_NPC];
client_info g_clients[MAX_USER];

/* TIMER 관련 변수 */
priority_queue<event_type> g_timer_queue;
mutex g_timer_lock;

/*==============================================================함수 선언부========================================================================*/
void Init_obj();					// 클라이언트 객체 & NPC 객체 자료구조 초기화
void Ready_Server();				// 서버 메인 루프 초기화

void add_new_client(SOCKET ns);		// 새로운 유저 접속 함수

void time_worker();					// Timer Thread Enter Function
void worker_thread();				// Worker Thread Enter Function


/*================================================================================================================================================*/
int main()
{
	Ready_Server();

	/* Time Thread 생성 */
	thread time_thread{ time_worker };

	/* Worker Threads 생성 */
	vector<thread> vecWorker;
	for (int i = 0; i < 6; ++i)
		vecWorker.emplace_back(worker_thread);

	/* Worker Threads 소멸 */
	for (auto& th : vecWorker)
		th.join();

	/* Time Thread 소멸 */
	time_thread.join();

	closesocket(g_hListenSock);
	WSACleanup();

	return NO_EVENT;
}
/*==============================================================함수 정의부========================================================================*/
void Init_obj()
{
	memset(&g_clients, 0, sizeof(g_clients));
	memset(&g_npcs, 0, sizeof(g_npcs));
}
void Ready_Server()
{
	std::wcout.imbue(std::locale("korean"));

	/* 1. Winsock 초기화 */
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);

	/* IOCP 생성 */
	g_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

	/* Listen Socket 생성*/
	g_hListenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	/* Listen Socket을 IOCP에 등록 */
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

	/* 통신용 Client Socket 생성*/
	SOCKET cSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_accept_over.op_mode = OP_MODE_ACCEPT;
	g_accept_over.wsa_buf.len = static_cast<int>(cSocket);
	ZeroMemory(&g_accept_over.wsa_over, sizeof(&g_accept_over.wsa_over));

	/* 비동기 Accept */
	AcceptEx(g_hListenSock, cSocket, g_accept_over.iocp_buf, 0, 32, 32, NULL, &g_accept_over.wsa_over);

}

void add_new_client(SOCKET ns)
{
	// 새로운 클라이언트를 관리할 서버 번호
	int i;								
	
	g_id_lock.lock();	
	for (i = 0; i < MAX_USER; ++i)
	{
		if (!g_clients[i].in_use)
			break;
	}
	g_id_lock.unlock();

	// 최대 서버 인원 초과 여부
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

		/* 새로 들어온 유저 정보 초기화 */
		g_clients[i].c_lock.lock();
		g_clients[i].in_use = true;
		g_clients[i].m_sock = ns;
		g_clients[i].name[0] = 0;
		g_clients[i].c_lock.unlock();

		g_clients[i].m_packet_start = g_clients[i].m_recv_over.iocp_buf;
		g_clients[i].m_recv_over.op_mode = OP_MODE_RECV;
		g_clients[i].m_recv_over.wsa_buf.buf = reinterpret_cast<CHAR*>(g_clients[i].m_recv_over.iocp_buf);
		g_clients[i].m_recv_over.wsa_buf.len = sizeof(g_clients[i].m_recv_over.iocp_buf);
		ZeroMemory(&g_clients[i].m_recv_over.wsa_over, sizeof(g_clients[i].m_recv_over.wsa_over));
		g_clients[i].m_recv_start = g_clients[i].m_recv_over.iocp_buf;

		g_clients[i].x = rand() % WORLD_WIDTH;
		g_clients[i].y = rand() % WORLD_HEIGHT;
		g_clients[i].hp = 100;
		g_clients[i].lev = 1;
		g_clients[i].exp = 0;

		/* 해당 클라이언트 소켓 IOCP에 등록 */
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(ns), g_hIocp, i, 0);

		DWORD flags = 0;
		int ret;

		/* 해당 클라이언트의 정보 RECV */
		g_clients[i].c_lock.lock();
		if (g_clients[i].in_use)
		{
			ret = WSARecv(g_clients[i].m_sock, &g_clients[i].m_recv_over.wsa_buf, 1, NULL, &flags, &g_clients[i].m_recv_over.wsa_over, NULL);
		}
		g_clients[i].c_lock.lock();

		if (SOCKET_ERROR == ret) 
		{
			int err_no = WSAGetLastError();
			if (ERROR_IO_PENDING != err_no)
				error_display("WSARecv : ", err_no);
		}
	}

	/* 새로 들어온 유저의 접속 처리 완료 -> 다시 비동기 ACCEPT */
	SOCKET cSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_accept_over.op_mode = OP_MODE_ACCEPT;
	g_accept_over.wsa_buf.len = static_cast<ULONG> (cSocket);
	ZeroMemory(&g_accept_over.wsa_over, sizeof(&g_accept_over.wsa_over));
	AcceptEx(g_hListenSock, cSocket, g_accept_over.iocp_buf, 0, 32, 32, NULL, &g_accept_over.wsa_over);
}

void time_worker()
{
	while (true)
	{
		this_thread::sleep_for(1ms);

		while (true)
		{
			g_timer_lock.lock();

			/* Timer Queue 실행 목록 확인 */
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
			default:
				break;
			}
		}
	}
}
void worker_thread()
{
	// [worker_thread가 하는 일 - MAIN]
	// 현재 쓰레드를 IOCP thread pool에 등록  => GQCS
	// iocp가 처리를 맡긴 완료된 I/O 데이터를 꺼내기 => GQCS
	// 꺼낸 데이터를 처리

	while (true)
	{
	
		DWORD io_size;				// 받은 I/O 크기
		int key;					// 보낸이의 KEY
		ULONG_PTR iocp_key;			// 보낸이의 KEY
		WSAOVERLAPPED* lpover;		// IOCP는 OVERLAPPED 기반의 I/O

		/* GQCS => I/O 꺼내기 */
		int ret = GetQueuedCompletionStatus(g_hIocp, &io_size, &iocp_key, &lpover, INFINITE);

#ifdef CHECKING
		cout << "Completion Detected" << endl;
#endif // CHECKING

		if (ret == FALSE)
			error_display("GQCS Error : ", WSAGetLastError());

		/* I/O 처리 작업 */
		OVER_EX* over_ex = reinterpret_cast<OVER_EX*>(lpover);

		switch (over_ex->op_mode)
		{
		case OPMODE::OP_MODE_ACCEPT:
			add_new_client(static_cast<SOCKET>(over_ex->wsa_buf.len));
			break;

		case OPMODE::OP_MODE_RECV:
			break;

		case OPMODE::OP_MODE_SEND:
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