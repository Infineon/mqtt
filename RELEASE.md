# Cypress MQTT Client Library

## What's Included?

Refer to the [README.md](./README.md) for a complete description of the MQTT Client Library

## Known Issues
| Problem | Workaround |
| ------- | ---------- |
| The MQTT library version v3.x doesn’t support asynchronous APIs. Hence, it uses a thread based polling logic to check for incoming MQTT packets. The same thread is used for sending a periodic PING mqtt message to broker. This may wake-up the host MCU periodically in deep-sleep mode, which could impact low-power system use case. | Use MQTT v2.x for low-power use case. |


## Changelog
### v3.1.1
- Documentation updates.

- Upgraded the library to integrate with wifi-mw-core 3.x library version for AnyCloud.

- Introduced ARMC6 compiler support for AnyCloud build.

### v3.1.0
- Performance improvements.

- Documentation updates.

### v3.0.0
- Supports QoS-2.

- Fully compliant with MQTT spec v3.1.1.

- Provides simplified C APIs.

- Integrated with the latest AWS IoT Device C SDK library version #202011.00.

- This version of the library is not backward compatible with pervious library versions.

### v2.1.0
-  Introduced asynchronous receive logic.

-  Added support for SAS token based authentication, for Azure borker connection.

### v2.0.0

- Changes to adapt to ModusToolbox® 2.2.0 flow and AnyCloud's support for multiple Wi-Fi interfaces - STA, softAP, and concurrent STA+softAP mode.

### v1.0.1

- Code snippets added to the documentation.

### v1.0.0

- Initial release of the MQTT Client Library.

### Supported Software and Tools

This version of the library was validated for compatibility with the following software and tools:

| Software and Tools                                      | Version |
| :---                                                    | :----:  |
| ModusToolbox Software Environment                       | 2.3     |
| - ModusToolbox Device Configurator                      | 3.0     |
| - ModusToolbox CapSense Configurator / Tuner tools      | 3.15    |
| PSoC 6 Peripheral Driver Library (PDL)                  | 2.2.0   |
| GCC Compiler                                            | 9.3.1   |
| IAR Compiler                                            | 8.32    |
| Arm Compiler 6                                          | 6.14    |
