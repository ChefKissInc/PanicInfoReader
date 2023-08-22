//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#![deny(warnings, clippy::nursery, clippy::cargo, unused_extern_crates)]

#[cfg(target_os = "windows")]
#[macro_use]
extern crate windows;

#[cfg(target_os = "windows")]
fn read_from_nvram() -> Option<Vec<u8>> {
    use windows::Win32::System::WindowsProgramming::GetFirmwareEnvironmentVariableA;
    let name = w!("aapl,panic-info");
    let guid = w!("{7C436110-AB2A-4BBB-A880-FE41995C9F82}");
    let mut buf = vec![0u8; 65476];
    let size = unsafe {
        GetFirmwareEnvironmentVariableW(name, guid, Some(buf.as_mut_ptr().cast()), 65476)
    };
    if size == 0 {
        let err = unsafe { GetLastError() };
        let err_string = 'a: {
            let mut buff = [0u16; 256];
            let buff_size = 256;

            let chars_copied = unsafe {
                FormatMessageW(
                    FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | 8192,
                    core::ptr::null(),
                    err,
                    0,
                    PWSTR::from_raw(buff.as_mut_ptr()),
                    (buff_size + 1) as u32,
                    core::ptr::null_mut(),
                )
            };

            if chars_copied == 0 {
                break 'a None;
            }

            let mut curr_char = chars_copied as usize;
            while curr_char > 0 {
                let ch = buff[curr_char];

                if ch >= ' ' as u16 {
                    break;
                }
                curr_char -= 1;
            }

            String::from_utf16(&buff).ok()
        };
        eprintln!("Error: {}", err_string.unwrap_or_else(|| err.to_string()));
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
