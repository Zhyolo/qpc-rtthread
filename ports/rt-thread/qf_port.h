/**
* @file
* @brief QF/C, port to RT-Thread
* @cond
******************************************************************************
* Last updated for version 6.9.3
* Last updated on  2021-04-08
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2005-2021 Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Alternatively, this program may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GNU General Public License and are specifically designed for
* licensees interested in retaining the proprietary status of their code.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <www.gnu.org/licenses>.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
******************************************************************************
* @endcond
*/
#ifndef QF_PORT_H
#define QF_PORT_H

/* RT-Thread event queue and thread types */
#define QF_EQUEUE_TYPE      struct rt_mailbox
#define QF_THREAD_TYPE      struct rt_thread

/* The maximum number of active objects in the application, see NOTE2 */
#define QF_MAX_ACTIVE       (RT_THREAD_PRIORITY_MAX)

/* QF critical section for RT-Thread, see NOTE3 */
#define QF_CRIT_ENTRY(stat_)  (rt_enter_critical())
#define QF_CRIT_EXIT(stat_)   (rt_exit_critical())

enum RT_Thread_ThreadAttrs {
    THREAD_NAME_ATTR
};

#include <rtthread.h>   /* RT-Thread API */

#include "qep_port.h"  /* QEP port */
#include "qequeue.h"   /* used for event deferral */
#include "qf.h"        /* QF platform-independent public interface */
#ifdef Q_SPY
#include "qmpool.h"    /* needed only for QS-RX */
#endif

/*****************************************************************************
* interface used only inside QF, but not in applications
*/
#ifdef QP_IMPL

    #define QF_SCHED_STAT_
    #define QF_SCHED_LOCK_(prio_)   rt_enter_critical()
    #define QF_SCHED_UNLOCK_()      rt_exit_critical()

    /* TreadX block pool operations... */
    #define QF_EPOOL_TYPE_              struct rt_mempool
    #define QF_EPOOL_INIT_(pool_, poolSto_, poolSize_, evtSize_)            \
        Q_ALLEGE(rt_mp_init(&(pool_), (char *)"QP",                         \
                 (poolSto_), (poolSize_), (evtSize_)) == RT_EOK)

    #define QF_EPOOL_EVENT_SIZE_(pool_)                                     \
        ((uint_fast16_t)(pool_).block_size)

    #define QF_EPOOL_GET_(pool_, e_, margin_, qs_id_) do {                  \
        if ((pool_).block_free_count > (margin_)) {                         \
            e_ = rt_mp_alloc(&(pool_), RT_WAITING_NO);                      \
        }                                                                   \
        else {                                                              \
            (e_) = (QEvt *)0;                                               \
        }                                                                   \
    } while (false)

    #define QF_EPOOL_PUT_(dummy, e_, qs_id_)                                \
        rt_mp_free((void *)(e_));

#endif /* ifdef QP_IMPL */

#endif /* QF_PORT_H */

