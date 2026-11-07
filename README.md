# test_c_tbox: tbox 六大功能样例（一个功能一个可执行程序）

用 CMake 组织的 [tbox](https://github.com/tboox/tbox) 功能样例，对应 README 中的六项功能，每个功能一个可执行程序：

| 目录 | 可执行文件 | 演示功能 |
|---|---|---|
| `samples/stream` | `sample_stream` | 流处理：stream 文件读写（阻塞模式）、static_stream 位流、data 流（内存流） |
| `samples/coroutine` | `sample_coroutine` | 协程：stackful 协程 + scheduler + channel 生产/消费者 |
| `samples/parser` | `sample_parser` | 数据解析：XML 流式读取（类 SAX）、XML DOM、XML writer、JSON 读写、plist 输出 |
| `samples/container` | `sample_container` | 容器与算法：hash_map、vector、stack、queue、排序/二分查找/计数（iterator） |
| `samples/database` | `sample_database` | 数据库：统一 sql 接口（sqlite3，CRUD + statement 绑定参数） |
| `samples/memory` | `sample_memory` | 内存管理：tbox 内存池（default allocator）、内存检测 dump、fixed_pool |

## 工程结构

```
CMakeLists.txt            # cmake 组织：检测 TBOX_ROOT，为每个功能生成一个可执行文件
samples/<feature>/...c    # 六个功能的样例源码
.github/workflows/build.yml
```

## 构建方式

本地不编译 tbox（避免 xmake 本地依赖），按两种方式二选一：

### 1. 直接引用 CI 编好的 tbox（推荐日常开发）

在 GitHub Actions 上把 tbox 编译并安装到 `tbox-install/`（include + lib），本地配置时：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTBOX_ROOT=<tbox-install 路径>
```

### 2. CI 中现场编译 tbox

工作流第一步用 xmake 编译 tbox（官方仅提供 xmake 构建），并安装：

```bash
git clone https://github.com/tboox/tbox.git tbox && cd tbox
xmake f -c -y -m release \
    --small=false --demo=false   \
    --mysql=n --zlib=n --pcre=n --pcre2=n --polarssl=n --mbedtls=n --openssl=n
xmake -y
xmake install -o ../tbox-install tbox
```

然后 CMake 通过 `-DTBOX_ROOT=...` 使用它：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTBOX_ROOT=$PWD/tbox-install
cmake --build build -j
ctest -C Release --output-on-failure
```

## CI 矩阵（OS 版本 + CPU 架构）

`.github/workflows/build.yml` 覆盖：

- 原生运行：ubuntu-22.04/24.04（x64）、ubuntu-22.04/24.04-arm（arm64）、macos-13（x64）、macos-15（arm64）、windows-2022/2025（x64）——构建后运行全部样例（ctest）
- 交叉编译：linux-armv7（gcc-arm-linux-gnueabihf）、linux-riscv64、linux-i386（-m32 multilib）、windows mingw-w64——使用 CMake 交叉工具链，仅构建
- 依赖缓存：xmake 包缓存（actions/cache）

说明：

- Windows 上 tbox 以静态 CRT（/MT）构建，CMake 侧同步设置 `CMAKE_MSVC_RUNTIME_LIBRARY`。
- 交叉架构禁用 database 模块（`--database=n`），`sample_database` 会输出“not supported”并正常退出；原生架构链接 xmake 预编译的 sqlite3。
- macos 上会链接 CoreFoundation/CoreServices 框架（静态 tbox 需求）。
