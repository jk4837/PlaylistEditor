#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <fstream>

#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/TextureWrapMode.hpp"
#include "UnityEngine/SpriteMeshType.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/LayoutGroup.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/ExternalComponents.hpp"
#include "questui/shared/CustomTypes/Components/ClickableText.hpp"
#include "codegen/include/HMUI/HoverHintController.hpp"
#include "codegen/include/HMUI/ViewController_AnimationDirection.hpp"
#include "codegen/include/HMUI/ScrollView.hpp"
#include "codegen/include/HMUI/TableView.hpp"
#include "codegen/include/HMUI/TableView_ScrollPositionType.hpp"
#include "codegen/include/HMUI/ImageView.hpp"
#include "codegen/include/System/Action.hpp"
#include "playlistmanager/shared/PlaylistManager.hpp"
#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsViewController.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSegmentedControlController.hpp"
#include "GlobalNamespace/BeatmapDifficultySegmentedControlController.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
// #include "GlobalNamespace/IPlaylist.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/LoadingControl.hpp"
#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"

#include "main.hpp"
#include "logging.hpp"
// #include "toast.hpp"
#include "CustomTypes/Toast.hpp"

const std::string CustomLevelPackPath = "/sdcard/ModData/com.beatgames.beatsaber/Mods/PlaylistManager/Playlists/";
const std::string CustomLevelPackPrefixID = "custom_levelPack_";
const std::string CustomLevelID = "custom_levelPack_CustomLevels";
const std::string CustomLevelPrefixID = "custom_level_";

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup
// static PlaylistEditor::Toast *Toast;
// static CustomPreviewBeatmapLevel *selectedlevel = nullptr;
static GlobalNamespace::LevelSelectionFlowCoordinator *LevelSelectionFlowCoordinator = nullptr;
static GlobalNamespace::LevelSelectionNavigationController *LevelSelectionNavigationController = nullptr;
static GlobalNamespace::LevelFilteringNavigationController *LevelFilteringNavigationController = nullptr;
static GlobalNamespace::LevelCollectionNavigationController *LevelCollectionNavigationController = nullptr;
static GlobalNamespace::SelectLevelCategoryViewController *LevelCategoryViewController = nullptr;
static GlobalNamespace::BeatmapLevelsModel* BeatmapLevelsModel = nullptr;
static GlobalNamespace::LevelCollectionViewController *LevelCollectionViewController = nullptr;
static GlobalNamespace::LevelCollectionTableView *LevelCollectionTableView = nullptr;
static GlobalNamespace::StandardLevelDetailViewController *LevelDetailViewController = nullptr;
static GlobalNamespace::StandardLevelDetailView *StandardLevelDetailView = nullptr;
static GlobalNamespace::BeatmapDifficultySegmentedControlController *LevelDifficultyViewController = nullptr;
static GlobalNamespace::BeatmapCharacteristicSegmentedControlController *BeatmapCharacteristicSelectionViewController = nullptr;
static GlobalNamespace::AnnotatedBeatmapLevelCollectionsViewController *AnnotatedBeatmapLevelCollectionsViewController = nullptr;

std::map<::StringW, std::string> playlists;
static UnityEngine::UI::Button *createListButton = nullptr;
HMUI::InputFieldView* createListInput = nullptr;
static UnityEngine::UI::Button *deleteListButton = nullptr;
static HMUI::ImageView *deleteListButtonImageView = nullptr;
static UnityEngine::UI::Button *deleteButton = nullptr;
static HMUI::ImageView *deleteButtonImageView = nullptr;
static HMUI::ImageView *deleteAndRemoveButtonImageView = nullptr;
static UnityEngine::UI::Button *removeButton = nullptr;
static UnityEngine::UI::Button *insertButton = nullptr;
static UnityEngine::UI::Button *deleteAndRemoveButton = nullptr;
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

template <class T>
constexpr ArrayW<T> listToArrayW(::System::Collections::Generic::IReadOnlyList_1<T>* list) {
    return ArrayW<T>(reinterpret_cast<Array<T>*>(list));
}

typedef enum LIST_ACTION {
    ITEM_INSERT, ITEM_REMOVE, ITEM_MOVE_UP, ITEM_MOVE_DOWN
} LIST_ACTION_T;

typedef enum SCROLL_ACTION {
    NO_STAY, SCROLL_STAY, SCROLL_REMOVE_STAY, SCROLL_MOVE_UP, SCROLL_MOVE_DOWN
} MOVE_ACTION_T;

static std::string rapidjsonToString(rapidjson::Document &document) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    return std::string(buffer.GetString());
}

bool WriteFile(const std::string &path, rapidjson::Document &document) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    document.Accept(writer);
    if (!writefile(path, buffer.GetString())) {
        ERROR("Failed to write file %s", path.data());
        return false;
    }
    return true;
}

