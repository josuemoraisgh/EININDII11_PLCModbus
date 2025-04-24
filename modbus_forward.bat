@echo off
setlocal

::—————————————————————————————————————————
:: Auto‐elevate to Administrator
::—————————————————————————————————————————
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
if '%errorlevel%' NEQ '0' (
  echo Solicitando permissões de administrador…
  powershell -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
  exit /b
)

::—————————————————————————————————————————
:: Garantir IP Helper e IPv6
::—————————————————————————————————————————
echo [+] Iniciando serviço IP Helper e habilitando IPv6…
sc config iphlpsvc start= auto >nul
sc start iphlpsvc         >nul

::—————————————————————————————————————————
:: Limpar regras antigas
::—————————————————————————————————————————
echo [+] Resetando regras antigas de portproxy…
netsh interface portproxy reset >nul

::—————————————————————————————————————————
:: Parâmetros
::—————————————————————————————————————————
set LISTEN4=127.0.0.1
set LISTEN6=::1
set PORT=502
set REMOTE=10.0.67.199

::—————————————————————————————————————————
:: Criar portproxy v4→v4
::—————————————————————————————————————————
echo [+] Criando portproxy IPv4→IPv4: %LISTEN4%:%PORT% → %REMOTE%:%PORT%
netsh interface portproxy add v4tov4 ^
    listenaddress=%LISTEN4% listenport=%PORT% ^
    connectaddress=%REMOTE% connectport=%PORT%

::—————————————————————————————————————————
:: Criar portproxy v6→v4 (para conexões via ::1)
::—————————————————————————————————————————
echo [+] Criando portproxy IPv6→IPv4: [%LISTEN6%]:%PORT% → %REMOTE%:%PORT%
netsh interface portproxy add v6tov4 ^
    listenaddress=%LISTEN6% listenport=%PORT% ^
    connectaddress=%REMOTE% connectport=%PORT%

::—————————————————————————————————————————
:: Mostrar status
::—————————————————————————————————————————
echo.
echo Regras ativas de portproxy:
netsh interface portproxy show all
echo.
echo TCP LISTEN em portas locais:
netstat -an | findstr ":%PORT%"
echo.
echo Proxy ativo em %LISTEN4%:%PORT% e [::1]:%PORT%
echo Pressione qualquer tecla para remover as regras e sair...
pause >nul

::—————————————————————————————————————————
:: Remover regras
::—————————————————————————————————————————
echo [-] Removendo portproxy IPv4→IPv4…
netsh interface portproxy delete v4tov4 ^
    listenaddress=%LISTEN4% listenport=%PORT%
echo [-] Removendo portproxy IPv6→IPv4…
netsh interface portproxy delete v6tov4 ^
    listenaddress=%LISTEN6% listenport=%PORT%

echo.
echo Regras removidas. Saindo.
endlocal
exit /b