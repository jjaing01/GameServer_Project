#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <windows.h>
#include <iostream>
#include <unordered_map>
#include <chrono>
using namespace std;
using namespace chrono;

#include "..\..\IOCPServer_LDH_2015182030\IOCPServer_LDH_2015182030\protocol.h"
#define ADD
sf::TcpSocket g_socket;

constexpr auto SCREEN_WIDTH = 20;
constexpr auto SCREEN_HEIGHT = 20;

constexpr auto TILE_WIDTH = 65;
constexpr auto WINDOW_WIDTH = TILE_WIDTH * SCREEN_WIDTH / 2 + 10;   // size of window
constexpr auto WINDOW_HEIGHT = TILE_WIDTH * SCREEN_WIDTH / 2 + 10;
constexpr auto BUF_SIZE = 200;

// 추후 확장용.
int NPC_ID_START = 10000;

int g_left_x;
int g_top_y;
int g_myid;
bool g_bIsChat = false;
bool g_bIsChatStart = false;

sf::RenderWindow* g_window;
sf::Font g_font;

#ifdef ADD
string chatting;
#endif // ADD

class OBJECT {
private:
	bool m_showing;
	sf::Sprite m_sprite;

	char m_mess[MAX_STR_LEN];
	high_resolution_clock::time_point m_time_out;
	sf::Text m_text;
	sf::Text m_name;
	sf::Text m_Level;

public:
	int m_x, m_y;
	short hp = 100;
	short level = 1;
	int exp = 0;
	char name[MAX_ID_LEN];
	OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
		m_showing = false;
		m_sprite.setTexture(t);
		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
		m_time_out = high_resolution_clock::now();
	}
	OBJECT() {
		m_showing = false;
		m_time_out = high_resolution_clock::now();
	}
	void show()
	{
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void a_move(int x, int y) {
		m_sprite.setPosition((float)x, (float)y);
	}

	void a_draw() {
		g_window->draw(m_sprite);
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	void draw() {
		if (false == m_showing) return;
		float rx = (m_x - g_left_x) * 65.0f + 8;
		float ry = (m_y - g_top_y) * 65.0f + 8;
		m_sprite.setPosition(rx, ry);
		g_window->draw(m_sprite);
		m_name.setPosition(rx - 10, ry - 10);
		g_window->draw(m_name);
		if (high_resolution_clock::now() < m_time_out) 
		{
			m_text.setCharacterSize(35);
			m_text.setPosition(rx - 10, ry - 50);
			g_window->draw(m_text);
		}
	}
	void npc_draw()
	{
		if (false == m_showing) return;
		float rx = (m_x - g_left_x) * 65.0f + 8;
		float ry = (m_y - g_top_y) * 65.0f + 8;
		m_sprite.setPosition(rx, ry);
		g_window->draw(m_sprite);
		m_name.setPosition(rx - 10, ry - 10);
		g_window->draw(m_name);

		m_Level.setCharacterSize(35); // term project
		m_Level.setPosition(rx - 20, ry - 60);
		g_window->draw(m_Level);

		if (high_resolution_clock::now() < m_time_out)
		{
			m_text.setCharacterSize(35); // term project
			m_text.setPosition(rx - 10, ry - 50);
			g_window->draw(m_text);
		}
	}
	void set_name(char str[]) {
		m_name.setFont(g_font);
		m_name.setString(str);
		m_name.setFillColor(sf::Color(255, 255, 0));
		m_name.setOutlineThickness(2); // 굵기
		m_name.setOutlineColor(sf::Color::Black); // 윤곽선
		m_name.setStyle(sf::Text::Bold);
	}

	void set_lev_hp(char str[]) {
		m_Level.setFont(g_font);
		m_Level.setString(str);
		m_Level.setFillColor(sf::Color(255, 0, 0));
		m_Level.setOutlineThickness(2); // 굵기
		m_Level.setOutlineColor(sf::Color::Yellow); // 윤곽선
		m_Level.setStyle(sf::Text::Bold);
	}

	void add_chat(char chat[]) {
		m_text.setFont(g_font);
		m_text.setString(chat);

		m_text.setFillColor(sf::Color::Red);
		m_text.setOutlineThickness(5);
		m_text.setOutlineColor(sf::Color::Black);
		

		cout << chat << endl;//
		m_time_out = high_resolution_clock::now() + 3s;
	}
};

OBJECT avatar;
unordered_map <int, OBJECT> npcs;

OBJECT white_tile;
OBJECT black_tile;

sf::Texture* board;
sf::Texture* pieces;

void client_initialize()
{
	board = new sf::Texture;
	pieces = new sf::Texture;
	if (false == g_font.loadFromFile("cour.ttf")) {
		cout << "Font Loading Error!\n";
		while (true);
	}
	board->loadFromFile("chessmap.bmp");
	pieces->loadFromFile("chess2.png");
	white_tile = OBJECT{ *board, 5, 5, TILE_WIDTH, TILE_WIDTH };
	black_tile = OBJECT{ *board, 69, 5, TILE_WIDTH, TILE_WIDTH };
	avatar = OBJECT{ *pieces, 128, 0, 64, 64 };
	avatar.move(4, 4);
}

void client_finish()
{
	delete board;
	delete pieces;
}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_PACKET_LOGIN_OK:
	{
		sc_packet_login_ok* my_packet = reinterpret_cast<sc_packet_login_ok*>(ptr);
		g_myid = my_packet->id;
		avatar.move(my_packet->x, my_packet->y);
		g_left_x = my_packet->x - (SCREEN_WIDTH / 2);
		g_top_y = my_packet->y - (SCREEN_HEIGHT / 2);
		avatar.show();
	}
	break;

	case SC_PACKET_ENTER:
	{
		sc_packet_enter* my_packet = reinterpret_cast<sc_packet_enter*>(ptr);
		int id = my_packet->id;
		int obj_type = my_packet->o_type;

		if (id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - (SCREEN_WIDTH / 2);
			g_top_y = my_packet->y - (SCREEN_HEIGHT / 2);
			avatar.show();
		}
		else {
			switch (obj_type)
			{
			case TYPE_PLAYER:
				npcs[id] = OBJECT{ *pieces, 256, 0, 64, 64 };  // 나이트
				break;
			case TYPE_ORC:
				npcs[id] = OBJECT{ *pieces, 320, 0, 64, 64 };  // 나이트
				break;
			case TYPE_ELF:
				npcs[id] = OBJECT{ *pieces, 64, 0, 64, 64 };  // 엘프
				break;
			case TYPE_BOSS:
				npcs[id] = OBJECT{ *pieces, 192, 0, 64, 64 };  // 퀸
				break;
			case TYPE_NORMAL:
				npcs[id] = OBJECT{ *pieces, 0, 0, 64, 64 };  // 노말운영자
				break;
			default:
				break;
			}
			strcpy_s(npcs[id].name, my_packet->name);
			npcs[id].set_name(my_packet->name);
			npcs[id].move(my_packet->x, my_packet->y);
			npcs[id].show();
		}
	}
	break;
	case SC_PACKET_MOVE:
	{
		sc_packet_move* my_packet = reinterpret_cast<sc_packet_move*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - (SCREEN_WIDTH / 2);
			g_top_y = my_packet->y - (SCREEN_HEIGHT / 2);
		}
		else {
			if (0 != npcs.count(other_id))
				npcs[other_id].move(my_packet->x, my_packet->y);
		}
	}
	break;

	case SC_PACKET_LEAVE:
	{
		sc_packet_leave* my_packet = reinterpret_cast<sc_packet_leave*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.hide();
		}
		else {
			if (0 != npcs.count(other_id))
				npcs[other_id].hide();
		}
	}
	break;

	case SC_PACKET_CHAT:
	{
		sc_packet_chat* my_packet = reinterpret_cast<sc_packet_chat*>(ptr);

		npcs[my_packet->id].add_chat(my_packet->message);
	}
	break;
