/* Online demo: the part of sdmmc_card_t the Hardware information page reads. */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "sd_protocol_defs.h"
typedef struct { int capacity; int sector_size; } sdmmc_csd_t;
typedef struct { char name[8]; } sdmmc_cid_t;
typedef struct {
    uint32_t ocr;
    sdmmc_cid_t cid;
    sdmmc_csd_t csd;
    uint32_t is_mmc : 1;
    uint32_t is_sdio : 1;
    int real_freq_khz;
} sdmmc_card_t;
