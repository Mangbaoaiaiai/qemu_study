 /*
 * QEMU mycpu System Emulator
 */

#include "qemu/osdep.h"         //提供与操作系统相关的定义和依赖处理，封装了平台特定的头文件和系统调用，使得QEMU能在不同平台上兼容。
#include "qemu/units.h"         //定义了与单位换算相关的常量
#include "qemu/error-report.h"  //提供了错误报告机制的接口     
#include "qapi/error.h"         // QEMU API（QAPI）中与错误相关的定义，用于处理QAPI接口中的错误信息。
#include "qemu/datadir.h"       // 定义了与QEMU数据目录相关的路径和函数，通常用于管理和查找QEMU的资源文件，如BIOS、固件等。
#include "cpu.h"                //与CPU仿真相关的头文件，包含了CPU模型、寄存器等的定义和操作。
#include "hw/irq.h"             //定义了硬件中断处理接口，用于仿真系统中的中断控制和中断处理逻辑
#include "qemu/timer.h"         // 定义了QEMU中的定时器机制，提供虚拟机内的时间管理功能，支持虚拟机中的计时和延时操作。
#include "hw/ptimer.h"          //与QEMU中的物理定时器（ptimer）相关，提供硬件定时器的仿真接口。
#include "hw/qdev-properties.h" //定义了QEMU设备模型的属性，用于设备仿真过程中配置设备参数。
#include "sysemu/sysemu.h"      // 与系统仿真（system emulation）相关的接口，提供了系统仿真中管理和控制虚拟机的功能。
#include "sysemu/qtest.h"       //提供了QTest接口，QTest是QEMU的一个测试框架，用于测试仿真设备和功能。
#include "sysemu/reset.h"       //提供了系统复位（reset）机制的接口，允许在系统仿真中对仿真设备或系统进行复位操作。
#include "hw/boards.h"          //定义了硬件板（boards）仿真相关的接口，用于仿真具体硬件平台。
#include "hw/loader.h"          // 与加载器（loader）相关的头文件，提供了加载内存、二进制文件（如ELF文件）等仿真系统初始化的功能。
#include "elf.h"                //提供ELF（Executable and Linkable Format）文件格式的相关定义和操作，用于处理QEMU中ELF格式的二进制文件。
#include "trace.h"              //定义了QEMU中的跟踪机制，用于在仿真过程中记录和追踪系统行为。


#include "hw/timer/grlib_gptimer.h"     //定义了GRLIB中的通用定时器（GPTimer），这是LEON系列处理器使用的一种硬件定时器。
#include "hw/char/grlib_uart.h"         //提供GRLIB中的UART（通用异步收发传输器）仿真接口，仿真串行通信。
#include "hw/intc/grlib_irqmp.h"        //定义GRLIB中的中断控制器（IRQMP），用于处理中断请求和管理中断分发。
#include "hw/misc/grlib_ahb_apb_pnp.h"  // 定义GRLIB中的AHB-APB桥接器及PNP（Plug and Play）控制器的仿真，用于处理总线的桥接和设备的自动配置。'\


/*
什么是GRLIB：GRLIB（General-purpose Reusable Library）是由 Cobham Gaisler 公司开发的一套用于设计和实现 片上系统（SoC） 的 开源 IP 核库。
GRLIB 是一个强大的 IP 核库，特别适合嵌入式系统的设计和仿真。
核心是 LEON 处理器，但它同时提供了丰富的外设和控制模块，用于构建复杂的 SoC 系统。
在 QEMU 中，这些 GRLIB 模块可以用于仿真，帮助开发人员在硬件不可用时进行软件开发和测试。
Leon3是GRLIB中的一种CPU
*/

#define MAX_CPUS  4                         //指定了系统支持的最大CPU数为4 
#define CPU_CLK (40 * 1000 * 1000)




typedef struct ResetDate {
    struct CPUResetData{
        int id;
        SPARCCPU *cpu;
    }info[MAX_CPU];
    uint32_t entry;
}ResetDate; //定义了保存CPU复位信息的结构体

static uint32_t *gen_store_u32(uint32_t *code, hwadddr addr, uint32_t val){
    stl_p(code++, 0x82100000)
}