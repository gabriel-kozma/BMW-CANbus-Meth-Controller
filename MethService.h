/********************************************************************/
/*    Methanol Injection Service Implementation Header              */
/*    MethService.h                                                 */
/*                                                                  */
/*    Author:         Jacob Moroni (jakemoroni@gmail.com)           */
/*                                                                  */
/*    Description:    This file implements the methanol injection   */
/*                    service. This is one of many possible         */
/*                    services that can be incorporated into the    */
/*                    main controller.                              */
/*                                                                  */
/*    Revision:       Oct 12, 2015 (First)                          */
/*                                                                  */
/*    Copyright (C) 2015 Jacob Moroni.  All right reserved.         */
/*                                                                  */
/*    This library is free software; you can redistribute it and/or */
/*    modify it under the terms of the GNU Lesser General Public    */
/*    License as published by the Free Software Foundation; either  */
/*    version 2.1 of the License, or (at your option) any later     */
/*    version.                                                      */
/*    This library is distributed in the hope that it will be       */
/*    useful, but WITHOUT ANY WARRANTY; without even the implied    */
/*    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/*    PURPOSE. See the GNU Lesser General Public License for more   */
/*    details.                                                      */
/*    You should have received a copy of the GNU Lesser General     */
/*    Public License along with this library; if not, write to the  */
/*    Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,  */
/*    Boston, MA 02110-1301 USA                                     */
/********************************************************************/

#ifndef _METHSERVICE_H_
#define _METHSERVICE_H_

/********************************************************************/
/*                            INCLUDES                              */
/********************************************************************/

/* Use standard types. */
#include <stdint.h>

/********************************************************************/
/*                             DEFINES                              */
/********************************************************************/

/* Enable debug output. */
#define METH_SERVICE_SERIAL_DEBUG

/* The PWM output pin. This drives an N-channel MOSFET which controls */
/* the solenoid. */
#define METH_SERVICE_PWM_OUTPUT_PIN                9

/* The pump control output pin. This drives 5V directly to the speed */
/* control input of the Diener pump's BLDC controller. */
#define METH_SERVICE_PUMP_OUTPUT_PIN               6

/* The failsafe pin. This drives relay 3 on the relay board. */
/* This is actually used as the 24V power enable now, but it still can be */
/* considered a failsafe. */
/* The relay is N-O and it's held closed when in a "good" state. */
/* Therefore, a failure will open the relay and cut power to the pump and */
/* solenoid. */
#define METH_SERVICE_FAILSAFE_OUTPUT_PIN           8

/* The tank sensor input. This pin is pulled high internally and grounded */
/* to indicate closure of the tank level sensor. */
#define METH_SERVICE_TANK_LEVEL_INPUT_PIN          A2

/* The disable input pin. This pin is pulled high internally and grounded */
/* to indicate a user-forced disabling of the meth controller. */
#define METH_SERVICE_DISABLE_INPUT_PIN             A3

/* The rate of which we request the fuel load from the ECU. 50ms = 20Hz. */
#define METH_SERVICE_FUEL_LOAD_REQUEST_PERIOD_MS   50

/* This is how many times we have to have transmitted without receiving */
/* a response before we declare a failure. */
#define METH_SERVICE_FUEL_LOAD_SAMPLE_ERROR_LIMIT  2

/* This is the threshold of when we start spraying (cc/min). */
/* There should ideally be 0% meth delivery rate at the X value after */
/* this point to ensure a gradual transition after the pump turns on. */
#define METH_SERVICE_FUEL_LOAD_UPPER_BOUND         ((double)700.0)
/* --- HYSTERESIS --- */
/* This is the threshold of when we stop spraying (cc/min). */
#define METH_SERVICE_FUEL_LOAD_LOWER_BOUND         ((double)600.0)

/* This is the percentage of meth to inject with respect to the current fuel delivery rate. */
/* The fuel delivery rate is in cc/min TOTAL. */
/* If the rate goes further than the rightmost cell, it uses the last value. */
/* The first value in the reference array MUST BE ZERO! */
#define METH_SERVICE_FUEL_DELIVERY_RATE_ARRAY \
(const double[]){0.0,  240.0,  480.0,  720.0,  960.0,  1200.0,  1440.0,  1680.0,  1920.0,  2160.0,  2400.0,  2640.0}
/* Keep in mind that this is a percentage of cc/min of GASOLINE. Pure methanol has a specific gravity */
/* of .791 while gasoline's is .737. This means that if we are injecting pure meth, and we want to have */
/* 15% of the injected fuel mass be methanol, we only need to inject about 14% of the cc/min. */
/* Normally, this won't matter. We can just think in terms of volume flow rather than mass flow. */
#define METH_SERVICE_DESIRED_METH_FLOW_FUEL_DELIVERY_PERCENTAGE \
(const double[]){0.0,    0.0,    0.0,    0.0,    2.0,     4.0,     6.0,     8.0,    10.0,    12.0,    14.0,    14.0}
/* EST HORSEPOWER  0      43      85     128     170      208      234      265      302      321      352      387 */
/* Horsepower estimates are based off of .5 BSFC in low end to .55 BSFC in high end. */

/* Flag to indicate whether the system will use a progressive spraying scheme or not. */
/* For now, use non-progressive until I can verify the performance of the solenoid. */
//#define METH_SERVICE_PROGRESSIVE_SPRAY

/* Support progressive and non-progressive spray schemes. */
#if defined(METH_SERVICE_PROGRESSIVE_SPRAY)

/* Minimum allowed injector duty cycle. This is used to disable the */
/* non-linear region and prevent fluid from not being atomized during low */
/* demand situations. */
#define METH_SERVICE_MINIMUM_IDC                   0.0

