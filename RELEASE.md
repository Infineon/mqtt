# CYPRESS&trade; MQTT client library

## What's included?

See the [README.md](./README.md) for a complete description of the MQTT client library.

## Known issues

- None.

## Changelog

### v3.4.1

- Documentation update

### v3.4.0

- Added support for CY8CEVAL-062S2-MUR-43439M2 kit

### v3.3.0

- Added support for CYW943907AEVAL1F and CYW954907AEVAL1F kits

### v3.2.0

- Added support for secured kit (for example: CY8CKIT-064S0S2-4343W)

- Integrated with secure sockets PKCS support

- Added support for Low power assistant (LPA) applications

- Upgraded the library to integrate with the wifi-mw-core 3.1.1 library version

- General bug fixes


### v3.1.1
- Documentation updates

- Upgraded the library to integrate the with wifi-mw-core 3.x library version for AnyCloud

- Introduced ARMC6 compiler support for AnyCloud build


### v3.1.0
- Performance improvements

- Documentation updates


### v3.0.0
- Supports QoS-2

- Fully compliant with MQTT spec v3.1.1

- Provides simplified C APIs

- Integrated with the latest AWS IoT Device C SDK library version #202011.00

- This version of the library is not backward-compatible with pervious library versions


### v2.1.0
-  Introduced asynchronous receive logic

-  Added support for SAS token-based authentication for Azure broker connection


### v2.0.0

- Changes to adapt to ModusToolbox&trade; 2.2.0 flow and AnyCloud's support for multiple Wi-Fi interfaces - STA, softAP, and concurrent STA+softAP mode


### v1.0.1

- Code snippets added to the documentation


### v1.0.0

- Initial release of the MQTT client library


### Supported software and tools

This version of the library was validated for compatibility with the following software and tools:

| Software and tools                                             | Version |
| :---                                                           | :----:  |
| ModusToolbox&trade; software environment                       | 2.4     |
| - ModusToolbox&trade; device configurator                      | 3.10    |
| - ModusToolbox&trade; CAPSENSE&trade; configurator/tuner tools | 4.0     |
| PSoC&trade; 6 peripheral driver library (PDL)                  | 2.3.0   |
| GCC compiler                                                   | 10.3.1  |
| IAR compiler                                                   | 8.32    |
| Arm&reg; compiler 6                                            | 6.14    |
