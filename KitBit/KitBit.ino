#include <ArduinoBLE.h>
#include <Arduino_LSM6DSOX.h>
#include <Scheduler.h>
#include <WiFiNINA.h>

// UUID for the KitBit BLE service.
#define BLE_SERVICE_UUID "615061c9-f304-4a14-8ff8-5014e60d020d"
BLEService ble_service(BLE_SERVICE_UUID);

// UUID for the KitBit AccelX characteristic.
#define BLE_ACCEL_X_UUID "2e47d8ad-f98d-46f3-beda-34ceebb3706c"
BLEFloatCharacteristic ble_accel_x_characteristic(BLE_ACCEL_X_UUID, BLERead | BLENotify);

#define BLE_ACCEL_Y_UUID "86c81537-b535-4fb5-ab6c-5cb3e57533bb"
BLEFloatCharacteristic ble_accel_y_characteristic(BLE_ACCEL_Y_UUID, BLERead | BLENotify);

#define BLE_ACCEL_Z_UUID "78834ccf-1141-4f54-ab86-30f4db51516f"
BLEFloatCharacteristic ble_accel_z_characteristic(BLE_ACCEL_Z_UUID, BLERead | BLENotify);

float accel_x;
float accel_y;
float accel_z;

float gyro_x;
float gyro_y;
float gyro_z;

float temperature;

unsigned long num_readings;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);

    // Wait for serial port availability.
    while (!Serial) {
    }
    Serial.println("=== KitBit ===");

    // Initialize the IMU.
    if (IMU.begin()) {
        Serial.println("[+] IMU initialized.");
        Serial.print("    Accelerometer Sample rate = ");
        Serial.print(IMU.accelerationSampleRate());
        Serial.println(" Hz");
        Serial.print("    Gyroscope Sample rate = ");
        Serial.print(IMU.gyroscopeSampleRate());
        Serial.println(" Hz");
    } else {
        Serial.println("[!] Failed to initialize IMU.");
        while (true) {
        }
    }

    // Initialize bluetooth.
    if (BLE.begin()) {
        Serial.println("[+] Bluetooth LE initialized.");
        BLE.setLocalName("KitBit");
        BLE.setDeviceName("KitBit Device");
        BLE.setAdvertisedService(ble_service);

        ble_service.addCharacteristic(ble_accel_x_characteristic);
        ble_service.addCharacteristic(ble_accel_y_characteristic);
        ble_service.addCharacteristic(ble_accel_z_characteristic);

        BLE.addService(ble_service);
        BLE.advertise();
        Serial.println("    Advertising KitBit and listening for connections.");
    } else {
        Serial.println("[!] Failed to initialize Bluetooth LE.");
    }

    // Start the sensor-polling task.
    Scheduler.startLoop(poll_sensors);
}

void loop() {
    static int tick = 0;
    tick++;

    // Update BLE devices.
    BLEDevice central = BLE.central();
    if (central) {
        digitalWrite(LEDB, HIGH);

        Serial.print("ble: connected to central mac=");
        Serial.print(central.address());
        Serial.println();

        digitalWrite(LEDB, HIGH);

        float old_accel_x = 0, old_accel_y = 0, old_accel_z = 0;
        unsigned int num_updates = 0;
        while (central.connected()) {

            if (num_updates != 0) {
                delay(100);
            }

            if (old_accel_x != accel_x) {
                ble_accel_x_characteristic.writeValue(accel_x);
                old_accel_x = accel_x;
                Serial.println("ble: update accel_x");
            }
            if (old_accel_y != accel_y) {
                ble_accel_y_characteristic.writeValue(accel_y);
                old_accel_y = accel_y;
                Serial.println("ble: update accel_y");
            }
            if (old_accel_z != accel_z) {
                ble_accel_z_characteristic.writeValue(accel_z);
                old_accel_z = accel_z;
                Serial.println("ble: update accel_z");
            }

            if (num_updates % 64 == 0) {
                Serial.print("ble: num_updates=");
                Serial.print(num_updates);
                Serial.println();
            }

            yield();
            num_updates++;
        }

        Serial.println("ble: disconnected");

        digitalWrite(LEDB, LOW);
    }

    yield();
}

void poll_sensors() {
    if (IMU.accelerationAvailable()) {
        float x, y, z;
        IMU.readAcceleration(x, y, z);

        accel_x = x;
        accel_y = y;
        accel_z = z;
    }

    if (IMU.gyroscopeAvailable()) {
        float x, y, z;
        IMU.readGyroscope(x, y, z);

        gyro_x = x;
        gyro_y = y;
        gyro_z = z;
    }

    if (IMU.temperatureAvailable()) {
        float temp;
        IMU.readTemperatureFloat(temp);

        temperature = temp;
    }

    if (num_readings % 64 == 0) {
        Serial.print("sensors: taken num_readings=");
        Serial.print(num_readings);
        Serial.println();
    }

    num_readings++;
    delay(3);
    yield();
}
