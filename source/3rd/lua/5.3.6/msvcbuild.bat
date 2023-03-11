@rem Either open a "Visual Studio .NET Command Prompt"
@rem (Note that the Express Edition does not contain an x64 compiler)
@rem -or-
@rem Open a "Windows SDK Command Shell" and set the compiler environment:
@rem     setenv /release /x86
@rem   -or-
@rem     setenv /release /x64
@rem
@rem Then cd to this directory and run this script.

@if not defined INCLUDE goto :FAIL
@echo off

REM 动态链接库编译开关 true/false
REM 如果选择编译动态链接库，编译完成后要将lua(d).dll从X64目录下复制一份到product目录下
@set BUILD_DYNAMIC_LIB="true"

md bin
md lib
md ..\include
cd src

@if "%1" neq "debug" goto :RELEASE
:DEBUG
@set DLLNAME=luad.dll
@set LIBNAME=luad.lib
goto :WORKFLOW
:RELEASE
@set DLLNAME=lua.dll
@set LIBNAME=lua.lib


:WORKFLOW
if %BUILD_DYNAMIC_LIB% equ "true" goto :BUILD_DYNAMIC_LIBRARY

cl /c /nologo /W3 /O2 /Ob1 /Oi /Gs /MD /D_CRT_SECURE_NO_DEPRECATE l*.c Y*.c
@rem 重命名lua.c,luac.c的中间文件，lib是不需要连接该文件的
ren lua.obj lua.o
ren luac.obj luac.o
lib /OUT:%LIBNAME% *.obj
copy %LIBNAME% ..\lib\%LIBNAME%
del *.o *.obj *.exp *.lib *.dll *.exe
goto :BUILD_END


:BUILD_DYNAMIC_LIBRARY
cl /c /nologo /W3 /O2 /Ob1 /Oi /Gs /MD /D_CRT_SECURE_NO_DEPRECATE /DLUA_BUILD_AS_DLL l*.c Y*.c
ren lua.obj lua.o
ren luac.obj luac.o
link /DLL /IMPLIB:%LIBNAME% /OUT:%DLLNAME% *.obj
link /OUT:lua53.exe lua.o %LIBNAME%
link /OUT:luac53.exe luac.o *.obj
copy lua53.exp ..\bin\lua53.exp
copy %LIBNAME% ..\lib\%LIBNAME%
copy %DLLNAME% ..\bin\%DLLNAME%
copy lua53.exe ..\bin\lua53.exe
copy luac53.exe ..\bin\luac53.exe
del *.o *.obj *.exp *.lib *.dll *.exe


:BUILD_END
copy lauxlib.h ..\..\include\lauxlib.h
copy lua.h ..\..\include\lua.h
copy lua.hpp ..\..\include\lua.hpp
copy luaconf.h ..\..\include\luaconf.h
copy lualib.h ..\..\include\lualib.h
cd ..\

copy lib\%LIBNAME% ..\..\lib\x64\%LIBNAME%
if %BUILD_DYNAMIC_LIB% equ "true" (copy bin\%DLLNAME% ..\..\lib\x64\%DLLNAME%)

REM rd /s /q lib
REM rd /s /q bin

@echo === Successfully built LuaSrc for Windows ===
@goto :END

:FAIL
@echo You must open a "Visual Studio .NET Command Prompt" to run this script
:END

