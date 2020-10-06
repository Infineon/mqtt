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

/**
 * @brief The timeout for the Cypress secure socket poll call in the receive thread.
 *
 * After the timeout expires, the receive thread will check will queries the
 * connection flags to ensure that the connection is still open. Therefore,
 * this flag represents the maximum time it takes for the receive thread to
 * detect a closed connection.
 *
 * This timeout is also used to wait for all receive threads to exit during
 * global cleanup.
 *
 * Since this value only affects the shutdown sequence, it generally does not
 * need to be changed.
 */
#ifndef IOT_NETWORK_CY_SOCKETS_POLL_TIMEOUT_MS
    #define IOT_NETWORK_CY_SOCKETS_POLL_TIMEOUT_MS      ( 100UL )
#endif

/* Flags to track connection state. */
#define FLAG_HAS_RECEIVE_CALLBACK                       ( 0x00000002UL ) /**< @brief Connection has receive callback. */
#define FLAG_CONNECTION_DESTROYED                       ( 0x00000004UL ) /**< @brief Connection has been destroyed. */

#define IOT_NETWORK_CY_SOCKETS_POLL_DELAY_IN_MSEC       ( 100UL )
#define IOT_NETWORK_CY_SOCKETS_READ_DEALY_IN_MSEC       ( 50UL )

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
    IotSemaphore_t destroyNotification;          /**< @brief Notifies the receive callback that the connection was destroyed. */
} _networkConnection_t;

/*-----------------------------------------------------------*/

/**
 * @brief Tracks the number of active receive threads.
 */
static uint32_t _receiveThreadCount = 0;

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

    if( ( pConnection->flags & FLAG_HAS_RECEIVE_CALLBACK ) == FLAG_HAS_RECEIVE_CALLBACK )
    {
        IotSemaphore_Destroy( &( pConnection->destroyNotification ) );
    }

    /* Free memory. */
    IotNetwork_Free( pConnection );
}

/*-----------------------------------------------------------*/
/**
 * @brief Network receive thread.
 *
 * This thread polls the network socket and reads data when data is available.
 * It then invokes the receive callback, if any.
 *
 * @param[in] pArgument The connection associated with this receive thread.
 */

static void _receiveThread( void * pArgument )
{
    cy_rslt_t res = CY_RSLT_SUCCESS;
    uint32_t flags = CY_SOCKET_POLL_READ;

    /* Cast function parameter to correct type. */
    _networkConnection_t * pConnection = pArgument;

    /* Continuously poll the network connection for events. */
    while( true )
    {
        flags = CY_SOCKET_POLL_READ;
        res = cy_socket_poll( pConnection->handle, &flags, IOT_NETWORK_CY_SOCKETS_POLL_TIMEOUT_MS );
        if( res != CY_RSLT_SUCCESS )
        {
            break;
        }
        else
        {
            /* Invoke receive callback if data is available. */
            if( flags & CY_SOCKET_POLL_READ )
            {
                IotMutex_Lock( &( pConnection->callbackMutex ) );

                /* Only run the receive callback if the connection has not been
                 * destroyed. */
                if( ( pConnection->flags & FLAG_CONNECTION_DESTROYED ) == 0 )
                {
                    pConnection->receiveCallback( pConnection,
                                                  pConnection->pReceiveContext );
                }

                IotMutex_Unlock( &( pConnection->callbackMutex ) );
            }
            else
            {
                IotClock_SleepMs( IOT_NETWORK_CY_SOCKETS_POLL_DELAY_IN_MSEC );
            }
        }
    }

    /**
     * If a close callback has been defined, invoke it now; since we
     * don't know what caused the close, use "unknown" as the reason.
     */
    if( pConnection->closeCallback != NULL )
    {
        pConnection->closeCallback( pConnection,
                                    IOT_NETWORK_UNKNOWN_CLOSED,
                                    pConnection->pCloseContext );
    }

    /* Wait for the call to network destroy, then destroy the connection. */
    IotSemaphore_Wait( &( pConnection->destroyNotification ) );
    IotLogDebug( "(Network connection %p) Receive thread terminating.", pConnection );
    _destroyConnection( pConnection );

    ( void ) Atomic_Decrement_u32( &_receiveThreadCount );

}

