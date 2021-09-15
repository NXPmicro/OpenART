// <MCU>_prefix.c becomes the initial portion of the generated pins file.
/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */
// args= ['.\\make-pins.py', '-b', '.\\pins.csv']
// This file was automatically generated by make-pins.py
//
// --af ./1170_af.csv
// --pinmap ./rt1170_fsl_iomuxc.csv
// --board .\pins.csv
// --prefix ./mimxrt117x_prefix.c

// <MCU>_prefix.c becomes the initial portion of the generated pins file.

#include <stdio.h>

#include "py/obj.h"
#include "py/mphal.h"
#include <drivers/pin.h>
#include "pin_defs_mcu.h"


#define AF(af_idx, af_fn, af_unit, af_type, af_ptr, af_inSelReg, af_inSelVal) \
{ \
    { &pin_af_type }, \
    .name = MP_QSTR_AF ## af_idx ## _ ## af_fn ## af_unit, \
    .idx = (af_idx), \
    .fn = AF_FN_ ## af_fn, \
    .unit = (af_unit), \
    .af_pin_type = AF_PIN_TYPE_ ## af_fn ## _ ## af_type, \
    .reg = (af_ptr), \
    .inSelReg = (af_inSelReg), \
    .inSelVal = (af_inSelVal), \
}

#define PIN(p_name, p_board_name, p_port, p_pin, p_port_m4, p_pin_m4, p_af, p_adc_num, p_adc_channel, p_afReg, p_cfgReg) \
{ \
    { &pyb_pin_type }, \
    .name = MP_QSTR_ ## p_name, \
    .board_name = MP_QSTR_ ## p_board_name, \
    .port = PORT_ ## p_port, \
    .pin = (p_pin), \
    .port_m4 = PORT_ ## p_port_m4, \
    .pin_m4 = (p_pin_m4), \
    .num_af = (sizeof(p_af) / sizeof(pin_af_obj_t)), \
    .pin_mask = (1UL << ((p_pin) & 0x1f)), \
    .gpio = GPIO ## p_port, \
    .af = p_af, \
    .adc_num = p_adc_num, \
    .adc_channel = p_adc_channel, \
    .afReg = p_afReg, \
    .cfgReg = p_cfgReg, \
}

const pin_af_obj_t pin_AD_04_af[] = {
  //( 0, SIM     ,  1, PD              , SIM1       , 0x400E86A0U, 0x1U), // SIM1_PD
  AF( 1, LPUART  ,  8, CTS_B           , LPUART8    , 0          , 0   ), // LPUART8_CTS_B
  //( 2, ENET    ,  0, 1G_1588_EVENT3_IN, ENET       , 0          , 0   ), // ENET_1G_1588_EVENT3_IN
  AF( 3, GPT     ,  2, COMPARE3        , GPT2       , 0          , 0   ), // GPT2_COMPARE3
  AF( 4, PWM     ,  1, PWMA2           , PWM1       , 0x400E8508U, 0x1U), // FLEXPWM1_PWMA2
  AF( 5, GPIO    ,  3, PIN3            , GPIO3      , 0          , 0   ), // GPIO3_PIN3
  //( 6, WDOG    ,  1, WDOG_B          , WDOG1      , 0          , 0   ), // WDOG1_WDOG_B
  //( 7, TPSMP   ,  0, HADDR9          , TPSMP      ,            ,     ), // TPSMP_HADDR9
  //( 8, FLEXIO  ,  2, FLEXIO4         , FLEXIO2    , 0          , 0   ), // FLEXIO2_FLEXIO4
  //( 9, QTIMER  ,  4, TIMER0          , QTIMER4    , 0x400E8660U, 0x1U), // QTIMER4_TIMER0
  AF(10, GPIO    ,  9, PIN3            , GPIO9      , 0          , 0   ), // GPIO9_PIN3
};

const pin_obj_t pin_AD_04 = PIN(AD_04, LED1, 3, 3, 9, 3, pin_AD_04_af, 0, 0, 0x400E811CU, 0x400E8360U);

