#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <imgui_demo.cpp>

#include "../game/game.h"
#include "overlay.h"

#define DEFAULT_ALPHA 0.87f

static char szHostIp[IP_BUFFER_SIZE];
static unsigned short nSyncPort;
static unsigned short nOurGGPOPort;


IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); 


/* Parses input flags, which may be based on non-default button settings,
and returns normalized input flags */
unsigned int normalizeInput(GameState* lpGameState, unsigned int* input) {
	int p = lpGameState->ggpoState.localPlayerIndex;

	unsigned int normalizedInput = 0;

	normalizedInput |= (*input & Up);
	normalizedInput |= (*input & Down);
	normalizedInput |= (*input & Left);
	normalizedInput |= (*input & Right);

	if (*input & lpGameState->arrPlayerData[p].ctrlP) {
		normalizedInput |= Punch;
	}
	if (*input & lpGameState->arrPlayerData[p].ctrlK) {
		normalizedInput |= Kick;
	}
	if (*input & lpGameState->arrPlayerData[p].ctrlS) {
		normalizedInput |= Slash;
	}
	if (*input & lpGameState->arrPlayerData[p].ctrlH) {
		normalizedInput |= HSlash;
	}
	if (*input & lpGameState->arrPlayerData[p].ctrlD) {
		normalizedInput |= Dust;
	}
	if (*input & lpGameState->arrPlayerData[p].ctrlRespect) {
		normalizedInput |= Respect;
	}
	if (*input & lpGameState->arrPlayerData[p].ctrlPKMacro) {
		normalizedInput |= Punch;
		normalizedInput |= Kick;
	}
	if (*input & lpGameState->arrPlayerData[p].ctrlPDMacro) {
		normalizedInput |= Punch;
		normalizedInput |= Dust;
	}
	if (*input & lpGameState->arrPlayerData[p].ctrlPKSMacro) {
		normalizedInput |= Punch;
		normalizedInput |= Kick;
		normalizedInput |= Slash;
	}
	if (*input & lpGameState->arrPlayerData[p].ctrlPKSHMacro) {
		normalizedInput |= Punch;
		normalizedInput |= Kick;
		normalizedInput |= Slash;
		normalizedInput |= HSlash;
	}
	

	return normalizedInput;
}

