// Copyright © 2023-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#![deny(warnings, clippy::nursery, clippy::cargo, unused_extern_crates)]

use std::process::ExitCode;

#[cfg(target_os = "windows")]
fn read_from_nvram() -> Option<Vec<u8>> {
    use windows::{core::w, Win32::System::WindowsProgramming::GetFirmwareEnvironmentVariableW};
    let name = w!("aapl,panic-info");
    let guid = w!("{7C436110-AB2A-4BBB-A880-FE41995C9F82}");
    let mut buf = vec![0u8; 65476];
    let size = unsafe {
        GetFirmwareEnvironmentVariableW(name, guid, Some(buf.as_mut_ptr().cast()), 65476)
    };
    if size == 0 {
        let err = windows::core::Error::from_win32();
        eprintln!("Failed to read panic info from NVRAM: {}", err.message());
        return None;
    }
    buf.truncate(size as usize);
    Some(buf)
}

#[cfg(target_os = "macos")]
fn read_from_nvram() -> Option<Vec<u8>> {
    let output = std::process::Command::new("nvram")
        .arg("-x")
        .arg("aapl,panic-info")
        .output()
        .ok()?;
    if !output.status.success() {
        if let Ok(err_msg) = std::str::from_utf8(&output.stderr) {
            eprintln!("Failed to read panic info from NVRAM: {}", err_msg.trim());
        }
        return None;
    }
    let d: plist::Dictionary = plist::from_bytes(&output.stdout).ok()?;
    d.get("aapl,panic-info")
        .and_then(|v| v.as_data())
        .map(|v| v.to_vec())
}

#[cfg(all(not(target_os = "windows"), not(target_os = "macos")))]
const fn read_from_nvram() -> Option<Vec<u8>> {
    None
}

fn main() -> ExitCode {
    let data = if let Some(path) = std::env::args().nth(1) {
        match plist::from_file::<_, plist::Dictionary>(&path) {
            Ok(v) => {
                let Some(key) = v.keys().find(|v| v.to_lowercase() == "aapl,panic-info") else {
                    eprintln!("Plist specified does not contain panic info");
                    return ExitCode::FAILURE;
                };
                let Some(data) = v.get(key).unwrap().as_data() else {
                    eprintln!("Plist panic info key is not of type Data");
                    return ExitCode::FAILURE;
                };
                data.to_owned()
            }
            Err(e) => match std::fs::read(path) {
                Ok(v) => {
                    eprintln!("Warning: Failed to read file as plist: {e}");
                    eprintln!("Decompressing as raw data instead");
                    eprintln!();
                    v
                }
                Err(e) => {
                    eprintln!("Failed to read file: {e}");
                    return ExitCode::FAILURE;
                }
            },
        }
    } else {
        let Some(v) = read_from_nvram() else {
            return ExitCode::FAILURE;
        };
        v
    };
    let mut s = String::new();
    let mut expand_kext_info = false;
    let mut prev = 0;
    let mut low = 0;
    let mut bit = 0;
    let mut add_char = |c: u8| {
        if !expand_kext_info {
            expand_kext_info = s.ends_with("loaded kext");
            if !expand_kext_info {
                s.push(c.into());
                return;
            }
        }
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
                s.push(c.into());
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

    ExitCode::SUCCESS
}
