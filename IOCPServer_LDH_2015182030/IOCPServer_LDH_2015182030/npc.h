#pragma once
void initialize_NPC();															// NPC ���� �Լ�
void wake_up_npc(int id);														// NPC ����� �Լ�
void add_timer(int obj_id, OPMODE ev_type, system_clock::time_point t);			// Ÿ�̸� �����忡 ��� ��� �Լ�

void random_move_npc(int id);													// NPC �ڵ� ������ �Լ�

/* LUA FUNCTION */
int API_SendMessage(lua_State* L);
int API_get_x(lua_State* L);
int API_get_y(lua_State* L);
int API_IsBye(lua_State* L);
