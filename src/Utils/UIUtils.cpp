#include "Utils/UIUtils.hpp"

#include <fstream>

#include "CustomTypes/DoubleClickIconButton.hpp"
#include "Utils/Utils.hpp"

#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/InputFieldView_InputFieldChanged.hpp"
#include "Polyglot/LocalizedTextMeshProUGUI.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "UnityEngine/WaitForFixedUpdate.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/WeakPtrGO.hpp"

namespace PlaylistEditor::Utils
{

UnityEngine::Sprite *FileToSprite(const std::string_view &image_name)
{
    std::string path = string_format(IconPathTemplate.c_str(), image_name.data());

    if (!std::filesystem::is_regular_file(path)) {
        ERROR("Failed to load sprite from file %s", path.c_str());
        return UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>().First();
    }
    return QuestUI::BeatSaberUI::FileToSprite(path);
}

UnityEngine::UI::Button *CreateIconButton(const std::string_view &name, UnityEngine::Transform *parent, const std::string_view &buttonTemplate,
                                          const UnityEngine::Vector2 &anchoredPosition, const UnityEngine::Vector2 &sizeDelta,
                                          const std::function<void(void)> &onClick, UnityEngine::Sprite *icon, const std::string_view &hint)
{
    static UnityEngine::Material *templateMaterial = nullptr;

    if (!templateMaterial) {
        auto practiceButton = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::UI::Button*>().First([] (auto x) { return x->get_name() == "PracticeButton"; } );
        if (practiceButton)
            templateMaterial = practiceButton->get_gameObject()->GetComponentInChildren<HMUI::ImageView*>()->get_material();
    }
    auto btn = QuestUI::BeatSaberUI::CreateUIButton(parent, "", buttonTemplate, anchoredPosition, sizeDelta, onClick);

    QuestUI::BeatSaberUI::AddHoverHint(btn->get_gameObject(), hint);

    UnityEngine::Object::Destroy(btn->get_transform()->Find("Underline")->get_gameObject());

    UnityEngine::Transform *contentTransform = btn->get_transform()->Find("Content");
    UnityEngine::Object::Destroy(contentTransform->Find("Text")->get_gameObject());
    UnityEngine::Object::Destroy(contentTransform->GetComponent<UnityEngine::UI::LayoutElement*>());

    UnityEngine::UI::Image *iconImage = UnityEngine::GameObject::New_ctor("Icon")->AddComponent<HMUI::ImageView*>();
    if (templateMaterial)
        iconImage->set_material(templateMaterial);
    iconImage->get_rectTransform()->SetParent(contentTransform, false);
    iconImage->get_rectTransform()->set_sizeDelta(UnityEngine::Vector2(10.0f, 10.0f));
    iconImage->set_sprite(icon);
    iconImage->set_preserveAspect(true);

    return btn;
}

HMUI::InputFieldView *CreateStringInput(UnityEngine::Transform *parent, const StringW &settingsName, const StringW &currentValue,
                                        const UnityEngine::Vector2 &anchoredPosition, const float width,
                                        const std::function<void(StringW)> &onEnter) {
    auto originalFieldView = UnityEngine::Resources::FindObjectsOfTypeAll<HMUI::InputFieldView *>().First(
        [] (HMUI::InputFieldView *x) {
            return x->get_name() == "GuestNameInputField";
        }
    );
    UnityEngine::GameObject *gameObj = UnityEngine::Object::Instantiate(originalFieldView->get_gameObject(), parent, false);
    gameObj->set_name("QuestUIStringInput");

    UnityEngine::RectTransform *rectTransform = gameObj->GetComponent<UnityEngine::RectTransform*>();
    rectTransform->SetParent(parent, false);
    rectTransform->set_anchoredPosition(anchoredPosition);
    rectTransform->set_sizeDelta(UnityEngine::Vector2(width, 10.0f));
    rectTransform->set_localScale({0.75, 0.75, 1});

    HMUI::InputFieldView *fieldView = gameObj->GetComponent<HMUI::InputFieldView*>();
    fieldView->dyn__useGlobalKeyboard() = true;
    fieldView->dyn__textLengthLimit() = 28;

    fieldView->Awake();

    std::function<void()> enterFunction = (std::function<void()>) [fieldView, onEnter] () {
        if (onEnter)
            onEnter(fieldView->get_text());
    };
    auto clearButton = fieldView->get_gameObject()->Find("ClearButton")->GetComponent<UnityEngine::UI::Button*>();
    clearButton->set_onClick(UnityEngine::UI::Button::ButtonClickedEvent::New_ctor());
    clearButton->get_onClick()->AddListener(MakeDelegate<UnityEngine::Events::UnityAction*>(enterFunction));
    QuestUI::BeatSaberUI::SetButtonIcon(clearButton, FileToSprite("EnterIcon"));

    UnityEngine::Object::Destroy(fieldView->dyn__placeholderText()->GetComponent<Polyglot::LocalizedTextMeshProUGUI*>());
    fieldView->dyn__placeholderText()->GetComponent<TMPro::TextMeshProUGUI*>()->SetText(settingsName);
    fieldView->SetText(currentValue);

    return fieldView;
}

static HMUI::ModalView *CreateRestoreDialog(UnityEngine::Transform *parent, const std::function<void(void)> &onOK, const std::function<void(void)> &onCancel)
{
    const int width = 65;
    const int height = 33+8.5;
    auto restoreDialogPromptModal = QuestUI::BeatSaberUI::CreateModal(parent, UnityEngine::Vector2(width, height), nullptr, false);

    auto restoreButton = QuestUI::BeatSaberUI::CreateUIButton(restoreDialogPromptModal->get_transform(), "Restore", "ActionButton",
                                                              UnityEngine::Vector2(-width/4, -height/2 + 5.5), [restoreDialogPromptModal, onOK] {
        restoreDialogPromptModal->Hide(true, nullptr);
        onOK();
    });
    UnityEngine::Object::Destroy(restoreButton->get_transform()->Find("Content")->GetComponent<UnityEngine::UI::LayoutElement*>());

    auto cancelButton = QuestUI::BeatSaberUI::CreateUIButton(restoreDialogPromptModal->get_transform(), "Cancel",
                                                             UnityEngine::Vector2(width/4, -height/2 + 5.5), [restoreDialogPromptModal, onCancel] {
        restoreDialogPromptModal->Hide(true, nullptr);
        onCancel();
    });
    UnityEngine::Object::Destroy(cancelButton->get_transform()->Find("Content")->GetComponent<UnityEngine::UI::LayoutElement*>());

    TMPro::TextMeshProUGUI* title = QuestUI::BeatSaberUI::CreateText(restoreDialogPromptModal->get_transform(), "PLAYLIST EDITOR", false, {0, height/2 - 5.5}, {width-5, 8.5});
    title->set_alignment(TMPro::TextAlignmentOptions::Center);
    title->set_fontStyle(TMPro::FontStyles::_get_Bold());

    TMPro::TextMeshProUGUI* message = QuestUI::BeatSaberUI::CreateText(restoreDialogPromptModal->get_transform(),
                                      "Playlists was modified, changes made by Playlist Editor may lost, do you want to restore playlists with backup file?", false, {0, 0.5}, {width-5, 25.5});  // line height 8.5
    message->set_enableWordWrapping(true);
    message->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
    message->set_alignment(TMPro::TextAlignmentOptions::Center);

    restoreDialogPromptModal->get_transform()->SetAsLastSibling();
    return restoreDialogPromptModal;
}

static custom_types::Helpers::Coroutine DoShowRestoreDialog(UnityEngine::Transform *parent,
                                                            const std::function<void(void)> onOK, const std::function<void(void)> onCancel) // can not be reference, will crash
{
    co_yield reinterpret_cast<System::Collections::IEnumerator *>(UnityEngine::WaitForFixedUpdate::New_ctor());

    auto prompt = CreateRestoreDialog(parent, onOK, onCancel);
    if (prompt)
        prompt->Show(true, true, nullptr);

    co_return;
}

void ShowRestoreDialog(UnityEngine::Transform *parent, const std::function<void(void)> &onOK, const std::function<void(void)> &onCancel)
{
    // use coroutine or will showing without focus
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(DoShowRestoreDialog(parent, onOK, onCancel)));
}

}