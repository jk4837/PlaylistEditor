#include "CustomTypes/PlaylistEditor.hpp"

#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsTableView.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSegmentedControlController.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapDifficultySegmentedControlController.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IAnnotatedBeatmapLevelCollection.hpp"
#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelPackDetailViewController.hpp"
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
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/RectTransform_Axis.hpp"
#include "songloader/shared/API.hpp"
#include "questui/shared/BeatSaberUI.hpp"

#include "CustomTypes/Toast.hpp"
#include "CustomTypes/Logging.hpp"
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
    this->fileUtils.ReloadPlaylistPath();
    this->init = true;
}

void PlaylistEditor::AcquiredObject()
{
    auto LevelSelectionFlowCoordinator = il2cpp_utils::cast<GlobalNamespace::LevelSelectionFlowCoordinator>(this->FlowCoordinator);
    // gather flow coordinator elements
    this->LevelSelectionNavigationController = LevelSelectionFlowCoordinator->dyn_levelSelectionNavigationController();
    INFO("Acquired LevelSelectionNavigationController [%d]", this->LevelSelectionNavigationController->GetInstanceID());

    this->LevelSearchViewController = LevelSelectionFlowCoordinator->dyn__levelSearchViewController();
    INFO("Acquired LevelSearchViewController [%d]", this->LevelSearchViewController->GetInstanceID());

    this->LevelFilteringNavigationController = this->LevelSelectionNavigationController->dyn__levelFilteringNavigationController();
    INFO("Acquired LevelFilteringNavigationController [%d]", this->LevelFilteringNavigationController->GetInstanceID());

    this->LevelCollectionNavigationController = this->LevelSelectionNavigationController->dyn__levelCollectionNavigationController();
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

    if (!this->AnnotatedBeatmapLevelCollectionsViewController->dyn_didSelectAnnotatedBeatmapLevelCollectionEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event AnnotatedBeatmapLevelCollectionsViewController->dyn_didSelectAnnotatedBeatmapLevelCollectionEvent()");
        std::function<void(GlobalNamespace::IAnnotatedBeatmapLevelCollection*)> didSelectAnnotatedBeatmapLevelCollectionEventFun = [this] (GlobalNamespace::IAnnotatedBeatmapLevelCollection*) {
            INFO("AnnotatedBeatmapLevelCollectionsViewController SelectAnnotatedBeatmapLevelCollectionEvent");
            this->AdjustUI();
        };
        this->AnnotatedBeatmapLevelCollectionsViewController->add_didSelectAnnotatedBeatmapLevelCollectionEvent(
            Utils::MakeDelegate<System::Action_1<GlobalNamespace::IAnnotatedBeatmapLevelCollection*>*>(didSelectAnnotatedBeatmapLevelCollectionEventFun));
    }

    if (!this->LevelSelectionNavigationController->dyn_didPressActionButtonEvent()->dyn_delegates()) { // event removed useless, always execute twice when out and in
        INFO("Add event LevelSelectionNavigationController->dyn_didPressActionButtonEvent()");
        std::function<void(GlobalNamespace::LevelSelectionNavigationController*)> didPressActionButtonEventFun = [this] (GlobalNamespace::LevelSelectionNavigationController*) {
            INFO("LevelSelectionNavigationController PressActionButtonEvent");
            if (!this->IsSelectedCustomLevel())
                return;
            if (!this->recordListButton || this->recordListButton->GetIsFirstState())
                return;

            this->recordPackIdx = this->FindPackIdx(this->recordPackName, this->recordPackIdx);
            if (this->recordPackIdx < 0) {
                ERROR("Can't find index of record list %s", this->recordPackName.c_str());
                this->recordPackName = "";
                this->recordListButton->SetIsFirstState(true);
                Toast::GetInstance()->ShowMessage("Stop record playing level to new list");
                return;
            }

            const std::string recordPackId = CustomLevelPackPrefixID + this->recordPackName;
            if (this->UpdateFileWithSelected(FILE_ACTION::ITEM_INSERT, this->recordPackIdx, recordPackId)) {
                this->InsertSelectedSongToPack(this->recordPackIdx);
                Toast::GetInstance()->ShowMessage("Insert song to record list");
            }
        };
        this->LevelSelectionNavigationController->add_didPressActionButtonEvent(
            Utils::MakeDelegate<System::Action_1<GlobalNamespace::LevelSelectionNavigationController*>*>(didPressActionButtonEventFun));
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
           CustomLevelID != to_utf8(csstrtostr(this->LevelCollectionNavigationController->dyn__levelPack()->get_packID()));
}

bool PlaylistEditor::IsSelectedCustomLevel()
{
    return this->GetSelectedCustomLevelIdx() >= 0 &&
           this->LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel() &&
           il2cpp_functions::class_is_assignable_from(classof(GlobalNamespace::CustomPreviewBeatmapLevel*),
                                                      il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(this->LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel())));
}

