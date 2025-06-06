===========
NXP i.MX RT
===========

The i.MX RT series of chips from NXP Semiconductors is based around an ARM Cortex-M7 core running
at 500 MHz, 600 MHz or 1 GHz based on particular MCUs

Supported MCUs
==============

The following list includes MCUs from i.MX RT series and indicates whether they are supported in NuttX

======  =======  ==============  =================
MCU     Support  Core            Frequency
======  =======  ==============  =================
RT500   No       Cortex-M33      200 MHz
RT600   No       Cortex-M33      300 MHz
RT1010  No       Cortex-M7       500 MHz
RT1015  No       Cortex-M7       500 MHz
RT1020  Yes      Cortex-M7       500 MHz
RT1024  No       Cortex-M7       500 MHz
RT1050  Yes      Cortex-M7       600 MHz
RT1060  Yes      Cortex-M7       600 MHz
RT1064  Yes      Cortex-M7       600 MHz
RT1170  No       Cortex-M7 + M4  1 GHz + 400 MHz
======  =======  ==============  =================

Data and Instruction Cache
==========================

MCUs i.MX RT1010 and higher have separated caches for instructions and data. Data cache is initially
set as write-through but can be changed to write-back via Kconfig. While write-back gives better
performance than write-through, it is not supported for all peripherals in NuttX yet. Write-back data
cache can not be selected while running Ethernet or serial port over USB.

Tickless OS
===========

With Tickless OS, the periodic, timer interrupt is eliminated and replaced with a one-shot,
interval timer, that becomes event driven instead of polled. This allows to run the MCU with
higher resolution without using more of the CPU bandwidth processing useless interrupts.

Only tickless via an alarm is currently supported for i.MX RT MCU, which can be selected by
CONFIG_SCHED_TICKLESS_ALARM option. CONFIG_USEC_PER_TICK option determines the resolution
of time reported by :c:func:`clock_systime_ticks()` and the resolution of times that can be set
for certain delays including watchdog timers and delayed work. It is important that value set in
CONFIG_USEC_PER_TICK represents the frequency of GPT timer that runs the tickless mode. Clock
source of the timer is 16.6 MHz, which is then divided by the prescaler value from 1 to 4096.
Possible values for CONFIG_USEC_PER_TICK are 10 or 100 for example.

Peripheral Support
==================

The following list indicates peripherals supported in NuttX:

==========  =======
Peripheral  Support
==========  =======
ACMP        No
ADC         Yes
CAN         Yes
CSI         No
DAC         No
eLCDIF      Yes
ENC         Yes
ENET        Yes
FlexIO      No
GPIO        Yes
I2S         Yes
PWM         Yes
SAI         No
SPDIF       No
SPI         Yes
UART        Yes
USB         Yes
==========  =======

ACMP
----

The circout for comparing two analog input voltages designed to operate across the full
range of supply voltage (rail-to-rail operation).

ADC
---

ADC driver with the successive approximation analog/digital converter. The lower-half of
this driver is initialize by calling :c:func:`imxrt_adcinitialize`.

ADC module can use either continuous trigger (next conversion is started as soon as the
previous is finished) or hardware trigger. This option is selected by IMXRT_ADCx_ETC
(x = 1, 2) config option. If IMXRT_ADCx_ETC = -1 then continuous trigger is used. If
corresponding XBAR number is put in IMXRT_ADCx_ETC then that signal is used to trigger
the ADC conversion (for example PWM signal can be used as a source). For PWM XBAR options
please refer to PWM chapter of this documentation.

Hardware triggering is currently limited to maximum of 8 channels. HW trigger is automatically
disabled if there are more than 8 channels.

DMA is currently not supported for ADC modules.

CAN
---

FlexCAN driver is supported in MCUs i.MX RT1020 and higher. i.MX RT106x have both classical
CAN and also one CAN FD while i.MX RT1170 have 3 CAN FD peripherals. FlexCAN driver in imxrt
works beyond SocketCAN driver layout. The lower-half of this driver is initialize by
calling :c:func:`imxrt_cannitialize()`.

There is an booting option that automatically provides initialization of network interface
in the early stages of booting and therefore calling :c:func:`imxrt_cannitialize()` via
board specific logic is not necessary. This however works only when there is only one
interface in the chip. For running more interfaces (like CAN and Ethernet), network late
initialization must be turn on by CONFIG_NETDEV_LATEINIT and board specific logic must call
lower-half part of drivers.

CSI
---

CMOS Sensor interface which enables the chip to connect directly to external CMOS image sensors.

DAC
---

Digital/analog converter for external signal is only supported in i.MX RT1170 MCU. It is 12 bit
lower power, general purpose DAC.

