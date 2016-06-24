配置libvpx时

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
 $ mkdir build
 $ cd build
 $ ../libvpx/configure <options>
 $ make

 ./../libvpx/configure --target=x86-win32-vs9 --enable-static-msvcrt --disable-install-bins
 拷贝yasm到这个目录
 修改工程里面yasm配置 . -I..\..\libvpx