#ifndef BOARD_H
#define BOARD_H

#include <http.h>
#include <web_socket.h>
#include <mqtt.h>
#include <udp.h>
#include <string>
#include <functional>
#include <network_interface.h>

#include "led/led.h"
#include "backlight.h"
#include "camera.h"
#include "assets.h"

/**
 * 网络事件类型（统一回调接口）
 *
 * 无论是 WiFi 还是 4G 蜂窝网络，都通过这套枚举向 Application 报告状态变化。
 */
enum class NetworkEvent {
    Scanning,              ///< 正在扫描网络（WiFi 搜索等）
    Connecting,            ///< 正在连接（data 为 SSID 或运营商名称）
    Connected,             ///< 连接成功（data 为 SSID 或运营商名称）
    Disconnected,          ///< 连接断开
    WifiConfigModeEnter,   ///< 进入 WiFi 配网模式
    WifiConfigModeExit,    ///< 退出 WiFi 配网模式
    // 以下为 4G 蜂窝模组专用事件
    ModemDetecting,        ///< 正在检测模组（波特率、型号）
    ModemErrorNoSim,       ///< 未检测到 SIM 卡
    ModemErrorRegDenied,   ///< 网络注册被拒绝
    ModemErrorInitFailed,  ///< 模组初始化失败
    ModemErrorTimeout      ///< 操作超时
};

/**
 * 功耗级别枚举
 */
enum class PowerSaveLevel {
    LOW_POWER,    ///< 低功耗：降低 CPU 频率，关闭不必要的外设
    BALANCED,     ///< 平衡：中等性能和功耗
    PERFORMANCE,  ///< 高性能：不省电，CPU 全速运行（如升级/通话时）
};

// Network event callback type (event, data)
// data contains additional info like SSID for Connecting/Connected events
using NetworkEventCallback = std::function<void(NetworkEvent event, const std::string& data)>;

void* create_board();
class AudioCodec;
class Display;
class Board {
private:
    Board(const Board&) = delete; // 禁用拷贝构造函数
    Board& operator=(const Board&) = delete; // 禁用赋值操作

protected:
    Board();
    std::string GenerateUuid();

    // 软件生成的设备唯一标识
    std::string uuid_;

public:
    static Board& GetInstance() {
        static Board* instance = static_cast<Board*>(create_board());
        return *instance;
    }

    virtual ~Board() = default;
    virtual std::string GetBoardType() = 0;
    virtual std::string GetUuid() { return uuid_; }
    virtual Backlight* GetBacklight() { return nullptr; }
    virtual Led* GetLed();
    virtual AudioCodec* GetAudioCodec() = 0;
    virtual bool GetTemperature(float& esp32temp);
    virtual Display* GetDisplay();
    virtual Camera* GetCamera();
    virtual NetworkInterface* GetNetwork() = 0;
    virtual void StartNetwork() = 0;
    virtual void SetNetworkEventCallback(NetworkEventCallback callback) { (void)callback; }
    virtual const char* GetNetworkStateIcon() = 0;
    virtual bool GetBatteryLevel(int &level, bool& charging, bool& discharging);
    virtual std::string GetSystemInfoJson();
    virtual void SetPowerSaveLevel(PowerSaveLevel level) = 0;
    virtual std::string GetBoardJson() = 0;
    virtual std::string GetDeviceStatusJson() = 0;
};

#define DECLARE_BOARD(BOARD_CLASS_NAME) \
void* create_board() { \
    return new BOARD_CLASS_NAME(); \
}

#endif // BOARD_H
