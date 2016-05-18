@echo off

set argC=0
for %%x in (%*) do Set /A argC+=1

if %argC% LSS 1 (
	echo You need to provide to amount of tests to perform
	goto exit
)

set count=%1
set count=%count:"=%
for /l %%x in (1, 1, %count%) do (call :test %%x)
goto exit

:test
echo Test
echo %1
echo true
echo.

:exit