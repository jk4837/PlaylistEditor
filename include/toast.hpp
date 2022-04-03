#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "GlobalNamespace/HMTask.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "HMUI/CurvedCanvasSettings.hpp"
#include "System/Action.hpp"
#include "System/Threading/Thread.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"


namespace PlaylistEditor {

class Toast final
{
public:
    Toast();
    Toast(const Toast&) = delete;
    Toast(Toast&&) = delete;
    Toast& operator=(const Toast&) = delete;
    Toast& operator=(Toast&&) = delete;
    ~Toast();

public:
    void Show(std::string text, long long ms = 2000);
    void Hide();

private:
    bool isShowing() const;
    void CreateCanvas();

    std::atomic<bool> running_ = false;
    std::mutex mutex_;
    std::condition_variable cv_timer_;
    std::thread thread_;

    UnityEngine::GameObject* canvas = nullptr;
    TMPro::TextMeshProUGUI* textObject = nullptr;
};

}