bool LoadFile(const std::string &path, rapidjson::Document &document) {
    bool success = false;
    try {
        if(!fileexists(path))
            return false;
        auto json = readfile(path);
        document.Parse(json);
        if (document.HasParseError())
            throw std::invalid_argument("parsing error");
        if (!document.IsObject())
            throw std::invalid_argument("root isn't object");
        if (document.GetObject()["playlistTitle"].GetType() != rapidjson::kStringType)
            throw std::invalid_argument("root/playlistTitle not string");
        if (document.GetObject()["songs"].GetType() != rapidjson::kArrayType)
            throw std::invalid_argument("root/songs not array");
        success = true;
    } catch (const std::exception &e) {
        PlaylistEditor::Toast::GetInstance()->ShowMessage(e.what());
        ERROR("Error loading playlist %s: %s", path.data(), e.what());
    }
    return success;
}

bool FindSongIdx(rapidjson::Document &document, const std::string &selectedLevelID, int &idx) {
    const auto selectedHash = selectedLevelID.substr(CustomLevelPrefixID.length());
    try {
        const auto &songs = document.GetObject()["songs"].GetArray();
        for (rapidjson::SizeType i = 0; i < songs.Size(); i++) {
            if (!songs[i].IsObject())
                throw std::invalid_argument("invalid song object");
            if (!songs[i].HasMember("hash") || !songs[i]["hash"].IsString())
                throw std::invalid_argument("invalid hash in song object");
            if (0 != strcasecmp(songs[i]["hash"].GetString(), selectedHash.c_str()))
                continue;
            idx = i;
            return true;
        }
    } catch (const std::exception &e) {
        PlaylistEditor::Toast::GetInstance()->ShowMessage(e.what());
        ERROR("Error parsing playlist: %s", e.what());
    }
    return false;
}

std::string GetPlaylistPath(const ::StringW &listID = "", const bool fullRefresh = false) {
    if (CustomLevelID == listID)
        return "";
    if (fullRefresh)
        playlists.clear();
    if (!playlists.contains(listID)) {
        INFO("Don't have playlist %s, reload all", std::string(listID).c_str());
        playlists.clear();
        const std::string playlistPath = "/sdcard/ModData/com.beatgames.beatsaber/Mods/PlaylistManager/Playlists";

        if(!std::filesystem::is_directory(playlistPath)) {
            INFO("Don't have playlist dir %s", playlistPath.c_str());
            return "";
        }
        for (const auto& entry : std::filesystem::directory_iterator(playlistPath)) {
            if(!entry.is_directory()) {
                rapidjson::Document document;
                auto path = entry.path().string();
                if (!LoadFile(path, document))
                    continue;
                const std::string playlistTitle = document.GetObject()["playlistTitle"].GetString();
                playlists[CustomLevelPackPrefixID + playlistTitle] = path;
                INFO("LoadPlaylists %s : %s", playlistTitle.c_str(), path.c_str());
            }
        }
    }
    if (!playlists.contains(listID)) {
        INFO("Failed to find playlist of %s", std::string(listID).c_str());
        return "";
    }
    return playlists[listID];
}

bool CreateFile(const std::string &name) {
    try {
        rapidjson::Document document;
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

        document.SetObject();
        document.AddMember("playlistTitle", name, allocator);   // require
        document.AddMember("playlistDescription", "Created by " ID, allocator);
        document.AddMember("songs", rapidjson::Value(rapidjson::kArrayType), allocator);             // require
        document.AddMember("image", rapidjson::Value(), allocator);
        if (!WriteFile(CustomLevelPackPath + name, document))
            throw std::invalid_argument("failed to write file");
        return true;
    } catch (const std::exception &e) {
        PlaylistEditor::Toast::GetInstance()->ShowMessage(e.what());
        ERROR("Error create playlist %s: %s", name.c_str(), e.what());
    }
    return false;
}

bool DeleteFile(const std::string &path) {
    if(!fileexists(path))
        return true;
    return deletefile(path);
}

