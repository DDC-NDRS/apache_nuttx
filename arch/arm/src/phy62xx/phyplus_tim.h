/****************************************************************************
 * arch/arm/src/phy62xx/phyplus_tim.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __ARCH_ARM_SRC_PHY62XX_PHYPLUS_TIM_H
#define __ARCH_ARM_SRC_PHY62XX_PHYPLUS_TIM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "chip.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Helpers ******************************************************************/

/****************************************************************************
 * #define PHYPLUS_TIM_START(d)                         ((d)->ops->start(d))
 * #define PHYPLUS_TIM_STOP(d)                           ((d)->ops->stop(d))
 * #define PHYPLUS_TIM_CLEAR(d)                         ((d)->ops->clear(d))
 * #define PHYPLUS_TIM_SETMODE(d,mode)          ((d)->ops->setmode(d,mode))
 * #define PHYPLUS_TIM_GETCOUNTER(d,value)    ((d)->ops->getcounter(d,value))
 * #define PHYPLUS_TIM_SETCOUNTER(d,value)    ((d)->ops->setcounter(d,value))
 * #define PHYPLUS_TIM_SETISR(d,hnd,arg)        ((d)->ops->setisr(d,hnd,arg))
 * #define PHYPLUS_TIM_ENABLEINT(d)             ((d)->ops->enableint(d))
 * #define PHYPLUS_TIM_DISABLEINT(d)            ((d)->ops->disableint(d))
 * #define PHYPLUS_TIM_ACKINT(d)                   ((d)->ops->ackint(d))
 * #define PHYPLUS_TIM_SETISR(d,hnd,arg,s)    ((d)->ops->setisr(d,hnd,arg,s))
 * #define PHYPLUS_TIM_ENABLEINT(d,s)              ((d)->ops->enableint(d,s))
 * #define PHYPLUS_TIM_DISABLEINT(d,s)            ((d)->ops->disableint(d,s))
 * #define PHYPLUS_TIM_ACKINT(d,s)                   ((d)->ops->ackint(d,s))
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifndef __ASSEMBLY__
#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* register information... */

#define TIM_COUNT_OFFSET                  0x0000
#define TIM_CURRENT_OFFSET                0x0004
#define TIM_CONTROLREG_OFFSET             0x0008
#define TIM_EOI_OFFSET                    0x000c
#define TIM_STATUS_OFFSET                 0x0010

#define TIM_ENABLE                        (BIT(0))
#define TIM_MODE                          (BIT(1))
#define TIM_MASK                          (BIT(2))

#define TIM_EOI                           (BIT(0))
#define TIM_STATUS                        (BIT(0))

/* TIM Device Structure */

struct phyplus_tim_dev_s
{
  struct phyplus_tim_ops_s *ops;
};

typedef enum
{
  PHYPLUS_TIM_FREERUN = 0,
  PHYPLUS_TIM_COUNT,
}phyplus_tim_mode_t;

struct phyplus_tim_ops_s
{
  /* Basic Timers */

  void (*start)(struct phyplus_tim_dev_s *dev);
  void (*stop)(struct phyplus_tim_dev_s *dev);
  void (*clear)(struct phyplus_tim_dev_s *dev);
  void (*setmode)(struct phyplus_tim_dev_s *dev,
      phyplus_tim_mode_t mode);
  void (*getcounter)(struct phyplus_tim_dev_s *dev, uint32_t *value);
  void (*setcounter)(struct phyplus_tim_dev_s *dev, uint32_t value);
  int (*setisr)(struct phyplus_tim_dev_s *dev, xcpt_t handler,
      void *arg);
  void (*enableint)(struct phyplus_tim_dev_s *dev);
  void (*disableint)(struct phyplus_tim_dev_s *dev);
  void (*ackint)(struct phyplus_tim_dev_s *dev);
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct phyplus_tim_dev_s *phyplus_tim_init(int timer);
void phyplus_tim_deinit(struct phyplus_tim_dev_s *dev);

void phyplus_tim_start(struct phyplus_tim_dev_s *dev);
void phyplus_tim_stop(struct phyplus_tim_dev_s *dev);

void phyplus_tim_clear(struct phyplus_tim_dev_s *dev);
void phyplus_tim_setmode(struct phyplus_tim_dev_s *dev,
    phyplus_tim_mode_t mode);

void phyplus_tim_enableint(struct phyplus_tim_dev_s *dev);
void phyplus_tim_disableint(struct phyplus_tim_dev_s *dev);

void phyplus_tim_getcounter(struct phyplus_tim_dev_s *dev,
    uint32_t *value);

void phyplus_tim_setcounter(struct phyplus_tim_dev_s *dev,
    uint32_t value);

void phyplus_tim_getcurrent(struct phyplus_tim_dev_s *dev,
    uint32_t *value);

void phyplus_tim_getcontrolreg(struct phyplus_tim_dev_s *dev,
    uint32_t *value);

int phyplus_tim_setisr(struct phyplus_tim_dev_s *dev, xcpt_t handler,
    void *arg);

void phyplus_tim_ackint(struct phyplus_tim_dev_s *dev);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_ARM_SRC_PHY62XX_PHYPLUS_TIM_H */
