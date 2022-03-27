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
#include "codegen/include/HMUI/HoverHintController.hpp"
#include "codegen/include/HMUI/ViewController_AnimationDirection.hpp"
#include "codegen/include/HMUI/ScrollView.hpp"
#include "codegen/include/HMUI/TableView.hpp"
#include "codegen/include/HMUI/TableView_ScrollPositionType.hpp"
#include "codegen/include/HMUI/ImageView.hpp"
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

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup
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

static UnityEngine::UI::Button *deleteButton = nullptr;
static HMUI::ImageView *deleteButtonImageView = nullptr;

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

template <class T>
constexpr ArrayW<T> listToArrayW(::System::Collections::Generic::IReadOnlyList_1<T>* list) {
    return ArrayW<T>(reinterpret_cast<Array<T>*>(list));
}

// official pack
static GlobalNamespace::IBeatmapLevelPack* GetCurrentSelectedLevelPack()
{
    if (!LevelCollectionNavigationController)
        return nullptr;

    LevelCollectionNavigationController->dyn__levelPack();
    return LevelCollectionNavigationController->dyn__levelPack();
}

// customn plist
static GlobalNamespace::IAnnotatedBeatmapLevelCollection* GetCurrentSelectedPlaylist()
{
    if (!AnnotatedBeatmapLevelCollectionsViewController)
        return nullptr;

    return AnnotatedBeatmapLevelCollectionsViewController->get_selectedAnnotatedBeatmapLevelCollection();
}

static GlobalNamespace::IAnnotatedBeatmapLevelCollection* GetCurrentSelectedAnnotatedBeatmapLevelCollection()
{
    GlobalNamespace::IAnnotatedBeatmapLevelCollection* collection = reinterpret_cast<GlobalNamespace::IAnnotatedBeatmapLevelCollection*>(GetCurrentSelectedLevelPack());

    if (!collection)
        collection = reinterpret_cast<GlobalNamespace::IAnnotatedBeatmapLevelCollection*>(GetCurrentSelectedPlaylist());

    return collection;
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
            getLogger().log(Logging::INFO, "Could not locate requested level collection...");
            return;
        }

        getLogger().log(Logging::INFO, "Selecting level collection: %s", std::string(levelCollectionName).c_str());
        // getLogger().log(Logging::INFO, "1 Selecting level collection: %s", std::string(levelCollectionName).c_str());

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


        getLogger().log(Logging::INFO, "2 Selecting level collection: %s", std::string(levelCollectionName).c_str());
        // make table to be specific play list
        // LevelFilteringNavigationController->HandleAnnotatedBeatmapLevelCollectionsViewControllerDidSelectAnnotatedBeatmapLevelCollection(collection);


        getLogger().log(Logging::INFO, "Done selecting level collection!");
    }
    catch (const std::exception& e)
    {
        getLogger().log(Logging::ERROR, "Failed to select level collection, err: %s", e.what());
    }
}