bool UpdateFile(const std::string &path, const LIST_ACTION act, const std::string &insertPath = "") {
    try {
        rapidjson::Document document;
        if(!path.empty() && !LoadFile(path, document))
            throw std::invalid_argument("failed to load file");

        int idx = 0;
        const std::string selectedLevelID = LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel()->get_levelID();
        const bool found = !path.empty() && FindSongIdx(document, selectedLevelID, idx);

        const auto &songs = document.GetObject()["songs"].GetArray();
        switch (act) {
            case ITEM_INSERT:
            {
                rapidjson::Document document2;
                rapidjson::Document::AllocatorType& allocator2 = document2.GetAllocator();
                rapidjson::Value insertSong(rapidjson::kObjectType);

                if (!found) {
                    GlobalNamespace::CustomPreviewBeatmapLevel *selectedlevel = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel());
                    insertSong.SetObject();
                    insertSong.AddMember("songName", std::string(selectedlevel->get_songName()), allocator2);
                    insertSong.AddMember("levelAuthorName", std::string(selectedlevel->get_levelAuthorName()), allocator2);
                    insertSong.AddMember("hash", std::string(selectedlevel->get_levelID()).substr(CustomLevelPrefixID.length()), allocator2);
                    insertSong.AddMember("levelid", std::string(selectedlevel->get_levelID()), allocator2);
                    insertSong.AddMember("uploader", std::string(selectedlevel->get_levelAuthorName()), allocator2);
                } else
                    insertSong = songs[idx];

                if (!LoadFile(insertPath, document2))
                    throw std::invalid_argument("failed to load file which want to insert to");
                document2.GetObject()["songs"].GetArray().PushBack(insertSong, allocator2);
                if (!WriteFile(insertPath, document2))
                    throw std::invalid_argument("failed to write file");
                PlaylistEditor::Toast::GetInstance()->ShowMessage("insert song");
            }
            break;
            case ITEM_REMOVE:
                if (path.empty()) {
                    bool has_remove = false;
                    if (playlists.empty())
                        GetPlaylistPath();
                    for (auto it : playlists)
                        has_remove |= UpdateFile(it.second, ITEM_REMOVE);
                    if (!has_remove)
                        return false;
                } else if (!found) {
                    ERROR("Failed to find %s in playlist dir %s", std::string(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel()->get_levelID()).c_str(), path.c_str());
                    return false;
                } else {
                    songs.Erase(songs.Begin() + idx);
                    if (!WriteFile(path, document))
                        throw std::invalid_argument("failed to write file");
                }
                PlaylistEditor::Toast::GetInstance()->ShowMessage("remove song");
                break;
            case ITEM_MOVE_DOWN:
                if (!found) {
                    ERROR("Failed to find %s in playlist dir %s", std::string(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel()->get_levelID()).c_str(), path.c_str());
                    return false;
                }
                if (idx >= songs.Size() - 1) // already at bottom
                    return false;
                songs[idx].Swap(songs[idx+1]);
                if (!WriteFile(path, document))
                    throw std::invalid_argument("failed to write file");
                PlaylistEditor::Toast::GetInstance()->ShowMessage("move donwn song");
                break;
            case ITEM_MOVE_UP:
                if (!found) {
                    ERROR("Failed to find %s in playlist dir %s", std::string(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel()->get_levelID()).c_str(), path.c_str());
                    return false;
                }
                if (idx <= 0) // already at top
                    return false;
                songs[idx].Swap(songs[idx-1]);
                if (!WriteFile(path, document))
                    throw std::invalid_argument("failed to write file");
                PlaylistEditor::Toast::GetInstance()->ShowMessage("move up song");
                break;
        }
        return true;
    } catch (const std::exception &e) {
        PlaylistEditor::Toast::GetInstance()->ShowMessage(e.what());
        ERROR("Error loading playlist %s: %s", path.data(), e.what());
    }
    return false;
}

