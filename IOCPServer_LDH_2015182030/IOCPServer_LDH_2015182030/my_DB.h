#pragma once

void connect_DB();						// Connect to Database Server 
void disconnect_DB();					// Disconnect Database Server

bool Check_ID(int id);					// Check Id in Login Server

void Insert_NewPlayer_DB(int id);		// Insert New Player in to Database Server
void Update_stat_DB(int id);			// Update User Stat in to Database Server
void Update_move_DB(int id);			// Update User position in to Database Server