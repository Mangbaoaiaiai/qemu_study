# qemu快速实践入门

## 1. qemu-arm命令和qemu-system-arm命令的区别
1. qemu-arm:
   - 用途: qemu-arm 是一个用户模式仿真器，用于运行为 ARM 架构编译的 Linux 用户模式二进制文件。

   - 模拟范围: 仅模拟用户态（user mode）的ARM架构程序，不能模拟完整的操作系统和硬件外设。

   - 典型使用场景: 主要用于在不同架构的主机上（如 x86）运行 ARM 的应用程序。它不模拟操作系统或硬件，只是用来测试和运行 ARM 架构的应用程序。例如，可以在 x86 主机上运行一个为 ARM 编译的 Linux 应用程序：
   ```qemu-arm ./your_arm_binary```
   - 使用限制: 由于它只仿真用户模式的代码，所以不支持内核态代码或硬件设备的仿真。这意味着你不能使用 qemu-arm 来启动一个完整的操作系统。
2. qemu-system-arm:
   - 用途: qemu-system-arm 是一个系统仿真器，用于仿真整个 ARM 系统，包括 CPU、内存、硬件外设、以及操作系统。

   - 模拟范围: 能够模拟整个计算机系统的环境，包括处理器、内存、I/O 设备、网络接口等，可以仿真 ARM 架构的各种单板计算机或系统。

   - 典型使用场景: 用于仿真完整的 ARM 操作系统（如 Linux 或 Android），你可以使用 qemu-system-arm 启动一个完整的 ARM 虚拟机。常见的使用场景包括嵌入式开发、操作系统内核开发、以及在不同架构上运行 ARM 操作系统。

   - 例如，启动一个完整的 Linux 内核：
    ```qemu-system-arm -M versatilepb -kernel zImage -append "root=/dev/sda1" -hda rootfs.img```
   - 使用特点: qemu-system-arm 提供了丰富的硬件仿真选项，你可以模拟多种 ARM 架构的机器类型，还可以添加虚拟设备（如网络、存储、显示等）。
## 2. U-BOOT
- U-Boot 的作用在嵌入式系统中类似于 BIOS 在传统 PC 上的作用，但它们的设计和使用场景不同。两者都负责系统的初步引导和硬件初始化，都是在操作系统加载之前运行的程序
- 但是在QEMU中 可以绕过 U-Boot，直接启动 Linux 内核。这在模拟环境中是完全可行的，因为 QEMU 自带了一些基本的硬件初始化功能。
>在 QEMU 中，你可以通过命令行直接启动内核，这是因为 QEMU 自带了引导和初始化硬件的功能。
在实际硬件中，你需要一个引导加载程序（如 U-Boot）来完成这些功能，因为硬件无法自行初始化和加载内核。
## 3. qemu-img的用法
- qemu-img [标准选项] 命令 [命令选项]
QEMU 磁盘镜像工具
-h，--help：显示帮助信息并退出
-V，--version：输出版本信息并退出
-T，--trace：[[enable=]<pattern>][,events=<file>][,file=<file>]
指定跟踪选项
- 命令语法：
  amend [--object objectdef] [--image-opts] [-p] [-q] [-f fmt] [-t cache] [--force] -o options filename
