<<<<<<< HEAD
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

static uint32_t *gen_store_u32(uint32_t *code, hwadddr addr, uint32_t val){//生成一段在特定地址存储32位值的汇编代码
    stl_p(code++, 0x82100000);
    stl_p(code++, 0x84100000);

    /*由于SPARC架构汇编指令不提供直接存取32位值的操作，因此对于32位值采用分布的方法：
    1. 直接取高位22位
    2. 采用或的方式取低位3  2位
    %g1存储addr， %g2存储要存入该地址的数据·    ·1
        NOTE:or对于寄存器的操作是全位的，但是对于立即数的操作是低13位的
    */


    stl_p(code++, 0x03000000 + extract32(addr, 10, 32)); //这里extract32(addr, 10, 22)的作用是返回addr中的高22位，并把它加到0x03000000的末尾，这样就生成了一条汇编指令
    stl_p(code++, 0x82106000 + extract32(addr, 0, 10));
    stl_p(code++, 0x05000000 + extract32(val, 10, 22));
    stl_p(code++, 0x8410a000 + extract32(val, 0, 10));
    stl_p(code++, 0xc4204000);
}

/*当将内核加载到 RAM 时，
 *机器预计会处于不同的状态（例如：由引导加载程序初始化）。
 *这段简短的代码模拟了这种行为。
 *此外，这段代码也可以由其他 CPU 执行，
 *因为它在进行任何初始化之前会检查 %asr17 寄存器，
 *这允许所有 CPU 使用相同的复位地址。
 */

static void write_bootloader(void *ptr, hwaddr kernel_addr){//这段代码的作用是模拟引导加载程序的功能，通过设置一些硬件寄存器和初始化外设，将 CPU 引导到指定的内核地址。
    uint32_t *p = ptr;
    uint32_t *sec_cpu_baranch_p = NULL;
/*代码首先检查当前是否为次级 CPU，
 *通过读取辅助状态寄存器 %asr17 并进行判断。
 *如果是次级 CPU，
 *代码会直接跳转到内核，不执行初始化。
 */
    stl_p(p++, 0x85444000);
    stl_p(p++, 0x8530a01c);
    stl_p(p++, 0x80908000);// 测试 %g2 寄存器是否为 0（次级 CPU 标志）
    sec_cpu_branch_p = p; //这里记录的是占位符的地址，也就是分支指令bne的地址，一会儿会将这个部分的机器码替换
    stl_p(p++, 0x0BADC0DE);// 根据条件跳转（如果不是主 CPU）
    /*0x0BADC0DE 是一种常见的“占位符”值（magic number）
     *在这段代码中，
     *跳转的目标地址（bne xxx 的 xxx 部分）还没有计算出来。
     *通常，跳转地址会在后续代码生成过程中动态计算。
     *在这种情况下，开发者先用一个占位值 0x0BADC0DE 暂时占据位置，
     *等计算出实际跳转距离时再用正确的机器码替换这个值。
     */
    
    
    stl_p(p++, 0x01000000);
/*
 *该部分用于初始化 UART（通用异步接收器/发射器）和定时器
 *这些外设对于嵌入式系统中的 I/O 和计时功能非常重要。
 * 
 */

    p = gen_store_u32(p, 0x80000108, 3);// 初始化 UART 控制寄存器
    p = gen_store_u32(p, 0x80000304, 3);// 初始化 GPTIMER 定时器，设置定时器缩放器
    p = gen_store_u32(p, 0x80000314, 0xFFFFFFFE);// 设置定时器初值
    p = gen_store_u32(p, 0x80000318, 3);// 启动定时器，设置重启功能


    stl_p(sec_cpu_branch_p, 0x12800000 + (p - sec_cpu_branch_p)); // 计算并填充相对跳转地址，也就是前面的条件分支指令

/*最后，代码生成一段跳转指令，将 CPU 跳转到内核的入口地址（kernel_addr）*/
    stl_p(p++, 0x82100000);
    stl_p(p++, 0x03000000 + extract32(kernel_addr, 10, 22));
    stl_p(p++, 0x82106000 + extract32(kernel_addr, 0, 10));
    stl_p(p++, 0x81c04000);
    stl_P(p++, 0x01000000);

}

