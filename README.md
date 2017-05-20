# 2sat

I assume you have the following libraries installed
# std C libraries
# POSIX 2011 libraries (includes std C)
# LibC (or BSD-libc) (includes POSIX)
Generally for Mac and Linux OS's these come installed out of the box.
But to be safe you may want to grab GNU GCC, it comes with POSIX, BSD-LibC is an additional install
# Compilers this code compiles without errors:
`Using built-in specs.
COLLECT_GCC=gcc
COLLECT_LTO_WRAPPER=/usr/lib/gcc/x86_64-linux-gnu/6/lto-wrapper
Target: x86_64-linux-gnu
Configured with: ../src/configure -v --with-pkgversion='Ubuntu 6.2.0-3ubuntu11~16.04' --with-bugurl=file:///usr/share/doc/gcc-6/README.Bugs --enable-languages=c,ada,c++,java,go,d,fortran,objc,obj-c++ --prefix=/usr --program-suffix=-6 --enable-shared --enable-linker-build-id --libexecdir=/usr/lib --without-included-gettext --enable-threads=posix --libdir=/usr/lib --enable-nls --with-sysroot=/ --enable-clocale=gnu --enable-libstdcxx-debug --enable-libstdcxx-time=yes --with-default-libstdcxx-abi=new --enable-gnu-unique-object --disable-vtable-verify --enable-libmpx --enable-plugin --with-system-zlib --disable-browser-plugin --enable-java-awt=gtk --enable-gtk-cairo --with-java-home=/usr/lib/jvm/java-1.5.0-gcj-6-amd64/jre --enable-java-home --with-jvm-root-dir=/usr/lib/jvm/java-1.5.0-gcj-6-amd64 --with-jvm-jar-dir=/usr/lib/jvm-exports/java-1.5.0-gcj-6-amd64 --with-arch-directory=amd64 --with-ecj-jar=/usr/share/java/eclipse-ecj.jar --enable-objc-gc --enable-multiarch --disable-werror --with-arch-32=i686 --with-abi=m64 --with-multilib-list=m32,m64,mx32 --enable-multilib --with-tune=generic --enable-checking=release --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu
Thread model: posix
gcc version 6.2.0 20160901 (Ubuntu 6.2.0-3ubuntu11~16.04)`
and
`Using built-in specs.
COLLECT_GCC=gcc
COLLECT_LTO_WRAPPER=/usr/local/Cellar/gcc/6.3.0_1/libexec/gcc/x86_64-apple-darwin15.6.0/6.3.0/lto-wrapper
Target: x86_64-apple-darwin15.6.0
Configured with: ../configure --build=x86_64-apple-darwin15.6.0 --prefix=/usr/local/Cellar/gcc/6.3.0_1 --libdir=/usr/local/Cellar/gcc/6.3.0_1/lib/gcc/6 --enable-languages=c,c++,objc,obj-c++,fortran --program-suffix=-6 --with-gmp=/usr/local/opt/gmp --with-mpfr=/usr/local/opt/mpfr --with-mpc=/usr/local/opt/libmpc --with-isl=/usr/local/opt/isl --with-system-zlib --enable-libstdcxx-time=yes --enable-stage1-checking --enable-checking=release --enable-lto --with-build-config=bootstrap-debug --disable-werror --with-pkgversion='Homebrew GCC 6.3.0_1' --with-bugurl=https://github.com/Homebrew/homebrew-core/issues --enable-plugin --disable-nls --enable-multilib
Thread model: posix
gcc version 6.3.0 (Homebrew GCC 6.3.0_1) `
# OS' tested on
- Ubuntu 16.04 LTS x64
- MacOS X El Capitan
