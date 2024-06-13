#include <list>
#include <string>
#include "../offsets.h"
#include <string.h>
#include "../../Vendor/Protection/xorst.h"
#include "../functions.h"
#include <iostream>
#include "../../game/aimbot/aimbot.h"
#include "../../game/aimbot/controller.h"
#include <vector>
#include <immintrin.h>
using namespace uee;

void RadarRange(float* x, float* y, float range)
{
	if (fabs((*x)) > range || fabs((*y)) > range)
	{
		if ((*y) > (*x))
		{
			if ((*y) > -(*x))
			{
				(*x) = range * (*x) / (*y);
				(*y) = range;
			}
			else
			{
				(*y) = -range * (*y) / (*x);
				(*x) = -range;
			}
		}
		else
		{
			if ((*y) > -(*x))
			{
				(*y) = range * (*y) / (*x);
				(*x) = range;
			}
			else
			{
				(*x) = -range * (*x) / (*y);
				(*y) = -range;
			}
		}

	}
}

void CalcRadarPoint(fvector vOrigin, int& screenx, int& screeny)
{
	fvector vAngle = camera_postion.rotation;
	auto fYaw = vAngle.y * M_PI / 180.0f;
	float dx = vOrigin.x - camera_postion.location.x;
	float dy = vOrigin.y - camera_postion.location.y;

	//x' = x * cos(p) - y * sin(p)
	//y' = y * sin(p) - y * -cos(p)
	float fsin_yaw = sinf(fYaw);
	float fminus_cos_yaw = -cosf(fYaw);

	float x = dy * fminus_cos_yaw + dx * fsin_yaw;
	x = -x;
	float y = dx * fminus_cos_yaw - dy * fsin_yaw;

	float range = (float)radar::radar_range * 1000.f;

	RadarRange(&x, &y, range);

	ImVec2 DrawPos = ImVec2(radar::radar_pos_x, radar::radar_pos_y);
	ImVec2 DrawSize = ImVec2(radar::radar_size, radar::radar_size);

	int rad_x = (int)DrawPos.x;
	int rad_y = (int)DrawPos.y;

	float r_siz_x = DrawSize.x;
	float r_siz_y = DrawSize.y;

	int x_max = (int)r_siz_x + rad_x - 5;
	int y_max = (int)r_siz_y + rad_y - 5;

	screenx = rad_x + ((int)r_siz_x / 2 + int(x / range * r_siz_x));
	screeny = rad_y + ((int)r_siz_y / 2 + int(y / range * r_siz_y));

	if (screenx > x_max)
		screenx = x_max;

	if (screenx < rad_x)
		screenx = rad_x;

	if (screeny > y_max)
		screeny = y_max;

	if (screeny < rad_y)
		screeny = rad_y;
}

void render_radar_main() {
	ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(radar::radar_pos_x, radar::radar_pos_y), ImVec2(radar::radar_pos_x + radar::radar_size, radar::radar_pos_y + radar::radar_size), ImGui::GetColorU32({ 0.2f, 0.2f, 0.f, 1.f }), 0.f, 0);


	ImVec2 center = ImVec2(radar::radar_pos_x + (radar::radar_size / 2), radar::radar_pos_y + (radar::radar_size / 2));
	float orangeCircleRadius = 2.3f;
	ImGui::GetBackgroundDrawList()->AddCircleFilled(center, orangeCircleRadius, ImGui::GetColorU32({ 1.f, 1.f, 1.f, 1.f }), 12);
}

void add_to_radar(fvector WorldLocation, float fDistance, bool visible) {
	int ScreenX, ScreenY = 0;
	CalcRadarPoint(WorldLocation, ScreenX, ScreenY);

	if (!visible)
	{
		ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(ScreenX, ScreenY), 6.f, ImGui::GetColorU32({ 1.f, 0.f, 0.f, 1.f }), 12);
	}
	else
	{
		ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(ScreenX, ScreenY), 6.f, ImGui::GetColorU32({ 0.f, 1.f, 0.f, 1.f }), 12);
	}

	ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(ScreenX, ScreenY), 6.f, ImGui::GetColorU32({ 0.f, 0.f, 0.f, 1.f }), 12, 1.f);
}

