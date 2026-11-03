/*---------------------------------------------------------*\
| ThermaltakeX1RGBPlugin.cpp                                |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "ThermaltakeX1RGBPlugin.h"

#include <chrono>

#define X1_PLUGIN_VERSION   "0.1.1"
#define X1_SETTINGS_KEY     "ThermaltakeX1RGBPlugin"

/*---------------------------------------------------------*\
| OpenRGB LogManager levels (LogManager.h)                  |
\*---------------------------------------------------------*/
enum
{
    X1_LL_ERROR     = 1,
    X1_LL_WARNING   = 2,
    X1_LL_INFO      = 3,
};

static const unsigned int   X1_MATRIX_NA        = 0xFFFFFFFF;
static const auto           X1_REFRESH_INTERVAL = std::chrono::seconds(5);
static const auto           X1_RETRY_INTERVAL   = std::chrono::seconds(2);

#define X1_LOG(level, ...)  do { if(api) api->LogEntry(__FILE__, __LINE__, level, __VA_ARGS__); } while(0)

/*=========================================================*\
| X1RGBKeyboard                                             |
\*=========================================================*/

X1RGBKeyboard::X1RGBKeyboard(OpenRGBPluginAPIInterface* api, const std::wstring& path, const X1Layout* layout) :
    api(api),
    controller(nullptr),
    device(path),
    layout(layout),
    frame_dirty(false),
    stop(false),
    connected(false),
    frames_sent(0)
{
}

X1RGBKeyboard::~X1RGBKeyboard()
{
    Unregister();
}

void X1RGBKeyboard::BuildSetup(RGBController_Setup& setup)
{
    setup.name          = "Thermaltake Premium X1 RGB";
    setup.vendor        = "Thermaltake";
    setup.description   = "Thermaltake Premium X1 RGB keyboard (ThermaltakeX1RGB plugin, " + std::string(layout->name) + " layout)";
    setup.version       = X1_PLUGIN_VERSION;
    setup.serial        = "";
    setup.location      = "HID: " + device.GetPathUTF8();
    setup.configuration = "";
    setup.type          = DEVICE_TYPE_KEYBOARD;
    setup.flags         = 0;
    setup.active_mode   = 0;

    /*-----------------------------------------------------*\
    | Modes - the firmware has no built-in effects reachable |
    | from the host; the vendor app renders every effect in  |
    | software, so Direct is the only mode.                 |
    \*-----------------------------------------------------*/
    mode direct;
    direct.name         = "Direct";
    direct.value        = 0;
    direct.flags        = MODE_FLAG_HAS_PER_LED_COLOR;
    direct.color_mode   = MODE_COLORS_PER_LED;
    setup.modes.push_back(direct);

    led_values.clear();
    setup.leds.clear();

    /*-----------------------------------------------------*\
    | Zone 0: key matrix                                    |
    \*-----------------------------------------------------*/
    std::vector<unsigned int> map(X1_MATRIX_HEIGHT * X1_MATRIX_WIDTH, X1_MATRIX_NA);

    for(unsigned int key_idx = 0; key_idx < layout->key_count; key_idx++)
    {
        const X1Key& key = layout->keys[key_idx];

        led new_led;
        new_led.name    = key.name;
        new_led.value   = key.value;
        setup.leds.push_back(new_led);
        led_values.push_back(key.value);

        map[(key.row * X1_MATRIX_WIDTH) + key.col] = key_idx;
    }

    zone keyboard_zone;
    keyboard_zone.name          = "Keyboard";
    keyboard_zone.display_name  = "Keyboard";
    keyboard_zone.type          = ZONE_TYPE_MATRIX;
    keyboard_zone.leds_count    = layout->key_count;
    keyboard_zone.leds_min      = layout->key_count;
    keyboard_zone.leds_max      = layout->key_count;
    keyboard_zone.matrix_map.Set(X1_MATRIX_HEIGHT, X1_MATRIX_WIDTH, map.data());
    setup.zones.push_back(keyboard_zone);

    /*-----------------------------------------------------*\
    | Zone 1: media keys outside the matrix                 |
    \*-----------------------------------------------------*/
    for(unsigned int extra_idx = 0; extra_idx < x1_extra_led_count; extra_idx++)
    {
        led new_led;
        new_led.name    = x1_extra_leds[extra_idx].name;
        new_led.value   = x1_extra_leds[extra_idx].value;
        setup.leds.push_back(new_led);
        led_values.push_back(x1_extra_leds[extra_idx].value);
    }

    zone extra_zone;
    extra_zone.name             = "Media Keys";
    extra_zone.display_name     = "Media Keys";
    extra_zone.type             = ZONE_TYPE_LINEAR;
    extra_zone.leds_count       = x1_extra_led_count;
    extra_zone.leds_min         = x1_extra_led_count;
    extra_zone.leds_max         = x1_extra_led_count;
    setup.zones.push_back(extra_zone);

    /*-----------------------------------------------------*\
    | Callbacks                                             |
    \*-----------------------------------------------------*/
    setup.object_ptr                                    = this;
    setup.DeviceConfigureZone                           = nullptr;
    setup.DeviceUpdateLEDs                              = &X1RGBKeyboard::CbUpdateLEDs;
    setup.DeviceUpdateZoneLEDs                          = &X1RGBKeyboard::CbUpdateZoneLEDs;
    setup.DeviceUpdateSingleLED                         = &X1RGBKeyboard::CbUpdateSingleLED;
    setup.DeviceUpdateMode                              = &X1RGBKeyboard::CbUpdateMode;
    setup.DeviceSaveMode                                = nullptr;
    setup.DeviceUpdateZoneMode                          = nullptr;
    setup.DeviceUpdateDeviceSpecificConfiguration       = nullptr;
    setup.DeviceUpdateDeviceSpecificZoneConfiguration   = nullptr;
}

