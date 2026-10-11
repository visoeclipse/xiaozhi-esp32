#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <esp_timer.h>

#include <string>
#include <mutex>
#include <deque>
#include <memory>

#include "protocol.h"
#include "ota.h"
#include "audio_service.h"
#include "device_state.h"
#include "device_state_machine.h"

/**
 * 主事件位定义
 *
 * Application::Run() 的主循环通过 FreeRTOS Event Group 等待这些事件。
 * 任意任务或回调可通过 xEventGroupSetBits() 发送事件，
 * 由主循环统一处理，避免多线程竞争。
 */
#define MAIN_EVENT_SCHEDULE             (1 << 0)  ///< 有待执行的主任务回调
#define MAIN_EVENT_SEND_AUDIO           (1 << 1)  ///< 音频发送队列有新数据
#define MAIN_EVENT_WAKE_WORD_DETECTED   (1 << 2)  ///< 检测到唤醒词
#define MAIN_EVENT_VAD_CHANGE           (1 << 3)  ///< 语音活动检测（VAD）状态变化
#define MAIN_EVENT_ERROR                (1 << 4)  ///< 发生错误（如网络异常）
#define MAIN_EVENT_ACTIVATION_DONE      (1 << 5)  ///< 设备激活流程完成
#define MAIN_EVENT_CLOCK_TICK           (1 << 6)  ///< 1 秒定时器触发（驱动状态栏刷新）
#define MAIN_EVENT_NETWORK_CONNECTED    (1 << 7)  ///< 网络连接成功
#define MAIN_EVENT_NETWORK_DISCONNECTED (1 << 8)  ///< 网络断开
#define MAIN_EVENT_TOGGLE_CHAT          (1 << 9)  ///< 切换对话状态（按键触发）
#define MAIN_EVENT_START_LISTENING      (1 << 10) ///< 请求开始收听
#define MAIN_EVENT_STOP_LISTENING       (1 << 11) ///< 请求停止收听
#define MAIN_EVENT_STATE_CHANGED        (1 << 12) ///< 设备状态机发生转换


enum AecMode {
    kAecOff,           ///< 回声消除关闭
    kAecOnDeviceSide,  ///< 设备端回声消除（需要硬件支持）
    kAecOnServerSide,  ///< 服务器端回声消除
};

/**
 * Application - 应用核心类（单例模式）
 *
 * 管理设备完整生命周期：
 * - 初始化显示、音频、网络等子系统
 * - 运行主事件循环，统一分发所有事件
 * - 管理设备状态机转换
 * - 管理通信协议（WebSocket / MQTT）的建立与销毁
 * - 处理 OTA 升级和设备激活流程
 */
class Application {
public:
    /// 获取全局单例实例
    static Application& GetInstance() {
        static Application instance;
        return instance;
    }
    // 单例模式：禁止拷贝和赋值
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /**
     * 初始化应用
     * 依次设置：显示屏 → 音频服务 → 状态机监听 → MCP 工具 → 网络回调
     * 网络连接异步启动，不会阻塞此函数
     */
    void Initialize();

    /**
     * 运行主事件循环
     * 此函数在主任务中运行且永不返回。
     * 通过 Event Group 等待并处理所有事件（网络/状态变化/用户交互/音频等）。
     */
    void Run();

    /// 获取当前设备状态
    DeviceState GetDeviceState() const { return state_machine_.GetState(); }
    /// 是否检测到用户正在说话（VAD 结果）
    bool IsVoiceDetected() const { return audio_service_.IsVoiceDetected(); }
    
    /**
     * 请求设备状态转换
     * @param state 目标状态
     * @return true 转换成功；false 非法转换（被状态机拒绝）
     */
    bool SetDeviceState(DeviceState state);

    /**
     * 调度一个回调到主任务中执行（线程安全）
     * 任何任务都可以调用此方法，回调会在主循环中被安全执行
     */
    void Schedule(std::function<void()>&& callback);

    /**
     * 显示警告信息（状态栏 + 表情 + 聊天消息 + 可选提示音）
     */
    void Alert(const char* status, const char* message, const char* emotion = "", const std::string_view& sound = "");
    /// 恢复默认显示状态（清除警告）
    void DismissAlert();

