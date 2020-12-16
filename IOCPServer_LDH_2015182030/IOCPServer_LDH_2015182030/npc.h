#pragma once
void initialize_NPC();															// NPC ���� �Լ�
void wake_up_npc(int id);														// NPC ����� �Լ�
void attack_start_npc(int id);													// NPC ������� ON
void attack_stop_npc(int id);													// NPC ������� OFF
void dead_npc(int id);															// NPC ������� (30�� �� ��Ȱ)
void regen_npc(int id);															// NPC Regen ���
void add_timer(int obj_id, OPMODE ev_type, system_clock::time_point t);			// Ÿ�̸� �����忡 ��� ��� �Լ�

void random_move_npc(int id);													// NPC �ڵ� ������ �Լ�
void agro_move_orc(int id);														// ORC MOVE �Լ�(AGRO + ROAMING)
void process_regen_npc(int id);													// NPC Regen �Լ�
void process_attack_npc(int id);												// NPC Attack to Player						

/* LUA FUNCTION */
int API_SendMessage(lua_State* L);
int API_get_x(lua_State* L);
int API_get_y(lua_State* L);
int API_IsBye(lua_State* L);
