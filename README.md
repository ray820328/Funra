# Funra
- 游戏服务器框架，服务轻量级实现，致力于完成一个强实用性，高可维护的分布式框架系统。
- Light weight backend framework of game.

# 前言
忙忙碌碌拧螺丝，一直在各种项目中徘徊，终于还是下决心写点按自己想法的东西了。
* 关于项目，不写太多服务逻辑进程，仅写框架部分；
* 关于代码实现，尽量简洁，尽量实现成可模块替换；
* 关于IPC，考虑部署的局限性，主要以Socket实现，
优化阶段根据实际需要，封装mmap或共享内存实现，优化同一VM环境下部署的多进程之间的通信效率；
* 关于分布式数据存储，暂不涉及分布式算法实现，间接使用第三方服务实现AP和最终一致性，
服务发现和Meta分布式数据管理采用Consul，业务数据存储采用MongoDB分片副本集集群；
* 关于语言，为了与工作剥离，以c为主，脚本以lua为主，编译脚本以cmake为主，并只做简单流程处理；
* 关于跨平台，因时间有限目前仅准备提供x86-64 & Linux版本（编译环境仅CentOS 7）；
* 开发IDE为VS2017，并附上工程文件在proj目录（直接cmake生成，暂时不做文件和目录抽离以及维护）；
* 可使用build目录内的第三方静态库直接编译，ld版本为： GNU ld version 2.25.1；

因为时间和精力有限，纰漏和错误之处在所难免，希望不吝赐教，再此提前感谢。

# 设计思路
- 针对业务数据特点，服务进程设计为有状态服务器和无状态服务器两种；
- 有状态服务器允许一定时间范围的热数据丢失，如场景服务器崩溃丢失场景当前未存盘的npc状态数据；

# 计划
目前计划分几个阶段，因只有非工作时间可以开发，每阶段计划三个月左右，
阶段不一定严格按顺序执行，存在部分交叉开发，目前计划三年左右时间完成。
* 项目初始化，基本类库，基础脚本整理；
* 网络和rpc实现部分；
* 基础接口和模块实现；
* 集群，分布式和负载均衡部分；
* 尽可能的网络和接口压力测试，可能会延迟维护接口单元测试；
* 一个基于Funra框架的Demo小游戏；
* 性能分析和优化，包括同VM内IPC方案优化等；

大方向基本不变，其余怎么方便怎么来了。

# 关于编码
- 一切命名以简单明了为宗旨，旧文件和第三方库保持不变，新文件尽量遵守以下规则；
- 文件统一utf8不带bom；
- c代码，文件名统一小写，宏和函数名小写 + 下划线分隔，变量名首字母小写驼峰，一般不带变量类型，宏可能带macro前后缀；
- lua代码，文件名和函数都驼峰并大写首字母，变量名驼峰并加类型前缀（n/sz/tb/em），Chunk变量以单下划线'\_'开始；
- 未统一部分在合适的时候可能会做调整；
- 编码质量控制，后期Coverity做静态代码检测，集成测试尽量多跑Valgrind；

# 编译和运行
支持Windows环境和Linux环境，编译后可执行文件位于目录 build/bin 下
## Windows（测试环境为Win 10）
### 打开 VS 工程编译
- 工程位于目录 proj\vs2017 下
- lib位于目录 build\lib\Windows_AMD64 下（可直接使用或重新编译各模块，cmocka建议本地重新编译一次）

### 命令行编译
- 开始菜单打开以下命令行工具
适用于 VS 2017的x64 本级工具命令行
- 执行source目录下cmd文件
build.cmd

## Linux（测试环境为CentOS 7）
- gcc 4.6+
- cmake 3.10+
- 重新编译（脚本位于目录 source 下）
./build.sh build [level] [Debug/Release/All]
- level
level: 2（重新编译全部，第一次编译必须使用此值）
level: 1（rbase + rserver）
level: 0（仅rserver）
- 样例
./build.sh build 2 All

## 目录说明
- build
bin: 生成可执行文件
lib： 生成库文件
- proj
生成工程文件，ide工程，目前仅支持 VS2017
- source
3rd： 第三方库（如果选择VS2017重新编译，注意 工程属性->代码生成->运行库 与引用工程保持一致，如MT或者MD）
rbase： 框架基础库实现
rserver： 服务器整合实现
-tools
工具文件集合
- 其他
test： 每个主目录下包含单元测试用例文件夹

# 关于测试
- 有限的单元和性能测试；
- 单元测试可能会延迟维护；
- 接口单元测试和性能测试可能会混在一起，后期有时间再整理；
- 集成压力测试视后期工具和环境进行；

# 问题和解决
- 因Funra用到多线程和lua脚本，曾经项目中重度lua逻辑，heap内存使用ptmalloc分配，
sbrk高并发环境下导致top chunk激增，内存碎片引起rss不能释放问题，本框架生产环境直接使用jemalloc，后续不做过多解释；
- libunwind如果是直接从git下载的版本（Funra已修改），在某些机器下如果编译失败（亲测1.6.2版本在 x86-64 CentOS 7 下），需要做如下修改，
 - 修改文件：configure.ac
    AM_INIT_AUTOMAKE([1.6 subdir-objects])
    AC_CONFIG_MACRO_DIR([m4])
 - configure带上参数
    autoreconf -i
    ./configure ACLOCAL_AMFLAGS="-I m4"

# 工具
- 考虑多线程和压测，性能分析采用gperftools以侵入式动态采样的方式进行profile，配合QCachegrind查看分析数据；
- 64位服务器环境gperftools需安装新版的libunwind配合进行backtrace，否则实测会产生core；
- 日常开发检测valgrind；
- 压测Locust；

# 后记
留空，希望以后能开心的来写这里。
这就开始...

# Thanks
All codes mentioned.

# LICENSE
Funra project is under the license of MIT. See the LICENSE for details plz.
