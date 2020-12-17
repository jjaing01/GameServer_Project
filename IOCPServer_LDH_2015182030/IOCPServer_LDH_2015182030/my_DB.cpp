#include "stdafx.h"
#include "my_DB.h"

void connect_DB()
{
	// Allocate environment handle  
	g_retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &g_henv);

	// Set the ODBC version environment attribute  
	if (g_retcode == SQL_SUCCESS || g_retcode == SQL_SUCCESS_WITH_INFO)
	{
		g_retcode = SQLSetEnvAttr(g_henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (g_retcode == SQL_SUCCESS || g_retcode == SQL_SUCCESS_WITH_INFO)
		{
			g_retcode = SQLAllocHandle(SQL_HANDLE_DBC, g_henv, &g_hdbc);

			// Set login timeout to 5 seconds  
			if (g_retcode == SQL_SUCCESS || g_retcode == SQL_SUCCESS_WITH_INFO)
			{
				SQLSetConnectAttr(g_hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				g_retcode = SQLConnect(g_hdbc, (SQLWCHAR*)L"gs_2015182030", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (g_retcode == SQL_SUCCESS || g_retcode == SQL_SUCCESS_WITH_INFO)
				{
					cout << "DATABASE Connected" << endl;
					g_retcode = SQLAllocHandle(SQL_HANDLE_STMT, g_hdbc, &g_hstmt);
				}
				else
					db_show_error(g_hdbc, SQL_HANDLE_DBC, g_retcode);
			}
		}
	}
}

void disconnect_DB()
{
	SQLFreeHandle(SQL_HANDLE_STMT, g_hstmt);
	SQLDisconnect(g_hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, g_hdbc);
	SQLFreeHandle(SQL_HANDLE_ENV, g_henv);
}

bool Check_ID(int id)
{
	SQLINTEGER level, posX, posY, hp, maxhp, exp, maxexp, att, ori_x, ori_y;
	SQLLEN cbLevel = 0, cbPosX = 0, cbPosY = 0, cbHp = 0, cbMaxHp = 0, cbExp = 0, cbMaxExp = 0, cbAtt = 0, cbOriX = 0, cbOriY = 0;

	/* 유효한 ID인지 검사하는 쿼리문 */
	std::string str_order
		= "SELECT user_level, user_x, user_y, user_hp,user_maxhp, user_exp, user_maxexp, user_att, user_ori_x, user_ori_y FROM USER_DATA WHERE user_name = '";

	str_order += g_clients[id].name;
	str_order += "'";

	std::wstring wsr_order = L"";
	wsr_order.assign(str_order.begin(), str_order.end());

	g_retcode = SQLExecDirect(g_hstmt, (SQLWCHAR*)wsr_order.c_str(), SQL_NTS);

	// 데이터 처리 시작 - 유효한 ID일 경우
	if (g_retcode == SQL_SUCCESS || g_retcode == SQL_SUCCESS_WITH_INFO)
	{
		/* Load Data on player info */
		g_retcode = SQLBindCol(g_hstmt, 1, SQL_C_LONG, &level, 100, &cbLevel);
		g_retcode = SQLBindCol(g_hstmt, 2, SQL_C_LONG, &posX, 100, &cbPosX);
		g_retcode = SQLBindCol(g_hstmt, 3, SQL_C_LONG, &posY, 100, &cbPosY);
		g_retcode = SQLBindCol(g_hstmt, 4, SQL_C_LONG, &hp, 100, &cbHp);
		g_retcode = SQLBindCol(g_hstmt, 5, SQL_C_LONG, &maxhp, 100, &cbMaxHp);
		g_retcode = SQLBindCol(g_hstmt, 6, SQL_C_LONG, &exp, 100, &cbExp);
		g_retcode = SQLBindCol(g_hstmt, 7, SQL_C_LONG, &maxexp, 100, &cbMaxExp);
		g_retcode = SQLBindCol(g_hstmt, 8, SQL_C_LONG, &att, 100, &cbAtt);
		g_retcode = SQLBindCol(g_hstmt, 9, SQL_C_LONG, &ori_x, 100, &cbOriX);
		g_retcode = SQLBindCol(g_hstmt, 10, SQL_C_LONG, &ori_y, 100, &cbOriY);

		for (int i = 0; ; i++)
		{
			g_retcode = SQLFetch(g_hstmt);
			if (g_retcode == SQL_ERROR || g_retcode == SQL_SUCCESS_WITH_INFO)
				db_show_error(g_hstmt, SQL_HANDLE_STMT, g_retcode);

			if (g_retcode == SQL_SUCCESS || g_retcode == SQL_SUCCESS_WITH_INFO)
			{
				g_clients[id].lev = level;
				g_clients[id].x = posX;
				g_clients[id].y = posY;
				g_clients[id].ori_x = ori_x;
				g_clients[id].ori_y = ori_y;
				g_clients[id].hp = hp;
				g_clients[id].maxhp = maxhp;
				g_clients[id].exp = exp;
				g_clients[id].att = att;

				SQLCloseCursor(g_hstmt);
				return true;
			}
			else
				break;
		}
	}
	else db_show_error(g_hstmt, SQL_HANDLE_STMT, g_retcode);

	SQLCloseCursor(g_hstmt);

	return false;
}

void Insert_NewPlayer_DB(int id)
{
	string name{ "'" };
	name += g_clients[id].name;
	name += "'";

	/* ID가 유효하지 않을 경우 - 새로운 회원으로 추가 */
	std::string str_order
		= "EXEC insert_user " + name + ", "
		+ to_string(g_clients[id].lev) + ", "
		+ to_string(g_clients[id].x) + ", "
		+ to_string(g_clients[id].y) + ", "
		+ to_string(g_clients[id].hp) + ", "
		+ to_string(g_clients[id].maxhp) + ", "
		+ to_string(g_clients[id].exp) + ", "
		+ to_string(g_clients[id].lev * LEVEL_UP_EXP) + ", "
		+ to_string(g_clients[id].att) + ", "
		+ to_string(g_clients[id].ori_x) + ", "
		+ to_string(g_clients[id].ori_y);

	std::wstring wstr_order = L"";
	wstr_order.assign(str_order.begin(), str_order.end());
	g_retcode = SQLExecDirect(g_hstmt, (SQLWCHAR*)wstr_order.c_str(), SQL_NTS);
	
	SQLCloseCursor(g_hstmt);
	SQLCancel(g_hstmt);
}

void Update_stat_DB(int id)
{
	if (!g_clients[id].in_use)
		return;

	string name{ "'" };
	name += g_clients[id].name;
	name += "'";

	/* 전투 후 스탯 갱신 */
	std::string str_order
		= "EXEC update_user_stat " 
		+ to_string(g_clients[id].lev) + ", "
		+ to_string(g_clients[id].exp) + ", "
		+ to_string(g_clients[id].hp) + ", "
		+ name;

	std::wstring wstr_order = L"";
	wstr_order.assign(str_order.begin(), str_order.end());
	g_retcode = SQLExecDirect(g_hstmt, (SQLWCHAR*)wstr_order.c_str(), SQL_NTS);

	SQLCloseCursor(g_hstmt);
	SQLCancel(g_hstmt);
}

void Update_move_DB(int id)
{
	if (!g_clients[id].in_use)
		return;

	string name{ "'" };
	name += g_clients[id].name;
	name += "'";

	/* 이동 후 위치 갱신 */
	std::string str_order
		= "EXEC update_user_position "
		+ to_string(g_clients[id].x) + ", "
		+ to_string(g_clients[id].y) + ", "
		+ name;

	std::wstring wstr_order = L"";
	wstr_order.assign(str_order.begin(), str_order.end());
	g_retcode = SQLExecDirect(g_hstmt, (SQLWCHAR*)wstr_order.c_str(), SQL_NTS);

	SQLCloseCursor(g_hstmt);
	SQLCancel(g_hstmt);
}
