/*
 * Copyright 2020 Cypress Semiconductor Corporation
 */
/*
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 *********************************************************************************
 * \mainpage Overview
 *********************************************************************************
 * Cypress MQTT library provides an easy-to-use APIs for Cypress devices to connect with cloud MQTT brokers and perform MQTT publish and subscribe operations.
 *
 * \section section1 Library Components
 * This MQTT library consists of two components:
 *  1) AWS IoT Device SDK library
 *  2) Platform porting layer
 *
 * AWS IoT Device SDK library is an open source library and it uses the platform porting layer functions to perform MQTT operations. Hence, the application that
 * uses this library should invoke the AWS IoT Device SDK library APIs and the platform porting layer APIs to perform MQTT publish/subscribe operations.
 *
 * <b>Library link:</b> https://github.com/aws/aws-iot-device-sdk-embedded-C/tree/v4_beta
 *
 * \subsection section1_1 AWS IoT Device SDK C - v4.0.0
 *  This is an open source MQTT library developed and maintained by Amazon. This library implements the core MQTT protocol logic.
 *
 *  * "AWS IoT Device SDK C v4.0.0" library provides following features:
 *      * MQTT 3.1.1 client
 *      * Asynchronous API for MQTT operations.
 *      * Multithreaded API by default.
 *      * Complete separation of MQTT and network stack, allowing MQTT to run on top of any network stack.
 *      * Configurable memory allocation (static-only or dynamic). Memory allocation functions may also be set by the user.
 *      * MQTT persistent session support.
 *      * Supports Quality of Service (QoS) levels 0 and 1
 *      * Supports MQTT connection over both secured and non secured TCP connections.
 *
 * \subsection section1_2 Platform Porting Layer
 * This portable layer provides the APIs that are required for 'AWS IoT Device SDK library' to perform OS and Network related operations on the Cypress platform.
 * This layer contains APIs that are used for creating thread, mutex, timers, etc. And it also contains APIs to establish secure/non-secure socket connections with MQTT broker to send and receive MQTT messages.
 *
 * \section section_platforms Supported Platforms
 * This library and its features are supported on following Cypress platforms:
 * * [PSoC6 WiFi-BT Prototyping Kit (CY8CPROTO-062-4343W)](https://www.cypress.com/documentation/development-kitsboards/psoc-6-wi-fi-bt-prototyping-kit-cy8cproto-062-4343w)
 * * [PSoC 62S2 Wi-Fi BT Pioneer Kit (CY8CKIT-062S2-43012)](https://www.cypress.com/documentation/development-kitsboards/psoc-62s2-wi-fi-bt-pioneer-kit-cy8ckit-062s2-43012)
 *
 * \section section_dependencies Dependent libraries
 * This MQTT client library depends on the following libraries. Both these libraries are included by default.
 * * [Wi-Fi Middleware Core](https://github.com/cypresssemiconductorco/wifi-mw-core)
 * * [AWS IoT Device SDK MQTT library](https://github.com/aws/aws-iot-device-sdk-embedded-C/tree/v4_beta/libraries/standard/mqtt)
 *
 * \section section_quick_start Quick start
 * * A "reasonable amount of time" to wait for keep-alive responses from the MQTT broker is configured using IOT_MQTT_RESPONSE_WAIT_MS in [iot_config.h](./cyport/include/iot_config.h). This value may be adjusted to suit the use case and network environment.
 * * The reference [iot_config.h](./cyport/include/iot_config.h) that is bundled with this library provides the configurations required for [AWS IoT Device SDK](https://github.com/aws/aws-iot-device-sdk-embedded-C/tree/v4_beta/libraries/standard/mqtt)
 * library. The application must copy this file to the root directory where the application Makefile is present, and suitably adjust the default settings.
 * * This MQTT client library does not support secure connections to the public test.mosquitto.org broker by default, as the server uses SHA1 hashing algorithm. As cautioned by mbedTLS, SHA-1 is considered a weak message digest and is therefore not enabled by default by mbedTLS. The use of SHA-1 for certificate signing constitutes a security risk. It is recommended to avoid dependencies on it, and consider stronger message digests instead.
 * However, if it is desired to connect securely to test.mosquitto.org, follow these steps to enable support:
 *   1. Define the below macro in your application:
 *        \code CY_MQTT_ENABLE_SECURE_TEST_MOSQUITTO_SUPPORT  \endcode
 *   2. Enable SHA1 support in mbedTLS by defining the below macro in wifi-mw-core/configs/mbedtls_user_config.h:
 *        \code #define MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES  \endcode
 * * A set of pre-defined configuration files have been bundled with wifi-mw-core library for FreeRTOS, LwIP and MbedTLS. The developer is expected to review the configuration and make adjustments. Refer to the Quick start section in <a href="https://github.com/cypresssemiconductorco/wifi-mw-core/blob/master/README.md">README.md</a>
 * for more details
 * * A set of COMPONENTS have to be defined in the code example project's Makefile for MQTT library. Refer to the Quick start section in <a href="https://github.com/cypresssemiconductorco/wifi-mw-core/blob/master/README.md">README.md</a> for more details
 * * Configure the following macros defined in cyport/include/iot_config.h file to enable/disable debug logs in this library:
 *   \code
 *    #define IOT_LOG_LEVEL_GLOBAL                        IOT_LOG_ERROR
 *    #define IOT_LOG_LEVEL_DEMO                          IOT_LOG_ERROR
 *    #define IOT_LOG_LEVEL_PLATFORM                      IOT_LOG_ERROR
 *    #define IOT_LOG_LEVEL_NETWORK                       IOT_LOG_ERROR
 *    #define IOT_LOG_LEVEL_TASKPOOL                      IOT_LOG_ERROR
 *    #define IOT_LOG_LEVEL_MQTT                          IOT_LOG_ERROR
 *    #define AWS_IOT_LOG_LEVEL_SHADOW                    IOT_LOG_ERROR
 *    #define AWS_IOT_LOG_LEVEL_DEFENDER                  IOT_LOG_ERROR
 *    #define AWS_IOT_LOG_LEVEL_JOBS                      IOT_LOG_ERROR
 *   \endcode
 ********************************************************************************
 * \section section_code_snippet Code Snippets
 ********************************************************************************
 ********************************************************************************
 * \subsection snip1 Code Snippet 1: Initialize MQTT and dependent libraries
 *  This code snippet demonstrates the initialization of the various libraries like the AWS IoT device SDK, Cypress secure sockets, and the AWS IoT MQTT.
 * \snippet doxygen_mqtt_code_snippet.h snippet_mqtt_init
 * \subsection snip2 Code Snippet 2: Non Secure connection to MQTT broker
 * This code snippet demonstrates the initialization of configuration structures required for a non-secure MQTT connection and the usage of IotMqtt_Connect() API.
 * \snippet doxygen_mqtt_code_snippet.h snippet_mqtt_connection
 * \subsection snip3 Code Snippet 3: Secure connection to MQTT broker
 * This code snippet demonstrates the initialization of configuration structures required for a secure MQTT connection and the usage of IotMqtt_Connect() API.
 * \snippet doxygen_mqtt_code_snippet.h snippet_mqtt_secure_connection
 * \subsection snip4 Code Snippet 4: Operation completion callback
 * MQTT operation completion callback function. Checks for valid parameters and unblocks the waiting thread.
 * \snippet doxygen_mqtt_code_snippet.h mqttOperationCallback
 * \subsection snip5 Code Snippet 5: MQTT message publish
 * This code snippet demonstrates the initialization of the publish message information structure and the usage of IotMqtt_PublishAsync() API.
 * \snippet doxygen_mqtt_code_snippet.h snippet_mqtt_publish
 * \subsection snip6 Code Snippet 6: Subscribed message received callback
 * Callback to handle incoming MQTT messages from the broker.
 * \snippet doxygen_mqtt_code_snippet.h mqttSubscriptionCallback
 * \subsection snip7 Code Snippet 7: MQTT message subscribe
 * This code snippet demonstrates the initialization of the subscription information structure and the usage of IotMqtt_SubscribeAsync() API.
 * \snippet doxygen_mqtt_code_snippet.h snippet_mqtt_subscribe
 ********************************************************************************
 * \defgroup mqtt_api_call_sequence API Call Sequence
 * \brief This section provides the details of the API call sequence for performing various MQTT operations. This call sequence uses APIs defined by AWS IoT Device SDK library and the Cypress Platform Port layer.
 *
 * Refer to \ref  mqtt_aws_iot_device_sdk_function and \ref mqtt_cyport sections for the API details.
 * @ingroup mqtt_functions
 * \section section2_1 For Connect, Subscribe and Publish
 * The API call sequence given below should be used to connect to a MQTT broker and to perform MQTT subscribe and publish operations:
 *
 *                         +-----------------+
 *                         |  IotSdk_Init()  |
 *                         +-----------------+
 *                                  |
 *                                  v
 *                 +----------------------------------+
 *                 |  IotNetworkSecureSockets_Init()  |
 *                 +----------------------------------+
 *                                  |
 *                                  v
 *                        +------------------+
 *                        |  IotMqtt_Init()  |
 *                        +------------------+
 *                                  |
 *                                  v
 *                       +---------------------+
 *                       |  IotMqtt_Connect()  |
 *                       +---------------------+
 *                                  |
 *                                  v
 *                      +-----------------------+
 *                      |  IotMqtt_Subscribe()  |
 *                      +-----------------------+
 *                                  |
 *                                  v
 *                    +--------------------------+
 *                    |  IotMqtt_PublishAsync()  |
 *                    +--------------------------+
 *
 *
 * \section section2_2 For UnSubscribe, Disconnect and DeInit
 * The API call sequence given below should be used to perform MQTT unsubscribe and disconnect operations with the MQTT broker:
 *
 *                    +-------------------------+
 *                    |  IotMqtt_Unsubscribe()  |
 *                    +-------------------------+
 *                                  |
 *                                  v
 *                    +-------------------------+
 *                    |  IotMqtt_Disconnect()   |
 *                    +-------------------------+
 *                                  |
 *                                  v
 *                      +---------------------+
 *                      |  IotMqtt_Cleanup()  |
 *                      +---------------------+
 *                                  |
 *                                  v
 *               +-------------------------------------+
 *               |  IotNetworkSecureSockets_Cleanup()  |
 *               +-------------------------------------+
 *                                  |
 *                                  v
 *                       +--------------------+
 *                       |  IotSdk_Cleanup()  |
 *                       +--------------------+
 *
 * \defgroup mqtt_aws_iot_device_sdk_function AWS IoT Device SDK
 * \brief This SDK provides MQTT 3.1.1 client library implementation.
 * \note  <b>MQTT Definition:</b>\n
 *   MQTT stands for MQ Telemetry Transport. It is a publish/subscribe, extremely simple and lightweight messaging protocol, designed for constrained devices and low-bandwidth, high-latency or unreliable networks. The design principles are to minimise network bandwidth and device resource requirements whilst also attempting to ensure reliability and some degree of assurance of delivery. These principles also turn out to make the protocol ideal of the emerging "machine-to-machine" (M2M) or "Internet of Things" world of connected devices, and for mobile applications where bandwidth and battery power are at a premium.
 *   This MQTT library implements a subset of the MQTT 3.1.1 standard.
 *
 * Features of this library include:
 * * Both fully asynchronous and blocking API functions.
 * * Scalable performance and footprint. The configuration settings allow this library to be tailored to a system's resources.
 *
 * Refer to AWS IoT Device SDK MQTT documentation for the API details:
 *       https://docs.aws.amazon.com/freertos/latest/lib-ref/c-sdk/mqtt/mqtt_functions.html
 * @ingroup mqtt_functions
 *
 * \defgroup mqtt_cyport Platform Port Layer
 * \brief Cypress platform port layer functions and configurations.
 * @ingroup mqtt_functions
 *
 * \defgroup mqtt_cyport_function Functions
 * \brief Cypress platform port layer functions.\n
 * This layer provides set of portable layer functions that are required for AWS IoT Device SDK library to perform RTOS and socket operations. Those are:\n
 *  1. Thread functions
 *  2. Timer functions
 *  3. RTOS synchronization primitive functions
 *  4. Socket I/O functions to connect and transfer data over the network\n
 * The threads that are created by AWS IoT Device SDK library use priority \ref IOT_THREAD_DEFAULT_PRIORITY.
 *
 * @ingroup mqtt_cyport
 *
 * \defgroup mqtt_cyport_config Configurations
 * \brief Cypress platform port layer configurations.
 * @ingroup mqtt_cyport
 *
 */

