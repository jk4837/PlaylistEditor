#include "CustomTypes/PlaylistEditor.hpp"

#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsGridView.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/GridView.hpp"
#include "GlobalNamespace/IAnnotatedBeatmapLevelCollection.hpp"
#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
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
    ReloadPlaylistPath();
    this->init = true;
}

void PlaylistEditor::AcquiredObject()
{
    auto LevelSelectionFlowCoordinator = il2cpp_utils::cast<GlobalNamespace::LevelSelectionFlowCoordinator>(this->FlowCoordinator);
    // gather flow coordinator elements
    auto LevelSelectionNavigationController = LevelSelectionFlowCoordinator->dyn_levelSelectionNavigationController();
    INFO("Acquired LevelSelectionNavigationController [%d]", LevelSelectionNavigationController->GetInstanceID());

    this->LevelSearchViewController = LevelSelectionFlowCoordinator->dyn__levelSearchViewController();
    INFO("Acquired LevelSearchViewController [%d]", this->LevelSearchViewController->GetInstanceID());

    this->LevelFilteringNavigationController = LevelSelectionNavigationController->dyn__levelFilteringNavigationController();
    INFO("Acquired LevelFilteringNavigationController [%d]", this->LevelFilteringNavigationController->GetInstanceID());

    this->LevelCollectionNavigationController = LevelSelectionNavigationController->dyn__levelCollectionNavigationController();
    INFO("Acquired LevelCollectionNavigationController [%d]", this->LevelCollectionNavigationController->GetInstanceID());

    auto LevelCollectionViewController = this->LevelCollectionNavigationController->dyn__levelCollectionViewController();
    INFO("Acquired LevelCollectionViewController [%d]", LevelCollectionViewController->GetInstanceID());

    auto LevelDetailViewController = this->LevelCollectionNavigationController->dyn__levelDetailViewController();
    INFO("Acquired LevelDetailViewController [%d]", LevelDetailViewController->GetInstanceID());

    this->LevelCollectionTableView = LevelCollectionViewController->dyn__levelCollectionTableView();
    INFO("Acquired LevelPackLevelsTableView [%d]", this->LevelCollectionTableView->GetInstanceID());

    this->StandardLevelDetailView = LevelDetailViewController->dyn__standardLevelDetailView();
    INFO("Acquired StandardLevelDetailView [%d]", this->StandardLevelDetailView->GetInstanceID());

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
        this->LevelCategoryViewController->add_didSelectLevelCategoryEvent(
            Utils::MakeDelegate<System::Action_2<GlobalNamespace::SelectLevelCategoryViewController*, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory>*>(didSelectLevelCategoryEventFun));
    }

    if (!this->LevelCollectionNavigationController->dyn_didSelectLevelPackEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event LevelCollectionNavigationController->dyn_didSelectLevelPackEvent()");
        std::function<void(GlobalNamespace::LevelCollectionNavigationController*, GlobalNamespace::IBeatmapLevelPack*)> didSelectLevelPackEventFun = [this] (GlobalNamespace::LevelCollectionNavigationController*, GlobalNamespace::IBeatmapLevelPack*) {
            INFO("LevelCollectionNavigationController SelectLevelPackEvent"); // select to level list header
            this->AdjustUI();
        };
        this->LevelCollectionNavigationController->add_didSelectLevelPackEvent(
            Utils::MakeDelegate<System::Action_2<GlobalNamespace::LevelCollectionNavigationController*, GlobalNamespace::IBeatmapLevelPack*>*>(didSelectLevelPackEventFun));
    }

    if (!this->AnnotatedBeatmapLevelCollectionsViewController->dyn_didOpenBeatmapLevelCollectionsEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event AnnotatedBeatmapLevelCollectionsViewController->dyn_didOpenBeatmapLevelCollectionsEvent()");
        std::function<void()> didOpenBeatmapLevelCollectionsEventFun = [this] () {
            INFO("AnnotatedBeatmapLevelCollectionsViewController OpenBeatmapLevelCollectionsEvent");
            this->AdjustUI();
        };
        this->AnnotatedBeatmapLevelCollectionsViewController->add_didOpenBeatmapLevelCollectionsEvent(
            Utils::MakeDelegate<System::Action*>(didOpenBeatmapLevelCollectionsEventFun));
    }

    if (!this->AnnotatedBeatmapLevelCollectionsViewController->dyn_didSelectAnnotatedBeatmapLevelCollectionEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event AnnotatedBeatmapLevelCollectionsViewController->dyn_didSelectAnnotatedBeatmapLevelCollectionEvent()");
        std::function<void(GlobalNamespace::IAnnotatedBeatmapLevelCollection*)> didSelectAnnotatedBeatmapLevelCollectionEventFun = [this] (GlobalNamespace::IAnnotatedBeatmapLevelCollection*) {
            INFO("AnnotatedBeatmapLevelCollectionsViewController SelectAnnotatedBeatmapLevelCollectionEvent");
            this->AdjustUI();
        };
        this->AnnotatedBeatmapLevelCollectionsViewController->add_didSelectAnnotatedBeatmapLevelCollectionEvent(
            Utils::MakeDelegate<System::Action_1<GlobalNamespace::IAnnotatedBeatmapLevelCollection*>*>(didSelectAnnotatedBeatmapLevelCollectionEventFun));
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

int PlaylistEditor::GetSelectedCustomLevelIdx() // will regards pack header row
{
    if (!this->LevelCollectionTableView)
        return 0;
    return this->LevelCollectionTableView->dyn__selectedRow() -
           (this->LevelCollectionTableView->dyn__showLevelPackHeader() ? 1 : 0);
}

const std::string PlaylistEditor::GetSelectedPackID()
{
    return this->IsSelectedCustomPack() ? this->LevelCollectionNavigationController->dyn__levelPack()->get_packID() : "";
}

int PlaylistEditor::GetSelectedPackIdx()
{
    return this->IsSelectedCustomCategory() ? AnnotatedBeatmapLevelCollectionsViewController->get_selectedItemIndex() : 0;
}

void PlaylistEditor::CreateListActionButton()
{
    if (!this->init)
        return;
    if (!this->createListButton)
        this->createListButton = new IconButton("CreateListButton", this->LevelFilteringNavigationController->get_transform(), "PracticeButton",
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
                                                ReloadPlaylistPath();
                                                this->RefreshAndStayList(REFESH_TYPE::PACK_INSERT);
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
                                    if (DeleteFile(GetPlaylistPath(this->GetSelectedPackIdx(), this->GetSelectedPackID()))) {
                                        ReloadPlaylistPath();
                                        Toast::GetInstance()->ShowMessage("Delete selected list");
                                        this->RefreshAndStayList(REFESH_TYPE::PACK_DELETE);
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
        this->moveUpButton->ResetUI();
    if (this->moveDownButton)
        this->moveDownButton->ResetUI();
    if (this->listModal)
        this->listModal->get_gameObject()->set_active(false);
    if (this->moveDownButton)
        this->moveDownButton->ResetUI();
    if (this->createListInput)
        this->createListInput->get_gameObject()->set_active(false);
    if (this->deleteListButton) {
        this->deleteListButton->ResetUI();
    }
}

void PlaylistEditor::MoveUpSelectedSongInPack() {
    auto levels = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_beatmapLevelCollection()->get_beatmapLevels());
    auto levelIdx = this->GetSelectedCustomLevelIdx();
    std::swap(levels[levelIdx], levels[levelIdx-1]);
}

void PlaylistEditor::MoveDownSelectedSongInPack() {
    auto levels = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_beatmapLevelCollection()->get_beatmapLevels());
    auto levelIdx = this->GetSelectedCustomLevelIdx();
    std::swap(levels[levelIdx], levels[levelIdx+1]);
}

void PlaylistEditor::RemoveSelectedSongInPack() {
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> levels(this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_beatmapLevelCollection()->get_beatmapLevels());
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevels(levels.Length() - 1);
    INFO("RemoveSelectedSongInPack %s, %lu => %lu", std::string(this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_collectionName()).c_str(), levels.Length(), newLevels.Length());
    auto levelIdx = this->GetSelectedCustomLevelIdx();

    std::copy(levels.begin(), std::next(levels.begin(), levelIdx), newLevels.begin());
    std::copy(std::next(levels.begin(), levelIdx + 1), levels.end(), std::next(newLevels.begin(), levelIdx));

    ((GlobalNamespace::CustomBeatmapLevelCollection*) this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_beatmapLevelCollection())->dyn__customPreviewBeatmapLevels() = (System::Collections::Generic::IReadOnlyList_1<GlobalNamespace::CustomPreviewBeatmapLevel*>*) newLevels.convert();
    this->RemoveSelectedSongInTable();
}

void PlaylistEditor::RemoveSongsInPack(GlobalNamespace::IBeatmapLevelCollection *beatmapLevelCollection, const StringW &levelID) {
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> levels(beatmapLevelCollection->get_beatmapLevels());
    std::vector<size_t> levelIdxs;

    for (size_t i = 0; i < levels.Length(); i++)
    {
        if (levelID != levels[i]->get_levelID())
            continue;
        levelIdxs.push_back(i);
    }

    if (levelIdxs.empty())
        return;

    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevels(levels.Length() - levelIdxs.size());
    INFO("RemoveSongsInPack %lu => %lu", levels.Length(), newLevels.Length());

    std::copy(levels.begin(), std::next(levels.begin(), levelIdxs[0]), newLevels.begin());
    for (size_t i = 0; i < levelIdxs.size(); i++)
    {
        const auto start = std::next(levels.begin(), levelIdxs[i] + 1);
        const auto end = (i + 1 < levelIdxs.size()) ? std::next(levels.begin(), levelIdxs[i+1]) : levels.end();
        std::copy(start, end, std::next(newLevels.begin(), levelIdxs[i] - i));
    }

    ((GlobalNamespace::CustomBeatmapLevelCollection*) beatmapLevelCollection)->dyn__customPreviewBeatmapLevels() = (System::Collections::Generic::IReadOnlyList_1<GlobalNamespace::CustomPreviewBeatmapLevel*>*) newLevels.convert();
}

void PlaylistEditor::RemoveSelectedSongInTable() {
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> levels(this->LevelCollectionTableView->dyn__previewBeatmapLevels());
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevels(levels.Length() - 1);
    INFO("RemoveSelectedSongInTable %lu => %lu", levels.Length(), newLevels.Length());
    auto levelIdx = this->GetSelectedCustomLevelIdx();

    std::copy(levels.begin(), std::next(levels.begin(), levelIdx), newLevels.begin());
    std::copy(std::next(levels.begin(), levelIdx + 1), levels.end(), std::next(newLevels.begin(), levelIdx));

    this->LevelCollectionTableView->dyn__previewBeatmapLevels() = (System::Collections::Generic::IReadOnlyList_1<GlobalNamespace::IPreviewBeatmapLevel*>*) newLevels.convert();
}

void PlaylistEditor::InsertSelectedSongToTable() {
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> levels(this->LevelCollectionTableView->dyn__previewBeatmapLevels());
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevels(levels.Length() + 1);
    INFO("InsertSelectedSongToTable %lu => %lu", levels.Length(), newLevels.Length());
    auto levelIdx = this->GetSelectedCustomLevelIdx();

    std::copy(levels.begin(), levels.end(), newLevels.begin());
    newLevels[newLevels.Length() - 1] = levels[levelIdx];

    this->LevelCollectionTableView->dyn__previewBeatmapLevels() = (System::Collections::Generic::IReadOnlyList_1<GlobalNamespace::IPreviewBeatmapLevel*>*) newLevels.convert();
}

void PlaylistEditor::RemoveSongsInFilterList(const StringW &levelID) {
    auto packs = this->LevelSearchViewController->dyn__beatmapLevelPacks();
    if (packs.Length() <= 0) {
        ERROR("No packs");
        return;
    }

    // use readonly list first to get correct length, but second time will crash, need to use array type
    auto levelsRO = (reinterpret_cast<::GlobalNamespace::IAnnotatedBeatmapLevelCollection*>(packs[0]))
            ->get_beatmapLevelCollection()->get_beatmapLevels();
    auto levels = listToArrayW((reinterpret_cast<::GlobalNamespace::IAnnotatedBeatmapLevelCollection*>(packs[0]))
            ->get_beatmapLevelCollection()->get_beatmapLevels());

    bool useArray = levels.Length() < 50000;
    auto levelsLen = useArray ? levels.Length() : getCount(levelsRO);

    int levelIdx = -1;
    for (size_t i = 0; i < levelsLen; i++)
        if ((useArray && levelID == levels[i]->get_levelID()) ||
            (!useArray && levelID == levelsRO->get_Item(i)->get_levelID())) {
            levelIdx = i;
            break;
        }
    if (0 > levelIdx)
        return;

    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevels(levelsLen - 1);
    INFO("RemoveSongsInFilterList %lu => %lu", levelsLen, newLevels.Length());
    if (useArray) {
        std::copy(levels.begin(), std::next(levels.begin(), levelIdx), newLevels.begin());
        std::copy(std::next(levels.begin(), levelIdx + 1), levels.end(), std::next(newLevels.begin(), levelIdx));
    } else {
        for (size_t i = 0, j = 0; i < levelsLen; i++) {
            if (levelIdx == i)
                continue;
            newLevels[j++] = levelsRO->get_Item(i);
        }
    }

    ((GlobalNamespace::CustomBeatmapLevelCollection*) (reinterpret_cast<::GlobalNamespace::IAnnotatedBeatmapLevelCollection*>(packs[0]))
                ->get_beatmapLevelCollection())->dyn__customPreviewBeatmapLevels() = (System::Collections::Generic::IReadOnlyList_1<GlobalNamespace::CustomPreviewBeatmapLevel*>*) newLevels.convert(); // update filter param 就有用
}

void PlaylistEditor::RemoveSelectedSongInAllPack(const bool includeCustomLevel) {
    bool isSelectedCustomLevel = this->IsSelectedCustomCategory() && (0 == this->GetSelectedPackIdx());
    auto annotatedBeatmapLevelCollections = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());

    for (int i = includeCustomLevel ? 0 : 1; i < annotatedBeatmapLevelCollections.Length(); i++)
        this->RemoveSongsInPack(annotatedBeatmapLevelCollections[i]->get_beatmapLevelCollection(), this->GetSelectedCustomPreviewBeatmapLevel()->get_levelID());

    this->RemoveSongsInFilterList(this->GetSelectedCustomPreviewBeatmapLevel()->get_levelID());

    if (this->IsSelectedCustomPack() || includeCustomLevel) {
        this->RemoveSelectedSongInTable();
    }
}

