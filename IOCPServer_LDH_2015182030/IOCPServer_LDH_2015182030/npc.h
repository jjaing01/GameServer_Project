#pragma once
void initialize_NPC();															// NPC 생성 함수
void wake_up_npc(int id);														// NPC 깨우는 함수
void attack_start_npc(int id);													// NPC 전투모드 ON
void attack_stop_npc(int id);													// NPC 전투모드 OFF
void dead_npc(int id);															// NPC 죽음모드 (30초 뒤 부활)
void regen_npc(int id);															// NPC Regen 모드
void add_timer(int obj_id, OPMODE ev_type, system_clock::time_point t);			// 타이머 쓰레드에 명령 등록 함수

void random_move_npc(int id);													// NPC 자동 움직임 함수
void agro_move_orc(int id);														// ORC MOVE 함수(AGRO + ROAMING)
void process_regen_npc(int id);													// NPC Regen 함수
void process_attack_npc(int id);												// NPC Attack to Player						

/* LUA FUNCTION */
int API_SendMessage(lua_State* L);
int API_get_x(lua_State* L);
int API_get_y(lua_State* L);
int API_IsBye(lua_State* L);