bool PlaylistEditor::IsSelectedCustomPackUsingDefaultCover()
{
    if (!this->IsSelectedCustomPack())
        return false;

    auto annotatedBeatmapLevelCollections = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
    return annotatedBeatmapLevelCollections[0]->get_coverImage() == annotatedBeatmapLevelCollections[this->GetSelectedPackIdx()]->get_coverImage();
}

GlobalNamespace::CustomPreviewBeatmapLevel *PlaylistEditor::GetSelectedCustomPreviewBeatmapLevel()
{
    return (this->LevelCollectionTableView && this->LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel()) ?
            reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(this->LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel()) : nullptr;
}

int PlaylistEditor::GetSelectedCustomLevelIdx() // will regards pack header row
{
    if (!this->LevelCollectionTableView)
        return -1;
    return this->LevelCollectionTableView->dyn__selectedRow() -
           (this->LevelCollectionTableView->dyn__showLevelPackHeader() ? 1 : 0);
}

const std::string PlaylistEditor::GetSelectedPackID()
{
    return this->IsSelectedCustomPack() ? to_utf8(csstrtostr(this->LevelCollectionNavigationController->dyn__levelPack()->get_packID())) : "";
}

int PlaylistEditor::GetSelectedPackIdx()
{
    return this->IsSelectedCustomCategory() ? AnnotatedBeatmapLevelCollectionsViewController->get_selectedItemIndex() : 0;
}

std::string PlaylistEditor::GetSelectedCharStr()
{
    return (this->StandardLevelDetailView &&
           this->StandardLevelDetailView->dyn__beatmapCharacteristicSegmentedControlController() &&
           this->StandardLevelDetailView->dyn__beatmapCharacteristicSegmentedControlController()->get_selectedBeatmapCharacteristic()) ?
           to_utf8(csstrtostr(this->StandardLevelDetailView->dyn__beatmapCharacteristicSegmentedControlController()->get_selectedBeatmapCharacteristic()->get_serializedName())) : "";
}

int PlaylistEditor::GetSelectedDiff()
{
    return (this->StandardLevelDetailView &&
           this->StandardLevelDetailView->dyn__beatmapDifficultySegmentedControlController()) ?
           int(this->StandardLevelDetailView->dyn__beatmapDifficultySegmentedControlController()->get_selectedDifficulty()) : 0;
}

std::string PlaylistEditor::GetSelectedPackDuration()
{
    float sum = 0;

    if (!this->LevelCollectionTableView || !this->LevelCollectionTableView->dyn__previewBeatmapLevels())
        return "0";

    auto beatmapLevels = listToArrayW(this->LevelCollectionTableView->dyn__previewBeatmapLevels());
    for (size_t i = 0; i < beatmapLevels.Length(); i++)
    {
        sum += beatmapLevels[i]->get_songDuration();
    }

    //  HH:MM:SS
    const int s = (int)sum % 60;
    const int m = (int)sum/60 % 60;
    const int h = (int)sum/60/60 % 60;
    return std::to_string(h) + ":" + (m < 10 ? "0" : "") + std::to_string(m) + ":" + (s < 10 ? "0" : "") + std::to_string(s);
}

int PlaylistEditor::FindPackIdx(const std::string &name, const int startIdx)
{
    int foundIdx = -1;
    if (name.empty())
        return foundIdx;

    auto annotatedBeatmapLevelCollections = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
    for (int i = 0; i < annotatedBeatmapLevelCollections.Length(); i++) {
        if (name != to_utf8(csstrtostr(annotatedBeatmapLevelCollections[i]->get_collectionName())))
            continue;
        foundIdx = i;
        if (i >= startIdx)
            break;
    }
    return foundIdx;
}

void PlaylistEditor::SelectLockCharDiff()
{
    if (!this->IsSelectedCustomPack() ||
        !this->StandardLevelDetailView->dyn__beatmapCharacteristicSegmentedControlController()->get_selectedBeatmapCharacteristic())
        return;

    const std::string path = this->fileUtils.GetPlaylistPath(this->GetSelectedPackIdx(), this->GetSelectedPackID());
    if (!fileUtils.FindSongCharDiff(this->GetSelectedCustomLevelIdx(), to_utf8(csstrtostr(this->GetSelectedCustomPreviewBeatmapLevel()->get_levelID())), path,
                                     this->selectedLockCharStr, this->selectedLockDiff))
        return;

    if (this->selectedLockCharStr.empty())
        return;

    const std::string preCharStr = this->GetSelectedCharStr();
    if (this->selectedLockCharStr != preCharStr) {
        auto charSOs = *(this->StandardLevelDetailView->dyn__beatmapCharacteristicSegmentedControlController()->dyn__beatmapCharacteristics()->dyn__items());
        for (size_t i = 0; i < charSOs.Length(); i++)
        {
            if (!charSOs[i]) // not knowing why it is
                continue;
            if (this->selectedLockCharStr != to_utf8(csstrtostr(charSOs[i]->get_serializedName())))
                continue;

            // select but not change difficultyBeatmapSet
            this->StandardLevelDetailView->dyn__beatmapCharacteristicSegmentedControlController()->SetData(
                this->difficultyBeatmapSets, charSOs[i]);
            // change difficultyBeatmapSet
            this->StandardLevelDetailView->HandleBeatmapCharacteristicSegmentedControlControllerDidSelectBeatmapCharacteristic(
                this->StandardLevelDetailView->dyn__beatmapCharacteristicSegmentedControlController(), charSOs[i]);
        }
    }

    const int preDiff = this->GetSelectedDiff();
    if (this->selectedLockDiff != preDiff && this->difficultyBeatmaps) {
        this->StandardLevelDetailView->dyn__beatmapDifficultySegmentedControlController()->SetData(this->difficultyBeatmaps, this->selectedLockDiff);
        this->StandardLevelDetailView->HandleBeatmapDifficultySegmentedControlControllerDidSelectDifficulty( // useless
            this->StandardLevelDetailView->dyn__beatmapDifficultySegmentedControlController(), this->selectedLockDiff);
    }
}

