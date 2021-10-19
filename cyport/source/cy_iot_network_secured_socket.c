/*
 * Copyright 2021, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
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
 * @file cy_iot_network_secured_socket.c
 * @brief Implementation of the network interface functions in iot_network.h using
 * Cypress socket abstraction layer APIs.
 */

/* The config header is always included first. */
#include "iot_config.h"

/* Standard includes. */
#include <stdio.h>
#include <string.h>

/* Cypress secure socket implementation include. */
#include "cy_iot_network_secured_socket.h"
/* Error handling include. */
#include "iot_error.h"
/* Platform clock include. */
#include "iot_clock.h"
/* Platform threads include. */
#include "iot_threads.h"
/* Atomic include. */
#include "iot_atomic.h"
/* RTOS abstraction include. */
#include "cyabs_rtos.h"

/* Configure logs for the functions in this file. */
#ifdef IOT_LOG_LEVEL_NETWORK
    #define LIBRARY_LOG_LEVEL        IOT_LOG_LEVEL_NETWORK
#else
    #ifdef IOT_LOG_LEVEL_GLOBAL
        #define LIBRARY_LOG_LEVEL    IOT_LOG_LEVEL_GLOBAL
    #else
        #define LIBRARY_LOG_LEVEL    IOT_LOG_NONE
    #endif
#endif

#define LIBRARY_LOG_NAME    ( "NET" )
#include "iot_logging_setup.h"

/*
 * Provide default values for undefined memory allocation functions.
 */
#ifndef IotNetwork_Malloc
    #include <stdlib.h>

/**
 * @brief Memory allocation. This function should have the same signature
 * as [malloc](http://pubs.opengroup.org/onlinepubs/9699919799/functions/malloc.html).
 */
    #define IotNetwork_Malloc    malloc
#endif
#ifndef IotNetwork_Free
    #include <stdlib.h>

/**
 * @brief Free memory. This function should have the same signature as
 * [free](http://pubs.opengroup.org/onlinepubs/9699919799/functions/free.html).
 */
    #define IotNetwork_Free    free
#endif

#define IOT_NETWORK_CY_SOCKETS_READ_DEALY_IN_MSEC        ( 50UL )
#define CY_MQTT_SOCKET_EVENT_QUEUE_SIZE                  ( 10UL )
#define CY_MQTT_SOCKET_EVENT_QUEUE_TIMEOUT_IN_MSEC       ( 500UL )
#define CY_MQTT_SOCKET_EVENT_THREAD_PRIORITY             ( CY_RTOS_PRIORITY_NORMAL )
#if (IOT_LOG_LEVEL_NETWORK == IOT_LOG_NONE)
    #define CY_MQTT_SOCKET_EVENT_THREAD_STACK_SIZE       ( 1024 * 2 )
#else
    #define CY_MQTT_SOCKET_EVENT_THREAD_STACK_SIZE       ( (1024 * 2) + (1024 * 3) ) /* Additional 3kb of stack is added for enabling the prints */
#endif
#define UNUSED_ARG(arg)                                  (void)(arg)

/**
 * @brief Represents a network connection.
 */
typedef struct _networkConnection
{
    uint32_t flags;                              /**< @brief Connection state flags. */
    cy_socket_t handle;                          /**< @brief Socket Handle. */
    cy_socket_sockaddr_t address;                /**< @brief Socket Address. */
    void * tls_identity;                         /**< @brief TLS Socket Identity. */
    IotMutex_t contextMutex;                     /**< @brief Protects this network context from concurrent access. */
    IotNetworkReceiveCallback_t receiveCallback; /**< @brief Network receive callback, if any. */
    void * pReceiveContext;                      /**< @brief The context for the receive callback. */
    IotNetworkCloseCallback_t closeCallback;     /**< @brief Network close callback, if any. */
    void * pCloseContext;                        /**< @brief The context for the close callback. */
    IotMutex_t callbackMutex;                    /**< @brief Synchronizes the receive callback with calls to destroy. */
} _networkConnection_t;

/*-----------------------------------------------------------*/

/**
 * @brief An #IotNetworkInterface_t that uses the functions in this file.
 */
