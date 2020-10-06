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
 * @file cy_iot_clock.c
 * @brief Implementation of the functions in iot_clock.h for using cypress abstraction-rtos.
 */

/* Standard includes. */
#include <stdio.h>

#include "iot_clock.h"
#include "cyabs_rtos.h"
#include "cy_result.h"

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

#define LIBRARY_LOG_NAME    ( "CLOCK" )
#include "iot_logging_setup.h"

/*-----------------------------------------------------------*/

bool IotClock_TimerCreate( IotTimer_t * pNewTimer,
                           IotThreadRoutine_t expirationRoutine,
                           void * pArgument )
{
    _IotSystemTimer_t * pxTimer = (_IotSystemTimer_t *) pNewTimer;

    configASSERT( pNewTimer != NULL );
    configASSERT( expirationRoutine != NULL );

    IotLogDebug( "Creating new timer %p.", pNewTimer );

    cy_rslt_t res = cy_rtos_init_timer( &(pxTimer->timer), CY_TIMER_TYPE_ONCE,
                                        (cy_timer_callback_t) expirationRoutine, 
                                        (cy_timer_callback_arg_t) pArgument );
    if ( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Creating new Timer %p. failed", pNewTimer );
        return false;
    }
    else
    {
        return true;
    }
}

/*-----------------------------------------------------------*/

void IotClock_TimerDestroy( IotTimer_t * pTimer )
{
    cy_rslt_t res;
    _IotSystemTimer_t * pTimerInfo = (_IotSystemTimer_t *) pTimer;

    configASSERT( pTimerInfo != NULL );
    configASSERT( pTimerInfo->timer != NULL );

    IotLogDebug( "Destroying timer %p.", pTimer );
    res = cy_rtos_stop_timer( (cy_timer_t) &(pTimerInfo->timer) );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Destroying timer %p. Failed ", pTimer );
    }
    res = cy_rtos_deinit_timer( (cy_timer_t) &(pTimerInfo->timer) );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "Destroying timer %p. Failed in deinit", pTimer );
    }
}

/*-----------------------------------------------------------*/
uint64_t IotClock_GetTimeMs( void )
{
    cy_time_t TimeMs=0;
    cy_rslt_t res = cy_rtos_get_time( &TimeMs );
    if( res == CY_RSLT_SUCCESS )
    {
        return( (uint64_t) TimeMs );
    }
    else
    {
        IotLogError( "GetTimeMs failed with res = %ld", res );
        TimeMs = 0;
        return( (uint64_t) TimeMs );
    }
}

/*-----------------------------------------------------------*/
bool IotClock_GetTimestring( char * pBuffer,
                             size_t bufferSize,
                             size_t * pTimestringLength )
{
    uint64_t milliSeconds = IotClock_GetTimeMs();
    int timestringLength = 0;

    configASSERT( pBuffer != NULL );
    configASSERT( pTimestringLength != NULL );

    /* Convert the localTime struct to a string. */
    timestringLength = snprintf( pBuffer, bufferSize, "%llu", milliSeconds );

    /* Check for error from no string */
    if( timestringLength == 0 )
    {
        return false;
    }

    /* Set the output parameter. */
    *pTimestringLength = timestringLength;

    return true;
}

/*-----------------------------------------------------------*/

void IotClock_SleepMs( uint32_t sleepTimeMs )
{
    cy_rslt_t res = cy_rtos_delay_milliseconds( sleepTimeMs );
    if( res != CY_RSLT_SUCCESS )
    {
        IotLogError( "SleepMs failed res = %ld", res );
    }
}

/*-----------------------------------------------------------*/

bool IotClock_TimerArm( IotTimer_t * pTimer,
                        uint32_t relativeTimeoutMs,
                        uint32_t periodMs )
{
    _IotSystemTimer_t * pTimerInfo = (_IotSystemTimer_t *) pTimer;
    configASSERT( pTimerInfo != NULL );

    cy_timer_t xTimerHandle = pTimerInfo->timer;

    IotLogDebug( "Arming timer %p with timeout %llu and period %llu.",
                 pTimer,
                 relativeTimeoutMs,
                 periodMs );

    /* Set the timer period in ticks */
    pTimerInfo->xTimerPeriod = periodMs;

    while( cy_rtos_start_timer(&xTimerHandle, (cy_time_t) relativeTimeoutMs ) != CY_RSLT_SUCCESS )
    {
        cy_rtos_delay_milliseconds( 1 );
    }

    return true;
}

/*-----------------------------------------------------------*/
