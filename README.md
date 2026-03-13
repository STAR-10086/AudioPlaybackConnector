# AudioPlaybackConnector

Windows 10 2004+ 的蓝牙音频播放（A2DP Sink）连接器。

Microsoft 在 Windows 10 2004 中添加了蓝牙 A2DP Sink 功能。然而，需要第三方应用程序来管理连接。
已经有一个应用程序可以完成这项工作。但它不能隐藏到通知区域，并且不是开源的。
所以我编写了这个应用程序，提供一个简单、现代且开源的替代方案。

# 预览
![Preview](https://cdn.jsdelivr.net/gh/ysc3839/AudioPlaybackConnector@master/AudioPlaybackConnector.gif)

# 使用方法
* 从 [releases](https://github.com/ysc3839/AudioPlaybackConnector/releases) 下载并运行 AudioPlaybackConnector。
* 在系统蓝牙设置中添加蓝牙设备。您可以右键点击通知区域中的 AudioPlaybackConnector 图标，然后选择 "蓝牙设置"。
* 点击 AudioPlaybackConnector 图标并选择您要连接的设备。
* 享受！

## 功能

- 系统托盘集成
- 蓝牙设备发现和连接
- 自动重连功能
- 语言支持（中文/English）
- 持久化设置

## 要求

- Windows 10 2004或更高版本
- Visual Studio 2022或更高版本
- C++/WinRT支持

## 构建

1. 在Visual Studio中打开解决方案文件
2. 在Release配置中构建项目
3. 可执行文件将在bin目录中生成

## 许可证

MIT
