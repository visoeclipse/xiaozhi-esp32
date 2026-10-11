#ifndef DISPLAY_H
#define DISPLAY_H

#include "emoji_collection.h"

#ifndef CONFIG_USE_EMOTE_MESSAGE_STYLE
#define HAVE_LVGL 1
#include <lvgl.h>
#endif

#include <esp_timer.h>
#include <esp_log.h>
#include <esp_pm.h>

#include <string>
#include <chrono>

class Theme {
public:
    Theme(const std::string& name) : name_(name) {}
    virtual ~Theme() = default;

    inline std::string name() const { return name_; }
private:
    std::string name_;
};

/**
 * Display - 显示屏抽象基类
 *
 * 提供统一的显示接口，由具体的 LCD/OLED 实现类继承。
 * 管理状态栏、通知、表情和聊天消息的显示。
 */
class Display {
public:
    Display();
    virtual ~Display();

    /// 设置状态栏文本（如 "Standby"/"Listening"）
    virtual void SetStatus(const char* status);
    /// 显示临时通知（自动消失）
    virtual void ShowNotification(const char* notification, int duration_ms = 3000);
    virtual void ShowNotification(const std::string &notification, int duration_ms = 3000);
    /// 设置表情图标（如 "neutral"/"happy"/"laughing"）
    virtual void SetEmotion(const char* emotion);
    /// 设置聊天消息（role: user/assistant/system）
    virtual void SetChatMessage(const char* role, const char* content);
    /// 清空所有聊天消息
    virtual void ClearChatMessages();
    /// 设置显示主题
    virtual void SetTheme(Theme* theme);
    virtual Theme* GetTheme() { return current_theme_; }
    /// 刷新状态栏（包括 WiFi 信号、电池、时间等）
    virtual void UpdateStatusBar(bool update_all = false);
    /// 设置省电模式（降低刷新率或关闭背光）
    virtual void SetPowerSaveMode(bool on);
    /// 初始化 UI 布局（由具体实现类覆写）
    virtual void SetupUI() { 
        setup_ui_called_ = true;
    }

    /// 屏幕宽度（像素）
    inline int width() const { return width_; }
    /// 屏幕高度（像素）
    inline int height() const { return height_; }
    /// SetupUI() 是否已被调用
    inline bool IsSetupUICalled() const { return setup_ui_called_; }

protected:
    int width_ = 0;
    int height_ = 0;
    bool setup_ui_called_ = false;  ///< 跟踪 SetupUI() 是否已调用

    /// 当前主题
    Theme* current_theme_ = nullptr;

    // 显示锁（LVGL 不是线程安全的，需要加锁保护）
    friend class DisplayLockGuard;
    virtual bool Lock(int timeout_ms = 0) = 0;
    virtual void Unlock() = 0;
};


class DisplayLockGuard {
public:
    DisplayLockGuard(Display *display) : display_(display) {
        if (!display_->Lock(30000)) {
            ESP_LOGE("Display", "Failed to lock display");
        }
    }
    ~DisplayLockGuard() {
        display_->Unlock();
    }

private:
    Display *display_;
};

class NoDisplay : public Display {
private:
    virtual bool Lock(int timeout_ms = 0) override {
        return true;
    }
    virtual void Unlock() override {}
};

#endif
