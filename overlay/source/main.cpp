#define TESLA_INIT_IMPL
#include <tesla.hpp>

#include <switch.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>

#define CONFIG_PATH "sdmc:/config/charge-guard/config.ini"

static const int s_limitValues[] = { 80, 85, 90, 95, 100 };
static constexpr int s_limitCount = 5;

static int s_currentLimit = 100;
static bool s_initialized = false;

static void loadConfig(void) {
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) { s_currentLimit = 100; return; }

    char line[64];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '[' || line[0] == 0) continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;

        char *key = line;
        char *val = eq + 1;
        while (*key == ' ') key++;
        while (*val == ' ') val++;

        if (strcmp(key, "ChargeLimit") == 0)
            s_currentLimit = atoi(val);
    }
    fclose(f);
}

static void saveConfig(void) {
    mkdir("sdmc:/config", 0777);
    mkdir("sdmc:/config/charge-guard", 0777);

    FILE *f = fopen(CONFIG_PATH, "w");
    if (!f) return;
    fprintf(f, "[ChargeGuard]\n");
    fprintf(f, "ChargeLimit = %d\n", s_currentLimit);
    fclose(f);
    fsdevCommitDevice("sdmc");
}

static void applyChargeLimit(void) {
    if (!s_initialized) return;

    u32 batPct = 0;
    psmGetBatteryChargePercentage(&batPct);

    if (s_currentLimit >= 100) {
        psmEnableBatteryCharging();
    } else {
        if ((int)batPct >= s_currentLimit)
            psmDisableBatteryCharging();
        else
            psmEnableBatteryCharging();
    }
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
        bar->setValueChangedListener([](u8 val) {
            if (val < s_limitCount) {
                s_currentLimit = s_limitValues[val];
                saveConfig();
                applyChargeLimit();
            }
        });
        list->addItem(bar);

        frame->setContent(list);
        return frame;
    }

    virtual void update() override {
        if (s_initialized)
            applyChargeLimit();
    }

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
        psmEnableBatteryCharging();
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