void PlaylistEditor::InsertSelectedSongToPack(const int collectionIdx) {
    auto annotatedBeatmapLevelCollections = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
    auto beatmapLevelCollection = annotatedBeatmapLevelCollections[collectionIdx]->get_beatmapLevelCollection();
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> levels(beatmapLevelCollection->get_beatmapLevels());
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevels(levels.Length() + 1);

    std::copy(levels.begin(), levels.end(), newLevels.begin());
    newLevels[newLevels.Length() - 1] = (GlobalNamespace::IPreviewBeatmapLevel*)this->GetSelectedCustomPreviewBeatmapLevel();

    ((GlobalNamespace::CustomBeatmapLevelCollection*) beatmapLevelCollection)->dyn__customPreviewBeatmapLevels() = (System::Collections::Generic::IReadOnlyList_1<GlobalNamespace::CustomPreviewBeatmapLevel*>*) newLevels.convert();

    // process cover image if not default image
    if (annotatedBeatmapLevelCollections[0]->get_coverImage() != annotatedBeatmapLevelCollections[collectionIdx]->get_coverImage())
        return;
    auto image = this->GetSelectedCustomPreviewBeatmapLevel()->dyn__coverImage();
    reinterpret_cast<::GlobalNamespace::CustomBeatmapLevelPack*>(annotatedBeatmapLevelCollections[collectionIdx])->dyn_$smallCoverImage$k__BackingField() = image;
    reinterpret_cast<::GlobalNamespace::CustomBeatmapLevelPack*>(annotatedBeatmapLevelCollections[collectionIdx])->dyn_$coverImage$k__BackingField() = image;
    this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollectionsGridView()->dyn__gridView()->ReloadData();
}

