@echo on
:: vcvarsall.bat sets various env vars like PATH, INCLUDE, LIB, LIBPATH for the
:: specified build architecture
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
@echo on

pip3 install --upgrade --user meson~=1.2 || goto :error
meson setup -Dbackend_max_links=1 -Ddebug=false _build || goto :error
meson compile -C _build || goto :error
meson test -C _build -t "%MESON_TEST_TIMEOUT_MULTIPLIER%" --print-errorlogs --suite pango || goto :error

exit /b 0

:error
exit /b 1