eLCDIF
------

The enhanced Liquid Crystal Display interface (LCDIF) is a general purpose display controller.

ENC
---

The enhanced quadrature encoder/decoder module supported in i.MX RT1015 and higher. The lower-half
of this driver is initialize by calling :c:func:`imxrt_qeinitialize`.

ENET
----

Ethernet driver supported in i.MX RT1020 and higher. The lower-half of this driver is initialize
by calling :c:func:`imxrt_netnitialize`.

There is an booting option that automatically provides initialization of network interface
in the early stages of booting and therefore calling :c:func:`imxrt_cannitialize()` via
board specific logic is not necessary. This however works only when there is only one
interface in the chip. For running more interfaces (like CAN and Ethernet), network late
initialization must be turn on by CONFIG_NETDEV_LATEINIT and board specific logic must call
lower-half part of drivers.

FlexIO
------

A configurable module providing a range of functionality like emulation of a variety of
serial/parallel communication protocols, flexible 16-bit timers or programmable logic blocks.
This module is supported in i.MX RT1010 and higher.

GPIO
----

Pins can be configured using :c:func:`imxrt_config_gpio` function. Writing to pins is
done by :c:func:`imxrt_gpio_write` function and reading is done by :c:func:`imxrt_gpio_read`.

MCUs i.MX RT1060 and higher includes both standard speed GPIOs (1-5) and high speed
GPIOS (6-9). Regular and high speed GPIO share the same pins (GPIO1 is with GPIO6 etc),
therefore IOMUXC_GPR_GPR26-29 registers are used to determine what module is used for the
GPIO pins.

I2C
---

Inter-Integrated Circout module supporting an interface to an I2C bus as master and/or
as a slave. The lower-half of this driver is initialize by calling :c:func:`imxrt_i2cbus_initialize`.

PWM
---

Pulse width modulator supported in i.MX RT1010 and higher. Multiple channels option is available.
Output on pin B is currently supported only as a complementary option to pin A.
The lower-half of this driver is initialize by calling :c:func:`imxrt_pwminitialize`.

PWM module can be synchronized by an external signal. The external signal used for synchronization
is selected by IMXRT_FLEXPWMx_MODx_SYNC_SRC config option. The number in IMXRT_FLEXPWMx_MODx_SYNC_SRC
corresponds with the XBAR number. Following numbers can be used for synchronization of PWMs with other
PWM module when using iMXRT1020, iMXRT1050 or iMXRT1060.

- PWM1 Module 1 = 40
- PWM1 Module 2 = 41
- PWM1 Module 3 = 42
- PWM1 Module 4 = 43
- PWM2 Module 1 = 44
- PWM2 Module 2 = 45
- PWM2 Module 3 = 46
- PWM2 Module 4 = 47
- PWM3 Module 1 = 48
- PWM3 Module 2 = 49
- PWM3 Module 3 = 50
- PWM3 Module 4 = 51
- PWM4 Module 1 = 52
- PWM4 Module 2 = 53
- PWM4 Module 3 = 54
- PWM4 Module 4 = 55

iMXRT1170 has different XBAR connections:

- PWM1 Module 1 = 74
- PWM1 Module 2 = 75
- PWM1 Module 3 = 76
- PWM1 Module 4 = 77
- PWM2 Module 1 = 78
- PWM2 Module 2 = 79
- PWM2 Module 3 = 80
- PWM2 Module 4 = 81
- PWM3 Module 1 = 82
- PWM3 Module 2 = 83
- PWM3 Module 3 = 84
- PWM3 Module 4 = 85
- PWM4 Module 1 = 86
- PWM4 Module 2 = 87
- PWM4 Module 3 = 88
- PWM4 Module 4 = 89

Option IMXRT_FLEXPWMx_MODx_TRIG allows the module to generate a trigger signal. The trigger is generated on
timer capture of either period or duty cycle value based on the configuration.

SAI
---

Synchronous audio interface provided by I2C module. Supported in i.MX RT1015 and higher.

SPDIF
-----

Sony/Philips digital interface audio block. It is a stereo transceiver that allows the
processor to receive and transmit digital audio. Supported in i.MX RT1010 and higher.

SPI
---

Serial Peripheral interface module that supports an interface to an SPI bus as a master
and/or a slave. The lower-half of this driver is initialize by calling :c:func:`imxrt_lpsibus_initialize`.

UART
----

Universal Asynchronous Receiver/Transmitter module. UART is initialized automatically during
MCU boot.

USB
---

Console communication over USB is supported via CDC-ACM. Only USB Device is currently supported
for i.MX RT in NuttX

Supported Boards
================

.. toctree::
   :glob:
   :maxdepth: 1

   boards/*/*
