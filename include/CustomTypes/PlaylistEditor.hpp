#pragma once

#include "Utils/UIUtils.hpp"

#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsViewController.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSegmentedControlController.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
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
    GlobalNamespace::BeatmapCharacteristicSegmentedControlController *BeatmapCharacteristicSelectionViewController = nullptr;
    GlobalNamespace::LevelCollectionNavigationController *LevelCollectionNavigationController = nullptr;
    GlobalNamespace::LevelCollectionTableView *LevelCollectionTableView = nullptr;
    GlobalNamespace::LevelCollectionViewController *LevelCollectionViewController = nullptr;
    GlobalNamespace::LevelFilteringNavigationController *LevelFilteringNavigationController = nullptr;
    GlobalNamespace::LevelSelectionNavigationController *LevelSelectionNavigationController = nullptr;
    GlobalNamespace::SelectLevelCategoryViewController *LevelCategoryViewController = nullptr;
    GlobalNamespace::StandardLevelDetailView *StandardLevelDetailView = nullptr;
    GlobalNamespace::StandardLevelDetailViewController *LevelDetailViewController = nullptr;

    DoubleClickIconButton *deleteAndRemoveButton = nullptr;
    DoubleClickIconButton *deleteButton = nullptr;
    DoubleClickIconButton *deleteListButton = nullptr;
    UnityEngine::UI::Button *createListButton = nullptr;
    UnityEngine::UI::Button *insertButton = nullptr;
    UnityEngine::UI::Button *moveDownButton = nullptr;
    UnityEngine::UI::Button *moveUpButton = nullptr;
    UnityEngine::UI::Button *removeButton = nullptr;
    UnityEngine::GameObject *listContainer = nullptr;
    HMUI::ImageView *moveDownButtonImageView = nullptr;
    HMUI::ImageView *moveUpButtonImageView = nullptr;
    HMUI::InputFieldView *createListInput = nullptr;
    HMUI::ModalView *listModal = nullptr;
    std::vector<QuestUI::ClickableText *> listModalItem;
};

}