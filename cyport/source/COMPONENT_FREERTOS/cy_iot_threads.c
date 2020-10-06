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
    size_t stackdepth = 0;

    configASSERT( threadRoutine != NULL );

    IotLogDebug( "Creating new thread." );
    threadInfo_t * pThreadInfo = IotThreads_Malloc( sizeof( threadInfo_t ) );

    if( pThreadInfo == NULL )
    {
        IotLogError( "Unable to allocate memory for threadRoutine %p.", threadRoutine );
        status = false;
    }

    snprintf(th_name, sizeof(th_name), "%s%d",  "IoTSDK-", thread_sno++);

    /*
     * xTaskCreate expects StackDepth (i.e., the number of variables the stack can hold) and not the stack size in bytes. Hence converting
     * stack size to StackDepth. for 32 bit system, stack type will be 4 bytes, hence stack depth will be 512 for 2048 bytes stack size
    */
    stackdepth = ( IOT_THREAD_DEFAULT_STACK_SIZE + ( ( sizeof(StackType_t) - ( IOT_THREAD_DEFAULT_STACK_SIZE % sizeof(StackType_t) ) ) % sizeof(StackType_t) ) ) >> (sizeof(StackType_t) >> 1);

    /* Create the FreeRTOS task that will run the thread. */
    if( status )
    {
        pThreadInfo->threadRoutine = threadRoutine;
        pThreadInfo->pArgument = pArgument;

        if( xTaskCreate( _threadRoutineWrapper,
                         th_name,
                         ( configSTACK_DEPTH_TYPE ) stackdepth,
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