MAKE_HOOK_MATCH(FlowCoordinator_PresentFlowCoordinator, &HMUI::FlowCoordinator::PresentFlowCoordinator, void, HMUI::FlowCoordinator* self, HMUI::FlowCoordinator* flowCoordinator, System::Action* finishedCallback, HMUI::ViewController::AnimationDirection animationDirection, bool immediately, bool replaceTopViewController)
{
    FlowCoordinator_PresentFlowCoordinator(self, flowCoordinator, finishedCallback, animationDirection, immediately, replaceTopViewController);
    if (il2cpp_utils::try_cast<GlobalNamespace::SoloFreePlayFlowCoordinator>(flowCoordinator))
    {
        getLogger().log(Logging::INFO, "Initializing PlayListEditor for Single Player Mode");
        LevelSelectionFlowCoordinator = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::SoloFreePlayFlowCoordinator *>().Last();
        // gather flow coordinator elements
        LevelSelectionNavigationController = LevelSelectionFlowCoordinator->dyn_levelSelectionNavigationController();
        getLogger().log(Logging::INFO, "Acquired LevelSelectionNavigationController [%d]", LevelSelectionNavigationController->GetInstanceID());

        LevelFilteringNavigationController = LevelSelectionNavigationController->dyn__levelFilteringNavigationController();
        getLogger().log(Logging::INFO, "Acquired LevelFilteringNavigationController [%d]", LevelFilteringNavigationController->GetInstanceID());

        LevelCollectionNavigationController = LevelSelectionNavigationController->dyn__levelCollectionNavigationController();
        getLogger().log(Logging::INFO, "Acquired LevelCollectionNavigationController [%d]", LevelCollectionNavigationController->GetInstanceID());

        LevelCollectionViewController = LevelCollectionNavigationController->dyn__levelCollectionViewController();
        getLogger().log(Logging::INFO, "Acquired LevelCollectionViewController [%d]", LevelCollectionViewController->GetInstanceID());

        LevelDetailViewController = LevelCollectionNavigationController->dyn__levelDetailViewController();
        getLogger().log(Logging::INFO, "Acquired LevelDetailViewController [%d]", LevelDetailViewController->GetInstanceID());

        LevelCollectionTableView = LevelCollectionViewController->dyn__levelCollectionTableView();
        getLogger().log(Logging::INFO, "Acquired LevelPackLevelsTableView [%d]", LevelCollectionTableView->GetInstanceID());

        StandardLevelDetailView = LevelDetailViewController->dyn__standardLevelDetailView();
        getLogger().log(Logging::INFO, "Acquired StandardLevelDetailView [%d]", StandardLevelDetailView->GetInstanceID());

        BeatmapCharacteristicSelectionViewController = StandardLevelDetailView->dyn__beatmapCharacteristicSegmentedControlController();
        getLogger().log(Logging::INFO, "Acquired BeatmapCharacteristicSegmentedControlController [%d]", BeatmapCharacteristicSelectionViewController->GetInstanceID());

        LevelDifficultyViewController = StandardLevelDetailView->dyn__beatmapDifficultySegmentedControlController();
        getLogger().log(Logging::INFO, "Acquired BeatmapDifficultySegmentedControlController [%d]", LevelDifficultyViewController->GetInstanceID());

        AnnotatedBeatmapLevelCollectionsViewController = LevelFilteringNavigationController->dyn__annotatedBeatmapLevelCollectionsViewController();
        getLogger().log(Logging::INFO, "Acquired AnnotatedBeatmapLevelCollectionsViewController from LevelFilteringNavigationController [%d]", AnnotatedBeatmapLevelCollectionsViewController->GetInstanceID());

        LevelCategoryViewController = LevelFilteringNavigationController->dyn__selectLevelCategoryViewController();
        getLogger().log(Logging::INFO, "Acquired LevelCategoryViewController from LevelFilteringNavigationController [%d]", LevelCategoryViewController->GetInstanceID());

        BeatmapLevelsModel = LevelFilteringNavigationController->dyn__beatmapLevelsModel();
        getLogger().log(Logging::INFO, "Acquired BeatmapLevelsModel [%d]", BeatmapLevelsModel->GetInstanceID());
    }
}

template <class T>
static void listAllName(UnityEngine::Transform *parent, const std::string &prefix = "") {
    getLogger().log(Logging::INFO, "%s #p: tag: %s, name: %s, id: %u", prefix.c_str(), std::string(parent->get_tag()).c_str(), std::string(parent->get_name()).c_str(), parent->GetInstanceID());
    auto childs = parent->GetComponentsInChildren<T *>();
    // auto childs = parent->GetComponentsInChildren<UnityEngine::Transform *>();
    // auto childs = parent->GetComponentsInChildren<TMPro::TextMeshProUGUI *>();
    // auto childs = parent->GetComponentsInChildren<UnityEngine::UI::Button *>();
    // auto childs = parent->GetComponentsInChildren<GlobalNamespace::StandardLevelDetailViewController *>();
    for (size_t i = 0; i < childs.Length(); i++)
    {
        if (parent->GetInstanceID() == childs.get(i)->GetInstanceID())
            continue;
        getLogger().log(Logging::INFO, "%s #%zu: tag: %s, name: %s, id: %u", prefix.c_str(), i, std::string(childs.get(i)->get_tag()).c_str(), std::string(childs.get(i)->get_name()).c_str(), childs.get(i)->GetInstanceID());
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
        getLogger().log(Logging::INFO, "Inactive level delete button, id: %u", deleteButtonTransform->GetInstanceID());
        deleteButtonTransform->get_gameObject()->SetActive(false);
    }
}

static void logPacks(::StringW lastCollectionName = "") {
    auto packArr = BeatmapLevelsModel->get_customLevelPackCollection()->get_beatmapLevelPacks();
    for (int j = 0; j < packArr.Length(); j++)
    {
        auto tmp = listToArrayW(((GlobalNamespace::IAnnotatedBeatmapLevelCollection *)(packArr.get(j)))->get_beatmapLevelCollection()->get_beatmapLevels());
        getLogger().log(Logging::INFO, "pack #%d %s has %lu", j, std::string(packArr.get(j)->get_packName()).c_str(), tmp.Length());
        // if (!packArr.get(j)->get_packName()->Equals(lastCollectionName))
        //     continue;
        // for (int i = 0; i < tmp.Length(); i++)
        //     getLogger().log(Logging::INFO, "    pack #%d %s", i, std::string(tmp[i]->get_songName()).c_str());
    }
}

