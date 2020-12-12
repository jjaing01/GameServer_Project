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

/* __________________________________기본 OBJECT(user,npc) 구조체______________________________________ */
struct obj_info
{
public:
										/* ===게임 컨텐츠=== */
	/* 접속 상태 */
	bool in_use;
	/* object의 이름 */
	char name[MAX_ID_LEN];
	/* object의 위치 */
	short x, y;
	/* object의 레벨 & HP */
	int lev, hp;
	/* object의 상태 */
	atomic<STATUS> m_status;
										/* ===시스템 컨텐츠=== */
	/* obj mutex */
	mutex c_lock;
	/* lua machine */
	lua_State* L;
};

/* _____________________________________________USER 구조체_____________________________________________ */
struct client_info : public obj_info
{
public:
										/* ===시스템 컨텐츠=== */
	SOCKET	m_sock;
	OVER_EX	m_recv_over;
	unsigned char* m_packet_start;
	unsigned char* m_recv_start;

	int move_time;
	mutex vl;
	unordered_set <int> view_list;
										/* ===게임 컨텐츠=== */
	int exp;
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