/****************************************************************************
 * arch/xtensa/src/esp32/esp32_spiflash.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_ESP32_SPIFLASH

#include <stdint.h>
#include <assert.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/init.h>
#include <nuttx/kthread.h>
#include <nuttx/mutex.h>
#include <nuttx/mtd/mtd.h>

#include "sched/sched.h"

#include "xtensa.h"
#include "xtensa_attr.h"

#include "rom/esp32_spiflash.h"

#include "hardware/esp32_soc.h"
#include "hardware/esp32_spi.h"
#include "hardware/esp32_dport.h"
#include "hardware/esp32_efuse.h"

#include "esp32_spicache.h"
#ifdef CONFIG_ESP32_SPIRAM
#include "esp32_spiram.h"
#endif
#include "esp32_irq.h"

#include "esp32_spiflash.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SPI_FLASH_WRITE_BUF_SIZE    (32)
#define SPI_FLASH_READ_BUF_SIZE     (64)

#define SPI_FLASH_WRITE_WORDS       (SPI_FLASH_WRITE_BUF_SIZE / 4)
#define SPI_FLASH_READ_WORDS        (SPI_FLASH_READ_BUF_SIZE / 4)

#define SPI_FLASH_MMU_PAGE_SIZE     (0x10000)

#define SPI_FLASH_ENCRYPT_UNIT_SIZE (32)
#define SPI_FLASH_ENCRYPT_WORDS     (32 / 4)
#define SPI_FLASH_ERASED_STATE      (0xff)

#define SPI_FLASH_ENCRYPT_MIN_SIZE  (16)

#define MTD2PRIV(_dev)              ((struct esp32_spiflash_s *)_dev)
#define MTD_SIZE(_priv)             ((_priv)->chip->chip_size)
#define MTD_BLKSIZE(_priv)          ((_priv)->chip->page_size)
#define MTD_ERASESIZE(_priv)        ((_priv)->chip->sector_size)
#define MTD_BLK2SIZE(_priv, _b)     (MTD_BLKSIZE(_priv) * (_b))
#define MTD_SIZE2BLK(_priv, _s)     ((_s) / MTD_BLKSIZE(_priv))

#define MMU_ADDR2PAGE(_addr)        ((_addr) / SPI_FLASH_MMU_PAGE_SIZE)
#define MMU_ADDR2OFF(_addr)         ((_addr) % SPI_FLASH_MMU_PAGE_SIZE)
#define MMU_BYTES2PAGES(_n)         (((_n) + SPI_FLASH_MMU_PAGE_SIZE - 1) \
                                     / SPI_FLASH_MMU_PAGE_SIZE)
#define MMU_ALIGNUP_SIZE(_s)        (((_s) + SPI_FLASH_MMU_PAGE_SIZE - 1) \
                                     & ~(SPI_FLASH_MMU_PAGE_SIZE - 1))
#define MMU_ALIGNDOWN_SIZE(_s)      ((_s) & ~(SPI_FLASH_MMU_PAGE_SIZE - 1))

/* Flash MMU table for PRO CPU */

#define PRO_MMU_TABLE ((volatile uint32_t *)DPORT_PRO_FLASH_MMU_TABLE_REG)

/* Flash MMU table for APP CPU */

#define APP_MMU_TABLE ((volatile uint32_t *)DPORT_APP_FLASH_MMU_TABLE_REG)

#define PRO_IRAM0_FIRST_PAGE  ((SOC_IRAM_LOW - SOC_DRAM_HIGH) /\
                               (SPI_FLASH_MMU_PAGE_SIZE + IROM0_PAGES_START))

#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
/* SPI flash work operation code */

enum spiflash_op_code_e
{
  SPIFLASH_OP_CODE_WRITE = 0,
  SPIFLASH_OP_CODE_READ,
  SPIFLASH_OP_CODE_ERASE,
  SPIFLASH_OP_CODE_SET_BANK,
  SPIFLASH_OP_CODE_ENCRYPT_READ,
  SPIFLASH_OP_CODE_ENCRYPT_WRITE
};
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* SPI Flash device hardware configuration */

struct esp32_spiflash_config_s
{
  /* SPI register base address */

  uint32_t reg_base;
};

/* SPI Flash device private data  */

struct esp32_spiflash_s
{
  struct mtd_dev_s mtd;

  /* Port configuration */

  struct esp32_spiflash_config_s *config;

  /* SPI Flash data */

  esp32_spiflash_chip_t *chip;

  /* SPI Flash communication dummy number */

  uint8_t *dummies;
};

/* SPI Flash map request data */

struct spiflash_map_req
{
  /* Request mapping SPI Flash base address */

  uint32_t  src_addr;

  /* Request mapping SPI Flash size */

  uint32_t  size;

  /* Mapped memory pointer */

  void      *ptr;

  /* Mapped started MMU page index */

  uint32_t  start_page;

  /* Mapped MMU page count */

  uint32_t  page_cnt;
};

#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
/* SPI flash work operation arguments */

struct spiflash_work_arg
{
  enum spiflash_op_code_e op_code;

  struct
  {
    struct esp32_spiflash_s *priv;
    uint32_t addr;
    uint8_t *buffer;
    uint32_t size;
    uint32_t paddr;
  } op_arg;

  volatile int ret;

  sem_t sem;
};
#endif

/****************************************************************************
 * ROM function prototypes
 ****************************************************************************/

extern void cache_flush(int cpu);

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

/* SPI helpers */

static inline void spi_set_reg(struct esp32_spiflash_s *priv,
                               int offset, uint32_t value);
static inline uint32_t spi_get_reg(struct esp32_spiflash_s *priv,
                                   int offset);
static inline void spi_set_regbits(struct esp32_spiflash_s *priv,
                                   int offset, uint32_t bits);
static inline void spi_reset_regbits(struct esp32_spiflash_s *priv,
                                     int offset, uint32_t bits);

/* Misc. helpers */

inline void IRAM_ATTR
esp32_spiflash_opstart(void);
inline void IRAM_ATTR
esp32_spiflash_opdone(void);

static bool IRAM_ATTR spiflash_pagecached(uint32_t phypage);
static void IRAM_ATTR spiflash_flushmapped(size_t start, size_t size);

/* Flash helpers */

static void IRAM_ATTR esp32_set_read_opt(struct esp32_spiflash_s *priv);
static void IRAM_ATTR esp32_set_write_opt(struct esp32_spiflash_s *priv);
static int  IRAM_ATTR  esp32_read_status(struct esp32_spiflash_s *priv,
                                         uint32_t *status);
static int IRAM_ATTR esp32_wait_idle(struct esp32_spiflash_s *priv);
static int IRAM_ATTR esp32_enable_write(struct esp32_spiflash_s *priv);
static int IRAM_ATTR esp32_erasesector(struct esp32_spiflash_s *priv,
                                       uint32_t addr, uint32_t size);
static int IRAM_ATTR esp32_writedata(struct esp32_spiflash_s *priv,
                                     uint32_t addr,
                                     const uint8_t *buffer, uint32_t size);
static int IRAM_ATTR esp32_readdata(struct esp32_spiflash_s *priv,
                                    uint32_t addr,
                                    uint8_t *buffer, uint32_t size);
static int IRAM_ATTR esp32_readdata_encrypted(struct esp32_spiflash_s *priv,
                                              uint32_t addr,
                                              uint8_t *buffer,
                                              uint32_t size);
static int IRAM_ATTR esp32_writedata_encrypted(struct esp32_spiflash_s *priv,
                                               uint32_t addr,
                                               const uint8_t *buffer,
                                               uint32_t size);
static int esp32_writeblk_encrypted(struct esp32_spiflash_s *priv,
                                    uint32_t offset,
                                    const uint8_t *buffer,
                                    uint32_t nbytes);

#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
static void esp32_spiflash_work(void *p);
#endif

/* MTD driver methods */

static int esp32_erase(struct mtd_dev_s *dev, off_t startblock,
                       size_t nblocks);
static ssize_t esp32_read(struct mtd_dev_s *dev, off_t offset,
                          size_t nbytes, uint8_t *buffer);
static ssize_t esp32_read_decrypt(struct mtd_dev_s *dev,
                                  off_t offset,
                                  size_t nbytes,
                                  uint8_t *buffer);
static ssize_t esp32_bread(struct mtd_dev_s *dev, off_t startblock,
                           size_t nblocks, uint8_t *buffer);
static ssize_t esp32_bread_decrypt(struct mtd_dev_s *dev,
                                   off_t startblock,
                                   size_t nblocks,
                                   uint8_t *buffer);
#ifdef CONFIG_MTD_BYTE_WRITE
static ssize_t esp32_write(struct mtd_dev_s *dev, off_t offset,
                           size_t nbytes, const uint8_t *buffer);
static ssize_t esp32_write_encrypt(FAR struct mtd_dev_s *dev, off_t offset,
                                   size_t nbytes, FAR const uint8_t *buffer);
#endif
static ssize_t esp32_bwrite(struct mtd_dev_s *dev, off_t startblock,
                            size_t nblocks, const uint8_t *buffer);
static ssize_t esp32_bwrite_encrypt(struct mtd_dev_s *dev,
                                    off_t startblock,
                                    size_t nblocks,
                                    const uint8_t *buffer);
static int esp32_ioctl(struct mtd_dev_s *dev, int cmd,
                       unsigned long arg);
static int esp32_ioctl_encrypt(struct mtd_dev_s *dev, int cmd,
                               unsigned long arg);
#ifdef CONFIG_ESP32_SPIRAM
static int esp32_set_mmu_map(int vaddr, int paddr, int num);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct esp32_spiflash_config_s g_esp32_spiflash1_config =
{
  .reg_base = REG_SPI_BASE(1)
};