static bool IsSelectedCustomPack()
{
    return LevelFilteringNavigationController && LevelCollectionNavigationController && LevelCollectionNavigationController->dyn__levelPack() &&
            GlobalNamespace::SelectLevelCategoryViewController::LevelCategory::CustomSongs == LevelFilteringNavigationController->get_selectedLevelCategory() &&
            CustomLevelID != LevelCollectionNavigationController->dyn__levelPack()->get_packID();
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

static GlobalNamespace::IAnnotatedBeatmapLevelCollection* GetLevelCollectionByName(const ::StringW &levelCollectionName)
{
    GlobalNamespace::IAnnotatedBeatmapLevelCollection* levelCollection = nullptr;

    // search level packs
    auto beatMapLevelPackCollection = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::BeatmapLevelPackCollectionSO*>().Last();
    auto levelpacks = beatMapLevelPackCollection->dyn__allBeatmapLevelPacks();
    int length = levelpacks.Length();
    for (int i = 0; i < length; i++)
    {
        auto o = reinterpret_cast<GlobalNamespace::IAnnotatedBeatmapLevelCollection*>(levelpacks[i]);
        if (o->get_collectionName()->Equals(levelCollectionName))
        {
            levelCollection = o;
            break;
        }
    }

    // search playlists
    if (!levelCollection)
    {
        auto annotatedBeatmapLevelCollections = listToArrayW(AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections());
        //IReadOnlyList<IAnnotatedBeatmapLevelCollection> _annotatedBeatmapLevelCollections = AnnotatedBeatmapLevelCollectionsViewController.GetField<IReadOnlyList<IAnnotatedBeatmapLevelCollection>, AnnotatedBeatmapLevelCollectionsViewController>("_annotatedBeatmapLevelCollections");
        // length = ::System::Collections::Generic::IReadOnlyCollection_1<::GlobalNamespace::IAnnotatedBeatmapLevelCollection*>(*annotatedBeatmapLevelCollections).get_Count();
        length = annotatedBeatmapLevelCollections.Length();
        for (int i = 0; i < length; i++)
        {
            auto c = annotatedBeatmapLevelCollections[i];
            if (c->get_collectionName()->Equals(levelCollectionName))
            {
                levelCollection = c;
                break;
            }
        }
    }

    return levelCollection;
}

static void SelectLevelCollection(const int lastCollectionIdx, const ::StringW &levelCollectionName)
{
    try
    {
        auto collection = GetLevelCollectionByName(levelCollectionName);
        if (!collection)
        {
            INFO("Could not locate requested level collection...");
            return;
        }

        INFO("Selecting level collection: %s", std::string(levelCollectionName).c_str());
        // INFO("1 Selecting level collection: %s", std::string(levelCollectionName).c_str());

        /* no need
        LevelFilteringNavigationController->SelectAnnotatedBeatmapLevelCollection(reinterpret_cast<GlobalNamespace::IBeatmapLevelPack*>(collection));
        // ^^^ will reset to music packs category
        LevelCategoryViewController->Setup(GlobalNamespace::SelectLevelCategoryViewController::LevelCategory::CustomSongs, LevelFilteringNavigationController->dyn__enabledLevelCategories());
        // ^^^ only cate change, but play list view is music packs
        LevelFilteringNavigationController->UpdateSecondChildControllerContent(GlobalNamespace::SelectLevelCategoryViewController::LevelCategory::CustomSongs);
        // ^^^ after cate change, this can change play list
        */

        // AnnotatedBeatmapLevelCollectionsViewController->dyn__selectedItemIndex() = lastCollectionIdx;
        // AnnotatedBeatmapLevelCollectionsViewController->HandleDidSelectAnnotatedBeatmapLevelCollection(collection);
        // ^^^ no use
        AnnotatedBeatmapLevelCollectionsViewController->SetData(AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections(), lastCollectionIdx, false);
        // AnnotatedBeatmapLevelCollectionsViewController->HandleDidSelectAnnotatedBeatmapLevelCollection(collection);


        INFO("2 Selecting level collection: %s", std::string(levelCollectionName).c_str());
        // make table to be specific play list
        // LevelFilteringNavigationController->HandleAnnotatedBeatmapLevelCollectionsViewControllerDidSelectAnnotatedBeatmapLevelCollection(collection);


        INFO("Done selecting level collection!");
    }
    catch (const std::exception& e)
    {
        ERROR("Failed to select level collection, err: %s", e.what());
    }
}

MAKE_HOOK_MATCH(FlowCoordinator_PresentFlowCoordinator, &HMUI::FlowCoordinator::PresentFlowCoordinator, void, HMUI::FlowCoordinator* self, HMUI::FlowCoordinator* flowCoordinator, System::Action* finishedCallback, HMUI::ViewController::AnimationDirection animationDirection, bool immediately, bool replaceTopViewController)
{
    FlowCoordinator_PresentFlowCoordinator(self, flowCoordinator, finishedCallback, animationDirection, immediately, replaceTopViewController);
    if (il2cpp_utils::try_cast<GlobalNamespace::SoloFreePlayFlowCoordinator>(flowCoordinator))
    {
        INFO("Initializing PlayListEditor for Single Player Mode");
        LevelSelectionFlowCoordinator = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::SoloFreePlayFlowCoordinator *>().Last();
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

        LevelDifficultyViewController = StandardLevelDetailView->dyn__beatmapDifficultySegmentedControlController();
        INFO("Acquired BeatmapDifficultySegmentedControlController [%d]", LevelDifficultyViewController->GetInstanceID());

        AnnotatedBeatmapLevelCollectionsViewController = LevelFilteringNavigationController->dyn__annotatedBeatmapLevelCollectionsViewController();
        INFO("Acquired AnnotatedBeatmapLevelCollectionsViewController from LevelFilteringNavigationController [%d]", AnnotatedBeatmapLevelCollectionsViewController->GetInstanceID());

        LevelCategoryViewController = LevelFilteringNavigationController->dyn__selectLevelCategoryViewController();
        INFO("Acquired LevelCategoryViewController from LevelFilteringNavigationController [%d]", LevelCategoryViewController->GetInstanceID());

        BeatmapLevelsModel = LevelFilteringNavigationController->dyn__beatmapLevelsModel();
        INFO("Acquired BeatmapLevelsModel [%d]", BeatmapLevelsModel->GetInstanceID());
    }
}

template <class T>
static void listAllName(UnityEngine::Transform *parent, const std::string &prefix = "") {
    INFO("%s #p: tag: %s, name: %s, id: %u", prefix.c_str(), std::string(parent->get_tag()).c_str(), std::string(parent->get_name()).c_str(), parent->GetInstanceID());
    auto childs = parent->GetComponentsInChildren<T *>();
    // auto childs = parent->GetComponentsInChildren<UnityEngine::Transform *>();
    // auto childs = parent->GetComponentsInChildren<TMPro::TextMeshProUGUI *>();
    // auto childs = parent->GetComponentsInChildren<UnityEngine::UI::Button *>();
    // auto childs = parent->GetComponentsInChildren<GlobalNamespace::StandardLevelDetailViewController *>();
    for (size_t i = 0; i < childs.Length(); i++)
    {
        if (parent->GetInstanceID() == childs.get(i)->GetInstanceID())
            continue;
        INFO("%s #%zu: tag: %s, name: %s, id: %u", prefix.c_str(), i, std::string(childs.get(i)->get_tag()).c_str(), std::string(childs.get(i)->get_name()).c_str(), childs.get(i)->GetInstanceID());
        // for class == ImageView
        // childs.get(i)->set_color(UnityEngine::Color::get_red());
        // childs.get(i)->set_color0(UnityEngine::Color::get_red());
        // childs.get(i)->set_color1(UnityEngine::Color::get_red());
        // for class == Transform
        // childs.get(i)->Translate(1,1,1);
        // listAllName(childs.get(i), prefix + "  ");
    }
}

static void inActiveLevelDeleteButton(UnityEngine::Transform *parent) {
    static auto deleteLevelButtonName = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("DeleteLevelButton");
    auto deleteButtonTransform = parent->FindChild(deleteLevelButtonName);

    if (deleteButtonTransform) {
        INFO("Inactive level delete button, id: %u", deleteButtonTransform->GetInstanceID());
        deleteButtonTransform->get_gameObject()->SetActive(false);
    }
}

static void logPacks(::StringW lastCollectionName = "") {
    auto packArr = BeatmapLevelsModel->get_customLevelPackCollection()->get_beatmapLevelPacks();
    for (int j = 0; j < packArr.Length(); j++)
    {
        auto tmp = listToArrayW(((GlobalNamespace::IAnnotatedBeatmapLevelCollection *)(packArr.get(j)))->get_beatmapLevelCollection()->get_beatmapLevels());
        INFO("pack #%d %s has %lu", j, std::string(packArr.get(j)->get_packName()).c_str(), tmp.Length());
        // if (!packArr.get(j)->get_packName()->Equals(lastCollectionName))
        //     continue;
        // for (int i = 0; i < tmp.Length(); i++)
        //     INFO("    pack #%d %s", i, std::string(tmp[i]->get_songName()).c_str());
    }
}

#define MakeDelegate(DelegateType, varName) (il2cpp_utils::MakeDelegate<DelegateType>(classof(DelegateType), varName))

UnityEngine::UI::Button* CreateIconButton(std::string_view name, UnityEngine::Transform* parent, std::string_view buttonTemplate, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, std::function<void(void)> onClick, UnityEngine::Sprite* icon, std::string_view hint)
{
    assert(deleteButton);
    auto btn = QuestUI::BeatSaberUI::CreateUIButton(parent, "", buttonTemplate, anchoredPosition, sizeDelta, onClick);

    QuestUI::BeatSaberUI::AddHoverHint(btn->get_gameObject(), hint);

    UnityEngine::Object::Destroy(btn->get_transform()->Find("Underline")->get_gameObject());

    UnityEngine::Transform* contentTransform = btn->get_transform()->Find("Content");
    UnityEngine::Object::Destroy(contentTransform->Find("Text")->get_gameObject());
    UnityEngine::Object::Destroy(contentTransform->GetComponent<UnityEngine::UI::LayoutElement*>());

    UnityEngine::UI::Image* iconImage = UnityEngine::GameObject::New_ctor("Icon")->AddComponent<HMUI::ImageView*>();
    iconImage->set_material(deleteButton->get_gameObject()->GetComponentInChildren<HMUI::ImageView*>()->get_material());
    iconImage->get_rectTransform()->SetParent(contentTransform, false);
    iconImage->get_rectTransform()->set_sizeDelta(UnityEngine::Vector2(10.0f, 10.0f));
    iconImage->set_sprite(icon);
    iconImage->set_preserveAspect(true);

    return btn;
}

static UnityEngine::Sprite* FileToSprite(const std::string_view &image_name)
{
    std::string path = string_format("/sdcard/ModData/com.beatgames.beatsaber/Mods/%s/Icons/%s.png", modInfo.id.c_str(),image_name.data());
    std::ifstream instream(path, std::ios::in | std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(instream)), std::istreambuf_iterator<char>());
    Array<uint8_t>* bytes = il2cpp_utils::vectorToArray(data);
    UnityEngine::Texture2D* texture = UnityEngine::Texture2D::New_ctor(0, 0, UnityEngine::TextureFormat::RGBA32, false, false);
    if (UnityEngine::ImageConversion::LoadImage(texture, bytes, false)) {
        texture->set_wrapMode(UnityEngine::TextureWrapMode::Clamp);
        return UnityEngine::Sprite::Create(texture, UnityEngine::Rect(0.0f, 0.0f, (float)texture->get_width(), (float)texture->get_height()), UnityEngine::Vector2(0.5f,0.5f), 100.0f, 1u, UnityEngine::SpriteMeshType::Tight, UnityEngine::Vector4(0.0f, 0.0f, 0.0f, 0.0f), false);
    }
    return nullptr;
}