void PlaylistEditor::CreateSongActionButton() {
    if (!this->init)
        return;
    if (this->deleteButton)
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
            this->RemoveSelectedSongInAllPack(true);
            this->RefreshAndStayList(REFESH_TYPE::SONG_STAY);
        });
    });

    // create new button
    auto posX = -22.5f;
    this->deleteAndRemoveButton = new DoubleClickIconButton("DeleteAndRemoveFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(20.0f,7.0f), [this] () {
            GlobalNamespace::CustomPreviewBeatmapLevel *selectedlevel = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel());
            RuntimeSongLoader::API::DeleteSong(std::string(selectedlevel->get_customLevelPath()), [this] {
                    if (UpdateFile(this->GetSelectedCustomLevelIdx(), this->GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(this->GetSelectedPackIdx(), this->GetSelectedPackID()), FILE_ACTION::ITEM_REMOVE)) {
                        Toast::GetInstance()->ShowMessage(this->IsSelectedCustomPack() ? "Delete song and remove from the list" : "Delete song and remove from all list" );
                        this->RemoveSelectedSongInAllPack(true);
                    } else
                        Toast::GetInstance()->ShowMessage("Delete song");
                    this->RefreshAndStayList(REFESH_TYPE::SONG_STAY);
                }
            );
        }, FileToSprite("DeleteAndRemoveIcon"), "Delete and Remove Song from List");
    this->deleteAndRemoveButton->SetActive(false);

    posX += 15.0f + 1.25f ;
    this->removeButton = new IconButton("RemoveFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (UpdateFile(this->GetSelectedCustomLevelIdx(), this->GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(this->GetSelectedPackIdx(), this->GetSelectedPackID()), FILE_ACTION::ITEM_REMOVE)) {
                Toast::GetInstance()->ShowMessage(this->IsSelectedCustomPack() ? "Remove song from the list" : "Remove song from all list");
                if (this->IsSelectedCustomPack()) {
                    this->RemoveSelectedSongInPack();
                    this->RefreshAndStayList(REFESH_TYPE::SONG_STAY);
                } else
                    this->RemoveSelectedSongInAllPack(false);
            } else
                Toast::GetInstance()->ShowMessage("Song isn't in any list");
        }, FileToSprite("RemoveIcon"), "Remove Song from List");
    this->removeButton->SetActive(false);

    posX += 10.0f + 1.25f;
    this->moveUpButton = new IconButton("MoveUpFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (UpdateFile(this->GetSelectedCustomLevelIdx(), this->GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(this->GetSelectedPackIdx(), this->GetSelectedPackID()), FILE_ACTION::ITEM_MOVE_UP)) {
                Toast::GetInstance()->ShowMessage("Move up song");
                this->MoveUpSelectedSongInPack();
                this->RefreshAndStayList(REFESH_TYPE::SONG_MOVE_UP);
            } else
                Toast::GetInstance()->ShowMessage("Already on top");
        }, FileToSprite("MoveUpIcon"), "Move Up Song from List");
    this->moveUpButton->SetActive(false);

    posX += 10.0f + 1.25f;
    this->moveDownButton = new IconButton("MoveDownFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {

            if (UpdateFile(this->GetSelectedCustomLevelIdx(), this->GetSelectedCustomPreviewBeatmapLevel(), GetPlaylistPath(this->GetSelectedPackIdx(), this->GetSelectedPackID()), FILE_ACTION::ITEM_MOVE_DOWN)) {
                Toast::GetInstance()->ShowMessage("Move down song");
                this->MoveDownSelectedSongInPack();
                this->RefreshAndStayList(REFESH_TYPE::SONG_MOVE_DOWN);
            } else
                Toast::GetInstance()->ShowMessage("Already on bottom");
        }, FileToSprite("MoveDownIcon"), "Move Down Song from List");
    this->moveDownButton->SetActive(false);

    posX += 10.0f + 1.25f;
    this->insertButton = new IconButton("InsertFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
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
                this->listModalItem.push_back(QuestUI::BeatSaberUI::CreateClickableText(
                                              this->listContainer->get_transform(), selectedPackName, false, [this, i, selectedPackName] () {
                    std::string selectedPackId = CustomLevelPackPrefixID + selectedPackName;
                    this->listModal->get_gameObject()->set_active(false);
                    this->insertButton->SetInteractable(true);
                    if (UpdateFile(this->GetSelectedCustomLevelIdx(), this->GetSelectedCustomPreviewBeatmapLevel(),
                                   GetPlaylistPath(this->GetSelectedPackIdx(), this->GetSelectedPackID()), FILE_ACTION::ITEM_INSERT,
                                   GetPlaylistPath(i, selectedPackId))) {
                        Toast::GetInstance()->ShowMessage("Insert song to selected list");
                        this->RefreshAndStayList(SCROLL_ACTION::SCROLL_STAY);
                    }
                }));
            }
            this->listModal->get_gameObject()->set_active(true);
        }, FileToSprite("InsertIcon"), "Insert to List");
    this->insertButton->SetActive(false);
}

