#pragma once
#include <string>

namespace MecchaCheatV::WebPanel
{
	bool Start();
	void Stop();
	void OpenInBrowser();
	std::string GetPanelUrl();
	bool IsRunning();
}