/*-----------------------------------------------------------*/

const IotNetworkInterface_t * IotNetworkSecureSockets_GetInterface( void )
{
    return &_networkCYSecuredSocket;
}

/*-----------------------------------------------------------*/

IotNetworkError_t IotNetworkSecureSockets_Init( void )
{
    IotNetworkError_t status = IOT_NETWORK_SUCCESS;
    /* Clear the counter of receive threads. */
    _receiveThreadCount = 0;

    cy_rslt_t res = cy_socket_init();
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Network library initialization failed." );
        status = IOT_NETWORK_FAILURE;
    }
    else
    {
        IotLogInfo( "Network library initialized." );
    }

    return status;
}

/*-----------------------------------------------------------*/

void IotNetworkSecureSockets_Cleanup( void )
{
    /* Atomically read the receive thread count by adding 0 to it. Sleep and
     * wait for all receive threads to exit. */
    while( Atomic_Add_u32( &_receiveThreadCount, 0 ) > 0 )
    {
        IotClock_SleepMs( IOT_NETWORK_CY_SOCKETS_POLL_TIMEOUT_MS );
    }
    cy_rslt_t res = cy_socket_deinit();
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Network library cleanup failed." );
    }
    else
    {
        IotLogInfo( "Network library cleanup done" );
    }
}

/*-----------------------------------------------------------*/

