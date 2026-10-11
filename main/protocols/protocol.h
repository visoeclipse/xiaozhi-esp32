#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cJSON.h>
#include <string>
#include <functional>
#include <chrono>
#include <vector>

/**
 * 音频流数据包
 *
 * 封装一帧音频数据及其元信息，用于在编码/发送/接收/解码各阶段传递。
 */
struct AudioStreamPacket {
    /// 采样率（Hz），如 16000 或 24000
    int sample_rate = 0;
    /// 帧时长（毫秒），通常为 60ms
    int frame_duration = 0;
    /// 时间戳（毫秒），用于服务器端 AEC 同步
    uint32_t timestamp = 0;
    /// Opus 编码后的音频数据
    std::vector<uint8_t> payload;
};

/**
 * 二进制协议 v2 包头
 * 用于 WebSocket 二进制帧，包含版本号和消息类型
 */
struct BinaryProtocol2 {
    uint16_t version;
    uint16_t type;          // Message type (0: OPUS, 1: JSON)
    uint32_t reserved;      // Reserved for future use
    uint32_t timestamp;     // Timestamp in milliseconds (used for server-side AEC)
    uint32_t payload_size;  // Payload size in bytes
    uint8_t payload[];      // Payload data
} __attribute__((packed));

/**
 * 二进制协议 v3 包头
 * 简化版，包含类型和负载大小
 */
struct BinaryProtocol3 {
    uint8_t type;
    uint8_t reserved;
    uint16_t payload_size;
    uint8_t payload[];
} __attribute__((packed));

enum AbortReason {
    kAbortReasonNone,              ///< 用户按键打断
    kAbortReasonWakeWordDetected   ///< 新的唤醒词触发打断
};

/**
 * 监听模式
 */
enum ListeningMode {
    kListeningModeAutoStop,    ///< 自动停止：检测到用户说完后自动切换到 AI 回复
    kListeningModeManualStop,  ///< 手动停止：用户需要按键结束收听
    kListeningModeRealtime     ///< 实时模式：全双工，需要 AEC 支持
};

/**
 * Protocol - 通信协议抽象基类
 *
 * 定义设备与服务器之间的通信接口，包括：
 * - 音频通道的建立和关闭
 * - 音频数据的发送和接收
 * - JSON 控制消息的收发（TTS/STT/LLM/MCP 等）
 *
 * 具体实现由 WebsocketProtocol 和 MqttProtocol 提供。
 */
class Protocol {
public:
    virtual ~Protocol() = default;

    /// 服务器端音频采样率（可能和设备不同，需重采样）
    inline int server_sample_rate() const {
        return server_sample_rate_;
    }
    /// 服务器端音频帧时长
    inline int server_frame_duration() const {
        return server_frame_duration_;
    }
    /// 当前会话 ID（由服务器分配）
    inline const std::string& session_id() const {
        return session_id_;
    }

    // 注册各类回调
    void OnIncomingAudio(std::function<void(std::unique_ptr<AudioStreamPacket> packet)> callback);
    void OnIncomingJson(std::function<void(const cJSON* root)> callback);
    void OnAudioChannelOpened(std::function<void()> callback);
    void OnAudioChannelClosed(std::function<void()> callback);
    void OnNetworkError(std::function<void(const std::string& message)> callback);
    void OnConnected(std::function<void()> callback);
    void OnDisconnected(std::function<void()> callback);

    /// 启动协议（建立底层连接）
    virtual bool Start() = 0;
    /// 打开音频通道（发送 hello + 等待服务器 hello）
    virtual bool OpenAudioChannel() = 0;
    /// 关闭音频通道
    virtual void CloseAudioChannel(bool send_goodbye = true) = 0;
    /// 音频通道是否已打开
    virtual bool IsAudioChannelOpened() const = 0;
    /// 发送一帧音频数据
    virtual bool SendAudio(std::unique_ptr<AudioStreamPacket> packet) = 0;
    /// 发送唤醒词检测通知
    virtual void SendWakeWordDetected(const std::string& wake_word);
    /// 发送开始收听指令
    virtual void SendStartListening(ListeningMode mode);
    /// 发送停止收听指令
    virtual void SendStopListening();
    /// 发送打断说话指令
    virtual void SendAbortSpeaking(AbortReason reason);
    /// 发送 MCP 控制消息
    virtual void SendMcpMessage(const std::string& message);

protected:
    std::function<void(const cJSON* root)> on_incoming_json_;
    std::function<void(std::unique_ptr<AudioStreamPacket> packet)> on_incoming_audio_;
    std::function<void()> on_audio_channel_opened_;
    std::function<void()> on_audio_channel_closed_;
    std::function<void(const std::string& message)> on_network_error_;
    std::function<void()> on_connected_;
    std::function<void()> on_disconnected_;

    int server_sample_rate_ = 24000;
    int server_frame_duration_ = 60;
    bool error_occurred_ = false;
    std::string session_id_;
    std::chrono::time_point<std::chrono::steady_clock> last_incoming_time_;

    virtual bool SendText(const std::string& text) = 0;
    virtual void SetError(const std::string& message);
    virtual bool IsTimeout() const;
};

#endif // PROTOCOL_H
