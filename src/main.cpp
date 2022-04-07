
#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsViewController.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSegmentedControlController.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapDifficultySegmentedControlController.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IAnnotatedBeatmapLevelCollection.hpp"
#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/PartyFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "System/Action_1.hpp"
#include "System/Action_2.hpp"
#include "System/Action_4.hpp"
#include "System/Action.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "playlistmanager/shared/PlaylistManager.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/ClickableText.hpp"
#include "questui/shared/CustomTypes/Components/ExternalComponents.hpp"

#include "main.hpp"
#include "CustomTypes/DoubleClickIconButton.hpp"
#include "CustomTypes/Toast.hpp"
#include "logging.hpp"
#include "Utils/FileUtils.hpp"
#include "Utils/UIUtils.hpp"
#include "Utils/Utils.hpp"

using namespace PlaylistEditor::Utils;

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup
// static CustomPreviewBeatmapLevel *selectedlevel = nullptr;
static HMUI::FlowCoordinator* FlowCoordinator = nullptr;
static GlobalNamespace::LevelSelectionNavigationController *LevelSelectionNavigationController = nullptr;
static GlobalNamespace::LevelFilteringNavigationController *LevelFilteringNavigationController = nullptr;
static GlobalNamespace::LevelCollectionNavigationController *LevelCollectionNavigationController = nullptr;
static GlobalNamespace::SelectLevelCategoryViewController *LevelCategoryViewController = nullptr;
static GlobalNamespace::LevelCollectionViewController *LevelCollectionViewController = nullptr;
static GlobalNamespace::LevelCollectionTableView *LevelCollectionTableView = nullptr;
static GlobalNamespace::StandardLevelDetailViewController *LevelDetailViewController = nullptr;
static GlobalNamespace::StandardLevelDetailView *StandardLevelDetailView = nullptr;
static GlobalNamespace::BeatmapCharacteristicSegmentedControlController *BeatmapCharacteristicSelectionViewController = nullptr;
static GlobalNamespace::AnnotatedBeatmapLevelCollectionsViewController *AnnotatedBeatmapLevelCollectionsViewController = nullptr;

static UnityEngine::UI::Button *createListButton = nullptr;
static HMUI::InputFieldView* createListInput = nullptr;
static PlaylistEditor::DoubleClickIconButton *deleteListButton = nullptr;
static PlaylistEditor::DoubleClickIconButton *deleteButton = nullptr;
static UnityEngine::UI::Button *removeButton = nullptr;
static UnityEngine::UI::Button *insertButton = nullptr;
static PlaylistEditor::DoubleClickIconButton *deleteAndRemoveButton = nullptr;
static UnityEngine::UI::Button *moveUpButton = nullptr;
static HMUI::ImageView *moveUpButtonImageView = nullptr;
static UnityEngine::UI::Button *moveDownButton = nullptr;
static HMUI::ImageView *moveDownButtonImageView = nullptr;
static HMUI::ModalView *listModal = nullptr;
static UnityEngine::GameObject *listContainer = nullptr;
static std::vector<QuestUI::ClickableText *> listModalItem;

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

typedef enum SCROLL_ACTION {
    NO_STAY, SCROLL_STAY, SCROLL_REMOVE_STAY, SCROLL_MOVE_UP, SCROLL_MOVE_DOWN
} MOVE_ACTION_T;


static bool IsSelectedSoloOrPartyPlay() {
    return FlowCoordinator &&
           (il2cpp_utils::try_cast<GlobalNamespace::SoloFreePlayFlowCoordinator>(FlowCoordinator) ||
            il2cpp_utils::try_cast<GlobalNamespace::PartyFreePlayFlowCoordinator>(FlowCoordinator)) ? true : false;
}

static bool IsSelectedCustomCategory() {
    return IsSelectedSoloOrPartyPlay() &&
           LevelFilteringNavigationController &&
           (GlobalNamespace::SelectLevelCategoryViewController::LevelCategory::CustomSongs == LevelFilteringNavigationController->get_selectedLevelCategory());
}

static bool IsSelectedCustomPack() {
    return IsSelectedCustomCategory() &&
           LevelCollectionNavigationController &&
           LevelCollectionNavigationController->dyn__levelPack() &&
           CustomLevelID != LevelCollectionNavigationController->dyn__levelPack()->get_packID();
}

