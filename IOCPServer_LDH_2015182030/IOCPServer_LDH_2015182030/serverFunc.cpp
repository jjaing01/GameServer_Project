#include "stdafx.h"
#include "serverFunc.h"

bool is_npc(int p1)
{
	return p1 >= MAX_USER;
}

bool is_near(int p1, int p2)
{
	int dist = (g_clients[p1].x - g_clients[p2].x) * (g_clients[p1].x - g_clients[p2].x);
	dist += (g_clients[p1].y - g_clients[p2].y) * (g_clients[p1].y - g_clients[p2].y);

	return dist <= VIEW_LIMIT * VIEW_LIMIT;
}

bool is_attack(int p1, int p2)
{
	int dist = (g_clients[p1].x - g_clients[p2].x) * (g_clients[p1].x - g_clients[p2].x);
	dist += (g_clients[p1].y - g_clients[p2].y) * (g_clients[p1].y - g_clients[p2].y);

	return dist <= ATTCK_RANGE * ATTCK_RANGE;
}

bool is_orc_target(int p1, int p2)
{
	int dist = (g_clients[p1].x - g_clients[p2].x) * (g_clients[p1].x - g_clients[p2].x);
	dist += (g_clients[p1].y - g_clients[p2].y) * (g_clients[p1].y - g_clients[p2].y);

	return dist <= ORC_TARGET_RANGE * ORC_TARGET_RANGE;
}

int cal_degree(int player, int monster)
{
	int w = g_clients[player].x - g_clients[monster].x;
	int h = g_clients[player].y - g_clients[monster].y;
	float dist = sqrtf(w * w + h * h);

	float fAngle = acosf(w / dist);

	if (g_clients[monster].y < g_clients[player].y)
		fAngle *= -1.f;

	return (float)(fAngle * 180.f / PI);
}
