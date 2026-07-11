#include "Includes.h"
#include "../../features/features_includes.h"
#include "main/webpanel/overlay_canvas.h"

using namespace MecchaCheatV;

void Hooks::HkProcessEvent(SDK::UObject* Object, SDK::UFunction* Function, void* Params)
{
	LOG_CALL_UPDATE("Called ProcessEvent");

	static thread_local bool is_executing_hook = false;
	if (is_executing_hook)
	{
		return Globals::hookedProcessEvent(Object, Function, Params);
	}

    if (Globals::ForTests)
    {
        Globals::ForTests = false;
		NOTIFY_INFO_QUICK("Test called!");

		
    }

	is_executing_hook = true;

	CALL_METHOD_IF_ACTIVE(Player, SprintMultiplier, SprintMultiplierHandler);

	is_executing_hook = false;

	if (Function)
	{
		std::string fname = Function->GetName();
		std::string outer = Function->Outer ? Function->Outer->GetName() : std::string();

		if (Globals::needTeleport && Utils::GetAcknowledgedPawn() && Utils::isObjectValid(Utils::GetAcknowledgedPawn()))
			Utils::ProcessTeleport(Utils::GetAcknowledgedPawn());

		if (fname == "OnRep_BodyVisibility")
			CALL_METHOD_IF_ACTIVE_ARGS(Player, AlwaysVisible, AlwaysVisibleHandle, Object);
		
        if (fname == "PaintTick")
			CALL_METHOD_IF_ACTIVE_ARGS(Player, AutoDissShadow, AutoDissShadowHandle, Object);
	}

	Globals::hookedProcessEvent(Object, Function, Params);

	if (Function)
	{
		const std::string fname = Function->GetName();
		if ( fname == "ReceiveTick" )
		{
			SDK::APawn* localPawn = Utils::GetAcknowledgedPawn();
			if ( localPawn && Object == localPawn && Utils::isObjectValid( localPawn ) )
			{
				CALL_METHOD_IF_ACTIVE( Player , NoDetection , NoDetectionHandler );

				if ( GET_FEATURE_HANDLER() && GET_FEATURE_HANDLER()->IsWebOnlyEnabled()
					&& !Globals::WebEspFrameDone.exchange( true , std::memory_order_acq_rel ) )
				{
					const float frameW = static_cast<float>( Renderer::BufferWidth );
					const float frameH = static_cast<float>( Renderer::BufferHeight );
					if ( frameW > 0.f && frameH > 0.f )
					{
						ImGui::GetIO().DisplaySize = ImVec2( frameW , frameH );
						OverlayCanvas::BeginFrame( frameW , frameH );
						GET_FEATURE_HANDLER()->RenderAll();
					}
				}
			}
		}
	}
}
