#include "CustomTypes/PlaylistEditor.hpp"

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IAnnotatedBeatmapLevelCollection.hpp"
#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/PartyFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/ScrollView.hpp"
#include "System/Action_1.hpp"
#include "System/Action_2.hpp"
#include "System/Action_4.hpp"
#include "System/Action.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Transform.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/ClickableText.hpp"
#include "songloader/shared/API.hpp"

#include "CustomTypes/Toast.hpp"
#include "CustomTypes/Logging.hpp"
#include "Utils/FileUtils.hpp"
#include "Utils/Utils.hpp"

using namespace PlaylistEditor::Utils;

namespace PlaylistEditor
{

static PlaylistEditor *Instance = nullptr;

PlaylistEditor *PlaylistEditor::GetInstance()
{
    if (!Instance)
        Instance = new PlaylistEditor();
    return Instance;
}

void PlaylistEditor::Init(HMUI::FlowCoordinator *flowCoordinator)
{
    this->FlowCoordinator = flowCoordinator;

    if (il2cpp_utils::try_cast<GlobalNamespace::PartyFreePlayFlowCoordinator>(flowCoordinator)) {
        INFO("Initializing PlayListEditor for Party Mode");
    } else if (il2cpp_utils::try_cast<GlobalNamespace::SoloFreePlayFlowCoordinator>(flowCoordinator)) {
        INFO("Initializing PlayListEditor for Solo Mode");
    } else
        return;

    this->AcquiredObject();
    this->RegistEvent();
}

void PlaylistEditor::AcquiredObject()
{
    auto LevelSelectionFlowCoordinator = il2cpp_utils::cast<GlobalNamespace::LevelSelectionFlowCoordinator>(this->FlowCoordinator);
    // gather flow coordinator elements
    this->LevelSelectionNavigationController = LevelSelectionFlowCoordinator->dyn_levelSelectionNavigationController();
    INFO("Acquired LevelSelectionNavigationController [%d]", this->LevelSelectionNavigationController->GetInstanceID());

    this->LevelFilteringNavigationController = this->LevelSelectionNavigationController->dyn__levelFilteringNavigationController();
    INFO("Acquired LevelFilteringNavigationController [%d]", this->LevelFilteringNavigationController->GetInstanceID());

    this->LevelCollectionNavigationController = this->LevelSelectionNavigationController->dyn__levelCollectionNavigationController();
    INFO("Acquired LevelCollectionNavigationController [%d]", this->LevelCollectionNavigationController->GetInstanceID());

    this->LevelCollectionViewController = this->LevelCollectionNavigationController->dyn__levelCollectionViewController();
    INFO("Acquired LevelCollectionViewController [%d]", this->LevelCollectionViewController->GetInstanceID());

    this->LevelDetailViewController = this->LevelCollectionNavigationController->dyn__levelDetailViewController();
    INFO("Acquired LevelDetailViewController [%d]", this->LevelDetailViewController->GetInstanceID());

    this->LevelCollectionTableView = this->LevelCollectionViewController->dyn__levelCollectionTableView();
    INFO("Acquired LevelPackLevelsTableView [%d]", this->LevelCollectionTableView->GetInstanceID());

    this->StandardLevelDetailView = this->LevelDetailViewController->dyn__standardLevelDetailView();
    INFO("Acquired StandardLevelDetailView [%d]", this->StandardLevelDetailView->GetInstanceID());

    this->BeatmapCharacteristicSelectionViewController = this->StandardLevelDetailView->dyn__beatmapCharacteristicSegmentedControlController();
    INFO("Acquired BeatmapCharacteristicSegmentedControlController [%d]", this->BeatmapCharacteristicSelectionViewController->GetInstanceID());

    this->AnnotatedBeatmapLevelCollectionsViewController = this->LevelFilteringNavigationController->dyn__annotatedBeatmapLevelCollectionsViewController();
    INFO("Acquired AnnotatedBeatmapLevelCollectionsViewController from LevelFilteringNavigationController [%d]", this->AnnotatedBeatmapLevelCollectionsViewController->GetInstanceID());

    this->LevelCategoryViewController = this->LevelFilteringNavigationController->dyn__selectLevelCategoryViewController();
    INFO("Acquired LevelCategoryViewController from LevelFilteringNavigationController [%d]", this->LevelCategoryViewController->GetInstanceID());
}

void PlaylistEditor::RegistEvent()
{
    if (!this->LevelCategoryViewController->dyn_didSelectLevelCategoryEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event LevelCategoryViewController->dyn_didSelectLevelCategoryEvent()");
        std::function<void(GlobalNamespace::SelectLevelCategoryViewController*, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory)> didSelectLevelCategoryEventFun = [this] (GlobalNamespace::SelectLevelCategoryViewController*, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory) {
            INFO("LevelCategoryViewController SelectLevelCategoryEvent");
            this->AdjustUI();
        };
        this->LevelCategoryViewController->add_didSelectLevelCategoryEvent(Utils::MakeDelegate<System::Action_2<GlobalNamespace::SelectLevelCategoryViewController*, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory>*>(didSelectLevelCategoryEventFun));
    }

    if (!this->LevelCollectionNavigationController->dyn_didSelectLevelPackEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event LevelCollectionNavigationController->dyn_didSelectLevelPackEvent()");
        std::function<void(GlobalNamespace::LevelCollectionNavigationController*, GlobalNamespace::IBeatmapLevelPack*)> didSelectLevelPackEventFun = [this] (GlobalNamespace::LevelCollectionNavigationController*, GlobalNamespace::IBeatmapLevelPack*) {
            INFO("LevelCollectionNavigationController SelectLevelPackEvent"); // select to level list header
            this->AdjustUI();
        };
        this->LevelCollectionNavigationController->add_didSelectLevelPackEvent(Utils::MakeDelegate<System::Action_2<GlobalNamespace::LevelCollectionNavigationController*, GlobalNamespace::IBeatmapLevelPack*>*>(didSelectLevelPackEventFun));
    }

    if (!this->AnnotatedBeatmapLevelCollectionsViewController->dyn_didOpenBeatmapLevelCollectionsEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event AnnotatedBeatmapLevelCollectionsViewController->dyn_didOpenBeatmapLevelCollectionsEvent()");
        std::function<void()> didOpenBeatmapLevelCollectionsEventFun = [this] () {
            INFO("AnnotatedBeatmapLevelCollectionsViewController OpenBeatmapLevelCollectionsEvent");
            this->AdjustUI();
        };
        this->AnnotatedBeatmapLevelCollectionsViewController->add_didOpenBeatmapLevelCollectionsEvent(Utils::MakeDelegate<System::Action*>(didOpenBeatmapLevelCollectionsEventFun));
    }

    if (!this->AnnotatedBeatmapLevelCollectionsViewController->dyn_didSelectAnnotatedBeatmapLevelCollectionEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event AnnotatedBeatmapLevelCollectionsViewController->dyn_didSelectAnnotatedBeatmapLevelCollectionEvent()");
        std::function<void(GlobalNamespace::IAnnotatedBeatmapLevelCollection*)> didSelectAnnotatedBeatmapLevelCollectionEventFun = [this] (GlobalNamespace::IAnnotatedBeatmapLevelCollection*) {
            INFO("AnnotatedBeatmapLevelCollectionsViewController SelectAnnotatedBeatmapLevelCollectionEvent");
            this->AdjustUI();
        };
        this->AnnotatedBeatmapLevelCollectionsViewController->add_didSelectAnnotatedBeatmapLevelCollectionEvent(Utils::MakeDelegate<System::Action_1<GlobalNamespace::IAnnotatedBeatmapLevelCollection*>*>(didSelectAnnotatedBeatmapLevelCollectionEventFun));
    }
}

bool PlaylistEditor::IsSelectedSoloOrPartyPlay()
{
    return this->FlowCoordinator &&
           (il2cpp_utils::try_cast<GlobalNamespace::SoloFreePlayFlowCoordinator>(this->FlowCoordinator) ||
            il2cpp_utils::try_cast<GlobalNamespace::PartyFreePlayFlowCoordinator>(this->FlowCoordinator)) ? true : false;
}

bool PlaylistEditor::IsSelectedCustomCategory()
{
    return this->IsSelectedSoloOrPartyPlay() &&
           this->LevelFilteringNavigationController &&
           (GlobalNamespace::SelectLevelCategoryViewController::LevelCategory::CustomSongs == this->LevelFilteringNavigationController->get_selectedLevelCategory());
}

bool PlaylistEditor::IsSelectedCustomPack()
{
    return this->IsSelectedCustomCategory() &&
           this->LevelCollectionNavigationController &&
           this->LevelCollectionNavigationController->dyn__levelPack() &&
           CustomLevelID != this->LevelCollectionNavigationController->dyn__levelPack()->get_packID();
}

bool PlaylistEditor::IsSelectedCustomLevel()
{
    return this->LevelCollectionTableView &&
           this->LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel() &&
           il2cpp_functions::class_is_assignable_from(classof(GlobalNamespace::CustomPreviewBeatmapLevel*),
                                                      il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(this->LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel())));
}

