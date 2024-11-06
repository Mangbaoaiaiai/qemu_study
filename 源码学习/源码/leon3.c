/*
 * QEMU Leon3 System Emulator
 *
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2010-2024 AdaCore
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h" // 提供与操作系统相关的定义和依赖处理，封装了平台特定的头文件和系统调用，使得QEMU能在不同平台上兼容。
#include "qemu/units.h"//定义了与单位换算相关的常量
#include "qemu/error-report.h"//提供了错误报告机制的接口                
#include "qapi/error.h"// QEMU API（QAPI）中与错误相关的定义，用于处理QAPI接口中的错误信息。
#include "qemu/datadir.h"// 定义了与QEMU数据目录相关的路径和函数，通常用于管理和查找QEMU的资源文件，如BIOS、固件等。
#include "cpu.h"//与CPU仿真相关的头文件，包含了CPU模型、寄存器等的定义和操作。
#include "hw/irq.h"//定义了硬件中断处理接口，用于仿真系统中的中断控制和中断处理逻辑。
#include "qemu/timer.h"// 定义了QEMU中的定时器机制，提供虚拟机内的时间管理功能，支持虚拟机中的计时和延时操作。
#include "hw/ptimer.h"//与QEMU中的物理定时器（ptimer）相关，提供硬件定时器的仿真接口。
#include "hw/qdev-properties.h"//定义了QEMU设备模型的属性，用于设备仿真过程中配置设备参数。
#include "sysemu/sysemu.h"// 与系统仿真（system emulation）相关的接口，提供了系统仿真中管理和控制虚拟机的功能。
#include "sysemu/qtest.h"//提供了QTest接口，QTest是QEMU的一个测试框架，用于测试仿真设备和功能。
#include "sysemu/reset.h"//提供了系统复位（reset）机制的接口，允许在系统仿真中对仿真设备或系统进行复位操作。
#include "hw/boards.h"//定义了硬件板（boards）仿真相关的接口，用于仿真具体硬件平台。
#include "hw/loader.h"// 与加载器（loader）相关的头文件，提供了加载内存、二进制文件（如ELF文件）等仿真系统初始化的功能。
#include "elf.h"//提供ELF（Executable and Linkable Format）文件格式的相关定义和操作，用于处理QEMU中ELF格式的二进制文件。
#include "trace.h"//定义了QEMU中的跟踪机制，用于在仿真过程中记录和追踪系统行为。

#include "hw/timer/grlib_gptimer.h"//定义了GRLIB中的通用定时器（GPTimer），这是LEON系列处理器使用的一种硬件定时器。
#include "hw/char/grlib_uart.h"//提供GRLIB中的UART（通用异步收发传输器）仿真接口，仿真串行通信。
#include "hw/intc/grlib_irqmp.h"//定义GRLIB中的中断控制器（IRQMP），用于处理中断请求和管理中断分发。
#include "hw/misc/grlib_ahb_apb_pnp.h"// 定义GRLIB中的AHB-APB桥接器及PNP（Plug and Play）控制器的仿真，用于处理总线的桥接和设备的自动配置。

/* Default system clock.  */
#define CPU_CLK (40 * 1000 * 1000)          //CPU时钟频率——40MHZ

#define LEON3_PROM_FILENAME "u-boot.bin"    //指定了启动时打开的ROM文件
#define LEON3_PROM_OFFSET    (0x00000000)   //指定了ROM（只读存储器，通常存放固件或启动代码）在系统中的起始地址为 0x00000000，即通常在系统启动时，处理器从这个地址开始执行代码。
#define LEON3_RAM_OFFSET     (0x40000000)   //定义了 RAM（随机访问存储器）在系统中的起始地址为 0x40000000。系统的主要内存操作将会在这个地址空间中进行。

#define MAX_CPUS  4                         //指定了系统支持的最大CPU数为4

#define LEON3_UART_OFFSET  (0x80000100)     //定义了 UART（通用异步收发器）设备的内存映射基地址为 0x80000100。UART 用于串行通信。

/*/UART：是一种异步收发传输器，实现了串行通信协议，用于在计算机和外部设备之间进行数据传输。它以异步方式工作，不需要时钟信号来同步发送和接收的数据。
数据传输：数据通过单一的信号线以串行方式逐位传输，通常包括发送线（TX）和接收线（RX）。

配置：UART 设备通常配置波特率（传输速率）、数据位数（通常为 5-8 位）、停止位（1 或 2 位）和奇偶校验位，以确保数据传输的有效性和正确性。

应用广泛：UART 被广泛用于计算机和外部设备（如传感器、GPS 模块、蓝牙模块等）之间的通信，因为它简单、成本低廉且易于实现。

点对点通信：UART 通常用于点对点的通信，即两个设备之间的直接连接。
*/

