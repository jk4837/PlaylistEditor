#include "GlobalNamespace/BeatmapCharacteristicSegmentedControlController.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapDifficultySegmentedControlController.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "UnityEngine/Resources.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "System/Action.hpp"
#include "playlistmanager/shared/PlaylistManager.hpp"
#include "songloader/shared/API.hpp"

#include "main.hpp"
#include "CustomTypes/PlaylistEditor.hpp"
#include "CustomTypes/Logging.hpp"
#include "Utils/FileUtils.hpp"
#include "Utils/UIUtils.hpp"

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
    playlistEditor->CreatePackHeaderDetail();
    playlistEditor->AdjustUI();
}

MAKE_HOOK_MATCH(StandardLevelDetailView_RefreshContent, &GlobalNamespace::StandardLevelDetailView::RefreshContent,
                void, GlobalNamespace::StandardLevelDetailView *self)
{
    // when changing char or diff
    StandardLevelDetailView_RefreshContent(self);
    playlistEditor->AdjustUI();
}

MAKE_HOOK_MATCH(StandardLevelDetailViewController_ShowContent, &GlobalNamespace::StandardLevelDetailViewController::ShowContent,
                void, GlobalNamespace::StandardLevelDetailViewController *self,
                GlobalNamespace::StandardLevelDetailViewController::ContentType contentType,
                ::StringW errorText, float downloadingProgress, ::StringW downloadingText)
{
    // when select new level
    StandardLevelDetailViewController_ShowContent(self, contentType, errorText, downloadingProgress, downloadingText);
    playlistEditor->CreateSongActionButton();
    playlistEditor->SelectLockCharDiff();
    playlistEditor->AdjustUI();
}

static void showRestoreDialog()
{
    auto mainScreen = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Transform*>().First([] (auto x) {
        return x->get_name()->Equals("MainScreen");
    });
    PlaylistEditor::Utils::ShowRestoreDialog(mainScreen, [&] () {
        INFO("Restore playlists");
        PlaylistEditor::FileUtils::RestorePlaylistFile();

        INFO("Refresh playlists");
        auto customBeatmapLevelPackCollectionSO = UnityEngine::Resources::FindObjectsOfTypeAll<RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO*>().First();
        customBeatmapLevelPackCollectionSO->ClearLevelPacks();
        PlaylistManager::LoadPlaylists(customBeatmapLevelPackCollectionSO, true);
        RuntimeSongLoader::API::RefreshPacks(true);
    }, [] () {
        INFO("Not restore playlists");
        PlaylistEditor::FileUtils::RemoveTmpDir();
    });
}

MAKE_HOOK_MATCH(MainFlowCoordinator_DidActivate, &GlobalNamespace::MainFlowCoordinator::DidActivate, void, GlobalNamespace::MainFlowCoordinator *self,
                bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    INFO("MainFlowCoordinator_DidActivate");
    MainFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    playlistEditor->AdjustUI(true);
    if (PlaylistEditor::FileUtils::askUserRestore) {
        PlaylistEditor::FileUtils::askUserRestore = false;
        showRestoreDialog();
    } else
        PlaylistEditor::FileUtils::RemoveTmpDir();
}

// for getting difficultyBeatmaps
MAKE_HOOK_MATCH(BeatmapDifficultySegmentedControlController_SetData, &GlobalNamespace::BeatmapDifficultySegmentedControlController::SetData,
                void, GlobalNamespace::BeatmapDifficultySegmentedControlController *self,
                System::Collections::Generic::IReadOnlyList_1<GlobalNamespace::IDifficultyBeatmap*>* difficultyBeatmaps,
                GlobalNamespace::BeatmapDifficulty selectedDifficulty)
{
    if (playlistEditor->IsSelectedCustomPack())
        playlistEditor->difficultyBeatmaps = difficultyBeatmaps;
    BeatmapDifficultySegmentedControlController_SetData(self, difficultyBeatmaps, selectedDifficulty);
}

// for getting difficultyBeatmapSets
MAKE_HOOK_MATCH(BeatmapCharacteristicSegmentedControlController_SetData, &GlobalNamespace::BeatmapCharacteristicSegmentedControlController::SetData,
                void, GlobalNamespace::BeatmapCharacteristicSegmentedControlController *self,
                System::Collections::Generic::IReadOnlyList_1<::GlobalNamespace::IDifficultyBeatmapSet*>* difficultyBeatmapSets,
                GlobalNamespace::BeatmapCharacteristicSO* selectedBeatmapCharacteristic)
{
    if (playlistEditor->IsSelectedCustomPack())
        playlistEditor->difficultyBeatmapSets = difficultyBeatmapSets;
    BeatmapCharacteristicSegmentedControlController_SetData(self, difficultyBeatmapSets, selectedBeatmapCharacteristic);
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo &info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;

    getConfig().Load(); // Load the config file
    PlaylistEditor::FileUtils::ShrinkPlaylistPath();
    PlaylistEditor::FileUtils::AppendPlaylistData();
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
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), BeatmapDifficultySegmentedControlController_SetData);
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), BeatmapCharacteristicSegmentedControlController_SetData);
    INFO("Installed all hooks!");
}
