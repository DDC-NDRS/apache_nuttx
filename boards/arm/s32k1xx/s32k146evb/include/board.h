/****************************************************************************
 * boards/arm/s32k1xx/s32k146evb/include/board.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __BOARDS_ARM_S32K1XX_S32K146EVB_INCLUDE_BOARD_H
#define __BOARDS_ARM_S32K1XX_S32K146EVB_INCLUDE_BOARD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <hardware/s32k1xx_pinmux.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Clocking *****************************************************************/

/* The S32K146EVB is fitted with a 8 MHz crystal */
#define BOARD_XTAL_FREQUENCY            8000000UL   /* BSP_SPECIFIC */

/* The S32K146 will run at 80 MHz in RUN mode */
#define S32K146EVB_RUN_SYSCLK_FREQUENCY 80000000UL

/* LED definitions **********************************************************/
/* The S32K146EVB has one RGB LED:
 *
 *   RedLED    PTD15  (FTM0 CH0)
 *   GreenLED  PTD16  (FTM0 CH1)
 *   BlueLED   PTD0   (FTM0 CH2)
 *
 * If CONFIG_ARCH_LEDS is not defined, then the user can control the LEDs in
 * any way.  The following definitions are used to access individual RGB
 * components.
 *
 * The RGB components could, alternatively be controlled through PWM using
 * the common RGB LED driver.
 */

/* LED index values for use with board_userled() */
#define BOARD_LED_R     0       /* BSP_SPECIFIC */
#define BOARD_LED_G     1       /* BSP_SPECIFIC */
#define BOARD_LED_B     2       /* BSP_SPECIFIC */
#define BOARD_NLEDS     3       /* BSP_SPECIFIC */

/* LED bits for use with board_userled_all() */
#define BOARD_LED_R_BIT (1 << BOARD_LED_R)
#define BOARD_LED_G_BIT (1 << BOARD_LED_G)
#define BOARD_LED_B_BIT (1 << BOARD_LED_B)

/* If CONFIG_ARCH_LEDs is defined, then NuttX will control the LEDs on board
 * the S32K146EVB.  The following definitions describe how NuttX controls the
 * LEDs:
 *
 *      SYMBOL              Meaning                           LED state
 *                                                            RED    GREEN  BLUE
 *      ------------------  --------------------------------  -------------------
 */
#define LED_STARTED         1   /* NuttX has been started     OFF    OFF    OFF */
#define LED_HEAPALLOCATE    2   /* Heap has been allocated    OFF    OFF    ON  */
#define LED_IRQSENABLED     0   /* Interrupts enabled         OFF    OFF    ON  */
#define LED_STACKCREATED    3   /* Idle stack created         OFF    ON     OFF */
#define LED_INIRQ           0   /* In an interrupt           (No change)        */
#define LED_SIGNAL          0   /* In a signal handler       (No change)        */
#define LED_ASSERTION       0   /* An assertion failed       (No change)        */
#define LED_PANIC           4   /* The system has crashed     FLASH  OFF    OFF */
#undef  LED_IDLE                /* S32K146 is in sleep mode  (Not used)         */

/* Button definitions *******************************************************/
/* The S32K146EVB supports two buttons:
 *   SW2  PTC12
 *   SW3  PTC13
 */
#define BUTTON_SW2          0
#define BUTTON_SW3          1
#define NUM_BUTTONS         2

#define BUTTON_SW2_BIT      (1 << BUTTON_SW2)
#define BUTTON_SW3_BIT      (1 << BUTTON_SW3)

#define PIN_SW2     (PIN_PTC12 | GPIO_INPUT)  /* SW2  PTC12 */
#define PIN_SW3     (PIN_PTC13 | GPIO_INPUT)  /* SW3  PTC13 */

#define PIN_LED_R   (PIN_PTD15 | GPIO_LOWDRIVE | GPIO_OUTPUT_ZERO)
#define PIN_LED_G   (PIN_PTD16 | GPIO_LOWDRIVE | GPIO_OUTPUT_ZERO)
#define PIN_LED_B   (PIN_PTD0  | GPIO_LOWDRIVE | GPIO_OUTPUT_ZERO)

/* UART selections **********************************************************/
/* By default, the serial console will be provided on the OpenSDA VCOM port:
 *
 *   OpenSDA UART RX  PTC6  (LPUART1_RX)
 *   OpenSDA UART TX  PTC7  (LPUART1_TX)
 */
#define PIN_LPUART1_RX  PIN_LPUART1_RX_1    /* PTC6_LPUART1_RX */
#define PIN_LPUART1_TX  PIN_LPUART1_TX_1    /* PTC7_LPUART1_TX */

/* SPI selections ***********************************************************/
/* MC33664ATL1EGR2 TX (LPSPI0 : Master) */
#define PIN_LPSPI0_SCK  PIN_LPSPI0_SCK_4    /* PTD15_LPSPI0_SCK  */
#define PIN_LPSPI0_MISO PIN_LPSPI0_SOUT_3   /* PTB04_LPSPI0_SOUT */
#define PIN_LPSPI0_MOSI PIN_LPSPI0_SIN_3    /* PTD16_LPSPI0_SIN  */
#define PIN_LPSPI0_PCS  PIN_LPSPI0_PCS0_3   /* PTB05_LPSPI0_CS   */

