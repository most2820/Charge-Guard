#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#define INNER_HEAP_SIZE 0x20000
#define CONFIG_PATH "sdmc:/config/charge-guard/config.ini"
#define POLL_INTERVAL_NS 10000000000ULL

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

    rc = psmInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    smExit();
}

void __appExit(void) {
    psmEnableBatteryCharging();
    psmExit();
    fsdevUnmountAll();
    fsExit();
}

static int read_config_limit(void) {
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) return 100;

    char line[64];
    int limit = 100;

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

        if (strcmp(key, "ChargeLimit") == 0) {
            limit = atoi(val);
        }
    }

    fclose(f);
    return limit;
}

int main(int argc, char* argv[]) {
    svcSleepThread(5000000000ULL);

    while (1) {
        int limit = read_config_limit();

        if (limit < 100) {
            u32 batPct = 0;
            psmGetBatteryChargePercentage(&batPct);

            if ((int)batPct >= limit)
                psmDisableBatteryCharging();
            else
                psmEnableBatteryCharging();
        } else {
            psmEnableBatteryCharging();
        }

        svcSleepThread(POLL_INTERVAL_NS);
    }

    return 0;
}
