#pragma once

bool is_npc(int p1);
bool is_near(int p1, int p2);
bool is_attack(int p1, int p2);
bool is_orc_target(int p1, int p2);

/* MATH FUNC */
int cal_degree(int player, int monster);
