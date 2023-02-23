.SUFFIXES: # disable builtin make rules



FQBN=arduino:mbed_nano:nanorp2040connect

ARDUINO=./bin/arduino-cli

.PHONY: build
build: target/KitBit.ino.bin

.PHONY: upload
upload: env-kitbit-port $(ARDUINO) target/KitBit.ino.bin
	$(ARDUINO) upload --port $(KITBIT_PORT) --input-dir target --fqbn $(FQBN)

# Compile the sketch
target/KitBit.ino.bin: $(shell find KitBit)
	$(ARDUINO) compile --fqbn $(FQBN) --output-dir target KitBit

.PHONY: env-kitbit-port
env-kitbit-port:
ifndef KITBIT_PORT
	$(error KITBIT_PORT is undefined. Run 'export KITBIT_PORT=<path to serial port>')
endif

.PHONY: preflight
preflight: $(ARDUINO)
	$(ARDUINO) core install arduino:mbed_nano

bin/arduino-cli:
	curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$(PWD)/bin" sh

clean: 
	rm -rf target
