#include "toast.hpp"
#include "logging.hpp"

#include <assert.h>
#include <chrono>

#include "UnityEngine/WaitForEndOfFrame.hpp"
#include "codegen/include/System/Collections/IEnumerator.hpp"
#include "custom-types/shared/coroutine.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace TMPro;
using namespace HMUI;
using namespace QuestUI;

namespace PlaylistEditor {

Toast::Toast() :
    running_(false),
    thread_()
{}

Toast::~Toast()
{
    INFO("TTTTT ~Toast");
    Hide();
}

void Toast::CreateCanvas() {
    if(!canvas) {
        canvas = BeatSaberUI::CreateCanvas();
        canvas->AddComponent<CurvedCanvasSettings*>()->SetRadius(100.0f);
        RectTransform* transform = canvas->GetComponent<RectTransform*>();
        transform->set_position(UnityEngine::Vector3(0.0f, 0.1f, 4.2f));
        transform->set_eulerAngles(UnityEngine::Vector3(24.0f, 0.0f, 0.0f));
        VerticalLayoutGroup* layout = BeatSaberUI::CreateVerticalLayoutGroup(transform);
        GameObject* layoutGameObject = layout->get_gameObject();
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

void Toast::Show(std::string text, long long ms)
{
    INFO("Next Show 0");
    if (isShowing())
        Hide();
    running_ = true;
    INFO("Next Show 1");

    assert(thread_.get_id() != std::this_thread::get_id());

    if (!canvas)
        CreateCanvas();
    textObject->set_text(text);

    thread_ = std::thread([this, ms] () {
        INFO("TTTTT SetActive(true)");
        canvas->SetActive(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        INFO("TTTTT SetActive(false)");
        reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForEndOfFrame::New_ctor());
        canvas->SetActive(false);
        INFO("TTTTT thread end");
    }); // end thread_
}

void Toast::Hide()
{
    assert(thread_.get_id() != std::this_thread::get_id());
    cv_timer_.notify_one();
    if (thread_.joinable())
        thread_.join();
    else
        INFO("TTTTT QQQQQQQQQQQQQQQQ thread join failed");
    running_ = false;
}

bool Toast::isShowing() const
{
    return running_;
}

}