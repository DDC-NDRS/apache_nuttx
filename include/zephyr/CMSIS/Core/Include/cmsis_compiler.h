/**************************************************************************//**
 * @file     cmsis_compiler.h
 * @brief    CMSIS compiler generic header file
 * @version  V5.1.0
 * @date     09. October 2018
 ******************************************************************************/
 /*
  * Copyright (c) 2009-2018 Arm Limited. All rights reserved.
  *
  * SPDX-License-Identifier: Apache-2.0
  *
  * Licensed under the Apache License, Version 2.0 (the License); you may
  * not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an AS IS BASIS, WITHOUT
  * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */

#ifndef __CMSIS_COMPILER_H
#define __CMSIS_COMPILER_H

#include <stdint.h>

  /*
   * Arm Compiler 4/5
   */
#if   defined ( __CC_ARM )
#include "cmsis_armcc.h"


   /*
    * Arm Compiler 6.6 LTM (armclang)
    */
#elif defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) && (__ARMCC_VERSION < 6100100)
#include "cmsis_armclang_ltm.h"

   /*
  * Arm Compiler above 6.10.1 (armclang)
  */
#elif defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6100100)
#include "cmsis_armclang.h"


   /*
    * GNU Compiler
    */
#elif defined ( __GNUC__ )
#include "cmsis_gcc.h"


   /*
    * IAR Compiler
    */
#elif defined ( __ICCARM__ )
#include <cmsis_iccarm.h>


   /*
    * TI Arm Compiler
    */
#elif defined ( __TI_ARM__ )
#include <cmsis_ccs.h>

#ifndef   __ASM
#define __ASM                                  __asm
#endif
#ifndef   __INLINE
#define __INLINE                               inline
#endif
#ifndef   __STATIC_INLINE
#define __STATIC_INLINE                        static inline
#endif
#ifndef   __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE                   __STATIC_INLINE
#endif
#ifndef   __NO_RETURN
#define __NO_RETURN                            __attribute__((noreturn))
#endif
#ifndef   __USED
#define __USED                                 __attribute__((used))
#endif
#ifndef   __WEAK
#define __WEAK                                 __attribute__((weak))
#endif
#ifndef   __PACKED
#define __PACKED                               __attribute__((packed))
#endif
#ifndef   __PACKED_STRUCT
#define __PACKED_STRUCT                        struct __attribute__((packed))
#endif
#ifndef   __PACKED_UNION
#define __PACKED_UNION                         union __attribute__((packed))
#endif
#ifndef   __UNALIGNED_UINT32        /* deprecated */
struct __attribute__((packed)) T_UINT32 { uint32_t v; };
#define __UNALIGNED_UINT32(x)                  (((struct T_UINT32 *)(x))->v)
#endif
#ifndef   __UNALIGNED_UINT16_WRITE
__PACKED_STRUCT T_UINT16_WRITE{uint16_t v;};
#define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void*)(addr))->v) = (val))
#endif
#ifndef   __UNALIGNED_UINT16_READ
__PACKED_STRUCT T_UINT16_READ{uint16_t v;};
#define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
#endif
#ifndef   __UNALIGNED_UINT32_WRITE
__PACKED_STRUCT T_UINT32_WRITE{uint32_t v;};
#define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
#endif
#ifndef   __UNALIGNED_UINT32_READ
__PACKED_STRUCT T_UINT32_READ{uint32_t v;};
#define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
#endif
#ifndef   __ALIGNED
#define __ALIGNED(x)                           __attribute__((aligned(x)))
#endif
#ifndef   __RESTRICT
#define __RESTRICT                             __restrict
#endif
#ifndef   __COMPILER_BARRIER
#warning No compiler specific solution for __COMPILER_BARRIER.__COMPILER_BARRIER is ignored.
#define __COMPILER_BARRIER()                   (void)0
#endif


/*
 * TASKING Compiler
 */
#elif defined ( __TASKING__ )
   /*
    * The CMSIS functions have been implemented as intrinsics in the compiler.
    * Please use "carm -?i" to get an up to date list of all intrinsics,
    * Including the CMSIS ones.
    */

