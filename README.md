# arch-lab

## lab1

`/lab1` 为实验一相关内容，虽然是与 Pin 相关但是做的时候暂时没实现 CMake Pin，于是独立出来。

## labs

`/labs` 为实验 2~3 相关内容，其中 Pin 工具包将作为项目依赖在 CMake Configure 阶段由 CMake 下载到构建文件夹，请保证配置时对 Github 的网络访问正常。

此外，此项目使用到一个以 CPM CMake 模块提供的模块 [debug-macros](https://github.com/chiro2001/debug-macros)，请 Configure 的时候保证对 Github 的网络访问正常。

`/labs/data/`：程序运行结果和图片等

`/labs/scripts`：解析结果并绘图的脚本

### Usage

`/Makefile`：

1. `make`  / `make submit`：生成提交包
2. `make docs`：编译实验文档

`/labs/Makefile`：

1. `make `/ `make all`

   1. 运行当前实验的测试，默认将会运行 `gcc, astar, zeusmp, tonto, coremark`

   2. 参数 `CACHE_TEST` 指定当前缓存测试类型

   3. 运行前请保证 `runspec` 在环境变量中：

      ```bash
      pushd /opt/spec2006
      source shrc
      popd
      ```

2. `make prepare`：提前编译 SPEC2006 测试程序。在非官方虚拟机环境下可能不生效

3. `make cache`：开始并行测试 `$(tests)` 并保存结果到 `data/cacheModel/$(CACHE_TEST)`

4. `make doxygen`：生成 doxygen 文档

5. `make clean`：清理实验环境
