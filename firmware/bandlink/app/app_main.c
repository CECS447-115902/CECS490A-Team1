/**
 * Firmware entry. CubeMX-generated main() calls app_main() in its
 * USER CODE BEGIN 2 block after HAL_Init() and the generated MX_*_Init().
 */
#include "scheduler.h"
#include "events.h"
#include "bsp.h"

/* Each module lives in its own service/driver folder and exposes these. */
void radio_process(uint32_t events);
void ranging_process(uint32_t events);
void gnss_process(uint32_t events);
void imu_process(uint32_t events);
void nfc_process(uint32_t events);
void ui_process(uint32_t events);
void system_process(uint32_t events);

/* Table order is priority order. Keep each process() under its budget:
 * radio/ranging < 2 ms, gnss/imu/system < 1 ms, nfc < 3 ms, ui < 5 ms. */
static const module_t k_modules[] = {
    { "radio",   EVT_RADIO,   radio_process   },
    { "ranging", EVT_RANGING, ranging_process },
    { "gnss",    EVT_GNSS,    gnss_process    },
    { "imu",     EVT_IMU,     imu_process     },
    { "nfc",     EVT_NFC,     nfc_process     },
    { "ui",      EVT_UI,      ui_process      },
    { "system",  EVT_SYSTEM,  system_process  },
};

void app_main(void)
{
    bsp_init();
    /* TODO: init drivers and services here, in dependency order. */
    scheduler_run(k_modules, sizeof k_modules / sizeof k_modules[0]);
}