bool X1RGBKeyboard::Register()
{
    if(!device.Open())
    {
        X1_LOG(X1_LL_ERROR, "[ThermaltakeX1RGB] Cannot open %s", device.GetPathUTF8().c_str());
        return(false);
    }

    connected = true;

    RGBController_Setup setup;
    BuildSetup(setup);

    controller = api->CreateVirtualRGBController(&setup);

    if(!controller)
    {
        X1_LOG(X1_LL_ERROR, "[ThermaltakeX1RGB] CreateVirtualRGBController failed");
        device.Close();
        connected = false;
        return(false);
    }

    stop        = false;
    worker      = std::thread(&X1RGBKeyboard::WorkerThread, this);

    api->RegisterVirtualRGBController(controller);

    X1_LOG(X1_LL_INFO, "[ThermaltakeX1RGB] Registered %s (%s layout, %u LEDs)",
           device.GetPathUTF8().c_str(), layout->name, (unsigned int)led_values.size());

    return(true);
}

void X1RGBKeyboard::Unregister()
{
    {
        std::lock_guard<std::mutex> lock(frame_mutex);
        stop = true;
    }
    frame_cv.notify_all();

    if(worker.joinable())
    {
        worker.join();
    }

    if(controller)
    {
        api->UnregisterVirtualRGBController(controller);
        api->DeleteVirtualRGBController(controller);
        controller = nullptr;
    }

    device.Close();
    connected = false;
}

std::string X1RGBKeyboard::Status() const
{
    std::string s = std::string(layout->name) + " layout, " + std::to_string(led_values.size()) + " LEDs, ";
    s += connected ? "connected" : "disconnected (retrying)";
    s += ", frames sent: " + std::to_string(frames_sent.load());
    return(s);
}

/*---------------------------------------------------------*\
| Snapshot the controller colours and wake the worker.      |
| Called from OpenRGB threads with the controller mutex     |
| released, so GetColor() is safe here.                     |
\*---------------------------------------------------------*/
void X1RGBKeyboard::QueueCurrentColors()
{
    if(!controller)
    {
        return;
    }

    std::vector<RGBColor> frame(led_values.size());

    for(std::size_t idx = 0; idx < frame.size(); idx++)
    {
        frame[idx] = controller->GetColor((unsigned int)idx);
    }

    {
        std::lock_guard<std::mutex> lock(frame_mutex);
        pending_frame   = std::move(frame);
        frame_dirty     = true;
    }

    frame_cv.notify_one();
}