std::function<void()> getDeleteFunction() {
    static std::function<void()> deleteFunction = (std::function<void()>) [] () {
        if (!LevelCollectionTableView) {
            getLogger().log(Logging::INFO, "Null LevelCollectionTableView");
            return;
        }
        if (!deleteButton) {
            getLogger().log(Logging::INFO, "Null deleteButton");
            return;
        }

        if (UnityEngine::Color::get_red() != deleteButtonImageView->get_color()) {
            deleteButtonImageView->set_color(UnityEngine::Color::get_red());
            return;
        }

        static Il2CppClass* customPreviewBeatmapLevelClass = classof(GlobalNamespace::CustomPreviewBeatmapLevel*);
        bool customLevel = LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel() && il2cpp_functions::class_is_assignable_from(customPreviewBeatmapLevelClass, il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel())));
        if(!customLevel) {
            getLogger().log(Logging::INFO, "Not custom level");
            return;
        }

        int selectedRow = LevelCollectionTableView->dyn__selectedRow();
        int nextSelectedRow = (selectedRow < LevelCollectionTableView->NumberOfCells() - 1) ? selectedRow : LevelCollectionTableView->NumberOfCells() - 2;
        auto lastCollectionIdx = AnnotatedBeatmapLevelCollectionsViewController->dyn__selectedItemIndex();
        // auto lastCollectionName = GetCurrentSelectedAnnotatedBeatmapLevelCollection()->get_collectionName();
        auto lastScrollPos = LevelCollectionTableView->dyn__tableView()->get_scrollView()->dyn__destinationPos();
        GlobalNamespace::CustomPreviewBeatmapLevel *selectedlevel = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(LevelCollectionTableView->dyn__selectedPreviewBeatmapLevel());

        getLogger().log(Logging::INFO, "Delete %d/%d song %s at %s in list idx %d",
                        selectedRow, LevelCollectionTableView->NumberOfCells(), // if at customn category, cells will add a pack header cell
                        std::string(selectedlevel->get_songName()).c_str(), std::string(selectedlevel->get_customLevelPath()).c_str(),
                        AnnotatedBeatmapLevelCollectionsViewController->dyn__selectedItemIndex());

        if (LevelCollectionTableView->NumberOfCells() <= 1) {
            getLogger().log(Logging::INFO, "No song remain after");
        }
        RuntimeSongLoader::API::DeleteSong(std::string(selectedlevel->get_customLevelPath()),
            [lastScrollPos, nextSelectedRow, lastCollectionIdx] {
                getLogger().log(Logging::INFO, "Success delete song");

                RuntimeSongLoader::API::RefreshSongs(true,
                [lastScrollPos, nextSelectedRow, lastCollectionIdx] (const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&) {
                    getLogger().log(Logging::INFO, "Success refresh song");

                    // select level collection
                    AnnotatedBeatmapLevelCollectionsViewController->SetData(AnnotatedBeatmapLevelCollectionsViewController->dyn__annotatedBeatmapLevelCollections(), lastCollectionIdx, false);

                    // select level
                    // LevelCollectionTableView->SelectLevel(nextPreviewBeatmapLevels); // this will jump to center
                    LevelCollectionTableView->dyn__tableView()->SelectCellWithIdx(nextSelectedRow, true); // select but scroll at head
                    LevelCollectionTableView->dyn__tableView()->get_scrollView()->ScrollTo(lastScrollPos, false);
                });
            }
        );
    };
    return deleteFunction;
}

#define MakeDelegate(DelegateType, varName) (il2cpp_utils::MakeDelegate<DelegateType>(classof(DelegateType), varName))

UnityEngine::UI::Button* CreateBaseButton(std::string_view name, UnityEngine::Transform* parent, std::string_view buttonTemplate)
{
    Il2CppString* templCS = il2cpp_utils::newcsstr(buttonTemplate.data());
    auto btn = UnityEngine::Object::Instantiate((UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::UI::Button*>()).Last([&](UnityEngine::UI::Button* x) {
        return x->get_name()->Equals(templCS);
    }), parent, false);

    btn->set_name(il2cpp_utils::newcsstr(name.data()));
    btn->set_interactable(true);
    return btn;
}

