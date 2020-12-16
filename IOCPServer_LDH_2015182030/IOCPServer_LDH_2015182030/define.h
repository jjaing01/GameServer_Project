#pragma once

/* Buffer Size */
constexpr int MAX_BUFFER = 4096;
constexpr int MIN_BUFFER = 1024;

/* IOCP KEY */
constexpr int KEY_SERVER = 1000000;

/* SYSTEM DEFINE */
#define NO_EVENT 0
#define DEAD_OBJ 1
#define NO_INIT -1
#define PI 3.141592

/* GAME CONTENTS */
constexpr short ZERO_HP = 0;
constexpr short ZERO_EXP = 0;
constexpr short LEVEL_UP_EXP = 100;
constexpr int ATTCK_RANGE	= 1;
constexpr int ORC_TARGET_RANGE = 5;
constexpr int ORC_RAOMING_DIST = 20;

constexpr int INIT_HP		= 100;	// 플레이어 초기 HP
constexpr int ORC_HP		= 70;	// ORC HP
constexpr int ELF_HP		= 80;	// ELF HP
constexpr int BOSS_HP		= 150;	// BOSS HP

constexpr int INIT_ATT		= 5;	// 플레이어 초기 공격력
constexpr int ORC_ATT		= 7;	// ORC 공격력
constexpr int ELF_ATT		= 3;	// ELF 공격력
constexpr int BOSS_ATT		= 10;	// BOSS 공격력

constexpr int MASTER		= 99999;