#define LEON3_UART_IRQ     (3)              //定义了 UART 设备的中断号为 3，表示 UART 发生事件时，CPU 将收到 3 号中断信号。

#define LEON3_IRQMP_OFFSET (0x80000200)     //定义了 IRQMP（中断控制器）的内存映射基地址为 0x80000200，用于处理多个中断信号的管理。

#define LEON3_TIMER_OFFSET (0x80000300)     //定义了定时器的内存地址映射为0x80000300
#define LEON3_TIMER_IRQ    (6)              //定时器设备的终端号为6，当定时器触发事件时，CPU收到6号终端
#define LEON3_TIMER_COUNT  (2)              //定义了系统拥有两个定时器

#define LEON3_APB_PNP_OFFSET (0x800FF000)   //定义了 APB（高级外设总线）的 PnP（即插即用）控制区域的基地址为 0x800FF000。这个区域可能用于外设的动态配置。
#define LEON3_AHB_PNP_OFFSET (0xFFFFF000)   //定义了 AHB（高级高速总线）的 PnP 控制区域的基地址为 0xFFFFF000，用于管理 AHB 总线上的设备及其配置。

typedef struct ResetData {
    struct CPUResetData {
        int id; //CPU标识符
        SPARCCPU *cpu; //用于指向CPU状态信息的指针
    } info[MAX_CPUS];
    uint32_t entry;             /* save kernel entry in case of reset   保存了复位时系统的入口地址*/
} ResetData;

/* 
SPARC架构指令集：




*/
static uint32_t *gen_store_u32(uint32_t *code, hwaddr addr, uint32_t val)//生成一段在特定地址存储32位值的汇编代码
{
    stl_p(code++, 0x82100000); /* mov %g0, %g1                初始化全局寄存器1*/
    stl_p(code++, 0x84100000); /* mov %g0, %g2                初始化全局寄存器0*/
    stl_p(code++, 0x03000000 +
      extract32(addr, 10, 22));
                               /* sethi %hi(addr), %g1     将addr的高位部分存储到g1（高22位）   */
    stl_p(code++, 0x82106000 +
      extract32(addr, 0, 10));
                               /* or %g1, addr, %g1       将addr的低位部分 与g1寄存器的内容进行OR操作，得到完整的addr的值   */
    stl_p(code++, 0x05000000 +
      extract32(val, 10, 22));
                               /* sethi %hi(val), %g2      将val的高位部分存储到g2（高22位）   */
    stl_p(code++, 0x8410a000 +
      extract32(val, 0, 10));
                               /* or %g2, val, %g2         将val的低位部分与g2寄存器的内容进行OR操作，得到完整的val的值   */
    stl_p(code++, 0xc4204000); /* st %g2, [ %g1 ]          将g2的值存入g1   */

    return code;
}

/*
 * When loading a kernel in RAM the machine is expected to be in a different
 * state (eg: initialized by the bootloader).  This little code reproduces
 * this behavior.  Also this code can be executed by the secondary cpus as
 * well since it looks at the %asr17 register before doing any
 * initialization, it allows to use the same reset address for all the
 * cpus.
 */
 /*这段代码是在描述加载内核到内存时，机器处于不同的状态（例如：已由引导程序初始化）。这段代码再现了这种行为。此外，由于代码会在执行初始化之前检查%asr17寄存器，它同样可以由次级CPU执行，这允许所有CPU使用相同的重置地址。*/