bool PlaylistEditor::UpdateFileWithSelected(const FILE_ACTION act)
{
    const std::string path = this->fileUtils.GetPlaylistPath(this->GetSelectedPackIdx(), this->GetSelectedPackID());
    if (ITEM_LOCK == act || ITEM_UNLOCK == act) {
        return this->fileUtils.UpdateSongLock(this->GetSelectedCustomLevelIdx(),
                                              to_utf8(csstrtostr(this->GetSelectedCustomPreviewBeatmapLevel()->get_levelID())),
                                              path,
                                              act, this->GetSelectedCharStr(), this->GetSelectedDiff());
    } else
        return this->fileUtils.UpdateFile(this->GetSelectedCustomLevelIdx(),
                                          this->GetSelectedCustomPreviewBeatmapLevel(),
                                          path,
                                          act);
}

bool PlaylistEditor::UpdateFileWithSelected(const FILE_ACTION act, const int selectedPackIdx, const std::string &selectedPackId)
{
    return this->fileUtils.UpdateFile(this->GetSelectedCustomLevelIdx(),
                                       this->GetSelectedCustomPreviewBeatmapLevel(),
                                       this->fileUtils.GetPlaylistPath(this->GetSelectedPackIdx(), this->GetSelectedPackID()),
                                       act,
                                       this->fileUtils.GetPlaylistPath(selectedPackIdx, selectedPackId), this->GetSelectedCharStr(), this->GetSelectedDiff());
}

void PlaylistEditor::SetSelectedCoverImage(const int collectionIdx, UnityEngine::Sprite *image)
{
    auto annotatedBeatmapLevelCollections = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());

    if (!image)
        image = annotatedBeatmapLevelCollections[0]->get_coverImage(); // default cover
    reinterpret_cast<::GlobalNamespace::CustomBeatmapLevelPack*>(annotatedBeatmapLevelCollections[collectionIdx])->dyn__coverImage() = image;
    this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollectionsTableView()->dyn__tableView()->ReloadData();

    if (this->IsSelectedCustomPack() && !this->IsSelectedCustomLevel()) // select pack header
        this->LevelCollectionNavigationController->dyn__levelPackDetailViewController()->dyn__packImage()->set_sprite(image);
}

bool PlaylistEditor::CreateList(const std::string &name)
{
    // validate
    auto annotatedBeatmapLevelCollections = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
    for (int i = 0; i < annotatedBeatmapLevelCollections.Length(); i++) {
        std::string selectedPackName = to_utf8(csstrtostr(annotatedBeatmapLevelCollections[i]->get_collectionName()));
        if (name != selectedPackName)
            continue;
        Toast::GetInstance()->ShowMessage("List already exist");
        return false;
    }

    if (!this->fileUtils.CreateFile(name))
        return false;

    this->fileUtils.ReloadPlaylistPath();
    this->RefreshAndStayList(REFESH_TYPE::PACK_INSERT);
    return true;
}

void PlaylistEditor::AdjustPackTableViewWidth() {
    auto tableView = this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollectionsTableView()->dyn__tableView();

    if (tableView->dyn__viewportTransform()->get_rect().get_width() > 78.0f) {
        tableView->ChangeRectSize(UnityEngine::RectTransform::Axis::_get_Horizontal(), 78.0f);
        tableView->get_transform()->Translate(-0.40f, 0, 0);
    }
}

