#pragma once

#define NPC

/* SYSTEM HEADER */
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include <queue>
#include <atomic>
#include <vector>
#include <sqlext.h>
#include <string>

// Lua in C Environment
extern "C"
{
#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"
}


/* NAMESPACE */
using namespace std;
using namespace chrono;

/* LINK TO LIBRARY */
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "lua54.lib")
#pragma comment(lib, "odbc32.lib")

/* USER HEADER */
#include "protocol.h"
#include "define.h"
#include "enum.h"
#include "struct.h"
#include "extern.h"
#include "function.h"

#include "serverFunc.h"
#include "PacketMgr.h"
#include "npc.h"
#include "my_DB.h"