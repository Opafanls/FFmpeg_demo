<!--
 * @Author: hongqianhui hongqianhui@bytedance.com
 * @Date: 2022-05-05 21:59:53
 * @LastEditTime: 2022-05-05 22:02:17
 * @FilePath: /c_pro/ffmpeg_demos/README.md
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
-->
# ffmpeg安装

ffmpeg安装可以参考这里：
https://trac.ffmpeg.org/wiki/CompilationGuide

Ubuntu/Debian等可以直接参考这：https://trac.ffmpeg.org/wiki/CompilationGuide/Ubuntu

判断ffmpeg是否安装完成：
```shell
pkg-config --cflags libavcodec
```
如果正确输出即安装完成，pkg-config需要预先安装。


## subtitle_1

需要安装sdl2、libass等，具体可以查看Makefile的依赖项。