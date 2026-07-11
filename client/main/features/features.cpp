#include "features.h"
#include "features_includes.h"
#include "../menu/xv_widgets.h"

using namespace MecchaCheatV;
using namespace MecchaCheatV::Globals;
using namespace MecchaCheatV::XvWidgets;
using namespace Features::Visuals;
using namespace Features::Player;

const char* MecchaCheatV::GetFeatureTypeIcon(const FeatureType type)
{
    switch (type)
    {
    case TYPE_NONE: return "N";
    case TYPE_VISUALS: return "V";
    case TYPE_PLAYER: return "P";
    default: return "U";
    }
}

const char* MecchaCheatV::GetFeatureTypeName(const FeatureType type)
{
    switch (type)
    {
    case TYPE_NONE: return "None";
    case TYPE_VISUALS: return "Visuals";
    case TYPE_PLAYER: return "Player(s)";
    default: return "Unknown";
    }
}

FeatureHandler::FeatureHandler() : CurrentType(TYPE_NONE)
{
    FeatureRegistry = std::vector<std::pair<std::string, std::unique_ptr<FeatureCore>>>();

    
    ADD_FEATURE(this, WebOnly);
    ADD_FEATURE(this, PlayerESP);
    ADD_FEATURE(this, DecoyESP);

    
	ADD_FEATURE(this, SetName);
	ADD_FEATURE(this, AlwaysVisible);
    ADD_FEATURE(this, NoDetection);
	ADD_FEATURE(this, AutoDissShadow);
	ADD_FEATURE(this, Teleport);
	ADD_FEATURE(this, SprintMultiplier);

    MainFeatureHandler = this;
}

FeatureHandler::~FeatureHandler()
{
    for (auto& [name, feature] : FeatureRegistry)
    {
        if (feature && feature->IsActive())
        {
            feature->OnDeactivate();
        }
    }

    MainFeatureHandler = nullptr;
}

void FeatureHandler::RegisterFeature(std::string_view name, std::unique_ptr<FeatureCore> feature)
{
    auto* ptr = feature.get();
    FeatureRegistry.emplace_back(std::string(name), std::move(feature));
    if (ptr->IsActive())
        ptr->OnActivate();
}

FeatureCore* FeatureHandler::FindFeature(const std::string_view name) const
{
    for (const auto& [featureName, feature] : FeatureRegistry)
    {
        if (featureName == name)
            return feature.get();
    }
    return nullptr;
}

void FeatureHandler::RenderAll() const
{
    for (const auto& [name, feature] : FeatureRegistry)
    {
        if (feature->IsActive())
        {
            feature->OnRender();
        }
    }
}