/**
 * @file cy_iot_network_secured_socket.h
 * @brief This file declares the callback functions that are required by iot network abstraction layer
 * (specified in iot_network.h) for AWS IoT device SDK to work on Cypress platforms.
 */

#ifndef CY_IOT_NETWORK_SECURED_SOCKET_H_
#define CY_IOT_NETWORK_SECURED_SOCKET_H_

/* The config header is always included first. */
#include "iot_config.h"

/* Platform network include. */
#include "iot_network.h"

#include "cy_secure_sockets.h"
#include "cy_tls.h"

/**
 * @brief Provides a default value for an #IotNetworkServerInfo.
 *
 * All instances of #IotNetworkServerInfo should be initialized with
 * this constant when using Cypress secure sockets implementation.
 *
 * @warning Failing to initialize an #IotNetworkServerInfo may result in
 * a crash!
 * @note This initializer may change at any time in future versions, but its
 * name will remain the same.
 */
#define IOT_NETWORK_SERVER_INFO_CY_SECURE_SOCKETS_INITIALIZER       { 0 }

/**
 * @brief This is the ALPN (Application-Layer Protocol Negotiation) string
 * required for password-based authentication to the MQTT broker,
 * TCP port 443, and Cypress secure sockets implementation.
 */
#define IOT_PASSWORD_ALPN_FOR_CY_SECURE_SOCKETS                     ""