bench [-c count] [-d depth] [-f fmt] [--flush-interval=flush_interval] [-i aio] [-n] [--no-drain] [-o offset] [--pattern=pattern] [-q] [-s buffer_size] [-S step_size] [-t cache] [-w] [-U] filename
bitmap (--merge SOURCE | --add | --remove | --clear | --enable | --disable)... [-b source_file [-F source_fmt]] [-g granularity] [--object objectdef] [--image-opts | -f fmt] filename bitmap
check [--object objectdef] [--image-opts] [-q] [-f fmt] [--output=ofmt] [-r [leaks | all]] [-T src_cache] [-U] filename
commit [--object objectdef] [--image-opts] [-q] [-f fmt] [-t cache] [-b base] [-r rate_limit] [-d] [-p] filename
compare [--object objectdef] [--image-opts] [-f fmt] [-F fmt] [-T src_cache] [-p] [-q] [-s] [-U] filename1 filename2
convert [--object objectdef] [--image-opts] [--target-image-opts] [--target-is-zero] [--bitmaps] [-U] [-C] [-c] [-p] [-q] [-n] [-f fmt] [-t cache] [-T src_cache] [-O output_fmt] [-B backing_file [-F backing_fmt]] [-o options] [-l snapshot_param] [-S sparse_size] [-r rate_limit] [-m num_coroutines] [-W] [--salvage] filename [filename2 [...]] output_filename
create [--object objectdef] [-q] [-f fmt] [-b backing_file [-F backing_fmt]] [-u] [-o options] filename [size]
dd [--image-opts] [-U] [-f fmt] [-O output_fmt] [bs=block_size] [count=blocks] [skip=blocks] if=input of=output
info [--object objectdef] [--image-opts] [-f fmt] [--output=ofmt] [--backing-chain] [-U] filename
map [--object objectdef] [--image-opts] [-f fmt] [--start-offset=offset] [--max-length=len] [--output=ofmt] [-U] filename
measure [--output=ofmt] [-O output_fmt] [-o options] [--size N | [--object objectdef] [--image-opts] [-f fmt] [-l snapshot_param] filename]
snapshot [--object objectdef] [--image-opts] [-U] [-q] [-l | -a snapshot | -c snapshot | -d snapshot] filename
rebase [--object objectdef] [--image-opts] [-U] [-q] [-f fmt] [-t cache] [-T src_cache] [-p] [-u] -b backing_file [-F backing_fmt] filename
resize [--object objectdef] [--image-opts] [-f fmt] [--preallocation=prealloc] [-q] [--shrink] filename [+ | -]size
- 命令参数：
   filename 是磁盘镜像文件名
   objectdef 是 QEMU 用户可创建的对象定义。详见 qemu(1) 手册页中的对象属性说明。最常见的对象类型是 secret，用于提供密码和/或加密密钥。
   fmt 是磁盘镜像格式。在大多数情况下，会自动猜测格式
   cache 是用于写入输出磁盘镜像的缓存模式，可用的选项包括：none，writeback（默认，除 convert 外），writethrough，directsync 和 unsafe（convert 默认）
   src_cache 是用于读取输入磁盘镜像的缓存模式，有效选项与 cache 一致
   size 是磁盘镜像的大小（以字节为单位）。支持可选的后缀：k 或 K（千字节，1024），M（兆字节，1024k），G（千兆字节，1024M），T（太字节，1024G），P（拍字节，1024T）和 E（艾字节，1024P）。b 会被忽略
   output_filename 是目标磁盘镜像文件名
   output_fmt 是目标格式
   options 是以 name=value 形式表示的格式特定选项的逗号分隔列表。使用 -o help 查看所使用格式支持的选项概览
   snapshot_param 是用于内部快照的参数，格式为 snapshot.id=[ID],snapshot.name=[NAME]，或 [ID_OR_NAME]
   -c 表示目标镜像必须被压缩（仅限 qcow 格式）
   -u 允许不安全的底层文件链。在重新基准化时，假设旧的和新的底层文件完全匹配。在这种情况下，重新基准化前镜像不需要有效的底层文件（适用于重命名底层文件）。在镜像创建时，允许创建时不尝试打开底层文件
   -h 在有或没有命令的情况下显示帮助并列出支持的格式
   -p 显示命令进度（仅限某些命令）
   -q 使用安静模式——不打印任何输出（除错误外）
   -S 表示连续的字节数（默认为 4k），这些字节必须只包含零，qemu-img 才会在转换过程中创建稀疏镜像。如果字节数为 0，则不会扫描源的未分配或零扇区，目标镜像将始终完全分配
   --output 指定输出的格式（human 或 json）
   -n 跳过目标卷的创建（如果卷是在运行 qemu-img 之前创建的，则有用）