void X1RGBKeyboard::WorkerThread()
{
    std::unique_lock<std::mutex> lock(frame_mutex);

    while(!stop)
    {
        /*-------------------------------------------------*\
        | Wait for a new frame, or time out to resend the   |
        | last one (keeps host mode alive and recovers      |
        | after unplug / sleep).                            |
        \*-------------------------------------------------*/
        frame_cv.wait_for(lock, X1_REFRESH_INTERVAL, [this] { return(frame_dirty || stop); });

        if(stop)
        {
            break;
        }

        if(pending_frame.empty())
        {
            continue;
        }

        std::vector<RGBColor> frame = pending_frame;
        frame_dirty = false;
        lock.unlock();

        bool ok = device.IsOpen() || device.Open();

        if(ok)
        {
            std::vector<X1LedColor> leds(frame.size());

            for(std::size_t idx = 0; idx < frame.size(); idx++)
            {
                leds[idx].led   = led_values[idx];
                leds[idx].r     = (uint8_t)RGBGetRValue(frame[idx]);
                leds[idx].g     = (uint8_t)RGBGetGValue(frame[idx]);
                leds[idx].b     = (uint8_t)RGBGetBValue(frame[idx]);
            }

            ok = device.SendFrame(leds);
        }

        if(ok)
        {
            if(!connected)
            {
                X1_LOG(X1_LL_INFO, "[ThermaltakeX1RGB] Keyboard connection restored");
            }
            connected = true;
            frames_sent++;
            lock.lock();
        }
        else
        {
            if(connected)
            {
                X1_LOG(X1_LL_WARNING, "[ThermaltakeX1RGB] Write failed, keyboard disconnected? Retrying.");
            }
            connected = false;
            device.Close();

            lock.lock();
            frame_dirty = true;
            frame_cv.wait_for(lock, X1_RETRY_INTERVAL, [this] { return(stop); });
        }
    }
}

void X1RGBKeyboard::CbUpdateLEDs(void* self)
{
    static_cast<X1RGBKeyboard*>(self)->QueueCurrentColors();
}

void X1RGBKeyboard::CbUpdateZoneLEDs(void* self, int /*zone*/)
{
    static_cast<X1RGBKeyboard*>(self)->QueueCurrentColors();
}

void X1RGBKeyboard::CbUpdateSingleLED(void* self, int /*led*/)
{
    static_cast<X1RGBKeyboard*>(self)->QueueCurrentColors();
}

void X1RGBKeyboard::CbUpdateMode(void* self)
{
    static_cast<X1RGBKeyboard*>(self)->QueueCurrentColors();
}

/*=========================================================*\
| ThermaltakeX1RGBPlugin                                    |
\*=========================================================*/

ThermaltakeX1RGBPlugin::ThermaltakeX1RGBPlugin() : api(nullptr)
{
}

ThermaltakeX1RGBPlugin::~ThermaltakeX1RGBPlugin()
{
    keyboards.clear();
}

OpenRGBPluginInfo ThermaltakeX1RGBPlugin::GetPluginInfo()
{
    OpenRGBPluginInfo info;

    info.Name           = "Thermaltake X1 RGB";
    info.Description    = "Per-key Direct control of the Thermaltake Premium X1 RGB keyboard (USB 264A:3013)";
    info.Version        = X1_PLUGIN_VERSION;
    info.Commit         = "";
    info.URL            = "https://github.com/polosatiktiger/openrgb-thermaltake-x1-rgb-plugin";
    info.Location       = OPENRGB_PLUGIN_LOCATION_INFORMATION;
    info.Label          = "Thermaltake X1 RGB";
    info.TabIconString  = "";
    info.ProtocolVersion = 0;

    return(info);
}

unsigned int ThermaltakeX1RGBPlugin::GetPluginAPIVersion()
{
    return(OPENRGB_PLUGIN_API_VERSION);
}

