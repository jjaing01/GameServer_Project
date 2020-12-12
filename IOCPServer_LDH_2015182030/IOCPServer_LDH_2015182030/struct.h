#pragma once

/* _______________________________________확장 OVERAPPED 구조체________________________________________ */
struct OVER_EX 
{
	WSAOVERLAPPED wsa_over;
	OPMODE	op_mode;
	WSABUF	wsa_buf;
	unsigned char iocp_buf[MAX_BUFFER];

	int object_id;
};

/* _____________________________________________USER 구조체_____________________________________________ */
struct client_info 
{		
	/*=============시스템 컨텐츠==============*/
	int move_time;
	SOCKET	m_sock;
	OVER_EX	m_recv_over;
	unsigned char* m_packet_start;
	unsigned char* m_recv_start;

	/*=============게임 컨텐츠==============*/
	bool in_use;
	char name[MAX_ID_LEN];
	short x, y;
	int hp, lev, exp;
	bool m_bIsBye;
	int m_iMoveDir;
	mutex c_lock;
				 
	mutex vl;
	unordered_set <int> view_list;
	atomic<STATUS> m_status;

	lua_State* L;
};

/* _____________________________________________TIMER THERAD - NPC AI STRUCT_____________________________________________ */
struct event_type 
{
	int obj_id;
	system_clock::time_point wakeup_time;
	OPMODE event_id;
	int target_id;
	constexpr bool operator < (const event_type& _Left) const
	{
		return (wakeup_time > _Left.wakeup_time);
	}
};