@echo off

rem chcp 65001 >nul
setlocal enabledelayedexpansion

set "REPLACER=replacer.exe"
set "PASS=0"
set "FAIL=0"

echo.
echo ========================================
echo   REPLACER TEST SUITE v26.0424b
echo ========================================
echo.

REM ========================================
REM Test 1: Basic text replacement
REM ========================================
echo [TEST 1] Basic text replacement
echo Command: echo test line ^| %REPLACER% -:- "'test':'demo'"
set "EXPECTED=demo line "
echo test line | %REPLACER% -:- "'test':'demo'" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 2: Hex replacement
REM ========================================
echo [TEST 2] Hex byte replacement
echo Command: echo AB ^| %REPLACER% -:- 0x41:0x58
echo AB | %REPLACER% -:- 0x41:0x58 > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=XB "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 3: Multiple operations
REM ========================================
echo [TEST 3] Multiple operations
echo Command: echo test line ^| %REPLACER% -:- "'test':'demo'" "'line':'text'"
echo test line | %REPLACER% -:- "'test':'demo'" "'line':'text'" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=demo text "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 4: Case-insensitive
REM ========================================
echo [TEST 4] Case-insensitive /i
echo Command: echo TeSt LiNe ^| %REPLACER% -:- "'test':'demo'/i"
echo TeSt LiNe | %REPLACER% -:- "'test':'demo'/i" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=demo LiNe "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 5: Wildcard
REM ========================================
echo [TEST 5] Wildcard pattern
echo Command: echo Version: 1.2.3 ^| %REPLACER% -:- "'Version: '+*:'Found'"
echo Version: 1.2.3 | %REPLACER% -:- "'Version: '+*:'Found'" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=Found"
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 6: Capture \0
REM ========================================
echo [TEST 6] Capture group \0
echo Command: echo error ^| %REPLACER% -:- "'error':'[\0]'"
echo error | %REPLACER% -:- "'error':'[\0]'" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=[error] "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 7: Numbered captures
REM ========================================
echo [TEST 7] Numbered captures \1 \2
echo Command: echo [ERROR] Message ^| %REPLACER% -:- "'['+{*}+'] '+{*}+'\n':'\2 (\1)'"
echo [ERROR] Message | %REPLACER% -:- "'['+{*}+'] '+{*}+'\n':'\2 (\1)'" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=Message  (ERROR)"
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 8: Named captures
REM ========================================
echo [TEST 8] Named captures
echo Command: echo Name: John, Age: 30 ^| %REPLACER% -:- "'Name: '+{name=*}+', Age: '+{age=*}+'\n':{age}+' years, '+{name}"
echo Name: John, Age: 30 | %REPLACER% -:- "'Name: '+{name=*}+', Age: '+{age=*}+'\n':{age}+' years, '+{name}" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=30  years, John"
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 9: Counter simple
REM ========================================
echo [TEST 9] Counter {#}
echo Command: echo TODO TODO TODO ^| %REPLACER% -:- "'TODO':'Task '+{#}"
echo TODO TODO TODO | %REPLACER% -:- "'TODO':'Task '+{#}" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=Task 1 Task 2 Task 3 "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 10: Counter zero-padded
REM ========================================
echo [TEST 10] Counter {#:001}
echo Command: echo TODO TODO TODO ^| %REPLACER% -:- "'TODO':'Item '+{#:001}"
echo TODO TODO TODO | %REPLACER% -:- "'TODO':'Item '+{#:001}" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=Item 001 Item 002 Item 003 "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 11: Counter alphabetic
REM ========================================
echo [TEST 11] Counter {#:A}
echo Command: echo TODO TODO TODO ^| %REPLACER% -:- "'TODO':'Task '+{#:A}"
echo TODO TODO TODO | %REPLACER% -:- "'TODO':'Task '+{#:A}" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=Task A Task B Task C "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 12: Counter hex
REM ========================================
echo [TEST 12] Counter {#:0x01}
echo Command: echo TODO TODO TODO ^| %REPLACER% -:- "'TODO':'0x'+{#:0x01}"
echo TODO TODO TODO | %REPLACER% -:- "'TODO':'0x'+{#:0x01}" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=0x0x01 0x0x02 0x0x03 "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 13: Counter range
REM ========================================
echo [TEST 13] Counter range {#:1-2}
echo Command: echo TODO TODO TODO TODO ^| %REPLACER% -:- "'TODO':'Task '+{#:1-2}"
echo TODO TODO TODO TODO | %REPLACER% -:- "'TODO':'Task '+{#:1-2}" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=Task 1 Task 2 TODO TODO "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 14: Delete
REM ========================================
echo [TEST 14] Delete operation
echo Command: echo test line here ^| %REPLACER% -:- "'test ':"
echo test line here | %REPLACER% -:- "'test ':" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=line here "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 15: Encoding parameter
REM ========================================
echo [TEST 15] Encoding @dos
echo Command: echo test ^| %REPLACER% -:- "@dos" "'test':'demo'"
echo test | %REPLACER% -:- "@dos" "'test':'demo'" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=demo "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 16: File with encoding
REM ========================================
echo [TEST 16] File encoding win@dos
echo Command: %REPLACER% test_file.txt "win@dos" "'test':'demo'"
echo test > test_file.txt
%REPLACER% test_file.txt "win@dos" "'test':'demo'" 2>nul
if exist test_file_OUT.txt (
    echo   PASS - File created
    set /a PASS+=1
) else (
    echo   FAIL - File not created
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 17: Counter + Capture
REM ========================================
echo [TEST 17] Counter + Capture
echo Command: echo error warning error ^| %REPLACER% -:- "'error':'['+{#}+'] \0'"
echo error warning error | %REPLACER% -:- "'error':'['+{#}+'] \0'" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=[1] error warning [2] error "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 18: Case-insensitive + Counter
REM ========================================
echo [TEST 18] Case-insensitive + Counter
echo Command: echo TODO todo ToDo ^| %REPLACER% -:- "'todo':'Task '+{#}/i"
echo TODO todo ToDo | %REPLACER% -:- "'todo':'Task '+{#}/i" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=Task 1 Task 2 Task 3 "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 19: Cyrillic text replacement
REM ========================================
echo [TEST 19] Cyrillic text replacement
echo Command: echo €¡¢£¤1 á«®¢®2 ^| %REPLACER% -:- "'€¡¢£¤1':'’àŸ¬3'"
echo €¡¢£¤1 á«®¢®2 | %REPLACER% -:- "'€¡¢£¤1':'’àŸ¬3'" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=’àŸ¬3 á«®¢®2 "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM Test 20: Cyrillic case-insensitive
REM ========================================
echo [TEST 20] Cyrillic case-insensitive
echo Command: echo ©æãª4 –¨à5 ä¨®6 ^| %REPLACER% -:- "'æ¨à5':'ˆ¬ï7'/i"
echo ©æãª4 –¨à5 ä¨®6 | %REPLACER% -:- "'–¨à5':'ˆ¬ï7'/i" > temp_out.txt 2>nul
for /f "delims=" %%i in (temp_out.txt) do set "RESULT=%%i"
set "EXPECTED=©æãª4 ˆ¬ï7 ä¨®6 "
echo   Expected: %EXPECTED%
echo   Got:      %RESULT%
if "%RESULT%"=="%EXPECTED%" (
    echo   PASS
    set /a PASS+=1
) else (
    echo   FAIL
    set /a FAIL+=1
)
echo.

REM ========================================
REM SUMMARY
REM ========================================
echo.
echo ========================================
echo   TEST SUMMARY
echo ========================================
set /a TOTAL=%PASS%+%FAIL%
echo   Total tests: %TOTAL%
echo   Passed: %PASS%
echo   Failed: %FAIL%
echo.

if %FAIL%==0 (
    echo   ALL TESTS PASSED
) else (
    echo   SOME TESTS FAILED
)
echo.
echo ========================================

REM Cleanup
del temp_out.txt 2>nul
del test_file.txt 2>nul
del test_file_OUT.txt 2>nul

pause