/*---------------------------------------------------------*\
| Layout from OpenRGB settings:                             |
|   "ThermaltakeX1RGBPlugin": { "layout": "ANSI" | "ISO" }   |
\*---------------------------------------------------------*/
const X1Layout* ThermaltakeX1RGBPlugin::SelectLayout()
{
    try
    {
        nlohmann::json settings = api->GetSettings(X1_SETTINGS_KEY);

        if(settings.contains("layout") && settings["layout"].is_string())
        {
            std::string name = settings["layout"];

            if(name == "ISO" || name == "iso")
            {
                return(&x1_layout_iso);
            }
        }
    }
    catch(...)
    {
    }

    return(&x1_layout_ansi);
}

void ThermaltakeX1RGBPlugin::Load(OpenRGBPluginAPIInterface* plugin_api_ptr)
{
    api = plugin_api_ptr;

    const X1Layout* layout = SelectLayout();

    std::vector<std::wstring> paths = X1RGBDevice::Enumerate();

    X1_LOG(X1_LL_INFO, "[ThermaltakeX1RGB] Plugin %s loaded, %u keyboard(s) found", X1_PLUGIN_VERSION, (unsigned int)paths.size());

    for(const std::wstring& path : paths)
    {
        std::unique_ptr<X1RGBKeyboard> kb(new X1RGBKeyboard(api, path, layout));

        if(kb->Register())
        {
            keyboards.push_back(std::move(kb));
        }
    }

    if(status_label)
    {
        status_label->setText(StatusText());
    }
}

void ThermaltakeX1RGBPlugin::Unload()
{
    keyboards.clear();

    if(status_label)
    {
        status_label->setText(StatusText());
    }

    X1_LOG(X1_LL_INFO, "[ThermaltakeX1RGB] Plugin unloaded");
}

QString ThermaltakeX1RGBPlugin::StatusText() const
{
    QString text = "<b>Thermaltake Premium X1 RGB</b> plugin " X1_PLUGIN_VERSION "<br><br>";

    if(keyboards.empty())
    {
        text += "No keyboard found (USB 264A:3013, interface MI_01).<br>"
                "Plug it in and restart OpenRGB.";
    }
    else
    {
        for(std::size_t idx = 0; idx < keyboards.size(); idx++)
        {
            text += QString("Keyboard %1: %2<br>").arg(idx + 1).arg(QString::fromStdString(keyboards[idx]->Status()));
        }
    }

    text += "<br><br>Layout can be set in OpenRGB.json: "
            "<code>\"" X1_SETTINGS_KEY "\": { \"layout\": \"ANSI\" }</code> (ANSI or ISO).";

    return(text);
}

QWidget* ThermaltakeX1RGBPlugin::GetWidget()
{
    QLabel* label = new QLabel();
    label->setTextFormat(Qt::RichText);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    label->setText(StatusText());
    status_label = label;
    return(label);
}

QMenu* ThermaltakeX1RGBPlugin::GetTrayMenu()
{
    return(nullptr);
}

void ThermaltakeX1RGBPlugin::OnProfileAboutToLoad()
{
}

void ThermaltakeX1RGBPlugin::OnProfileLoad(nlohmann::json /*profile_data*/)
{
}

nlohmann::json ThermaltakeX1RGBPlugin::OnProfileSave()
{
    return(nlohmann::json());
}

unsigned char* ThermaltakeX1RGBPlugin::OnSDKCommand(unsigned int /*pkt_id*/, unsigned char* /*pkt_data*/, unsigned int* pkt_size)
{
    if(pkt_size)
    {
        *pkt_size = 0;
    }
    return(nullptr);
}

void ThermaltakeX1RGBPlugin::ProfileManagerUpdated(unsigned int /*update_reason*/)
{
}

void ThermaltakeX1RGBPlugin::ResourceManagerUpdated(unsigned int /*update_reason*/)
{
}

void ThermaltakeX1RGBPlugin::SettingsManagerUpdated(unsigned int /*update_reason*/)
{
}
