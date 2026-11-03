/*---------------------------------------------------------*\
| ThermaltakeX1RGBPlugin.h                                  |
|                                                           |
|   OpenRGB plugin (API 5, OpenRGB 1.1) that adds the       |
|   Thermaltake Premium X1 RGB keyboard (264A:3013) as a    |
|   per-key Direct-mode device.                             |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <QObject>
#include <QPointer>
#include <QLabel>

#include "OpenRGBPluginInterface.h"
#include "X1RGBDevice.h"
#include "X1RGBLayout.h"

class ThermaltakeX1RGBPlugin;

/*---------------------------------------------------------*\
| One physical keyboard: HID device + OpenRGB controller +  |
| a worker thread that streams frames to the hardware.      |
\*---------------------------------------------------------*/
class X1RGBKeyboard
{
public:
    X1RGBKeyboard(OpenRGBPluginAPIInterface* api, const std::wstring& path, const X1Layout* layout);
    ~X1RGBKeyboard();

    bool                        Register();
    void                        Unregister();
    std::string                 Status() const;

private:
    void                        BuildSetup(RGBController_Setup& setup);
    void                        QueueCurrentColors();
    void                        WorkerThread();

    static void                 CbUpdateLEDs(void* self);
    static void                 CbUpdateZoneLEDs(void* self, int zone);
    static void                 CbUpdateSingleLED(void* self, int led);
    static void                 CbUpdateMode(void* self);

    OpenRGBPluginAPIInterface*  api;
    RGBControllerInterface*     controller;
    X1RGBDevice                 device;
    const X1Layout*             layout;
    std::vector<uint8_t>        led_values;     /* OpenRGB LED index -> firmware LED value */

    std::thread                 worker;
    mutable std::mutex          frame_mutex;
    std::condition_variable     frame_cv;
    std::vector<RGBColor>       pending_frame;
    bool                        frame_dirty;
    bool                        stop;

    std::atomic<bool>           connected;
    std::atomic<unsigned long>  frames_sent;
};

class ThermaltakeX1RGBPlugin : public QObject, public OpenRGBPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID OpenRGBPluginInterface_IID FILE "ThermaltakeX1RGBPlugin.json")
    Q_INTERFACES(OpenRGBPluginInterface)

public:
    ThermaltakeX1RGBPlugin();
    ~ThermaltakeX1RGBPlugin();

    OpenRGBPluginInfo           GetPluginInfo()                                                                     override;
    unsigned int                GetPluginAPIVersion()                                                               override;

    void                        Load(OpenRGBPluginAPIInterface* plugin_api_ptr)                                     override;
    QWidget*                    GetWidget()                                                                         override;
    QMenu*                      GetTrayMenu()                                                                       override;
    void                        Unload()                                                                            override;
    void                        OnProfileAboutToLoad()                                                              override;
    void                        OnProfileLoad(nlohmann::json profile_data)                                          override;
    nlohmann::json              OnProfileSave()                                                                     override;
    unsigned char*              OnSDKCommand(unsigned int pkt_id, unsigned char* pkt_data, unsigned int* pkt_size)  override;

    void                        ProfileManagerUpdated(unsigned int update_reason)                                   override;
    void                        ResourceManagerUpdated(unsigned int update_reason)                                  override;
    void                        SettingsManagerUpdated(unsigned int update_reason)                                  override;

private:
    const X1Layout*             SelectLayout();
    QString                     StatusText() const;

    OpenRGBPluginAPIInterface*                  api;
    std::vector<std::unique_ptr<X1RGBKeyboard>> keyboards;
    QPointer<QLabel>                            status_label;
};