/**
 * @brief Generic initializer for an #IotNetworkCredentials when using this
 * Cypress secure sockets implementation.
 *
 * @note This initializer may change at any time in future versions, but its
 * name will remain the same.
 */
#define IOT_NETWORK_CREDENTIALS_CY_SECURE_SOCKETS_INITIALIZER       { 0 }

/**
 * @brief Provides a pointer to an #IotNetworkInterface_t that uses the functions
 * declared in this file.
 */
#define IOT_NETWORK_INTERFACE_CY_SECURE_SOCKETS                     ( IotNetworkSecureSockets_GetInterface() )

/**
 * @brief Retrieve the network interface using the functions in this file.
 * @return Network interface handle.
 */
const IotNetworkInterface_t * IotNetworkSecureSockets_GetInterface( void );

/**
 * @addtogroup mqtt_cyport_function
 * @{
 */
/**
 * @brief One-time initialization function for Cypress secure sockets implementation.
 *
 * This function performs internal setup of this network stack. <b>It must be
 * called once (and only once) before calling any other function in this network
 * stack</b>. Calling this function more than once without first calling
 * #IotNetworkSecureSockets_Cleanup may result in a crash.
 *
 * @return IOT_NETWORK_SUCCESS or IOT_NETWORK_FAILURE.
 *
 * @warning No thread-safety guarantees are provided for this function.
 */