static const IotNetworkInterface_t _networkCYSecuredSocket =
{
    .create             = IotNetworkSecureSockets_Create,
    .setReceiveCallback = IotNetworkSecureSockets_SetReceiveCallback,
    .send               = IotNetworkSecureSockets_Send,
    .receive            = IotNetworkSecureSockets_Receive,
    .close              = IotNetworkSecureSockets_Close,
    .destroy            = IotNetworkSecureSockets_Destroy
};

/**
 * MQTT event types.
 */
typedef enum cy_mqtt_socket_event_type
{
    CY_MQTT_SOCKET_EVENT_TYPE_SUBSCRIPTION_MESSAGE_RECIEVE = 0, /**< Incoming subscribed message */
    CY_MQTT_SOCKET_EVENT_TYPE_DISCONNECT                   = 1  /**< Disconnected from MQTT broker. */
} cy_mqtt_socket_event_type_t;

/**
 * MQTT event information structure.
 */
typedef struct cy_mqtt_event
{
    cy_mqtt_socket_event_type_t    type;             /**< Event type */
    IotNetworkConnection_t  pConnection;             /**< Connection handle */
} cy_mqtt_event_t;

static cy_thread_t   mqtt_socket_event_thread = NULL;
static cy_queue_t    mqtt_socket_event_queue = NULL;

/*-----------------------------------------------------------*/
/**
 * @brief Destroy a network connection.
 *
 * @param[in] pConnection The network connection to destroy.
 */
static void _destroyConnection( _networkConnection_t * pConnection )
{
    cy_rslt_t res = CY_RSLT_SUCCESS;

    /* Clean up the context of secure socket connections. */
    res = cy_socket_delete( pConnection->handle );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "cy_socket_delete failed\n" );
    }

    /* Destroy synchronization objects. */
    IotMutex_Destroy( &( pConnection->contextMutex ) );
    IotMutex_Destroy( &( pConnection->callbackMutex ) );

    /* Free memory. */
    IotNetwork_Free( pConnection );
}

/*-----------------------------------------------------------*/
const IotNetworkInterface_t * IotNetworkSecureSockets_GetInterface( void )
{
    return &_networkCYSecuredSocket;
}

/*----------------------------------------------------------------------------------------------------------*/
static void mqtt_socket_event_task( cy_thread_arg_t arg )
{
    cy_rslt_t         result = CY_RSLT_SUCCESS;
    cy_mqtt_event_t   event;
    IotNetworkConnection_t pConnection = NULL;
    UNUSED_ARG( arg );

    IotLogInfo( "\nStarting mqtt_socket_event_thread...\n" );

    while( true )
    {
        memset( &event, 0x00, sizeof(cy_mqtt_event_t) );
        result = cy_rtos_get_queue( &mqtt_socket_event_queue, (void *)&event, CY_RTOS_NEVER_TIMEOUT, false );
        if( CY_RSLT_SUCCESS != result )
        {
            IotLogError( "\ncy_rtos_get_queue for mqtt_socket_event_queue failed with Error : [0x%X] ", (unsigned int)result );
            continue;
        }
        switch( event.type )
        {
            case CY_MQTT_SOCKET_EVENT_TYPE_SUBSCRIPTION_MESSAGE_RECIEVE:
                pConnection = (IotNetworkConnection_t)event.pConnection;
                IotMutex_Lock( &( pConnection->callbackMutex ) );
                if ( pConnection->receiveCallback != NULL )
                {
                    pConnection->receiveCallback( pConnection, pConnection->pReceiveContext );
                }
                IotMutex_Unlock( &( pConnection->callbackMutex ) );
                break;

            case CY_MQTT_SOCKET_EVENT_TYPE_DISCONNECT:
                pConnection = (IotNetworkConnection_t)event.pConnection;
                /**
                 * If a close callback has been defined, invoke it now; since we
                 * don't know what caused the close, use "unknown" as the reason.
                 */
                IotMutex_Lock( &( pConnection->callbackMutex ) );
                if( pConnection->closeCallback != NULL )
                {
                    pConnection->closeCallback( pConnection,
                                                IOT_NETWORK_UNKNOWN_CLOSED,
                                                pConnection->pCloseContext );
                }
                IotMutex_Unlock( &( pConnection->callbackMutex ) );
                break;

                /* Unknown event type. */
            default:
                IotLogError( "Unknown event type." );
                break;
        }
    }
}

