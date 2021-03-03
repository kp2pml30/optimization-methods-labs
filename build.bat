@echo off

SET BUILDDIR="tmp\build\methopts"
SET BTYPE=%1

MKDIR %BUILDDIR% 2>nul

IF DEFINED BTYPE (
	RMDIR /S /Q %BUILDDIR%

	WHERE ninja 2>nul
	IF "%ERRORLEVEL%"=="0" (
		SET GEN=" -G Ninja "
	)
	cmake "%GEN%" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "-DCMAKE_BUILD_TYPE="%*" -S . -B "%BUILDDIR%"

	MKLINK compile_commands.json "%BUILDDIR%\compile_commands.json" 2>nul
)

cmake --build "%BUILDDIR%"
