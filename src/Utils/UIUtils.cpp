#include "Utils/UIUtils.hpp"

#include "CustomTypes/DoubleClickIconButton.hpp"
#include "Utils/Utils.hpp"

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
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/WeakPtrGO.hpp"

namespace PlaylistEditor::Utils
{

UnityEngine::Sprite *FileToSprite(const std::string_view &image_name)
{
    std::string path = string_format(IconPathTemplate.c_str(), image_name.data());
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

}