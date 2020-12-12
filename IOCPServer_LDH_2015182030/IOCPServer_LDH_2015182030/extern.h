#pragma once
/*=================================�ý��� ������=================================*/
extern HANDLE g_hIocp;
extern SOCKET g_hListenSock;
extern OVER_EX g_accept_over;

/*==================================���� ������=================================*/
/* ID LOCK */
extern mutex g_id_lock;

/* NPC ��ü �ڷᱸ�� */
extern obj_info g_npcs[NUM_NPC];

/* CLIENT(USER) ��ü �ڷᱸ�� */
extern client_info g_clients[MAX_USER];

/* Timer Queue */
extern priority_queue<event_type> g_timer_queue;
extern mutex g_timer_lock;