namespace g_loop {
	class g_fn {
	public:

		fvector2d head;
		fvector2d root;

		auto cache_entities() -> void
		{
			temporary_entity_list.clear();

			uintptr_t uworld = io::Read<uintptr_t>(base_address + addresses::UWORLD);
			uintptr_t game_instance = io::Read<uintptr_t>(g_ptr->uworld + offset::GameInstance);
			uintptr_t localplayer = io::Read<uintptr_t>(io::Read<uintptr_t>(g_ptr->game_instance + offset::LocalPlayer));
			uintptr_t player_controller = io::Read<uintptr_t>(g_ptr->local_player + offset::PlayerController);
			uintptr_t acknowledged_pawn = io::Read<uintptr_t>(g_ptr->player_controller + offset::LocalPawn);
			uintptr_t skeletal_mesh = io::Read<uintptr_t>(g_ptr->acknowledged_pawn + offset::SkeletonMesh);
			uintptr_t player_state = io::Read<uintptr_t>(g_ptr->acknowledged_pawn + offset::PlayerState);
			uintptr_t root_component = io::Read<uintptr_t>(g_ptr->acknowledged_pawn + offset::RootComp);
			fvector relative_location = io::Read<fvector>(g_ptr->root_component + offset::RelativeLocation);
			int team_index = io::Read<int>(g_ptr->player_state + offset::TeamIndex);
			uintptr_t game_state = io::Read<uintptr_t>(g_ptr->uworld + offset::GameState);
			uintptr_t player_array = io::Read<uintptr_t>(g_ptr->game_state + offset::PlayerArray);
			int player_array_size = io::Read<int>(g_ptr->game_state + (offset::PlayerArray + sizeof(uintptr_t)));

			g_ptr->uworld = uworld;
			g_ptr->game_instance = game_instance;
			g_ptr->local_player = localplayer;
			g_ptr->player_controller = player_controller;
			g_ptr->acknowledged_pawn = acknowledged_pawn;
			g_ptr->skeletal_mesh = skeletal_mesh;
			g_ptr->player_state = player_state;
			g_ptr->root_component = root_component;
			g_ptr->relative_location = relative_location;
			g_ptr->team_index = team_index;
			g_ptr->game_state = game_state;
			g_ptr->player_array = player_array;
			g_ptr->player_array_size = player_array_size;

			for (int i = 0; i < g_ptr->player_array_size; ++i)
			{
				auto player_state = io::Read<uintptr_t>(g_ptr->player_array + (i * sizeof(uintptr_t)));
				auto current_actor = io::Read<uintptr_t>(player_state + offset::PawnPrivate);

				if (current_actor == g_ptr->acknowledged_pawn) continue;

				auto skeletalmesh = io::Read<uintptr_t>(current_actor + offset::SkeletonMesh);
				if (!skeletalmesh) continue;

				auto base_bone = ue5->get_bone_3d(skeletalmesh, 110);
				if (base_bone.x == 0 && base_bone.y == 0 && base_bone.z == 0) continue;

				if (!ue5->in_screen(ue5->w2s(ue5->get_bone_3d(skeletalmesh, 110)))) continue;

				auto is_despawning = (io::Read<char>(current_actor + 0x758) >> 3); //bDying or bIsDying
				if (is_despawning) continue;

				entity cached_actors{};
				cached_actors.entity = current_actor;
				cached_actors.skeletal_mesh = io::Read<uintptr_t>(current_actor + offset::SkeletonMesh);
				cached_actors.relative_location = io::Read<fvector>(current_actor + offset::RelativeLocation);
				cached_actors.root_component = io::Read<uintptr_t>(current_actor + offset::RootComp);
				cached_actors.player_state = io::Read<uintptr_t>(current_actor + offset::PlayerState);
				cached_actors.team_index = io::Read<int>(cached_actors.player_state + offset::TeamIndex);

				temporary_entity_list.push_back(cached_actors);
			}

			entity_list.clear();
			entity_list = temporary_entity_list;
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		auto gun_loop() -> void
		{
			if (g_ptr->acknowledged_pawn)
			{
				if (aimbot::weaponCFG)
				{
					uint64_t player_weapon = io::Read<uint64_t>(g_ptr->acknowledged_pawn + WeaponOffsets::CurrentWeapon); // 	struct AFortWeapon* CurrentWeapon;

					if (is_valid(player_weapon))
					{
						uint64_t weapon_data = io::Read<uint64_t>(player_weapon + WeaponOffsets::WeaponData);	//struct UFortWeaponItemDefinition* WeaponData;

						if (is_valid(weapon_data)) {

							uint64_t ftext_ptr = io::Read<uint64_t>(weapon_data + WeaponOffsets::ItemName);

							if (is_valid(ftext_ptr)) {
								uint64_t ftext_data = io::Read<uint64_t>(ftext_ptr + 0x28);
								int ftext_length = io::Read<int>(ftext_ptr + 0x30);
								if (ftext_length > 0 && ftext_length < 50) {
									wchar_t* ftext_buf = new wchar_t[ftext_length];
									io::readm((PVOID)ftext_data, ftext_buf, ftext_length * sizeof(wchar_t));

									wchar_t* WeaponName = ftext_buf;

									delete[] ftext_buf;

									if (wcsstr(WeaponName, E(L"Assault Rifle")) || wcsstr(WeaponName, E(L"Havoc Suppressed Assault Rifle")) || wcsstr(WeaponName, E(L"Red-Eye Assault Rifle"))
										|| wcsstr(WeaponName, E(L"Suppressed Assault Rifle")) || wcsstr(WeaponName, E(L"Striker Burst Rifle")) || wcsstr(WeaponName, E(L"Burst Assault Rifle"))
										|| wcsstr(WeaponName, E(L"Ranger Assault Rifle")) || wcsstr(WeaponName, E(L"Flapjack Rifle")) || wcsstr(WeaponName, E(L"Heavy Assault Rifle"))
										|| wcsstr(WeaponName, E(L"MK-Seven Assault Rifle")) || wcsstr(WeaponName, E(L"MK-Alpha Assault Rifle")) || wcsstr(WeaponName, E(L"Combat Assault Rifle"))
										|| wcsstr(WeaponName, E(L"Nemesis AR")) || wcsstr(WeaponName, E(L"Ambush Striker AR")) || wcsstr(WeaponName, E(L"Striker AR"))
										|| wcsstr(WeaponName, E(L"Tactical Assault Rifle")) || wcsstr(WeaponName, E(L"Hammer Assault Rifle")) || wcsstr(WeaponName, E(L"Sideways Rifle")) || wcsstr(WeaponName, E(L"Makeshift Rifle")) || wcsstr(WeaponName, E(L"Drum Gun")) || wcsstr(WeaponName, E(L"Nemisis Assault Rifle"))
										|| wcsstr(WeaponName, E(L"Warforged Assault Rifle"))) {
										HeldWeaponType = EFortWeaponType::Rifle;
										aimbot::enable = rifle::aimbotEnable;
										aimbot::fovsize = rifle::fov;
										aimbot::smoothsize = rifle::smoothness;
										aimbot::prediction = rifle::prediction;
										if (misc::debug_weapons)
										{
											std::cout << E("RIFLE HELD") << std::endl;
										}
									}
									if (wcsstr(WeaponName, E(L"Shotgun")) || wcsstr(WeaponName, E(L"Frenzy Auto Shotgun")) || wcsstr(WeaponName, E(L"Iron Warrior Hammer Pump Shogtun"))) {
										HeldWeaponType = EFortWeaponType::Shotgun;
										aimbot::enable = shotgun::aimbotEnable;
										aimbot::fovsize = shotgun::fov;
										aimbot::smoothsize = shotgun::smoothness;
										aimbot::prediction = shotgun::prediction;
										if (misc::debug_weapons)
										{
											std::cout << E("SHOTGUN HELD") << std::endl;
										}
									}
									if (wcsstr(WeaponName, E(L"Smg")) || wcsstr(WeaponName, E(L"Ambush Hyper SMG")) || wcsstr(WeaponName, E(L"Submachine Gun")) || wcsstr(WeaponName, E(L"Combat Smg")) || wcsstr(WeaponName, E(L"Pistol")) || wcsstr(WeaponName, E(L"Machine Smg"))
										|| wcsstr(WeaponName, E(L"Scoped Burst SMG")) || wcsstr(WeaponName, E(L"Hyper SMG")) || wcsstr(WeaponName, E(L"Thunder Burst SMG")), wcsstr(WeaponName, E(L"Ranger Pistol"))) {
										HeldWeaponType = EFortWeaponType::Smg;
										aimbot::enable = smg::aimbotEnable;
										aimbot::fovsize = smg::fov;
										aimbot::smoothsize = smg::smoothness;
										aimbot::prediction = smg::prediction;
										if (misc::debug_weapons)
										{
											std::cout << E("SMG HELD") << std::endl;
										}
									}
									if (wcsstr(WeaponName, E(L"Hunting Rifle")) || wcsstr(WeaponName, E(L"Heavy Sniper Rifle")) || wcsstr(WeaponName, E(L"Suppressed Sniper Rifle"))
										|| wcsstr(WeaponName, E(L"Storm Scout")) || wcsstr(WeaponName, E(L"Bolt-Action Sniper Rifle")) || wcsstr(WeaponName, E(L"Automatic Sniper Rifle"))
										|| wcsstr(WeaponName, E(L"DMR")) || wcsstr(WeaponName, E(L"Thermal DMR")) || wcsstr(WeaponName, E(L"Hunter Bolt-Action Sniper"))
										|| wcsstr(WeaponName, E(L"Reaper Sniper Rifle")) || wcsstr(WeaponName, E(L"Semi-Auto Sniper Rifle"))
										|| wcsstr(WeaponName, E(L"Crossbow")) || wcsstr(WeaponName, E(L"Mechanical Bow")) || wcsstr(WeaponName, E(L"Hand Cannon"))) {
										HeldWeaponType = EFortWeaponType::Sniper;
										aimbot::enable = sniper::aimbotEnable;
										aimbot::fovsize = sniper::fov;
										aimbot::smoothsize = sniper::smoothness;
										aimbot::prediction = sniper::prediction;
										if (misc::debug_weapons)
										{
											std::cout << E("SNIPER HELD") << std::endl;
										}
									}
								}
							}
						}
					}
				}
			}
		}

		fvector2d head_box;
		fvector2d root_box;
		fvector2d root_box1;

		auto actor_loop() -> void
		{
			ue5->get_camera();

			ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
			float target_dist = FLT_MAX;
			uintptr_t target_entity = 0;

			int dynamicfov = (ue5->dynamicfovval - 30) + aimbot::fovsize;

			if (aimbot::drawfov)
			{
				if (aimbot::fovoutline)
				{
					ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(screen_width / 2, screen_height / 2), dynamicfov + 2, ImColor(0, 0, 0), 200, 1);
					ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(screen_width / 2, screen_height / 2), dynamicfov - 2, ImColor(0, 0, 0), 200, 1);
				}

				ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(screen_width / 2, screen_height / 2), dynamicfov, ImColor(255, 255, 255), 200, 1);
			}

