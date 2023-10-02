source ~/esp/esp-idf/export.sh
ESPPORT=/dev/tty.usbserial-1420 /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake --build cmake-build-debug-esp32 --target monitor -j 6