void PlaylistEditor::CreateListActionButton()
{
    if (!this->init)
        return;

    this->AdjustPackTableViewWidth();

    if (!this->createListButton)
        this->createListButton = new IconButton("CreateListButton", this->LevelFilteringNavigationController->get_transform(), "PracticeButton",
                                                UnityEngine::Vector2(75.0f, -3.0f), UnityEngine::Vector2(7.0f, 7.0f), [this] () {
            if (!this->createListInput) {
                auto screenContainer = QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Transform*>(), [] (auto x) {
                    return x->get_name()->Equals(il2cpp_utils::newcsstr("ScreenContainer"));
                });
                this->createListInput = CreateStringInput(screenContainer->get_transform(), "Ente new playlist name", "",
                                                          UnityEngine::Vector2(63.0f, -15.0f), 50.0f, [this] (StringW value) {
                                            INFO("Enter %s", std::string(value).c_str());
                                            if (!CreateList(value))
                                                return;

                                            Toast::GetInstance()->ShowMessage("Create new list");
                                            this->lastInsertPackIdx = 0;
                                            this->lastInsertPackName = std::string(value);
                                            this->createListInput->get_gameObject()->set_active(false);
                                            this->createListInput->SetText(il2cpp_utils::newcsstr(""));
                });
                this->createListInput->get_gameObject()->set_active(true);
                return;
            }
            this->createListInput->get_gameObject()->set_active(!this->createListInput->get_gameObject()->get_active());
        }, FileToSprite("InsertIcon"), "Create List");

    if (!this->deleteListButton)
        this->deleteListButton = new DoubleClickIconButton("DeleteListButton", this->LevelFilteringNavigationController->get_transform(), "PracticeButton",
                                                           UnityEngine::Vector2(75.0f, 3.0f), UnityEngine::Vector2(7.0f, 7.0f), [this] () {
                                    if (this->fileUtils.DeleteFile(this->fileUtils.GetPlaylistPath(this->GetSelectedPackIdx(), this->GetSelectedPackID()))) {
                                        this->fileUtils.ReloadPlaylistPath();
                                        Toast::GetInstance()->ShowMessage("Delete selected list");
                                        this->RefreshAndStayList(REFESH_TYPE::PACK_DELETE);
                                    }
        }, FileToSprite("DeleteIcon"), "Delete List");

    if (!this->imageListButton)
        this->imageListButton = new TwoStateIconButton("ImageListButton", this->LevelFilteringNavigationController->get_transform(), "PracticeButton",
                                                           UnityEngine::Vector2(67.0f, 3.0f), UnityEngine::Vector2(7.0f, 7.0f),
                                    [this] () {
                                        if (this->fileUtils.SetCoverImage(this->fileUtils.GetPlaylistPath(this->GetSelectedPackIdx(), this->GetSelectedPackID()))) {
                                            this->SetSelectedCoverImage(this->GetSelectedPackIdx());
                                            Toast::GetInstance()->ShowMessage("Delete cover image");
                                        }
                                    }, FileToSprite("DeleteImageIcon"), "Delete Cover Image",
                                    [this] () {
                                        if (this->fileUtils.SetCoverImage(this->fileUtils.GetPlaylistPath(this->GetSelectedPackIdx(), this->GetSelectedPackID()), this->GetSelectedCustomPreviewBeatmapLevel())) {
                                            this->SetSelectedCoverImage(this->GetSelectedPackIdx(), this->GetSelectedCustomPreviewBeatmapLevel()->dyn__coverImage());
                                            Toast::GetInstance()->ShowMessage("Set cover image");
                                        }
                                    }, FileToSprite("AddImageIcon"), "Set Cover Image with Selected Level Image");

    if (!this->recordListButton)
        this->recordListButton = new TwoStateIconButton("RecordListButton", this->LevelFilteringNavigationController->get_transform(), "PracticeButton",
                                                           UnityEngine::Vector2(67.0f, -3.0f), UnityEngine::Vector2(7.0f, 7.0f),
                                    [this] () {
                                        if (!this->recordListInput) {
                                            auto screenContainer = QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Transform*>(), [] (auto x) {
                                                return x->get_name()->Equals(il2cpp_utils::newcsstr("ScreenContainer"));
                                            });
                                            this->recordListInput = CreateStringInput(screenContainer->get_transform(), "Ente new playlist name", "",
                                                                                    UnityEngine::Vector2(55.0f, -15.0f), 50.0f, [this] (StringW value) {
                                                INFO("Enter %s", std::string(value).c_str());
                                                if (!CreateList(value))
                                                    return;

                                                Toast::GetInstance()->ShowMessage("Create new list and start record");
                                                this->recordPackIdx = 0;
                                                this->recordPackName = std::string(value);
                                                this->recordListInput->get_gameObject()->set_active(false);
                                                this->recordListInput->SetText(il2cpp_utils::newcsstr(""));
                                                this->recordListButton->SetIsFirstState(false);
                                            });
                                            this->recordListInput->get_gameObject()->set_active(true);
                                            return;
                                        }
                                        this->recordListInput->get_gameObject()->set_active(!this->recordListInput->get_gameObject()->get_active());
                                    }, FileToSprite("StartRecordIcon"), "Start Record Playing Level to New List",
                                    [this] () {
                                        recordPackIdx = -1;
                                        recordPackName = "";
                                        this->recordListButton->SetIsFirstState(true);
                                        if(!this->IsSelectedCustomCategory())
                                            this->recordListButton->SetActive(false);
                                        Toast::GetInstance()->ShowMessage("Stop record playing level to new list");
                                    }, FileToSprite("StopRecordIcon"), "Stop Record Playing Level to New List",
                                    false);
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
        this->listModal->SetActive(false);
    if (this->moveDownButton)
        this->moveDownButton->ResetUI();
    if (this->lockButton)
        this->lockButton->ResetUI();
    if (this->createListInput)
        this->createListInput->get_gameObject()->set_active(false);
    if (this->deleteListButton) {
        this->deleteListButton->ResetUI();
    }
    if (this->imageListButton)
        this->imageListButton->ResetUI();
    if (this->recordListButton)
        this->recordListButton->ResetUI();
    if (this->recordListInput)
        this->recordListInput->get_gameObject()->set_active(false);
    if (this->packDurationText)
        this->packDurationText->get_gameObject()->set_active(false);
}

