#pragma once

/* _______________________________________Ȯ�� OVERAPPED ����ü________________________________________ */
struct OVER_EX 
{
	WSAOVERLAPPED wsa_over;
	char	op_mode;
	WSABUF	wsa_buf;
	unsigned char iocp_buf[MAX_BUFFER];

	int object_id;
};

/* __________________________________�⺻ OBJECT(user,npc) ����ü______________________________________ */
struct obj_info
{
public:
										/* ===���� ������=== */
	/* ���� ���� */
	bool in_use;
	/* object�� �̸� */
	char name[MAX_ID_LEN];
	/* object�� ��ġ */
	short x, y;
	/* object�� ���� */
	atomic<STATUS> m_status;
										/* ===�ý��� ������=== */
	/* obj mutex */
	mutex c_lock;

};

/* _____________________________________________USER ����ü_____________________________________________ */
struct client_info : public obj_info
{
public:
										/* ===�ý��� ������=== */
	SOCKET	m_sock;
	OVER_EX	m_recv_over;
	unsigned char* m_packet_start;
	unsigned char* m_recv_start;

	int move_time;
	mutex vl;
	unordered_set <int> view_list;
};