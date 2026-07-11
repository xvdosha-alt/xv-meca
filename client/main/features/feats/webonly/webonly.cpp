#include "webonly.h"

using namespace MecchaCheatV::Features::Visuals;

WebOnly::WebOnly() : FeatureCore( "Web Only" , TYPE_VISUALS )
{
	SET_CONFIG_VALUE( GetConfigManager() , "Enabled" , bool , false );
}
