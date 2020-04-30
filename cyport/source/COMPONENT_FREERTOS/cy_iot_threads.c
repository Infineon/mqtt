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
 * @file cy_iot_threads.c
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

static uint16_t thread_sno = 0;
/*-----------------------------------------------------------*/

static void _threadRoutineWrapper( void * pArgument )
{
    threadInfo_t * pThreadInfo = ( threadInfo_t * ) pArgument;

    /* Run the thread routine. */
    pThreadInfo->threadRoutine( pThreadInfo->pArgument );
    IotThreads_Free( pThreadInfo );

    vTaskDelete( NULL );
}

/*-----------------------------------------------------------*/

bool Iot_CreateDetachedThread( IotThreadRoutine_t threadRoutine,
                               void * pArgument,
                               int32_t priority,
                               size_t stackSize )
{
    bool status = true;
    char th_name[32];

    configASSERT( threadRoutine != NULL );

    IotLogDebug( "Creating new thread." );
    threadInfo_t * pThreadInfo = IotThreads_Malloc( sizeof( threadInfo_t ) );

    if( pThreadInfo == NULL )
    {
        IotLogError( "Unable to allocate memory for threadRoutine %p.", threadRoutine );
        status = false;
    }

    snprintf(th_name, sizeof(th_name), "%s%d",  "IoTSDK-", thread_sno++);

    /* Create the FreeRTOS task that will run the thread. */
    if( status )
    {
        pThreadInfo->threadRoutine = threadRoutine;
        pThreadInfo->pArgument = pArgument;

        if( xTaskCreate( _threadRoutineWrapper,
                         th_name,
                         ( configSTACK_DEPTH_TYPE ) stackSize,
                         pThreadInfo,
                         priority,
                         &( pThreadInfo->handle ) ) != pdPASS )
        {
            /* Task creation failed. */
            IotLogWarn( "Failed to create thread." );
            IotThreads_Free( pThreadInfo );
            status = false;
        }
    }

    return status;
}

/*-----------------------------------------------------------*/
