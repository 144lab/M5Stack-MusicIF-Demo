
build:
	arduino-cli compile --fqbn esp32:esp32:esp32 mft-demo

flash:
	esptool.py --chip esp32 --chip esp32 write_flash --flash_mode dio --flash_freq 80m --flash_size 4MB --flash_mode dio -z 0x10000 mft-demo/mft-demo.esp32.esp32.esp32.bin