/*-----------------------------------------------------------*/
IotNetworkError_t IotNetworkSecureSockets_Init( void )
{
    IotNetworkError_t status = IOT_NETWORK_SUCCESS;
    cy_rslt_t res = CY_RSLT_SUCCESS;

    /*
     * Initialize the queue for network socket events.
     */
    res = cy_rtos_init_queue( &mqtt_socket_event_queue, CY_MQTT_SOCKET_EVENT_QUEUE_SIZE, sizeof(cy_mqtt_event_t) );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "\ncy_rtos_init_queue failed with Error : [0x%X] ", (unsigned int)res );
        return IOT_NETWORK_FAILURE;
    }

    res = cy_socket_init();
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Network library initialization failed." );
        (void)cy_rtos_deinit_queue( &mqtt_socket_event_queue );
        mqtt_socket_event_queue = NULL;
        return IOT_NETWORK_FAILURE;
    }
    else
    {
        IotLogInfo( "Network library initialized." );
    }

    res = cy_rtos_create_thread( &mqtt_socket_event_thread, mqtt_socket_event_task, "MQTTEventThread", NULL,
                                 CY_MQTT_SOCKET_EVENT_THREAD_STACK_SIZE, CY_MQTT_SOCKET_EVENT_THREAD_PRIORITY, NULL );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "cy_rtos_create_thread failed with Error : [0x%X] ", (unsigned int)res );
        (void)cy_socket_deinit();
        (void)cy_rtos_deinit_queue( &mqtt_socket_event_queue );
        mqtt_socket_event_queue = NULL;
        return IOT_NETWORK_FAILURE;
    }

    return status;
}

/*-----------------------------------------------------------*/
// TODO : API needs to return error type instead of void to the caller.
void IotNetworkSecureSockets_Cleanup( void )
{
    cy_rslt_t res = CY_RSLT_SUCCESS;

    res = cy_socket_deinit();
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Network library cleanup failed." );
    }
    else
    {
        IotLogInfo( "Network library cleanup done" );
    }

    if( mqtt_socket_event_thread != NULL )
    {
        IotLogInfo( "Terminating MQTT event thread %p..!\n", mqtt_socket_event_thread );
        res = cy_rtos_terminate_thread( &mqtt_socket_event_thread  );
        if( res != CY_RSLT_SUCCESS )
        {
            IotLogError( "Terminate MQTT event thread failed with Error : [0x%X] ", (unsigned int)res );
            return;
        }

        IotLogInfo( "Joining MQTT event thread %p..!\n", mqtt_socket_event_thread );
        res = cy_rtos_join_thread( &mqtt_socket_event_thread );
        if( res != CY_RSLT_SUCCESS )
        {
            IotLogError( "Join MQTT event thread failed with Error : [0x%X] ", (unsigned int)res );
            return;
        }
        mqtt_socket_event_thread = NULL;
    }

    if( mqtt_socket_event_queue != NULL )
    {
        IotLogInfo( "Deinitializing mqtt_socket_event_queue %p..!\n", mqtt_socket_event_queue );
        res = cy_rtos_deinit_queue( &mqtt_socket_event_queue );
        if( res != CY_RSLT_SUCCESS )
        {
            IotLogError( "cy_rtos_deinit_queue failed with Error : [0x%X] ", (unsigned int)res );
            return;
        }
        else
        {
            IotLogDebug( "cy_rtos_deinit_queue successful." );
        }
        mqtt_socket_event_queue = NULL;
    }
}

