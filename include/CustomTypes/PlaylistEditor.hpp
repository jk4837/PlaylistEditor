#pragma once

#include "Utils/UIUtils.hpp"

#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsViewController.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "HMUI/ModalView.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "questui/shared/CustomTypes/Components/ClickableText.hpp"

#include "CustomTypes/DoubleClickIconButton.hpp"

namespace PlaylistEditor
{

class PlaylistEditor
{
public:

    static PlaylistEditor *GetInstance();

    void Init(HMUI::FlowCoordinator *flowCoordinator);
    void CreateListActionButton();
    void CreateSongActionButton();
    void AdjustUI(const bool forceDisable = false);   // use forceDisable, cause don't know how to decide if now at main menu

private:
    typedef enum SCROLL_ACTION {
        NO_STAY, SCROLL_STAY, SCROLL_REMOVE_STAY, SCROLL_MOVE_UP, SCROLL_MOVE_DOWN
    } MOVE_ACTION_T;

    bool IsSelectedSoloOrPartyPlay();
    bool IsSelectedCustomCategory();
    bool IsSelectedCustomPack();
    bool IsSelectedCustomLevel();
    GlobalNamespace::CustomPreviewBeatmapLevel *GetSelectedCustomPreviewBeatmapLevel();
    const std::string GetSelectedPackID();

    void AcquiredObject();
    void RegistEvent();
    void ResetUI();
    void RefreshAndStayList(const SCROLL_ACTION act);

    HMUI::FlowCoordinator *FlowCoordinator = nullptr;
    GlobalNamespace::AnnotatedBeatmapLevelCollectionsViewController *AnnotatedBeatmapLevelCollectionsViewController = nullptr;
    GlobalNamespace::LevelCollectionNavigationController *LevelCollectionNavigationController = nullptr;
    GlobalNamespace::LevelCollectionTableView *LevelCollectionTableView = nullptr;
    GlobalNamespace::LevelFilteringNavigationController *LevelFilteringNavigationController = nullptr;
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
    UnityEngine::GameObject *listContainer = nullptr;
    HMUI::InputFieldView *createListInput = nullptr;
    HMUI::ModalView *listModal = nullptr;
    std::vector<QuestUI::ClickableText *> listModalItem;
};

}