#ifndef   __ASM
#define __ASM                                  __asm
#endif
#ifndef   __INLINE
#define __INLINE                               inline
#endif
#ifndef   __STATIC_INLINE
#define __STATIC_INLINE                        static inline
#endif
#ifndef   __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE                   __STATIC_INLINE
#endif
#ifndef   __NO_RETURN
#define __NO_RETURN                            __attribute__((noreturn))
#endif
#ifndef   __USED
#define __USED                                 __attribute__((used))
#endif
#ifndef   __WEAK
#define __WEAK                                 __attribute__((weak))
#endif
#ifndef   __PACKED
#define __PACKED                               __packed__
#endif
#ifndef   __PACKED_STRUCT
#define __PACKED_STRUCT                        struct __packed__
#endif
#ifndef   __PACKED_UNION
#define __PACKED_UNION                         union __packed__
#endif
#ifndef   __UNALIGNED_UINT32        /* deprecated */
struct __packed__ T_UINT32 { uint32_t v; };
#define __UNALIGNED_UINT32(x)                  (((struct T_UINT32 *)(x))->v)
#endif
#ifndef   __UNALIGNED_UINT16_WRITE
__PACKED_STRUCT T_UINT16_WRITE{uint16_t v;};
#define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void *)(addr))->v) = (val))
#endif
#ifndef   __UNALIGNED_UINT16_READ
__PACKED_STRUCT T_UINT16_READ{uint16_t v;};
#define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
#endif
#ifndef   __UNALIGNED_UINT32_WRITE
__PACKED_STRUCT T_UINT32_WRITE{uint32_t v;};
#define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
#endif
#ifndef   __UNALIGNED_UINT32_READ
__PACKED_STRUCT T_UINT32_READ{uint32_t v;};
#define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
#endif
#ifndef   __ALIGNED
#define __ALIGNED(x)              __align(x)
#endif
#ifndef   __RESTRICT
#warning No compiler specific solution for __RESTRICT.__RESTRICT is ignored.
#define __RESTRICT
#endif
#ifndef   __COMPILER_BARRIER
#warning No compiler specific solution for __COMPILER_BARRIER.__COMPILER_BARRIER is ignored.
#define __COMPILER_BARRIER()                   (void)0
#endif


/*
 * COSMIC Compiler
 */
#elif defined ( __CSMC__ )
#include <cmsis_csm.h>

#ifndef   __ASM
#define __ASM                                  _asm
#endif
#ifndef   __INLINE
#define __INLINE                               inline
#endif
#ifndef   __STATIC_INLINE
#define __STATIC_INLINE                        static inline
#endif
#ifndef   __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE                   __STATIC_INLINE
#endif
#ifndef   __NO_RETURN
   // NO RETURN is automatically detected hence no warning here
#define __NO_RETURN
#endif
#ifndef   __USED
#warning No compiler specific solution for __USED.__USED is ignored.
#define __USED
#endif
#ifndef   __WEAK
#define __WEAK                                 __weak
#endif
#ifndef   __PACKED
#define __PACKED                               @packed
#endif
#ifndef   __PACKED_STRUCT
#define __PACKED_STRUCT                        @packed struct
#endif
#ifndef   __PACKED_UNION
#define __PACKED_UNION                         @packed union
#endif
#ifndef   __UNALIGNED_UINT32        /* deprecated */
@packed struct T_UINT32 { uint32_t v; };
#define __UNALIGNED_UINT32(x)                  (((struct T_UINT32 *)(x))->v)
#endif
#ifndef   __UNALIGNED_UINT16_WRITE
__PACKED_STRUCT T_UINT16_WRITE{uint16_t v;};
#define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void *)(addr))->v) = (val))
#endif
#ifndef   __UNALIGNED_UINT16_READ
__PACKED_STRUCT T_UINT16_READ{uint16_t v;};
#define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
#endif
#ifndef   __UNALIGNED_UINT32_WRITE
__PACKED_STRUCT T_UINT32_WRITE{uint32_t v;};
#define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
#endif
#ifndef   __UNALIGNED_UINT32_READ
__PACKED_STRUCT T_UINT32_READ{uint32_t v;};
#define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
#endif
#ifndef   __ALIGNED
#warning No compiler specific solution for __ALIGNED.__ALIGNED is ignored.
#define __ALIGNED(x)
#endif
#ifndef   __RESTRICT
#warning No compiler specific solution for __RESTRICT.__RESTRICT is ignored.
#define __RESTRICT
#endif
#ifndef   __COMPILER_BARRIER
#warning No compiler specific solution for __COMPILER_BARRIER.__COMPILER_BARRIER is ignored.
#define __COMPILER_BARRIER()                   (void)0
#endif


