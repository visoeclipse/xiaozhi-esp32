#ifndef _DEVICE_STATE_H_
#define _DEVICE_STATE_H_

/**
 * 设备运行状态枚举
 *
 * 状态转换规则由 DeviceStateMachine::IsValidTransition() 严格定义。
 * 主流程：Starting → WifiConfiguring/Activating → Idle → Connecting → Listening ↔ Speaking
 */
enum DeviceState {
    /** 初始状态，仅在上电后短暂存在 */
    kDeviceStateUnknown,
    /** 系统启动中，正在初始化各子系统 */
    kDeviceStateStarting,
    /** WiFi 配网模式，设备发出热点等待用户配置 */
    kDeviceStateWifiConfiguring,
    /** 空闲状态，等待唤醒词或按键触发对话 */
    kDeviceStateIdle,
    /** 正在建立与服务器的音频通道 */
    kDeviceStateConnecting,
    /** 正在收音，采集用户语音并上传 */
    kDeviceStateListening,
    /** 正在播放 AI 回复音频 */
    kDeviceStateSpeaking,
    /** OTA 固件或资源升级中 */
    kDeviceStateUpgrading,
    /** 正在与服务器进行设备激活/绑定 */
    kDeviceStateActivating,
    /** 配网模式下的音频测试（本地回环） */
    kDeviceStateAudioTesting,
    /** 致命错误，不可恢复（终态） */
    kDeviceStateFatalError
};

#endif // _DEVICE_STATE_H_ 
