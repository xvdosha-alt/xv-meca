#pragma once
#include "Includes.h"

namespace MecchaCheatV::Features::Player
{
	class Teleport : public FeatureCore
	{
	public:
		Teleport();
		~Teleport() override = default;

		void OnActivate() override;
		void OnDeactivate() override;
		void OnRender() override {}
		void OnMenuRender() override;

		std::string BuildPlayersJson();
		bool TeleportToCoords( float x , float y , float z );
		bool TeleportToPlayerIndex( int index );
		void RefreshPlayers();
	private:
		std::vector<SDK::APawn*> cachedPlayers;
		int selectedPlayerIndex = -1;
		void UpdatePlayers();
	};
}