#elif defined ( _MSC_VER ) /* #CUSTOM@NDRS */
#ifndef __STATIC_INLINE
#define __STATIC_INLINE                        static inline
#endif

#ifndef   __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE                   static inline
#endif

#ifndef   __RESTRICT
#define __RESTRICT                             __restrict
#endif

#ifndef   __COMPILER_BARRIER
#define __COMPILER_BARRIER()                   (void)0
#endif

#ifndef   __NO_RETURN
// NO RETURN is automatically detected hence no warning here
#define __NO_RETURN
#endif

// Pushes the current packing alignment value on the internal compiler stack, and sets the current packing alignment value to n (1).
#pragma pack(push, 1)   
#ifndef   __UNALIGNED_UINT16_WRITE
struct T_UINT16_WRITE {
    uint16_t v;
};
#define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE*)(void*)(addr))->v) = (val))
#endif

#ifndef   __UNALIGNED_UINT16_READ
struct T_UINT16_READ {
    uint16_t v;
};
#define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ*)(const void*)(addr))->v)
#endif

#ifndef   __UNALIGNED_UINT32_WRITE
struct T_UINT32_WRITE {
    uint32_t v;
};
#define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE*)(void*)(addr))->v) = (val))
#endif

#ifndef   __UNALIGNED_UINT32_READ
struct T_UINT32_READ {
    uint32_t v;
};
#define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ*)(const void*)(addr))->v)
#endif
#pragma pack(pop)

extern uint32_t __regPriMask;
extern uint32_t __regBasePri;
extern uint32_t __regIPSR;
extern uint32_t __regConTrol;

inline void __disable_irq(void) {
    __regPriMask |= 0x01;
}

inline void __enable_irq(void) {
    __regPriMask &= ~0x01;
}

inline void __WFE(void) {
    /* pass */
}

inline void __DSB(void) {
    /* pass */
}

inline void __ISB(void) {
    /* pass */
}

inline void __NOP(void) {
    /* pass */
}

inline void __DMB(void) {
    /* pass */
}

__STATIC_INLINE uint32_t __get_PRIMASK(void) {
    return (__regPriMask);
}

__STATIC_INLINE uint32_t __get_BASEPRI(void) {
    return (__regBasePri);
}

__STATIC_FORCEINLINE uint32_t __get_IPSR(void) {
    return(__regIPSR);
}

__STATIC_FORCEINLINE uint32_t __get_CONTROL(void) {
    return (__regConTrol);
}

__STATIC_FORCEINLINE void __set_MSP(uint32_t topOfMainStack) {
	/* pass */
}

__STATIC_FORCEINLINE uint32_t __RBIT(uint32_t value) {
    uint32_t result;
    uint32_t s = (4U /*sizeof(v)*/ * 8U) - 1U; /* extra shift needed at end */

    result = value;                      /* r will be reversed bits of v; first get LSB of v */
    for (value >>= 1U; value != 0U; value >>= 1U)
    {
        result <<= 1U;
        result |= value & 1U;
        s--;
    }
    result <<= s;                        /* shift when v's highest bits are zero */
    return result;
}

__STATIC_FORCEINLINE uint8_t __CLZ(uint32_t value) {
    /* Even though __builtin_clz produces a CLZ instruction on ARM, formally
       __builtin_clz(0) is undefined behaviour, so handle this case specially.
       This guarantees ARM-compatible results if happening to compile on a non-ARM
       target, and ensures the compiler doesn't decide to activate any
       optimisations using the logic "value was passed to __builtin_clz, so it
       is non-zero".
       ARM GCC 7.3 and possibly earlier will optimise this test away, leaving a
       single CLZ instruction.
     */
    if (value == 0U) {
        return (32U);
    }

    return ((uint8_t)value);     // @TODO implement cmsis_msc.h
}

#else
#error Unknown compiler.
#endif


#endif /* __CMSIS_COMPILER_H */

