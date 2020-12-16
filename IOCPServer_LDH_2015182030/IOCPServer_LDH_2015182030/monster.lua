myid = 99999;
moveCnt = 0;
bIsBye = false;
dir = -1;
function set_uid(x)
	myid = x;
end

-- 플레이어 & NPC 충돌 -> NPC say "HELLO"
-- NPC 3칸 이동 후 -> NPC say "BYE""
function event_player_move(player)
	player_x = API_get_x(player);
	player_y = API_get_y(player);
	my_x = API_get_x(myid);
	my_y = API_get_y(myid);

	--print (moveCnt);
	if (bIsBye == true) then							-- NPC 3칸 이동 카운트 시작
		if (moveCnt >= 2) then								-- NPC 3칸 이동 완료했을 경우 
			API_SendMessage(myid, player, "Thank You!!");			-- NPC say "BYE"
			moveCnt = 0;									-- 이동 카운트 0으로 초기화
			dir = -1;										-- 움직일 방향 초기화
			bIsBye = false;									-- NPC 3칸 카운트 중지
			API_IsBye(myid,dir,0);							-- 해당 NPC 움직임 중지, 방향 초기화 한 정보를 보내주기 
		else											-- NPC가 아직 3칸을 움직이지 않았을 경우
			moveCnt = moveCnt + 1;							-- 카운트를 계속 증가시킨다.
			API_IsBye(myid,dir,1);							-- 움직일 방향과 움직임 시작을 알려준다.
		end
	end

	if (player_x == my_x) then
		if (player_y == my_y) then						-- 플레이어 & NPC 충돌
			API_SendMessage(myid, player, "Help Me.....");			-- NPC say "HELLO"
			moveCnt = 0;									-- 카운트 변수 0 초기화
			dir = math.random(0,3)							-- 움직일 방향 설정
			if(bIsBye == false) then						
				API_IsBye(myid,dir,1);						-- 초반 1번은 방향 & 움직임 명령 직접 전달
			end
			bIsBye = true;									-- NPC 3칸 이동 카운트 시작
		end
	end
end