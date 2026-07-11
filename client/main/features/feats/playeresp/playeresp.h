#pragma once
#include "Includes.h"

namespace MecchaCheatV::Features::Visuals
{
	class PlayerESP : public FeatureCore
	{
	public:
		PlayerESP();
		~PlayerESP() override = default;

		void OnActivate() override {}
		void OnDeactivate() override {}
		void OnRender() override;
		void OnMenuRender() override;

		bool DrawBox(SDK::AActor* actor, float& outMinX, float& outMinY, float& outMaxX, float& outMaxY);
		void DrawSkeleton(SDK::USkeletalMeshComponent* Mesh, SDK::APlayerController* PC, ImU32 color, float thickness);
		void DrawCornerBox(float minX, float minY, float maxX, float maxY, ImU32 color, float thickness);
		void DrawFullBox(float minX, float minY, float maxX, float maxY, ImU32 color, float thickness);
		void DrawRoundedBox(float minX, float minY, float maxX, float maxY, ImU32 color, float thickness);
		ImU32 GetPlayerColor(SDK::AActor* actor, bool isHunter, bool isVisible);
		void DrawSnapline(const SDK::FVector2D& target, ImU32 color, float thickness);
		std::string GetRoleText(SDK::AActor* actor);
	};
}