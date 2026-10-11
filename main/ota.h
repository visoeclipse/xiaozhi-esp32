#ifndef _OTA_H
#define _OTA_H

#include <functional>
#include <string>

#include <esp_err.h>
#include "board.h"

/**
 * Ota - OTA 固件升级与设备激活管理
 *
 * 负责与 OTA 服务器通信：
 * - 检查新固件版本
 * - 下载并安装固件
 * - 获取服务器配置（MQTT/WebSocket 参数）
 * - 处理设备激活流程（获取验证码 → 用户绑定 → 确认激活）
 */
class Ota {
public:
    Ota();
    ~Ota();

    /// 向服务器请求版本检查，获取是否有新固件和服务器配置
    esp_err_t CheckVersion();
    /// 向服务器发送激活请求，确认设备绑定
    esp_err_t Activate();
    /// 是否有激活挑战（需要用户绑定）
    bool HasActivationChallenge() { return has_activation_challenge_; }
    /// 是否有新固件版本可升级
    bool HasNewVersion() { return has_new_version_; }
    /// 服务器是否下发了 MQTT 配置
    bool HasMqttConfig() { return has_mqtt_config_; }
    /// 服务器是否下发了 WebSocket 配置
    bool HasWebsocketConfig() { return has_websocket_config_; }
    /// 是否有激活码需要显示给用户
    bool HasActivationCode() { return has_activation_code_; }
    /// 是否已获取服务器时间
    bool HasServerTime() { return has_server_time_; }
    /// 开始下载固件（实例方法，带进度回调）
    bool StartUpgrade(std::function<void(int progress, size_t speed)> callback);
    /// 下载固件（静态方法，可直接调用）
    static bool Upgrade(const std::string& firmware_url, std::function<void(int progress, size_t speed)> callback);
    /// 标记当前固件版本为有效（防止 OTA 失败后回滚）
    void MarkCurrentVersionValid();

    /// 新固件版本号
    const std::string& GetFirmwareVersion() const { return firmware_version_; }
    /// 当前运行的固件版本号
    const std::string& GetCurrentVersion() const { return current_version_; }
    /// 新固件的下载 URL
    const std::string& GetFirmwareUrl() const { return firmware_url_; }
    /// 激活提示消息
    const std::string& GetActivationMessage() const { return activation_message_; }
    /// 设备激活码（用户需要在 xiaozhi.me 输入的数字）
    const std::string& GetActivationCode() const { return activation_code_; }
    /// 获取 OTA 版本检查的完整 URL
    std::string GetCheckVersionUrl();

private:
    std::string activation_message_;
    std::string activation_code_;
    bool has_new_version_ = false;
    bool has_mqtt_config_ = false;
    bool has_websocket_config_ = false;
    bool has_server_time_ = false;
    bool has_activation_code_ = false;
    bool has_serial_number_ = false;
    bool has_activation_challenge_ = false;
    std::string current_version_;
    std::string firmware_version_;
    std::string firmware_url_;
    std::string activation_challenge_;
    std::string serial_number_;
    int activation_timeout_ms_ = 30000;

    std::function<void(int progress, size_t speed)> upgrade_callback_;
    std::vector<int> ParseVersion(const std::string& version);
    bool IsNewVersionAvailable(const std::string& currentVersion, const std::string& newVersion);
    std::string GetActivationPayload();
    std::unique_ptr<Http> SetupHttp();
};

#endif // _OTA_H