static bool IsSelectedCustomLevel() {
    return LevelCollectionTableView &&
           LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel() &&
           il2cpp_functions::class_is_assignable_from(classof(GlobalNamespace::CustomPreviewBeatmapLevel*), il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel())));
}

static GlobalNamespace::CustomPreviewBeatmapLevel *GetSelectedCustomPreviewBeatmapLevel()
{
    return (LevelCollectionTableView && LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel()) ?
            reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel()) : nullptr;
}

static const std::string GetSelectedPackID()
{
    return IsSelectedCustomPack() ? LevelCollectionNavigationController->dyn__levelPack()->get_packID() : "";
}

static void RefreshAndStayList(const SCROLL_ACTION act) {
    const auto lastCollectionName = AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection() ? AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_collectionName() : "";
    const auto lastScrollPos = LevelCollectionTableView->dyn__tableView()->get_scrollView()->dyn__destinationPos();
    auto nextScrollPos = lastScrollPos;
    const int selectedRow = LevelCollectionTableView->dyn__selectedRow();
    int nextSelectedRow;
    const float rowHeight = LevelCollectionTableView->CellSize();
    switch (act) {
        case SCROLL_MOVE_DOWN:
            nextSelectedRow = selectedRow + 1;
            if (nextSelectedRow*rowHeight > (lastScrollPos + 6*rowHeight))
                nextScrollPos = lastScrollPos + rowHeight;
            break;
        case SCROLL_MOVE_UP:
            nextSelectedRow = selectedRow - 1;
            if (nextSelectedRow*rowHeight < lastScrollPos)
                nextScrollPos = lastScrollPos - rowHeight;
            break;
        case SCROLL_STAY:
            nextSelectedRow = selectedRow;
        case SCROLL_REMOVE_STAY:
            nextSelectedRow = (selectedRow < LevelCollectionTableView->NumberOfCells() - 1) ? selectedRow : LevelCollectionTableView->NumberOfCells() - 2;
            break;
        default:
            break;
    }

    RuntimeSongLoader::API::RefreshSongs(true,
    [lastScrollPos, nextScrollPos, nextSelectedRow, lastCollectionName, act] (const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&) {
        INFO("Success refresh song");
        if (NO_STAY == act)
            return;
        if ("" != lastCollectionName) {
            // select level collection
            auto annotatedBeatmapLevelCollections = listToArrayW(AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
            for (int i = 0; i < annotatedBeatmapLevelCollections.Length(); i++)
                if (annotatedBeatmapLevelCollections[i]->get_collectionName()->Equals(lastCollectionName)) {
                    AnnotatedBeatmapLevelCollectionsViewController->SetData(AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections(), i, false);
                    break;
                }
        }

        // select level
        // LevelCollectionTableView->SelectLevel(nextPreviewBeatmapLevels); // this will jump to center
        LevelCollectionTableView->dyn__tableView()->SelectCellWithIdx(nextSelectedRow, true); // select but scroll at head
        LevelCollectionTableView->dyn__tableView()->get_scrollView()->ScrollTo(lastScrollPos, false);
        if (nextScrollPos != lastScrollPos)
            LevelCollectionTableView->dyn__tableView()->get_scrollView()->ScrollTo(nextScrollPos, true);
    });
}

static void resetUI() {
    if (deleteButton)
        deleteButton->ResetUI();
    if (deleteAndRemoveButton)
        deleteAndRemoveButton->ResetUI();
    if (moveUpButton)
        moveUpButton->set_interactable(true);
    if (moveUpButtonImageView)
        moveUpButtonImageView->set_color(UnityEngine::Color::get_white());
    if (moveDownButton)
        moveDownButton->set_interactable(true);
    if (moveDownButtonImageView)
        moveDownButtonImageView->set_color(UnityEngine::Color::get_white());
    if (listModal)
        listModal->get_gameObject()->set_active(false);
    if (moveDownButton)
        moveDownButton->set_interactable(true);
    if (createListInput)
        createListInput->get_gameObject()->set_active(false);
    if (deleteListButton) {
        deleteListButton->SetInteractable(true);
    }
}

void adjustUI(bool forceDisable = false) // use forceDisable, casue don't know how to decide if now at main menu
{
    bool atCustomCategory = IsSelectedCustomCategory();
    bool atCustomPack = IsSelectedCustomPack();
    bool atCustomLevel = IsSelectedCustomLevel();
    INFO("adjustUI: %s%s%s", atCustomCategory ? "atCustomCategory " : "", atCustomPack ? "atCustomPack " : "", atCustomLevel ? "atCustomLevel " : "");
    resetUI();
    // if (deleteButton) {  // manage by songloader
    //     deleteButton->get_gameObject()->set_active(true);
    // }
    if (deleteAndRemoveButton)
        deleteAndRemoveButton->operator->()->get_gameObject()->set_active(!forceDisable && atCustomLevel);
    if (removeButton)
        removeButton->get_gameObject()->set_active(!forceDisable && atCustomLevel);
    if (insertButton)
        insertButton->get_gameObject()->set_active(!forceDisable && atCustomLevel);

    if (moveUpButton) {
        moveUpButton->get_gameObject()->set_active(!forceDisable && atCustomLevel);
        if (!atCustomPack) {
            moveUpButton->set_interactable(false);
            if (moveUpButtonImageView)
                moveUpButtonImageView->set_color(UnityEngine::Color::get_gray());
        }
    }
    if (moveDownButton) {
        moveDownButton->get_gameObject()->set_active(!forceDisable && atCustomLevel);
        if (!atCustomPack) {
            moveDownButton->set_interactable(false);
            if (moveDownButtonImageView)
                moveDownButtonImageView->set_color(UnityEngine::Color::get_gray());
        }
    }
    if (createListButton)
        createListButton->get_gameObject()->set_active(!forceDisable && atCustomCategory);
    if (deleteListButton) {
        deleteListButton->operator->()->get_gameObject()->set_active(!forceDisable && atCustomCategory);
        if (!atCustomPack)
            deleteListButton->SetInteractable(false);
    }
}

static void createListActionButton() {
    if (!createListButton) {
        createListButton = CreateIconButton("CreateListButton", LevelFilteringNavigationController->get_transform(), "PracticeButton",
                                            UnityEngine::Vector2(69.0f, -3.0f), UnityEngine::Vector2(10.0f, 7.0f), [] () {
                if (!createListInput) {
                    auto screenContainer = QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Transform*>(), [](auto x) {
                        return x->get_name()->Equals("ScreenContainer");
                    });
                    createListInput = CreateStringInput(screenContainer->get_transform(), "Ente new playlist name", "",
                                        UnityEngine::Vector2(55.0f, -17.0f), 50.0f, [] (StringW value) {
                                            INFO("Enter %s", std::string(value).c_str());
                                            // validate
                                            auto annotatedBeatmapLevelCollections = listToArrayW(AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
                                            for (int i = 0; i < annotatedBeatmapLevelCollections.Length(); i++) {
                                                std::string selectedPackName = annotatedBeatmapLevelCollections[i]->get_collectionName();
                                                if (value != selectedPackName)
                                                    continue;
                                                PlaylistEditor::Toast::GetInstance()->ShowMessage("List already exist");
                                                return;
                                            }
                                            if (CreateFile(value)) {
                                                RefreshAndStayList(SCROLL_ACTION::SCROLL_STAY);
                                                PlaylistEditor::Toast::GetInstance()->ShowMessage("Create new list");
                                                createListInput->SetText("");
                                            }
                                            createListInput->get_gameObject()->set_active(false);
                    });
                    createListInput->get_gameObject()->set_active(true);
                    return;
                }
                createListInput->get_gameObject()->set_active(!createListInput->get_gameObject()->get_active());
            }, FileToSprite("InsertIcon"), "Create List");
    }
    if (!deleteListButton) {
        deleteListButton = new PlaylistEditor::DoubleClickIconButton("DeleteListButton", LevelFilteringNavigationController->get_transform(), "PracticeButton",
                                            UnityEngine::Vector2(69.0f, 3.0f), UnityEngine::Vector2(10.0f, 7.0f), [] () {
                                if (DeleteFile(GetPlaylistPath(GetSelectedPackID()))) {
                                    PlaylistEditor::Toast::GetInstance()->ShowMessage("Delete selected list");
                                    RefreshAndStayList(SCROLL_ACTION::NO_STAY);
                                }
                            }, FileToSprite("DeleteIcon"), "Delete List");
    }
}

MAKE_HOOK_MATCH(FlowCoordinator_PresentFlowCoordinator, &HMUI::FlowCoordinator::PresentFlowCoordinator, void, HMUI::FlowCoordinator* self, HMUI::FlowCoordinator* flowCoordinator, System::Action* finishedCallback, HMUI::ViewController::AnimationDirection animationDirection, bool immediately, bool replaceTopViewController)
{
    FlowCoordinator_PresentFlowCoordinator(self, flowCoordinator, finishedCallback, animationDirection, immediately, replaceTopViewController);
    FlowCoordinator = flowCoordinator;
    if (il2cpp_utils::try_cast<GlobalNamespace::PartyFreePlayFlowCoordinator>(FlowCoordinator)) {
        INFO("Initializing PlayListEditor for Party Mode");
    } else if (il2cpp_utils::try_cast<GlobalNamespace::SoloFreePlayFlowCoordinator>(FlowCoordinator)) {
        INFO("Initializing PlayListEditor for Solo Mode");
    } else
        return;
    auto LevelSelectionFlowCoordinator = il2cpp_utils::cast<GlobalNamespace::LevelSelectionFlowCoordinator>(flowCoordinator);
    // gather flow coordinator elements
    LevelSelectionNavigationController = LevelSelectionFlowCoordinator->dyn_levelSelectionNavigationController();
    INFO("Acquired LevelSelectionNavigationController [%d]", LevelSelectionNavigationController->GetInstanceID());

    LevelFilteringNavigationController = LevelSelectionNavigationController->dyn__levelFilteringNavigationController();
    INFO("Acquired LevelFilteringNavigationController [%d]", LevelFilteringNavigationController->GetInstanceID());

    LevelCollectionNavigationController = LevelSelectionNavigationController->dyn__levelCollectionNavigationController();
    INFO("Acquired LevelCollectionNavigationController [%d]", LevelCollectionNavigationController->GetInstanceID());

    LevelCollectionViewController = LevelCollectionNavigationController->dyn__levelCollectionViewController();
    INFO("Acquired LevelCollectionViewController [%d]", LevelCollectionViewController->GetInstanceID());

    LevelDetailViewController = LevelCollectionNavigationController->dyn__levelDetailViewController();
    INFO("Acquired LevelDetailViewController [%d]", LevelDetailViewController->GetInstanceID());

    LevelCollectionTableView = LevelCollectionViewController->dyn__levelCollectionTableView();
    INFO("Acquired LevelPackLevelsTableView [%d]", LevelCollectionTableView->GetInstanceID());

    StandardLevelDetailView = LevelDetailViewController->dyn__standardLevelDetailView();
    INFO("Acquired StandardLevelDetailView [%d]", StandardLevelDetailView->GetInstanceID());

    BeatmapCharacteristicSelectionViewController = StandardLevelDetailView->dyn__beatmapCharacteristicSegmentedControlController();
    INFO("Acquired BeatmapCharacteristicSegmentedControlController [%d]", BeatmapCharacteristicSelectionViewController->GetInstanceID());

    AnnotatedBeatmapLevelCollectionsViewController = LevelFilteringNavigationController->dyn__annotatedBeatmapLevelCollectionsViewController();
    INFO("Acquired AnnotatedBeatmapLevelCollectionsViewController from LevelFilteringNavigationController [%d]", AnnotatedBeatmapLevelCollectionsViewController->GetInstanceID());

    LevelCategoryViewController = LevelFilteringNavigationController->dyn__selectLevelCategoryViewController();
    INFO("Acquired LevelCategoryViewController from LevelFilteringNavigationController [%d]", LevelCategoryViewController->GetInstanceID());

    if (!LevelCategoryViewController->dyn_didSelectLevelCategoryEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event LevelCategoryViewController->dyn_didSelectLevelCategoryEvent()");
        std::function<void(GlobalNamespace::SelectLevelCategoryViewController*, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory)> didSelectLevelCategoryEventFun = [] (GlobalNamespace::SelectLevelCategoryViewController*, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory) {
            INFO("LevelCategoryViewController SelectLevelCategoryEvent");
            adjustUI();
        };
        LevelCategoryViewController->add_didSelectLevelCategoryEvent(PlaylistEditor::Utils::MakeDelegate<System::Action_2<GlobalNamespace::SelectLevelCategoryViewController*, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory>*>(didSelectLevelCategoryEventFun));
    }

    if (!LevelCollectionNavigationController->dyn_didSelectLevelPackEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event LevelCollectionNavigationController->dyn_didSelectLevelPackEvent()");
        std::function<void(GlobalNamespace::LevelCollectionNavigationController*, GlobalNamespace::IBeatmapLevelPack*)> didSelectLevelPackEventFun = [] (GlobalNamespace::LevelCollectionNavigationController*, GlobalNamespace::IBeatmapLevelPack*) {
            INFO("LevelCollectionNavigationController SelectLevelPackEvent"); // select to level list header
            adjustUI();
        };
        LevelCollectionNavigationController->add_didSelectLevelPackEvent(PlaylistEditor::Utils::MakeDelegate<System::Action_2<GlobalNamespace::LevelCollectionNavigationController*, GlobalNamespace::IBeatmapLevelPack*>*>(didSelectLevelPackEventFun));
    }

    if (!AnnotatedBeatmapLevelCollectionsViewController->dyn_didOpenBeatmapLevelCollectionsEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event AnnotatedBeatmapLevelCollectionsViewController->dyn_didOpenBeatmapLevelCollectionsEvent()");
        std::function<void()> didOpenBeatmapLevelCollectionsEventFun = [] () {
            INFO("AnnotatedBeatmapLevelCollectionsViewController OpenBeatmapLevelCollectionsEvent");
            adjustUI();
        };
        AnnotatedBeatmapLevelCollectionsViewController->add_didOpenBeatmapLevelCollectionsEvent(PlaylistEditor::Utils::MakeDelegate<System::Action*>(didOpenBeatmapLevelCollectionsEventFun));
    }

    if (!AnnotatedBeatmapLevelCollectionsViewController->dyn_didSelectAnnotatedBeatmapLevelCollectionEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event AnnotatedBeatmapLevelCollectionsViewController->dyn_didSelectAnnotatedBeatmapLevelCollectionEvent()");
        std::function<void(GlobalNamespace::IAnnotatedBeatmapLevelCollection*)> didSelectAnnotatedBeatmapLevelCollectionEventFun = [] (GlobalNamespace::IAnnotatedBeatmapLevelCollection*) {
            INFO("AnnotatedBeatmapLevelCollectionsViewController SelectAnnotatedBeatmapLevelCollectionEvent");
            adjustUI();
        };
        AnnotatedBeatmapLevelCollectionsViewController->add_didSelectAnnotatedBeatmapLevelCollectionEvent(PlaylistEditor::Utils::MakeDelegate<System::Action_1<GlobalNamespace::IAnnotatedBeatmapLevelCollection*>*>(didSelectAnnotatedBeatmapLevelCollectionEventFun));
    }

    createListActionButton();
    adjustUI();
}

static void createActionButton(UnityEngine::Transform *parent) {
    auto deleteButtonTransform = parent->FindChild("DeleteLevelButton");
    if (!deleteButtonTransform)
        return;
    INFO("SetOnClick level delete button, id: %u", deleteButtonTransform->GetInstanceID());
    auto btn = deleteButtonTransform->get_gameObject()->GetComponent<UnityEngine::UI::Button*>();
    deleteButton = new PlaylistEditor::DoubleClickIconButton(btn, [] () {
        GlobalNamespace::CustomPreviewBeatmapLevel *selectedlevel = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel());
        RuntimeSongLoader::API::DeleteSong(std::string(selectedlevel->get_customLevelPath()), [] {
            PlaylistEditor::Toast::GetInstance()->ShowMessage("Delete song");
            RefreshAndStayList(IsSelectedCustomPack() ? SCROLL_ACTION::SCROLL_REMOVE_STAY : SCROLL_ACTION::SCROLL_STAY);
        });
    });

    auto posX = -22.5f;
    deleteAndRemoveButton = new PlaylistEditor::DoubleClickIconButton("DeleteAndRemoveFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(20.0f,7.0f), [] () {
            GlobalNamespace::CustomPreviewBeatmapLevel *selectedlevel = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel());
            RuntimeSongLoader::API::DeleteSong(std::string(selectedlevel->get_customLevelPath()), [] {
                    if (UpdateFile(GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(GetSelectedPackID()), FILE_ACTION::ITEM_REMOVE))
                        PlaylistEditor::Toast::GetInstance()->ShowMessage(IsSelectedCustomPack() ? "Delete song and remove from the list" : "Delete song and remove from all list" );
                    else
                        PlaylistEditor::Toast::GetInstance()->ShowMessage("Delete song");
                    RefreshAndStayList(IsSelectedCustomPack() ? SCROLL_ACTION::SCROLL_REMOVE_STAY : SCROLL_ACTION::SCROLL_STAY);
                }
            );
        }, FileToSprite("DeleteAndRemoveIcon"), "Delete and Remove Song from List");
    deleteAndRemoveButton->operator->()->get_gameObject()->set_active(false);
    // deleteAndRemoveButton->get_transform()->SetAsLastSibling();

    posX += 15.0f + 1.25f ;
    removeButton = CreateIconButton("RemoveFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (UpdateFile(GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(GetSelectedPackID()), FILE_ACTION::ITEM_REMOVE)) {
                PlaylistEditor::Toast::GetInstance()->ShowMessage(IsSelectedCustomPack() ? "Remove song from the list" : "Remove song from all list");
                RefreshAndStayList(IsSelectedCustomPack() ? SCROLL_ACTION::SCROLL_REMOVE_STAY : SCROLL_ACTION::SCROLL_STAY);
            } else
                PlaylistEditor::Toast::GetInstance()->ShowMessage("Song isn't in any list");
        }, FileToSprite("RemoveIcon"), "Remove Song from List");
    removeButton->get_gameObject()->set_active(false);
    removeButton->get_transform()->SetAsLastSibling();

    posX += 10.0f + 1.25f;
    moveUpButton = CreateIconButton("MoveUpFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (UpdateFile(GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(GetSelectedPackID()), FILE_ACTION::ITEM_MOVE_UP)) {
                PlaylistEditor::Toast::GetInstance()->ShowMessage("Move up song");
                RefreshAndStayList(SCROLL_ACTION::SCROLL_MOVE_UP);
            }
        }, FileToSprite("MoveUpIcon"), "Move Up Song from List");
    moveUpButton->get_gameObject()->set_active(false);
    moveUpButtonImageView = moveUpButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([&] (auto x) -> bool {
        return "Icon" == x->get_name();
    });
    moveUpButton->get_transform()->SetAsLastSibling();

    posX += 10.0f + 1.25f;
    moveDownButton = CreateIconButton("MoveDownFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (UpdateFile(GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(GetSelectedPackID()), FILE_ACTION::ITEM_MOVE_DOWN)) {
                PlaylistEditor::Toast::GetInstance()->ShowMessage("Move down song");
                RefreshAndStayList(SCROLL_ACTION::SCROLL_MOVE_DOWN);
            }
        }, FileToSprite("MoveDownIcon"), "Move Down Song from List");
    moveDownButton->get_gameObject()->set_active(false);
    moveDownButtonImageView = moveDownButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([&] (auto x) -> bool {
        return "Icon" == x->get_name();
    });
    moveDownButton->get_transform()->SetAsLastSibling();

    posX += 10.0f + 1.25f;
    insertButton = CreateIconButton("InsertFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (!listModal) {
                auto screenContainer = QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Transform*>(), [](auto x) {
                    return x->get_name()->Equals("ScreenContainer");
                });

                listModal = QuestUI::BeatSaberUI::CreateModal(screenContainer->get_transform(),
                    UnityEngine::Vector2(30.0f, 25.0f), UnityEngine::Vector2(posX + 84.0f, -25.0f), nullptr);
                listContainer = QuestUI::BeatSaberUI::CreateScrollableModalContainer(listModal);
                listModal->get_transform()->set_localScale({0.75, 0.75, 1});

            }
            if (listModal->get_gameObject()->get_active()) {
                listModal->get_gameObject()->set_active(false);
                return;
            }
            listModal->get_gameObject()->set_active(true);
            listModalItem.clear();

            auto annotatedBeatmapLevelCollections = listToArrayW(AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
            for (int i = 0; i < annotatedBeatmapLevelCollections.Length(); i++) {
                std::string selectedPackName = annotatedBeatmapLevelCollections[i]->get_collectionName();
                if (CustomLevelName == selectedPackName)
                    continue;
                listModalItem.push_back(QuestUI::BeatSaberUI::CreateClickableText(listContainer->get_transform(), selectedPackName, false, [selectedPackName] () {
                    std::string selectedPackId = CustomLevelPackPrefixID + selectedPackName;
                    listModal->get_gameObject()->set_active(false);
                    insertButton->set_interactable(true);
                    if (UpdateFile(GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(GetSelectedPackID()), FILE_ACTION::ITEM_INSERT, GetPlaylistPath(selectedPackId))) {
                        PlaylistEditor::Toast::GetInstance()->ShowMessage("Insert song to selected list");
                        RefreshAndStayList(SCROLL_ACTION::SCROLL_STAY);
                    }
                }));
            }
        }, FileToSprite("InsertIcon"), "Insert to List");
    insertButton->get_gameObject()->set_active(false);
    insertButton->get_transform()->SetAsLastSibling();

    createListActionButton();
}