/* UJA1169TK/F SBC SPI  (LPSPI1) What ? both share the same I/F !!! */
/* MC33664ATL1EGR2 RX (LPSPI1 : Slave) */
#define PIN_LPSPI1_SCK  PIN_LPSPI1_SCK_2    /* PTB14_LPSPI1_SCK  */
#define PIN_LPSPI1_MISO PIN_LPSPI1_SIN_1    /* PTB15_LPSPI1_SIN  */
#define PIN_LPSPI1_MOSI PIN_LPSPI1_SOUT_2   /* PTB16_LPSPI1_SOUT */
#define PIN_LPSPI1_PCS  PIN_LPSPI1_PCS3     /* PTB17_LPSPI1_CS   */

/* CAN selections ***********************************************************/
/* UJA1169TK/F SBC CAN  (CAN0) */
#define PIN_CAN0_RX PIN_CAN0_RX_4           /* PTE4_CAN0_RX */
#define PIN_CAN0_TX PIN_CAN0_TX_4           /* PTE5_CAN0_TX */

/* CAN1/CAN2 configuration */
#if defined(CONFIG_BOARD_BMU_A23C)
#define PIN_CAN1_RX PIN_CAN1_RX_1           /* PTA12_CAN1_RX */
#define PIN_CAN1_TX PIN_CAN1_TX_1           /* PTA13_CAN1_TX */

#define PIN_CAN2_RX PIN_CAN2_RX_1           /* PTC16_CAN2_RX */
#define PIN_CAN2_TX PIN_CAN2_TX_1           /* PTC17_CAN2_TX */
#else
#define PIN_CAN1_RX PIN_CAN1_RX_1           /* PTA12_CAN1_RX */
#define PIN_CAN1_TX PIN_CAN1_TX_1           /* PTA13_CAN1_TX */

#define PIN_CAN2_RX PIN_CAN2_RX_1           /* PTC16_CAN2_RX */
#define PIN_CAN2_TX PIN_CAN2_TX_1           /* PTC17_CAN2_TX */
#endif

/* LPUART0/LPUART2 configuration */
#if defined(CONFIG_BOARD_BMU_A23C)
#define PIN_LPUART0_RX  PIN_LPUART0_RX_1    /* PTB0_LPUART0_RX */
#define PIN_LPUART0_TX  PIN_LPUART0_TX_1    /* PTB1_LPUART0_TX */

#define PIN_LPUART2_RX  PIN_LPUART2_RX_2    /* PTD6_LPUART2_RX */
#define PIN_LPUART2_TX  PIN_LPUART2_TX_2    /* PTD7_LPUART2_TX */
#else /* S32K146EVB */
#define PIN_LPUART0_RX  PIN_LPUART0_RX_2    /* PTA28 */
#define PIN_LPUART0_TX  PIN_LPUART0_TX_2    /* PTA27 */

#define PIN_LPUART2_RX  PIN_LPUART2_RX_1    /* PTA8 */
#define PIN_LPUART2_TX  PIN_LPUART2_TX_1    /* PTA9 */
#endif

/* LPI2C0 configuration */
#if defined(CONFIG_BOARD_BMU_A23C)
#define PIN_LPI2C0_SCL  PIN_LPI2C0_SCL_2    /* PTA3_LPI2C0_SCL */
#define PIN_LPI2C0_SDA  PIN_LPI2C0_SDA_2    /* PTA2_LPI2C0_SDA */
#else
#define PIN_LPI2C0_SCL  PIN_LPI2C0_SCL_2    /* PTA3(N/A) */
#define PIN_LPI2C0_SDA  PIN_LPI2C0_SDA_2    /* PTA2(N/A) */
#endif

/* GPIO pins to be registered to the GPIO driver.  These definitions need to
 * be added to the g_gpiopins array in s32k1xx_gpio.c!
 */
#define DEV_GPIO0   (PIN_PTD15 | GPIO_OUTPUT)   /* RedLED    PTD15  (FTM0 CH0) */
#define DEV_GPIO1   (PIN_PTD16 | GPIO_OUTPUT)   /* GreenLED  PTD16  (FTM0 CH1) */
#define DEV_GPIO2   (PIN_PTD0  | GPIO_OUTPUT)   /* BlueLED   PTD0   (FTM0 CH2) */
#define DEV_GPIO3   (PIN_PTC12 | PIN_INT_BOTH)  /* SW2  PTC12 */
#define DEV_GPIO4   (PIN_PTC13 | PIN_INT_BOTH)  /* SW3  PTC13 */

// #define DEV_GPIO5   (PIN_PTE8  | GPIO_INPUT | PIN_INT_BOTH)
// #define DEV_GPIO6   (PIN_PTC3  | GPIO_INPUT | PIN_INT_BOTH)
// #define DEV_GPIO7   (PIN_PTC14 | GPIO_INPUT | PIN_INT_BOTH)
// #define DEV_GPIO8   (PIN_PTC8  | GPIO_INPUT | PIN_INT_BOTH)
// #define DEV_GPIO9   (PIN_PTA11 | GPIO_INPUT | PIN_INT_BOTH)
// #define DEV_GPIO10  (PIN_PTC9  | GPIO_INPUT | PIN_INT_BOTH)
// #define DEV_GPIO11  (PIN_PTA13 | GPIO_INPUT | PIN_INT_BOTH)