#include "questui/shared/CustomTypes/Components/WeakPtrGO.hpp"
#include "codegen/include/HMUI/InputFieldView.hpp"
#include "codegen/include/Polyglot/LocalizedTextMeshProUGUI.hpp"
#include "codegen/include/HMUI/InputFieldView_InputFieldChanged.hpp"
HMUI::InputFieldView* CreateStringInput(UnityEngine::Transform* parent, StringW settingsName, StringW currentValue, UnityEngine::Vector2 anchoredPosition, float width,std::function<void(StringW)> onEnter) {
    auto originalFieldView = UnityEngine::Resources::FindObjectsOfTypeAll<HMUI::InputFieldView *>().First(
        [](HMUI::InputFieldView *x) {
            return x->get_name() == "GuestNameInputField";
        }
    );
    UnityEngine::GameObject* gameObj = UnityEngine::Object::Instantiate(originalFieldView->get_gameObject(), parent, false);
    gameObj->set_name("QuestUIStringInput");

    UnityEngine::RectTransform* rectTransform = gameObj->GetComponent<UnityEngine::RectTransform*>();
    rectTransform->SetParent(parent, false);
    rectTransform->set_anchoredPosition(anchoredPosition);
    rectTransform->set_sizeDelta(UnityEngine::Vector2(width, 10.0f));
    rectTransform->set_localScale({0.75, 0.75, 1});

    HMUI::InputFieldView* fieldView = gameObj->GetComponent<HMUI::InputFieldView*>();
    fieldView->dyn__useGlobalKeyboard() = true;
    fieldView->dyn__textLengthLimit() = 28;

    fieldView->Awake();

    std::function<void()> enterFunction = (std::function<void()>) [fieldView, onEnter] () {
        if (onEnter)
            onEnter(fieldView->get_text());
    };
    auto clearButton = fieldView->get_gameObject()->Find("ClearButton")->GetComponent<UnityEngine::UI::Button*>();
    clearButton->set_onClick(UnityEngine::UI::Button::ButtonClickedEvent::New_ctor());
    clearButton->get_onClick()->AddListener(MakeDelegate(UnityEngine::Events::UnityAction*, enterFunction));
    QuestUI::BeatSaberUI::SetButtonIcon(clearButton, FileToSprite("EnterIcon"));

    UnityEngine::Object::Destroy(fieldView->dyn__placeholderText()->GetComponent<Polyglot::LocalizedTextMeshProUGUI*>());
    fieldView->dyn__placeholderText()->GetComponent<TMPro::TextMeshProUGUI*>()->SetText(settingsName);
    fieldView->SetText(currentValue);

    return fieldView;
}

