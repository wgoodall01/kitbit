use anyhow::{ensure, Context, Error, Result};
use btleplug::api::{Central, Manager as _, Peripheral as _, ScanFilter};
use btleplug::api::{CentralEvent, CharPropFlags, Characteristic};
use btleplug::platform::{Manager, Peripheral};
use clap::Parser;
use lazy_static::lazy_static;
use std::collections::BTreeMap;
use std::time::Duration;
use tokio::time::timeout;
use tokio_stream::StreamExt;
use uuid::Uuid;

/// Include the header file containing the UUIDs of the KitBit's BLE services and characteristics.
const BLUETOOTH_UUID_HEADER: &str = include_str!("../../KitBit/bluetooth_uuids.h");

lazy_static! {
    static ref BLE_IDENTIFIERS: BleIdentifiers = parse_header_for_uuids();
    static ref ACCEL: Characteristic = Characteristic {
        uuid: BLE_IDENTIFIERS.char_accel_uuid,
        service_uuid: BLE_IDENTIFIERS.service_uuid,
        properties: CharPropFlags::NOTIFY | CharPropFlags::READ,
    };
    static ref GYRO: Characteristic = Characteristic {
        uuid: BLE_IDENTIFIERS.char_gyro_uuid,
        service_uuid: BLE_IDENTIFIERS.service_uuid,
        properties: CharPropFlags::NOTIFY | CharPropFlags::READ,
    };
    static ref TEMP: Characteristic = Characteristic {
        uuid: BLE_IDENTIFIERS.char_temp_uuid,
        service_uuid: BLE_IDENTIFIERS.service_uuid,
        properties: CharPropFlags::NOTIFY | CharPropFlags::READ,
    };
}

// Define the characteristics we're interested in

/// KitBit host data collector.
///
/// NOTE: On MacOS, you need to make sure your terminal is allowed to access Bluetooth in System
/// Settings > Privacy & Security.
#[derive(clap::Parser)]
struct Args {
    /// The number of the Bluetooth adapter to use.
    #[arg(long, default_value("0"))]
    adapter: usize,
}

#[tokio::main]
async fn main() -> Result<()> {
    let args = Args::parse();

    let manager = Manager::new()
        .await
        .context("Failed to initialize BLE manager")?;

    // Get the first bluetooth adapter in the system
    let central = manager
        .adapters()
        .await
        .context("Failed to get BLE adapters")?
        .into_iter()
        .nth(args.adapter)
        .with_context(|| format!("BLE adapter #{} does not exist", args.adapter))?;

    println!("[+] Bluetooth initialized.");

    loop {
        println!("[+] Scanning for KitBit devices...");
        println!(
            "    Looking for service uuid={}",
            BLE_IDENTIFIERS.service_uuid
        );

        // Start scanning for a KitBit device.
        central
            .start_scan(ScanFilter::default())
            .await
            .context("Failed to start scan.")?;

        // Listen for advertisement events.
        let kitbit_id = central
            .events()
            .await
            .context("Failed to listen for BLE events")?
            .filter_map(|e| match e {
                CentralEvent::ServicesAdvertisement { id, services }
                    if services.contains(&BLE_IDENTIFIERS.service_uuid) =>
                {
                    Some(id)
                }
                _ => None,
            })
            .next()
            .await
            .context("No KitBit service advertisements received")?;

        // Stop scanning once we've found our device.
        central
            .stop_scan()
            .await
            .context("Failed to stop scanning for devices.")?;

        // Get the peripheral for the KitBit.
        let kitbit = central
            .peripheral(&kitbit_id)
            .await
            .context("No peripheral found with the ID advertised")?;

        match log_data_from_kitbit(kitbit).await {
            Ok(_) => break Ok(()),
            Err(e) => eprintln!("[!] Error: {e}"),
        }
    }
}

