echo. "%CD%"
set vrtm_dir=%CD%
echo. "%vrtm_dir%"
set vrtmcmdstr="wscript '%vrtm_dir%\..\scripts\nocmd.vbs' '%vrtm_dir%\..\scripts\vrtmhandler.cmd'"
schtasks /create /sc ONSTART /tn vRTM /tr %vrtmcmdstr% /f
sc create vRTM binPath= "%vrtm_dir%\vrtmservice.exe" start= auto DisplayName= "Intel CIT vRTM"
sc description vRTM "Runs Intel CIT vRTM"
sc failure vRTM reset= 0 actions= restart/60000
net start vRTM /y
echo. "%vrtmcmdstr%"