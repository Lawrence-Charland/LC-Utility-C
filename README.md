# lcutilityc

LC Projects 的 C 语言个人工具箱。

说“工具箱”而不是“框架”，是因为这里没有宏大的分层设计，只有一些我写 C 时反复
要用的小东西：内存原语、比较器、排序算法等等，以后还会继续往里放。

需要特别说明：这个项目**不是**“排序库”。建立项目之后我一直在研究排序，所以
现阶段仓库里的代码、文档和验证都围绕排序展开，看起来像专做排序；实际上只是
暂时没开新的研究方向，`lcsorts` 是当前课题，不是项目的边界。

把一件事说清楚：项目名虽然叫 `lcutilityc`，但同名的可执行文件只是一段临时
草稿（`src/main.c`），随时可能被删掉或重写，不算这个项目的交付物。现阶段的
主体是 `include/` 下的头文件和 `liblcsorts` 共享库；`lcutilityc` 目标编不编都
无所谓。

说“试炼场”，是因为新点子和新算法通常先在这里落地——但落地时会按正式库的
规矩来：公开头文件写清接口契约、函数统一用错误码汇报失败、共享库可以安装、
CMake 预置了 debug/release/test 三套构建、改动必须过 `testsorts` 的正确性与
稳定性验证。

换句话说，这是练习项目，但不是随手扔的玩具：API 会尽量保持克制和稳定，方便
直接拿来用；同时也别指望它像 glib 或 STL 那样面面俱到——够用、能读、能改，
才是这个项目的目标。

- 只依赖 C11 标准库（`testsorts` 例外，见下文）。
- 公共头文件：`include/`
- 实现：`src/`
- 版本：0.1.0（`PROJECT_VERSION`，同时决定 `liblcsorts.so` 的 VERSION / SOVERSION）

## 模块一览

这些是已经落地的模块。排序只是目前研究得最深的一块，不是全部：

| 头文件 | 内容 |
| --- | --- |
| `lcdef.h` | `LC_SUCCESS` / `LC_FAILURE` 错误码；`lcmemcpy` / `lcswap` 内存原语；可选的比较/拷贝计数器 |
| `lccmp.h` | `lccmp` 比较器函数指针类型；整数、浮点、指针、字符串比较器，以及对应的 `_desc` 反向版本 |
| `lcsorts.h` | 插入、希尔、归并、归并+插入、堆排序，以及合并两个已排序区间的 `lcmerge` |

### lcsorts

当前阶段的研究重点。下面这些是已经做出来的排序算法，不是这个工具箱的全集。

所有排序函数共享同一签名：

```c
int lcxxx(void *begin, size_t size, size_t element_size, lccmp cmp);
```

`begin` 指向 `size` 个元素，每个元素占 `element_size` 字节；`cmp` 按
`< 0 / 0 / > 0` 返回先后关系。元素一律按原始字节搬移，因此元素类型必须可以
安全地逐字节复制（不能是需要特殊构造/析构的 C++ 对象）。

| 函数 | 算法 | 稳定 | 最坏时间 | 额外内存 |
| --- | --- | --- | --- | --- |
| `lcisort` | 直接插入排序 | 是 | O(n²) | 1 个元素 |
| `lcssort` | 希尔排序（折半增量） | 否 | O(n²)，常见输入下接近亚二次 | 1 个元素 |
| `lcmsort` | 自顶向下归并排序 | 是 | O(n log n) | n 个元素 |
| `lcmisort` | 归并排序 + 插入排序（阈值 32） | 是 | O(n log n) | n 个元素 |
| `lchsort` | 堆排序 | 否 | O(n log n) | 1 个元素 |
| `lcmerge` | 合并两个已排序区间到 `dst` | 是 | O(n) | 不分配 |

几条共同约定：

- `size == 0` 直接返回 `LC_SUCCESS`，不要求 `begin`、`cmp` 非空。
- 参数非法（空指针、`element_size == 0` 等）返回 `LC_FAILURE`；debug 构建里
  还会先触发 `assert`。
- 排序所需临时内存由库 `malloc`，调用者只负责提供数组本身，不需要额外传缓存。
- `lcmerge` 要求 `left`、`right`、`dst` 三者互不重叠，且两个输入区间各自已经
  有序；相等元素先取 `left`，所以结果是稳定的。
- 比较器必须满足严格弱序。`lccmp_f32` / `lccmp_f64` / `lccmp_fld` 把 NaN
  与任何值都视为相等，含 NaN 的数组会破坏这个前提，请事先处理数据。

## 目录结构

```text
lcutilityc/
├── CMakeLists.txt        构建脚本：liblcsorts 库（主）、lcutilityc 草稿、testsorts
├── CMakePresets.json     Ninja + GCC 的 debug / release / test 预设
├── include/
│   ├── lcdef.h           错误码、内存原语、可选计数器
│   ├── lccmp.h           比较器家族
│   └── lcsorts.h         排序与合并接口
└── src/
    ├── lcsorts.c         排序实现
    ├── main.c            临时草稿（目标 lcutilityc，可删）
    └── testsorts.c       正确性/稳定性验证与基准（目标 testsorts）
```

`src/main.c` 编出来的 `lcutilityc` 只是一段临时草稿，用来手工敲两下看看效果；
它不是 API 契约、不是测试套件，也不承诺任何行为，随时可以删改甚至整个删掉。
现阶段的实际产出是 `liblcsorts` 和三个头文件，真正的验证由 `testsorts` 负责。

## 构建

```sh
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
```

Release 同理换 `linux-gcc-release`。预设默认使用 Ninja 和 GCC，构建目录在
`build/<preset-name>`。

只想拿库的话，可以只构建 `lcsorts` 目标：