void SetHoverHint(UnityEngine::Transform* button, std::string_view name, std::string_view text)
{
    auto hover = button->get_gameObject()->AddComponent<HMUI::HoverHint*>();
    hover->set_text(text);
    hover->set_name(name);
    hover->dyn__hoverHintController() = UnityEngine::Resources::FindObjectsOfTypeAll<HMUI::HoverHintController*>().First();
}

UnityEngine::UI::Button* CreateIconButton(std::string_view name, UnityEngine::Transform* parent, std::string_view buttonTemplate, UnityEngine::Sprite* icon, std::string_view hint)
{
    auto btn = CreateBaseButton(name, parent, buttonTemplate);

    SetHoverHint(btn->get_transform(), string_format("%s_hoverHintText", name.data()), hint);
    btn->get_gameObject()->AddComponent<QuestUI::ExternalComponents*>()->Add(
        btn->GetComponentsInChildren<UnityEngine::UI::LayoutGroup*>().First([](auto x) -> bool {
        return x->get_gameObject()->get_name()->Equals("Content");
    }));

    UnityEngine::Transform* contentTransform = btn->get_transform()->Find("Content");
    UnityEngine::Object::Destroy(contentTransform->Find("Text")->get_gameObject());
    UnityEngine::UI::Image* iconImage = UnityEngine::GameObject::New_ctor("Icon")->AddComponent<HMUI::ImageView*>();
    // idk what mat that is

    auto orig = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::UI::Button*>().Last([&](UnityEngine::UI::Button* x) {
        return x->get_name()->Equals(buttonTemplate.data());
    });

    iconImage->set_material(orig->get_gameObject()->GetComponentInChildren<HMUI::ImageView*>()->get_material());
    iconImage->get_rectTransform()->SetParent(contentTransform, false);
    iconImage->get_rectTransform()->set_sizeDelta(UnityEngine::Vector2(10.0f, 10.0f));
    iconImage->set_sprite(icon);
    iconImage->set_preserveAspect(true);

    if (iconImage)
    {
        // auto btnIcon = btn->get_gameObject()->AddComponent<SongBrowser::Components::ButtonIconImage*>();
        // btnIcon->image = iconImage;

    }

    UnityEngine::Object::Destroy(btn->get_transform()->Find("Content")->GetComponent<UnityEngine::UI::LayoutElement*>());
    btn->GetComponentsInChildren<UnityEngine::RectTransform*>().First([](auto x) -> bool {
        return x->get_name()->Equals("Underline");
    })->get_gameObject()->SetActive(false);

    auto buttonSizeFitter = btn->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    buttonSizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);
    buttonSizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);

    btn->set_onClick(UnityEngine::UI::Button::ButtonClickedEvent::New_ctor());
    return btn;
}

