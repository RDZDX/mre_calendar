"C:\Program Files\MRE_SDK\tools\DllPackage.exe" "E:\MyGitHub\mre_calendar\mre_calendar.vcproj"
if %errorlevel% == 0 (
 echo postbuild OK.
  copy mre_calendar.vpp ..\..\..\MoDIS_VC9\WIN32FS\DRIVE_E\mre_calendar.vpp /y
exit 0
)else (
echo postbuild error
  echo error code: %errorlevel%
  exit 1
)