#ifdef ADD
	case SC_PACKET_STAT_CHANGE:
	{
		sc_packet_stat_change* my_packet = reinterpret_cast<sc_packet_stat_change*>(ptr);

		int other_id = my_packet->id;
		if (other_id == g_myid)
		{
			avatar.level = my_packet->level;
			avatar.exp = my_packet->exp;
			avatar.hp = my_packet->hp;
		}
		else
		{
			npcs[other_id].level = my_packet->level;
			npcs[other_id].exp = my_packet->exp;
			npcs[other_id].hp = my_packet->hp;
			
			char buf_npc[MAX_STR_LEN];
			sprintf_s(buf_npc, "Level:%d / HP:%d", npcs[other_id].level, npcs[other_id].hp);
			npcs[other_id].set_lev_hp(buf_npc);
		}
		
		break;
	}

#endif // ADD
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
		break;
	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void client_main()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = g_socket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		while (true);
	}

	if (recv_result == sf::Socket::Disconnected)
	{
		wcout << L"서버 접속 종료.";
		g_window->close();
	}

	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);

	for (int i = 0; i < SCREEN_WIDTH; ++i) {
		int tile_x = i + g_left_x;
		if (tile_x >= WORLD_WIDTH) break;
		if (tile_x < 0) continue;
		for (int j = 0; j < SCREEN_HEIGHT; ++j)
		{
			int tile_y = j + g_top_y;
			if (tile_y >= WORLD_HEIGHT) break;
			if (tile_y < 0) continue;
			if (((tile_x / 3 + tile_y / 3) % 2) == 0) {
				white_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
				white_tile.a_draw();
			}
			else
			{
				black_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
				black_tile.a_draw();
			}
		}
	}
	avatar.draw();
	
	for (auto& npc : npcs)
	{
		if (npc.second.hp <= 0)
			npc.second.hide();
		else
			npc.second.npc_draw();
	}
	
	/* 위치 좌표 */
	sf::Text text;
	text.setFont(g_font);
	char buf[100];
	