static void write_bootloader(void *ptr, hwaddr kernel_addr)
{
    uint32_t *p = ptr;
    uint32_t *sec_cpu_branch_p = NULL; //记录分支跳转位置

    /* If we are running on a secondary CPU, jump directly to the kernel.  */

    stl_p(p++, 0x85444000); /* rd %asr17, %g2      */ //%asr17是一个辅助状态寄存器
    stl_p(p++, 0x8530a01c); /* srl  %g2, 0x1c, %g2 */
    stl_p(p++, 0x80908000); /* tst  %g2            */
    /* Filled below.  */
    sec_cpu_branch_p = p;
    stl_p(p++, 0x0BADC0DE); /* bne xxx             */
    stl_p(p++, 0x01000000); /* nop */

    /* Initialize the UARTs                                        */
    /* *UART_CONTROL = UART_RECEIVE_ENABLE | UART_TRANSMIT_ENABLE; */
    p = gen_store_u32(p, 0x80000108, 3);

    /* Initialize the TIMER 0                                      */
    /* *GPTIMER_SCALER_RELOAD = 40 - 1;                            */
    p = gen_store_u32(p, 0x80000304, 39);
    /* *GPTIMER0_COUNTER_RELOAD = 0xFFFE;                          */
    p = gen_store_u32(p, 0x80000314, 0xFFFFFFFE);
    /* *GPTIMER0_CONFIG = GPTIMER_ENABLE | GPTIMER_RESTART;        */
    p = gen_store_u32(p, 0x80000318, 3);

    /* Now, the relative branch above can be computed.  */
    stl_p(sec_cpu_branch_p, 0x12800000
          + (p - sec_cpu_branch_p));

    /* JUMP to the entry point                                     */
    stl_p(p++, 0x82100000); /* mov %g0, %g1 */
    stl_p(p++, 0x03000000 + extract32(kernel_addr, 10, 22));
                            /* sethi %hi(kernel_addr), %g1 */
    stl_p(p++, 0x82106000 + extract32(kernel_addr, 0, 10));
                            /* or kernel_addr, %g1 */
    stl_p(p++, 0x81c04000); /* jmp  %g1 */
    stl_p(p++, 0x01000000); /* nop */
}

static void leon3_cpu_reset(void *opaque)
{
    struct CPUResetData *info = (struct CPUResetData *) opaque;
    int id = info->id;
    ResetData *s = container_of(info, ResetData, info[id]);
    CPUState *cpu = CPU(s->info[id].cpu);
    CPUSPARCState *env = cpu_env(cpu);

    cpu_reset(cpu);

    cpu->halted = cpu->cpu_index != 0;
    env->pc = s->entry;
    env->npc = s->entry + 4;
}

