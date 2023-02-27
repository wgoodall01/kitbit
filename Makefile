.SUFFIXES: # disable builtin make rules

FQBN=Seeeduino:mbed:xiaonRF52840Sense

ARDUINO=./bin/arduino-cli

.PHONY: build
build: target/KitBit.ino.bin

.PHONY: setup
setup: bin/arduino-cli
	$(ARDUINO) core update-index
	$(ARDUINO) core install Seeeduino:mbed
	$(ARDUINO) lib install ArduinoBLE@1.3.2
	$(ARDUINO) lib install "Seeed Arduino LSM6DS3@2.0.3"
	$(ARDUINO) lib install "Adafruit SleepyDog Library@1.6.3"

.PHONY: upload
upload: env_kitbit_port bin/arduino-cli target/KitBit.ino.bin
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
