@echo off

setlocal

pushd %CD%
cd %~dp0%


set prj_name=%1
set prj_dir=%2
set prj_platform=%3
set prj_config=%4
set sol_name=%5
set target_name=%6
set target_ext=%7
set pxe_upload=%8
if [%FILE_DESTINATION%]==[__EMPTY__] goto no_share
	set share_path=%FILE_DESTINATION%\%sol_name%
:no_share
set full_share_path=""
set full_project_path=""

echo "Project name: [%prj_name%]"
echo "Project dir: [%prj_dir%]"
echo "Platform name: [%prj_platform%]"
echo "Configuration name: [%prj_config%]"
echo "Solution name: [%sol_name%]"
echo "Target name: [%target_name%]"
echo "Target extension: [%target_ext%]"

set int_platform=x64

echo %prj_dir%..\bin\%prj_platform%\%prj_config% 
if NOT EXIST %prj_dir%..\bin\%prj_platform%\%prj_config% failmsg.cmd "%prj_dir%..\bin\%prj_platform%\%prj_config% NOT found"

set full_project_path=%prj_dir%..\bin\%prj_platform%\%prj_config%\%prj_name%

echo "Full project path: [%full_project_path%]"

if [%pxe_upload%]==[] goto pre_end

if [%PXE_PATH%]==[__EMPTY__] goto no_pxe
if [%PXE_PATH%]==[] goto no_pxe
echo %full_project_path%\%target_name%%target_ext%
echo %PXE_PATH%\%target_name%%target_ext%
xcopy /Y /S %full_project_path%\%target_name%%target_ext% %PXE_PATH%\* 
copy %full_project_path%\%target_name%%target_ext% %PXE_PATH%\HAL9000.bin

:no_pxe

if [%PXE_PATH2%]==[__EMPTY__] goto no_pxe_2
if [%PXE_PATH2%]==[] goto no_pxe_2
winscp /command "open ftp://tftp:tftp@access-pxe.clj.bitdefender.biz -passive=on" "option confirm off" "put %full_project_path%\%target_name%%target_ext% %PXE_PATH2%/pxeboot.bin" "exit"
:no_pxe_2

:pre_end
echo --INFO: build done!
goto end


:end
:: --- reload initial current directory ---
popd
exit /b %ERRORLEVEL%