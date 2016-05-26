/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2013 Semtech-Cycleo

Description:
    Functions used to handle FPGA register access for LoRa concentrator.
    Registers are addressed by name.
    Multi-bytes registers are handled automatically.
    Read-modify-write is handled automatically.

License: Revised BSD License, see LICENSE.TXT file include in the project
Maintainer: Michael Coracin
*/

/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf fprintf */

#include "loragw_spi.h"
#include "loragw_aux.h"
#include "loragw_hal.h"
#include "loragw_reg.h"
#include "loragw_fpga.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#if DEBUG_REG == 1
    #define DEBUG_MSG(str)                fprintf(stderr, str)
    #define DEBUG_PRINTF(fmt, args...)    fprintf(stderr,"%s:%d: "fmt, __FUNCTION__, __LINE__, args)
    #define CHECK_NULL(a)                if(a==NULL){fprintf(stderr,"%s:%d: ERROR: NULL POINTER AS ARGUMENT\n", __FUNCTION__, __LINE__);return LGW_REG_ERROR;}
#else
    #define DEBUG_MSG(str)
    #define DEBUG_PRINTF(fmt, args...)
    #define CHECK_NULL(a)                if(a==NULL){return LGW_REG_ERROR;}
#endif

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

/*
auto generated register mapping for C code : 11-Jul-2013 13:20:40
this file contains autogenerated C struct used to access the LoRa register from the Primer firmware
this file is autogenerated from registers description
293 registers are defined
*/
const struct lgw_reg_s fpga_regs[LGW_FPGA_TOTALREGS] = {
    {-1,0,0,0,1,0,0}, /* SOFT_RESET */
    {-1,0,1,0,7,1,0}, /* FPGA_FEATURE */
    {-1,1,0,0,8,1,0}, /* VERSION */
    {-1,2,0,0,8,1,0}, /* FPGA_STATUS */
    {-1,3,0,0,8,0,0}, /* FPGA_CTRL */
    {-1,4,0,0,8,0,0}, /* HISTO_RAM_ADDR */
    {-1,5,0,0,8,1,0}, /* HISTO_RAM_DATA */
    {-1,6,0,0,16,0,32000}, /* HISTO_TEMPO */
    {-1,8,0,0,16,0,1000}, /* HISTO_NB_READ */
    {-1,10,0,0,32,1,0}, /* TIMESTAMP */
    {-1,14,0,0,24,1,0}, /* LBT_TIMESTAMP_CH */
    {-1,17,0,0,8,0,0}, /* LBT_TIMESTAMP_SELECT_CH */
    {-1,18,0,0,8,0,8}, /* LBT_TIMESTAMP_NB_CH */
    {-1,19,0,0,8,0,7}, /* SPI_MASTER_SPEED_DIVIDER */
    {-1,20,0,0,8,0,10}, /* NB_READ_RSSI */
    {-1,21,0,0,8,0,10}, /* PLL_LOCK_TIME */
    {-1,22,0,0,8,0,160}, /* RSSI_TARGET */
    {-1,23,0,0,16,0,0}, /* LSB_START_FREQ */
    {-1,127,0,0,8,0,0} /* SPI_MUX_CTRL */
};

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

extern void *lgw_spi_target; /*! generic pointer to the SPI device */
extern uint8_t lgw_spi_mux_mode; /*! current SPI mux mode used */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

