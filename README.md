# arch-lab

## lab2

吃肉[^1]尝试尝试通过 [corrosion](https://github.com/chiro2001/corrosion-pin) 和 CMake 将 Pin 与 Rust 结合，通过 Pin 加载 Rust 的动态链接然后将数据送给 Rust 中的分支预测模型，但是失败了。替换了 Pin 的二进制底层库（使用 Pin 的闭源 `libc-dynamic`、`libm-dynamic`、`libdl-dynamic` 等而不是系统 gcc 的  `libc`、`libm`、`libc++`等）的 Rust，在 Pin 端加载动态库的话会出现 [dlopen failed: unknown reloc type 16](https://github.com/chiro2001/arch-lab/commit/4a1043fa6945350e10ac3bb097db8a73c4b04a2e)。

吃肉太菜了，没法继续调试 Pin 的二进制文件 `libdl-dynamic.so`，别学吃肉。

[^1]: Chiro