void DrawEnterVersus2PWindow(GameState* lpGameState, bool* pOpen) {
	static CharacterSelection* characters[2] = { &CHARACTERS[0], &CHARACTERS[1] };
	static int characterIDs[2] = { 1, 2 };
	static StageSelection* stage = &STAGES[0];

	ImGui::Begin("Character Select Helper", pOpen);

	for (int i = 0; i < 2; i++) {
		char* comboLabel = i == 0 ? "Character 1" : "Character 2";
		CharacterSelection* c = characters[i];
		if (ImGui::BeginCombo(comboLabel, c->name, 0)) {
			for (int j = 0; j < IM_ARRAYSIZE(CHARACTERS); j++) {
				bool is_selected = (c == &CHARACTERS[j]);
				if (ImGui::Selectable(CHARACTERS[j].name, is_selected)) {
					c = &CHARACTERS[j];
					characters[i] = c;
					characterIDs[i] = (unsigned short)CHARACTERS[j].value;
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	if (ImGui::BeginCombo("Stage", stage->name, 0))
	{
		for (int n = 0; n < IM_ARRAYSIZE(STAGES); n++)
		{
			bool is_selected = (stage == &STAGES[n]);
			if (ImGui::Selectable(STAGES[n].name, is_selected)) {
				stage = &STAGES[n];
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	

	if (ImGui::Button("Go")) {
		EnterVersus2P(lpGameState, characterIDs, stage);
	}
	ImGui::End();
}

void DrawGGPOStats(GameState* lpGameState) {
	static GGPONetworkStats stats;

	GGPOState* gs = &(lpGameState->ggpoState);
	int remotePlayerIndex = gs->localPlayerIndex == 0 ? 1 : 0;

	ggpo_get_network_stats(gs->ggpo, gs->player_handles[remotePlayerIndex], &stats);
	ImGui::Text("Handles: %d, %d", gs->player_handles[0], gs->player_handles[1]);
	ImGui::Text("Player IDs: %d, %d", gs->p1.player_num, gs->p2.player_num);
	ImGui::Text("Player addresses: %p, %p", &(gs->p1), &(gs->p2));
	ImGui::Text("Remote player address: %p", gs->remotePlayer);
	ImGui::Text("Local player address: %p", gs->localPlayer);

	ImGui::Text("Remote player values");
	ImGui::Text("num: %d", gs->remotePlayer->player_num);
	ImGui::Text("IP: %s", gs->remotePlayer->u.remote.ip_address);
	ImGui::Text("port: %d", gs->remotePlayer->u.remote.port);
	ImGui::Text("type: %d", gs->remotePlayer->type);

	ImGui::Text("Stats");
	ImGui::Text("send_queue_len %d", stats.network.send_queue_len);
	ImGui::Text("recv_queue_len %d", stats.network.recv_queue_len);
	ImGui::Text("ping %d", stats.network.ping);
	ImGui::Text("kbps_sent %d", stats.network.kbps_sent);
	ImGui::Text("local_frames_behind %d", stats.timesync.local_frames_behind);
	ImGui::Text("remote_frames_behind %d", stats.timesync.remote_frames_behind);
}

void DrawGGPOJoinWindow(GameState* lpGameState, bool* pOpen) {
	static GGPONetworkStats stats;
	static bool load_vdf = false;

	static CharacterSelection* lpCharacter = &CHARACTERS[0];

	GGPOState* gs = &(lpGameState->ggpoState);

	if (!load_vdf) {
		LoadGGPOInfo(lpGameState, nSyncPort, nOurGGPOPort, szHostIp);
		load_vdf = true;
	}

	ImGui::Begin("GGPO Join", pOpen);
	ImGui::Text("Num frames simulated per second: %d", lpGameState->lastSecondNumFramesSimulated);
	
	ImGui::InputText("Host IP", szHostIp, IP_BUFFER_SIZE);
	ImGui::InputScalar("Sync port", ImGuiDataType_U16, &nSyncPort);
	ImGui::InputScalar("GGPO Our port", ImGuiDataType_U16, &nOurGGPOPort);

	if (ImGui::BeginCombo("Selected character", lpCharacter->name, 0)) {
		for (int n = 0; n < IM_ARRAYSIZE(CHARACTERS); n++) {
			bool is_selected = (lpCharacter == &CHARACTERS[n]);
			if (ImGui::Selectable(CHARACTERS[n].name, is_selected)) {
				lpCharacter = &CHARACTERS[n];
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (lpGameState->ggpoState.ggpo == NULL) {
		if (lpGameState->sessionInitState.hSyncThread != NULL) {
			ImGui::Text("Synchronization thread started...");
		} else if (ImGui::Button("Prepare for connection")) {
			SaveGGPOInfo(lpGameState, nSyncPort, nOurGGPOPort, szHostIp);
			lpGameState->sessionInitState.hSyncThread = CreateSynchronizeClientThread(lpGameState,
				szHostIp,
				nSyncPort,
				nOurGGPOPort,
				lpCharacter->value);
		}
	}
	else {
		DrawGGPOStats(lpGameState);
	}
	ImGui::End();
}

void DrawGGPOHostWindow(GameState* lpGameState, bool* pOpen) {
	static GGPONetworkStats stats;
	static bool load_vdf = false;
//	char buf[2];
	static CharacterSelection* lpCharacter = &CHARACTERS[0];

	if (!load_vdf) {
		LoadGGPOInfo(lpGameState, nSyncPort, nOurGGPOPort);
		load_vdf = true;
	}

	ImGui::Begin("GGPO Host", pOpen);
	ImGui::InputScalar("Sync port", ImGuiDataType_U16, &nSyncPort);
	ImGui::InputScalar("Our GGPO port", ImGuiDataType_U16, &nOurGGPOPort);
	if (ImGui::BeginCombo("Selected character", lpCharacter->name, 0)) {
		for (int n = 0; n < IM_ARRAYSIZE(CHARACTERS); n++) {
			bool is_selected = (lpCharacter == &CHARACTERS[n]);
			if (ImGui::Selectable(CHARACTERS[n].name, is_selected)) {
				lpCharacter = &CHARACTERS[n];
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (lpGameState->ggpoState.ggpo == NULL) {
		if (lpGameState->sessionInitState.hSyncThread != NULL) {
			ImGui::Text("Synchronization thread started...");
		} else if (ImGui::Button("Prepare for connection")) {
			SaveGGPOInfo(lpGameState, nSyncPort, nOurGGPOPort);
			lpGameState->sessionInitState.hSyncThread = CreateSynchronizeServerThread(lpGameState,
				nSyncPort,
				nOurGGPOPort,
				lpCharacter->value);
		}
	}
	else {
		DrawGGPOStats(lpGameState);
	}
	ImGui::End();
}

void DrawGlobalStateWindow(GameState* lpGameState, bool* pOpen) {
	static CharacterSelection* selCharacter = &CHARACTERS[0];

	CharacterConstants* cc = &lpGameState->characterConstants;
	PlayData* pd = &lpGameState->playData;
	ImGui::Begin("Global State", pOpen, ImGuiWindowFlags_None);

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
	{
		if (ImGui::BeginTabItem("Global state"))
		{
			ImGui::Columns(2, NULL, false);

			ImGui::Text("Hitbox display enabled:"); ImGui::NextColumn(); ImGui::Text("%d", *lpGameState->bHitboxDisplayEnabled); ImGui::NextColumn();
			ImGui::Text("Camera x position:"); ImGui::NextColumn(); ImGui::Text("%f", *lpGameState->fCameraXPos); ImGui::NextColumn();
			ImGui::Text("Camera hold timer:"); ImGui::NextColumn(); ImGui::Text("%d", *lpGameState->nCameraHoldTimer); ImGui::NextColumn();
			ImGui::Text("Camera zoom:"); ImGui::NextColumn(); ImGui::Text("%d", *lpGameState->nCameraZoom); ImGui::NextColumn();
			ImGui::Text("Playfield left edge:"); ImGui::NextColumn(); ImGui::Text("%d", *lpGameState->nPlayfieldLeftEdge); ImGui::NextColumn();
			ImGui::Text("Playfield top edge:"); ImGui::NextColumn(); ImGui::Text("%d", *lpGameState->nPlayfieldTopEdge); ImGui::NextColumn();
			ImGui::Text("Round time remaining:"); ImGui::NextColumn(); ImGui::Text("%d", *lpGameState->nRoundTimeRemaining); ImGui::NextColumn();
			ImGui::Text("RNG1 index:"); ImGui::NextColumn(); ImGui::Text("%d", lpGameState->lpRNG1->cursor); ImGui::NextColumn();
			ImGui::Text("RNG2 index:"); ImGui::NextColumn(); ImGui::Text("%d", lpGameState->lpRNG2->cursor); ImGui::NextColumn();
			ImGui::Text("RNG3 index:"); ImGui::NextColumn(); ImGui::Text("%d", lpGameState->lpRNG3->cursor); ImGui::NextColumn();
			ImGui::Text("Character root:"); ImGui::NextColumn(); ImGui::Text("%p", *lpGameState->arrCharacters); ImGui::NextColumn();
			ImGui::Text("NPC root:"); ImGui::NextColumn(); ImGui::Text("%p", *lpGameState->arrNpcObjects); ImGui::NextColumn();
			ImGui::Text("Player data root:"); ImGui::NextColumn(); ImGui::Text("%p", lpGameState->arrPlayerData); ImGui::NextColumn();
			ImGui::Text("Window handle:"); ImGui::NextColumn(); ImGui::Text("%d", *lpGameState->hWnd); ImGui::NextColumn();
			ImGui::Text("Current Game Tick:"); ImGui::NextColumn(); ImGui::Text("%d", *lpGameState->cGameTick); ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::End();
}

void DrawCharacterDataWindow(GameState* lpGameState, bool* pOpen) {
	static CharacterSelection* selCharacter = &CHARACTERS[0];

	CharacterConstants* cc = &lpGameState->characterConstants;
	PlayData* pd = &lpGameState->playData;
	ImGui::Begin("Character Data", pOpen, ImGuiWindowFlags_None);
	

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
	{
		if (ImGui::BeginTabItem("Character constants"))
		{
			ImGui::BeginChild("left pane", ImVec2(150, 0), true);
			for (int i = 0; i < IM_ARRAYSIZE(CHARACTERS); i++) {
				if (ImGui::Selectable(CHARACTERS[i].name, selCharacter->value == CHARACTERS[i].value)) {
					selCharacter = &CHARACTERS[i];
				}
			}
			ImGui::EndChild();
			ImGui::SameLine();

			// right
			ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
			ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.33f);
			ImGui::InputScalar("Standing pushbox width", ImGuiDataType_S16, &cc->arrnStandingPushboxWidth[selCharacter->value]);
			ImGui::InputScalar("Vanilla standing pushbox height", ImGuiDataType_S16, &cc->arrnVanillaStandingPushboxHeight[selCharacter->value]);
			ImGui::InputScalar("+R standing pushbox height", ImGuiDataType_S16, &cc->arrnPlusRStandingPushboxHeight[selCharacter->value]);
			ImGui::InputScalar("Crouching pushbox width", ImGuiDataType_S16, &cc->arrnCrouchingPushboxWidth[selCharacter->value]);
			ImGui::InputScalar("Crouching pushbox height", ImGuiDataType_S16, &cc->arrnCrouchingPushboxHeight[selCharacter->value]);
			ImGui::InputScalar("Aerial pushbox width", ImGuiDataType_S16, &cc->arrnAerialPushboxWidth[selCharacter->value]);
			ImGui::InputScalar("Aerial pushbox height", ImGuiDataType_S16, &cc->arrnAerialPushboxHeight[selCharacter->value]);
			ImGui::InputScalar("Vanilla aerial pushbox Y offset", ImGuiDataType_S16, &cc->arrnVanillaAerialPushboxYOffset[selCharacter->value]);
			ImGui::InputScalar("+R aerial pushbox Y offset", ImGuiDataType_S16, &cc->arrnPlusRAerialPushboxYOffset[selCharacter->value]);
			ImGui::InputScalar("Close slash max distance", ImGuiDataType_S16, &cc->arrnCloseSlashMaxDistance[selCharacter->value]);
			ImGui::InputScalar("Vanilla allowed normals", ImGuiDataType_U32, &cc->arrnVanillaAllowedNormals[selCharacter->value]);
			ImGui::InputScalar("Vanilla EX allowed normals", ImGuiDataType_U32, &cc->arrnVanillaEXAllowedNormals[selCharacter->value]);
			ImGui::InputScalar("+R allowed normals", ImGuiDataType_U32, &cc->arrnPlusRAllowedNormals[selCharacter->value]);
			ImGui::InputScalar("+R EX allowed normals", ImGuiDataType_U32, &cc->arrnPlusREXAllowedNormals[selCharacter->value]);
			ImGui::InputScalar("Vanilla standing throw distance", ImGuiDataType_S16, &cc->arrnVanillaStandingThrowDistance[selCharacter->value]);
			ImGui::InputScalar("+R standing throw distance", ImGuiDataType_S16, &cc->arrnPlusRStandingThrowDistance[selCharacter->value]);
			ImGui::InputScalar("Vanilla aerial throw distance", ImGuiDataType_S16, &cc->arrnVanillaAerialThrowDistance[selCharacter->value]);
			ImGui::InputScalar("+R aerial throw distance", ImGuiDataType_S16, &cc->arrnPlusRAerialThrowDistance[selCharacter->value]);
			ImGui::InputScalar("Max aerial throw height difference", ImGuiDataType_S16, &cc->arrnMaxAerialThrowVerticalDifference[selCharacter->value]);
			ImGui::InputScalar("Min aerial throw height difference", ImGuiDataType_S16, &cc->arrnMinAerialThrowVerticalDifference[selCharacter->value]);
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Player Character Data"))
		{
			if (ImGui::BeginTabBar("playdata tab bars", tab_bar_flags)) {
				for (int i = 0; i < 2; i++) {
					if (ImGui::BeginTabItem(i == 0 ? "Player 1" : "Player 2")) {
						ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.33f);
						ImGui::InputScalar("Forward walk velocity", ImGuiDataType_S16, &pd->arrnFWalkVel[i]);
						ImGui::InputScalar("Backward walk velocity", ImGuiDataType_S16, &pd->arrnBWalkVel[i]);
						ImGui::InputScalar("Forward dash startup speed", ImGuiDataType_S16, &pd->arrnFDashStartupSpeed[i]);
						ImGui::InputScalar("Backdash X velocity", ImGuiDataType_S16, &pd->arrnBDashXVel[i]);
						ImGui::InputScalar("Backdash Y velocity", ImGuiDataType_S16, &pd->arrnBDashYVel[i]);
						ImGui::InputScalar("Backdash gravity", ImGuiDataType_S16, &pd->arrnBDashGravity[i]);
						ImGui::InputScalar("Forward jump minimum X velocity", ImGuiDataType_S16, &pd->arrnFJumpXVel[i]);
						ImGui::InputScalar("Back jump minimum X velocity", ImGuiDataType_S16, &pd->arrnBJumpXVel[i]);
						ImGui::InputScalar("Jump starting Y velocity", ImGuiDataType_S16, &pd->arrnJumpHeight[i]);
						ImGui::InputScalar("Jump gravity", ImGuiDataType_S16, &pd->arrnGravity[i]);
						ImGui::InputScalar("Forward superjump minimum X velocity", ImGuiDataType_S16, &pd->arrnFSuperJumpXVel[i]);
						ImGui::InputScalar("Back superjump minimum X velocity", ImGuiDataType_S16, &pd->arrnBSuperJumpXVel[i]);
						ImGui::InputScalar("Superjump starting Y velocity", ImGuiDataType_S16, &pd->arrnSuperJumpYVel[i]);
						ImGui::InputScalar("Superjump gravity", ImGuiDataType_S16, &pd->arrnSuperJumpGravity[i]);
						ImGui::InputScalar("Airdashes granted", ImGuiDataType_S16, &pd->arrnAirdashesGranted[i]);
						ImGui::InputScalar("Airjumps granted", ImGuiDataType_S16, &pd->arrnAirJumpsGranted[i]);
						ImGui::InputScalar("Forward walk tension value", ImGuiDataType_S16, &pd->arrnFWalkTension[i]);
						ImGui::InputScalar("Forward jump tension value", ImGuiDataType_S16, &pd->arrnFJumpAscentTension[i]);
						ImGui::InputScalar("Forward dash tension value", ImGuiDataType_S16, &pd->arrnFDashTension[i]);
						ImGui::InputScalar("Forward airdash tension value", ImGuiDataType_S16, &pd->arrnFAirdashTension[i]);
						ImGui::EndTabItem();
					}
				}
				ImGui::EndTabBar();
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::End();
}

void DrawObjectStateWindow(GameObjectData* lpGameObject, GameState* lpGameState, bool* pOpen) {
	ImGui::Begin(
		lpGameObject->playerIndex == 0 ? "Player 1 Object State" : "Player 2 Object State",
		pOpen,
		ImGuiWindowFlags_None
	);

	ImGui::Columns(2, NULL, false);
	static int lastAction[2] = { 0, 0 };
	static WORD currentFrame;
	static WORD previousFrame;
	

	if (lpGameObject->actNo != lastAction[lpGameObject->playerIndex]) {
		previousFrame = currentFrame;
		currentFrame = *lpGameState->cGameTick;

		lastAction[lpGameObject->playerIndex] = lpGameObject->actNo;	
	}

	ImGui::Text("Object address:");
	ImGui::Text("Object ID:");
	ImGui::Text("Action ID:");
	ImGui::Text("Action ID Entry Frame:");
	ImGui::Text("Object Facing:");
	ImGui::Text("Object Side:");
	ImGui::Text("Player data address:");
	ImGui::Text("X Position:");
	ImGui::Text("Y Position:");
	ImGui::Text("X Velocity:");
	ImGui::Text("Y Velocity:");
	ImGui::Text("Previous Action ID:");
	ImGui::Text("Previous Action ID Entry Frame:");
	ImGui::Text("Time in Previous Action ID:");


	ImGui::NextColumn();

	ImGui::Text("%p", lpGameObject);
	ImGui::Text("%X", lpGameObject->objectID);
	ImGui::Text("%X", lpGameObject->actNo);
	ImGui::Text("%d", currentFrame);
	ImGui::Text("%X", lpGameObject->facing);
	ImGui::Text("%X", lpGameObject->side);
	ImGui::Text("%p", lpGameObject->playerData);
	ImGui::Text("%i", lpGameObject->xPos);
	ImGui::Text("%i", lpGameObject->ypos);
	ImGui::Text("%i", lpGameObject->xvel);
	ImGui::Text("%i", lpGameObject->yvel);
	ImGui::Text("%X", lastAction);
	ImGui::Text("%d", previousFrame);
	ImGui::Text("%d", (currentFrame - previousFrame));


	ImGui::End();
}

void DrawFrameAdvantageWindow( GameState* lpGameState, bool* pOpen) {
	GameObjectData* lpGameObject[2] = { lpGameState->arrCharacters[0], lpGameState->arrCharacters[1] };

	ImGui::Begin(
		"Frame Advantage Display",
		pOpen,
		ImGuiWindowFlags_None
	);

	ImGui::Columns(2, NULL, false);
	static int lastAction[2] = { 0, 0 };
	static WORD currentFrame;
	static WORD previousFrame;


	if (lpGameObject[0]->actNo != lastAction[lpGameObject[0]->playerIndex]) {
		previousFrame = currentFrame;
		currentFrame = *lpGameState->cGameTick;

		lastAction[lpGameObject[0]->playerIndex] = lpGameObject[0]->actNo;
		lpGameObject[0]->dwGraphicalEffects = CE_FLAME;
	}

	ImGui::Text("Object address:");
	ImGui::Text("Object ID:");
	ImGui::Text("Action ID:");
	ImGui::Text("Action ID Entry Frame:");
	ImGui::Text("Object Facing:");
	ImGui::Text("Object Side:");
	ImGui::Text("Player data address:");
	ImGui::Text("X Position:");
	ImGui::Text("Y Position:");
	ImGui::Text("X Velocity:");
	ImGui::Text("Y Velocity:");
	ImGui::Text("Previous Action ID:");
	ImGui::Text("Previous Action ID Entry Frame:");
	ImGui::Text("Time in Previous Action ID:");
	ImGui::Text("Graphical Effects ID:");


	ImGui::NextColumn();

	ImGui::Text("%p", lpGameObject[0]);
	ImGui::Text("%X", lpGameObject[0]->objectID);
	ImGui::Text("%X", lpGameObject[0]->actNo);
	ImGui::Text("%d", currentFrame);
	ImGui::Text("%X", lpGameObject[0]->facing);
	ImGui::Text("%X", lpGameObject[0]->side);
	ImGui::Text("%p", lpGameObject[0]->playerData);
	ImGui::Text("%i", lpGameObject[0]->xPos);
	ImGui::Text("%i", lpGameObject[0]->ypos);
	ImGui::Text("%i", lpGameObject[0]->xvel);
	ImGui::Text("%i", lpGameObject[0]->yvel);
	ImGui::Text("%X", lastAction);
	ImGui::Text("%d", previousFrame);
	ImGui::Text("%d", (currentFrame - previousFrame));
	ImGui::Text("%p", lpGameObject[0]->dwGraphicalEffects);

	ImGui::NextColumn();

	ImGui::Text("Object address:");
	ImGui::Text("Object ID:");
	ImGui::Text("Action ID:");
	ImGui::Text("Action ID Entry Frame:");
	ImGui::Text("Object Facing:");
	ImGui::Text("Object Side:");
	ImGui::Text("Player data address:");
	ImGui::Text("X Position:");
	ImGui::Text("Y Position:");
	ImGui::Text("X Velocity:");
	ImGui::Text("Y Velocity:");
	ImGui::Text("Previous Action ID:");
	ImGui::Text("Previous Action ID Entry Frame:");
	ImGui::Text("Time in Previous Action ID:");
	ImGui::Text("Graphical Effects ID:");


	ImGui::NextColumn();

	ImGui::Text("%p", lpGameObject[1]);
	ImGui::Text("%X", lpGameObject[1]->objectID);
	ImGui::Text("%X", lpGameObject[1]->actNo);
	ImGui::Text("%d", currentFrame);
	ImGui::Text("%X", lpGameObject[1]->facing);
	ImGui::Text("%X", lpGameObject[1]->side);
	ImGui::Text("%p", lpGameObject[1]->playerData);
	ImGui::Text("%i", lpGameObject[1]->xPos);
	ImGui::Text("%i", lpGameObject[1]->ypos);
	ImGui::Text("%i", lpGameObject[1]->xvel);
	ImGui::Text("%i", lpGameObject[1]->yvel);
	ImGui::Text("%X", lastAction);
	ImGui::Text("%d", previousFrame);
	ImGui::Text("%d", (currentFrame - previousFrame));
	ImGui::Text("%p", lpGameObject[1]->dwGraphicalEffects);

	ImGui::End();
}



void DrawInputDisplay(TCHAR* windowName, PlayerData* lpPlayerData, GameState* lpGameState, bool* pOpen) {
	//ImGui::Begin(
	//	windowName,
	//	pOpen,
	//	ImGuiWindowFlags_None
	//);
	//ImDrawList* draw_list = ImGui::GetWindowDrawList();


	//ImGui::Text("Primitives");
	//static float sz = 36.0f;
	//static float thickness = 4.0f;
	//static ImVec4 col = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
	//ImGui::DragFloat("Size", &sz, 0.2f, 2.0f, 72.0f, "%.0f");
	//ImGui::DragFloat("Thickness", &thickness, 0.05f, 1.0f, 8.0f, "%.02f");
	//ImGui::ColorEdit4("Color", &col.x);
	//{
	//	const ImVec2 p = ImGui::GetCursorScreenPos();
	//	const ImU32 col32 = ImColor(col);
	//	float x = p.x + 4.0f, y = p.y + 4.0f, spacing = 8.0f;
	//	for (int n = 0; n < 2; n++)
	//	{
	//		// First line uses a thickness of 1.0, second line uses the configurable thickness
	//		float th = (n == 0) ? 1.0f : thickness;
	//		draw_list->AddCircle(ImVec2(x + sz * 0.5f, y + sz * 0.5f), sz * 0.5f, col32, 6, th); x += sz + spacing;     // Hexagon
	//		draw_list->AddCircle(ImVec2(x + sz * 0.5f, y + sz * 0.5f), sz * 0.5f, col32, 20, th); x += sz + spacing;    // Circle
	//		draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col32, 0.0f, ImDrawCornerFlags_All, th); x += sz + spacing;
	//		draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col32, 10.0f, ImDrawCornerFlags_All, th); x += sz + spacing;
	//		draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col32, 10.0f, ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotRight, th); x += sz + spacing;
	//		draw_list->AddTriangle(ImVec2(x + sz * 0.5f, y), ImVec2(x + sz, y + sz - 0.5f), ImVec2(x, y + sz - 0.5f), col32, th); x += sz + spacing;
	//		draw_list->AddLine(ImVec2(x, y), ImVec2(x + sz, y), col32, th); x += sz + spacing;               // Horizontal line (note: drawing a filled rectangle will be faster!)
	//		draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + sz), col32, th); x += spacing;                  // Vertical line (note: drawing a filled rectangle will be faster!)
	//		draw_list->AddLine(ImVec2(x, y), ImVec2(x + sz, y + sz), col32, th); x += sz + spacing;               // Diagonal line
	//		draw_list->AddBezierCurve(ImVec2(x, y), ImVec2(x + sz * 1.3f, y + sz * 0.3f), ImVec2(x + sz - sz * 1.3f, y + sz - sz * 0.3f), ImVec2(x + sz, y + sz), col32, th);
	//		x = p.x + 4;
	//		y += sz + spacing;
	//	}
	//	draw_list->AddCircleFilled(ImVec2(x + sz * 0.5f, y + sz * 0.5f), sz * 0.5f, col32, 6); x += sz + spacing;       // Hexagon
	//	draw_list->AddCircleFilled(ImVec2(x + sz * 0.5f, y + sz * 0.5f), sz * 0.5f, col32, 32); x += sz + spacing;      // Circle
	//	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), col32); x += sz + spacing;
	//	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), col32, 10.0f); x += sz + spacing;
	//	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), col32, 10.0f, ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotRight); x += sz + spacing;
	//	draw_list->AddTriangleFilled(ImVec2(x + sz * 0.5f, y), ImVec2(x + sz, y + sz - 0.5f), ImVec2(x, y + sz - 0.5f), col32); x += sz + spacing;
	//	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + thickness), col32); x += sz + spacing;          // Horizontal line (faster than AddLine, but only handle integer thickness)
	//	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + thickness, y + sz), col32); x += spacing + spacing;     // Vertical line (faster than AddLine, but only handle integer thickness)
	//	draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + 1, y + 1), col32);          x += sz;                  // Pixel (faster than AddLine)
	//	draw_list->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x + sz, y + sz), IM_COL32(0, 0, 0, 255), IM_COL32(255, 0, 0, 255), IM_COL32(255, 255, 0, 255), IM_COL32(0, 255, 0, 255));
	//	ImGui::Dummy(ImVec2((sz + spacing) * 8, (sz + spacing) * 3));
	//}
	//ImGui::Separator();

	ImGui::Begin("INPUT LOG", pOpen);


	


	static unsigned int lastAction[2] = { 0, 0 };
	static WORD currentFrame[2] = { *lpGameState->cGameTick, 0 };
	static ExampleAppLog actionLogs[2];
	
	

	ImGui::Text("Input Bits: %d", lastAction[0]); ImGui::NextColumn(); ImGui::Text("Entry Frame: %d", currentFrame[0]); ImGui::NextColumn(); ImGui::Text("Total Input Frames: %d", (*lpGameState->cGameTick - currentFrame[0]));

	if (normalizeInput(lpGameState, lpGameState->nP1CurrentFrameInputs) != lastAction[0]) {
		lastAction[1] = lastAction[0];
		currentFrame[1] = currentFrame[0];
		lastAction[0] = normalizeInput(lpGameState, lpGameState->nP1CurrentFrameInputs);
		currentFrame[0] = *lpGameState->cGameTick;

		

		
		
		if (lastAction[1] == 0)
		{
			actionLogs[0].AddLog(" Neutral ");

		}
		else
		{
			if (lastAction[1] & Up)
			{
				actionLogs[0].AddLog(" Up ");

			}
			if (lastAction[1] & Down)
			{
				actionLogs[0].AddLog(" Down ");

			}
			if (lastAction[1] & Left)
			{
				actionLogs[0].AddLog(" Left ");

			}
			if (lastAction[1] & Right)
			{
				actionLogs[0].AddLog(" Right ");

			}
		}
		if (lastAction[1] & Punch)
		{
			actionLogs[0].AddLog(" Punch ");

		}
		if (lastAction[1] & Kick)
		{
			actionLogs[0].AddLog(" Kick ");

		}
		if (lastAction[1] & HSlash)
		{
			actionLogs[0].AddLog(" HSlash ");

		}
		if (lastAction[1] & Slash)
		{
			actionLogs[0].AddLog(" Slash ");

		}
		if (lastAction[1] & Dust)
		{
			actionLogs[0].AddLog(" Dust ");

		}
		actionLogs[0].AddLog("\n");

		actionLogs[0].AddLog("Input on frame: %d\n", currentFrame[1]);
		actionLogs[0].AddLog("Input for: %d frames\n", currentFrame[0] - currentFrame[1]);
		actionLogs[0].AddLog("Current Input Bits: %d\n", lastAction[1]);

		actionLogs[0].AddLog("\n\n");
	}

	actionLogs[0].Draw(
		"INPUT TEST",
		pOpen
	);

	ImGui::End();
}

void DrawPlayerStateWindow(TCHAR* windowName, PlayerData* lpPlayerData, bool* pOpen) {
	// Current faint gets stored as a short and divided by 100 before compared
	// to the character's maxFaint, which is stored as a single byte. Be careful
	// with integer truncation- comparing currentFaint to maxFaint*100 is not the
	// same as comparing currentFaint / 100 to maxFaint!
	int currentFaint = (int)(lpPlayerData->currentFaint / 100);
	int maxFaint = (int)lpPlayerData->maxFaint;

	ImGui::Begin(
		windowName,
		pOpen,
		ImGuiWindowFlags_None
	);

	ImGui::SliderScalar("Health", ImGuiDataType_U16, &lpPlayerData->currentHealth, &MIN_HEALTH, &MAX_HEALTH);
	ImGui::SliderScalar("Burst", ImGuiDataType_U16, &lpPlayerData->currentBurst, &MIN_BURST, &MAX_BURST);
	ImGui::SliderScalar("Tension", ImGuiDataType_U16, &lpPlayerData->currentTension, &MIN_TENSION, &MAX_TENSION);
	ImGui::SliderScalar("Guard Balance", ImGuiDataType_S16, &lpPlayerData->guardBalance, &MIN_GUARD_BALANCE, &MAX_GUARD_BALANCE);
	ImGui::SliderScalar("Current Stun", ImGuiDataType_S32, &currentFaint, &MIN_FAINT, &maxFaint);
	ImGui::SliderScalar("Negative Penalty", ImGuiDataType_S32, &lpPlayerData->negativePenaltyCounter, &MIN_NEGATIVE_PENALTY, &MAX_NEGATIVE_PENALTY);

	ImGui::Separator();

	ImGui::Columns(2);
	ImGui::Text("Address"); ImGui::NextColumn(); ImGui::Text("%p", lpPlayerData); ImGui::NextColumn();
	ImGui::Text("Airdashes remaining"); ImGui::NextColumn(); ImGui::Text("%d", lpPlayerData->nAirDashesRemaining); ImGui::NextColumn();
	ImGui::Text("Airjumps remaining"); ImGui::NextColumn(); ImGui::Text("%d", lpPlayerData->nAirJumpsRemaining); ImGui::NextColumn();
	ImGui::Text("Rakusyo bonus"); ImGui::NextColumn(); ImGui::Text("%d", lpPlayerData->receiveRakushoBonus); ImGui::NextColumn();
	ImGui::Text("Tension mode"); ImGui::NextColumn(); ImGui::Text("%d", lpPlayerData->tensionMode); ImGui::NextColumn();
	ImGui::Text("Projectile thrown"); ImGui::NextColumn(); ImGui::Text("%d", lpPlayerData->IsProjectileThrown); ImGui::NextColumn();
	ImGui::Text("Character meter 1"); ImGui::NextColumn(); ImGui::Text("%d", lpPlayerData->characterMeter1); ImGui::NextColumn();
	ImGui::Text("Character meter 2"); ImGui::NextColumn(); ImGui::Text("%d", lpPlayerData->characterMeter2); ImGui::NextColumn();
	ImGui::Text("Num times clean hit"); ImGui::NextColumn(); ImGui::Text("%d", lpPlayerData->numTimesCleanHit); ImGui::NextColumn();
	ImGui::Text("Max stun before faint"); ImGui::NextColumn(); ImGui::Text("%d", maxFaint); ImGui::NextColumn();
	ImGui::Columns(1);

	ImGui::End();
}

void DrawActionLogWindow(GameObjectData* lpGameObject, GameState* lpGameState, bool* pOpen) {
	static int lastAction[2] = { 0, 0 };
	static ExampleAppLog actionLogs[2];

	if (lpGameObject->actNo != lastAction[lpGameObject->playerIndex]) {
		WORD currentFrame = *lpGameState->cGameTick;
		
		actionLogs[lpGameObject->playerIndex].AddLog("Action ID: %04X\n", lpGameObject->actNo);
		actionLogs[lpGameObject->playerIndex].AddLog("State Changed on Frame: %d\n", currentFrame); 
		actionLogs[lpGameObject->playerIndex].AddLog("Current State Flag: %d\n\n", lpGameObject->stateFlags);
		actionLogs[lpGameObject->playerIndex].AddLog("Current Input Bits: %d\n\n", normalizeInput(lpGameState, lpGameState->nP1CurrentFrameInputs));
		if (lpGameObject->actNo == 0x003A)
		{
			actionLogs[lpGameObject->playerIndex].AddLog("They Crouch blocked maybe\n");
			
		}
		if (normalizeInput(lpGameState, lpGameState->nP1CurrentFrameInputs) & HSlash)
		{
			actionLogs[lpGameObject->playerIndex].AddLog("They Pressed HSlash\n");

		}
		lastAction[lpGameObject->playerIndex] = lpGameObject->actNo;
	}

	actionLogs[lpGameObject->playerIndex].Draw(
		lpGameObject->playerIndex == 0 ? "Player 1 Action Log" : "Player 2 Action Log", 
		pOpen
	);
}

void DrawSaveLoadStateWindow(GameState* lpGameState, bool* pOpen) {
	static SavedGameState savedState;
	static int nFramesToSkipRender = 0;
	static int nMinSkip = 0;
	static int nMaxSkip = 60;
	static int nFrameStep = 1;

	ImGui::Begin("Save/Load State", pOpen, ImGuiWindowFlags_None);
	if (nFramesToSkipRender < 0) {
		nFramesToSkipRender = 0;
	}
	else if (nFramesToSkipRender > 60) {
		nFramesToSkipRender = 60;
	}

	ImGui::InputInt("Num frames to skip", &nFramesToSkipRender);
	for (int p = 0; p < 2; p++) {
		const char* headerLabel = p == 0 ? "P1 inputs during skip" : "P2 inputs during skip";
		PlayerData* lpPlayerData = &lpGameState->arrPlayerData[p];
		if (ImGui::CollapsingHeader(headerLabel)) {
			// left
			static int selected = 0;
			ImGui::BeginChild("left pane", ImVec2(150, 0), true);
			for (int i = 0; i < nFramesToSkipRender; i++)
			{
				char label[128];
				sprintf(label, "Frame %d", i);
				if (ImGui::Selectable(label, selected == i))
					selected = i;
			}
			ImGui::EndChild();
			ImGui::SameLine();

			// right
			ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
			ImGui::CheckboxFlags("Left", &lpGameState->arrInputsDuringFrameSkip[selected][p], Left); ImGui::NextColumn();
			ImGui::CheckboxFlags("Down", &lpGameState->arrInputsDuringFrameSkip[selected][p], Down); ImGui::NextColumn();
			ImGui::CheckboxFlags("Up", &lpGameState->arrInputsDuringFrameSkip[selected][p], Up); ImGui::NextColumn();
			ImGui::CheckboxFlags("Right", &lpGameState->arrInputsDuringFrameSkip[selected][p], Right); ImGui::NextColumn();
			ImGui::CheckboxFlags("P", &lpGameState->arrInputsDuringFrameSkip[selected][p], lpPlayerData->ctrlP); ImGui::NextColumn();
			ImGui::CheckboxFlags("K", &lpGameState->arrInputsDuringFrameSkip[selected][p], lpPlayerData->ctrlK); ImGui::NextColumn();
			ImGui::CheckboxFlags("S", &lpGameState->arrInputsDuringFrameSkip[selected][p], lpPlayerData->ctrlS); ImGui::NextColumn();
			ImGui::CheckboxFlags("H", &lpGameState->arrInputsDuringFrameSkip[selected][p], lpPlayerData->ctrlH); ImGui::NextColumn();
			ImGui::CheckboxFlags("D", &lpGameState->arrInputsDuringFrameSkip[selected][p], lpPlayerData->ctrlD); ImGui::NextColumn();
			ImGui::CheckboxFlags("Respect", &lpGameState->arrInputsDuringFrameSkip[selected][p], lpPlayerData->ctrlRespect); ImGui::NextColumn();
			ImGui::EndChild();
		}
	}

	if (ImGui::Button("Save")) {

		SaveGameState(lpGameState, &savedState);
	}

	if (ImGui::Button("Load")) {
		// This should probably trigger a load on the _next_ frame, or we're
		// likely to do something bad to graphics memory.
		lpGameState->nFramesSkipped = 0;
		lpGameState->nFramesToSkipRender = nFramesToSkipRender;
		LoadGameState(lpGameState, &savedState);
	}

	ImGui::End();
}

void DrawHelpWindow(bool* pOpen) {
	ImGui::Begin(
		"ImGui Help", 
		pOpen, 
		ImGuiWindowFlags_None
	);

	ImGui::ShowUserGuide();

	ImGui::End();
}

void DrawMarkUnlockWindow(GameMethods* lpGameMethods, bool* pOpen) {
	ImGui::Begin(
		"Mark Unlocks",
		pOpen,
		ImGuiWindowFlags_None
	);

	if (ImGui::Button("Mark all unlocks on")) {
		lpGameMethods->MarkAllUnlocksOn();
	}

	if (ImGui::Button("Mark all unlocks off")) {
		lpGameMethods->MarkAllUnlocksOff();
	}

	ImGui::End();
}

void DrawSaveLoadReplayWindow(GameState* lpGameState, bool* pOpen) {
	static ExampleAppLog saveloadreplay;
	static char* cLogpath = "./rec.acrec";
	static int prevRecStatus = 0;
	static int prevRecPlayer = 0;


	ImGui::Begin(
		"+R Recording Manager", 
		pOpen, 
		ImGuiWindowFlags_None
	);

	if (ImGui::Button("Read Config")) {
		saveloadreplay.Clear();
		saveloadreplay.AutoScroll = false;
		for (int p = 0; p < 2; p++) {
			saveloadreplay.AddLog("---P%X Button Config ---\n", p + 1);
			saveloadreplay.AddLog("P%i Punch = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlP);
			saveloadreplay.AddLog("P%i Kick = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlK);
			saveloadreplay.AddLog("P%i Slash = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlS);
			saveloadreplay.AddLog("P%i H-Slash = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlH);
			saveloadreplay.AddLog("P%i Dust = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlD);
			saveloadreplay.AddLog("P%i Respect = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlRespect);
			saveloadreplay.AddLog("P%i Reset = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlReset);
			saveloadreplay.AddLog("P%i Pause = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlPause);
			saveloadreplay.AddLog("P%i Rec Player = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlRecPlayer);
			saveloadreplay.AddLog("P%i Rec Enemy = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlRecEnemy);
			saveloadreplay.AddLog("P%i Play Memory = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlPlayMemory);
			saveloadreplay.AddLog("P%i Switch = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlSwitch);
			saveloadreplay.AddLog("P%i Enemy Walk = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlEnemyWalk);
			saveloadreplay.AddLog("P%i Enemy Jump = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlEnemyJump);
			saveloadreplay.AddLog("P%i P K = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlPKMacro);
			saveloadreplay.AddLog("P%i P D = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlPDMacro);
			saveloadreplay.AddLog("P%i P K S = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlPKSMacro);
			saveloadreplay.AddLog("P%i P K S H = 0x%X\n", p+1, lpGameState->arrPlayerData[p].ctrlPKSHMacro);
			saveloadreplay.AddLog("\n");
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Save")) {

		SaveRecording(cLogpath, lpGameState);
		saveloadreplay.AddLog("Save - Player %X recording saved to: %s\n", (lpGameState->recTarget->nPlayer + 1), cLogpath);

	}

	ImGui::SameLine();

	if (ImGui::Button("Load")) {

		LoadRecording(cLogpath, lpGameState);
		saveloadreplay.AddLog("Load - P%X recording loaded from: %s\n", (lpGameState->recTarget->nPlayer + 1), cLogpath);

	}

	ImGui::SameLine();

	if (ImGui::Button("Current Slot")) {
	
		saveloadreplay.AddLog("Slot - P%X recording currently loaded\n", (lpGameState->recTarget->nPlayer + 1));

	}

	//logs current Training Mode recording status
	if (*lpGameState->recStatus != prevRecStatus) {
		if (*lpGameState->recStatus == 0) {
			if (prevRecStatus == 1) {

				saveloadreplay.AddLog("State - Player Switch Cancelled \n");
				prevRecStatus = *lpGameState->recStatus;
			}
			if (prevRecStatus == 2) {

				saveloadreplay.AddLog("State - P%X Rec Standby Cancelled \n", (lpGameState->recTarget->nPlayer + 1));
				prevRecStatus = *lpGameState->recStatus;

			}
			if (prevRecStatus == 3) {

				saveloadreplay.AddLog("State - P%X Recording Stopped \n", (lpGameState->recTarget->nPlayer + 1));
				prevRecStatus = *lpGameState->recStatus;

			}
			if (prevRecStatus == 4) {

				saveloadreplay.AddLog("State - P%X Recording Playback Stopped \n", (lpGameState->recTarget->nPlayer + 1));
				prevRecStatus = *lpGameState->recStatus;

			}
		}
		if (*lpGameState->recStatus == 1) {

			saveloadreplay.AddLog("State - Switched to other Character\n");
			prevRecStatus = *lpGameState->recStatus;
		}
		if (*lpGameState->recStatus == 2) {


				saveloadreplay.AddLog("State - P%X Rec Standby\n", (lpGameState->recTarget->nPlayer + 1));
				prevRecStatus = *lpGameState->recStatus;
				prevRecPlayer = (lpGameState->recTarget->nPlayer + 1);

		}
		if (*lpGameState->recStatus == 3) {

				saveloadreplay.AddLog("State - P%X Rec Started\n", (lpGameState->recTarget->nPlayer + 1));
				prevRecStatus = *lpGameState->recStatus;

		}
		if (*lpGameState->recStatus == 4) {

				saveloadreplay.AddLog("State - P%X Rec Playback Started\n", (lpGameState->recTarget->nPlayer + 1));
				prevRecStatus = *lpGameState->recStatus;

		}
	}
	else {
		if(*lpGameState->recStatus != 0){
			if ((lpGameState->recTarget->nPlayer + 1) != prevRecPlayer) {

				saveloadreplay.AddLog("State - P%X Rec Standby\n", (lpGameState->recTarget->nPlayer + 1));
				prevRecStatus = *lpGameState->recStatus;
				prevRecPlayer = (lpGameState->recTarget->nPlayer + 1);

			}
		}
	}

	ImGui::Separator();

	saveloadreplay.Draw("+R Recording Manager", pOpen);

	ImGui::End();

}

void InitializeOverlay(GameState* lpGameState) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.Alpha = DEFAULT_ALPHA;
	ImGui_ImplWin32_Init(*lpGameState->hWnd);
	ImGui_ImplDX9_Init(*lpGameState->d3dDevice);
}

void DrawGameMenu() {
	if (ImGui::BeginMenu("Game")) {
		if (ImGui::BeginMenu("Netplay")) {
			ImGui::MenuItem("GGPO: Host...", NULL, &show_ggpo_host, (show_ggpo_join == false));
			ImGui::MenuItem("GGPO: Join...", NULL, &show_ggpo_join, (show_ggpo_host == false));
			ImGui::EndMenu();
		}
		ImGui::MenuItem("Character select", NULL, &show_character_select);
		ImGui::EndMenu();
	}
};
void DrawToolsMenu(bool* pHitbox) {
	if (ImGui::BeginMenu("Tools")) {
		ImGui::MenuItem(*pHitbox ? "Hide Hitboxes" : "Show Hitboxes", NULL, pHitbox);
		ImGui::MenuItem("Character Data", NULL, &show_character_data);
		if (ImGui::BeginMenu("Player State", (show_ggpo_join == show_ggpo_host))) {
			ImGui::MenuItem("Player 1 State", NULL, &show_p1_state, (show_ggpo_join == show_ggpo_host));
			ImGui::MenuItem("Player 2 State", NULL, &show_p2_state, (show_ggpo_join == show_ggpo_host));
			ImGui::EndMenu();
		}
		ImGui::MenuItem("Save/Load Replay", NULL, &show_save_load_replay);
		ImGui::MenuItem("Mark unlocks on/off", NULL, &show_mark_unlocks);
		ImGui::EndMenu();
	}
};
void DrawDebugMenu(GameState* lpGameState) {
	if (ImGui::BeginMenu("Debug")) {
		ImGui::MenuItem("Global State", NULL, &show_global_state);
		if (ImGui::BeginMenu("Object State")) {
			ImGui::MenuItem("Player 1 Object State", NULL, &show_p1_object_state, *lpGameState->arrCharacters != 0);
			ImGui::MenuItem("Player 1 Object Action Log", NULL, &show_p1_log, *lpGameState->arrCharacters != 0);
			ImGui::MenuItem("Player 2 Object State", NULL, &show_p2_object_state, *lpGameState->arrCharacters != 0);
			ImGui::MenuItem("Player 2 Object Action Log", NULL, &show_p2_log, *lpGameState->arrCharacters != 0);
			ImGui::MenuItem("Frame Advantage Display", NULL, &show_p_frame_advantage, *lpGameState->arrCharacters != 0);
			ImGui::MenuItem("Player 1 Input Display", NULL, &show_p1_input_display, *lpGameState->arrCharacters != 0);
			ImGui::MenuItem("Player 2 Input Display", NULL, &show_p2_input_display, *lpGameState->arrCharacters != 0);
			ImGui::EndMenu();
		}
		ImGui::MenuItem("Save/Load State", NULL, &show_saveload);
		ImGui::EndMenu();
	}
};

void DrawOverlay(GameMethods* lpGameMethods, GameState* lpGameState) {
	if (!load_config) {
		ApplyConfiguration(lpGameState);
		load_config = true;
	}

	bool show_hitboxes = *lpGameState->bHitboxDisplayEnabled != 0;

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (ImGui::IsMousePosValid() && ImGui::GetIO().MousePos.y < 200) {
		if (ImGui::BeginMainMenuBar()) {
			DrawGameMenu();
			DrawToolsMenu(&show_hitboxes);
			DrawDebugMenu(lpGameState);
			ImGui::MenuItem(show_help ? "Hide Help" : "Show Help",NULL,&show_help);

			ImGui::EndMainMenuBar();
		}
	}
	if (show_character_select) {
		DrawEnterVersus2PWindow(lpGameState, &show_character_select);
	}
	if (show_global_state) {
		DrawGlobalStateWindow(lpGameState, &show_global_state);
	}
	if (show_p1_state) {
		DrawPlayerStateWindow("Player 1 State", &lpGameState->arrPlayerData[0], &show_p1_state);
	}
	if (show_p1_object_state) {
		DrawObjectStateWindow(&(*lpGameState->arrCharacters)[0], lpGameState, &show_p1_object_state);
	}
	if (show_p1_log) {
		DrawActionLogWindow(&(*lpGameState->arrCharacters)[0], lpGameState , &show_p1_log);
	}
	if (show_p2_state) {
		DrawPlayerStateWindow("Player 2 State", &lpGameState->arrPlayerData[1], &show_p2_state);
	}
	if (show_p2_object_state) {
		DrawObjectStateWindow(&(*lpGameState->arrCharacters)[1], lpGameState, &show_p2_object_state);
	}
	if (show_p2_log) {
		DrawActionLogWindow(&(*lpGameState->arrCharacters)[1], lpGameState, &show_p2_log);
	}
	if (show_p_frame_advantage) {
		DrawFrameAdvantageWindow( lpGameState, &show_p_frame_advantage);
	}
	if (show_help) {
		DrawHelpWindow(&show_help);
	}
	if (show_save_load_replay) {
		DrawSaveLoadReplayWindow(lpGameState, &show_save_load_replay);
	}
	if (show_mark_unlocks) {
		DrawMarkUnlockWindow(lpGameMethods, &show_mark_unlocks);
	}
	if (show_character_data) {
		DrawCharacterDataWindow(lpGameState, &show_character_data);
	}
	if (show_hitboxes) {
		if (*lpGameState->bHitboxDisplayEnabled == 0) {
			EnableHitboxes(lpGameState);
		}
	}
	else {
		if (*lpGameState->bHitboxDisplayEnabled != 0) {
			DisableHitboxes(lpGameState);
		}
	}
	if (show_saveload) {
		DrawSaveLoadStateWindow(lpGameState, &show_saveload);
	}
	if (show_ggpo_host) {
		DrawGGPOHostWindow(lpGameState, &show_ggpo_host);
	}
	if (show_ggpo_join) {
		DrawGGPOJoinWindow(lpGameState, &show_ggpo_join);
	}
	if (show_p1_input_display) {
		DrawInputDisplay("Player 1 Input Display", &lpGameState->arrPlayerData[0], lpGameState, &show_p1_input_display);
	}
	if (show_p2_input_display) {
		DrawInputDisplay("Player 2 Input Display", &lpGameState->arrPlayerData[1], lpGameState, &show_p2_input_display);
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void FreeOverlay() {
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void InvalidateImGuiDeviceObjects() {
	ImGui_ImplDX9_InvalidateDeviceObjects();
}

void CreateImGuiDeviceObjects() {
	ImGui_ImplDX9_CreateDeviceObjects();
}

LRESULT WINAPI OverlayWindowFunc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