IotNetworkError_t IotNetworkSecureSockets_Init( void );

/**
 * @brief One-time deinitialization function for Cypress secure sockets implementation.
 *
 * This function frees resources taken in #IotNetworkSecureSockets_Init. It should be
 * called after destroying all network connections to clean up this network
 * stack. After this function returns, #IotNetworkSecureSockets_Init must be called
 * again before calling any other function in this network stack.
 *
 * @warning No thread-safety guarantees are provided for this function. Do not
 * call this function if any network connections exist!
 */
void IotNetworkSecureSockets_Cleanup( void );

/**
 * @}
 */


/**
 * @brief An implementation of #IotNetworkInterface_t::create for Cypress secure sockets implementation.
 */
IotNetworkError_t IotNetworkSecureSockets_Create( IotNetworkServerInfo_t pServerInfo,
                                            IotNetworkCredentials_t pCredentialInfo,
                                            IotNetworkConnection_t * pConnection );

/**
 * @brief An implementation of #IotNetworkInterface_t::setReceiveCallback for
 * Cypress secure sockets implementation.
 */
IotNetworkError_t IotNetworkSecureSockets_SetReceiveCallback( IotNetworkConnection_t pConnection,
                                                        IotNetworkReceiveCallback_t receiveCallback,
                                                        void * pContext );

/**
 * @brief An implementation of #IotNetworkInterface_t::setCloseCallback for
 * Cypress secure sockets implementation.
 */
IotNetworkError_t IotNetworkSecureSockets_SetCloseCallback( IotNetworkConnection_t pConnection,
                                                      IotNetworkCloseCallback_t closeCallback,
                                                      void * pContext );

/**
 * @brief An implementation of #IotNetworkInterface_t::send for Cypress secure sockets implementation.
 */
size_t IotNetworkSecureSockets_Send( IotNetworkConnection_t pConnection,
                               const uint8_t * pMessage,
                               size_t messageLength );

/**
 * @brief An implementation of #IotNetworkInterface_t::receive for Cypress secure sockets implementation.
 */
size_t IotNetworkSecureSockets_Receive( IotNetworkConnection_t pConnection,
                                  uint8_t * pBuffer,
                                  size_t bytesRequested );

/**
 * @brief An implementation of #IotNetworkInterface_t::close for Cypress secure sockets implementation.
 */
IotNetworkError_t IotNetworkSecureSockets_Close( IotNetworkConnection_t pConnection );

/**
 * @brief An implementation of #IotNetworkInterface_t::destroy for Cypress secure sockets implementation.
 */
IotNetworkError_t IotNetworkSecureSockets_Destroy( IotNetworkConnection_t pConnection );

/**
 * @brief Used by metrics to retrieve the socket (file descriptor) associated with
 * a connection.
 */
int IotNetworkSecureSockets_GetSocket( IotNetworkConnection_t pConnection );

#endif /* ifndef CY_IOT_NETWORK_SECURED_SOCKET_H_ */