GlobalNamespace::CustomPreviewBeatmapLevel *PlaylistEditor::GetSelectedCustomPreviewBeatmapLevel()
{
    return (this->LevelCollectionTableView && this->LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel()) ?
            reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(this->LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel()) : nullptr;
}

const std::string PlaylistEditor::GetSelectedPackID()
{
    return this->IsSelectedCustomPack() ? this->LevelCollectionNavigationController->dyn__levelPack()->get_packID() : "";
}

void PlaylistEditor::CreateListActionButton()
{
    if (!this->createListButton)
        this->createListButton = CreateIconButton("CreateListButton", this->LevelFilteringNavigationController->get_transform(), "PracticeButton",
                                            UnityEngine::Vector2(69.0f, -3.0f), UnityEngine::Vector2(10.0f, 7.0f), [this] () {
            if (!this->createListInput) {
                auto screenContainer = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Transform*>().First([] (auto x) {
                    return x->get_name()->Equals("ScreenContainer");
                });
                this->createListInput = CreateStringInput(screenContainer->get_transform(), "Ente new playlist name", "",
                                                    UnityEngine::Vector2(55.0f, -17.0f), 50.0f, [this] (StringW value) {
                                    INFO("Enter %s", std::string(value).c_str());
                                    // validate
                                    auto annotatedBeatmapLevelCollections = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
                                    for (int i = 0; i < annotatedBeatmapLevelCollections.Length(); i++) {
                                        std::string selectedPackName = annotatedBeatmapLevelCollections[i]->get_collectionName();
                                        if (value != selectedPackName)
                                            continue;
                                        Toast::GetInstance()->ShowMessage("List already exist");
                                        return;
                                    }
                                    if (CreateFile(value)) {
                                        this->RefreshAndStayList(SCROLL_ACTION::SCROLL_STAY);
                                        Toast::GetInstance()->ShowMessage("Create new list");
                                        this->createListInput->SetText("");
                                    }
                                    this->createListInput->get_gameObject()->set_active(false);
                });
                this->createListInput->get_gameObject()->set_active(true);
                return;
            }
            this->createListInput->get_gameObject()->set_active(!this->createListInput->get_gameObject()->get_active());
        }, FileToSprite("InsertIcon"), "Create List");

    if (!this->deleteListButton)
        this->deleteListButton = new DoubleClickIconButton("DeleteListButton", this->LevelFilteringNavigationController->get_transform(), "PracticeButton",
                                                                     UnityEngine::Vector2(69.0f, 3.0f), UnityEngine::Vector2(10.0f, 7.0f), [this] () {
                                if (DeleteFile(GetPlaylistPath(this->GetSelectedPackID()))) {
                                    Toast::GetInstance()->ShowMessage("Delete selected list");
                                    this->RefreshAndStayList(SCROLL_ACTION::NO_STAY);
                                }
        }, FileToSprite("DeleteIcon"), "Delete List");
}

