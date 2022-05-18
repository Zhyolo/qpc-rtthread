/**
* @file
* @brief QF/C, port to RT-Thread
* @ingroup ports
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
* along with this program. If not, see <www.gnu.org/licenses/>.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
******************************************************************************
* @endcond
*/
#define QP_IMPL           /* this is QP implementation */
#include "qf_port.h"      /* QF port */
#include "qf_pkg.h"
#include "qassert.h"
#ifdef Q_SPY              /* QS software tracing enabled? */
    #include "qs_port.h"  /* QS port */
    #include "qs_pkg.h"   /* QS package-scope internal interface */
#else
    #include "qs_dummy.h" /* disable the QS software tracing */
#endif /* Q_SPY */

Q_DEFINE_THIS_MODULE("qf_port")

/*..........................................................................*/
void QF_init(void) {
}
/*..........................................................................*/
int_t QF_run(void) {
    QS_CRIT_STAT_

    QF_onStartup();  /* QF callback to configure and start interrupts */

    /* produce the QS_QF_RUN trace record */
    QS_BEGIN_PRE_(QS_QF_RUN, 0U)
    QS_END_PRE_()

    return 0; /* return success */
}
/*..........................................................................*/
void QF_stop(void) {
    QF_onCleanup(); /* cleanup callback */
}
/*..........................................................................*/
static void thread_function(void *parameter) { /* RT-Thread signature */
    QActive *act = (QActive *)parameter;

    /* event-loop */
    for (;;) { /* for-ever */
        QEvt const *e = QActive_get_(act);
        QHSM_DISPATCH(&act->super, e, act->prio);
        QF_gc(e); /* check if the event is garbage, and collect it if so */
    }
}

/*..........................................................................*/
void QActive_start_(QActive * const me, uint_fast8_t prio,
                    QEvt const * * const qSto, uint_fast16_t const qLen,
                    void * const stkSto, uint_fast16_t const stkSize,
                    void const * const par)
{
    rt_uint8_t tx_prio; /* RT-Thread priority corresponding to the QF priority prio */

    /* allege that the RT-Thread queue is created successfully */
    Q_ALLEGE_ID(210,
        rt_mb_init(&me->eQueue,
            me->thread.name,
            (void *)qSto,
            (qLen),
            RT_IPC_FLAG_FIFO)
        == RT_EOK);

    me->prio = prio;  /* save the QF priority */
    QF_add_(me);      /* make QF aware of this active object */

    QHSM_INIT(&me->super, par, me->prio); /* initial tran. (virtual) */
    QS_FLUSH(); /* flush the trace buffer to the host */

    /* convert QF priority to the RT-Thread priority */
    tx_prio = QF_MAX_ACTIVE - prio;

    Q_ALLEGE_ID(220,
        rt_thread_init(
            &me->thread, /* RT-Thread thread control block */
            me->thread.name, /* unique thread name */
            &thread_function, /* thread function */
            me, /* thread parameter */
            stkSto,    /* stack start */
            stkSize,   /* stack size in bytes */
            tx_prio,   /* RT-Thread priority */
            5)
        == RT_EOK);
    rt_thread_startup(&me->thread);
}
/*..........................................................................*/
void QActive_setAttr(QActive *const me, uint32_t attr1, void const *attr2) {
    /* this function must be called before QACTIVE_START(),
    */
    switch (attr1) {
        case THREAD_NAME_ATTR:
            rt_memset(me->thread.name, 0x00, RT_NAME_MAX);
            rt_strncpy(me->thread.name, (char *)attr2, RT_NAME_MAX - 1);
            break;
        /* ... */
    }
}
/*..........................................................................*/
#ifndef Q_SPY
bool QActive_post_(QActive * const me, QEvt const * const e,
                   uint_fast16_t const margin)
#else
bool QActive_post_(QActive * const me, QEvt const * const e,
                   uint_fast16_t const margin, void const * const sender)