static struct esp32_spiflash_s g_esp32_spiflash1 =
{
  .mtd =
          {
            .erase  = esp32_erase,
            .bread  = esp32_bread,
            .bwrite = esp32_bwrite,
            .read   = esp32_read,
            .ioctl  = esp32_ioctl,
#ifdef CONFIG_MTD_BYTE_WRITE
            .write  = esp32_write,
#endif
            .name   = "esp32_mainflash"
          },
  .config = &g_esp32_spiflash1_config,
  .chip = &g_rom_flashchip,
  .dummies = g_rom_spiflash_dummy_len_plus
};

static struct esp32_spiflash_s g_esp32_spiflash1_encrypt =
{
  .mtd =
          {
            .erase  = esp32_erase,
            .bread  = esp32_bread_decrypt,
            .bwrite = esp32_bwrite_encrypt,
            .read   = esp32_read_decrypt,
            .ioctl  = esp32_ioctl_encrypt,
#ifdef CONFIG_MTD_BYTE_WRITE
            .write  = esp32_write_encrypt,
#endif
            .name   = "esp32_mainflash_encrypt"
          },
  .config = &g_esp32_spiflash1_config,
  .chip = &g_rom_flashchip,
  .dummies = g_rom_spiflash_dummy_len_plus
};

/* Ensure exclusive access to the driver */

static mutex_t g_lock = NXMUTEX_INITIALIZER;
#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
static struct work_s g_work;
static mutex_t g_work_lock = NXMUTEX_INITIALIZER;
#endif

static volatile bool g_flash_op_can_start = false;
static volatile bool g_flash_op_complete = false;
static volatile bool g_sched_suspended[CONFIG_SMP_NCPUS];
#ifdef CONFIG_SMP
static sem_t g_disable_non_iram_isr_on_core[CONFIG_SMP_NCPUS];
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: spi_set_reg
 *
 * Description:
 *   Set the content of the SPI register at offset
 *
 * Input Parameters:
 *   priv   - Private SPI device structure
 *   offset - Offset to the register of interest
 *   value  - Value to be written
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static inline void spi_set_reg(struct esp32_spiflash_s *priv,
                               int offset, uint32_t value)
{
  putreg32(value, priv->config->reg_base + offset);
}

/****************************************************************************
 * Name: spi_get_reg
 *
 * Description:
 *   Get the content of the SPI register at offset
 *
 * Input Parameters:
 *   priv   - Private SPI device structure
 *   offset - Offset to the register of interest
 *
 * Returned Value:
 *   The content of the register
 *
 ****************************************************************************/

static inline uint32_t spi_get_reg(struct esp32_spiflash_s *priv,
                                   int offset)
{
  return getreg32(priv->config->reg_base + offset);
}

/****************************************************************************
 * Name: spi_set_regbits
 *
 * Description:
 *   Set the bits of the SPI register at offset
 *
 * Input Parameters:
 *   priv   - Private SPI device structure
 *   offset - Offset to the register of interest
 *   bits   - Bits to be set
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static inline void IRAM_ATTR spi_set_regbits(struct esp32_spiflash_s *priv,
                                             int offset, uint32_t bits)
{
  uint32_t tmp = getreg32(priv->config->reg_base + offset);

  putreg32(tmp | bits, priv->config->reg_base + offset);
}

/****************************************************************************
 * Name: spi_reset_regbits
 *
 * Description:
 *   Clear the bits of the SPI register at offset
 *
 * Input Parameters:
 *   priv   - Private SPI device structure
 *   offset - Offset to the register of interest
 *   bits   - Bits to be cleared
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static inline void spi_reset_regbits(struct esp32_spiflash_s *priv,
                                     int offset, uint32_t bits)
{
  uint32_t tmp = getreg32(priv->config->reg_base + offset);

  putreg32(tmp & (~bits), priv->config->reg_base + offset);
}

/****************************************************************************
 * Name: esp32_spiflash_opstart
 *
 * Description:
 *   Prepare for an SPIFLASH operartion.
 *
 ****************************************************************************/

void esp32_spiflash_opstart(void)
{
  struct tcb_s *tcb = this_task();
  int saved_priority = tcb->sched_priority;
  int cpu;
#ifdef CONFIG_SMP
  int other_cpu;
#endif
  /* Temporary raise schedule priority */

  nxsched_set_priority(tcb, SCHED_PRIORITY_MAX);

  cpu = this_cpu();
#ifdef CONFIG_SMP
  other_cpu = cpu == 1 ? 0 : 1;
#endif

  DEBUGASSERT(cpu == 0 || cpu == 1);

#ifdef CONFIG_SMP
  DEBUGASSERT(other_cpu == 0 || other_cpu == 1);
  DEBUGASSERT(other_cpu != cpu);
  if (OSINIT_OS_READY())
    {
      g_flash_op_can_start = false;

      nxsem_post(&g_disable_non_iram_isr_on_core[other_cpu]);

      while (!g_flash_op_can_start)
        {
          /* Busy loop and wait for spi_flash_op_block_task to disable cache
           * on the other CPU
           */
        }
    }
#endif

  g_sched_suspended[cpu] = true;

  sched_lock();

  nxsched_set_priority(tcb, saved_priority);

  esp32_irq_noniram_disable();

  spi_disable_cache(cpu);
#ifdef CONFIG_SMP
  spi_disable_cache(other_cpu);
#endif
}

/****************************************************************************
 * Name: esp32_spiflash_opdone
 *
 * Description:
 *   Undo all the steps of opstart.
 *
 ****************************************************************************/

void esp32_spiflash_opdone(void)
{
  const int cpu = this_cpu();
#ifdef CONFIG_SMP
  const int other_cpu = cpu ? 0 : 1;
#endif

  DEBUGASSERT(cpu == 0 || cpu == 1);

#ifdef CONFIG_SMP
  DEBUGASSERT(other_cpu == 0 || other_cpu == 1);
  DEBUGASSERT(other_cpu != cpu);
#endif

  spi_enable_cache(cpu);
#ifdef CONFIG_SMP
  spi_enable_cache(other_cpu);
#endif

  /* Signal to spi_flash_op_block_task that flash operation is complete */

  g_flash_op_complete = true;

  esp32_irq_noniram_enable();

  sched_unlock();

  g_sched_suspended[cpu] = false;

#ifdef CONFIG_SMP
  while (g_sched_suspended[other_cpu])
    {
      /* Busy loop and wait for spi_flash_op_block_task to properly finish
       * and resume scheduler
       */
    }
#endif
}

/****************************************************************************
 * Name: stack_is_psram
 *
 * Description:
 *   Check if current task's stack space is in PSRAM
 *
 * Returned Value:
 *   true if it is in PSRAM or false if not.
 *
 ****************************************************************************/

#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
static inline bool IRAM_ATTR stack_is_psram(void)
{
  void *sp = (void *)up_getsp();

  return esp32_ptr_extram(sp);
}
#endif

/****************************************************************************
 * Name: spiflash_pagecached
 *
 * Description:
 *   Check if the given page is cached.
 *
 ****************************************************************************/

static bool IRAM_ATTR spiflash_pagecached(uint32_t phypage)
{
  int start[2];
  int end[2];
  int i;
  int j;

  /* Data ROM start and end pages */

  start[0] = DROM0_PAGES_START;
  end[0]   = DROM0_PAGES_END;

  /* Instruction RAM start and end pages */

  start[1] = PRO_IRAM0_FIRST_PAGE;
  end[1]   = IROM0_PAGES_END;

  for (i = 0; i < 2; i++)
    {
      for (j = start[i]; j < end[i]; j++)
        {
          if (PRO_MMU_TABLE[j] == phypage)
            {
              return true;
            }
        }
    }

  return false;
}

/****************************************************************************
 * Name: spiflash_flushmapped
 *
 * Description:
 *   Writeback PSRAM data and invalidate the cache if the address is mapped.
 *
 ****************************************************************************/

static void IRAM_ATTR spiflash_flushmapped(size_t start, size_t size)
{
  uint32_t page_start;
  uint32_t addr;
  uint32_t page;

  page_start = MMU_ALIGNDOWN_SIZE(start);
  size += (start - page_start);
  size = MMU_ALIGNUP_SIZE(size);

  for (addr = page_start; addr < page_start + size;
       addr += SPI_FLASH_MMU_PAGE_SIZE)
    {
      page = addr / SPI_FLASH_MMU_PAGE_SIZE;

      if (page >= 256)
        {
          return;
        }

      if (spiflash_pagecached(page))
        {
#ifdef CONFIG_ESP32_SPIRAM
          esp_spiram_writeback_cache();
#endif
          cache_flush(0);
#ifdef CONFIG_SMP
          cache_flush(1);
#endif
        }
    }
}

