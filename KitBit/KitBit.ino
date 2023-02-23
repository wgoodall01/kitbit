#include <ArduinoBLE.h>
#include <Arduino_LSM6DSOX.h>
#include <WiFiNINA.h>

// UUID for the KitBit BLE service.
#define BLE_SERVICE_UUID "615061c9-f304-4a14-8ff8-5014e60d020d"

// UUID for the KitBit BLE characteristic.
#define BLE_CHARACTERISTIC_UUID "2e47d8ad-f98d-46f3-beda-34ceebb3706c"

BLEService ble_service(BLE_SERVICE_UUID);
BLEFloatCharacteristic ble_characteristic(BLE_CHARACTERISTIC_UUID, BLERead | BLENotify);

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
        ble_service.addCharacteristic(ble_characteristic);
        BLE.addService(ble_service);
        BLE.advertise();
        Serial.println("    Advertising KitBit and listening for connections.");
    } else {
        Serial.println("[!] Failed to initialize Bluetooth LE.");
    }
}

void loop() {
    static int tick = 0;
    tick++;

    static int accel_readings = 0;
    static int gyro_readings = 0;

    static float accel_x = 0, accel_y = 0, accel_z = 0;
    if (IMU.accelerationAvailable()) {
        float x, y, z;
        IMU.readAcceleration(x, y, z);
        accel_x += x;
        accel_y += y;
        accel_z += z;

        accel_readings++;
    }

    static float gyro_x = 0, gyro_y = 0, gyro_z = 0;
    if (IMU.gyroscopeAvailable()) {
        float x, y, z;
        IMU.readGyroscope(x, y, z);
        gyro_x += x;
        gyro_y += y;
        gyro_z += z;

        gyro_readings++;
    }

    static float temperature;
    if (IMU.temperatureAvailable()) {
        IMU.readTemperatureFloat(temperature);
    }

    if (tick % 8 == 0) {
        Serial.print("Accel\t\t");
        Serial.print("x:");
        Serial.print(accel_x / accel_readings);
        Serial.print("\ty:");
        Serial.print(accel_y / accel_readings);
        Serial.print("\tz:");
        Serial.print(accel_z / accel_readings);
        Serial.print("\t\t\tGyro \t");
        Serial.print("x:");
        Serial.print(gyro_x / gyro_readings);
        Serial.print("\ty:");
        Serial.print(gyro_y / gyro_readings);
        Serial.print("\tz:");
        Serial.print(gyro_z / gyro_readings);
        Serial.print("\t\t\tTemp \t");
        Serial.print(temperature);
        Serial.println();

        gyro_x = 0;
        gyro_y = 0;
        gyro_z = 0;
        gyro_readings = 0;
        accel_x = 0;
        accel_y = 0;
        accel_z = 0;
        accel_readings = 0;

        static bool led = false;
        led = !led;
        digitalWrite(LED_BUILTIN, led ? HIGH : LOW);
    }

    // Update BLE devices.
    BLEDevice central = BLE.central();
    if (central) {
        digitalWrite(LEDB, HIGH);

        Serial.print("ble: connected to central mac=");
        Serial.print(central.address());
        Serial.println();
        digitalWrite(LEDB, HIGH);

        ble_characteristic.writeValue(accel_z);

        Serial.println("ble: wait for disconnect");
        while (central.connected()) {
        }
        Serial.println("ble: disconnected");

        digitalWrite(LEDB, LOW);
    }
}
