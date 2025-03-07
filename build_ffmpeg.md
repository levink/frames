# How to build ffpmeg

## Run 'X64 Native Tools Command Prompt for VS 2022' 
```
cd "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build"
vcvarsall.bat x64
```

## Run msys2 shell from terminal
```
cd C:\msys64
msys2_shell.cmd -msys -use-full-path
```

## In msys2 shell build ffmpeg
```          
cd C:/Projects/ffmpeg
./configure \
	--prefix=./install/static	\
	--toolchain=msvc	\
	--target-os=win64	\
	--arch=x86_64		\
	--enable-asm		\
	--disable-programs \
	--disable-doc \
	--enable-shared
	
make -j 4
make install
```