/****************************************************************************
 * Name: esp32_set_read_opt
 *
 * Description:
 *   Set SPI Flash to be direct read mode. Due to different SPI I/O mode
 *   including DIO, QIO and so on. Different command and communication
 *   timing sequence are needed.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void IRAM_ATTR esp32_set_read_opt(struct esp32_spiflash_s *priv)
{
  uint32_t regval;
  uint32_t ctrl;
  uint32_t mode;
  uint32_t cmd;
  uint32_t cycles = 0;
  uint32_t addrbits = 0;
  uint32_t dummy = 0;

  ctrl = spi_get_reg(priv, SPI_CTRL_OFFSET);
  mode = ctrl & (SPI_FREAD_QIO | SPI_FASTRD_MODE);
  if (mode == (SPI_FREAD_QIO | SPI_FASTRD_MODE))
    {
      cycles = SPI1_R_QIO_DUMMY_CYCLELEN + priv->dummies[1];
      dummy = 1;
      addrbits = SPI1_R_QIO_ADDR_BITSLEN;
      cmd = (0x7 << SPI_USR_COMMAND_BITLEN_S) | 0xeb;
    }
  else if (mode == SPI_FASTRD_MODE)
    {
      if (ctrl & SPI_FREAD_DIO)
        {
          if (priv->dummies[1] == 0)
            {
              addrbits = SPI1_R_DIO_ADDR_BITSLEN;
              cmd = (0x7 << SPI_USR_COMMAND_BITLEN_S) | 0xbb;
            }
          else
            {
              cycles = priv->dummies[1] - 1;
              dummy = 1;
              addrbits = SPI1_R_DIO_ADDR_BITSLEN;
              cmd = 0xbb;
            }
        }
      else
        {
          if (ctrl & SPI_FREAD_QUAD)
            {
              cmd = (0x7 << SPI_USR_COMMAND_BITLEN_S) | 0x6b;
            }
          else if (ctrl & SPI_FREAD_DUAL)
            {
              cmd = (0x7 << SPI_USR_COMMAND_BITLEN_S) | 0x3b;
            }
          else
            {
              cmd = (0x7 << SPI_USR_COMMAND_BITLEN_S) | 0x0b;
            }

          cycles = SPI1_R_FAST_DUMMY_CYCLELEN + priv->dummies[1];
          dummy = 1;
          addrbits = SPI1_R_DIO_ADDR_BITSLEN;
        }
    }
  else
    {
      if (priv->dummies[1] != 0)
        {
          cycles = priv->dummies[1] - 1;
          dummy = 1;
        }

      addrbits = SPI1_R_SIO_ADDR_BITSLEN ;
      cmd = (0x7 << SPI_USR_COMMAND_BITLEN_S) | 0x03;
    }

  regval = spi_get_reg(priv, SPI_USER_OFFSET);
  regval &= ~SPI_USR_MOSI;
  regval = SPI_USR_MISO | SPI_USR_ADDR;
  if (dummy)
    {
      regval |= SPI_USR_DUMMY;
    }
  else
    {
      regval &= ~SPI_USR_DUMMY;
    }

  spi_set_regbits(priv, SPI_USER_OFFSET, regval);

  regval = spi_get_reg(priv, SPI_USER1_OFFSET);
  regval &= ~SPI_USR_DUMMY_CYCLELEN_M;
  regval |= cycles << SPI_USR_DUMMY_CYCLELEN_S;
  regval &= ~SPI_USR_ADDR_BITLEN_M;
  regval |= addrbits << SPI_USR_ADDR_BITLEN_S;
  spi_set_reg(priv, SPI_USER1_OFFSET, regval);

  regval = spi_get_reg(priv, SPI_USER2_OFFSET);
  regval &= ~SPI_USR_COMMAND_VALUE;
  regval |= cmd;
  spi_set_reg(priv, SPI_USER2_OFFSET, regval);
}

/****************************************************************************
 * Name: esp32_set_write_opt
 *
 * Description:
 *   Set SPI Flash to be direct write mode. Due to different SPI I/O mode
 *   including DIO, QIO and so on. Different command and communication
 *   timing sequence are needed.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void IRAM_ATTR esp32_set_write_opt(struct esp32_spiflash_s *priv)
{
  uint32_t addrbits;
  uint32_t regval;

  spi_reset_regbits(priv, SPI_USER_OFFSET, SPI_USR_DUMMY);

  addrbits = ESP_ROM_SPIFLASH_W_SIO_ADDR_BITSLEN;
  regval = spi_get_reg(priv, SPI_USER1_OFFSET);
  regval &= ~SPI_USR_ADDR_BITLEN_M;
  regval |= addrbits << SPI_USR_ADDR_BITLEN_S;
  spi_set_reg(priv, SPI_USER1_OFFSET, regval);
}

/****************************************************************************
 * Name: esp32_read_status
 *
 * Description:
 *   Read SPI Flash status register value.
 *
 * Input Parameters:
 *   spi    - ESP32 SPI Flash chip data
 *   status - status buffer pointer
 *
 * Returned Value:
 *   OK if success or a negative value if fail.
 *
 ****************************************************************************/

static int IRAM_ATTR esp32_read_status(struct esp32_spiflash_s *priv,
                                       uint32_t *status)
{
  esp32_spiflash_chip_t *chip = priv->chip;
  uint32_t regval;
  uint32_t flags;
  bool direct = (priv->dummies[1] == 0);

  do
    {
      if (direct)
        {
          spi_set_reg(priv, SPI_RD_STATUS_OFFSET, 0);
          spi_set_reg(priv, SPI_CMD_OFFSET, SPI_FLASH_RDSR);
          while (spi_get_reg(priv, SPI_CMD_OFFSET) != 0)
            {
              ;
            }

          regval = spi_get_reg(priv, SPI_RD_STATUS_OFFSET);
          regval &= chip->status_mask;
          flags = regval & ESP_ROM_SPIFLASH_BUSY_FLAG;
        }
      else
        {
          if (esp_rom_spiflash_read_user_cmd(&regval, 0x05))
            {
              return -EIO;
            }

          flags = regval & ESP_ROM_SPIFLASH_BUSY_FLAG;
        }
    }
  while (flags == ESP_ROM_SPIFLASH_BUSY_FLAG);

  *status = regval;

  return OK;
}

/****************************************************************************
 * Name: esp32_wait_idle
 *
 * Description:
 *   Wait for SPI Flash to be in an idle state.
 *
 * Input Parameters:
 *   spi - ESP32 SPI Flash chip data
 *
 * Returned Value:
 *   OK if success or a negative value if fail.
 *
 ****************************************************************************/

static int IRAM_ATTR esp32_wait_idle(struct esp32_spiflash_s *priv)
{
  uint32_t status;

  while (spi_get_reg(priv, SPI_EXT2_OFFSET) & SPI_ST)
    {
      ;
    }

  while (getreg32(SPI_EXT2_REG(0)) & SPI_ST)
    {
      ;
    }

  if (esp32_read_status(priv, &status) != OK)
    {
      return -EIO;
    }

  return OK;
}

/****************************************************************************
 * Name: esp32_enable_write
 *
 * Description:
 *   Drive SPI flash entering into write mode.
 *
 * Input Parameters:
 *   spi    - ESP32 SPI Flash chip data
 *
 * Returned Value:
 *   OK if success or a negative value if fail.
 *
 ****************************************************************************/

static int IRAM_ATTR esp32_enable_write(struct esp32_spiflash_s *priv)
{
  uint32_t flags;
  uint32_t regval;

  if (esp32_wait_idle(priv) != OK)
    {
      return -EIO;
    }

  spi_set_reg(priv, SPI_RD_STATUS_OFFSET, 0);
  spi_set_reg(priv, SPI_CMD_OFFSET, SPI_FLASH_WREN);
  while (spi_get_reg(priv, SPI_CMD_OFFSET) != 0)
    {
      ;
    }

  do
    {
      if (esp32_read_status(priv, &regval) != OK)
        {
          return -EIO;
        }

      flags = regval & ESP_ROM_SPIFLASH_WRENABLE_FLAG;
    }
  while (flags != ESP_ROM_SPIFLASH_WRENABLE_FLAG);

  return OK;
}

/****************************************************************************
 * Name: esp32_erasesector
 *
 * Description:
 *   Erase SPI Flash sector at designated address.
 *
 * Input Parameters:
 *   spi    - ESP32 SPI Flash chip data
 *   addr   - erasing address
 *
 * Returned Value:
 *   0 if success or a negative value if fail.
 *
 ****************************************************************************/

static int IRAM_ATTR esp32_erasesector(struct esp32_spiflash_s *priv,
                                       uint32_t addr, uint32_t size)
{
  uint32_t offset;

  esp32_set_write_opt(priv);

  if (esp32_wait_idle(priv) != OK)
    {
      return -EIO;
    }

  for (offset = 0; offset < size; offset += MTD_ERASESIZE(priv))
    {
      esp32_spiflash_opstart();

      if (esp32_enable_write(priv) != OK)
        {
          esp32_spiflash_opdone();
          return -EIO;
        }

      spi_set_reg(priv, SPI_ADDR_OFFSET, (addr + offset) & 0xffffff);
      spi_set_reg(priv, SPI_CMD_OFFSET, SPI_FLASH_SE);
      while (spi_get_reg(priv, SPI_CMD_OFFSET) != 0)
        {
          ;
        }

      if (esp32_wait_idle(priv) != OK)
        {
          esp32_spiflash_opdone();
          return -EIO;
        }

      esp32_spiflash_opdone();
    }

  esp32_spiflash_opstart();
  spiflash_flushmapped(addr, size);
  esp32_spiflash_opdone();

  return 0;
}

/****************************************************************************
 * Name: esp32_writeonce
 *
 * Description:
 *   Write max 32 byte data to SPI Flash at designated address.
 *
 *   ESP32 can write max 32 byte once transmission by hardware.
 *
 * Input Parameters:
 *   spi    - ESP32 SPI Flash chip data
 *   addr   - target address
 *   buffer - data buffer pointer
 *   size   - data number by bytes
 *
 * Returned Value:
 *   0 if success or a negative value if fail.
 *
 ****************************************************************************/