UnityEngine::UI::Button* CreateIconButton(std::string_view name, UnityEngine::Transform* parent, std::string_view buttonTemplate, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, std::function<void(void)> onClick, UnityEngine::Sprite* icon, std::string_view hint)
{
    auto btn = CreateIconButton(name, parent, buttonTemplate, icon, hint);
    auto rect = reinterpret_cast<UnityEngine::RectTransform*>(btn->get_transform());
    rect->set_anchorMin(UnityEngine::Vector2(0.5f, 0.5f));
    rect->set_anchorMax(UnityEngine::Vector2(0.5f, 0.5f));
    rect->set_anchoredPosition(anchoredPosition);
    rect->set_sizeDelta(sizeDelta);

    if (onClick)
        btn->get_onClick()->AddListener(MakeDelegate(UnityEngine::Events::UnityAction*, onClick));

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

static void setOnClickLevelDeleteButton(UnityEngine::Transform *parent) {
    if (!deleteButton) {
        auto deleteButtonTransform = parent->FindChild("DeleteLevelButton");
        if (!deleteButtonTransform)
            return;
        getLogger().log(Logging::INFO, "SetOnClick level delete button, id: %u", deleteButtonTransform->GetInstanceID());
        deleteButton = deleteButtonTransform->get_gameObject()->GetComponent<UnityEngine::UI::Button*>();

        auto comps = deleteButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([&] (auto x) -> bool {
            return "Icon" == x->get_name();
        });
        if (comps) {
            deleteButtonImageView = comps;
        }
        deleteButton->set_onClick(UnityEngine::UI::Button::ButtonClickedEvent::New_ctor());
        deleteButton->get_onClick()->AddListener(MakeDelegate(UnityEngine::Events::UnityAction*, getDeleteFunction()));

        auto posX = -22.5f;
        auto deleteAndRemoveButton = CreateIconButton("DeleteAndRemoveFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                             UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(20.0f,7.0f), getDeleteFunction(),
                                             FileToSprite("DeleteAndRemoveIcon"), "Delete and Remove Song from List");
        deleteAndRemoveButton->get_transform()->SetAsLastSibling();

        posX += 15.0f + 1.25f ;
        auto removeButton = CreateIconButton("RemoveFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                             UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
                //     OnClearButtonClickEvent();
            }, FileToSprite("RemoveIcon"), "Remove Song from List");
        removeButton->get_transform()->SetAsLastSibling();

        posX += 10.0f + 1.25f;
        auto insertButton = CreateIconButton("InsertFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                             UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
                //     OnClearButtonClickEvent();
            }, FileToSprite("InsertIcon"), "Insert to List");
        insertButton->get_transform()->SetAsLastSibling();

        posX += 10.0f + 1.25f;
        auto moveUpButton = CreateIconButton("MoveUpFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                             UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
                //     OnClearButtonClickEvent();
            }, FileToSprite("MoveUpIcon"), "Move Up Song from List");
        moveUpButton->get_transform()->SetAsLastSibling();

        posX += 10.0f + 1.25f;
        auto moveDownButton = CreateIconButton("MoveDownFromListButton", deleteButtonTransform->get_parent()->get_parent(), "PracticeButton",
                                             UnityEngine::Vector2(posX, -15.0f), UnityEngine::Vector2(10.0f,7.0f), [&]() {
                //     OnClearButtonClickEvent();
            }, FileToSprite("MoveDownIcon"), "Move Down Song from List");
        moveDownButton->get_transform()->SetAsLastSibling();
    }
}

MAKE_HOOK_MATCH(StandardLevelDetailView_RefreshContent, &GlobalNamespace::StandardLevelDetailView::RefreshContent, void, GlobalNamespace::StandardLevelDetailView* self)
{
    StandardLevelDetailView_RefreshContent(self);

    if (deleteButtonImageView)
        deleteButtonImageView->set_color(UnityEngine::Color::get_white());
}

MAKE_HOOK_MATCH(StandardLevelDetailViewController_ShowContent, &GlobalNamespace::StandardLevelDetailViewController::ShowContent, void, GlobalNamespace::StandardLevelDetailViewController* self, ::GlobalNamespace::StandardLevelDetailViewController::ContentType contentType, ::StringW errorText, float downloadingProgress, ::StringW downloadingText)
{
    StandardLevelDetailViewController_ShowContent(self, contentType, errorText, downloadingProgress, downloadingText);

    setOnClickLevelDeleteButton(self->dyn__standardLevelDetailView()->get_practiceButton()->get_transform()->get_parent());
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;

    getConfig().Load(); // Load the config file
    getLogger().log(Logging::INFO, "Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();

    RuntimeSongLoader::API::AddRefreshLevelPacksEvent(
        [] (RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO) {
            getLogger().log(Logging::INFO, "SSSSS AddSongsLoadedEvent %d", customBeatmapLevelPackCollectionSO->GetInstanceID());
            PlaylistManager::LoadPlaylists(customBeatmapLevelPackCollectionSO, true);
            // ::ArrayW<::GlobalNamespace::IBeatmapLevelPack*> beatmapLevelPacks = (reinterpret_cast<GlobalNamespace::IBeatmapLevelPackCollection*>(customBeatmapLevelPackCollectionSO))->get_beatmapLevelPacks();
            // for (int i = 0; i < beatmapLevelPacks.Length(); i++)
            //     getLogger().log(Logging::INFO, "SSSSS    pack #%d [%s] [%s] [%s]", i, std::string(beatmapLevelPacks[i]->get_packID()).c_str(), std::string(beatmapLevelPacks[i]->get_packName()).c_str(), std::string(beatmapLevelPacks[i]->get_shortPackName()).c_str());
        }
    );

    getLogger().log(Logging::INFO, "Installing hooks...");
    INSTALL_HOOK(getLogger(), StandardLevelDetailViewController_ShowContent);
    INSTALL_HOOK(getLogger(), StandardLevelDetailView_RefreshContent);
    INSTALL_HOOK(getLogger(), FlowCoordinator_PresentFlowCoordinator);
    getLogger().log(Logging::INFO, "Installed all hooks!");
}