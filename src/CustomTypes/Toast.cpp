#include "CustomTypes/Toast.hpp"

#include <chrono>

#include "CustomTypes/Logging.hpp"
#include "Backport/typedefs-string.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "GlobalNamespace/HMTask.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "HMUI/CurvedCanvasSettings.hpp"
#include "System/Action.hpp"

#define ACTIVE_TIME 2000

using namespace PlaylistEditor;
using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace QuestUI;
using namespace HMUI;
using namespace TMPro;

DEFINE_TYPE(PlaylistEditor, Toast);

Toast *Toast::Instance = nullptr;

Toast *Toast::GetInstance() {
    if(!Instance) {
        static ConstString name("Toast");
        auto gameObject = GameObject::New_ctor(name);
        GameObject::DontDestroyOnLoad(gameObject);
        Instance = gameObject->AddComponent<Toast*>();
    }
    return Instance;
}

void Toast::ctor() {
    INVOKE_CTOR();
    IsLoading = false;
    HasLoaded = false;
    LoadingCancelled = false;
}

void Toast::Awake() {
    if(IsLoading)
        LoadingCancelled = true;
}

void Toast::Update() {
    UpdateState();
}

void Toast::ShowMessage(const std::string &text) {
    if(IsLoading)
        return;

    IsLoading = true;
    HasLoaded = false;

    HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
        (std::function<void()>)[=] {

            auto start = std::chrono::high_resolution_clock::now();

            UpdateLoadingProgress(text);

            QuestUI::MainThreadScheduler::Schedule(
                [this] {
                    IsLoading = false;
                    HasLoaded = true;
                }
            );
        }
    ), nullptr)->Run();
}

void Toast::CreateCanvas() {
    if(!canvas) {
        canvas = BeatSaberUI::CreateCanvas();
        canvas->AddComponent<CurvedCanvasSettings*>()->SetRadius(100.0f);
        RectTransform *transform = canvas->GetComponent<RectTransform*>();
        transform->set_position(UnityEngine::Vector3(0.0f, 0.1f, 4.2f));
        transform->set_eulerAngles(UnityEngine::Vector3(24.0f, 0.0f, 0.0f));
        VerticalLayoutGroup *layout = BeatSaberUI::CreateVerticalLayoutGroup(transform);
        GameObject *layoutGameObject = layout->get_gameObject();
        layoutGameObject->GetComponent<ContentSizeFitter*>()->set_verticalFit(ContentSizeFitter::FitMode::PreferredSize);
        static ConstString bgName("round-rect-panel");
        layoutGameObject->AddComponent<Backgroundable*>()->ApplyBackgroundWithAlpha(bgName, 0.50f);
        layout->set_padding(UnityEngine::RectOffset::New_ctor(2, 2, 1, 1));
        textObject = BeatSaberUI::CreateText(layout->get_transform(), "");
        textObject->set_alignment(TextAlignmentOptions::Center);
        textObject->set_fontSize(3.8f);
        Object::DontDestroyOnLoad(canvas);
    }
}

void Toast::UpdateLoadingProgress(const std::string &text) {
    newText = text;
    needsUpdate = true;
}

void Toast::SetActive(bool active) {
    isActive = active;
    if(active)
        lastActive = std::chrono::high_resolution_clock::now();
    if(canvas)
        canvas->SetActive(active);
}

void Toast::UpdateState() {
    if(!canvas) {
        CreateCanvas();
        return;
    }

    if(needsUpdate) {
        needsUpdate = false;
        SetActive(true);
        if(textObject)
            textObject->set_text(il2cpp_utils::newcsstr(newText));
    }

    if(!isActive)
        return;

    std::chrono::milliseconds delay = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastActive);
    if(delay.count() > ACTIVE_TIME)
        SetActive(false);
}