static int IRAM_ATTR esp32_writeonce(struct esp32_spiflash_s *priv,
                                     uint32_t addr,
                                     const uint32_t *buffer,
                                     uint32_t size)
{
  uint32_t regval;
  uint32_t i;

  if (size > SPI_FLASH_WRITE_BUF_SIZE)
    {
      return -EINVAL;
    }

  if (esp32_wait_idle(priv) != OK)
    {
      return -EIO;
    }

  if (esp32_enable_write(priv) != OK)
    {
      return -EIO;
    }

  regval = addr & 0xffffff;
  regval |= size << ESP_ROM_SPIFLASH_BYTES_LEN;
  spi_set_reg(priv, SPI_ADDR_OFFSET, regval);

  for (i = 0; i < (size / 4); i++)
    {
      spi_set_reg(priv, SPI_W0_OFFSET + i * 4, buffer[i]);
    }

  if (size & 0x3)
    {
      memcpy(&regval, &buffer[i], size & 0x3);
      spi_set_reg(priv, SPI_W0_OFFSET + i * 4, regval);
    }

  spi_set_reg(priv, SPI_RD_STATUS_OFFSET, 0);
  spi_set_reg(priv, SPI_CMD_OFFSET, SPI_FLASH_PP);
  while (spi_get_reg(priv, SPI_CMD_OFFSET) != 0)
    {
      ;
    }

  if (esp32_wait_idle(priv) != OK)
    {
      return -EIO;
    }

  return OK;
}

/****************************************************************************
 * Name: esp32_writedata
 *
 * Description:
 *   Write data to SPI Flash at designated address.
 *
 * Input Parameters:
 *   spi    - ESP32 SPI Flash chip data
 *   addr   - target address
 *   buffer - data buffer pointer
 *   size   - data number
 *
 * Returned Value:
 *   0 if success or a negative value if fail.
 *
 ****************************************************************************/

static int IRAM_ATTR esp32_writedata(struct esp32_spiflash_s *priv,
                                     uint32_t addr,
                                     const uint8_t *buffer,
                                     uint32_t size)
{
  int ret;
  uint32_t off = 0;
  uint32_t bytes;
  uint32_t tmp_buf[SPI_FLASH_WRITE_WORDS];

  esp32_set_write_opt(priv);

  while (size > 0)
    {
      bytes = MTD_BLKSIZE(priv) - addr % MTD_BLKSIZE(priv) ;
      if (!bytes)
        {
          bytes = MIN(size, SPI_FLASH_WRITE_BUF_SIZE);
        }
      else
        {
          bytes = MIN(bytes, size);
          bytes = MIN(bytes, SPI_FLASH_WRITE_BUF_SIZE);
        }

      memcpy(tmp_buf, &buffer[off], bytes);

      esp32_spiflash_opstart();
      ret = esp32_writeonce(priv, addr, tmp_buf, bytes);
      esp32_spiflash_opdone();

      if (ret)
        {
          return ret;
        }

      addr += bytes;
      size -= bytes;
      off += bytes;
    }

  esp32_spiflash_opstart();
  spiflash_flushmapped(addr, size);
  esp32_spiflash_opdone();

  return OK;
}

/****************************************************************************
 * Name: esp32_writedata_encrypted
 *
 * Description:
 *   Write plaintext data to SPI Flash at designated address by SPI Flash
 *   hardware encryption, and written data in SPI Flash is ciphertext.
 *
 * Input Parameters:
 *   spi    - ESP32 SPI Flash chip data
 *   addr   - target address
 *   buffer - data buffer pointer
 *   size   - data number
 *
 * Returned Value:
 *   0 if success or a negative value if fail.
 *
 ****************************************************************************/

static int IRAM_ATTR esp32_writedata_encrypted(
  struct esp32_spiflash_s *priv,
  uint32_t addr,
  const uint8_t *buffer,
  uint32_t size)
{
  int i;
  int blocks;
  int ret = OK;
  uint32_t tmp_buf[SPI_FLASH_ENCRYPT_WORDS];

  if (addr % SPI_FLASH_ENCRYPT_UNIT_SIZE)
    {
      ferr("ERROR: address=0x%" PRIx32 " is not %d-byte align\n",
           addr, SPI_FLASH_ENCRYPT_UNIT_SIZE);
      return -EINVAL;
    }

  if (size % SPI_FLASH_ENCRYPT_UNIT_SIZE)
    {
      ferr("ERROR: size=%" PRIu32 " is not %d-byte align\n",
           size, SPI_FLASH_ENCRYPT_UNIT_SIZE);
      return -EINVAL;
    }

  blocks = size / SPI_FLASH_ENCRYPT_UNIT_SIZE;

  for (i = 0; i < blocks; i++)
    {
      memcpy(tmp_buf, buffer, SPI_FLASH_ENCRYPT_UNIT_SIZE);

      esp32_set_write_opt(priv);

      esp32_spiflash_opstart();
      esp_rom_spiflash_write_encrypted_enable();

      ret = esp_rom_spiflash_prepare_encrypted_data(addr, tmp_buf);
      if (ret)
        {
          ferr("ERROR: Failed to prepare encrypted data\n");
          goto exit;
        }

      ret = esp32_writeonce(priv, addr, tmp_buf,
                            SPI_FLASH_ENCRYPT_UNIT_SIZE);
      if (ret)
        {
          ferr("ERROR: Failed to write encrypted data @ 0x%" PRIx32 "\n",
               addr);
          goto exit;
        }

      esp_rom_spiflash_write_encrypted_disable();
      esp32_spiflash_opdone();

      addr += SPI_FLASH_ENCRYPT_UNIT_SIZE;
      buffer += SPI_FLASH_ENCRYPT_UNIT_SIZE;
      size -= SPI_FLASH_ENCRYPT_UNIT_SIZE;
    }

  esp32_spiflash_opstart();
  spiflash_flushmapped(addr, size);
  esp32_spiflash_opdone();

  return 0;

exit:
  esp_rom_spiflash_write_encrypted_disable();
  esp32_spiflash_opdone();

  return ret;
}

/****************************************************************************
 * Name: esp32_readdata
 *
 * Description:
 *   Read max 64 byte data data from SPI Flash at designated address.
 *
 *   ESP32 can read max 64 byte once transmission by hardware.
 *
 * Input Parameters:
 *   spi    - ESP32 SPI Flash chip data
 *   addr   - target address
 *   buffer - data buffer pointer
 *   size   - data number by bytes
 *
 * Returned Value:
 *   OK if success or a negative value if fail.
 *
 ****************************************************************************/

static int IRAM_ATTR esp32_readonce(struct esp32_spiflash_s *priv,
                                    uint32_t addr,
                                    uint32_t *buffer,
                                    uint32_t size)
{
  uint32_t regval;
  uint32_t i;

  if (size > SPI_FLASH_READ_BUF_SIZE)
    {
      return -EINVAL;
    }

  if (esp32_wait_idle(priv) != OK)
    {
      return -EIO;
    }

  regval = ((size << 3) - 1) << SPI_USR_MISO_DBITLEN_S;
  spi_set_reg(priv, SPI_MISO_DLEN_OFFSET, regval);

  regval = addr << 8;
  spi_set_reg(priv, SPI_ADDR_OFFSET, regval);

  spi_set_reg(priv, SPI_RD_STATUS_OFFSET, 0);
  spi_set_reg(priv, SPI_CMD_OFFSET, SPI_USR);
  while (spi_get_reg(priv, SPI_CMD_OFFSET) != 0)
    {
      ;
    }

  for (i = 0; i < (size / 4); i++)
    {
      buffer[i] = spi_get_reg(priv, SPI_W0_OFFSET + i * 4);
    }

  if (size & 0x3)
    {
      regval = spi_get_reg(priv, SPI_W0_OFFSET + i * 4);
      memcpy(&buffer[i], &regval, size & 0x3);
    }

  return OK;
}

/****************************************************************************
 * Name: esp32_readdata
 *
 * Description:
 *   Read data from SPI Flash at designated address.
 *
 * Input Parameters:
 *   spi    - ESP32 SPI Flash chip data
 *   addr   - target address
 *   buffer - data buffer pointer
 *   size   - data number
 *
 * Returned Value:
 *   OK if success or a negative value if fail.
 *
 ****************************************************************************/

static int IRAM_ATTR esp32_readdata(struct esp32_spiflash_s *priv,
                                    uint32_t addr,
                                    uint8_t *buffer,
                                    uint32_t size)
{
  int ret;
  uint32_t off = 0;
  uint32_t bytes;
  uint32_t tmp_buf[SPI_FLASH_READ_WORDS];

  while (size > 0)
    {
      bytes = MIN(size, SPI_FLASH_READ_BUF_SIZE);

      esp32_spiflash_opstart();
      ret = esp32_readonce(priv, addr, tmp_buf, bytes);
      esp32_spiflash_opdone();

      if (ret)
        {
          return ret;
        }

      memcpy(&buffer[off], tmp_buf, bytes);

      addr += bytes;
      size -= bytes;
      off += bytes;
    }

  return OK;
}

/****************************************************************************
 * Name: esp32_mmap
 *
 * Description:
 *   Mapped SPI Flash address to ESP32's address bus, so that software
 *   can read SPI Flash data by reading data from memory access.
 *
 *   If SPI Flash hardware encryption is enable, the read from mapped
 *   address is decrypted.
 *
 * Input Parameters:
 *   spi - ESP32 SPI Flash chip data
 *   req - SPI Flash mapping requesting parameters
 *
 * Returned Value:
 *   0 if success or a negative value if fail.
 *
 ****************************************************************************/

static int IRAM_ATTR esp32_mmap(struct esp32_spiflash_s *priv,
                                struct spiflash_map_req *req)
{
  int ret;
  int i;
  int start_page;
  int flash_page;
  int page_cnt;
  bool flush = false;

  esp32_spiflash_opstart();

  for (start_page = DROM0_PAGES_START;
       start_page < DROM0_PAGES_END;
       ++start_page)
    {
      if (PRO_MMU_TABLE[start_page] == INVALID_MMU_VAL
#ifdef CONFIG_SMP
          && APP_MMU_TABLE[start_page] == INVALID_MMU_VAL
#endif
          )
        {
          break;
        }
    }

