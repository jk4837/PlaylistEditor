#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "System/Action.hpp"
#include "playlistmanager/shared/PlaylistManager.hpp"
#include "songloader/shared/API.hpp"

#include "main.hpp"
#include "CustomTypes/PlaylistEditor.hpp"
#include "CustomTypes/Logging.hpp"
#include "Utils/FileUtils.hpp"

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup
static PlaylistEditor::PlaylistEditor *playlistEditor = nullptr;

// Loads the config from disk using our modInfo, then returns it for use
Configuration &getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

MAKE_HOOK_MATCH(FlowCoordinator_PresentFlowCoordinator, &HMUI::FlowCoordinator::PresentFlowCoordinator, void,
                HMUI::FlowCoordinator *self, HMUI::FlowCoordinator *flowCoordinator, System::Action *finishedCallback,
                HMUI::ViewController::AnimationDirection animationDirection, bool immediately, bool replaceTopViewController)
{
    FlowCoordinator_PresentFlowCoordinator(self, flowCoordinator, finishedCallback, animationDirection, immediately, replaceTopViewController);
    playlistEditor->Init(flowCoordinator);
    playlistEditor->CreateListActionButton();
    playlistEditor->AdjustUI();
}

MAKE_HOOK_MATCH(StandardLevelDetailView_RefreshContent, &GlobalNamespace::StandardLevelDetailView::RefreshContent,
                void, GlobalNamespace::StandardLevelDetailView *self)
{
    StandardLevelDetailView_RefreshContent(self);
    playlistEditor->AdjustUI();
}

MAKE_HOOK_MATCH(StandardLevelDetailViewController_ShowContent, &GlobalNamespace::StandardLevelDetailViewController::ShowContent,
                void, GlobalNamespace::StandardLevelDetailViewController *self,
                GlobalNamespace::StandardLevelDetailViewController::ContentType contentType,
                ::StringW errorText, float downloadingProgress, ::StringW downloadingText)
{
    StandardLevelDetailViewController_ShowContent(self, contentType, errorText, downloadingProgress, downloadingText);
    playlistEditor->CreateSongActionButton();
    playlistEditor->AdjustUI();
}


MAKE_HOOK_MATCH(MainFlowCoordinator_DidActivate, &GlobalNamespace::MainFlowCoordinator::DidActivate, void, GlobalNamespace::MainFlowCoordinator *self,
                bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    INFO("MainFlowCoordinator_DidActivate");
    MainFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    playlistEditor->AdjustUI(true);
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo &info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;

    getConfig().Load(); // Load the config file
    PlaylistEditor::FileUtils::ShrinkPlaylistPath();
    INFO("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    playlistEditor = PlaylistEditor::PlaylistEditor::GetInstance();

    INFO("Installing hooks...");
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), MainFlowCoordinator_DidActivate);
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), StandardLevelDetailViewController_ShowContent);
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), StandardLevelDetailView_RefreshContent);
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), FlowCoordinator_PresentFlowCoordinator);
    INFO("Installed all hooks!");
}