#ifdef ADD
	/* 레벨 & 경험치 */
	sf::Text level;
	level.setFont(g_font);
	//char buf[40];
	sprintf_s(buf, "Level:%d / Exp:%d", avatar.level, avatar.exp);
	level.setString(buf);
	level.setFillColor(sf::Color::Green);
	level.setOutlineThickness(5);
	level.setCharacterSize(50);
	level.setOutlineThickness(5);
	level.setOutlineColor(sf::Color::Black);
	g_window->draw(level);

	/* HP & POS */
	sf::Text HpPos;
	HpPos.setFont(g_font);
	sprintf_s(buf, "Hp:%d / Pos:(%d, %d)", avatar.hp, avatar.m_x, avatar.m_y);
	HpPos.setString(buf);
	HpPos.setFillColor(sf::Color::Green);
	HpPos.setOutlineThickness(5);
	HpPos.setOutlineColor(sf::Color::Black);
	HpPos.setCharacterSize(50);
	//HpPos.setPosition(0, 1250);
	HpPos.setPosition(0, 100);
	g_window->draw(HpPos);

	/* chatting box */
	sf::RectangleShape chat;
	chat.setPosition(800, 900);
	chat.setSize(sf::Vector2f(800, 400));
	chat.setFillColor(sf::Color(0, 0, 0, 200));
	g_window->draw(chat);

	sf::RectangleShape chat2;
	chat2.setPosition(800, 1250);
	chat2.setSize(sf::Vector2f(800, 40));
	chat2.setFillColor(sf::Color(200, 200, 200, 200));
	g_window->draw(chat2);

	sf::Text Chatting;
	Chatting.setFont(g_font);
	Chatting.setString(chatting);
	Chatting.setPosition(810, 1250);
	g_window->draw(Chatting);