  flash_page = MMU_ADDR2PAGE(req->src_addr);
  page_cnt = MMU_BYTES2PAGES(MMU_ADDR2OFF(req->src_addr) + req->size);

  if (start_page + page_cnt < DROM0_PAGES_END)
    {
      for (i = 0; i < page_cnt; i++)
        {
          PRO_MMU_TABLE[start_page + i] = flash_page + i;
#ifdef CONFIG_SMP
          APP_MMU_TABLE[start_page + i] = flash_page + i;
#endif
        }

      req->start_page = start_page;
      req->page_cnt = page_cnt;
      req->ptr = (void *)(VADDR0_START_ADDR +
                          start_page * SPI_FLASH_MMU_PAGE_SIZE +
                          MMU_ADDR2OFF(req->src_addr));
      flush = true;
      ret = 0;
    }
  else
    {
      ret = -ENOBUFS;
    }

  if (flush)
    {
#ifdef CONFIG_ESP32_SPIRAM
      esp_spiram_writeback_cache();
#endif
      cache_flush(0);
#ifdef CONFIG_SMP
      cache_flush(1);
#endif
    }

  esp32_spiflash_opdone();

  return ret;
}

/****************************************************************************
 * Name: esp32_ummap
 *
 * Description:
 *   Unmap SPI Flash address in ESP32's address bus, and free resource.
 *
 * Input Parameters:
 *   spi - ESP32 SPI Flash chip data
 *   req - SPI Flash mapping requesting parameters
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void IRAM_ATTR esp32_ummap(struct esp32_spiflash_s *priv,
                                  const struct spiflash_map_req *req)
{
  int i;

  esp32_spiflash_opstart();

  for (i = req->start_page; i < req->start_page + req->page_cnt; ++i)
    {
      PRO_MMU_TABLE[i] = INVALID_MMU_VAL;
#ifdef CONFIG_SMP
      APP_MMU_TABLE[i] = INVALID_MMU_VAL;
#endif
    }

#ifdef CONFIG_ESP32_SPIRAM
  esp_spiram_writeback_cache();
#endif
  cache_flush(0);
#ifdef CONFIG_SMP
  cache_flush(1);
#endif
  esp32_spiflash_opdone();
}

/****************************************************************************
 * Name: esp32_readdata_encrypted
 *
 * Description:
 *   Read decrypted data from SPI Flash at designated address when
 *   enable SPI Flash hardware encryption.
 *
 * Input Parameters:
 *   spi    - ESP32 SPI Flash chip data
 *   addr   - target address
 *   buffer - data buffer pointer
 *   size   - data number
 *
 * Returned Value:
 *   OK if success or a negative value if fail.
 *
 ****************************************************************************/

static int IRAM_ATTR esp32_readdata_encrypted(
  struct esp32_spiflash_s *priv,
  uint32_t addr,
  uint8_t *buffer,
  uint32_t size)
{
  int ret;
  struct spiflash_map_req req =
    {
      .src_addr = addr,
      .size = size
    };

  ret = esp32_mmap(priv, &req);
  if (ret)
    {
      return ret;
    }

  memcpy(buffer, req.ptr, size);

  esp32_ummap(priv, &req);

  return OK;
}

/****************************************************************************
 * Name: esp32_writeblk_encrypted
 *
 * Description:
 *   Write plaintext block data to SPI Flash at designated address by SPI
 *   Flash hardware encryption, and written data in SPI Flash is ciphertext.
 *
 * Input Parameters:
 *   priv    - ESP32 SPI Flash private data
 *   offset - target address
 *   buffer - data buffer pointer
 *   nbytes - data number
 *
 * Returned Value:
 *   0 if success or a negative value if fail.
 *
 ****************************************************************************/

static int esp32_writeblk_encrypted(struct esp32_spiflash_s *priv,
                                    uint32_t offset,
                                    const uint8_t *buffer,
                                    uint32_t nbytes)
{
  uint8_t *wbuf;
  uint8_t *rbuf;
  off_t addr;
  ssize_t n;
  uint8_t tmp_buf[SPI_FLASH_ENCRYPT_UNIT_SIZE];
  size_t wbytes = 0;
  int ret = 0;

  while (nbytes > 0)
    {
      if ((offset % SPI_FLASH_ENCRYPT_UNIT_SIZE) != 0)
        {
          wbuf = tmp_buf;
          rbuf = tmp_buf;
          addr = offset - SPI_FLASH_ENCRYPT_MIN_SIZE;

          n = SPI_FLASH_ENCRYPT_MIN_SIZE;

          ret = esp32_readdata_encrypted(priv, addr, rbuf, n);
          if (ret < 0)
            {
              ferr("esp32_readdata_encrypted failed ret=%d\n", ret);
              break;
            }

          memcpy(wbuf + n, buffer, n);
        }
      else if ((nbytes % SPI_FLASH_ENCRYPT_UNIT_SIZE) != 0)
        {
          wbuf = tmp_buf;
          if ((offset % SPI_FLASH_ENCRYPT_UNIT_SIZE) != 0)
            {
              rbuf = tmp_buf;
              addr = offset - SPI_FLASH_ENCRYPT_MIN_SIZE;
            }
          else
            {
              rbuf = tmp_buf + SPI_FLASH_ENCRYPT_MIN_SIZE;
              addr = offset;
            }

          n = SPI_FLASH_ENCRYPT_MIN_SIZE;

          ret = esp32_readdata_encrypted(priv, addr, rbuf, n);
          if (ret < 0)
            {
              ferr("esp32_readdata_encrypted failed ret=%d\n", ret);
              break;
            }

          if ((offset % SPI_FLASH_ENCRYPT_UNIT_SIZE) != 0)
            {
              memcpy(wbuf + n, buffer, n);
            }
          else
            {
              memcpy(wbuf, buffer, n);
            }
        }
      else
        {
          n = SPI_FLASH_ENCRYPT_UNIT_SIZE;
          wbuf = (uint8_t *)buffer;
          addr = offset;
        }

      ret = esp32_writedata_encrypted(priv, addr, wbuf,
                                      SPI_FLASH_ENCRYPT_UNIT_SIZE);
      if (ret < 0)
        {
          ferr("esp32_writedata_encrypted failed ret=%d\n", ret);
          break;
        }

      offset += n;
      nbytes -= n;
      buffer += n;
      wbytes += n;
    }

  return wbytes;
}

/****************************************************************************
 * Name: esp32_spiflash_work
 *
 * Description:
 *   Do SPI Flash operation, cache result and send semaphore to wake up
 *   blocked task.
 *
 * Input Parameters:
 *   p - SPI Flash work arguments
 *
 ****************************************************************************/

#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
static void esp32_spiflash_work(void *p)
{
  struct spiflash_work_arg *work_arg = (struct spiflash_work_arg *)p;

  if (work_arg->op_code == SPIFLASH_OP_CODE_WRITE)
    {
      work_arg->ret = esp32_writedata(work_arg->op_arg.priv,
                                      work_arg->op_arg.addr,
                                      work_arg->op_arg.buffer,
                                      work_arg->op_arg.size);
    }
  else if (work_arg->op_code == SPIFLASH_OP_CODE_READ)
    {
      esp32_set_read_opt(work_arg->op_arg.priv);
      work_arg->ret = esp32_readdata(work_arg->op_arg.priv,
                                     work_arg->op_arg.addr,
                                     work_arg->op_arg.buffer,
                                     work_arg->op_arg.size);
    }
  else if (work_arg->op_code == SPIFLASH_OP_CODE_ERASE)
    {
      work_arg->ret = esp32_erasesector(work_arg->op_arg.priv,
                                        work_arg->op_arg.addr,
                                        work_arg->op_arg.size);
    }
#ifdef CONFIG_ESP32_SPIRAM
  else if (work_arg->op_code == SPIFLASH_OP_CODE_SET_BANK)
    {
      work_arg->ret = esp32_set_mmu_map(work_arg->op_arg.addr,
                                        work_arg->op_arg.paddr,
                                        work_arg->op_arg.size);
    }
#endif
  else if (work_arg->op_code == SPIFLASH_OP_CODE_ENCRYPT_READ)
    {
      esp32_set_read_opt(work_arg->op_arg.priv);
      work_arg->ret = esp32_readdata_encrypted(work_arg->op_arg.priv,
                                               work_arg->op_arg.addr,
                                               work_arg->op_arg.buffer,
                                               work_arg->op_arg.size);
    }
  else if (work_arg->op_code == SPIFLASH_OP_CODE_ENCRYPT_WRITE)
    {
      work_arg->ret = esp32_writeblk_encrypted(work_arg->op_arg.priv,
                                               work_arg->op_arg.addr,
                                               work_arg->op_arg.buffer,
                                               work_arg->op_arg.size);
    }
  else
    {
      ferr("ERROR: op_code=%d is not supported\n", work_arg->op_code);
    }

  nxsem_post(&work_arg->sem);
}

/****************************************************************************
 * Name: esp32_async_op
 *
 * Description:
 *   Send operation code and arguments to workqueue so that workqueue do SPI
 *   Flash operation actually.
 *
 * Input Parameters:
 *   p - SPI Flash work arguments
 *
 * Returned Value:
 *   0 if success or a negative value if fail.
 *
 ****************************************************************************/

