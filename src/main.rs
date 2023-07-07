//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#![deny(warnings, clippy::nursery, clippy::cargo, unused_extern_crates)]

#[cfg(target_os = "windows")]
const APPLE_BOOT_VAR_GUID: &str = "{7C436110-AB2A-4BBB-A880-FE41995C9F82}";
#[cfg(any(target_os = "windows", target_os = "macos"))]
const AAPL_PANIC_INFO: &str = "aapl,panic-info";

#[cfg(target_os = "windows")]
fn read_from_nvram() -> Option<Vec<u8>> {
    use windows::{
        core::PCSTR, Win32::System::WindowsProgramming::GetFirmwareEnvironmentVariableA,
    };
    let lpname = PCSTR::from_raw(AAPL_PANIC_INFO.as_ptr());
    let lpguid = PCSTR::from_raw(APPLE_BOOT_VAR_GUID.as_ptr());
    let mut buf = vec![0u8; 512];
    let size = unsafe {
        GetFirmwareEnvironmentVariableA(lpname, lpguid, Some(buf.as_mut_ptr().cast()), size)
    };
    if size == 0 {
        return None;
    }
    buf.truncate(size as usize);
    Some(buf)
}

#[cfg(target_os = "macos")]
fn read_from_nvram() -> Option<Vec<u8>> {
    let output = std::process::Command::new("nvram")
        .arg("-x")
        .arg(AAPL_PANIC_INFO)
        .output()
        .ok()?;
    if !output.status.success() {
        return None;
    }
    let d: plist::Dictionary = plist::from_bytes(&output.stdout).ok()?;
    d.get(AAPL_PANIC_INFO)
        .and_then(|v| v.as_data())
        .map(|v| v.to_vec())
}

#[cfg(all(not(target_os = "windows"), not(target_os = "macos")))]
const fn read_from_nvram() -> Option<Vec<u8>> {
    None
}

fn main() {
    let data = std::env::args()
        .nth(1)
        .and_then(|v| std::fs::read(v).ok())
        .or_else(read_from_nvram)
        .expect("No aapl,panic-info data");
    let mut s = String::new();
    let mut prev = 0;
    let mut low = 0;
    let mut bit = 0;
    let mut add_char = |c: u8| {
        let v = match (prev, c) {
            (_, b'>') => "com.apple.driver.",
            (_, b'|') => "com.apple.iokit.",
            (_, b'$') => "com.apple.security.",
            (_, b'@') => "com.apple.",
            (b'!', b'A') => "Apple",
            (b'!', b'a') => "Action",
            (b'!', b'B') => "Bluetooth",
            (b'!', b'C') => "Controller",
            (b'!', b'F') => "Family",
            (b'!', b'I') => "Intel",
            (b'!', b'P') => "Profile",
            (b'!', b'S') => "Storage",
            (b'!', b'U') => "AppleUSB",
            _ => {
                prev = c;
                s.push(char::from(c));
                return;
            }
        };
        if prev == b'!' {
            s.pop();
        }
        prev = 0;
        s.push_str(v);
    };
    for v in data {
        add_char(((v & (0x7F >> bit)) << bit) | low);
        low = v >> (7 - bit);
        if bit == 6 {
            add_char(low);
            low = 0;
            bit = 0;
            continue;
        }
        bit += 1;
    }
    println!("{s}");
}
