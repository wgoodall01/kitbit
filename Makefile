.SUFFIXES: # disable builtin make rules

FQBN=arduino:mbed_nano:nanorp2040connect

ARDUINO=./bin/arduino-cli

.PHONY: build
build: target/KitBit.ino.bin

.PHONY: setup
setup: bin/arduino-cli
	$(ARDUINO) core install arduino:mbed_nano
	$(ARDUINO) lib install Arduino_LSM6DSOX@1.1.2
	$(ARDUINO) lib install ArduinoBLE@1.3.2
	$(ARDUINO) lib install WiFiNINA@1.8.13

.PHONY: upload
upload: env_kitbit_port bin/arduino-cli target/arduino_setup_done target/KitBit.ino.bin
	$(ARDUINO) upload --port $(KITBIT_PORT) --input-dir target --fqbn $(FQBN)

# Compile the sketch
target/KitBit.ino.bin: $(shell find KitBit)
	$(ARDUINO) compile --fqbn $(FQBN) --output-dir target KitBit

.PHONY: env_kitbit_port
env_kitbit_port:
ifndef KITBIT_PORT
	$(error KITBIT_PORT is undefined. Run 'export KITBIT_PORT=<path to serial port>')
endif

bin/arduino-cli:
	curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$(PWD)/bin" sh

clean: 
	rm -rf target
