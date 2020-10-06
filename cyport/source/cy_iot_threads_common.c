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
 * @file cy_iot_threads_common.c
 * @brief Implementation of the functions in iot_threads.h for using cypress abstraction-rtos.
 */

/* Platform threads include. */
#include "iot_threads.h"

#include "cyabs_rtos.h"
#include "cy_result.h"

#include <stdio.h>

/* Configure logs for the functions in this file. */
#ifdef IOT_LOG_LEVEL_PLATFORM
    #define LIBRARY_LOG_LEVEL        IOT_LOG_LEVEL_PLATFORM
#else
    #ifdef IOT_LOG_LEVEL_GLOBAL
        #define LIBRARY_LOG_LEVEL    IOT_LOG_LEVEL_GLOBAL
    #else
        #define LIBRARY_LOG_LEVEL    IOT_LOG_NONE
    #endif
#endif

#define LIBRARY_LOG_NAME    ( "THREAD" )
#include "iot_logging_setup.h"

/*
 * Provide default values for undefined memory allocation functions based on
 * the usage of dynamic memory allocation.
 */
#ifndef IotThreads_Malloc
    #include <stdlib.h>
    #define IotThreads_Malloc    malloc
#endif

#ifndef IotThreads_Free
    #include <stdlib.h>
    #define IotThreads_Free    free
#endif

/*-----------------------------------------------------------*/

bool IotMutex_Create( IotMutex_t * pNewMutex, bool recursive )
{
    _IotSystemMutex_t * internalMutex = (_IotSystemMutex_t *) pNewMutex;

    configASSERT( internalMutex != NULL );

    IotLogDebug( "Creating new mutex %p.", pNewMutex );

    cy_rslt_t res = cy_rtos_init_mutex2( internalMutex, recursive );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Creating new mutex %p. failed", pNewMutex );
        return false;
    }

    return true;
}

/*-----------------------------------------------------------*/

void IotMutex_Destroy( IotMutex_t * pMutex )
{
    _IotSystemMutex_t * internalMutex = (_IotSystemMutex_t *) pMutex;

    configASSERT( internalMutex != NULL );
    cy_rslt_t res = cy_rtos_deinit_mutex( internalMutex );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Destroying mutex %p. failed", internalMutex );
    }
}

/*-----------------------------------------------------------*/

bool prIotMutexTimedLock( IotMutex_t * pMutex,
                          TickType_t timeout )
{
    _IotSystemMutex_t * internalMutex = (_IotSystemMutex_t *) pMutex;

    configASSERT( internalMutex != NULL );

    IotLogDebug( "Locking mutex %p.", internalMutex );

    cy_rslt_t res = cy_rtos_get_mutex( internalMutex, (cy_time_t) timeout );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Get mutex %p. failed", pMutex );
        return false;
    }
    else
    {
        return true;
    }
 }

/*-----------------------------------------------------------*/

void IotMutex_Lock( IotMutex_t * pMutex )
{
    bool lockResult = false;
    lockResult = prIotMutexTimedLock( pMutex, portMAX_DELAY );
    if( lockResult == false )
    {
        IotLogError( "Locking mutex %p. failed", pMutex );
        configASSERT( false );
    }
}

/*-----------------------------------------------------------*/

bool IotMutex_TryLock( IotMutex_t * pMutex )
{
    return prIotMutexTimedLock( pMutex, 0 );
}

/*-----------------------------------------------------------*/

void IotMutex_Unlock( IotMutex_t * pMutex )
{
    _IotSystemMutex_t * internalMutex = (_IotSystemMutex_t *) pMutex;

    configASSERT( internalMutex != NULL );

    IotLogDebug( "Unlocking mutex %p.", internalMutex );

    cy_rslt_t res = cy_rtos_set_mutex( internalMutex );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Set mutex %p. failed", pMutex );
    }

}

/*-----------------------------------------------------------*/

