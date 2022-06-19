#include "Backport/BeatSaberUI.hpp"

#include "CustomTypes/Logging.hpp"
#include "Libraries/HM/HMLib/VR/HapticPresetSO.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Resources.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace UnityEngine::Events;
using namespace TMPro;
using namespace HMUI;

using HapticPresetSO = Libraries::HM::HMLib::VR::HapticPresetSO;
static SafePtr<HapticPresetSO> hapticFeedbackPresetSO;

namespace QuestUI::BeatSaberUI {

    ClickableText* CreateClickableText(UnityEngine::Transform* parent, StringW text, bool italic, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, std::function<void()> onClick)
    {
        static ConstString name("QuestUIText");
        GameObject* gameObj = GameObject::New_ctor(name);
        gameObj->SetActive(false);

        ClickableText* textMesh = gameObj->AddComponent<ClickableText*>();
        RectTransform* rectTransform = textMesh->get_rectTransform();
        rectTransform->SetParent(parent, false);
        textMesh->set_font(GetMainTextFont());
        textMesh->set_fontSharedMaterial(GetMainUIFontMaterial());
        if (italic) text = StringW("<i>" + text + "</i>");
        textMesh->set_text(text);
        textMesh->set_fontSize(4.0f);
        textMesh->set_color(UnityEngine::Color::get_white());
        textMesh->set_richText(true);
        rectTransform->set_anchorMin(UnityEngine::Vector2(0.5f, 0.5f));
        rectTransform->set_anchorMax(UnityEngine::Vector2(0.5f, 0.5f));
        rectTransform->set_anchoredPosition(anchoredPosition);
        rectTransform->set_sizeDelta(sizeDelta);

        gameObj->AddComponent<LayoutElement*>();

        if (onClick)
            textMesh->get_onPointerClickEvent() += [onClick](auto _){ onClick(); };

        // try
        // {
        //     auto menuShockWave = Resources::FindObjectsOfTypeAll<GlobalNamespace::MenuShockwave*>().First();
        //     auto buttonClickedSignal = menuShockWave->buttonClickEvents.Last();
        //     textMesh->buttonClickedSignal = buttonClickedSignal;
        // }
        // catch(const std::exception& e)
        // {
        //     ERROR("%s", e.what());
        // }

        if (!hapticFeedbackPresetSO)
        {
            hapticFeedbackPresetSO.emplace(UnityEngine::ScriptableObject::CreateInstance<HapticPresetSO*>());
            hapticFeedbackPresetSO->duration = 0.02f;
            hapticFeedbackPresetSO->strength = 1.0f;
            hapticFeedbackPresetSO->frequency = 0.2f;
        }

        auto hapticFeedbackController = UnityEngine::Object::FindObjectOfType<GlobalNamespace::HapticFeedbackController*>();
        textMesh->hapticFeedbackController = hapticFeedbackController;
        textMesh->hapticFeedbackPresetSO = (HapticPresetSO*)hapticFeedbackPresetSO;

        gameObj->SetActive(true);
        return textMesh;
    }

    ClickableText* CreateClickableText(UnityEngine::Transform* parent, StringW text, bool italic, std::function<void()> onClick)
    {
        return CreateClickableText(parent, text, italic, UnityEngine::Vector2(0.0f, 0.0f), UnityEngine::Vector2(60.0f, 10.0f), onClick);
    }

}