/*-----------------------------------------------------------*/
IotNetworkError_t IotNetworkSecureSockets_Create( IotNetworkServerInfo_t pServerInfo,
                                                  IotNetworkCredentials_t pCredentialInfo,
                                                  IotNetworkConnection_t * pConnection )
{
    IotNetworkError_t status = IOT_NETWORK_SUCCESS;
    _networkConnection_t *pNewNetworkConnection = NULL;
    cy_socket_tls_auth_mode_t authmode = CY_SOCKET_TLS_VERIFY_NONE;

    /* Flags to track initialization. */
    bool socketContextCreated = false;
    bool networkMutexCreated = false;
    bool callbackMutexCreated = false;
    bool rootca_loaded = false;

    cy_rslt_t res = CY_RSLT_SUCCESS;
    cy_socket_ip_address_t ip_addr;
    char *ptr = NULL;
    UNUSED_ARG( ptr );

    /* Allocate memory for a new connection. */
    pNewNetworkConnection = IotNetwork_Malloc( sizeof( _networkConnection_t ) );
    if( pNewNetworkConnection == NULL )
    {
        IotLogError( "Failed to allocate memory for new network connection." );
        return IOT_NETWORK_NO_MEMORY;
    }

    /* Clear connection data. */
    memset( pNewNetworkConnection, 0x00, sizeof( _networkConnection_t ) );

    /* Initialize the network context mutex. */
    networkMutexCreated = IotMutex_Create( &( pNewNetworkConnection->contextMutex ),
                                           false );
    if( networkMutexCreated == false )
    {
        IotLogError( "Failed to create mutex for network context." );
        status = IOT_NETWORK_FAILURE;
        goto exit;
    }

    /* Initialize the mutex for the receive callback. */
    callbackMutexCreated = IotMutex_Create( &( pNewNetworkConnection->callbackMutex ),
                                            true );
    if( callbackMutexCreated == false )
    {
        IotLogError( "Failed to create mutex for receive callback." );
        status = IOT_NETWORK_FAILURE;
        goto exit;
    }

    res = cy_socket_gethostbyname( pServerInfo->pHostName, CY_SOCKET_IP_VER_V4, &ip_addr );
    if( res != CY_RSLT_SUCCESS)
    {
        IotLogError( "cy_socket_gethostbyname failed\n" );
        status = IOT_NETWORK_FAILURE;
        goto exit;
    }

    ptr = (char *) &ip_addr.ip.v4;
    IotLogInfo( "IP bytes %d %d %d %d \n", *(ptr+0), *(ptr+1), *(ptr+2), *(ptr+3) );

    /* Check for secured connection. */
    if( pCredentialInfo != NULL )
    {
        if( (pCredentialInfo->pRootCa != NULL) && (pCredentialInfo->rootCaSize > 0) )
        {
            res = cy_tls_load_global_root_ca_certificates( pCredentialInfo->pRootCa, pCredentialInfo->rootCaSize - 1 );
            if( res != CY_RSLT_SUCCESS)
            {
                IotLogError( "cy_tls_load_global_root_ca_certificates failed %d\n", res );
                status = IOT_NETWORK_FAILURE;
                goto exit;
            }
            rootca_loaded = true;
            authmode = CY_SOCKET_TLS_VERIFY_REQUIRED;
        }

#if defined( CY_MQTT_ENABLE_SECURE_TEST_MOSQUITTO_SUPPORT )
        res = cy_tls_config_cert_profile_param( CY_TLS_MD_SHA1, CY_TLS_RSA_MIN_KEY_LEN_1024 );
        if( res != CY_RSLT_SUCCESS)
        {
            IotLogError( "cy_tls_config_cert_profile_param failed %d\n", res );
            status = IOT_NETWORK_FAILURE;
            goto exit;
        }
#endif
        pNewNetworkConnection->tls_identity = NULL;
        if( (pCredentialInfo->pClientCert != NULL) && (pCredentialInfo->clientCertSize > 0) &&
            (pCredentialInfo->pPrivateKey != NULL) && (pCredentialInfo->privateKeySize > 0) )
        {
            res = cy_tls_create_identity( pCredentialInfo->pClientCert, pCredentialInfo->clientCertSize - 1,
                                          pCredentialInfo->pPrivateKey, pCredentialInfo->privateKeySize - 1,
                                          &(pNewNetworkConnection->tls_identity) );
            if( res != CY_RSLT_SUCCESS )
            {
                IotLogError( "cy_tls_create_identity failed\n" );
                status = IOT_NETWORK_FAILURE;
                goto exit;
            }
        }

        res = cy_socket_create( CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM,
                                CY_SOCKET_IPPROTO_TLS, &(pNewNetworkConnection->handle) );
        if( res != CY_RSLT_SUCCESS )
        {
            IotLogError("Socket create failed\n");
            status = IOT_NETWORK_FAILURE;
            goto exit;
        }
        socketContextCreated = true;
        IotLogInfo( "cy_socket_create Success\n" );

        if( pNewNetworkConnection->tls_identity != NULL )
        {
            /* FALSE-POSITIVE:
             * CID: 217093 Wrong sizeof argument
             *     The last argument is expected to be the size of the pointer itself which is 4 bytes, hence this is a false positive.
             */
            res = cy_socket_setsockopt( pNewNetworkConnection->handle, CY_SOCKET_SOL_TLS,
                                        CY_SOCKET_SO_TLS_IDENTITY, pNewNetworkConnection->tls_identity,
                                        (uint32_t) sizeof( pNewNetworkConnection->tls_identity ) );
            if( res != CY_RSLT_SUCCESS)
            {
                IotLogError( "cy_socket_setsockopt failed\n" );
                status = IOT_NETWORK_FAILURE;
                goto exit;
            }
        }

        res = cy_socket_setsockopt( pNewNetworkConnection->handle, CY_SOCKET_SOL_TLS,
                                    CY_SOCKET_SO_TLS_AUTH_MODE, (const void *) authmode,
                                    (uint32_t) sizeof( authmode ) );
        if( res != CY_RSLT_SUCCESS)
        {
            IotLogError( "cy_socket_setsockopt failed\n" );
            status = IOT_NETWORK_FAILURE;
            goto exit;
        }
    }
    else
    {
        res = cy_socket_create( CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM,
                                CY_SOCKET_IPPROTO_TCP, &(pNewNetworkConnection->handle) );
        if( res != CY_RSLT_SUCCESS )
        {
            IotLogError("Socket create failed\n");
            status = IOT_NETWORK_FAILURE;
            goto exit;
        }
        socketContextCreated = true;
        IotLogInfo( "cy_socket_create Success\n" );
    }

    pNewNetworkConnection->address.ip_address.ip.v4 = ip_addr.ip.v4;
    pNewNetworkConnection->address.ip_address.version = CY_SOCKET_IP_VER_V4;
    pNewNetworkConnection->address.port = pServerInfo->port;

    res = cy_socket_connect( pNewNetworkConnection->handle, &(pNewNetworkConnection->address),
                             sizeof(cy_socket_sockaddr_t) );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "cy_socket_connect failed\n" );
        status = IOT_NETWORK_FAILURE;
        goto exit;
    }
    IotLogInfo( "cy_socket_connect Success\n" );