async fn log_data_from_kitbit(kitbit: Peripheral) -> Result<()> {
    println!("[-] Connecting to KitBit, id={}", kitbit.id());
    kitbit
        .connect()
        .await
        .context("Failed to connect to KitBit")?;

    // Subscribe to the characteristics.
    println!("[-] Subscribing to data updates...");
    kitbit
        .subscribe(&ACCEL)
        .await
        .context("Failed to subscribe to accelerometer updates")?;
    println!("    Subscribed to accelerometer updates");
    kitbit
        .subscribe(&GYRO)
        .await
        .context("Failed to subscribe to gyro updates")?;
    println!("    Subscribed to gyro updates");
    kitbit
        .subscribe(&TEMP)
        .await
        .context("Failed to subscribe to temperature updates")?;
    println!("    Subscribed to temp updates");

    println!("[-] Collecting data...");
    let mut notifications = kitbit
        .notifications()
        .await
        .context("Failed to get notification stream from KitBit")?;

    let mut last_time = std::time::Instant::now();
    let mut count_since_last = 0;

    while let Some(notif) = timeout(Duration::from_secs(1), notifications.next())
        .await
        .context("Timeout elapsed while waiting for data notification")?
    {
        if notif.uuid == ACCEL.uuid {
            let reading =
                Vec3::try_from(notif.value.as_slice()).context("Failed to parse acceleration")?;
            println!("    Accel: {reading}");
        }
        if notif.uuid == GYRO.uuid {
            let reading =
                Vec3::try_from(notif.value.as_slice()).context("Failed to parse acceleration")?;
            println!("    Gyro : {reading}");
        }
        if notif.uuid == TEMP.uuid {
            let temp = f32::from_le_bytes(
                notif
                    .value
                    .as_slice()
                    .try_into()
                    .context("Invalid temperature")?,
            );
            println!("    Temp : {temp}");
        }

        count_since_last += 1;
        if count_since_last >= 32 {
            let avg_time = last_time.elapsed() / count_since_last;
            eprintln!(
                "[?] Avg. time per reading: {:.2}ms",
                avg_time.as_secs_f64() * 1000.0
            );
            count_since_last = 0;
            last_time = std::time::Instant::now();
        }
    }

    loop {
        let start_time = std::time::Instant::now();

        // Read the three characteristics on a timeout.
        let read_kitbit_fut = async {
            let accel_raw = kitbit
                .read(&ACCEL)
                .await
                .context("Failed to read acceleration characteristic")?;
            let gyro_raw = kitbit
                .read(&GYRO)
                .await
                .context("Failed to read acceleration characteristic")?;
            let temp_raw = kitbit
                .read(&TEMP)
                .await
                .context("Failed to read temperature characteristic")?;
            anyhow::Ok((accel_raw, gyro_raw, temp_raw))
        };
        let (accel_raw, gyro_raw, temp_raw) = timeout(Duration::from_millis(500), read_kitbit_fut)
            .await
            .context("Timeout elapsed while reading data from KitBit")?
            .context("Failed to read data from KitBit")?;

        let time_to_read = start_time.elapsed();

        // Parse the data
        let accel = Vec3::try_from(&accel_raw[..]).context("Invalid acceleration")?;
        let gyro = Vec3::try_from(&gyro_raw[..]).context("Invalid acceleration")?;
        let temp = f32::from_le_bytes(
            temp_raw
                .as_slice()
                .try_into()
                .context("Invalid temperature")?,
        );

        println!(
            "    accel={accel}  gyro={gyro} temp={temp:.2} in {:.2}ms",
            time_to_read.as_secs_f64() * 1000.0
        );
    }
}

#[derive(Debug, Clone, Copy)]
struct Vec3 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

impl std::fmt::Display for Vec3 {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "({:.2}, {:.2}, {:.2})", self.x, self.y, self.z)
    }
}

impl TryFrom<&[u8]> for Vec3 {
    type Error = Error;

    fn try_from(source: &[u8]) -> Result<Vec3> {
        ensure!(source.len() == 12, "Buffer must be 12 bytes long.");
        let x = f32::from_le_bytes(source[0..4].try_into().unwrap());
        let y = f32::from_le_bytes(source[4..8].try_into().unwrap());
        let z = f32::from_le_bytes(source[8..12].try_into().unwrap());
        Ok(Vec3 { x, y, z })
    }
}

struct BleIdentifiers {
    service_uuid: Uuid,
    char_accel_uuid: Uuid,
    char_gyro_uuid: Uuid,
    char_temp_uuid: Uuid,
}

/// Parse the [`BLUETOOTH_UUID_HEADER`] string, and get all the UUIDs out of it.
fn parse_header_for_uuids() -> BleIdentifiers {
    // Split the file into lines, find only those starting with `#define`
    let defines: BTreeMap<&str, &str> = BLUETOOTH_UUID_HEADER
        .lines()
        .filter_map(|l| l.strip_prefix("#define ")) // Find all the #define lines
        .filter_map(|l| l.split_once(' ')) // Split them after the define name
        .map(|(name, val)| (name, val.trim_matches('"'))) // Remove quotes from values
        .collect();

    // Parse the UUIDs
    let service_uuid: Uuid = defines
        .get("BLE_SERVICE_UUID")
        .expect("BLE_SERVICE_UUID not specified")
        .parse()
        .expect("Failed to parse BLE_SERVICE_UUID");
    let char_accel_uuid: Uuid = defines
        .get("BLE_CHAR_ACCEL_UUID")
        .expect("BLE_CHAR_ACCEL_UUID not specified")
        .parse()
        .expect("Failed to parse BLE_CHAR_ACCEL_UUID");
    let char_gyro_uuid: Uuid = defines
        .get("BLE_CHAR_GYRO_UUID")
        .expect("BLE_CHAR_GYRO_UUID not specified")
        .parse()
        .expect("Failed to parse BLE_CHAR_GYRO_UUID");
    let char_temp_uuid: Uuid = defines
        .get("BLE_CHAR_TEMP_UUID")
        .expect("BLE_CHAR_TEMP_UUID not specified")
        .parse()
        .expect("Failed to parse BLE_CHAR_TEMP_UUID");

    BleIdentifiers {
        service_uuid,
        char_accel_uuid,
        char_gyro_uuid,
        char_temp_uuid,
    }
}
