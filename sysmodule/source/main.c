#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#define INNER_HEAP_SIZE 0x20000
#define CONFIG_PATH "sdmc:/config/charge-guard/config.ini"
#define LOG_PATH    "sdmc:/config/charge-guard/charge-guard.log"
#define POLL_INTERVAL_NS 2000000000ULL
#define SLEEP_WAKE_THRESHOLD_TICKS 3000000000ULL

#define BQ24193_REG_CCC    0x01
#define BQ24193_ICHRG_MASK 0xFC
#define BQ24193_ICHRG_DIS  0x00
#define BQ24193_ICHRG_1024 0x80

u32 __nx_applet_type = AppletType_None;
u32 __nx_fs_num_sessions = 1;

static u8 inner_heap[INNER_HEAP_SIZE];

void __libnx_initheap(void) {
    extern void* fake_heap_start;
    extern void* fake_heap_end;
    fake_heap_start = inner_heap;
    fake_heap_end   = inner_heap + sizeof(inner_heap);
}

void __appInit(void) {
    Result rc;

    rc = smInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    rc = setsysInitialize();
    if (R_SUCCEEDED(rc)) {
        SetSysFirmwareVersion fw;
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }

    rc = fsInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    fsdevMountSdmc();

    rc = i2cInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    rc = psmInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    smExit();
}

void __appExit(void) {
    psmEnableBatteryCharging();
    psmExit();
    i2cExit();
    fsdevUnmountAll();
    fsExit();
}

static void log_msg(const char *msg) {
    FILE *f = fopen(LOG_PATH, "a");
    if (!f) return;
    fprintf(f, "%s\n", msg);
    fclose(f);
}

static void log_status(int limit, u32 bat, bool chg_enabled, bool hw_charging, const char *action) {
    FILE *f = fopen(LOG_PATH, "a");
    if (!f) return;
    fprintf(f, "lim=%d bat=%u sw=%d hw=%d %s\n",
            limit, bat, (int)chg_enabled, (int)hw_charging, action);
    fclose(f);
}

static int read_config_limit(void) {
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) return 100;

    char line[128];
    int limit = 100;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '[' || line[0] == 0 || line[0] == '#')
            continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;

        char *key = line;
        char *val = eq + 1;
        while (*key == ' ' || *key == '\t') key++;
        while (*val == ' ' || *val == '\t') val++;

        if (strcmp(key, "ChargeLimit") == 0) {
            int parsed = atoi(val);
            if (parsed >= 0 && parsed <= 100)
                limit = parsed;
            break;
        }
    }

    fclose(f);
    return limit;
}

static I2cSession s_bq;
static bool s_bq_ok = false;
static u8 s_bq_orig_ccc = 0;

static void bq_init(void) {
    Result rc = i2cOpenSession(&s_bq, I2cDevice_Bq24193);
    if (R_SUCCEEDED(rc)) {
        s_bq_ok = true;

        u8 reg = BQ24193_REG_CCC;
        i2csessionSendAuto(&s_bq, &reg, 1, I2cTransactionOption_Start);
        i2csessionReceiveAuto(&s_bq, &s_bq_orig_ccc, 1, I2cTransactionOption_Stop);

        log_msg("BQ24193 I2C opened OK");
    } else {
        log_msg("BQ24193 I2C FAILED, using PSM only");
    }
}

static void bq_set_charge_current(bool enable) {
    if (!s_bq_ok) return;

    u8 reg = BQ24193_REG_CCC;
    i2csessionSendAuto(&s_bq, &reg, 1, I2cTransactionOption_Start);
    u8 ccc = 0;
    i2csessionReceiveAuto(&s_bq, &ccc, 1, I2cTransactionOption_Stop);

    u8 new_ccc;
    if (enable)
        new_ccc = (ccc & ~BQ24193_ICHRG_MASK) | BQ24193_ICHRG_1024;
    else
        new_ccc = (ccc & ~BQ24193_ICHRG_MASK) | BQ24193_ICHRG_DIS;

    if (new_ccc != ccc) {
        u8 cmd[2] = { BQ24193_REG_CCC, new_ccc };
        i2csessionSendAuto(&s_bq, cmd, 2, I2cTransactionOption_All);
    }
}

int main(int argc, char* argv[]) {
    svcSleepThread(5000000000ULL);

    bq_init();

    log_msg("ChargeGuard sysmodule started");

    u64 last_tick = svcGetSystemTick();
    bool was_above = false;

    while (1) {
        int limit = read_config_limit();
        u32 batPct = 0;
        bool charging_sw = false;

        psmGetBatteryChargePercentage(&batPct);
        psmIsBatteryChargingEnabled(&charging_sw);

        u64 now_tick = svcGetSystemTick();
        u64 tick_diff = now_tick - last_tick;
        last_tick = now_tick;
        bool woke = (tick_diff > SLEEP_WAKE_THRESHOLD_TICKS);

        if (limit < 100) {
            bool should_charge = ((int)batPct < limit);
            bool above = ((int)batPct >= limit);

            if (should_charge) {
                if (!charging_sw)
                    psmEnableBatteryCharging();
                bq_set_charge_current(true);
                if (was_above || woke)
                    log_status(limit, batPct, charging_sw, true, "ALLOW_CHARGE");
            } else {
                if (charging_sw)
                    psmDisableBatteryCharging();
                bq_set_charge_current(false);
                if (!was_above || woke)
                    log_status(limit, batPct, charging_sw, false, "FORBID_CHARGE");
            }

            was_above = above;
        } else {
            if (!charging_sw)
                psmEnableBatteryCharging();
            bq_set_charge_current(true);
        }

        svcSleepThread(POLL_INTERVAL_NS);
    }

    return 0;
}
