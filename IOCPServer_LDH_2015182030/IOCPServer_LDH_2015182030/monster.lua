myid = 99999;
moveCnt = 0;
bIsBye = false;
dir = -1;
function set_uid(x)
	myid = x;
end

-- �÷��̾� & NPC �浹 -> NPC say "HELLO"
-- NPC 3ĭ �̵� �� -> NPC say "BYE""
function event_player_move(player)
	player_x = API_get_x(player);
	player_y = API_get_y(player);
	my_x = API_get_x(myid);
	my_y = API_get_y(myid);

	--print (moveCnt);
	if (bIsBye == true) then							-- NPC 3ĭ �̵� ī��Ʈ ����
		if (moveCnt >= 2) then								-- NPC 3ĭ �̵� �Ϸ����� ��� 
			API_SendMessage(myid, player, "Thank You!!");			-- NPC say "BYE"
			moveCnt = 0;									-- �̵� ī��Ʈ 0���� �ʱ�ȭ
			dir = -1;										-- ������ ���� �ʱ�ȭ
			bIsBye = false;									-- NPC 3ĭ ī��Ʈ ����
			API_IsBye(myid,dir,0);							-- �ش� NPC ������ ����, ���� �ʱ�ȭ �� ������ �����ֱ� 
		else											-- NPC�� ���� 3ĭ�� �������� �ʾ��� ���
			moveCnt = moveCnt + 1;							-- ī��Ʈ�� ��� ������Ų��.
			API_IsBye(myid,dir,1);							-- ������ ����� ������ ������ �˷��ش�.
		end
	end

	if (player_x == my_x) then
		if (player_y == my_y) then						-- �÷��̾� & NPC �浹
			API_SendMessage(myid, player, "Help Me.....");			-- NPC say "HELLO"
			moveCnt = 0;									-- ī��Ʈ ���� 0 �ʱ�ȭ
			dir = math.random(0,3)							-- ������ ���� ����
			if(bIsBye == false) then						
				API_IsBye(myid,dir,1);						-- �ʹ� 1���� ���� & ������ ��� ���� ����
			end
			bIsBye = true;									-- NPC 3ĭ �̵� ī��Ʈ ����
		end
	end
end