void PlaylistEditor::AdjustUI(const bool forceDisable) // use forceDisable, casue don't know how to decide if now at main menu
{
    if (!this->init)
        return;
    bool atCustomCategory = this->IsSelectedCustomCategory();
    bool atCustomPack = this->IsSelectedCustomPack();
    bool atCustomLevel = this->IsSelectedCustomLevel();
    // INFO("playlistEditor->AdjustUI: %s%s%s", atCustomCategory ? "atCustomCategory " : "", atCustomPack ? "atCustomPack " : "", atCustomLevel ? "atCustomLevel " : "");
    this->ResetUI();
    // if (deleteButton) {  // manage by songloader
    //     deleteButton->SetActive(true);
    // }
    if (this->deleteAndRemoveButton) {
        this->deleteAndRemoveButton->SetActive(!forceDisable && atCustomLevel);
        this->deleteAndRemoveButton->ChangeHoverHint(atCustomPack ? "Delete and Remove Song from This List" : "Delete and Remove Song from All List");
    }
    if (this->removeButton) {
        this->removeButton->SetActive(!forceDisable && atCustomLevel);
        this->removeButton->ChangeHoverHint(atCustomPack ? "Remove Song from This List" : "Remove Song from All List");
    }
    if (this->insertButton)
        this->insertButton->SetActive(!forceDisable && atCustomLevel);

    if (this->moveUpButton) {
        this->moveUpButton->SetActive(!forceDisable && atCustomLevel);
        if (!atCustomPack)
            this->moveUpButton->SetInteractable(false);
    }
    if (this->moveDownButton) {
        this->moveDownButton->SetActive(!forceDisable && atCustomLevel);
        if (!atCustomPack)
            this->moveDownButton->SetInteractable(false);
    }
    if (this->createListButton)
        this->createListButton->SetActive(!forceDisable && atCustomCategory);
    if (this->deleteListButton) {
        this->deleteListButton->SetActive(!forceDisable && atCustomCategory);
        if (!atCustomPack)
            this->deleteListButton->SetInteractable(false);
    }
}