			if (radar::enable);
			{
				render_radar_main();
			}

			const float centerWidth = screen_width / 2;
			const float centerHeight = screen_height / 2;

			for (auto& cached : entity_list)
			{
				auto root_bone = ue5->get_bone_3d(cached.skeletal_mesh, 0);
				root = ue5->w2s(root_bone);
				root_box = ue5->w2s(fvector(root_bone.x, root_bone.y, root_bone.z + 30));
				root_box1 = ue5->w2s(fvector(root_bone.x, root_bone.y, root_bone.z - 15));

				auto neck_bone = ue5->get_bone_3d(cached.skeletal_mesh, 66);
				auto head_bone = ue5->get_bone_3d(cached.skeletal_mesh, 110);
				head = ue5->w2s(head_bone);
				head_box = ue5->w2s(fvector(head_bone.x, head_bone.y, head_bone.z + 15));
				auto head_location = ue5->get_bone_3d(cached.skeletal_mesh, 110);

				float box_height = abs(head.y - root_box1.y);
				float box_width = box_height * 0.40f;
				float distance = camera_postion.location.distance(root_bone) / 100;

				auto pawn_private1 = io::Read<uintptr_t>(PlayerState + 0x308);

				if (distance > visuals::settings::renderDistance && g_ptr->acknowledged_pawn) continue;

				static ImColor box_visible;
				static ImColor box_filled;

				if (ue5->is_visible(cached.skeletal_mesh))
				{
					box_visible = ImGui::GetColorU32({ colors::box_visible[0], colors::box_visible[1], colors::box_visible[2],  1.0f }); // Green for visible
					box_filled = ImGui::GetColorU32({ colors::box_filled_visible[0], colors::box_filled_visible[1], colors::box_filled_visible[2], 0.2f }); // Green for visible
				}
				else
				{
					box_visible = ImGui::GetColorU32({ colors::box_invisible[0], colors::box_invisible[1], colors::box_invisible[2],  1.0f }); // Red for non-visible
					box_filled = ImGui::GetColorU32({ colors::box_filled_invisible[0], colors::box_filled_invisible[1], colors::box_filled_invisible[2],  0.2f }); // Red for non-visible
				}

				if (visuals::settings::auto_thickness)
				{
					if (distance <= 2)
					{
						visuals::settings::skeleton_thickness = 7.5;
						visuals::settings::box_thick = 7.5;
					}
					else if (distance <= 5)
					{
						visuals::settings::skeleton_thickness = 6.5;
						visuals::settings::box_thick = 6.5;
					}
					else if (distance <= 8)
					{
						visuals::settings::skeleton_thickness = 5.8;
						visuals::settings::box_thick = 5.8;
					}
					else if (distance <= 10)
					{
						visuals::settings::skeleton_thickness = 5.2;
						visuals::settings::box_thick = 5.2;
					}
					else if (distance <= 15)
					{
						visuals::settings::skeleton_thickness = 4.3;
						visuals::settings::box_thick = 4.3;
					}
					else if (distance <= 20)
					{
						visuals::settings::skeleton_thickness = 3.2;
						visuals::settings::box_thick = 3.2;
					}
					else if (distance <= 30)
					{
						visuals::settings::skeleton_thickness = 2.5;
						visuals::settings::box_thick = 2.5;
					}
					else if (distance <= 50)
					{
						visuals::settings::skeleton_thickness = 2;
						visuals::settings::box_thick = 2;
					}
					else if (distance <= 90)
					{
						visuals::settings::skeleton_thickness = 1.3;
						visuals::settings::box_thick = 1.3;
					}

					else if (distance >= 90)
					{
						visuals::settings::skeleton_thickness = 1;
						visuals::settings::box_thick = 1;
					}
				}

				if (aimbot::enable)
				{
					auto dx = head.x - (screen_width / 2);
					auto dy = head.y - (screen_height / 2);
					auto dist = sqrtf(dx * dx + dy * dy);

					if (aimbot::vischeck)
					{
						if (ue5->is_visible(cached.skeletal_mesh))
						{
							if (dist < aimbot::fovsize && dist < target_dist)
							{
								target_dist = dist;
								target_entity = cached.entity;
							}
						}
					}
					else
					{
						if (dist < aimbot::fovsize && dist < target_dist)
						{
							target_dist = dist;
							target_entity = cached.entity;
						}
					}
				}

				if (misc::bIsStaggered)
				{
					bool bIsStaggered = io::Read<BYTE>(cached.entity + 0x759); //https://dumpspace.spuckwaffel.com/Games/?hash=6b77eceb&type=classes&idx=AFortPawn&member=bIsStaggered : 1
					if (!bIsStaggered)
						continue;
				}

				if (misc::bIsInvulnerable)
				{
					bool bIsInvulnerable = io::Read<BYTE>(cached.entity + 0x75a); //https://dumpspace.spuckwaffel.com/Games/?hash=6b77eceb&type=classes&idx=AFortPawn&member=bIsInvulnerable : 1
					if (!bIsInvulnerable)
						continue;
				}

				if (misc::bIgnoreDead)
				{
					bool bIsDying = (io::Read<BYTE>(cached.entity + 0x758) & 0x10); // https://dumpspace.spuckwaffel.com/Games/?hash=6b77eceb&type=classes&idx=AFortPawn&member=bIsDying
					bool bisDBNO = (io::Read<BYTE>(cached.entity + 0x93a) & 0x10); // https://dumpspace.spuckwaffel.com/Games/?hash=6b77eceb&type=classes&idx=AFortPawn&member=bIsDBNO 
					if (bisDBNO || bIsDying)
						continue;
				}

				if (visuals::box && visuals::enable)
				{
					const float halfWidth = box_width / 2.0f;
					const ImVec2 topLeft(head_box.x - halfWidth, head_box.y);
					const ImVec2 topRight(root.x + halfWidth, head_box.y);
					const ImVec2 bottomLeft(head_box.x - halfWidth, root.y);
					const ImVec2 bottomRight(root.x + halfWidth, root.y);

					const int boxThickness = visuals::settings::box_thick;

					if (visuals::filled && visuals::enable)
					{
						draw_list->AddRectFilled(topLeft, bottomRight, box_filled, 0.0f, ImDrawCornerFlags_All);
					}

					const ImColor outlineColor(0, 0, 0, 255);
					const int outlineThickness = visuals::settings::box_thick;

					if (visuals::settings::box_outline)
					{
						draw_list->AddRect(topLeft, bottomRight, ImColor(0, 0, 0), 0.0f, ImDrawCornerFlags_All, outlineThickness + 2.5);
					}

					draw_list->AddRect(topLeft, bottomRight, box_visible, 0.0f, ImDrawCornerFlags_All, outlineThickness);
				}

				if (visuals::username && visuals::enable)
				{
					std::string username = "[" + ue5->get_player_name(cached.player_state) + "]";
					ImVec2 username_text_size = ImGui::CalcTextSize(username.c_str());
					float username_text_x = head_box.x - (username_text_size.x / 2);
					float username_text_y = head_box.y - 20;

					DrawString(15, username_text_x, username_text_y, box_visible, false, true, username.c_str());
				}

				if (visuals::distance && visuals::enable)
				{
					std::string distance_str = "[" + std::to_string(static_cast<int>(distance)) + ".0M]";
					ImVec2 text_size = ImGui::CalcTextSize(distance_str.c_str());

					DrawString(15, root.x - (text_size.x / 2), root.y + 5, box_visible, false, true, distance_str.c_str());
				}

				if (visuals::skeleton && visuals::enable && aimbot::vischeck && distance <= 50)
				{
					fvector2d bonePositions[16];
					const int boneIndices[] = { 110, 3, 66, 9, 38, 10, 39, 11, 40, 78, 71, 79, 72, 75, 82, 67 };
					for (int i = 0; i < 16; ++i)
					{
						bonePositions[i] = ue5->w2s(ue5->get_bone_3d(cached.skeletal_mesh, boneIndices[i]));
					}
					std::pair<int, int> bonePairs[] =
					{
						{1, 2}, {3, 4}, {4, 3}, {5, 3}, {6, 4}, {5, 7}, {6, 8},
						{10, 1}, {9, 1}, {12, 10}, {11, 9}, {13, 12}, {14, 11}, {2, 15}, { 15, 0 }
					};
					ImGui::GetBackgroundDrawList()->PushClipRectFullScreen();
					for (const auto& pair : bonePairs)
					{
						ImVec2 start(bonePositions[pair.first].x, bonePositions[pair.first].y);
						ImVec2 end(bonePositions[pair.second].x, bonePositions[pair.second].y);

						// Adjust neck bone length
						if (pair.first == 110)
						{
							end.y += 20;
						}

						if (visuals::settings::skel_outline)
						{
							ImGui::GetBackgroundDrawList()->AddLine(start, end, ImColor(0, 0, 0, 255), visuals::settings::skeleton_thickness + 2);
						}

						ImGui::GetBackgroundDrawList()->AddLine(start, end, box_visible, visuals::settings::skeleton_thickness);
					}
					ImGui::GetBackgroundDrawList()->PopClipRect();
				}

				if (exploits::freeze_player)
				{
					if (GetAsyncKeyState_Spoofed(VK_F4))
					{
						io::Write<float>(target_entity + 0x68, 0);
						//CustomTimeDilation Offset

					}
					else {
						io::Write<float>(target_entity + 0x68, 1); //CustomTimeDilation Offset
					}
				}

				if (radar::enable)
				{
					add_to_radar(root_bone, 187, ue5->is_visible);
				}

				if (visuals::tracers)
				{
					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(screen_width / 2, 0), ImVec2(head_box.x, head_box.y), box_visible, visuals::settings::tracer_thickness);
				}
			}

			if (target_entity && aimbot::enable)
			{
				auto closest_mesh = io::Read<uint64_t>(target_entity + offset::SkeletonMesh);
				auto root = io::Read<uintptr_t>(target_entity + offset::RootComp);
				fvector Velocity = io::Read<fvector>(root + offset::Velocity);

				fvector hitbox;
				float projectileSpeed = 50000.f;//0;
				float projectileGravityScale = 3.5f;	//0;

				if (aimbot::Hitbox == 0)
					hitbox = ue5->get_bone_3d(closest_mesh, 110);
				else if (aimbot::Hitbox == 1)
					hitbox = ue5->get_bone_3d(closest_mesh, 66);
				else if (aimbot::Hitbox == 2)
					hitbox = ue5->get_bone_3d(closest_mesh, 7);
				else if (aimbot::Hitbox == 3)
					hitbox = ue5->get_bone_3d(closest_mesh, 2);

				if (aimbot::prediction)
				{
					projectileSpeed = 50000.f;
					projectileGravityScale = 3.5f;

					int bone;
					if (projectileSpeed)
					{
						bone = 110;
					}

					auto Distance = camera_postion.location.distance(hitbox);

					if (projectileSpeed)
					{
						hitbox = ue5->target_prediction(hitbox, Velocity, projectileSpeed, projectileGravityScale, Distance);  //1
					}
				}

				fvector2d hitbox_screen = ue5->w2s(hitbox);

				if (hitbox.x != 0 || hitbox.y != 0 && (get_cross_distance(hitbox.x, hitbox.y, screen_width / 2, screen_height / 2) <= aimbot::fovsize))
				{
					if (GetAsyncKeyState_Spoofed(aimbot::aimkey))
						input->move(hitbox_screen);
				}
			}
		}

		void carfly()
		{
			while (true)
			{
				uint64_t CurrentVehicle = io::Read<uint64_t>(g_ptr->acknowledged_pawn + 0x29e8); //AFortPlayerPawn::CurrentVehicle
				io::Write<bool>(CurrentVehicle + 0x813, 0);
				if (GetAsyncKeyState(VK_F4))
				{
					io::Write<bool>(CurrentVehicle + 0x87a, 5); //AFortAthenaVehicle::bUseGravity
				}

				Sleep(1);
			}
		}
	};
} static g_loop::g_fn* g_main = new g_loop::g_fn();