static void leon3_cache_control_int(CPUSPARCState *env)
{
    uint32_t state = 0;

    if (env->cache_control & CACHE_CTRL_IF) {
        /* Instruction cache state */
        state = env->cache_control & CACHE_STATE_MASK;
        if (state == CACHE_ENABLED) {
            state = CACHE_FROZEN;
            trace_int_helper_icache_freeze();
        }

        env->cache_control &= ~CACHE_STATE_MASK;
        env->cache_control |= state;
    }

    if (env->cache_control & CACHE_CTRL_DF) {
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

static void leon3_irq_ack(CPUSPARCState *env, int intno)
{
    CPUState *cpu = CPU(env_cpu(env));
    grlib_irqmp_ack(env->irq_manager, cpu->cpu_index, intno);
}

/*
 * This device assumes that the incoming 'level' value on the
 * qemu_irq is the interrupt number, not just a simple 0/1 level.
 */
static void leon3_set_pil_in(void *opaque, int n, int level)
{
    DeviceState *cpu = opaque;
    CPUState *cs = CPU(cpu);
    CPUSPARCState *env = cpu_env(cs);
    uint32_t pil_in = level;

    assert(env != NULL);

    env->pil_in = pil_in;

    if (env->pil_in && (env->interrupt_index == 0 ||
                        (env->interrupt_index & ~15) == TT_EXTINT)) {
        unsigned int i;

        for (i = 15; i > 0; i--) {
            if (env->pil_in & (1 << i)) {
                int old_interrupt = env->interrupt_index;

                env->interrupt_index = TT_EXTINT | i;
                if (old_interrupt != env->interrupt_index) {
                    trace_leon3_set_irq(i);
                    cpu_interrupt(cs, CPU_INTERRUPT_HARD);
                }
                break;
            }
        }
    } else if (!env->pil_in && (env->interrupt_index & ~15) == TT_EXTINT) {
        trace_leon3_reset_irq(env->interrupt_index & 15);
        env->interrupt_index = 0;
        cpu_reset_interrupt(cs, CPU_INTERRUPT_HARD);
    }
}

static void leon3_start_cpu_async_work(CPUState *cpu, run_on_cpu_data data)
{
    cpu->halted = 0;
}

static void leon3_start_cpu(void *opaque, int n, int level)
{
    DeviceState *cpu = opaque;
    CPUState *cs = CPU(cpu);

    assert(level == 1);
    async_run_on_cpu(cs, leon3_start_cpu_async_work, RUN_ON_CPU_NULL);
}

static void leon3_irq_manager(CPUSPARCState *env, int intno)
{
    leon3_irq_ack(env, intno);
    leon3_cache_control_int(env);
}

static void leon3_generic_hw_init(MachineState *machine)
{
    ram_addr_t ram_size = machine->ram_size;
    const char *bios_name = machine->firmware ?: LEON3_PROM_FILENAME;
    const char *kernel_filename = machine->kernel_filename;
    SPARCCPU *cpu;
    CPUSPARCState   *env;
    MemoryRegion *address_space_mem = get_system_memory();
    MemoryRegion *prom = g_new(MemoryRegion, 1);
    int         ret;
    char       *filename;
    int         bios_size;
    int         prom_size;
    ResetData  *reset_info;
    DeviceState *dev, *irqmpdev;
    int i;
    AHBPnp *ahb_pnp;
    APBPnp *apb_pnp;

    reset_info = g_malloc0(sizeof(ResetData));

    for (i = 0; i < machine->smp.cpus; i++) {
        /* Init CPU */
        cpu = SPARC_CPU(object_new(machine->cpu_type));
        qdev_init_gpio_in_named(DEVICE(cpu), leon3_start_cpu, "start_cpu", 1);
        qdev_init_gpio_in_named(DEVICE(cpu), leon3_set_pil_in, "pil", 1);
        qdev_realize(DEVICE(cpu), NULL, &error_fatal);
        env = &cpu->env;

        cpu_sparc_set_id(env, i);

        /* Reset data */
        reset_info->info[i].id = i;
        reset_info->info[i].cpu = cpu;
        qemu_register_reset(leon3_cpu_reset, &reset_info->info[i]);
    }

    ahb_pnp = GRLIB_AHB_PNP(qdev_new(TYPE_GRLIB_AHB_PNP));
    sysbus_realize_and_unref(SYS_BUS_DEVICE(ahb_pnp), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(ahb_pnp), 0, LEON3_AHB_PNP_OFFSET);
    grlib_ahb_pnp_add_entry(ahb_pnp, 0, 0, GRLIB_VENDOR_GAISLER,
                            GRLIB_LEON3_DEV, GRLIB_AHB_MASTER,
                            GRLIB_CPU_AREA);

    apb_pnp = GRLIB_APB_PNP(qdev_new(TYPE_GRLIB_APB_PNP));
    sysbus_realize_and_unref(SYS_BUS_DEVICE(apb_pnp), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(apb_pnp), 0, LEON3_APB_PNP_OFFSET);
    grlib_ahb_pnp_add_entry(ahb_pnp, LEON3_APB_PNP_OFFSET, 0xFFF,
                            GRLIB_VENDOR_GAISLER, GRLIB_APBMST_DEV,
                            GRLIB_AHB_SLAVE, GRLIB_AHBMEM_AREA);

    /* Allocate IRQ manager */
    irqmpdev = qdev_new(TYPE_GRLIB_IRQMP);
    object_property_set_int(OBJECT(irqmpdev), "ncpus", machine->smp.cpus,
                            &error_fatal);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(irqmpdev), &error_fatal);

    for (i = 0; i < machine->smp.cpus; i++) {
        cpu = reset_info->info[i].cpu;
        env = &cpu->env;
        qdev_connect_gpio_out_named(irqmpdev, "grlib-start-cpu", i,
                                    qdev_get_gpio_in_named(DEVICE(cpu),
                                                           "start_cpu", 0));
        qdev_connect_gpio_out_named(irqmpdev, "grlib-irq", i,
                                    qdev_get_gpio_in_named(DEVICE(cpu),
                                                           "pil", 0));
        env->irq_manager = irqmpdev;
        env->qemu_irq_ack = leon3_irq_manager;
    }

    sysbus_mmio_map(SYS_BUS_DEVICE(irqmpdev), 0, LEON3_IRQMP_OFFSET);
    grlib_apb_pnp_add_entry(apb_pnp, LEON3_IRQMP_OFFSET, 0xFFF,
                            GRLIB_VENDOR_GAISLER, GRLIB_IRQMP_DEV,
                            2, 0, GRLIB_APBIO_AREA);

    /* Allocate RAM */
    if (ram_size > 1 * GiB) {
        error_report("Too much memory for this machine: %" PRId64 "MB,"
                     " maximum 1G",
                     ram_size / MiB);
        exit(1);
    }

    memory_region_add_subregion(address_space_mem, LEON3_RAM_OFFSET,
                                machine->ram);

    /* Allocate BIOS */
    prom_size = 8 * MiB;
    memory_region_init_rom(prom, NULL, "Leon3.bios", prom_size, &error_fatal);
    memory_region_add_subregion(address_space_mem, LEON3_PROM_OFFSET, prom);

    /* Load boot prom */
    filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, bios_name);

    if (filename) {
        bios_size = get_image_size(filename);
    } else {
        bios_size = -1;
    }

    if (bios_size > prom_size) {
        error_report("could not load prom '%s': file too big", filename);
        exit(1);
    }

    if (bios_size > 0) {
        ret = load_image_targphys(filename, LEON3_PROM_OFFSET, bios_size);
        if (ret < 0 || ret > prom_size) {
            error_report("could not load prom '%s'", filename);
            exit(1);
        }
    } else if (kernel_filename == NULL && !qtest_enabled()) {
        error_report("Can't read bios image '%s'", filename
                                                   ? filename
                                                   : LEON3_PROM_FILENAME);
        exit(1);
    }
    g_free(filename);

    /* Can directly load an application. */
    if (kernel_filename != NULL) {
        long     kernel_size;
        uint64_t entry;

        kernel_size = load_elf(kernel_filename, NULL, NULL, NULL,
                               &entry, NULL, NULL, NULL,
                               1 /* big endian */, EM_SPARC, 0, 0);
        if (kernel_size < 0) {
            kernel_size = load_uimage(kernel_filename, NULL, &entry,
                                      NULL, NULL, NULL);
        }
        if (kernel_size < 0) {
            error_report("could not load kernel '%s'", kernel_filename);
            exit(1);
        }
        if (bios_size <= 0) {
            /*
             * If there is no bios/monitor just start the application but put
             * the machine in an initialized state through a little
             * bootloader.
             */
            write_bootloader(memory_region_get_ram_ptr(prom), entry);
            reset_info->entry = LEON3_PROM_OFFSET;
            for (i = 0; i < machine->smp.cpus; i++) {
                reset_info->info[i].cpu->env.pc = LEON3_PROM_OFFSET;
                reset_info->info[i].cpu->env.npc = LEON3_PROM_OFFSET + 4;
            }
        }
    }

    /* Allocate timers */
    dev = qdev_new(TYPE_GRLIB_GPTIMER);
    qdev_prop_set_uint32(dev, "nr-timers", LEON3_TIMER_COUNT);
    qdev_prop_set_uint32(dev, "frequency", CPU_CLK);
    qdev_prop_set_uint32(dev, "irq-line", LEON3_TIMER_IRQ);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);

    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, LEON3_TIMER_OFFSET);
    for (i = 0; i < LEON3_TIMER_COUNT; i++) {
        sysbus_connect_irq(SYS_BUS_DEVICE(dev), i,
                           qdev_get_gpio_in(irqmpdev, LEON3_TIMER_IRQ + i));
    }

    grlib_apb_pnp_add_entry(apb_pnp, LEON3_TIMER_OFFSET, 0xFFF,
                            GRLIB_VENDOR_GAISLER, GRLIB_GPTIMER_DEV,
                            0, LEON3_TIMER_IRQ, GRLIB_APBIO_AREA);

    /* Allocate uart */
    dev = qdev_new(TYPE_GRLIB_APB_UART);
    qdev_prop_set_chr(dev, "chrdev", serial_hd(0));
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, LEON3_UART_OFFSET);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0,
                       qdev_get_gpio_in(irqmpdev, LEON3_UART_IRQ));
    grlib_apb_pnp_add_entry(apb_pnp, LEON3_UART_OFFSET, 0xFFF,
                            GRLIB_VENDOR_GAISLER, GRLIB_APBUART_DEV, 1,
                            LEON3_UART_IRQ, GRLIB_APBIO_AREA);
}

static void leon3_generic_machine_init(MachineClass *mc)
{
    mc->desc = "Leon-3 generic";
    mc->init = leon3_generic_hw_init;
    mc->default_cpu_type = SPARC_CPU_TYPE_NAME("LEON3");
    mc->default_ram_id = "leon3.ram";
    mc->max_cpus = MAX_CPUS;
}

DEFINE_MACHINE("leon3_generic", leon3_generic_machine_init)