void PlaylistEditor::MoveUpSelectedSong() {
    auto levelsInPack = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_beatmapLevelCollection()->get_beatmapLevels());
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevelsInPack(levelsInPack.Length());
    auto levelIdx = this->GetSelectedCustomLevelIdx();

    std::copy(levelsInPack.begin(), levelsInPack.end(), newLevelsInPack.begin());
    std::swap(newLevelsInPack[levelIdx], newLevelsInPack[levelIdx-1]);
    ((GlobalNamespace::CustomBeatmapLevelCollection*) this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_beatmapLevelCollection())->dyn__customPreviewBeatmapLevels() = (Array<GlobalNamespace::CustomPreviewBeatmapLevel*>*) newLevelsInPack.convert();

    auto levelsInTable = listToArrayW(this->LevelCollectionTableView->dyn__previewBeatmapLevels());
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevelsInTable(levelsInTable.Length());

    std::copy(levelsInTable.begin(), levelsInTable.end(), newLevelsInTable.begin());
    std::swap(newLevelsInTable[levelIdx], newLevelsInTable[levelIdx-1]);
    this->LevelCollectionTableView->dyn__previewBeatmapLevels() = (Array<GlobalNamespace::IPreviewBeatmapLevel*>*) newLevelsInTable.convert();
}

void PlaylistEditor::MoveDownSelectedSong() {
    auto levelsInPack = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_beatmapLevelCollection()->get_beatmapLevels());
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevelsInPack(levelsInPack.Length());
    auto levelIdx = this->GetSelectedCustomLevelIdx();

    std::copy(levelsInPack.begin(), levelsInPack.end(), newLevelsInPack.begin());
    std::swap(newLevelsInPack[levelIdx], newLevelsInPack[levelIdx+1]);
    ((GlobalNamespace::CustomBeatmapLevelCollection*) this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_beatmapLevelCollection())->dyn__customPreviewBeatmapLevels() = (Array<GlobalNamespace::CustomPreviewBeatmapLevel*>*) newLevelsInPack.convert();

    auto levelsInTable = listToArrayW(this->LevelCollectionTableView->dyn__previewBeatmapLevels());
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevelsInTable(levelsInTable.Length());

    std::copy(levelsInTable.begin(), levelsInTable.end(), newLevelsInTable.begin());
    std::swap(newLevelsInTable[levelIdx], newLevelsInTable[levelIdx+1]);
    this->LevelCollectionTableView->dyn__previewBeatmapLevels() = (Array<GlobalNamespace::IPreviewBeatmapLevel*>*) newLevelsInTable.convert();
}

void PlaylistEditor::RemoveSelectedSongInPack() {
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> levels(this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_beatmapLevelCollection()->get_beatmapLevels());
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevels(levels.Length() - 1);
    INFO("RemoveSelectedSongInPack %s, %lu => %lu", to_utf8(csstrtostr(this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_collectionName())).c_str(), levels.Length(), newLevels.Length());
    auto levelIdx = this->GetSelectedCustomLevelIdx();

    std::copy(levels.begin(), std::next(levels.begin(), levelIdx), newLevels.begin());
    std::copy(std::next(levels.begin(), levelIdx + 1), levels.end(), std::next(newLevels.begin(), levelIdx));

    ((GlobalNamespace::CustomBeatmapLevelCollection*) this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_beatmapLevelCollection())->dyn__customPreviewBeatmapLevels() = (Array<GlobalNamespace::CustomPreviewBeatmapLevel*>*) newLevels.convert();
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

    ((GlobalNamespace::CustomBeatmapLevelCollection*) beatmapLevelCollection)->dyn__customPreviewBeatmapLevels() = (Array<GlobalNamespace::CustomPreviewBeatmapLevel*>*) newLevels.convert();
}

void PlaylistEditor::RemoveSelectedSongInTable() {
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> levels(this->LevelCollectionTableView->dyn__previewBeatmapLevels());
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevels(levels.Length() - 1);
    INFO("RemoveSelectedSongInTable %lu => %lu", levels.Length(), newLevels.Length());
    auto levelIdx = this->GetSelectedCustomLevelIdx();

    std::copy(levels.begin(), std::next(levels.begin(), levelIdx), newLevels.begin());
    std::copy(std::next(levels.begin(), levelIdx + 1), levels.end(), std::next(newLevels.begin(), levelIdx));

    this->LevelCollectionTableView->dyn__previewBeatmapLevels() = (Array<GlobalNamespace::IPreviewBeatmapLevel*>*) newLevels.convert();
}