#define DEV_NUM_OF_GPIO 5

/* VNQ9080AJ High-Side Driver
 *
 * CONTACT1             PTD18   control input pin
 * CONTACT2             PTD19
 * CONTACT3             PTD22
 * CONTACT4             PTD23
 *
 * VNQ_SEN              PTD24   enable CS diagnostic
 * VNQ_FAULT_RST        PTD27   auto restart mode active low
 * VNQ_SEL0             PTD28   CS multiplexer
 * VNQ_CS               PTD29   input pin
 * VNQ_SEL1             PTD30
 */
#define PIN_CONF_CONTACT1        (PIN_PTD18 | GPIO_OUTPUT | GPIO_OUTPUT_ZERO)
#define PIN_CONF_CONTACT2        (PIN_PTD19 | GPIO_OUTPUT | GPIO_OUTPUT_ZERO)
#define PIN_CONF_CONTACT3        (PIN_PTD22 | GPIO_OUTPUT | GPIO_OUTPUT_ZERO)
#define PIN_CONF_CONTACT4        (PIN_PTD23 | GPIO_OUTPUT | GPIO_OUTPUT_ZERO)

#define PIN_CONF_VNQ_SEN         (PIN_PTD24 | GPIO_OUTPUT | GPIO_OUTPUT_ZERO)
#define PIN_CONF_VNQ_FAULT_RST   (PIN_PTD27 | GPIO_OUTPUT | GPIO_OUTPUT_ONE)
#define PIN_CONF_VNQ_SEL0        (PIN_PTD28 | GPIO_OUTPUT | GPIO_OUTPUT_ZERO)
#define PIN_CONF_VNQ_CS          (PIN_PTD29 | GPIO_INPUT)
#define PIN_CONF_VNQ_SEL1        (PIN_PTD30 | GPIO_OUTPUT | GPIO_OUTPUT_ZERO)

/* UCC21750DWR IGBT driver 
 * 
 * UCC_INP      PTE20   Control pin
 * UCC_INN      PTE21   OUTPUT always LOW
 * UCC_RDY      PTE22   INPUT active HIGH
 * UCC_FLT      PTE23   INPUT active low
 * UCC_EN       PTE24   OUTPUT (RST)
 */
#define PIN_CONF_UCC_INP        (PIN_PTE20 | GPIO_OUTPUT | GPIO_OUTPUT_ZERO)
#define PIN_CONF_UCC_INN        (PIN_PTE21 | GPIO_OUTPUT | GPIO_OUTPUT_ZERO)
#define PIN_CONF_UCC_RDY        (PIN_PTE22 | GPIO_INPUT)
#define PIN_CONF_UCC_FLT        (PIN_PTE23 | GPIO_INPUT)
#define PIN_CONF_UCC_EN         (PIN_PTE24 | GPIO_OUTPUT | GPIO_OUTPUT_ONE)

/* MC33664 Isolated network high-speed transceiver for BCC
 * BCC_INT      PTB12   Input interrupt
 * BCC_EN       PTE8    Output chip enable
 * BCC_PCS      PTB5    Output chip select for wakeup
 */

/* VNQ9080AJ High-Side Driver */
#define DEV_CONTACT1        "/dev/gpio0"    // PTD18 OUTPUT_ZERO
#define DEV_CONTACT2        "/dev/gpio1"    // PTD19 OUTPUT_ZERO
#define DEV_CONTACT3        "/dev/gpio2"    // PTD22 OUTPUT_ZERO
#define DEV_CONTACT4        "/dev/gpio3"    // PTD23 OUTPUT_ZERO
#define DEV_VNQ_SEN         "/dev/gpio4"    // PTD24 OUTPUT_ZERO
#define DEV_VNQ_FAULT_RST   "/dev/gpio5"    // PTD27 OUTPUT_ONE
#define DEV_VNQ_SEL0        "/dev/gpio6"    // PTD28 OUTPUT_ZERO
#define DEV_VNQ_CS          "/dev/gpio7"    // PTD29 INPUT
#define DEV_VNQ_SEL1        "/dev/gpio8"    // PTD30 OUTPUT_ZERO

/* UCC21750DWR IGBT driver */
#define DEV_UCC_INP         "/dev/gpio9"    // PTE20 OUTPUT_ZERO
#define DEV_UCC_INN         "/dev/gpio10"   // PTE21 OUTPUT_ZERO
#define DEV_UCC_RDY         "/dev/gpio11"   // PTE22 INPUT
#define DEV_UCC_FLT         "/dev/gpio12"   // PTE23 INPUT
#define DEV_UCC_EN          "/dev/gpio13"   // PTE24 OUTPUT_ONE

#endif /* __BOARDS_ARM_S32K1XX_S32K146EVB_INCLUDE_BOARD_H */