int lgw_fpga_configure(void) {
    int x;
    int32_t val;
    bool tx_filter_support, spectral_scan_support, lbt_support;

    /* FPGA settings */
    uint32_t input_sync_edge  = 0;
    uint32_t output_sync_edge = 0;
    uint32_t filt_on = 1;

    /* Get supported FPGA features */
    printf("INFO: FPGA supported features:");
    lgw_fpga_reg_r(LGW_FPGA_FPGA_FEATURE, &val);
    tx_filter_support = TAKE_N_BITS_FROM((uint8_t)val, 0, 1);
    if (tx_filter_support) {
        printf(" [TX filter] ");
    }
    spectral_scan_support = TAKE_N_BITS_FROM((uint8_t)val, 1, 1);
    if (spectral_scan_support) {
        printf(" [Spectral Scan] ");
    }
    lbt_support = TAKE_N_BITS_FROM((uint8_t)val, 2, 1);
    if (lbt_support) {
        printf(" [LBT] ");
    }
    printf("\n");

    /* Configure TX filter */
    x = lgw_fpga_reg_w(LGW_FPGA_FPGA_CTRL, (filt_on << 4) | (input_sync_edge << 2) | (output_sync_edge << 3) | (1 << 1)); /* Reset Radio */
    x |= lgw_fpga_reg_w(LGW_FPGA_FPGA_CTRL, (filt_on << 4) | (input_sync_edge << 2) | (output_sync_edge << 3));
    if( x != LGW_REG_SUCCESS )
    {
        DEBUG_MSG("ERROR: Failed to configure FPGA\n");
        return LGW_REG_ERROR;
    }

    return LGW_REG_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Write to a register addressed by name */
int lgw_fpga_reg_w(uint16_t register_id, int32_t reg_value) {
    int spi_stat = LGW_SPI_SUCCESS;
    struct lgw_reg_s r;

    /* check input parameters */
    if (register_id >= LGW_FPGA_TOTALREGS) {
        DEBUG_MSG("ERROR: REGISTER NUMBER OUT OF DEFINED RANGE\n");
        return LGW_REG_ERROR;
    }

    /* check if SPI is initialised */
    if (lgw_spi_target == NULL) {
        DEBUG_MSG("ERROR: CONCENTRATOR UNCONNECTED\n");
        return LGW_REG_ERROR;
    }

    /* get register struct from the struct array */
    r = fpga_regs[register_id];

    /* reject write to read-only registers */
    if (r.rdon == 1){
        DEBUG_MSG("ERROR: TRYING TO WRITE A READ-ONLY REGISTER\n");
        return LGW_REG_ERROR;
    }

    spi_stat += reg_w_align32(lgw_spi_target, LGW_SPI_MUX_MODE1, LGW_SPI_MUX_TARGET_FPGA, r, reg_value);

    if (spi_stat != LGW_SPI_SUCCESS) {
        DEBUG_MSG("ERROR: SPI ERROR DURING REGISTER WRITE\n");
        return LGW_REG_ERROR;
    } else {
        return LGW_REG_SUCCESS;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Read to a register addressed by name */
int lgw_fpga_reg_r(uint16_t register_id, int32_t *reg_value) {
    int spi_stat = LGW_SPI_SUCCESS;
    struct lgw_reg_s r;

    /* check input parameters */
    CHECK_NULL(reg_value);
    if (register_id >= LGW_FPGA_TOTALREGS) {
        DEBUG_MSG("ERROR: REGISTER NUMBER OUT OF DEFINED RANGE\n");
        return LGW_REG_ERROR;
    }

    /* check if SPI is initialised */
    if (lgw_spi_target == NULL) {
        DEBUG_MSG("ERROR: CONCENTRATOR UNCONNECTED\n");
        return LGW_REG_ERROR;
    }

    /* get register struct from the struct array */
    r = fpga_regs[register_id];

    spi_stat += reg_r_align32(lgw_spi_target, LGW_SPI_MUX_MODE1, LGW_SPI_MUX_TARGET_FPGA, r, reg_value);

    if (spi_stat != LGW_SPI_SUCCESS) {
        DEBUG_MSG("ERROR: SPI ERROR DURING REGISTER WRITE\n");
        return LGW_REG_ERROR;
    } else {
        return LGW_REG_SUCCESS;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Point to a register by name and do a burst write */
int lgw_fpga_reg_wb(uint16_t register_id, uint8_t *data, uint16_t size) {
    int spi_stat = LGW_SPI_SUCCESS;
    struct lgw_reg_s r;

    /* check input parameters */
    CHECK_NULL(data);
    if (size == 0) {
        DEBUG_MSG("ERROR: BURST OF NULL LENGTH\n");
        return LGW_REG_ERROR;
    }
    if (register_id >= LGW_FPGA_TOTALREGS) {
        DEBUG_MSG("ERROR: REGISTER NUMBER OUT OF DEFINED RANGE\n");
        return LGW_REG_ERROR;
    }

    /* check if SPI is initialised */
    if (lgw_spi_target == NULL) {
        DEBUG_MSG("ERROR: CONCENTRATOR UNCONNECTED\n");
        return LGW_REG_ERROR;
    }

    /* get register struct from the struct array */
    r = fpga_regs[register_id];

    /* reject write to read-only registers */
    if (r.rdon == 1){
        DEBUG_MSG("ERROR: TRYING TO BURST WRITE A READ-ONLY REGISTER\n");
        return LGW_REG_ERROR;
    }

    /* do the burst write */
    spi_stat += lgw_spi_wb(lgw_spi_target, LGW_SPI_MUX_MODE1, LGW_SPI_MUX_TARGET_FPGA, r.addr, data, size);

    if (spi_stat != LGW_SPI_SUCCESS) {
        DEBUG_MSG("ERROR: SPI ERROR DURING REGISTER BURST WRITE\n");
        return LGW_REG_ERROR;
    } else {
        return LGW_REG_SUCCESS;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Point to a register by name and do a burst read */
int lgw_fpga_reg_rb(uint16_t register_id, uint8_t *data, uint16_t size) {
    int spi_stat = LGW_SPI_SUCCESS;
    struct lgw_reg_s r;

    /* check input parameters */
    CHECK_NULL(data);
    if (size == 0) {
        DEBUG_MSG("ERROR: BURST OF NULL LENGTH\n");
        return LGW_REG_ERROR;
    }
    if (register_id >= LGW_FPGA_TOTALREGS) {
        DEBUG_MSG("ERROR: REGISTER NUMBER OUT OF DEFINED RANGE\n");
        return LGW_REG_ERROR;
    }

    /* check if SPI is initialised */
    if (lgw_spi_target == NULL) {
        DEBUG_MSG("ERROR: CONCENTRATOR UNCONNECTED\n");
        return LGW_REG_ERROR;
    }

    /* get register struct from the struct array */
    r = fpga_regs[register_id];

    /* do the burst read */
    spi_stat += lgw_spi_rb(lgw_spi_target, LGW_SPI_MUX_MODE1, LGW_SPI_MUX_TARGET_FPGA, r.addr, data, size);

    if (spi_stat != LGW_SPI_SUCCESS) {
        DEBUG_MSG("ERROR: SPI ERROR DURING REGISTER BURST READ\n");
        return LGW_REG_ERROR;
    } else {
        return LGW_REG_SUCCESS;
    }
}

/* --- EOF ------------------------------------------------------------------ */
