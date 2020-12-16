#pragma once
/* 게임 컨텐츠용 패킷 */
void send_packet(int id, void* p);												// Packet 전송 함수
void send_login_ok(int id);														// 로그인 수락 패킷
void send_chat_packet(int to, int id, char* message);							// 채팅 패킷
void send_move_packet(int to_client, int id);									// 움직임 패킷
void send_enter_packet(int to_client, int new_id);								// 등장 패킷
void send_leave_packet(int to_client, int new_id);								// 떠남 패킷
void send_stat_change_packet(int to, int id);									// 상태정보(HP,LV,EXP) 변경 패킷

void process_move(int id, char dir);											// 움직임 처리 함수
void process_attck(int id);														// 공격 처리 함수
void update_view_leave(int id);													// NPC가 죽은 후 유저의 시야 관리					

/* 패킷 관리 및 처리 함수 */
void process_packet(int id);													// 패킷 처리 함수 (모든 컨텐츠 처리)
void process_recv(int id, DWORD iosize);										// 패킷 재조립 함수