```sh
cmake --build build/linux-gcc-debug --target lcsorts
```

`lcutilityc` 那个草稿目标编不编都无所谓。

跑验证与基准（`linux-gcc-test` 会定义 `LC_ENABLE_COUNT`，因此额外统计比较和
拷贝次数）：

```sh
cmake --workflow --preset linux-gcc-test
```

或者分开来：

```sh
cmake --preset linux-gcc-test
cmake --build --preset linux-gcc-test
ctest --preset linux-gcc-test
```

## 安装

```sh
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
cmake --install build/linux-gcc-release
```

默认前缀是 `$HOME/.local`，安装后得到 `liblcsorts.so.*` 和 `include/` 下的三个
头文件。

## 快速上手

```c
#include <lccmp.h>
#include <lcsorts.h>

int arr[10] = {1, 0, 2, 9, 3, 8, 4, 7, 5, 6};

lcisort(arr, 10, sizeof(int), lccmp_int);

/* 反向排序 */
lcmsort(arr, 10, sizeof(int), lccmp_int_desc);
```

字符串数组按字典序排序（比较器接收的是“指向字符串指针”的地址）：

```c
const char *names[] = {"pear", "apple", "fig"};
lcmsort(names, 3, sizeof(names[0]), lccmp_str);
```

需要统计一次排序做了多少次比较和拷贝时，在**构建库和使用方时都**定义
`LC_ENABLE_COUNT`。用本项目的 CMake 选项最省事：

```sh
cmake --preset linux-gcc-test      # 这个预设已经打开 LC_ENABLE_COUNT
```

或者手工配置时加 `-DLC_ENABLE_COUNT=ON`。开启后：

```c
#include <lcdef.h>
#include <lcsorts.h>

LC_CNT_RESET();
lcmisort(arr, 10, sizeof(int), lccmp_int);
/* cnt_cmp、cnt_cpy 现在可以直接读 */
```

未定义 `LC_ENABLE_COUNT` 时，计数宏展开成空操作，不产生任何运行时开销。
注意 `cnt_cmp` / `cnt_cpy` 的定义在 `lcsorts.c` 里：如果只在调用方的源文件
里 `#define LC_ENABLE_COUNT`，而库本身没开，就会变成“有声明、没定义”的链接
错误。要么统一走 CMake 选项，要么连库一起重新编译。

## testsorts

`testsorts` 是排序模块的测试与基准程序，**由 DeepSeek V4 Flash 1017 生成**。
它做两件事：

- `verify`：用参考实现 `qsort` 对拍正确性，并检查稳定算法的稳定性；
- `bench`：对指定规模、指定输入分布测速，可选输出比较/拷贝次数。

它**不属于 lcutilityc 的库 API，也不会被安装**，只是一个随项目走的验证工具。

### 它依赖外部的 vector 库

`testsorts` 的基准部分用到了同属 LC Projects 的 `vector`（定长元素动态数组），
仓库地址：<https://github.com/Lawrence-Charland/LC-Vector>。另外 `CMakeLists.txt`
里把 `vector` 的搜索路径写死成了 `$HOME/.local/include` 和 `$HOME/.local/lib`。
所以克隆本仓库后，如果没装过 `vector`，`testsorts` 会编译或链接失败。

解决方式二选一：

1. **先构建并安装 vector**（推荐，顺便也能用上这个库）：

   ```sh
   git clone https://github.com/Lawrence-Charland/LC-Vector.git
   cd LC-Vector
   cmake --preset linux-gcc-release
   cmake --build --preset linux-gcc-release
   cmake --install build/linux-gcc-release
   ```

   如果 `vector` 已经和本仓库放在同一目录下，也可以直接 `cd ../vector`。

   如果 `vector` 装在别的前缀，改一下本仓库 `CMakeLists.txt` 里
   `target_include_directories(testsorts ...)` 和
   `target_link_directories(testsorts ...)` 的路径即可。

2. **不构建 testsorts**：从 `CMakeLists.txt` 里删掉以下内容——

   - `add_executable(testsorts ...)`
   - `set_target_properties(testsorts ...)`
   - `target_include_directories(testsorts ...)`
   - `target_link_directories(testsorts ...)`
   - `target_link_libraries(testsorts ...)`
   - 末尾的 `add_test(NAME testsorts-verify COMMAND testsorts verify)`

   `lcutilityc` 和 `lcsorts` 两个目标不依赖 `vector`，删掉之后照常构建。

### 用法

```sh
# 验证全部算法
./testsorts verify

# 只验证归并和堆排序
./testsorts verify m h

# 基准：1k/10k/100k 个元素，每种规模跑 5 次取最快
./testsorts bench -n 1k,10k,100k -r 5

# 全部输入分布，指定随机种子
./testsorts bench -d all -S 42
```

算法代号：`i`（插入）、`s`（希尔）、`m`（归并）、`mi`（归并+插入）、
`h`（堆）、`q`（标准库 `qsort`，仅作对比基线）、`all`（全部）。完整选项见
`./testsorts help`。

`q` 只是把 C 标准库的 `qsort` 包成统一的 `lcsort` 签名，用来看自家算法和
“官方基准”差多少，不属于 lcutilityc 提供的排序实现。`qsort` 内部怎样搬元素
不经过 `lcmemcpy`，所以 bench 里 `q` 的 `cpy/elem` 一栏显示 `n/a`；比较次数
仍然可统计，因为每次比较都会调用传入的比较器。

## 版本

0.1.0。和 LC Projects 的其它练手项目一样，这里不做显式的语义化版本承诺；
`VERSION 0.1.0` 主要是 CMake 给共享库填 `VERSION` / `SOVERSION` 用的。