bool IotSemaphore_Create( IotSemaphore_t * pNewSemaphore,
                          uint32_t initialValue,
                          uint32_t maxValue )
{
    _IotSystemSemaphore_t * internalSemaphore = (_IotSystemSemaphore_t *) pNewSemaphore;

    configASSERT( internalSemaphore != NULL );

    IotLogDebug( "Creating new semaphore %p.", pNewSemaphore );

    cy_rslt_t res = cy_rtos_init_semaphore( internalSemaphore, maxValue, initialValue );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Creating new semaphore %p. failed", internalSemaphore );
        return false;
    }
    return true;
}

/*-----------------------------------------------------------*/

void IotSemaphore_Destroy( IotSemaphore_t * pSemaphore )
{
    _IotSystemSemaphore_t * internalSemaphore = (_IotSystemSemaphore_t *) pSemaphore;

    configASSERT( internalSemaphore != NULL );

    IotLogDebug( "Destroying semaphore %p.", internalSemaphore );

    cy_rslt_t res = cy_rtos_deinit_semaphore( internalSemaphore );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Destroying semaphore %p. failed", internalSemaphore );
    }

}

/*-----------------------------------------------------------*/

void IotSemaphore_Wait( IotSemaphore_t * pSemaphore )
{
    _IotSystemSemaphore_t * internalSemaphore = (_IotSystemSemaphore_t *) pSemaphore;

    configASSERT( internalSemaphore != NULL );

    IotLogDebug( "Waiting on semaphore %p.", internalSemaphore );

    if( cy_rtos_get_semaphore( internalSemaphore,
                               CY_RTOS_NEVER_TIMEOUT, false) != CY_RSLT_SUCCESS )
    {
        IotLogWarn( "Failed to wait on semaphore %p.",
                    pSemaphore );

        /* Assert here, debugging we always want to know that this happened because you think
         *   that you are waiting successfully on the semaphore but you are not   */
        configASSERT( false );
    }
}

/*-----------------------------------------------------------*/

bool IotSemaphore_TryWait( IotSemaphore_t * pSemaphore )
{
    _IotSystemSemaphore_t * internalSemaphore = (_IotSystemSemaphore_t *) pSemaphore;

    configASSERT( internalSemaphore != NULL );

    IotLogDebug( "Attempting to wait on semaphore %p.", internalSemaphore );

    return IotSemaphore_TimedWait( pSemaphore, 0 );
}

/*-----------------------------------------------------------*/

bool IotSemaphore_TimedWait( IotSemaphore_t * pSemaphore,
                             uint32_t timeoutMs )
{
    _IotSystemSemaphore_t * internalSemaphore = (_IotSystemSemaphore_t *) pSemaphore;

    configASSERT( internalSemaphore != NULL );

    if( cy_rtos_get_semaphore( internalSemaphore,
                                      timeoutMs, false) != CY_RSLT_SUCCESS )
    {
        /* Only warn if timeout > 0 */
        if( timeoutMs > 0 )
        {
            IotLogWarn( "Timeout waiting on semaphore %p.",
                        internalSemaphore );
        }
        return false;
    }
    return true;
}

/*-----------------------------------------------------------*/

void IotSemaphore_Post( IotSemaphore_t * pSemaphore )
{
    _IotSystemSemaphore_t * internalSemaphore = (_IotSystemSemaphore_t *) pSemaphore;

    configASSERT( internalSemaphore != NULL );
    IotLogDebug( "Posting to semaphore %p.", internalSemaphore );
    cy_rslt_t res = cy_rtos_set_semaphore( internalSemaphore, false);
    if(res != CY_RSLT_SUCCESS)
    {
        IotLogError( "Posting to semaphore %p. failed", internalSemaphore );
    }
}

/*-----------------------------------------------------------*/

uint32_t IotSemaphore_GetCount( IotSemaphore_t * pSemaphore )
{
    _IotSystemSemaphore_t * internalSemaphore = (_IotSystemSemaphore_t *) pSemaphore;
    size_t count = 0;

    configASSERT( internalSemaphore != NULL );
    cy_rslt_t res = cy_rtos_get_count_semaphore( internalSemaphore, &count );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Get count of semaphore %p. failed", pSemaphore );
    }
    else
    {
        IotLogInfo( "Semaphore %p has count %d.", pSemaphore, count );
    }

    return ( (uint32_t) count );
}

/*-----------------------------------------------------------*/