static void createListActionButton() {
    createListButton = CreateIconButton("CreateListButton", LevelFilteringNavigationController->get_transform(), "PracticeButton",
                                        UnityEngine::Vector2(69.0f, -3.0f), UnityEngine::Vector2(10.0f, 7.0f), [] () {
            if (!createListInput) {
                INFO("0 createListInput");
                auto screenContainer = QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Transform*>(), [](auto x) {
                    return x->get_name()->Equals("ScreenContainer");
                });
                createListInput = CreateStringInput(screenContainer->get_transform(), "Ente new playlist name", "",
                                    UnityEngine::Vector2(55.0f, -17.0f), 50.0f, [] (StringW value) {
                                        INFO("Enter %s", std::string(value).c_str());
                                        if (CreateFile(value))
                                            RefreshAndStayList(SCROLL_ACTION::SCROLL_STAY);
                                        createListInput->get_gameObject()->set_active(false);
                                        createListInput->SetText("");
                });
                createListInput->get_gameObject()->set_active(true);
                return;
            }
            createListInput->get_gameObject()->set_active(!createListInput->get_gameObject()->get_active());
        }, FileToSprite("InsertIcon"), "Create List");
    deleteListButton = CreateIconButton("DeleteListButton", LevelFilteringNavigationController->get_transform(), "PracticeButton",
                                        UnityEngine::Vector2(69.0f, 3.0f), UnityEngine::Vector2(10.0f, 7.0f), [] () {
                            if (UnityEngine::Color::get_red() != deleteListButtonImageView->get_color()) {
                                deleteListButtonImageView->set_color(UnityEngine::Color::get_red());
                                return;
                            }
                            DeleteFile(GetPlaylistPath(GetSelectedPackID()));
                            deleteListButtonImageView->set_color(UnityEngine::Color::get_white());
                            RefreshAndStayList(SCROLL_ACTION::NO_STAY);
                        }, FileToSprite("DeleteIcon"), "Delete List");
    deleteListButtonImageView = deleteListButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([&] (auto x) -> bool {
        return "Icon" == x->get_name();
    });
    deleteListButtonImageView->get_transform()->SetAsLastSibling();
}