void PlaylistEditor::ResetUI()
{
    if (this->deleteButton)
        this->deleteButton->ResetUI();
    if (this->deleteAndRemoveButton)
        this->deleteAndRemoveButton->ResetUI();
    if (this->moveUpButton)
        this->moveUpButton->set_interactable(true);
    if (this->moveUpButtonImageView)
        this->moveUpButtonImageView->set_color(UnityEngine::Color::get_white());
    if (this->moveDownButton)
        this->moveDownButton->set_interactable(true);
    if (this->moveDownButtonImageView)
        this->moveDownButtonImageView->set_color(UnityEngine::Color::get_white());
    if (this->listModal)
        this->listModal->get_gameObject()->set_active(false);
    if (this->moveDownButton)
        this->moveDownButton->set_interactable(true);
    if (this->createListInput)
        this->createListInput->get_gameObject()->set_active(false);
    if (this->deleteListButton) {
        this->deleteListButton->SetInteractable(true);
    }
}

void PlaylistEditor::CreateSongActionButton() {
    if (deleteButton)
        return;
    auto parent = this->StandardLevelDetailView->get_practiceButton()->get_transform()->get_parent();
    auto deleteButtonTransform = parent->FindChild("DeleteLevelButton");
    if (!deleteButtonTransform)
        return;

    // rewrite delete button
    auto btn = deleteButtonTransform->get_gameObject()->GetComponent<UnityEngine::UI::Button*>();
    this->deleteButton = new DoubleClickIconButton(btn, [this] () {
        GlobalNamespace::CustomPreviewBeatmapLevel *selectedlevel = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(this->LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel());
        RuntimeSongLoader::API::DeleteSong(std::string(selectedlevel->get_customLevelPath()), [this] {
            Toast::GetInstance()->ShowMessage("Delete song");
            this->RefreshAndStayList(this->IsSelectedCustomPack() ? SCROLL_ACTION::SCROLL_REMOVE_STAY : SCROLL_ACTION::SCROLL_STAY);
        });
    });

    auto posX = -22.5f;
    this->deleteAndRemoveButton = new DoubleClickIconButton("DeleteAndRemoveFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(20.0f,7.0f), [this] () {
            GlobalNamespace::CustomPreviewBeatmapLevel *selectedlevel = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel());
            RuntimeSongLoader::API::DeleteSong(std::string(selectedlevel->get_customLevelPath()), [this] {
                    if (UpdateFile(this->GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(this->GetSelectedPackID()), FILE_ACTION::ITEM_REMOVE))
                        Toast::GetInstance()->ShowMessage(this->IsSelectedCustomPack() ? "Delete song and remove from the list" : "Delete song and remove from all list" );
                    else
                        Toast::GetInstance()->ShowMessage("Delete song");
                    this->RefreshAndStayList(this->IsSelectedCustomPack() ? SCROLL_ACTION::SCROLL_REMOVE_STAY : SCROLL_ACTION::SCROLL_STAY);
                }
            );
        }, FileToSprite("DeleteAndRemoveIcon"), "Delete and Remove Song from List");
    this->deleteAndRemoveButton->operator->()->get_gameObject()->set_active(false);
    // deleteAndRemoveButton->get_transform()->SetAsLastSibling();

    posX += 15.0f + 1.25f ;
    this->removeButton = CreateIconButton("RemoveFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (UpdateFile(this->GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(this->GetSelectedPackID()), FILE_ACTION::ITEM_REMOVE)) {
                Toast::GetInstance()->ShowMessage(this->IsSelectedCustomPack() ? "Remove song from the list" : "Remove song from all list");
                this->RefreshAndStayList(this->IsSelectedCustomPack() ? SCROLL_ACTION::SCROLL_REMOVE_STAY : SCROLL_ACTION::SCROLL_STAY);
            } else
                Toast::GetInstance()->ShowMessage("Song isn't in any list");
        }, FileToSprite("RemoveIcon"), "Remove Song from List");
    this->removeButton->get_gameObject()->set_active(false);
    this->removeButton->get_transform()->SetAsLastSibling();

    posX += 10.0f + 1.25f;
    this->moveUpButton = CreateIconButton("MoveUpFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (UpdateFile(this->GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(this->GetSelectedPackID()), FILE_ACTION::ITEM_MOVE_UP)) {
                Toast::GetInstance()->ShowMessage("Move up song");
                this->RefreshAndStayList(SCROLL_ACTION::SCROLL_MOVE_UP);
            }
        }, FileToSprite("MoveUpIcon"), "Move Up Song from List");
    this->moveUpButton->get_gameObject()->set_active(false);
    this->moveUpButtonImageView = this->moveUpButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([&] (auto x) -> bool {
        return "Icon" == x->get_name();
    });
    this->moveUpButton->get_transform()->SetAsLastSibling();

    posX += 10.0f + 1.25f;
    this->moveDownButton = CreateIconButton("MoveDownFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (UpdateFile(this->GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(this->GetSelectedPackID()), FILE_ACTION::ITEM_MOVE_DOWN)) {
                Toast::GetInstance()->ShowMessage("Move down song");
                this->RefreshAndStayList(SCROLL_ACTION::SCROLL_MOVE_DOWN);
            }
        }, FileToSprite("MoveDownIcon"), "Move Down Song from List");
    this->moveDownButton->get_gameObject()->set_active(false);
    this->moveDownButtonImageView = this->moveDownButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([&] (auto x) -> bool {
        return "Icon" == x->get_name();
    });
    this->moveDownButton->get_transform()->SetAsLastSibling();

    posX += 10.0f + 1.25f;
    this->insertButton = CreateIconButton("InsertFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (this->listModal && this->listModal->get_gameObject()->get_active()) {
                this->listModal->get_gameObject()->set_active(false);
                // not knowning how to refresh container, so destroy and recreate every time
                for (auto item : listModalItem) {
                    UnityEngine::Object::Destroy(item);
                }
                this->listModalItem.clear();
                UnityEngine::Object::Destroy(this->listModal);
                UnityEngine::Object::Destroy(this->listContainer);
                this->listModal = nullptr;
                return;
            }

            auto screenContainer = QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Transform*>(), [](auto x) {
                return x->get_name()->Equals("ScreenContainer");
            });
            this->listModal = QuestUI::BeatSaberUI::CreateModal(screenContainer->get_transform(),
                UnityEngine::Vector2(30.0f, 25.0f), UnityEngine::Vector2(posX + 84.0f, -25.0f), nullptr);
            this->listContainer = QuestUI::BeatSaberUI::CreateScrollableModalContainer(this->listModal);
            this->listModal->get_transform()->set_localScale({0.75, 0.75, 1});

            auto annotatedBeatmapLevelCollections = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
            for (int i = 0; i < annotatedBeatmapLevelCollections.Length(); i++) {
                std::string selectedPackName = annotatedBeatmapLevelCollections[i]->get_collectionName();
                if (CustomLevelName == selectedPackName)
                    continue;
                this->listModalItem.push_back(QuestUI::BeatSaberUI::CreateClickableText(this->listContainer->get_transform(), selectedPackName, false, [this, selectedPackName] () {
                    std::string selectedPackId = CustomLevelPackPrefixID + selectedPackName;
                    this->listModal->get_gameObject()->set_active(false);
                    this->insertButton->set_interactable(true);
                    if (UpdateFile(this->GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(this->GetSelectedPackID()), FILE_ACTION::ITEM_INSERT, GetPlaylistPath(selectedPackId))) {
                        Toast::GetInstance()->ShowMessage("Insert song to selected list");
                        this->RefreshAndStayList(SCROLL_ACTION::SCROLL_STAY);
                    }
                }));
            }
            this->listModal->get_gameObject()->set_active(true);
        }, FileToSprite("InsertIcon"), "Insert to List");
    this->insertButton->get_gameObject()->set_active(false);
    this->insertButton->get_transform()->SetAsLastSibling();
}