void PlaylistEditor::InsertSelectedSongToTable() {
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> levels(this->LevelCollectionTableView->dyn__previewBeatmapLevels());
    ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> newLevels(levels.Length() + 1);
    INFO("InsertSelectedSongToTable %lu => %lu", levels.Length(), newLevels.Length());
    auto levelIdx = this->GetSelectedCustomLevelIdx();

    std::copy(levels.begin(), levels.end(), newLevels.begin());
    newLevels[newLevels.Length() - 1] = levels[levelIdx];

    this->LevelCollectionTableView->dyn__previewBeatmapLevels() = (Array<GlobalNamespace::IPreviewBeatmapLevel*>*) newLevels.convert();
}

void PlaylistEditor::RemoveSongsInFilterList(const StringW &levelID) {
    auto packs = *(this->LevelSearchViewController->dyn__beatmapLevelPacks());
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
            (!useArray && levelID == levelsRO->get(i)->get_levelID())) {
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
            newLevels[j++] = levelsRO->get(i);
        }
    }

    ((GlobalNamespace::CustomBeatmapLevelCollection*) (reinterpret_cast<::GlobalNamespace::IAnnotatedBeatmapLevelCollection*>(packs[0]))
                ->get_beatmapLevelCollection())->dyn__customPreviewBeatmapLevels() = (Array<GlobalNamespace::CustomPreviewBeatmapLevel*>*) newLevels.convert(); // update filter param 就有用
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

    ((GlobalNamespace::CustomBeatmapLevelCollection*) beatmapLevelCollection)->dyn__customPreviewBeatmapLevels() = (Array<GlobalNamespace::CustomPreviewBeatmapLevel*>*) newLevels.convert();

    // process cover image if not default image
    if (annotatedBeatmapLevelCollections[0]->get_coverImage() != annotatedBeatmapLevelCollections[collectionIdx]->get_coverImage())
        return;
    this->SetSelectedCoverImage(collectionIdx, this->GetSelectedCustomPreviewBeatmapLevel()->dyn__coverImage());
}

void PlaylistEditor::CreatePackHeaderDetail() {
    if (!this->init)
        return;
    if (this->packDurationText)
        return;
    if (!this->LevelCollectionNavigationController->dyn__levelPackDetailViewController() ||
        !this->LevelCollectionNavigationController->dyn__levelPackDetailViewController()->dyn__packImage())
        return;

    this->packDurationText = QuestUI::BeatSaberUI::CreateText(
                                this->LevelCollectionNavigationController->dyn__levelPackDetailViewController()->get_transform(),
                                "text", {28, -23}, {20, 30});
    this->packDurationText->set_fontStyle(TMPro::FontStyles::Bold);
    this->packDurationText->set_alignment(TMPro::TextAlignmentOptions::Center);
    this->packDurationText->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
    this->packDurationText->get_gameObject()->SetActive(false);
}