static int esp32_async_op(enum spiflash_op_code_e opcode,
                          struct esp32_spiflash_s *priv,
                          uint32_t addr,
                          const uint8_t *buffer,
                          uint32_t size,
                          uint32_t paddr)
{
  int ret;
  struct spiflash_work_arg work_arg =
  {
    .op_code = opcode,
    .op_arg =
    {
      .priv = priv,
      .addr = addr,
      .buffer = (uint8_t *)buffer,
      .size = size,
      .paddr = paddr,
    },
    .sem = NXSEM_INITIALIZER(0, 0)
  };

  ret = nxmutex_lock(&g_work_lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = work_queue(LPWORK, &g_work, esp32_spiflash_work, &work_arg, 0);
  if (ret == 0)
    {
      nxsem_wait_uninterruptible(&work_arg.sem);
      ret = work_arg.ret;
    }

  nxmutex_unlock(&g_work_lock);

  return ret;
}
#endif

/****************************************************************************
 * Name: esp32_erase
 *
 * Description:
 *   Erase SPI Flash designated sectors.
 *
 * Input Parameters:
 *   dev        - ESP32 MTD device data
 *   startblock - start block number, it is not equal to SPI Flash's block
 *   nblocks    - blocks number
 *
 * Returned Value:
 *   0 if success or a negative value if fail.
 *
 ****************************************************************************/

static int esp32_erase(struct mtd_dev_s *dev, off_t startblock,
                       size_t nblocks)
{
  int ret;
  struct esp32_spiflash_s *priv = MTD2PRIV(dev);
  uint32_t addr = startblock * MTD_ERASESIZE(priv);
  uint32_t size = nblocks * MTD_ERASESIZE(priv);

  if ((addr >= MTD_SIZE(priv)) || (addr + size > MTD_SIZE(priv)))
    {
      return -EINVAL;
    }

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_erase(%p, %d, %d)\n", dev, startblock, nblocks);
#endif

  ret = nxmutex_lock(&g_lock);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
  if (stack_is_psram())
    {
      ret = esp32_async_op(SPIFLASH_OP_CODE_ERASE, priv, addr, NULL,
                           size, 0);
    }
  else
    {
      ret = esp32_erasesector(priv, addr, size);
    }
#else
  ret = esp32_erasesector(priv, addr, size);
#endif

  nxmutex_unlock(&g_lock);
  if (ret == OK)
    {
      ret = nblocks;
    }

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_erase()=%d\n", ret);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp32_read
 *
 * Description:
 *   Read data from SPI Flash at designated address.
 *
 * Input Parameters:
 *   dev    - ESP32 MTD device data
 *   offset - target address offset
 *   nbytes - data number
 *   buffer - data buffer pointer
 *
 * Returned Value:
 *   Read data bytes if success or a negative value if fail.
 *
 ****************************************************************************/

static ssize_t esp32_read(struct mtd_dev_s *dev, off_t offset,
                          size_t nbytes, uint8_t *buffer)
{
  ssize_t ret;
  struct esp32_spiflash_s *priv = MTD2PRIV(dev);

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_read(%p, 0x%x, %d, %p)\n", dev, offset, nbytes, buffer);
#endif

  /* Acquire the mutex. */

  ret = nxmutex_lock(&g_lock);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
  if (stack_is_psram())
    {
      ret = esp32_async_op(SPIFLASH_OP_CODE_READ, priv,
                           offset, buffer, nbytes, 0);
    }
  else
    {
      esp32_set_read_opt(priv);
      ret = esp32_readdata(priv, offset, buffer, nbytes);
    }
#else
  esp32_set_read_opt(priv);
  ret = esp32_readdata(priv, offset, buffer, nbytes);
#endif

  nxmutex_unlock(&g_lock);
  if (ret == OK)
    {
      ret = nbytes;
    }

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_read()=%d\n", ret);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp32_bread
 *
 * Description:
 *   Read data from designated blocks.
 *
 * Input Parameters:
 *   dev        - ESP32 MTD device data
 *   startblock - start block number, it is not equal to SPI Flash's block
 *   nblocks    - blocks number
 *   buffer     - data buffer pointer
 *
 * Returned Value:
 *   Read block number if success or a negative value if fail.
 *
 ****************************************************************************/

static ssize_t esp32_bread(struct mtd_dev_s *dev, off_t startblock,
                           size_t nblocks, uint8_t *buffer)
{
  int ret;
  struct esp32_spiflash_s *priv = MTD2PRIV(dev);
  uint32_t addr = MTD_BLK2SIZE(priv, startblock);
  uint32_t size = MTD_BLK2SIZE(priv, nblocks);

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_bread(%p, 0x%x, %d, %p)\n",
        dev, startblock, nblocks, buffer);
#endif

  ret = esp32_read(dev, addr, size, buffer);
  if (ret == size)
    {
      ret = nblocks;
    }

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_bread()=%d\n", ret);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp32_read_decrypt
 *
 * Description:
 *   Read encrypted data and decrypt automatically from SPI Flash
 *   at designated address.
 *
 * Input Parameters:
 *   dev    - ESP32 MTD device data
 *   offset - target address offset
 *   nbytes - data number
 *   buffer - data buffer pointer
 *
 * Returned Value:
 *   Read data bytes if success or a negative value if fail.
 *
 ****************************************************************************/

static ssize_t esp32_read_decrypt(struct mtd_dev_s *dev,
                                  off_t offset,
                                  size_t nbytes,
                                  uint8_t *buffer)
{
  ssize_t ret;
  uint8_t *tmpbuff = buffer;
  struct esp32_spiflash_s *priv = MTD2PRIV(dev);

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_read_decrypt(%p, 0x%x, %d, %p)\n",
        dev, offset, nbytes, buffer);
#endif

  /* Acquire the mutex. */

  ret = nxmutex_lock(&g_lock);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
  if (stack_is_psram())
    {
      ret = esp32_async_op(SPIFLASH_OP_CODE_ENCRYPT_READ, priv,
                           offset, buffer, nbytes, 0);
    }
  else
    {
      ret = esp32_readdata_encrypted(priv, offset, tmpbuff, nbytes);
    }
#else
  ret = esp32_readdata_encrypted(priv, offset, tmpbuff, nbytes);
#endif

  nxmutex_unlock(&g_lock);
  if (ret == OK)
    {
      ret = nbytes;
    }

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_read_decrypt()=%d\n", ret);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp32_bread_decrypt
 *
 * Description:
 *   Read encrypted data and decrypt automatically from designated blocks.
 *
 * Input Parameters:
 *   dev        - ESP32 MTD device data
 *   startblock - start block number, it is not equal to SPI Flash's block
 *   nblocks    - blocks number
 *   buffer     - data buffer pointer
 *
 * Returned Value:
 *   Read block number if success or a negative value if fail.
 *
 ****************************************************************************/

static ssize_t esp32_bread_decrypt(struct mtd_dev_s *dev,
                                   off_t startblock,
                                   size_t nblocks,
                                   uint8_t *buffer)
{
  int ret;
  struct esp32_spiflash_s *priv = MTD2PRIV(dev);
  uint32_t addr = MTD_BLK2SIZE(priv, startblock);
  uint32_t size = MTD_BLK2SIZE(priv, nblocks);

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_bread_decrypt(%p, 0x%x, %d, %p)\n",
        dev, startblock, nblocks, buffer);
#endif

  ret = esp32_read_decrypt(dev, addr, size, buffer);
  if (ret == size)
    {
      ret = nblocks;
    }

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_bread_decrypt()=%d\n", ret);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp32_write
 *
 * Description:
 *   write data to SPI Flash at designated address.
 *
 * Input Parameters:
 *   dev    - ESP32 MTD device data
 *   offset - target address offset
 *   nbytes - data number
 *   buffer - data buffer pointer
 *
 * Returned Value:
 *   Written bytes if success or a negative value if fail.
 *
 ****************************************************************************/

#ifdef CONFIG_MTD_BYTE_WRITE
static ssize_t esp32_write(struct mtd_dev_s *dev, off_t offset,
                           size_t nbytes, const uint8_t *buffer)
{
  ssize_t ret;
  struct esp32_spiflash_s *priv = MTD2PRIV(dev);

  ASSERT(buffer);

  if ((offset > MTD_SIZE(priv)) || ((offset + nbytes) > MTD_SIZE(priv)))
    {
      return -EINVAL;
    }

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_write(%p, 0x%x, %d, %p)\n", dev, offset, nbytes, buffer);
#endif

  /* Acquire the mutex. */

  ret = nxmutex_lock(&g_lock);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
  if (stack_is_psram())
    {
      ret = esp32_async_op(SPIFLASH_OP_CODE_WRITE, priv,
                           offset, buffer, nbytes, 0);
    }
  else
    {
      ret = esp32_writedata(priv, offset, buffer, nbytes);
    }
#else
  ret = esp32_writedata(priv, offset, buffer, nbytes);
#endif

  nxmutex_unlock(&g_lock);
  if (ret == OK)
    {
      ret = nbytes;
    }

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_write()=%d\n", ret);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp32_write_encrypt
 *
 * Description:
 *   write data to SPI Flash at designated address by SPI Flash hardware
 *   encryption.
 *
 * Input Parameters:
 *   dev    - ESP32 MTD device data
 *   offset - target address offset
 *   nbytes - data number
 *   buffer - data buffer pointer
 *
 * Returned Value:
 *   Written bytes if success or a negative value if fail.
 *
 ****************************************************************************/

static ssize_t esp32_write_encrypt(FAR struct mtd_dev_s *dev, off_t offset,
                                   size_t nbytes, FAR const uint8_t *buffer)
{
  ssize_t ret;
  struct esp32_spiflash_s *priv = MTD2PRIV(dev);

  ASSERT(buffer);

  if ((offset % SPI_FLASH_ENCRYPT_MIN_SIZE) ||
    (nbytes % SPI_FLASH_ENCRYPT_MIN_SIZE))
    {
      return -EINVAL;
    }

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_write_encrypt(%p, 0x%x, %zu, %p)\n", dev, offset,
        nbytes, buffer);
#endif

  /* Acquire the mutex. */

  ret = nxmutex_lock(&g_lock);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
  if (stack_is_psram())
    {
      ret = esp32_async_op(SPIFLASH_OP_CODE_ENCRYPT_WRITE, priv,
                           offset, buffer, nbytes, 0);
    }
  else
    {
      ret = esp32_writeblk_encrypted(priv, offset, buffer, nbytes);
    }
#else
  ret = esp32_writeblk_encrypted(priv, offset, buffer, nbytes);
#endif

  nxmutex_unlock(&g_lock);

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_write_encrypt()=%d\n", ret);
#endif

  return ret;
}
#endif

/****************************************************************************
 * Name: esp32_bwrite
 *
 * Description:
 *   Write data to designated blocks.
 *
 * Input Parameters:
 *   dev        - ESP32 MTD device data
 *   startblock - start MTD block number,
 *                it is not equal to SPI Flash's block
 *   nblocks    - blocks number
 *   buffer     - data buffer pointer
 *
 * Returned Value:
 *   Written block number if success or a negative value if fail.
 *
 ****************************************************************************/

static ssize_t esp32_bwrite(struct mtd_dev_s *dev, off_t startblock,
                            size_t nblocks, const uint8_t *buffer)
{
  ssize_t ret;
  struct esp32_spiflash_s *priv = MTD2PRIV(dev);
  uint32_t addr = MTD_BLK2SIZE(priv, startblock);
  uint32_t size = MTD_BLK2SIZE(priv, nblocks);

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_bwrite(%p, 0x%x, %d, %p)\n",
        dev, startblock, nblocks, buffer);
#endif

  ret = esp32_write(dev, addr, size, buffer);
  if (ret == size)
    {
      ret = nblocks;
    }

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_bwrite()=%d\n", ret);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp32_bwrite_encrypt
 *
 * Description:
 *   Write data to designated blocks by SPI Flash hardware encryption.
 *
 * Input Parameters:
 *   dev        - ESP32 MTD device data
 *   startblock - start MTD block number,
 *                it is not equal to SPI Flash's block
 *   nblocks    - blocks number
 *   buffer     - data buffer pointer
 *
 * Returned Value:
 *   Written block number if success or a negative value if fail.
 *
 ****************************************************************************/

static ssize_t esp32_bwrite_encrypt(struct mtd_dev_s *dev,
                                    off_t startblock,
                                    size_t nblocks,
                                    const uint8_t *buffer)
{
  ssize_t ret;
  struct esp32_spiflash_s *priv = MTD2PRIV(dev);
  uint32_t offset = MTD_BLK2SIZE(priv, startblock);
  uint32_t nbytes = MTD_BLK2SIZE(priv, nblocks);

  if ((offset % SPI_FLASH_ENCRYPT_MIN_SIZE) ||
      (nbytes % SPI_FLASH_ENCRYPT_MIN_SIZE))
    {
      return -EINVAL;
    }

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_bwrite_encrypt(%p, 0x%x, %d, %p)\n",
        dev, startblock, nblocks, buffer);
#endif

  ret = nxmutex_lock(&g_lock);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
  if (stack_is_psram())
    {
      ret = esp32_async_op(SPIFLASH_OP_CODE_ENCRYPT_WRITE, priv,
                           offset, buffer, nbytes, 0);
    }
  else
    {
      ret = esp32_writeblk_encrypted(priv, offset, buffer, nbytes);
    }
#else
  ret = esp32_writeblk_encrypted(priv, offset, buffer, nbytes);
#endif
  if (ret == nbytes)
    {
      ret = nblocks;
    }

  nxmutex_unlock(&g_lock);
  if (ret == OK)
    {
      ret = nblocks;
    }

#ifdef CONFIG_ESP32_SPIFLASH_DEBUG
  finfo("esp32_bwrite_encrypt()=%d\n", ret);
#endif
  return ret;
}

/****************************************************************************
 * Name: esp32_ioctl
 *
 * Description:
 *   Set/Get option to/from ESP32 SPI Flash MTD device data.
 *
 * Input Parameters:
 *   dev - ESP32 MTD device data
 *   cmd - operation command
 *   arg - operation argument
 *
 * Returned Value:
 *   0 if success or a negative value if fail.
 *
 ****************************************************************************/

static int esp32_ioctl(struct mtd_dev_s *dev, int cmd,
                       unsigned long arg)
{
  int ret = -EINVAL;
  struct esp32_spiflash_s *priv = MTD2PRIV(dev);

  finfo("cmd: %d\n", cmd);

  switch (cmd)
    {
      case MTDIOC_GEOMETRY:
        {
          struct mtd_geometry_s *geo = (struct mtd_geometry_s *)arg;
          if (geo)
            {
              memset(geo, 0, sizeof(*geo));

              geo->blocksize    = MTD_BLKSIZE(priv);
              geo->erasesize    = MTD_ERASESIZE(priv);
              geo->neraseblocks = MTD_SIZE(priv) / MTD_ERASESIZE(priv);
              ret               = OK;

              finfo("blocksize: %" PRIu32 " erasesize: %" PRIu32 ""
                    " neraseblocks: %" PRIu32 "\n",
                    geo->blocksize, geo->erasesize, geo->neraseblocks);
            }
        }
        break;

      case BIOC_PARTINFO:
        {
          struct partition_info_s *info =
            (struct partition_info_s *)arg;
          if (info != NULL)
            {
              info->numsectors  = MTD_SIZE(priv) / MTD_BLKSIZE(priv);
              info->sectorsize  = MTD_BLKSIZE(priv);
              info->startsector = 0;
              info->parent[0]   = '\0';
              ret               = OK;
            }
        }
        break;

      case MTDIOC_ERASESTATE:
        {
          uint8_t *result = (uint8_t *)arg;
          *result = SPI_FLASH_ERASED_STATE;

          ret = OK;
        }
        break;

      default:
        ret = -ENOTTY;
        break;
    }

  finfo("return %d\n", ret);
  return ret;
}

/****************************************************************************
 * Name: esp32_ioctl_encrypt
 *
 * Description:
 *   Set/Get option to/from ESP32 SPI Flash Hardware Encryption MTD
 *   device data.
 *
 * Input Parameters:
 *   dev - ESP32 MTD device data
 *   cmd - operation command
 *   arg - operation argument
 *
 * Returned Value:
 *   0 if success or a negative value if fail.
 *
 ****************************************************************************/

static int esp32_ioctl_encrypt(struct mtd_dev_s *dev, int cmd,
                               unsigned long arg)
{
  int ret = -EINVAL;
  struct esp32_spiflash_s *priv = MTD2PRIV(dev);

  finfo("cmd: %d\n", cmd);

  switch (cmd)
    {
      case MTDIOC_GEOMETRY:
        {
          struct mtd_geometry_s *geo = (struct mtd_geometry_s *)arg;
          if (geo)
            {
              memset(geo, 0, sizeof(*geo));

              geo->blocksize    = SPI_FLASH_ENCRYPT_MIN_SIZE;
              geo->erasesize    = MTD_ERASESIZE(priv);
              geo->neraseblocks = MTD_SIZE(priv) / geo->erasesize;
              ret               = OK;

              finfo("blocksize: %" PRIu32 " erasesize: %" PRIu32 ""
                    " neraseblocks: %" PRIu32 "\n",
                    geo->blocksize, geo->erasesize, geo->neraseblocks);
            }
        }
        break;

      case BIOC_PARTINFO:
        {
          struct partition_info_s *info =
            (struct partition_info_s *)arg;
          if (info != NULL)
            {
              info->sectorsize  = SPI_FLASH_ENCRYPT_MIN_SIZE;
              info->numsectors  = MTD_SIZE(priv) / info->sectorsize;
              info->startsector = 0;
              info->parent[0]   = '\0';
              ret               = OK;
            }
        }
        break;

      case MTDIOC_ERASESTATE:
        {
          uint8_t *result = (uint8_t *)arg;
          *result = SPI_FLASH_ERASED_STATE;

          ret = OK;
        }
        break;

      default:
        ret = -ENOTTY;
        break;
    }

  finfo("return %d\n", ret);
  return ret;
}

#ifdef CONFIG_SMP

/****************************************************************************
 * Name: spi_flash_op_block_task
 *
 * Description:
 *   Disable the non-IRAM interrupts on the other core (the one that isn't
 *   handling the SPI flash operation) and notify that the SPI flash
 *   operation can start. Wait on a busy loop until it's finished and then
 *   re-enable the non-IRAM interrupts.
 *
 * Input Parameters:
 *   argc          - Not used.
 *   argv          - Not used.
 *
 * Returned Value:
 *   Zero (OK) is returned on success. A negated errno value is returned to
 *   indicate the nature of any failure.
 *
 ****************************************************************************/

static int spi_flash_op_block_task(int argc, char *argv[])
{
  struct tcb_s *tcb = this_task();
  int cpu = this_cpu();

  for (; ; )
    {
      DEBUGASSERT((1 << cpu) & tcb->affinity);
      /* Wait for a SPI flash operation to take place and this (the other
       * core) being asked to disable its non-IRAM interrupts.
       */

      nxsem_wait(&g_disable_non_iram_isr_on_core[cpu]);

      sched_lock();

      esp32_irq_noniram_disable();

      /* g_flash_op_complete flag is cleared on *this* CPU, otherwise the
       * other CPU may reset the flag back to false before this task has a
       * chance to check it (if it's preempted by an ISR taking non-trivial
       * amount of time).
       */

      g_flash_op_complete = false;
      g_flash_op_can_start = true;
      while (!g_flash_op_complete)
        {
          /* Busy loop here and wait for the other CPU to finish the SPI
           * flash operation.
           */
        }

      /* Flash operation is complete, re-enable cache */

      spi_enable_cache(cpu);

      /* Restore interrupts that aren't located in IRAM */

      esp32_irq_noniram_enable();

      sched_unlock();
    }

  return OK;
}

/****************************************************************************
 * Name: spiflash_init_spi_flash_op_block_task
 *
 * Description:
 *   Starts a kernel thread that waits for a semaphore indicating that a SPI
 *   flash operation is going to take place in the other CPU. It disables
 *   non-IRAM interrupts, indicates to the other core that the SPI flash
 *   operation can start and waits for it to be finished in a busy loop.
 *
 * Input Parameters:
 *   cpu - The CPU core that will run the created task to wait on a busy
 *         loop while the SPI flash operation finishes
 *
 * Returned Value:
 *   0 (OK) on success; A negated errno value on failure.
 *
 ****************************************************************************/

int spiflash_init_spi_flash_op_block_task(int cpu)
{
  int pid;
  int ret = OK;
  char *argv[2];
  char arg1[32];
  cpu_set_t cpuset;

  snprintf(arg1, sizeof(arg1), "%p", &cpu);
  argv[0] = arg1;
  argv[1] = NULL;

  pid = kthread_create("spiflash_op",
                       SCHED_PRIORITY_MAX,
                       CONFIG_ESP32_SPIFLASH_OP_TASK_STACKSIZE,
                       spi_flash_op_block_task,
                       argv);
  if (pid > 0)
    {
      if (cpu < CONFIG_SMP_NCPUS)
        {
          CPU_ZERO(&cpuset);
          CPU_SET(cpu, &cpuset);
          ret = nxsched_set_affinity(pid, sizeof(cpuset), &cpuset);
          if (ret < 0)
            {
              return ret;
            }
        }
    }
  else
    {
      return -EPERM;
    }

  return ret;
}
#endif /* CONFIG_SMP */

#ifdef CONFIG_ESP32_SPIRAM

/****************************************************************************
 * Name: esp32_set_mmu_map
 *
 * Description:
 *   Set Ext-SRAM-Cache mmu mapping.
 *
 * Input Parameters:
 *   vaddr - Virtual address in CPU address space
 *   paddr - Physical address in Ext-SRAM
 *   num   - Pages to be set
 *
 * Returned Value:
 *   0 if success or a negative value if fail.
 *
 ****************************************************************************/

static int esp32_set_mmu_map(int vaddr, int paddr, int num)
{
  int ret;
  ret = cache_sram_mmu_set(0, 0, vaddr, paddr, 32, num);
  DEBUGASSERT(ret == 0);
  ret = cache_sram_mmu_set(1, 0, vaddr, paddr, 32, num);
  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_ESP32_SPIRAM

/****************************************************************************
 * Name: esp32_set_bank
 *
 * Description:
 *   Set Ext-SRAM-Cache mmu mapping.
 *
 * Input Parameters:
 *   virt_bank - Beginning of the virtual bank
 *   phys_bank - Beginning of the physical bank
 *   ct        - Number of banks
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void esp32_set_bank(int virt_bank, int phys_bank, int ct)
{
  int ret;
  uint32_t vaddr = SOC_EXTRAM_DATA_LOW + CACHE_BLOCKSIZE * virt_bank;
  uint32_t paddr = phys_bank * CACHE_BLOCKSIZE;
#ifdef CONFIG_ESP32_SPI_FLASH_SUPPORT_PSRAM_STACK
  if (stack_is_psram())
    {
      ret = esp32_async_op(SPIFLASH_OP_CODE_SET_BANK, NULL, vaddr, NULL,
                           ct, paddr);
    }
  else
#endif
    {
      ret = esp32_set_mmu_map(vaddr, paddr, ct);
    }

  DEBUGASSERT(ret == 0);
  UNUSED(ret);
}
#endif

/****************************************************************************
 * Name: esp32_spiflash_init
 *
 * Description:
 *   Initialize ESP32 SPI flash driver.
 *
 * Returned Value:
 *   OK if success or a negative value if fail.
 *
 ****************************************************************************/

int esp32_spiflash_init(void)
{
  int cpu;
  int ret = OK;

#ifdef CONFIG_SMP
  sched_lock();

  for (cpu = 0; cpu < CONFIG_SMP_NCPUS; cpu++)
    {
      nxsem_init(&g_disable_non_iram_isr_on_core[cpu], 0, 0);

      ret = spiflash_init_spi_flash_op_block_task(cpu);
      if (ret != OK)
        {
          return ret;
        }
    }

  sched_unlock();
#else
  UNUSED(cpu);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp32_spiflash_alloc_mtdpart
 *
 * Description:
 *   Allocate an MTD partition from the ESP32 SPI Flash.
 *
 * Input Parameters:
 *   mtd_offset - MTD Partition offset from the base address in SPI Flash.
 *   mtd_size   - Size for the MTD partition.
 *   encrypted  - Flag indicating whether the newly allocated partition will
 *                have its content encrypted.
 *
 * Returned Value:
 *   ESP32 SPI Flash MTD data pointer if success or NULL if fail.
 *
 ****************************************************************************/

struct mtd_dev_s *esp32_spiflash_alloc_mtdpart(uint32_t mtd_offset,
                                               uint32_t mtd_size,
                                               bool encrypted)
{
  struct esp32_spiflash_s *priv;
  esp32_spiflash_chip_t *chip;
  struct mtd_dev_s *mtd_part;
  uint32_t blocks;
  uint32_t startblock;
  uint32_t size;

  if (encrypted)
    {
      priv = &g_esp32_spiflash1_encrypt;
    }
  else
    {
      priv = &g_esp32_spiflash1;
    }

  chip = priv->chip;

  finfo("ESP32 SPI Flash information:\n");
  finfo("\tID = 0x%" PRIx32 "\n", chip->device_id);
  finfo("\tStatus mask = %" PRIx32 "\n", chip->status_mask);
  finfo("\tChip size = %" PRIu32 " KB\n", chip->chip_size / 1024);
  finfo("\tPage size = %" PRIu32 " B\n", chip->page_size);
  finfo("\tSector size = %" PRIu32 " KB\n", chip->sector_size / 1024);
  finfo("\tBlock size = %" PRIu32 " KB\n", chip->block_size / 1024);

  ASSERT((mtd_offset + mtd_size) <= chip->chip_size);
  ASSERT((mtd_offset % chip->sector_size) == 0);
  ASSERT((mtd_size % chip->sector_size) == 0);

  if (mtd_size == 0)
    {
      size = chip->chip_size - mtd_offset;
    }
  else
    {
      size = mtd_size;
    }

  finfo("\tMTD offset = 0x%" PRIx32 "\n", mtd_offset);
  finfo("\tMTD size = 0x%" PRIx32 "\n", size);

  startblock = MTD_SIZE2BLK(priv, mtd_offset);
  blocks = MTD_SIZE2BLK(priv, size);

  mtd_part = mtd_partition(&priv->mtd, startblock, blocks);
  if (!mtd_part)
    {
      ferr("ERROR: create MTD partition");
      return NULL;
    }

  return mtd_part;
}

/****************************************************************************
 * Name: esp32_spiflash_get_mtd
 *
 * Description:
 *   Get ESP32 SPI Flash raw MTD.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   ESP32 SPI Flash raw MTD data pointer.
 *
 ****************************************************************************/

struct mtd_dev_s *esp32_spiflash_get_mtd(void)
{
  struct esp32_spiflash_s *priv = &g_esp32_spiflash1;

  return &priv->mtd;
}

/****************************************************************************
 * Name: esp32_spiflash_get_mtd
 *
 * Description:
 *   Get ESP32 SPI Flash encryption raw MTD.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   ESP32 SPI Flash encryption raw MTD data pointer.
 *
 ****************************************************************************/

struct mtd_dev_s *esp32_spiflash_encrypt_get_mtd(void)
{
  struct esp32_spiflash_s *priv = &g_esp32_spiflash1_encrypt;

  return &priv->mtd;
}

/****************************************************************************
 * Name: esp32_flash_encryption_enabled
 *
 * Description:
 *   Check if ESP32 enables SPI Flash encryption.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   True: SPI Flash encryption is enable, False if not.
 *
 ****************************************************************************/

bool esp32_flash_encryption_enabled(void)
{
  bool enabled = false;
  uint32_t regval;
  uint32_t flash_crypt_cnt;

  regval = getreg32(EFUSE_BLK0_RDATA0_REG);
  flash_crypt_cnt = (regval >> EFUSE_RD_FLASH_CRYPT_CNT_S) &
                    EFUSE_RD_FLASH_CRYPT_CNT_V;

  while (flash_crypt_cnt)
    {
      if (flash_crypt_cnt & 1)
        {
          enabled = !enabled;
        }

      flash_crypt_cnt >>= 1;
    }

  return enabled;
}

/****************************************************************************
 * Name: esp32_get_flash_address_mapped_as_text
 *
 * Description:
 *   Get flash address which is currently mapped as text
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   flash address which is currently mapped as text
 *
 ****************************************************************************/

uint32_t esp32_get_flash_address_mapped_as_text(void)
{
  uint32_t i = MMU_ADDR2PAGE((uint32_t)&_stext - SOC_IROM_MASK_LOW)
               + IROM0_PAGES_START;
  return (PRO_MMU_TABLE[i] & DPORT_MMU_ADDRESS_MASK)
         * SPI_FLASH_MMU_PAGE_SIZE;
}

#endif /* CONFIG_ESP32_SPIFLASH */