bool FeatureHandler::FeatureMatchesSearch(const std::string& featureName, const std::string& query) const
{
    if (query.empty()) return false;

    auto it = std::search(
        featureName.begin(), featureName.end(),
        query.begin(), query.end(),
        [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    );

    return (it != featureName.end());
}

void FeatureHandler::ShowTypeSelector()
{
    constexpr std::array<FeatureType, 2> TYPES = { TYPE_VISUALS, TYPE_PLAYER };

    const float width = ImGui::GetContentRegionAvail().x;
    const float btnSize = std::min( ( width - 40.f * dpiScale ) * 0.5f , 220.f * dpiScale );
    const ImVec2 size( btnSize , btnSize );

    ImGui::SetNextItemWidth( width - 20.f * dpiScale );
    ImGui::InputTextWithHint( "##SearchFeatures" , "Search features..." , SearchBuffer , sizeof( SearchBuffer ) );
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if ( SearchBuffer[0] != '\0' )
    {
        std::string query = SearchBuffer;
        bool foundAny = false;

        ImGui::BeginChild( "SearchResults" , ImVec2( 0 , 0 ) , true );

        for ( const auto& [name, feature] : FeatureRegistry )
        {
            if ( FeatureMatchesSearch( name , query ) || FeatureMatchesSearch( feature->GetFeatureTitle() , query ) )
            {
                if ( !feature->ShowInMenu )
                    continue;

                bool enabled = feature->IsActive();
                if ( BeginFeatureCard( name.c_str() , feature->GetFeatureTitle().c_str() , enabled ) )
                {
                    if ( enabled != feature->IsActive() )
                    {
                        SET_CONFIG_VALUE( feature->GetConfigManager() , "Enabled" , bool , enabled );
                        if ( enabled )
                            feature->OnActivate();
                        else
                            feature->OnDeactivate();
                    }

                    feature->OnMenuRender();
                    EndFeatureCard();
                    foundAny = true;
                }
            }
        }

        if ( !foundAny )
            ImGui::TextDisabled( "No features found" );

        ImGui::EndChild();
        return;
    }

    ImGui::Spacing();
    ImGui::BeginGroup();

    int count = 0;
    for ( FeatureType type : TYPES )
    {
        if ( count > 0 )
            ImGui::SameLine( 0.f , 16.f * dpiScale );

        const char* icon = GetFeatureTypeIcon( type );
        const char* label = GetFeatureTypeName( type );

        if ( RenderCategoryTile( icon , label , size ) )
            CurrentType = type;

        ++count;
    }

    ImGui::EndGroup();
}

void FeatureHandler::ShowFeaturesByType()
{
    const char* categoryName = GetFeatureTypeName( CurrentType );

    ImGui::PushFont( ImGui::GetIO().Fonts->Fonts[1] );
    ImGui::TextColored( accentPurple , "%s" , categoryName );
    ImGui::PopFont();
    ImGui::SameLine();
    if ( ImGui::SmallButton( "Back" ) )
    {
        CurrentType = TYPE_NONE;
        return;
    }

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::BeginChild( "FeaturesContent" , ImVec2( 0 , 0 ) , ImGuiChildFlags_None , ImGuiWindowFlags_AlwaysVerticalScrollbar );

    for ( const auto& [name, feature] : FeatureRegistry )
    {
        if ( feature->GetFeatureType() != CurrentType || !feature->ShowInMenu )
            continue;

        bool enabled = feature->IsActive();
        if ( !BeginFeatureCard( name.c_str() , feature->GetFeatureTitle().c_str() , enabled ) )
            continue;

        if ( enabled != feature->IsActive() )
        {
            SET_CONFIG_VALUE( feature->GetConfigManager() , "Enabled" , bool , enabled );
            if ( enabled )
                feature->OnActivate();
            else
                feature->OnDeactivate();
        }

        feature->OnMenuRender();
        EndFeatureCard();
        ImGui::Spacing();
    }

    ImGui::EndChild();
}

void FeatureHandler::ApplyConfigStates()
{
    for (auto& [name, feature] : FeatureRegistry)
    {
        if (feature->IsActive())
            feature->OnActivate();
    }
}

void FeatureHandler::RenderMenu()
{
    if (CurrentType == TYPE_NONE)
    {
        ShowTypeSelector();
    }
    else
    {
        ShowFeaturesByType();
    }
}

std::string FeatureHandler::BuildWebStateJson() const
{
    nlohmann::json root;
    root["name"] = CheatName;
    root["port"] = WebPanelPort;

    nlohmann::json features = nlohmann::json::array();

    for ( const auto& [name , feature] : FeatureRegistry )
    {
        if ( !feature->ShowInMenu )
            continue;

        nlohmann::json entry;
        entry["id"] = name;
        entry["title"] = feature->GetFeatureTitle();
        entry["category"] = GetFeatureTypeName( feature->GetFeatureType() );
        entry["enabled"] = feature->IsActive();

        nlohmann::json options = nlohmann::json::array();
        for ( const auto& config : feature->GetConfigManager()->GetAllConfigs() )
        {
            const auto& configName = config->GetEntryName();
            if ( configName == "Enabled" )
                continue;

            const auto& value = config->GetEntryValue();
            nlohmann::json option;

            if ( std::holds_alternative<bool>( value ) )
            {
                option["key"] = configName;
                option["type"] = "bool";
                option["value"] = std::get<bool>( value );
                options.push_back( option );
            }
            else if ( std::holds_alternative<int>( value ) )
            {
                option["key"] = configName;
                option["type"] = "int";
                option["value"] = std::get<int>( value );
                options.push_back( option );
            }
            else if ( std::holds_alternative<float>( value ) )
            {
                option["key"] = configName;
                option["type"] = "float";
                option["value"] = std::get<float>( value );
                options.push_back( option );
            }
            else if ( std::holds_alternative<std::string>( value ) )
            {
                option["key"] = configName;
                option["type"] = "string";
                option["value"] = std::get<std::string>( value );
                options.push_back( option );
            }
        }

        entry["options"] = options;
        features.push_back( entry );
    }

    root["features"] = features;
    return root.dump();
}

bool FeatureHandler::ApplyWebFeatureUpdate( const nlohmann::json& body )
{
    if ( !body.contains( "id" ) || !body["id"].is_string() || !body.contains( "enabled" ) || !body["enabled"].is_boolean() )
        return false;

    const std::string id = body["id"].get<std::string>();
    auto* feature = FindFeature( id );
    if ( !feature )
        return false;

    const bool enabled = body["enabled"].get<bool>();
    SET_CONFIG_VALUE( feature->GetConfigManager() , "Enabled" , bool , enabled );

    if ( enabled )
        feature->OnActivate();
    else
        feature->OnDeactivate();

    return true;
}

bool FeatureHandler::ApplyWebOptionUpdate( const nlohmann::json& body )
{
    if ( !body.contains( "id" ) || !body["id"].is_string() || !body.contains( "key" ) || !body["key"].is_string() || !body.contains( "value" ) )
        return false;

    const std::string id = body["id"].get<std::string>();
    const std::string key = body["key"].get<std::string>();

    auto* feature = FindFeature( id );
    if ( !feature || key == "Enabled" )
        return false;

    auto* manager = feature->GetConfigManager();

    try
    {
        const auto& config = manager->GetConfig( key );
        const auto& value = config->GetEntryValue();

        if ( std::holds_alternative<bool>( value ) && body["value"].is_boolean() )
        {
            SET_CONFIG_VALUE( manager , key , bool , body["value"].get<bool>() );
            return true;
        }

        if ( std::holds_alternative<int>( value ) && body["value"].is_number_integer() )
        {
            SET_CONFIG_VALUE( manager , key , int , body["value"].get<int>() );
            return true;
        }

        if ( std::holds_alternative<float>( value ) && body["value"].is_number() )
        {
            SET_CONFIG_VALUE( manager , key , float , body["value"].get<float>() );
            return true;
        }

        if ( std::holds_alternative<std::string>( value ) && body["value"].is_string() )
        {
            SET_CONFIG_VALUE( manager , key , std::string , body["value"].get<std::string>() );
            return true;
        }
    }
    catch ( ... )
    {
        return false;
    }

    return false;
}

std::string FeatureHandler::BuildTeleportPlayersJson() const
{
    auto* teleport = GetFeature<Features::Player::Teleport>( "Teleport" );
    if ( !teleport )
        return R"({"players":[]})";

    return teleport->BuildPlayersJson();
}

bool FeatureHandler::ApplyTeleportCoords( float x , float y , float z )
{
    auto* teleport = GetFeature<Features::Player::Teleport>( "Teleport" );
    if ( !teleport )
        return false;

    return teleport->TeleportToCoords( x , y , z );
}

bool FeatureHandler::ApplyTeleportToPlayer( int index )
{
    auto* teleport = GetFeature<Features::Player::Teleport>( "Teleport" );
    if ( !teleport )
        return false;

    return teleport->TeleportToPlayerIndex( index );
}

bool FeatureHandler::ApplySetName( const std::string& name )
{
    auto* setName = GetFeature<Features::Player::SetName>( "SetName" );
    if ( !setName )
        return false;

    return setName->ApplyName( name );
}

bool FeatureHandler::IsWebOnlyEnabled() const
{
    auto* webOnly = GetFeature<Features::Visuals::WebOnly>( "WebOnly" );
    return webOnly && webOnly->IsActive();
}