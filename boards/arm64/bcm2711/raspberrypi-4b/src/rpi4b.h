/****************************************************************************
 * boards/arm64/bcm2711/raspberrypi-4b/src/rpi4b.h
 *
 * Author: Matteo Golin
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

#ifndef __BOARDS_ARM64_BCM2711_RASPBERRYPI_4B_SRC_RPI4B_H
#define __BOARDS_ARM64_BCM2711_RASPBERRYPI_4B_SRC_RPI4B_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Functions Definitions
 ****************************************************************************/

/****************************************************************************
 * Name: rpi4b_bringup
 *
 * Description:
 *   Bring up board features
 *
 ****************************************************************************/

#if defined(CONFIG_BOARDCTL) || defined(CONFIG_BOARD_LATE_INITIALIZE)
int rpi4b_bringup(void);
#endif

#if defined(CONFIG_DEV_GPIO)
int bcm2711_dev_gpio_init(void);
#endif /* defined(CONFIG_DEV_GPIO) */

#endif /* __BOARDS_ARM64_BCM2711_RASPBERRYPI_4B_SRC_RPI4B_H */