exit:
    /* Clean up on error. */
    if( status != IOT_NETWORK_SUCCESS )
    {
        if( networkMutexCreated == true )
        {
            IotMutex_Destroy( &( pNewNetworkConnection->contextMutex ) );
        }

        if( callbackMutexCreated == true )
        {
            IotMutex_Destroy( &( pNewNetworkConnection->callbackMutex ) );
        }

        if( rootca_loaded == true )
        {
            (void)cy_tls_release_global_root_ca_certificates();
        }

        if( pNewNetworkConnection->tls_identity != NULL )
        {
            (void)cy_tls_delete_identity( pNewNetworkConnection->tls_identity );
            pNewNetworkConnection->tls_identity = NULL;
        }

        if( socketContextCreated == true )
        {
            (void)cy_socket_delete( pNewNetworkConnection->handle );
        }

        if( pNewNetworkConnection != NULL )
        {
            IotNetwork_Free( pNewNetworkConnection );
            pNewNetworkConnection = NULL;
        }
    }
    else
    {
        IotLogInfo( "(Network connection %p) New network connection established.",
                    pNewNetworkConnection );

        /* Set the output parameter. */
        *pConnection = pNewNetworkConnection;
    }

    return status;
}

/*-----------------------------------------------------------*/
static cy_rslt_t close_callback( cy_socket_t socket_handle, void *arg )
{
    cy_rslt_t res = CY_RSLT_SUCCESS;
    cy_mqtt_event_t event;
    IotNetworkConnection_t pConnection = NULL;

    if( arg == NULL )
    {
        IotLogError( "\nInvalid handle to connection close callback, so nothing to do..!\n" );
        return res;
    }

    memset( &event, 0x00, sizeof(cy_mqtt_event_t) );
    pConnection = (IotNetworkConnection_t)arg;
    event.type = CY_MQTT_SOCKET_EVENT_TYPE_DISCONNECT;
    event.pConnection = pConnection;
    res = cy_rtos_put_queue( &mqtt_socket_event_queue, (void *)&event, CY_MQTT_SOCKET_EVENT_QUEUE_TIMEOUT_IN_MSEC, false );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "\nPushing to MQTT socket event queue failed with Error : [0x%X] ", (unsigned int)res );
        return res;
    }
    return res;
}