#endif /* Q_SPY */
{
    uint_fast16_t nFree;
    bool status;
    QF_CRIT_STAT_

    QF_CRIT_E_();
    nFree = (uint_fast16_t)(me->eQueue.size - me->eQueue.entry);

    if (margin == QF_NO_MARGIN) {
        if (nFree > 0U) {
            status = true; /* can post */
        }
        else {
            status = false; /* cannot post */
            Q_ERROR_ID(510); /* must be able to post the event */
        }
    }
    else if (nFree > (QEQueueCtr)margin) {
        status = true; /* can post */
    }
    else {
        status = false; /* cannot post */
    }

    if (status) { /* can post the event? */

        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST, me->prio)
            QS_TIME_PRE_();       /* timestamp */
            QS_OBJ_PRE_(sender);  /* the sender object */
            QS_SIG_PRE_(e->sig);  /* the signal of the event */
            QS_OBJ_PRE_(me);      /* this active object (recipient) */
            QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
            QS_EQC_PRE_(nFree);   /* # free entries available */
            QS_EQC_PRE_(0U);      /* min # free entries (unknown) */
        QS_END_NOCRIT_PRE_()

        if (e->poolId_ != 0U) { /* is it a pool event? */
            QF_EVT_REF_CTR_INC_(e); /* increment the reference counter */
        }

        QF_CRIT_X_();

        /* posting to the RT-Thread message queue must succeed, see NOTE1 */
        Q_ALLEGE_ID(520,
            rt_mb_send(&me->eQueue, (rt_ubase_t)e)
            == RT_EOK);
    }
    else {

        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST_ATTEMPT, me->prio)
            QS_TIME_PRE_();       /* timestamp */
            QS_OBJ_PRE_(sender);  /* the sender object */
            QS_SIG_PRE_(e->sig);  /* the signal of the event */
            QS_OBJ_PRE_(me);      /* this active object (recipient) */
            QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
            QS_EQC_PRE_(nFree);   /* # free entries available */
            QS_EQC_PRE_(0U);      /* min # free entries (unknown) */
        QS_END_NOCRIT_PRE_()

        QF_CRIT_X_();
    }

    return status;
}
/*..........................................................................*/
void QActive_postLIFO_(QActive * const me, QEvt const * const e) {
    QF_CRIT_STAT_
    QF_CRIT_E_();

    QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST_LIFO, me->prio)
        QS_TIME_PRE_();       /* timestamp */
        QS_SIG_PRE_(e->sig);  /* the signal of this event */
        QS_OBJ_PRE_(me);      /* this active object */
        QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
        QS_EQC_PRE_(me->eQueue.size - me->eQueue.entry); /* # free */
        QS_EQC_PRE_(0U);      /* min # free entries (unknown) */
    QS_END_NOCRIT_PRE_()

    if (e->poolId_ != 0U) { /* is it a pool event? */
        QF_EVT_REF_CTR_INC_(e); /* increment the reference counter */
    }

    QF_CRIT_X_();

    /* LIFO posting must succeed, see NOTE1 */
    Q_ALLEGE_ID(610,
        rt_mb_urgent(&me->eQueue, (rt_ubase_t)e)
        == RT_EOK);
}
/*..........................................................................*/
QEvt const *QActive_get_(QActive * const me) {
    QEvt *e;
    QS_CRIT_STAT_

    Q_ALLEGE_ID(710,
        rt_mb_recv(&me->eQueue, (rt_ubase_t *)&e, RT_WAITING_FOREVER)
        == RT_EOK);

    QS_BEGIN_PRE_(QS_QF_ACTIVE_GET, me->prio)
        QS_TIME_PRE_();       /* timestamp */
        QS_SIG_PRE_(e->sig);  /* the signal of this event */
        QS_OBJ_PRE_(me);      /* this active object */
        QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
        QS_EQC_PRE_(me->eQueue.size - me->eQueue.entry);/* # free */
    QS_END_PRE_()

    return e;
}
