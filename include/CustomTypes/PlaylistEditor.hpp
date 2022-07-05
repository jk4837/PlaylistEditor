#pragma once

#include "Utils/FileUtils.hpp"
#include "Utils/UIUtils.hpp"

#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsViewController.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/LevelSearchViewController.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "HMUI/ModalView.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "questui/shared/CustomTypes/Components/ClickableText.hpp"

#include "CustomTypes/DoubleClickIconButton.hpp"
#include "CustomTypes/ListModal.hpp"
#include "CustomTypes/TwoStateIconButton.hpp"

namespace PlaylistEditor
{

class PlaylistEditor
{
public:

    static PlaylistEditor *GetInstance();

    bool IsSelectedCustomPack();

    void Init(HMUI::FlowCoordinator *flowCoordinator);
    void CreateListActionButton();
    void CreateSongActionButton();
    void CreatePackHeaderDetail();
    void SelectLockCharDiff();
    custom_types::Helpers::Coroutine doAdjustUI(const bool forceDisable = false);
    void AdjustUI(const bool forceDisable = false);   // use forceDisable, cause don't know how to decide if now at main menu

    bool isLevelDetailReady = false;
    ArrayW<GlobalNamespace::IDifficultyBeatmap*>* difficultyBeatmaps = nullptr;
    ArrayW<::GlobalNamespace::IDifficultyBeatmapSet*>* difficultyBeatmapSets = nullptr;
private:
    typedef enum REFESH_TYPE {
        SONG_STAY, SONG_REMOVE_STAY, SONG_MOVE_UP, SONG_MOVE_DOWN, PACK_INSERT, PACK_DELETE
    } REFESH_TYPE_T;

    bool IsSelectedSoloOrPartyPlay();
    bool IsSelectedCustomCategory();
    bool IsSelectedCustomLevel();
    bool IsSelectedCustomPackUsingDefaultCover();
    GlobalNamespace::CustomPreviewBeatmapLevel *GetSelectedCustomPreviewBeatmapLevel();
    int GetSelectedCustomLevelIdx();
    const std::string GetSelectedPackID();
    int GetSelectedPackIdx();
    std::string GetSelectedCharStr();
    int GetSelectedDiff();
    std::string GetSelectedPackDuration();
    int FindPackIdx(const std::string &name, const int startIdx = 0);

    bool UpdateFileWithSelected(const FILE_ACTION act);
    bool UpdateFileWithSelected(const FILE_ACTION act, const int selectedPackIdx, const std::string &selectedPackId); // for insert action

    void MoveUpSelectedSong();
    void MoveDownSelectedSong();
    void InsertSelectedSongToPack(int collectionIdx);
    void RemoveSelectedSongInPack();
    void RemoveSongsInPack(GlobalNamespace::IBeatmapLevelCollection *beatmapLevelCollection, const StringW &levelID);
    void RemoveSelectedSongInAllPack(const bool includeCustomLevel);
    void RemoveSongsInFilterList(const StringW &levelID);
    void SetSelectedCoverImage(const int collectionIdx, UnityEngine::Sprite *image = nullptr);
    bool CreateList(const std::string &name);

    void AcquiredObject();
    void RegistEvent();
    void ResetUI();
    void RemoveSelectedSongInTable();
    void InsertSelectedSongToTable();
    void RefreshAndStayList(const REFESH_TYPE act);

    bool init = false;

    HMUI::FlowCoordinator *FlowCoordinator = nullptr;
    GlobalNamespace::AnnotatedBeatmapLevelCollectionsViewController *AnnotatedBeatmapLevelCollectionsViewController = nullptr;
    GlobalNamespace::LevelCollectionNavigationController *LevelCollectionNavigationController = nullptr;
    GlobalNamespace::LevelCollectionTableView *LevelCollectionTableView = nullptr;
    GlobalNamespace::LevelFilteringNavigationController *LevelFilteringNavigationController = nullptr;
    GlobalNamespace::LevelSearchViewController *LevelSearchViewController = nullptr;
    GlobalNamespace::LevelSelectionNavigationController *LevelSelectionNavigationController = nullptr;
    GlobalNamespace::SelectLevelCategoryViewController *LevelCategoryViewController = nullptr;
    GlobalNamespace::StandardLevelDetailView *StandardLevelDetailView = nullptr;

    DoubleClickIconButton *deleteAndRemoveButton = nullptr;
    DoubleClickIconButton *deleteButton = nullptr;
    DoubleClickIconButton *deleteListButton = nullptr;
    IconButton *createListButton = nullptr;
    IconButton *insertButton = nullptr;
    IconButton *moveDownButton = nullptr;
    IconButton *moveUpButton = nullptr;
    IconButton *removeButton = nullptr;
    TwoStateIconButton *lockButton = nullptr;
    TwoStateIconButton *imageListButton = nullptr;
    TwoStateIconButton *recordListButton = nullptr;
    HMUI::InputFieldView *createListInput = nullptr;
    HMUI::InputFieldView *recordListInput = nullptr;
    ListModal *listModal;

    int lastInsertPackIdx = -1;
    std::string lastInsertPackName = "";
    FileUtils fileUtils;

    int selectedLockDiff = 0;
    std::string selectedLockCharStr = "";

    int recordPackIdx = -1;
    std::string recordPackName = "";

    TMPro::TextMeshProUGUI *packDurationText = nullptr;
};

}