static void createActionButton(UnityEngine::Transform *parent) {
    auto deleteButtonTransform = parent->FindChild("DeleteLevelButton");
    if (!deleteButtonTransform)
        return;
    INFO("SetOnClick level delete button, id: %u", deleteButtonTransform->GetInstanceID());
    deleteButton = deleteButtonTransform->get_gameObject()->GetComponent<UnityEngine::UI::Button*>();
    deleteButtonImageView = deleteButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([&] (auto x) -> bool {
        return "Icon" == x->get_name();
    });
    std::function<void()> deleteFunction = (std::function<void()>) [] () {
        if (!deleteButtonImageView) {
            INFO("Null deleteButtonImageView");
            return;
        }
        if (UnityEngine::Color::get_red() != deleteButtonImageView->get_color()) {
            deleteButtonImageView->set_color(UnityEngine::Color::get_red());
            return;
        }
        GlobalNamespace::CustomPreviewBeatmapLevel *selectedlevel = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel());
        RuntimeSongLoader::API::DeleteSong(std::string(selectedlevel->get_customLevelPath()), [] {
                INFO("Success delete song");
                RefreshAndStayList(IsSelectedCustomPack() ? SCROLL_ACTION::SCROLL_REMOVE_STAY : SCROLL_ACTION::SCROLL_STAY);
            }
        );
    };
    deleteButton->set_onClick(UnityEngine::UI::Button::ButtonClickedEvent::New_ctor());
    deleteButton->get_onClick()->AddListener(MakeDelegate(UnityEngine::Events::UnityAction*, deleteFunction));

    auto posX = -22.5f;
    deleteAndRemoveButton = CreateIconButton("DeleteAndRemoveFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(20.0f,7.0f), [] () {
            if (!deleteAndRemoveButtonImageView) {
                INFO("Null deleteAndRemoveButtonImageView");
                return;
            }
            if (UnityEngine::Color::get_red() != deleteAndRemoveButtonImageView->get_color()) {
                deleteAndRemoveButtonImageView->set_color(UnityEngine::Color::get_red());
                return;
            }
            GlobalNamespace::CustomPreviewBeatmapLevel *selectedlevel = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel());
            RuntimeSongLoader::API::DeleteSong(std::string(selectedlevel->get_customLevelPath()), [] {
                    INFO("Success delete song");
                    UpdateFile(GetPlaylistPath(GetSelectedPackID()), LIST_ACTION::ITEM_REMOVE);
                    RefreshAndStayList(IsSelectedCustomPack() ? SCROLL_ACTION::SCROLL_REMOVE_STAY : SCROLL_ACTION::SCROLL_STAY);
                }
            );
        }, FileToSprite("DeleteAndRemoveIcon"), "Delete and Remove Song from List");
    deleteAndRemoveButtonImageView = deleteAndRemoveButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([&] (auto x) -> bool {
        return "Icon" == x->get_name();
    });
    deleteAndRemoveButton->get_gameObject()->set_active(false);
    deleteAndRemoveButton->get_transform()->SetAsLastSibling();

    posX += 15.0f + 1.25f ;
    removeButton = CreateIconButton("RemoveFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            if (UpdateFile(GetPlaylistPath(GetSelectedPackID()), LIST_ACTION::ITEM_REMOVE))
                RefreshAndStayList(IsSelectedCustomPack() ? SCROLL_ACTION::SCROLL_REMOVE_STAY : SCROLL_ACTION::SCROLL_STAY);
        }, FileToSprite("RemoveIcon"), "Remove Song from List");
    removeButton->get_gameObject()->set_active(false);
    removeButton->get_transform()->SetAsLastSibling();

    posX += 10.0f + 1.25f;
    moveUpButton = CreateIconButton("MoveUpFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            UpdateFile(GetPlaylistPath(GetSelectedPackID()), LIST_ACTION::ITEM_MOVE_UP);
            RefreshAndStayList(SCROLL_ACTION::SCROLL_MOVE_UP);
        }, FileToSprite("MoveUpIcon"), "Move Up Song from List");
    moveUpButton->get_gameObject()->set_active(false);
    moveUpButtonImageView = moveUpButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([&] (auto x) -> bool {
        return "Icon" == x->get_name();
    });
    moveUpButton->get_transform()->SetAsLastSibling();

    posX += 10.0f + 1.25f;
    moveDownButton = CreateIconButton("MoveDownFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                            UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
            UpdateFile(GetPlaylistPath(GetSelectedPackID()), LIST_ACTION::ITEM_MOVE_DOWN);
            RefreshAndStayList(SCROLL_ACTION::SCROLL_MOVE_DOWN);
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
            GetPlaylistPath("", true);
            for (auto it : playlists) {
                std::string selectedPackName = std::string(it.first).substr(CustomLevelPackPrefixID.length());
                listModalItem.push_back(QuestUI::BeatSaberUI::CreateClickableText(listContainer->get_transform(), selectedPackName, false, [selectedPackName] () {
                    std::string selectedPackId = CustomLevelPackPrefixID + selectedPackName;
                    INFO("Cell %s clicked", selectedPackId.c_str());
                    listModal->get_gameObject()->set_active(false);
                    insertButton->set_interactable(true);
                    UpdateFile(GetPlaylistPath(GetSelectedPackID()), LIST_ACTION::ITEM_INSERT, GetPlaylistPath(selectedPackId));
                    RefreshAndStayList(SCROLL_ACTION::SCROLL_STAY);
                }));
            }
        }, FileToSprite("InsertIcon"), "Insert to List");
    insertButton->get_gameObject()->set_active(false);
    insertButton->get_transform()->SetAsLastSibling();

    createListActionButton();
}

