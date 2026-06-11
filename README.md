# PanicInfoReader

Command line utility for extracting/decompressing XNU kernel panics.

The PanicInfoReader project is licensed under the `Thou Shalt Not Profit License version 1.5`. See `LICENSE`

Supported sources: `aapl,panic-info` and `AAPL,PanicInfo-xxxx` NVRAM variables, plist with `aapl,panic-info` key, `.panic` and raw binary file.

For the NVRAM reading functionality, execute the utility without arguments.

Supports macOS, Windows, Linux and BSD.