void PlaylistEditor::CreateSongActionButton() {
    if (!this->init)
        return;
    if (this->deleteButton)
        return;
    auto parent = this->StandardLevelDetailView->get_practiceButton()->get_transform()->get_parent();
    auto deleteButtonTransform = parent->FindChild(il2cpp_utils::newcsstr("DeleteLevelButton"));
    if (!deleteButtonTransform)
        return;

    // rewrite delete button
    auto btn = deleteButtonTransform->get_gameObject()->GetComponent<UnityEngine::UI::Button*>();
    this->deleteButton = new DoubleClickIconButton(btn, [this] () {
        GlobalNamespace::CustomPreviewBeatmapLevel *selectedlevel = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(this->LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel());
        RuntimeSongLoader::API::DeleteSong(to_utf8(csstrtostr(selectedlevel->get_customLevelPath())), [this] {
            Toast::GetInstance()->ShowMessage("Delete song");
            this->RemoveSelectedSongInAllPack(true);
            this->RefreshAndStayList(REFESH_TYPE::SONG_REMOVE_STAY);
        });
    });

    // create new button
    auto posX = -22.5f;
    auto posY = -0.5f;
    this->deleteAndRemoveButton = new DoubleClickIconButton("DeleteAndRemoveFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, posY - 15.0f), UnityEngine::Vector2(20.0f,7.0f), [this] () {
            GlobalNamespace::CustomPreviewBeatmapLevel *selectedlevel = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel());
            RuntimeSongLoader::API::DeleteSong(to_utf8(csstrtostr(selectedlevel->get_customLevelPath())), [this] {
                    if (this->UpdateFileWithSelected(FILE_ACTION::ITEM_REMOVE)) {
                        Toast::GetInstance()->ShowMessage(this->IsSelectedCustomPack() ? "Delete song and remove from the list" : "Delete song and remove from all list" );
                        this->RemoveSelectedSongInAllPack(true);
                    } else {
                        // song isn't in any list
                        this->RemoveSelectedSongInPack();
                        Toast::GetInstance()->ShowMessage("Delete song");
                    }
                    this->RefreshAndStayList(REFESH_TYPE::SONG_REMOVE_STAY);
                }
            );
        }, FileToSprite("DeleteAndRemoveIcon"), "Delete and Remove Song from List");
    this->deleteAndRemoveButton->SetActive(false);

    posX += 15.0f + 1.25f ;
    this->removeButton = new IconButton("RemoveFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, posY - 15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (this->UpdateFileWithSelected(FILE_ACTION::ITEM_REMOVE)) {
                Toast::GetInstance()->ShowMessage(this->IsSelectedCustomPack() ? "Remove song from the list" : "Remove song from all list");
                if (this->IsSelectedCustomPack()) {
                    this->RemoveSelectedSongInPack();
                    this->RefreshAndStayList(REFESH_TYPE::SONG_REMOVE_STAY);
                } else
                    this->RemoveSelectedSongInAllPack(false);
            } else
                Toast::GetInstance()->ShowMessage("Song isn't in any list");
        }, FileToSprite("RemoveIcon"), "Remove Song from List");
    this->removeButton->SetActive(false);

    posX += 10.0f + 1.25f;
    this->moveUpButton = new IconButton("MoveUpFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, posY - 15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (this->UpdateFileWithSelected(FILE_ACTION::ITEM_MOVE_UP)) {
                Toast::GetInstance()->ShowMessage("Move up song");
                this->MoveUpSelectedSong();
                this->RefreshAndStayList(REFESH_TYPE::SONG_MOVE_UP);
            } else
                Toast::GetInstance()->ShowMessage("Already on top");
        }, FileToSprite("MoveUpIcon"), "Move Up Song from List");
    this->moveUpButton->SetActive(false);

    posX += 10.0f + 1.25f;
    this->moveDownButton = new IconButton("MoveDownFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, posY - 15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {

            if (this->UpdateFileWithSelected(FILE_ACTION::ITEM_MOVE_DOWN)) {
                Toast::GetInstance()->ShowMessage("Move down song");
                this->MoveDownSelectedSong();
                this->RefreshAndStayList(REFESH_TYPE::SONG_MOVE_DOWN);
            } else
                Toast::GetInstance()->ShowMessage("Already on bottom");
        }, FileToSprite("MoveDownIcon"), "Move Down Song from List");
    this->moveDownButton->SetActive(false);

    posX += 10.0f + 1.25f;
    this->insertButton = new IconButton("InsertFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, posY - 15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (this->listModal && this->listModal->GetActive()) {
                this->listModal->SetActive(false);
                return;
            }

            if (!this->listModal) {
                auto screenContainer = QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Transform*>(), [](auto x) {
                    return x->get_name()->Equals(il2cpp_utils::newcsstr("ScreenContainer"));
                });
                this->listModal = new ListModal(screenContainer->get_transform(), UnityEngine::Vector2(30.0f, 25.0f), UnityEngine::Vector2(posX + 84.0f, posY - 25.0f),
                                                [this] (const int i, const std::string &selectedPackName) {
                    const auto selectedPackIdx = i + 1;
                    const std::string selectedPackId = CustomLevelPackPrefixID + selectedPackName;
                    this->listModal->SetActive(false);
                    this->insertButton->SetInteractable(true);
                    this->lastInsertPackIdx = selectedPackIdx;
                    this->lastInsertPackName = selectedPackName;
                    if (UpdateFileWithSelected(FILE_ACTION::ITEM_INSERT, selectedPackIdx, selectedPackId)) {
                        this->InsertSelectedSongToPack(selectedPackIdx);
                        if (selectedPackIdx == this->GetSelectedPackIdx()) {
                            this->InsertSelectedSongToTable();
                            this->RefreshAndStayList(REFESH_TYPE::SONG_STAY);
                        }
                        Toast::GetInstance()->ShowMessage("Insert song to selected list");
                    }
                });

                auto practiceMaterial = QuestUI::ArrayUtil::First(this->StandardLevelDetailView->get_practiceButton()->GetComponentsInChildren<TMPro::TextMeshProUGUI*>())->get_materialForRendering();
                this->listModal->SetMaterial(practiceMaterial);
            }

            std::vector<std::string> listItem;
            auto annotatedBeatmapLevelCollections = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
            for (int i = 0; i < annotatedBeatmapLevelCollections.Length(); i++) {
                std::string selectedPackName = to_utf8(csstrtostr(annotatedBeatmapLevelCollections[i]->get_collectionName()));
                if (CustomLevelName == selectedPackName)
                    continue;
                listItem.push_back(selectedPackName);
                DEBUG("#%d add %s to insert list", i, std::string(selectedPackName).c_str()); // print this log to fix the insert list not showing bug
            }

            this->lastInsertPackIdx = this->FindPackIdx(this->lastInsertPackName, this->lastInsertPackIdx);

            this->listModal->SetListItem(listItem, (this->lastInsertPackIdx > 0) ? this->lastInsertPackIdx - 1 : 0);
            this->listModal->SetActive(true);
        }, FileToSprite("InsertIcon"), "Insert to List");
    this->insertButton->SetActive(false);

    this->lockButton = new TwoStateIconButton("LockButton", this->LevelCollectionNavigationController->get_transform(), "PracticeButton",
                                              UnityEngine::Vector2(4.0f, -5.5f), UnityEngine::Vector2(7.0f,14.5f),
        [&] () {
            INFO("Lock char %s, diff %d",
                to_utf8(csstrtostr(this->StandardLevelDetailView->dyn__beatmapCharacteristicSegmentedControlController()->get_selectedBeatmapCharacteristic()->get_serializedName())).c_str(),
                int(this->StandardLevelDetailView->dyn__beatmapDifficultySegmentedControlController()->get_selectedDifficulty()));
            if (this->UpdateFileWithSelected(FILE_ACTION::ITEM_LOCK)) {
                this->selectedLockCharStr = this->GetSelectedCharStr();
                this->selectedLockDiff = this->GetSelectedDiff();
                Toast::GetInstance()->ShowMessage("Lock Selected Char and Diff");
            }
        }, FileToSprite("UnlockIcon"), "Lock Selected Char and Diff",
        [&] () {
            if (this->UpdateFileWithSelected(FILE_ACTION::ITEM_UNLOCK)) {
                this->selectedLockCharStr = "";
                this->selectedLockDiff = 0;
                Toast::GetInstance()->ShowMessage("UnLock Selected Char and Diff");
            }
        }, FileToSprite("LockIcon"), "UnLock Selected Char and Diff");
    this->lockButton->SetActive(false);
    this->lockButton->SetButtonBackgroundActive(false);
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
    if (this->lockButton) {
        this->lockButton->SetActive(!forceDisable && atCustomPack && atCustomLevel && this->isLevelDetailReady); // not at pack header
        if (!forceDisable && atCustomPack && atCustomLevel)
            this->lockButton->SetIsFirstState(
                this->selectedLockCharStr != this->GetSelectedCharStr() ||
                this->selectedLockDiff != this->GetSelectedDiff());
    }
    if (this->createListButton)
        this->createListButton->SetActive(!forceDisable && atCustomCategory);
    if (this->deleteListButton) {
        this->deleteListButton->SetActive(!forceDisable && atCustomCategory);
        if (!atCustomPack)
            this->deleteListButton->SetInteractable(false);
    }
    if (this->imageListButton) {
        bool isDefaultCover = this->IsSelectedCustomPackUsingDefaultCover();

        this->imageListButton->SetActive(!forceDisable && atCustomCategory);
        if (!atCustomPack || (isDefaultCover && atCustomPack && !atCustomLevel)) // || at pack header
            this->imageListButton->SetInteractable(false);
        this->imageListButton->SetIsFirstState(!isDefaultCover);
    }
    if (this->recordListButton) {
        this->recordListButton->SetActive(!forceDisable && (atCustomCategory ||
                                                           (!atCustomCategory && atCustomLevel && !this->recordListButton->GetIsFirstState())));
        if (!this->recordListButton->GetIsFirstState()) {
            this->recordPackIdx = this->FindPackIdx(this->recordPackName, this->recordPackIdx);
            if (this->recordPackIdx < 0) {
                ERROR("Can't find index of record list %s", this->recordPackName.c_str());
                this->recordPackName = "";
                this->recordListButton->SetIsFirstState(true);
                Toast::GetInstance()->ShowMessage("Stop record playing level to new list");
                return;
            }
        }
    }
    if (this->packDurationText && (atCustomPack && !atCustomLevel)) { // at custom pack header
        this->packDurationText->set_text(il2cpp_utils::newcsstr("Duration\n" + this->GetSelectedPackDuration()));
        this->packDurationText->get_gameObject()->set_active(true);
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
                                        to_utf8(csstrtostr(this->AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection()->get_collectionName())) : "";
        const auto lastCollectionNameCS = il2cpp_utils::newcsstr(lastCollectionName);
        // RuntimeSongLoader::API::RefreshPacks(true); // not found
        if ("" != lastCollectionName) {
            INFO("select collection %d %s", lastCollectionIdx, lastCollectionName.c_str());
            // select level collection
            auto annotatedBeatmapLevelCollections = listToArrayW(this->AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
            for (int i = lastCollectionIdx; i < annotatedBeatmapLevelCollections.Length(); i++) // index may move back when new list created
            {
                if (annotatedBeatmapLevelCollections[i]->get_collectionName()->Equals(lastCollectionNameCS)) {
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
        } else if (SONG_REMOVE_STAY == act) {
            if (selectedRow >= this->LevelCollectionTableView->NumberOfCells())
                nextSelectedRow = this->LevelCollectionTableView->NumberOfCells() - 1;
        } else if (SONG_STAY == act) {
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