void adjustUI(const bool customLevel)
{
    if (!customLevel) {
        if (deleteButton)
            deleteButton->get_gameObject()->set_active(false);
        if (deleteAndRemoveButton)
            deleteAndRemoveButton->get_gameObject()->set_active(false);
        if (removeButton)
            removeButton->get_gameObject()->set_active(false);
        if (insertButton) {
            insertButton->get_gameObject()->set_active(false);
            if (listModal)
                listModal->get_gameObject()->set_active(false);
        }
        if (moveUpButton)
            moveUpButton->get_gameObject()->set_active(false);
        if (moveDownButton)
            moveDownButton->get_gameObject()->set_active(false);
    } else {
        if (deleteButton) {
            deleteButton->get_gameObject()->set_active(true);
            if (deleteButtonImageView)
                deleteButtonImageView->set_color(UnityEngine::Color::get_white());
        }
        if (deleteAndRemoveButton) {
            deleteAndRemoveButton->get_gameObject()->set_active(true);
            if (deleteAndRemoveButtonImageView)
                deleteAndRemoveButtonImageView->set_color(UnityEngine::Color::get_white());
        }
        if (removeButton)
            removeButton->get_gameObject()->set_active(true);
        if (insertButton) {
            insertButton->get_gameObject()->set_active(true);
            if (listModal)
                listModal->get_gameObject()->set_active(false);
        }
        if (moveUpButton) {
            moveUpButton->get_gameObject()->set_active(true);
            moveUpButton->set_interactable(IsSelectedCustomPack());
            if (moveUpButtonImageView)
                moveUpButtonImageView->set_color(IsSelectedCustomPack() ? UnityEngine::Color::get_white() : UnityEngine::Color::get_gray());
        }
        if (moveDownButton) {
            moveDownButton->get_gameObject()->set_active(true);
            moveDownButton->set_interactable(IsSelectedCustomPack());
            if (moveDownButtonImageView)
                moveDownButtonImageView->set_color(IsSelectedCustomPack() ? UnityEngine::Color::get_white() : UnityEngine::Color::get_gray());
        }
    }
}

MAKE_HOOK_MATCH(StandardLevelDetailView_RefreshContent, &GlobalNamespace::StandardLevelDetailView::RefreshContent, void, GlobalNamespace::StandardLevelDetailView* self)
{
    StandardLevelDetailView_RefreshContent(self);
    bool customLevel = self->dyn__level() && il2cpp_functions::class_is_assignable_from(classof(GlobalNamespace::CustomPreviewBeatmapLevel*), il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(self->dyn__level())));
    adjustUI(customLevel);
}

MAKE_HOOK_MATCH(StandardLevelDetailViewController_ShowContent, &GlobalNamespace::StandardLevelDetailViewController::ShowContent, void, GlobalNamespace::StandardLevelDetailViewController* self, ::GlobalNamespace::StandardLevelDetailViewController::ContentType contentType, ::StringW errorText, float downloadingProgress, ::StringW downloadingText)
{
    StandardLevelDetailViewController_ShowContent(self, contentType, errorText, downloadingProgress, downloadingText);

    if (!deleteButton) {
        createActionButton(self->dyn__standardLevelDetailView()->get_practiceButton()->get_transform()->get_parent());
        bool customLevel = self->dyn__previewBeatmapLevel() && il2cpp_functions::class_is_assignable_from(classof(GlobalNamespace::CustomPreviewBeatmapLevel*), il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(self->dyn__previewBeatmapLevel())));
        adjustUI(customLevel);
    }
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
    // Toast = PlaylistEditor::Toast::Create();

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
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), StandardLevelDetailViewController_ShowContent);
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), StandardLevelDetailView_RefreshContent);
    INSTALL_HOOK(PlaylistEditor::Logging::getLogger(), FlowCoordinator_PresentFlowCoordinator);
    INFO("Installed all hooks!");
}