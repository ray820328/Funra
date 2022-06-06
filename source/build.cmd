@ECHO off
SET ROOT_PATH=%CD%/..

setlocal EnableDelayedExpansion
if "%1"=="3rd" (
    echo "build 3rd..."
) else (
    echo  "start building funra..."
    SET FUNRAY_BUILD_OPTIONS=-DCMAKE_C_FLAGS="-g -wall"
	SET "SRC_RBASE_ROOT=%ROOT_PATH%/source/rbase"
	SET "DEST_RBASE_ROOT=%ROOT_PATH%/proj/vs2017/rbase"
	::MD "!DEST_RBASE_ROOT!/build"
    cmake -S "!SRC_RBASE_ROOT!" -B "!DEST_RBASE_ROOT!" !FUNRAY_BUILD_OPTIONS! -A x64
    ::cmake --build "!DEST_RBASE_ROOT!" --config Debug --target install
	cmake --build "!DEST_RBASE_ROOT!" --config Debug
    cmake --build "!DEST_RBASE_ROOT!" --config Release

    ::MOVE /Y "%INSTALL_PREFIX%/rcommon.lib" "%INSTALL_PREFIX%"
    ::RMDIR /S /Q "!DEST_RBASE_ROOT!/build"
)
setlocal Disabledelayedexpansion

