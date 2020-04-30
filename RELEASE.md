# Cypress MQTT client library

## What's Included?
Refer to the [README.md](./README.md) for a complete description of the MQTT client library

## Known Issues
| Problem | Workaround |
| ------- | ---------- |
| The implementation of newlib from GCC will leak ~1.4kb of heap memory per task/thread that uses stdio functions (i.e. printf, snprintf, etc.) | By default, only error log messages are enabled in the MQTT client library. It is recommended to enable information or warning messages, only for debugging purposes |


## Changelog
### v1.0.0
* Initial release of MQTT library

### Supported Software and Tools
This version of the library was validated for compatibility with the following software and tools:

| Software and Tools                                      | Version |
| :---                                                    | :----:  |
| ModusToolbox Software Environment                       | 2.1     |
| - ModusToolbox Device Configurator                      | 2.1     |
| - ModusToolbox CSD Personality in Device Configurator   | 2.0     |
| - ModusToolbox CapSense Configurator / Tuner tools      | 3.0     |
| PSoC6 Peripheral Driver Library (PDL)                   | 1.5.0   |
| GCC Compiler                                            | 7.2.1   |
| IAR Compiler                                            | 8.32    |