/*-----------------------------------------------------------*/
static cy_rslt_t receive_callback( cy_socket_t socket_handle, void *arg )
{
    cy_rslt_t res = CY_RSLT_SUCCESS;
    cy_mqtt_event_t event;
    IotNetworkConnection_t pConnection = NULL;

    if( arg == NULL )
    {
        IotLogError( "\nInvalid handle to data receive callback, so nothing to do..!\n" );
        return res;
    }

    memset( &event, 0x00, sizeof(cy_mqtt_event_t) );
    pConnection = (IotNetworkConnection_t)arg;
    event.type = CY_MQTT_SOCKET_EVENT_TYPE_SUBSCRIPTION_MESSAGE_RECIEVE;
    event.pConnection = pConnection;
    res = cy_rtos_put_queue( &mqtt_socket_event_queue, (void *)&event, CY_MQTT_SOCKET_EVENT_QUEUE_TIMEOUT_IN_MSEC, false );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "\nPushing to MQTT socket event queue failed with Error : [0x%X] ", (unsigned int)res );
        return res;
    }
    return res;
}

/*-----------------------------------------------------------*/
IotNetworkError_t IotNetworkSecureSockets_SetReceiveCallback( IotNetworkConnection_t pConnection,
                                                              IotNetworkReceiveCallback_t receiveCallback,
                                                              void * pContext )
{
    IotNetworkError_t status = IOT_NETWORK_SUCCESS;
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_socket_opt_callback_t *socket_callback_ptr = NULL;
    cy_socket_opt_callback_t socket_callback;
    uint32_t opt_len = 0;

    if( receiveCallback != NULL )
    {
        pConnection->receiveCallback = receiveCallback;
        pConnection->pReceiveContext = pContext;
        socket_callback.callback = receive_callback;
        socket_callback.arg = (void *)pConnection;
        socket_callback_ptr = &socket_callback;
        opt_len = sizeof( socket_callback );
    }

    result = cy_socket_setsockopt( pConnection->handle, CY_SOCKET_SOL_SOCKET,
                                   CY_SOCKET_SO_RECEIVE_CALLBACK,
                                   socket_callback_ptr, opt_len );
    if( result != CY_RSLT_SUCCESS )
    {
        IotLogError( "\nSet socket receive notification for socket handle = %p failed with Error : [0x%X]",
                       pConnection->handle, ( unsigned int )result );
        status = IOT_NETWORK_FAILURE;
    }
    return status;
}

/*-----------------------------------------------------------*/
IotNetworkError_t IotNetworkSecureSockets_SetCloseCallback( IotNetworkConnection_t pConnection,
                                                            IotNetworkCloseCallback_t closeCallback,
                                                            void * pContext )
{
    IotNetworkError_t status = IOT_NETWORK_SUCCESS;
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_socket_opt_callback_t *socket_callback_ptr = NULL;
    cy_socket_opt_callback_t socket_callback;
    uint32_t opt_len = 0;

    if( closeCallback != NULL )
    {
        /* Set the callback and parameter. */
        pConnection->closeCallback = closeCallback;
        pConnection->pCloseContext = pContext;
        socket_callback.callback = close_callback;
        socket_callback.arg = (void *)pConnection;
        socket_callback_ptr = &socket_callback;
        opt_len = sizeof( socket_callback );
    }

    result = cy_socket_setsockopt( pConnection->handle, CY_SOCKET_SOL_SOCKET,
                                   CY_SOCKET_SO_DISCONNECT_CALLBACK,
                                   socket_callback_ptr, opt_len );
    if( result != CY_RSLT_SUCCESS )
    {
        IotLogError( "\nSet socket close notification for socket handle = %p failed with Error : [0x%X]",
                       pConnection->handle, ( unsigned int )result );
        status = IOT_NETWORK_FAILURE;
    }
    return status;
}

