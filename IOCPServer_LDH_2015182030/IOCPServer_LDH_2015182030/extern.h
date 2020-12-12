#pragma once
/*=================================矫胶袍 能刨明=================================*/
extern HANDLE g_hIocp;
extern SOCKET g_hListenSock;
extern OVER_EX g_accept_over;

/*==================================霸烙 能刨明=================================*/
/* ID LOCK */
extern mutex g_id_lock;

/* NPC 按眉 磊丰备炼 */
extern obj_info g_npcs[NUM_NPC];

/* CLIENT(USER) 按眉 磊丰备炼 */
extern client_info g_clients[MAX_USER];

/* Timer Queue */
extern priority_queue<event_type> g_timer_queue;
extern mutex g_timer_lock;