const pin_af_obj_t pin_AD_26_af[] = {
  AF( 0, LPUART  ,  1, CTS_B           , LPUART1    , 0          , 0   ), // LPUART1_CTS_B
  AF( 1, LPSPI   ,  2, SDO             , LPSPI2     , 0x400E85ECU, 0x0U), // LPSPI2_SDO
  //( 2, SEMC    ,  0, CSX1            , SEMC       , 0          , 0   ), // SEMC_CSX1
  //( 3, ENET    ,  0, RDATA0          , ENET       , 0x400E84B0U, 0x0U), // ENET_RDATA0
  AF( 4, PWM     ,  2, PWMA1           , PWM2       , 0x400E851CU, 0x1U), // FLEXPWM2_PWMA1
  AF( 5, GPIO    ,  3, PIN25           , GPIO3      , 0          , 0   ), // GPIO3_PIN25
  //( 6, KPP     ,  0, ROW4            , KPP        , 0          , 0   ), // KPP_ROW4
  //( 7, TPSMP   ,  0, HADDR31         , TPSMP      ,            ,     ), // TPSMP_HADDR31
  //( 8, FLEXIO  ,  2, FLEXIO26        , FLEXIO2    , 0          , 0   ), // FLEXIO2_FLEXIO26
  //( 9, ENET    ,  0, QOS_MDC         , ENET       , 0          , 0   ), // ENET_QOS_MDC
  AF(10, GPIO    ,  9, PIN25           , GPIO9      , 0          , 0   ), // GPIO9_PIN25
  //(11, USDHC   ,  2, CD_B            , USDHC2     , 0x400E86D0U, 0x1U), // USDHC2_CD_B
};

const pin_obj_t pin_AD_26 = PIN(AD_26, LED2, 3, 25, 9, 25, pin_AD_26_af, 0, 0, 0x400E8174U, 0x400E83B8U);

STATIC const mp_rom_map_elem_t pin_cpu_pins_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_AD_04), MP_ROM_PTR(&pin_AD_04) },
  { MP_ROM_QSTR(MP_QSTR_AD_26), MP_ROM_PTR(&pin_AD_26) },
};
MP_DEFINE_CONST_DICT(pin_cpu_pins_locals_dict, pin_cpu_pins_locals_dict_table);

STATIC const mp_rom_map_elem_t pin_board_pins_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_LED1), MP_ROM_PTR(&pin_AD_04) },
  { MP_ROM_QSTR(MP_QSTR_LED2), MP_ROM_PTR(&pin_AD_26) },
};
MP_DEFINE_CONST_DICT(pin_board_pins_locals_dict, pin_board_pins_locals_dict_table);

const pin_obj_t * const pin_adc1[] = {
  NULL,    // 0
  NULL,    // 1
  NULL,    // 2
  NULL,    // 3
  NULL,    // 4
  NULL,    // 5
  NULL,    // 6
  NULL,    // 7
  NULL,    // 8
  NULL,    // 9
  NULL,    // 10
  NULL,    // 11
  NULL,    // 12
  NULL,    // 13
  NULL,    // 14
  NULL,    // 15
  NULL,    // 16
};

const pin_obj_t * const pin_adc2[] = {
  NULL,    // 0
  NULL,    // 1
  NULL,    // 2
  NULL,    // 3
  NULL,    // 4
  NULL,    // 5
  NULL,    // 6
  NULL,    // 7
  NULL,    // 8
  NULL,    // 9
  NULL,    // 10
  NULL,    // 11
  NULL,    // 12
  NULL,    // 13
  NULL,    // 14
  NULL,    // 15
  NULL,    // 16
};

const pin_obj_t * const pin_acmp1[] = {
  NULL,    // 0
  NULL,    // 1
  NULL,    // 2
  NULL,    // 3
  NULL,    // 4
};

const pin_obj_t * const pin_acmp2[] = {
  NULL,    // 0
  &pin_AD_04, // 1
  NULL,    // 2
  &pin_AD_26, // 3
  NULL,    // 4
};

const pin_obj_t * const pin_acmp3[] = {
  NULL,    // 0
  NULL,    // 1
  NULL,    // 2
  NULL,    // 3
  NULL,    // 4
};

const pin_obj_t * const pin_acmp4[] = {
  NULL,    // 0
  NULL,    // 1
  NULL,    // 2
  NULL,    // 3
  NULL,    // 4
};