IotNetworkError_t IotNetworkSecureSockets_Create( IotNetworkServerInfo_t pServerInfo,
                                                  IotNetworkCredentials_t pCredentialInfo,
                                                  IotNetworkConnection_t * pConnection )
{
    IotNetworkError_t status = IOT_NETWORK_SUCCESS;
    _networkConnection_t * pNewNetworkConnection = NULL;

    /* Flags to track initialization. */
    bool socketContextCreated = false;
    bool networkMutexCreated = false;
    bool callbackMutexCreated = false;

    cy_rslt_t res ;
    cy_socket_ip_address_t ip_addr;
    char *ptr ;

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
        return IOT_NETWORK_FAILURE;
    }

    /* Initialize the mutex for the receive callback. */
    callbackMutexCreated = IotMutex_Create( &( pNewNetworkConnection->callbackMutex ),
                                            true );
    if( callbackMutexCreated == false )
    {
        IotLogError( "Failed to create mutex for receive callback." );
        if( networkMutexCreated )
        {
            IotMutex_Destroy( &( pNewNetworkConnection->contextMutex ) );
        }
        return IOT_NETWORK_FAILURE;
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
        res = cy_tls_load_global_root_ca_certificates( pCredentialInfo->pRootCa, pCredentialInfo->rootCaSize - 1 );
        if( res != CY_RSLT_SUCCESS)
        {
            IotLogError( "cy_tls_load_global_root_ca_certificates failed %d\n", res );
            status = IOT_NETWORK_FAILURE;
            goto exit;
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

        res = cy_tls_create_identity( pCredentialInfo->pClientCert, pCredentialInfo->clientCertSize - 1,
                                      pCredentialInfo->pPrivateKey, pCredentialInfo->privateKeySize - 1,
                                      &(pNewNetworkConnection->tls_identity) );
        if( res != CY_RSLT_SUCCESS )
        {
            IotLogError( "cy_tls_create_identity failed\n" );
            status = IOT_NETWORK_FAILURE;
            goto exit;
        }

        res = cy_socket_create( CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM,
                                CY_SOCKET_IPPROTO_TLS, &(pNewNetworkConnection->handle) );
        if( res != CY_RSLT_SUCCESS )
        {
            IotLogError("Socket create failed\n");
            status = IOT_NETWORK_FAILURE;
            goto exit;
        }

        IotLogInfo( "cy_socket_create Success\n" );

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

        res = cy_socket_setsockopt( pNewNetworkConnection->handle, CY_SOCKET_SOL_TLS,
                                    CY_SOCKET_SO_TLS_AUTH_MODE, (const void *) CY_SOCKET_TLS_VERIFY_REQUIRED,
                                    (uint32_t) sizeof( CY_SOCKET_TLS_VERIFY_REQUIRED ) );
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

        IotLogInfo( "cy_socket_create Success\n" );
    }
    socketContextCreated = true;

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

        if( socketContextCreated == true )
        {
            cy_socket_delete( pNewNetworkConnection->handle );
        }

        if( pNewNetworkConnection != NULL )
        {
            IotNetwork_Free( pNewNetworkConnection );
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

IotNetworkError_t IotNetworkSecureSockets_SetReceiveCallback( IotNetworkConnection_t pConnection,
                                                              IotNetworkReceiveCallback_t receiveCallback,
                                                              void * pContext )
{
    IotNetworkError_t status = IOT_NETWORK_SUCCESS;

    /* Flags to track initialization. */
    bool notifyInitialized = false, countIncremented = false;

    /* Initialize the semaphore that notifies the receive thread of connection
     * destruction. */
    notifyInitialized = IotSemaphore_Create( &( pConnection->destroyNotification ), 0, 1 );
    if( notifyInitialized == false )
    {
        IotLogError( "(Network connection %p) Failed to create semaphore for "
                     "receive thread.", pConnection );

        status = IOT_NETWORK_SYSTEM_ERROR;
        goto exit;
    }

    /* Set the callback (must be non-NULL) and parameter. */
    if( receiveCallback == NULL )
    {
        status = IOT_NETWORK_BAD_PARAMETER;
        goto exit;
    }

    pConnection->receiveCallback = receiveCallback;
    pConnection->pReceiveContext = pContext;

    /* Set the receive callback flag and increment the count of receive threads. */
    pConnection->flags |= FLAG_HAS_RECEIVE_CALLBACK;
    ( void ) Atomic_Increment_u32( &_receiveThreadCount );
    countIncremented = true;

    /* Create the thread to receive incoming data. */
    if( Iot_CreateDetachedThread( _receiveThread,
                                  pConnection,
                                  IOT_THREAD_DEFAULT_PRIORITY,
                                  IOT_THREAD_DEFAULT_STACK_SIZE ) == false )
    {
        IotLogError( "(Network connection %p) Failed to create thread for receiving data.",
                     pConnection );
        status = IOT_NETWORK_SYSTEM_ERROR;
    }

 exit :
    /* Clean up on error. */
    if( status != IOT_NETWORK_SUCCESS )
    {
        if( notifyInitialized == true )
        {
            IotSemaphore_Destroy( &( pConnection->destroyNotification ) );
        }

        if( countIncremented == true )
        {
            pConnection->flags &= ~FLAG_HAS_RECEIVE_CALLBACK;
            ( void ) Atomic_Decrement_u32( &_receiveThreadCount );
        }
    }
    else
    {
        IotLogDebug( "(Network connection %p) Receive callback set.",
                     pConnection );
    }

    return status;
}

/*-----------------------------------------------------------*/

IotNetworkError_t IotNetworkSecureSockets_SetCloseCallback( IotNetworkConnection_t pConnection,
                                                            IotNetworkCloseCallback_t closeCallback,
                                                            void * pContext )
{
    IotNetworkError_t status = IOT_NETWORK_BAD_PARAMETER;

    if( closeCallback != NULL )
    {
        /* Set the callback and parameter. */
        pConnection->closeCallback = closeCallback;
        pConnection->pCloseContext = pContext;

        status = IOT_NETWORK_SUCCESS;
    }

    return status;
}

/*-----------------------------------------------------------*/

size_t IotNetworkSecureSockets_Send( IotNetworkConnection_t pConnection,
                                     const uint8_t * pMessage,
                                     size_t messageLength )
{
    size_t bytesSent = 0;
    cy_rslt_t res ;

    IotLogDebug( "(Network connection %p) Sending %lu bytes.",
                 pConnection,
                 ( unsigned long ) messageLength );
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
    cy_rslt_t res = 0 ;
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

    /* Check if this connection has a receive callback. If it does not, it can
     * be destroyed here. Otherwise, notify the receive callback that destroy
     * has been called and rely on the receive callback to clean up. */
    if( ( pConnection->flags & FLAG_HAS_RECEIVE_CALLBACK ) == 0 )
    {
        _destroyConnection( pConnection );
    }
    else
    {
        IotSemaphore_Post( &( pConnection->destroyNotification ) );
    }

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