void PlaylistEditor::RefreshAndStayList(const REFESH_TYPE act)
{
    const auto lastScrollPos = this->LevelCollectionTableView->dyn__tableView()->get_scrollView()->dyn__destinationPos();
    auto nextScrollPos = lastScrollPos;
    const int selectedRow = this->LevelCollectionTableView->dyn__selectedRow();
    auto nextSelectedRow = selectedRow;

    if (PACK_INSERT == act || PACK_DELETE == act) {
        const auto lastCollectionIdx = this->GetSelectedPackIdx();
        const auto lastCollectionName = this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection() ?
                                        this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_collectionName() : "";
        RuntimeSongLoader::API::RefreshPacks(true);
        if ("" != lastCollectionName) {
            INFO("select collection %d %s", lastCollectionIdx, std::string(lastCollectionName).c_str());
            // select level collection
            auto annotatedBeatmapLevelCollections = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
            for (int i = lastCollectionIdx; i < annotatedBeatmapLevelCollections.Length(); i++) // index may move back when new list created
            {
                if (annotatedBeatmapLevelCollections[i]->get_collectionName()->Equals(lastCollectionName)) {
                    this->AnnotatedBeatmapLevelCollectionsViewController->SetData(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections(), i, false);
                    break;
                }
            }
        }
    } else { // song action
        const float rowHeight = this->LevelCollectionTableView->CellSize();

        this->LevelCollectionTableView->dyn__tableView()->ReloadData();
        if (SONG_MOVE_DOWN == act) {
            nextSelectedRow = selectedRow + 1;
            if (nextSelectedRow*rowHeight > (lastScrollPos + 6*rowHeight))
                nextScrollPos = lastScrollPos + rowHeight;
        } else if (SONG_MOVE_UP == act) {
            nextSelectedRow = selectedRow - 1;
            if (nextSelectedRow*rowHeight < lastScrollPos)
                nextScrollPos = lastScrollPos - rowHeight;
        } else if (SONG_STAY == act) {
            if (selectedRow >= this->LevelCollectionTableView->NumberOfCells())
                nextSelectedRow = this->LevelCollectionTableView->NumberOfCells() - 1;
        }
    }

    INFO("select level %d %f %f", nextSelectedRow, lastScrollPos, nextScrollPos);
    // this->LevelCollectionTableView->SelectLevel(nextPreviewBeatmapLevels); // this will jump to center
    this->LevelCollectionTableView->dyn__tableView()->SelectCellWithIdx(nextSelectedRow, true); // select but scroll at head
    this->LevelCollectionTableView->dyn__tableView()->get_scrollView()->ScrollTo(lastScrollPos, false);
    if (nextScrollPos != lastScrollPos)
        this->LevelCollectionTableView->dyn__tableView()->get_scrollView()->ScrollTo(nextScrollPos, true);
}

}