    /// 中断当前 AI 说话（用户打断）
    void AbortSpeaking(AbortReason reason);

    /**
     * 切换对话状态（线程安全，通过事件触发）
     * 空闲 → 打开通道并开始收听；说话中 → 打断；收听中 → 关闭通道
     */
    void ToggleChatState();

    /**
     * 开始收听（线程安全，通过事件触发）
     */
    void StartListening();

    /**
     * 停止收听（线程安全，通过事件触发）
     */
    void StopListening();

    /// 重启设备
    void Reboot();
    /// 模拟唤醒词触发
    void WakeWordInvoke(const std::string& wake_word);
    /// 从指定 URL 下载并安装固件（成功后自动重启）
    bool UpgradeFirmware(const std::string& url, const std::string& version = "");
    /// 检查当前是否可以进入低功耗睡眠模式
    bool CanEnterSleepMode();
    /// 发送 MCP 协议消息到服务器
    void SendMcpMessage(const std::string& payload);
    /// 设置回声消除（AEC）模式
    void SetAecMode(AecMode mode);
    AecMode GetAecMode() const { return aec_mode_; }
    /// 播放内置音效（成功/提示/错误等）
    void PlaySound(const std::string_view& sound);
    AudioService& GetAudioService() { return audio_service_; }
    
    /**
     * 重置协议相关资源（线程安全）
     * 可从任意任务调用，释放网络连接后分配的资源，
     * 包括关闭音频通道、销毁 Protocol 和 OTA 对象
     */
    void ResetProtocol();

private:
    /// 私有构造函数：单例模式限制
    Application();
    ~Application();

    // 主任务回调队列（配合 Schedule() 使用）
    std::mutex mutex_;
    std::deque<std::function<void()>> main_tasks_;
    /// 当前通信协议实例（WebSocket 或 MQTT）
    std::unique_ptr<Protocol> protocol_;
    /// FreeRTOS 事件组，用于主循环事件等待
    EventGroupHandle_t event_group_ = nullptr;
    /// 1 秒周期定时器，驱动状态栏刷新
    esp_timer_handle_t clock_timer_handle_ = nullptr;
    /// 设备状态机实例
    DeviceStateMachine state_machine_;
    /// 当前监听模式（自动停止/手动/实时）
    ListeningMode listening_mode_ = kListeningModeAutoStop;
    /// 当前回声消除模式
    AecMode aec_mode_ = kAecOff;
    std::string last_error_message_;
    /// 音频服务：编解码、唤醒词、VAD 等
    AudioService audio_service_;
    /// OTA 升级对象（激活期间存在）
    std::unique_ptr<Ota> ota_;

    bool has_server_time_ = false;
    bool aborted_ = false;
    bool assets_version_checked_ = false;
    bool play_popup_on_listening_ = false;  // Flag to play popup sound after state changes to listening
    int clock_ticks_ = 0;
    TaskHandle_t activation_task_handle_ = nullptr;


    // Event handlers
    void HandleStateChangedEvent();
    void HandleToggleChatEvent();
    void HandleStartListeningEvent();
    void HandleStopListeningEvent();
    void HandleNetworkConnectedEvent();
    void HandleNetworkDisconnectedEvent();
    void HandleActivationDoneEvent();
    void HandleWakeWordDetectedEvent();
    void ContinueOpenAudioChannel(ListeningMode mode);
    void ContinueWakeWordInvoke(const std::string& wake_word);

    // Activation task (runs in background)
    void ActivationTask();

    // Helper methods
    void CheckAssetsVersion();
    void CheckNewVersion();
    void InitializeProtocol();
    void ShowActivationCode(const std::string& code, const std::string& message);
    void SetListeningMode(ListeningMode mode);
    ListeningMode GetDefaultListeningMode() const;
    
    // State change handler called by state machine
    void OnStateChanged(DeviceState old_state, DeviceState new_state);
};


class TaskPriorityReset {
public:
    TaskPriorityReset(BaseType_t priority) {
        original_priority_ = uxTaskPriorityGet(NULL);
        vTaskPrioritySet(NULL, priority);
    }
    ~TaskPriorityReset() {
        vTaskPrioritySet(NULL, original_priority_);
    }

private:
    BaseType_t original_priority_;
};

#endif // _APPLICATION_H_