void PlaylistEditor::AdjustUI(const bool forceDisable) // use forceDisable, casue don't know how to decide if now at main menu
{
    bool atCustomCategory = this->IsSelectedCustomCategory();
    bool atCustomPack = this->IsSelectedCustomPack();
    bool atCustomLevel = this->IsSelectedCustomLevel();
    // INFO("playlistEditor->AdjustUI: %s%s%s", atCustomCategory ? "atCustomCategory " : "", atCustomPack ? "atCustomPack " : "", atCustomLevel ? "atCustomLevel " : "");
    this->ResetUI();
    // if (deleteButton) {  // manage by songloader
    //     deleteButton->get_gameObject()->set_active(true);
    // }
    if (this->deleteAndRemoveButton)
        this->deleteAndRemoveButton->operator->()->get_gameObject()->set_active(!forceDisable && atCustomLevel);
    if (this->removeButton)
        this->removeButton->get_gameObject()->set_active(!forceDisable && atCustomLevel);
    if (this->insertButton)
        this->insertButton->get_gameObject()->set_active(!forceDisable && atCustomLevel);

    if (this->moveUpButton) {
        this->moveUpButton->get_gameObject()->set_active(!forceDisable && atCustomLevel);
        if (!atCustomPack) {
            this->moveUpButton->set_interactable(false);
            if (this->moveUpButtonImageView)
                this->moveUpButtonImageView->set_color(UnityEngine::Color::get_gray());
        }
    }
    if (this->moveDownButton) {
        this->moveDownButton->get_gameObject()->set_active(!forceDisable && atCustomLevel);
        if (!atCustomPack) {
            this->moveDownButton->set_interactable(false);
            if (this->moveDownButtonImageView)
                this->moveDownButtonImageView->set_color(UnityEngine::Color::get_gray());
        }
    }
    if (this->createListButton)
        this->createListButton->get_gameObject()->set_active(!forceDisable && atCustomCategory);
    if (this->deleteListButton) {
        this->deleteListButton->operator->()->get_gameObject()->set_active(!forceDisable && atCustomCategory);
        if (!atCustomPack)
            this->deleteListButton->SetInteractable(false);
    }
}

