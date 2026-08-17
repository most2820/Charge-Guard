#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include <exception_wrap.hpp>

#include <switch.h>
#include <string>

#define CONFIG_PATH "sdmc:/config/charge-guard/config.ini"

static const int s_limitValues[] = { 80, 85, 90, 95, 100 };
static constexpr int s_limitCount = 5;

static int s_currentLimit = 100;
static bool s_initialized = false;

static void loadConfig(void) {
    std::string val = ult::parseValueFromIniSection(CONFIG_PATH, "ChargeGuard", "ChargeLimit");
    if (!val.empty())
        s_currentLimit = std::stoi(val);
    else
        s_currentLimit = 100;
}

static void saveConfig(void) {
    ult::setIniFileValue(CONFIG_PATH, "ChargeGuard", "ChargeLimit", std::to_string(s_currentLimit));
}

class GuiMain : public tsl::Gui {
public:
    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("Charge Guard", "v1.0.0");
        auto list  = new tsl::elm::List();

        list->addItem(new tsl::elm::CategoryHeader("Charge Limit"));

        int initialIndex = s_limitCount - 1;
        for (int i = 0; i < s_limitCount; i++) {
            if (s_limitValues[i] == s_currentLimit) {
                initialIndex = i;
                break;
            }
        }

        auto *bar = new tsl::elm::NamedStepTrackBar("\uE132", {
            "80%", "85%", "90%", "95%", "100%"
        });
        bar->setProgress(initialIndex);
        bar->setValueChangedListener([](u16 val) {
            if (val < s_limitCount) {
                s_currentLimit = s_limitValues[val];
                saveConfig();
            }
        });
        list->addItem(bar);

        frame->setContent(list);
        return frame;
    }

    virtual void update() override {}

    virtual bool handleInput(u64 keysDown, u64 keysHeld,
        const HidTouchState &touchPos,
        HidAnalogStickState joyStickPosLeft,
        HidAnalogStickState joyStickPosRight) override {

        if (keysDown & HidNpadButton_B) {
            tsl::goBack();
            return true;
        }
        return false;
    }
};

class OverlayMain : public tsl::Overlay {
public:
    virtual void initServices() override {
        fsdevMountSdmc();
        psmInitialize();
        i2cInitialize();
        hidInitialize();
        s_initialized = true;
        loadConfig();
    }

    virtual void exitServices() override {
        s_initialized = false;
        hidExit();
        i2cExit();
        psmExit();
        fsdevUnmountDevice("sdmc");
    }

    virtual void onShow() override {}
    virtual void onHide() override {}

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<GuiMain>();
    }
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayMain>(argc, argv);
}