/*-----------------------------------------------------------*/
size_t IotNetworkSecureSockets_Send( IotNetworkConnection_t pConnection,
                                     const uint8_t * pMessage,
                                     size_t messageLength )
{
    size_t bytesSent = 0;
    cy_rslt_t res = CY_RSLT_SUCCESS;

    IotLogDebug( "(Network connection %p) Sending %lu bytes.",
                 pConnection, ( unsigned long ) messageLength );
    IotMutex_Lock( &( pConnection->contextMutex ) );

    res = cy_socket_send( pConnection->handle, pMessage, messageLength, 0, (uint32_t *)&bytesSent );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "cy_socket_send failed\n" );
    }

    IotMutex_Unlock( &( pConnection->contextMutex ) );

    return bytesSent;
}

/*-----------------------------------------------------------*/
size_t IotNetworkSecureSockets_Receive( IotNetworkConnection_t pConnection,
                                        uint8_t * pBuffer,
                                        size_t bytesRequested )
{
    cy_rslt_t res = CY_RSLT_SUCCESS;
    size_t bytesReceived = 0;
    size_t bytesRead = 0;
    size_t bytesTobeRead = bytesRequested;

    do
    {
        IotMutex_Lock( &( pConnection->contextMutex ) );
        bytesRead = 0;
        res = cy_socket_recv( pConnection->handle, pBuffer, bytesTobeRead, 0, (uint32_t *)&bytesRead );
        if( res != CY_RSLT_SUCCESS && res != CY_RSLT_MODULE_SECURE_SOCKETS_TIMEOUT )
        {
            IotLogError("cy_socket_recv failed. Error = [0x%X]\n", res);
            IotMutex_Unlock( &( pConnection->contextMutex ) );
            break;
        }
        IotMutex_Unlock( &( pConnection->contextMutex ) );

        if( bytesRead == 0 )
        {
            IotLogWarn( "cy_socket_recv bytesReceived is 0. Wait and retry\n" );
            IotClock_SleepMs( IOT_NETWORK_CY_SOCKETS_READ_DEALY_IN_MSEC );
        }
        else
        {
            bytesReceived += bytesRead;
            bytesTobeRead -= bytesRead;
            pBuffer += bytesReceived;
        }
    } while( bytesReceived < bytesRequested );

    return bytesReceived;
}

/*-----------------------------------------------------------*/
IotNetworkError_t IotNetworkSecureSockets_Close( IotNetworkConnection_t pConnection )
{
    cy_rslt_t res = CY_RSLT_SUCCESS;

    IotMutex_Lock( &( pConnection->contextMutex ) );

    /* Disconnect the network connection. */
    res = cy_socket_disconnect( pConnection->handle, 0 );
    if( ( res !=  CY_RSLT_SUCCESS ) && ( res != CY_RSLT_MODULE_SECURE_SOCKETS_NOT_CONNECTED ) )
    {
        IotLogError( "cy_socket_disconnect failed\n" );
    }

    IotMutex_Unlock( &( pConnection->contextMutex ) );

    IotLogInfo( "(Network connection %p) Connection closed.", pConnection );

    return IOT_NETWORK_SUCCESS;
}

/*-----------------------------------------------------------*/
IotNetworkError_t IotNetworkSecureSockets_Destroy( IotNetworkConnection_t pConnection )
{
    cy_rslt_t res = CY_RSLT_SUCCESS;

    /* Shutdown the network connection. */
    IotMutex_Lock( &( pConnection->callbackMutex ) );
    /* Free the tls_identity and free the root certificates for secured connection */
    if( pConnection->tls_identity != NULL )
    {
        res = cy_tls_delete_identity( pConnection->tls_identity );
        if( res != CY_RSLT_SUCCESS )
        {
            IotLogError( "cy_tls_delete_identity failed\n" );
            return IOT_NETWORK_FAILURE;
        }

        res = cy_tls_release_global_root_ca_certificates();
        if( res != CY_RSLT_SUCCESS )
        {
            IotLogError( "cy_tls_release_global_root_ca_certificates failed\n" );
            return IOT_NETWORK_FAILURE;
        }
    }
    IotMutex_Unlock( &( pConnection->callbackMutex ) );

    _destroyConnection( pConnection );

    return IOT_NETWORK_SUCCESS;
}

/*-----------------------------------------------------------*/
int IotNetworkSecureSockets_GetSocket( IotNetworkConnection_t pConnection )
{
    /*
     * This function is a stub implementation.
     * This is used only for metrics to collect the file descriptors associated wit the socket,
     * it is not applicable for RTOS based embedded platforms.
     */
    return 0;
}

/*-----------------------------------------------------------*/
