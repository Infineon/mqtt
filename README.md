# MQTT Client Library

This repo contains the MQTT client library that can work with the family of Cypress connectivity devices. This library uses the AWS IoT Device SDK MQTT Client library and implements the glue layer that is required for the library to work with Cypress connectivity platforms.

The ModusToolbox® MQTT Client code examples download this library automatically, so you don't need to.

## Features

All features supported by the [AWS IoT Device SDK MQTT Library](https://github.com/aws/aws-iot-device-sdk-embedded-C/tree/202011.00) are supported by this library.

Some of the key features include:

- MQTT 3.1.1 Client.

- Synchronous API for MQTT operations.

- Multi-threaded API by default.

- Complete separation of MQTT and network stack, allowing MQTT to run on top of any network stack.

- MQTT persistent session support.

- Supports Quality of Service (QoS) levels 0, 1, and 2.

- Supports MQTT connection over both secured and non-secured TCP connections.

- Glue layer implementation for MQTT library to work on Cypress connectivity platforms.

- Built on top of FreeRTOS, lwIP, and Mbed TLS (that are bundled as part of Wi-Fi Middleware Core library).

**Note:** Using this library in a project will cause the AWS IoT Device SDK to be downloaded on your computer. It is your responsibility to understand and accept the AWS IoT Device SDK license.

## Supported Platforms

- [PSoC 6 Wi-Fi BT Prototyping Kit (CY8CPROTO-062-4343W)](https://www.cypress.com/documentation/development-kitsboards/psoc-6-wi-fi-bt-prototyping-kit-cy8cproto-062-4343w)

- [PSoC 62S2 Wi-Fi BT Pioneer Kit (CY8CKIT-062S2-43012)](https://www.cypress.com/documentation/development-kitsboards/psoc-62s2-wi-fi-bt-pioneer-kit-cy8ckit-062s2-43012)

- [PSoC 6 WiFi-BT Pioneer Kit (CY8CKIT-062-WiFi-BT)](https://www.cypress.com/documentation/development-kitsboards/psoc-6-wifi-bt-pioneer-kit-cy8ckit-062-wifi-bt)

## Dependent Libraries

This MQTT Client library depends on the following libraries. These libraries are included by default.

- [Wi-Fi Middleware Core](https://github.com/cypresssemiconductorco/wifi-mw-core)

- [AWS IoT Device SDK Port](https://github.com/cypresssemiconductorco/aws-iot-device-sdk-port)

- [AWS IoT Device SDK MQTT library](https://github.com/aws/aws-iot-device-sdk-embedded-C/tree/202011.00)

## Quick Start

A "reasonable amount of time" to wait for keepalive responses from the MQTT broker is configured using `MQTT_PINGRESP_TIMEOUT_MS` in *./include/core_mqtt_config.h*. This value may be adjusted to suit the use case and network environment.

1. The reference *./include/core_mqtt_config.h* file that is bundled with this library provides the configurations required for the [AWS IoT Device SDK](https://github.com/aws/aws-iot-device-sdk-embedded-C/tree/202011.00) library. The application must copy this file to the root directory where the application Makefile is present, and suitably adjust the default settings.

2. This MQTT Client library does not support secure connections to the public `test.mosquitto.org` broker by default because the server uses the SHA1 hashing algorithm. As cautioned by Mbed TLS, SHA-1 is considered a weak message digest and is therefore not enabled in Mbed TLS by default. The use of SHA-1 for certificate signing constitutes a security risk. It is recommended to avoid dependencies on it, and consider stronger message digests instead.

3. A set of pre-defined configuration files have been bundled with the wifi-mw-core library for FreeRTOS, lwIP, and Mbed TLS. You should review the configuration and make the required adjustments. See the "Quick Start" section in [README.md](https://github.com/cypresssemiconductorco/wifi-mw-core/blob/master/README.md) for more details.

4. Define the following COMPONENTS in the application's makefile for the MQTT Library. For additional information, see the "Quick Start" section in [README.md](https://github.com/cypresssemiconductorco/wifi-mw-core/blob/master/README.md).
   ```
   COMPONENTS=FREERTOS MBEDTLS LWIP SECURE_SOCKETS
   ```

5. The "aws-iot-device-sdk-port" layer includes the "coreHTTP" and "coreMQTT" modules of the "aws-iot-device-sdk-embedded-C" library by default. If the user application doesn't use HTTP client features, add the following path in the .cyignore file of the application to exclude the coreHTTP source files from the build.
   ```
   $(SEARCH_aws-iot-device-sdk-embedded-C)/libraries/standard/coreHTTP
   libs/aws-iot-device-sdk-embedded-C/libraries/standard/coreHTTP
   ```

6. The MQTT Library disables all debug log messages by default. To enable log messages, the application must perform the following:

   1. Add the `ENABLE_MQTT_LOGS` macro to the *DEFINES* in the code example's Makefile. The Makefile entry would look like as follows:
     ```
       DEFINES+=ENABLE_MQTT_LOGS
     ```
   2. Call the `cy_log_init()` function provided by the *cy-log* module. cy-log is part of the *connectivity-utilities* library. See [connectivity-utilities library API documentation](https://cypresssemiconductorco.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html) for cy-log details.

## Library Usage Notes

- Functions `cy_mqtt_init()` and `cy_mqtt_deinit()` are not thread-safe.

- The application should allocate a network buffer; the same buffer must be passed while creating the MQTT object. The MQTT Library uses this buffer for sending and receiving MQTT packets. `CY_MQTT_MIN_NETWORK_BUFFER_SIZE` is the minimum size of the network buffer required for the library to handle MQTT packets. The `CY_MQTT_MIN_NETWORK_BUFFER_SIZE` is defined in *./include/cy_mqtt_api.h* .

- The application should call `cy_mqtt_init()` before calling any other MQTT library function, and should not call other MQTT library functions after calling `cy_mqtt_deinit()`.

- An MQTT instance is created using the `cy_mqtt_create` API function. The MQTT library allocates resources for each created instance; therefore, the application should delete the MQTT instance by calling `cy_mqtt_delete()` after performing the desired MQTT operations.

- After calling the `cy_mqtt_delete` API function, all the resource associated with the MQTT handle are freed. Therefore, you should not use the same MQTT handle after delete.

- The MQTT disconnection event notification is not sent to the application when disconnection is initiated from the application.

- When the application receives an MQTT disconnection event notification due to physical network disconnection or not receiving the ping response on time, the application should call the `cy_mqtt_disconnect()` function to release the resource allocated during `cy_mqtt_connect()`.

## Additional Information

- [MQTT Client Library RELEASE.md](./RELEASE.md)

- [MQTT Client API Documentation](https://cypresssemiconductorco.github.io/mqtt/api_reference_manual/html/index.html)

- [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.cypress.com/products/modustoolbox-software-environment)

- [AWS-IoT Device SDK Library](https://github.com/aws/aws-iot-device-sdk-embedded-C/tree/202011.00)

- [ModusToolbox AnyCloud code examples](https://github.com/cypresssemiconductorco?q=mtb-example-anycloud%20NOT%20Deprecated)
