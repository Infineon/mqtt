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
 * @file cy_iot_platform_types.h
 * @brief Definitions of platform layer types for cypress RTOS abstraction.
 */

#ifndef CY_IOT_PLATFORM_TYPES_H_
#define CY_IOT_PLATFORM_TYPES_H_

#include "cyabs_rtos_impl.h"

/**
 * @brief Holds information about an active Mutex
 */
typedef struct iot_mutex_internal
{
    cy_mutex_t xMutex;        /**< cyabs_rtos mutex. */
} iot_mutex_internal_t;

/**
 * @brief Holds information about an active Semaphore
 */
typedef struct iot_sem_internal
{
    cy_semaphore_t xSemaphore;        /**< cyabs_rtos semaphore. */
} iot_sem_internal_t;

/**
 * @brief Holds information about an active detached thread so that we can
 *        delete the FreeRTOS task when it completes
 */
typedef struct threadInfo
{
    cy_thread_t handle;                    /**< @brief Thread handle. */
    void * pArgument;                      /**< @brief Argument to `threadRoutine`. */
    void ( * threadRoutine )( void * );    /**< @brief Thread function to run. */
} threadInfo_t;

/**
 * @brief Holds information about an active timer.
 */
typedef struct timerInfo
{
    cy_timer_t timer;                   /**< @brief Underlying timer. */
    cy_time_t xTimerPeriod;             /**< Period of this timer. */
} timerInfo_t;

/**
 * @brief The native mutex implemented on cypress platforms.
 */
typedef cy_mutex_t   _IotSystemMutex_t;

/**
 * @brief The native semaphore implemented on cypress platforms.
 */
typedef cy_semaphore_t     _IotSystemSemaphore_t;

/**
 * @brief The native timer implemented on cypress platforms.
 */
typedef timerInfo_t            _IotSystemTimer_t;

/**
 * @cond DOXYGEN_IGNORE
 * Doxygen should ignore this section.
 *
 * Forward declarations of the network types defined in iot_network.h.
 */
struct IotNetworkServerInfo;
struct IotNetworkCredentials;
struct _networkConnection;
/** @endcond */

/**
 * @brief The format for remote server host and port on this system.
 */
typedef struct IotNetworkServerInfo * _IotNetworkServerInfo_t;

/**
 * @brief The format for network credentials on this system.
 */
typedef struct IotNetworkCredentials * _IotNetworkCredentials_t;

/**
 * @brief The handle of a network connection on this system.
 */
typedef struct _networkConnection * _IotNetworkConnection_t;

#endif /* ifndef CY_IOT_PLATFORM_TYPES_H_ */