/* This is the mapping of desired meth volume flow rate to output PWM duty. */
/* Ideally, this is linear. We can measure it later... */
/* This is assuming a 1.0MM (M6.0) Aquamist nozzle. */
#define METH_SERVICE_DESIRED_METH_FLOW_CC_MIN_ARRAY \
(const double[]){0.0,   43.2,   86.4,  129.6,  172.8,   216.0,   259.2,   302.4,   345.6,   388.8,   432.0,   480.0}
/* Duty cycle percentage (0-100). */
#define METH_SERVICE_DESIRED_METH_FLOW_PWM_ARRAY \
(const double[]){0.0,    9.0,   18.0,   27.0,   36.0,    45.0,    54.0,    63.0,    72.0,    81.0,    90.0,   100.0}

#else

/* In non-progressive mode, this is used to limit the static flow rate of the nozzle. */
#define METH_SERVICE_MINIMUM_IDC                   100.0

/* When the desired flow rate is >= this value, the solenoid is opened. */
/* A value of 140 should result in the solenoid opening at around 265-270 whp. */
#define METH_SERVICE_NON_PROGRESSIVE_REQ_THRESH    140.0

/* In non-progressive mode, force the table lookup routine to use the last value. */
/* If the target value falls between the last and second to last values, it ends up */
/* being clamped down to 0 by the MINIMUM_IDC check. */
#define METH_SERVICE_DESIRED_METH_FLOW_CC_MIN_ARRAY \
(const double[]){0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, METH_SERVICE_NON_PROGRESSIVE_REQ_THRESH}
/* Duty cycle percentage (0-100). */
#define METH_SERVICE_DESIRED_METH_FLOW_PWM_ARRAY \
(const double[]){0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, METH_SERVICE_MINIMUM_IDC}

#endif

/* We should add another table to compensate for manifold pressure being subtracted from */
/* the effective pump pressure, but we won't. Instead, just assume 15 psi everywhere. */

/* This is the size of the above tables. */
#define METH_SERVICE_TABLE_SIZE                    12

/********************************************************************/
/*                            DATATYPES                             */
/********************************************************************/

/* Meth injection service state. */
typedef enum
{
    METH_SERVICE_STATE_FAILURE,       /* External failure, sample drops, etc... */
    METH_SERVICE_STATE_READY_ARMED,   /* Ready to spray once fuel load is met.  */
    METH_SERVICE_STATE_INJECTING,     /* Currently spraying.                    */
    METH_SERVICE_STATE_DISABLED,      /* User disabled override.                */
    METH_SERVICE_STATE_DISABLED_WARN, /* Disabled, but should be spraying.      */
    METH_SERVICE_STATE_LOW_TANK,      /* Tank is empty.                         */
    METH_SERVICE_STATE_ANY            /* Invalid state.                         */
} MethServiceState_E;

/* Meth injection service events. */
typedef enum
{
    METH_SERVICE_EVENT_DISABLE_TRUE,          /* Disable switch is ON.             */
    METH_SERVICE_EVENT_DISABLE_FALSE,         /* Disable switch is OFF.            */
    METH_SERVICE_EVENT_FUEL_LOAD_ABOVE_UPPER, /* Current fuel load > upper thresh. */
    METH_SERVICE_EVENT_FUEL_LOAD_BELOW_LOWER, /* Current fuel load < lower thresh. */
    METH_SERVICE_EVENT_SAMPLE_TIMEOUT,        /* Sent X requests, got no response. */
    METH_SERVICE_EVENT_EXT_FAILURE,           /* Another controller failed.        */
    METH_SERVICE_EVENT_TANK_EMPTY_TRUE,       /* The tank sensor reports empty.    */
    METH_SERVICE_EVENT_RESTART,               /* Re/Initialize/Start the FSM.      */
    METH_SERVICE_EVENT_POWERUP,               /* Set FSM to known initial state.   */
    METH_SERVICE_EVENT_MSG_RX,                /* Fuel load message received.       */
    METH_SERVICE_EVENT_ANY                    /* Invalid event.                    */
} MethServiceEvent_E;

/* FSM state transition structure. */
typedef struct
{
    MethServiceState_E state;
    MethServiceEvent_E event;
    MethServiceState_E (*func)(void *);
} MethServiceFSMTransition_T;

/********************************************************************/
/*                    GLOBAL FUNCTION PROTOTYPES                    */
/********************************************************************/

/* This function is the "early" init function. */
/* These functions get called for each controller */
/* at the very beginning of boot. */
void MethServiceInitEarly(void);

/* After every controller's early init function has been called, */
/* each controller's "late" function get called. */
/* This function is the last to be executed before entering the */
/* "main" loop. */
/* NOTE: The late function may NEVER get called if the system */
/* fails to initialize, but the Early init function is guaranteed */
/* to get called. A system init failure may call abort() rather than init late. */
void MethServiceInitLate(void);

/* This controller's "thread". NOTE: This isn't a real thread. */
/* We use I guess what can be considered cooperative multitasking. */
/* In other words, don't block in here. */
void MethServiceExec(void);

/* Special function for this controller. This is called by */
/* the main controller when a fuel load message is received. */
/* We CAN modify the contents of the message buffer if needed. */
void MethServiceRXFuelLoadMsg(uint8_t * msg);

/* This is the fail signal handler. */
/* This gets called when another controller (or some other external */
/* influence) has caused a global system failure. */
/* This must stop the controller and leave it in a safe state. */
void MethServiceAbort(void);

/* This function returns the state of the controller. */
MethServiceState_E MethServiceGetState(void);


#endif /* _METHSERVICE_H_ */