MAKE_HOOK_MATCH(StandardLevelDetailView_RefreshContent, &GlobalNamespace::StandardLevelDetailView::RefreshContent, void, GlobalNamespace::StandardLevelDetailView* self)
{
    StandardLevelDetailView_RefreshContent(self);
    adjustUI();
}

MAKE_HOOK_MATCH(StandardLevelDetailViewController_ShowContent, &GlobalNamespace::StandardLevelDetailViewController::ShowContent, void, GlobalNamespace::StandardLevelDetailViewController* self, GlobalNamespace::StandardLevelDetailViewController::ContentType contentType, ::StringW errorText, float downloadingProgress, ::StringW downloadingText)
{
    StandardLevelDetailViewController_ShowContent(self, contentType, errorText, downloadingProgress, downloadingText);

    if (!deleteButton) {
        createActionButton(self->dyn__standardLevelDetailView()->get_practiceButton()->get_transform()->get_parent());
        adjustUI();
    }
}

#include "codegen/include/GlobalNamespace/MainFlowCoordinator.hpp"

MAKE_HOOK_MATCH(MainFlowCoordinator_DidActivate, &GlobalNamespace::MainFlowCoordinator::DidActivate, void, GlobalNamespace::MainFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    INFO("MainFlowCoordinator_DidActivate");
    MainFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    adjustUI(true);

    // auto mainMenuViewController = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::MainMenuViewController *>().First();
    // if (mainMenuViewController)
    //     mainMenuViewController->dyn__soloButton()->Press();
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;

    getConfig().Load(); // Load the config file
    INFO("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();

    RuntimeSongLoader::API::AddRefreshLevelPacksEvent(
        [] (RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO) {
            INFO("SSSSS AddSongsLoadedEvent %d", customBeatmapLevelPackCollectionSO->GetInstanceID());
            PlaylistManager::LoadPlaylists(customBeatmapLevelPackCollectionSO, true);
            // ::ArrayW<::GlobalNamespace::IBeatmapLevelPack*> beatmapLevelPacks = (reinterpret_cast<GlobalNamespace::IBeatmapLevelPackCollection*>(customBeatmapLevelPackCollectionSO))->get_beatmapLevelPacks();
            // for (int i = 0; i < beatmapLevelPacks.Length(); i++)
            //     INFO("SSSSS    pack #%d [%s] [%s] [%s]", i, std::string(beatmapLevelPacks[i]->get_packID()).c_str(), std::string(beatmapLevelPacks[i]->get_packName()).c_str(), std::string(beatmapLevelPacks[i]->get_shortPackName()).c_str());
        }
    );

    INFO("Installing hooks...");
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), MainFlowCoordinator_DidActivate);
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), StandardLevelDetailViewController_ShowContent);
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), StandardLevelDetailView_RefreshContent);
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), FlowCoordinator_PresentFlowCoordinator);
    INFO("Installed all hooks!");
}
