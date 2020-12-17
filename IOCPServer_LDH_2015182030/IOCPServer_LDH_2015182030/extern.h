#pragma once
/*=================================矫胶袍 能刨明=================================*/
extern HANDLE g_hIocp;
extern SOCKET g_hListenSock;
extern OVER_EX g_accept_over;

extern SQLHDBC g_hdbc;
extern SQLHSTMT g_hstmt;
extern SQLRETURN g_retcode;
extern SQLHENV g_henv;
/*==================================霸烙 能刨明=================================*/
/* ID LOCK */
extern mutex g_id_lock;

/* CLIENT(USER) 按眉 磊丰备炼 */
extern client_info g_clients[MAX_USER + NUM_NPC];

/* Timer Queue */
extern priority_queue<event_type> g_timer_queue;
extern mutex g_timer_lock;
