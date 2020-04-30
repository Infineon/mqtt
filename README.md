# MQTT Client Library
This repo contains the MQTT client library that can work with the family of Cypress connectivity devices. This library uses the AWS IoT Device SDK MQTT Client library and implements the glue layer that is required for the library to work with Cypress connectivity platforms.

The ModusToolbox™ MQTT Client code examples download this library automatically, so you don't need to.

## Features Supported
 
All features supported by the [AWS IoT Device SDK MQTT Library](https://github.com/aws/aws-iot-device-sdk-embedded-C/tree/v4_beta/libraries/standard/mqtt) are supported by this library. 

Some of the key features include:
- MQTT 3.1.1 Client.
- Asynchronous API for MQTT operations.
- Multi-threaded API by default.
- Complete separation of MQTT and network stack, allowing MQTT to run on top of any network stack.
- Configurable memory allocation (static-only or dynamic). Memory allocation functions may also be set by the user.
- MQTT persistent session support.
- Supports Quality of Service (QoS) levels 0 and 1.
- Supports MQTT connection over both secured and non-secured TCP connections.
- Glue layer implementation for MQTT library to work on Cypress connectivity platforms.
- Built on top of FreeRTOS, LwIP, and Mbed TLS (that are bundled as part of Wi-Fi Middleware Core library).

**Note**: Using this library in a project will cause AWS IoT Device SDK to be downloaded on your computer.  It is your responsibility to understand and accept the AWS IoT Device SDK license.

## Supported Platforms
-  [PSoC 6 Wi-Fi BT Prototyping Kit (CY8CPROTO-062-4343W)](https://www.cypress.com/documentation/development-kitsboards/psoc-6-wi-fi-bt-prototyping-kit-cy8cproto-062-4343w)

- [PSoC 62S2 Wi-Fi BT Pioneer Kit (CY8CKIT-062S2-43012)](https://www.cypress.com/documentation/development-kitsboards/psoc-62s2-wi-fi-bt-pioneer-kit-cy8ckit-062s2-43012)

## Dependent Libraries
This MQTT Client library depends on the following libraries. Both these libraries are included by default.

- [Wi-Fi Middleware Core](https://github.com/cypresssemiconductorco/wifi-mw-core)

- [AWS IoT Device SDK MQTT library](https://github.com/aws/aws-iot-device-sdk-embedded-C/tree/v4_beta/libraries/standard/mqtt)

## Quick Start
A "reasonable amount of time" to wait for keepalive responses from the MQTT broker is configured using `IOT_MQTT_RESPONSE_WAIT_MS` in *./cyport/include/iot_config.h*. This value may be adjusted to suit the use case and network environment.

- The reference *./cyport/include/iot_config.h* file that is bundled with this library provides the configurations required for the [AWS IoT Device SDK](https://github.com/aws/aws-iot-device-sdk-embedded-C/tree/v4_beta/libraries/standard/mqtt) library. The application must copy this file to the root directory where the application Makefile is present, and suitably adjust the default settings.

- This MQTT Client library does not support secure connections to the public `test.mosquitto.org` broker by default, because the server uses the SHA1 hashing algorithm. As cautioned by Mbed TLS, SHA-1 is considered a weak message digest and is therefore not enabled in Mbed TLS by default. The use of SHA-1 for certificate signing constitutes a security risk. It is recommended to avoid dependencies on it, and consider stronger message digests instead.

   However, if it is required to connect securely to `test.mosquitto.org`, do the following to enable support:

  1. Define the following macro in your application:  

     ```
     CY_MQTT_ENABLE_SECURE_TEST_MOSQUITTO_SUPPORT  
     ```
  2. Enable SHA1 support in Mbed TLS by defining the following macro in *wifi-mw-core/configs/mbedtls_user_config.h*:

     ```
     #define MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES  
     ```
- A set of pre-defined configuration files have been bundled with wifi-mw-core library for FreeRTOS, LwIP, and Mbed TLS. You should review the configuration and make the required adjustments. See the "Quick Start" section in [README.md](https://github.com/cypresssemiconductorco/wifi-mw-core/blob/master/README.md) for more details.

- A set of COMPONENTS have to be defined in the application's Makefile for MQTT library. See the "Quick Start" section in [README.md](https://github.com/cypresssemiconductorco/wifi-mw-core/blob/master/README.md) for more details.

- Configure the following macros defined in the *cyport/include/iot_config.h* file to enable/disable debug logs in this library:

  ```
  #define IOT_LOG_LEVEL_GLOBAL                        IOT_LOG_ERROR   
  #define IOT_LOG_LEVEL_DEMO                          IOT_LOG_ERROR   
  #define IOT_LOG_LEVEL_PLATFORM                      IOT_LOG_ERROR   
  #define IOT_LOG_LEVEL_NETWORK                       IOT_LOG_ERROR  
  #define IOT_LOG_LEVEL_TASKPOOL                      IOT_LOG_ERROR  
  #define IOT_LOG_LEVEL_MQTT                          IOT_LOG_ERROR  
  #define AWS_IOT_LOG_LEVEL_SHADOW                    IOT_LOG_ERROR  
  #define AWS_IOT_LOG_LEVEL_DEFENDER                  IOT_LOG_ERROR  
  #define AWS_IOT_LOG_LEVEL_JOBS                      IOT_LOG_ERROR  
  ```

## Additional Information
- [MQTT Client Library RELEASE.md](./RELEASE.md)

- [MQTT Client API Documentation](https://cypresssemiconductorco.github.io/mqtt/api_reference_manual/html/index.html)

- [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.cypress.com/products/modustoolbox-software-environment)

- [AWS-IoT Device SDK Library](https://github.com/aws/aws-iot-device-sdk-embedded-C/tree/v4_beta)

- [ModusToolbox AnyCloud code examples](https://github.com/cypresssemiconductorco?q=mtb-example-anycloud%20NOT%20Deprecated)

