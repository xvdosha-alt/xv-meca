#pragma once
#include "Includes.h"

namespace MecchaCheatV::Features::Visuals
{
	class WebOnly : public FeatureCore
	{
	public:
		WebOnly();
		~WebOnly() override = default;
		void OnActivate() override {}
		void OnDeactivate() override {}
		void OnRender() override {}
		void OnMenuRender() override {}
	};
}
