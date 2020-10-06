/*
 * Copyright 2020, Cypress Semiconductor Corporation or a subsidiary of
 * Cypress Semiconductor Corporation. All Rights Reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software"), is owned by Cypress Semiconductor Corporation
 * or one of its subsidiaries ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products. Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
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
 * @brief Provides a default value for an IotNetworkServerInfo.
 *
 * All instances of IotNetworkServerInfo should be initialized with
 * this constant when using Cypress secure sockets implementation.
 *
 * @warning Failing to initialize an IotNetworkServerInfo may result in
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
 * @brief Generic initializer for an IotNetworkCredentials when using this
 * Cypress secure sockets implementation.
 *
 * @note This initializer may change at any time in future versions, but its
 * name will remain the same.
 */
#define IOT_NETWORK_CREDENTIALS_CY_SECURE_SOCKETS_INITIALIZER       { 0 }

/**
 * @brief Provides a pointer to an IotNetworkInterface_t that uses the functions
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
 * @brief An implementation of IotNetworkInterface_t::create for Cypress secure sockets implementation.
 */
IotNetworkError_t IotNetworkSecureSockets_Create( IotNetworkServerInfo_t pServerInfo,
                                            IotNetworkCredentials_t pCredentialInfo,
                                            IotNetworkConnection_t * pConnection );

/**
 * @brief An implementation of IotNetworkInterface_t::setReceiveCallback for
 * Cypress secure sockets implementation.
 */
IotNetworkError_t IotNetworkSecureSockets_SetReceiveCallback( IotNetworkConnection_t pConnection,
                                                        IotNetworkReceiveCallback_t receiveCallback,
                                                        void * pContext );

/**
 * @brief An implementation of IotNetworkInterface_t::setCloseCallback for
 * Cypress secure sockets implementation.
 */
IotNetworkError_t IotNetworkSecureSockets_SetCloseCallback( IotNetworkConnection_t pConnection,
                                                      IotNetworkCloseCallback_t closeCallback,
                                                      void * pContext );

/**
 * @brief An implementation of IotNetworkInterface_t::send for Cypress secure sockets implementation.
 */
size_t IotNetworkSecureSockets_Send( IotNetworkConnection_t pConnection,
                               const uint8_t * pMessage,
                               size_t messageLength );

/**
 * @brief An implementation of IotNetworkInterface_t::receive for Cypress secure sockets implementation.
 */
size_t IotNetworkSecureSockets_Receive( IotNetworkConnection_t pConnection,
                                  uint8_t * pBuffer,
                                  size_t bytesRequested );

/**
 * @brief An implementation of IotNetworkInterface_t::close for Cypress secure sockets implementation.
 */
IotNetworkError_t IotNetworkSecureSockets_Close( IotNetworkConnection_t pConnection );

/**
 * @brief An implementation of IotNetworkInterface_t::destroy for Cypress secure sockets implementation.
 */
IotNetworkError_t IotNetworkSecureSockets_Destroy( IotNetworkConnection_t pConnection );

/**
 * @brief Used by metrics to retrieve the socket (file descriptor) associated with
 * a connection.
 */
int IotNetworkSecureSockets_GetSocket( IotNetworkConnection_t pConnection );

#endif /* ifndef CY_IOT_NETWORK_SECURED_SOCKET_H_ */