void PlaylistEditor::RefreshAndStayList(const SCROLL_ACTION act)
{
    const auto lastCollectionName = this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection() ?
                                    this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_collectionName() : "";
    const auto lastScrollPos = this->LevelCollectionTableView->dyn__tableView()->get_scrollView()->dyn__destinationPos();
    auto nextScrollPos = lastScrollPos;
    const int selectedRow = this->LevelCollectionTableView->dyn__selectedRow();
    int nextSelectedRow;
    const float rowHeight = this->LevelCollectionTableView->CellSize();
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
            break;
        case SCROLL_REMOVE_STAY:
            nextSelectedRow = (selectedRow < this->LevelCollectionTableView->NumberOfCells() - 1) ? selectedRow : this->LevelCollectionTableView->NumberOfCells() - 2;
            break;
        default:
            break;
    }

    RuntimeSongLoader::API::RefreshSongs(true,
    [this, lastScrollPos, nextScrollPos, nextSelectedRow, lastCollectionName, act] (const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&) {
        INFO("Success refresh song");
        if (NO_STAY == act)
            return;
        if ("" != lastCollectionName) {
            // select level collection
            auto annotatedBeatmapLevelCollections = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
            for (int i = 0; i < annotatedBeatmapLevelCollections.Length(); i++)
                if (annotatedBeatmapLevelCollections[i]->get_collectionName()->Equals(lastCollectionName)) {
                    this->AnnotatedBeatmapLevelCollectionsViewController->SetData(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections(), i, false);
                    break;
                }
        }

        // select level
        // this->LevelCollectionTableView->SelectLevel(nextPreviewBeatmapLevels); // this will jump to center
        this->LevelCollectionTableView->dyn__tableView()->SelectCellWithIdx(nextSelectedRow, true); // select but scroll at head
        this->LevelCollectionTableView->dyn__tableView()->get_scrollView()->ScrollTo(lastScrollPos, false);
        if (nextScrollPos != lastScrollPos)
            this->LevelCollectionTableView->dyn__tableView()->get_scrollView()->ScrollTo(nextScrollPos, true);
    });
}

}