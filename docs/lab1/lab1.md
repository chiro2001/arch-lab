## 附加题

修改前的结果：

```Bash
$ make perf MY_PIN_TOOL=insDependDist
/opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/pin -t obj-intel64/insDependDist.so --  ./coremark.exe
2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 7146
Total time (secs): 7.146000
Iterations/Sec   : 419.815281
ERROR! Must execute for at least 10 secs for a valid result!
Iterations       : 3000
Compiler version : GCC12.2.0
Compiler flags   : -O2   -lrt
Memory location  : Please put data memory location here
                        (e.g. code in flash, data on heap etc)
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0xcc42
Errors detected
```

![before](../../lab1/workspace/imgs/insDependDist-coremark.exe.png)



修改后的结果：

```Bash
$ make perf MY_PIN_TOOL=insDependDist2
/opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/pin -t obj-intel64/insDependDist2.so --  ./coremark.exe
2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 4736
Total time (secs): 4.736000
Iterations/Sec   : 633.445946
ERROR! Must execute for at least 10 secs for a valid result!
Iterations       : 3000
Compiler version : GCC12.2.0
Compiler flags   : -O2   -lrt
Memory location  : Please put data memory location here
                        (e.g. code in flash, data on heap etc)
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0xcc42
Errors detected
```

![after](../../lab1/workspace/imgs/insDependDist2-coremark.exe.png)