/*重置 LEON3 CPU 的状态。
 *函数通过设置 CPU 寄存器和状态，
 *初始化执行环境，
 *并根据 CPU 的 ID 决定是否暂停 CPU。
 *
*/
static void leon3_cpu_reset(void *opaque){ //这是一个 CPU 重置函数，opaque 是一个通用的指针，指向包含 CPU 和相关信息的数据结构。它在重置时用于传递所需的上下文信息。
    struct CPUResetData *info = (struct CPUResetData *) opaque;//将 opaque 转换为 CPUResetData 类型的指针。这个结构体存储了 CPU 重置相关的信息。
    int id = info->id;//从 info 中提取 CPU 的 id
    ResetDate *s = container_of(info, ResetDate, info[id]);//通过 container_of 宏将 info 还原为它所属的 ResetData 结构体，info[id] 是其中一个字段。
    CPUState *cpu = CPU(s->info[id].cpu);//通过 id 获取该 CPU 的 CPUState 结构体，这是 QEMU 中的 CPU 状态信息。
    CPUSPARCState *env = cpu_env(cpu);//获取 SPARC 架构的 CPU 环境，即 CPUSPARCState 结构体。这是专门用于 SPARC 架构 CPU 的环境数据结构
    //包含 SPARC 处理器的寄存器和其他状态信息。
    cpu_reset(cpu);//调用 QEMU 的 cpu_reset 函数来重置 cpu 的状态。它会将 CPU 的大部分寄存器和状态恢复到初始状态，类似于设备的硬件复位

    cpu->halted = cpu->cpu_index != 0;//根据 cpu->cpu_index，判断是否暂停 CPU 的执行。
    env->pc = s->entry;//将 PC（程序计数器）设置为 s->entry，即设置 CPU 的初始执行地址。
    env->npc = s->entry + 4;//设置 npc（下一条指令的程序计数器）为 s->entry + 4，即 PC 指向的地址加 4。即，每次都+4

}

/*处理 LEON3 CPU 的缓存控制操作
 *（即 Instruction Cache（指令缓存） 
 *和 Data Cache（数据缓存） 的状态控制）。
 */

static void leon3_cache_control_int(CPUSPARCState *env)
{
    uint32_t state = 0;


/*
 *检查 env->cache_control 的 指令缓存控制位，
 *CACHE_CTRL_IF 是一个宏，
 *代表指令缓存的控制位标志。如果该标志被设置，
 *则进入该块代码。
 */

    if (env->cache_control & CACHE_CTRL_IF) {
        /* Instruction cache state */
        state = env->cache_control & CACHE_STATE_MASK;//CACHE_STATE_MASK 是一个掩码，用于提取缓存状态位。
        if (state == CACHE_ENABLED) {
            state = CACHE_FROZEN;
            trace_int_helper_icache_freeze();
        }

        env->cache_control &= ~CACHE_STATE_MASK;//清除状态位
        env->cache_control |= state;//写入新的缓存状态
    }

    if (env->cache_control & CACHE_CTRL_DF) {//CACHE_CTRL_DF 是用于数据缓存控制的标志位。如果设置了这个标志，则进入该代码块。
        /* Data cache state */
        state = (env->cache_control >> 2) & CACHE_STATE_MASK;
        if (state == CACHE_ENABLED) {
            state = CACHE_FROZEN;
            trace_int_helper_dcache_freeze();
        }

        env->cache_control &= ~(CACHE_STATE_MASK << 2);
        env->cache_control |= (state << 2);
    }
}

static void leon3_irq_ack(CPUSPARCState *env, int intno)//向 GRLIB 中断控制器确认 CPU 已处理完指定中断。
{
    CPUState *cpu = CPU(env_cpu(env));
    grlib_irqmp_ack(env->irq_manager, cpu->cpu_index, intno);
}