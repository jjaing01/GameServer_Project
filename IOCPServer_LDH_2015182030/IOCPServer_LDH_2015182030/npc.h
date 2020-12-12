#pragma once
void initialize_NPC();															// NPC 생성 함수
void wake_up_npc(int id);														// NPC 깨우는 함수
void add_timer(int obj_id, OPMODE ev_type, system_clock::time_point t);			// 타이머 쓰레드에 명령 등록 함수

void random_move_npc(int id);													// NPC 자동 움직임 함수

/* LUA FUNCTION */
int API_SendMessage(lua_State* L);
int API_get_x(lua_State* L);
int API_get_y(lua_State* L);
int API_IsBye(lua_State* L);