#endif // ADD
}

void send_packet(void* packet)
{
	char* p = reinterpret_cast<char*>(packet);
	size_t sent;
	sf::Socket::Status st = g_socket.send(p, p[0], sent);
	int a = 3;
}

void send_move_packet(unsigned char dir)
{
	cs_packet_move m_packet;
	m_packet.type = CS_MOVE;
	m_packet.size = sizeof(m_packet);
	m_packet.direction = dir;
	send_packet(&m_packet);
}

void send_attack_packet()
{
	cs_packet_attack m_packet;
	m_packet.size = sizeof(m_packet);
	m_packet.type = CS_ATTACK;
	send_packet(&m_packet);
}

void send_chat_packet(const char chat[])
{
	cs_packet_chat m_packet;
	m_packet.size = sizeof(m_packet);
	m_packet.type = CS_CHAT;
	strcpy_s(m_packet.message, chat);
	send_packet(&m_packet);
}

int main()
{
	wcout.imbue(locale("korean"));
	sf::Socket::Status status = g_socket.connect("127.0.0.1", SERVER_PORT);
	g_socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"서버와 연결할 수 없습니다.\n";
		while (true);
	}

	client_initialize();

	cs_packet_login l_packet;
	l_packet.size = sizeof(l_packet);
	l_packet.type = CS_LOGIN;
	int t_id = GetCurrentProcessId();
	sprintf_s(l_packet.name, "P%03d", t_id % 1000);
	strcpy_s(avatar.name, l_packet.name);
	avatar.set_name(l_packet.name);
	send_packet(&l_packet);

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
	g_window = &window;

	sf::View view = g_window->getView();
	view.zoom(2.0f);
	view.move(SCREEN_WIDTH * TILE_WIDTH / 4, SCREEN_HEIGHT * TILE_WIDTH / 4);
	g_window->setView(view);

	g_bIsChat = false;
	g_bIsChatStart = false;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::KeyPressed) 
			{
				int p_type = -1;

				switch (event.key.code) 
				{
				case sf::Keyboard::Left:
					send_move_packet(MV_LEFT);
					break;
				case sf::Keyboard::Right:
					send_move_packet(MV_RIGHT);
					break;
				case sf::Keyboard::Up:
					send_move_packet(MV_UP);
					break;
				case sf::Keyboard::Down:
					send_move_packet(MV_DOWN);
					break;
#ifdef ADD
					/* 공격 */
				case sf::Keyboard::A:
					send_attack_packet();
					break;
				
				case sf::Keyboard::Enter: 
				{
					g_bIsChatStart = !g_bIsChatStart;
					if (!g_bIsChatStart)
					{
						avatar.add_chat((char*)chatting.c_str());
						send_chat_packet(chatting.c_str());
						chatting.clear();
						g_bIsChat = false;
					}
					else
						g_bIsChat = true;
				}
				break;
#endif // ADD
				case sf::Keyboard::BackSpace:
				{
					if (chatting.size() != 0)
					{
						chatting.pop_back();
						std::cout << chatting << std::endl;
					}
					g_bIsChat = false;
				}
				break;

				case sf::Keyboard::Escape:
					window.close();
					break;

				default:
					if (g_bIsChatStart)
						g_bIsChat = true;
					break;
				}

			}
#ifdef ADD
			if (g_bIsChat && (event.type == sf::Event::TextEntered) &&
				event.key.code != sf::Keyboard::BackSpace)
			{
				if (event.text.unicode < 128)
				{
					chatting.push_back((char)event.text.unicode);
					std::cout << chatting << "   " << chatting.size() << std::endl;
				}	
			}
#endif // ADD
		
		}

		window.clear();
		client_main();
		window.display();
	}
	client_finish();

	return 0;
}