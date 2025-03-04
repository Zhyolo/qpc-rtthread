/*$file${src::qf::qf_actq.c} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*
* Model: qpc.qm
* File:  ${src::qf::qf_actq.c}
*
* This code has been generated by QM 5.2.5 <www.state-machine.com/qm>.
* DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
*
* This code is covered by the following QP license:
* License #    : LicenseRef-QL-dual
* Issued to    : Any user of the QP/C real-time embedded framework
* Framework(s) : qpc
* Support ends : 2023-12-31
* License scope:
*
* Copyright (C) 2005 Quantum Leaps, LLC <state-machine.com>.
*
* SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
*
* This software is dual-licensed under the terms of the open source GNU
* General Public License version 3 (or any later version), or alternatively,
* under the terms of one of the closed source Quantum Leaps commercial
* licenses.
*
* The terms of the open source GNU General Public License version 3
* can be found at: <www.gnu.org/licenses/gpl-3.0>
*
* The terms of the closed source Quantum Leaps commercial licenses
* can be found at: <www.state-machine.com/licensing>
*
* Redistributions in source code must retain this top-level comment block.
* Plagiarizing this software to sidestep the license obligations is illegal.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
*/
/*$endhead${src::qf::qf_actq.c} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*! @file
* @brief ::QActive native queue operations (based on ::QEQueue)
*
* @note
* This `qf_actq.c` source file needs to be included in the application
* build only when the native ::QEQueue queue is used for ::QActive objects
* (instead of a message queue of an RTOS).
*/
#define QP_IMPL           /* this is QP implementation */
#include "qf_port.h"      /* QF port */
#include "qf_pkg.h"       /* QF package-scope interface */
#include "qassert.h"      /* QP embedded systems-friendly assertions */
#ifdef Q_SPY              /* QS software tracing enabled? */
    #include "qs_port.h"  /* QS port */
    #include "qs_pkg.h"   /* QS facilities for pre-defined trace records */
#else
    #include "qs_dummy.h" /* disable the QS software tracing */
#endif /* Q_SPY */

Q_DEFINE_THIS_MODULE("qf_actq")

/*==========================================================================*/
/*$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/* Check for the minimum required QP version */
#if (QP_VERSION < 700U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpc version 7.0.0 or higher required
#endif
/*$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/*$define${QF::QActive::post_} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QF::QActive::post_} ....................................................*/
/*! @private @memberof QActive */
bool QActive_post_(QActive * const me,
    QEvt const * const e,
    uint_fast16_t const margin,
    void const * const sender)
{
    #ifndef Q_SPY
    Q_UNUSED_PAR(sender);
    #endif

    Q_REQUIRE_ID(100, e != (QEvt *)0);

    QF_CRIT_STAT_
    QF_CRIT_E_();
    QEQueueCtr nFree = me->eQueue.nFree; /* get volatile into temporary */

    /* test-probe#1 for faking queue overflow */
    QS_TEST_PROBE_DEF(&QActive_post_)
    QS_TEST_PROBE_ID(1,
        nFree = 0U;
    )

    bool status;
    if (margin == QF_NO_MARGIN) {
        if (nFree > 0U) {
            status = true; /* can post */
        }
        else {
            status = false; /* cannot post */
            Q_ERROR_CRIT_(190); /* must be able to post the event */
        }
    }
    else if (nFree > (QEQueueCtr)margin) {
        status = true; /* can post */
    }
    else {
        status = false; /* cannot post, but don't assert */
    }

    /* is it a dynamic event? */
    if (e->poolId_ != 0U) {
        QEvt_refCtr_inc_(e); /* increment the reference counter */
    }

    if (status) { /* can post the event? */

        --nFree; /* one free entry just used up */
        me->eQueue.nFree = nFree; /* update the original */
        if (me->eQueue.nMin > nFree) {
            me->eQueue.nMin = nFree; /* increase minimum so far */
        }

        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST, me->prio)
            QS_TIME_PRE_();               /* timestamp */
            QS_OBJ_PRE_(sender);          /* the sender object */
            QS_SIG_PRE_(e->sig);          /* the signal of the event */
            QS_OBJ_PRE_(me);              /* this active object (recipient) */
            QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
            QS_EQC_PRE_(nFree);           /* number of free entries */
            QS_EQC_PRE_(me->eQueue.nMin); /* min number of free entries */
        QS_END_NOCRIT_PRE_()

    #ifdef Q_UTEST
        /* callback to examine the posted event under the same conditions
        * as producing the #QS_QF_ACTIVE_POST trace record, which are:
        * the local filter for this AO ('me->prio') is set
        */
        if (QS_LOC_CHECK_(me->prio)) {
            /* callback to examine the posted event */
            QS_onTestPost(sender, me, e, status);
        }
    #endif

        /* empty queue? */
        if (me->eQueue.frontEvt == (QEvt *)0) {
            me->eQueue.frontEvt = e;    /* deliver event directly */
            QACTIVE_EQUEUE_SIGNAL_(me); /* signal the event queue */
        }
        /* queue is not empty, insert event into the ring-buffer */
        else {
            /* insert event into the ring buffer (FIFO) */
            me->eQueue.ring[me->eQueue.head] = e;

            if (me->eQueue.head == 0U) { /* need to wrap head? */
                me->eQueue.head = me->eQueue.end;   /* wrap around */
            }
            --me->eQueue.head; /* advance the head (counter clockwise) */
        }

        QF_CRIT_X_();
    }
    else { /* cannot post the event */

        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST_ATTEMPT, me->prio)
            QS_TIME_PRE_();       /* timestamp */
            QS_OBJ_PRE_(sender);  /* the sender object */
            QS_SIG_PRE_(e->sig);  /* the signal of the event */
            QS_OBJ_PRE_(me);      /* this active object (recipient) */
            QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
            QS_EQC_PRE_(nFree);   /* number of free entries */
            QS_EQC_PRE_(margin);  /* margin requested */
        QS_END_NOCRIT_PRE_()

    #ifdef Q_UTEST
        /* callback to examine the posted event under the same conditions
        * as producing the #QS_QF_ACTIVE_POST trace record, which are:
        * the local filter for this AO ('me->prio') is set
        */
        if (QS_LOC_CHECK_(me->prio)) {
            QS_onTestPost(sender, me, e, status);
        }
    #endif

        QF_CRIT_X_();

    #if (QF_MAX_EPOOL > 0U)
        QF_gc(e); /* recycle the event to avoid a leak */
    #endif
    }

    return status;
}
/*$enddef${QF::QActive::post_} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*$define${QF::QActive::postLIFO_} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QF::QActive::postLIFO_} ................................................*/
/*! @private @memberof QActive */
void QActive_postLIFO_(QActive * const me,
    QEvt const * const e)
{
    QF_CRIT_STAT_
    QF_CRIT_E_();
    QEQueueCtr nFree = me->eQueue.nFree; /* get volatile into temporary */

    /* test-probe#1 for faking queue overflow */
    QS_TEST_PROBE_DEF(&QActive_postLIFO_)
    QS_TEST_PROBE_ID(1,
        nFree = 0U;
    )

    Q_REQUIRE_CRIT_(200, nFree != 0U);

    /* is it a dynamic event? */
    if (e->poolId_ != 0U) {
        QEvt_refCtr_inc_(e); /* increment the reference counter */
    }

    --nFree; /* one free entry just used up */
    me->eQueue.nFree = nFree; /* update the original */
    if (me->eQueue.nMin > nFree) {
        me->eQueue.nMin = nFree; /* update minimum so far */
    }

    QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST_LIFO, me->prio)
        QS_TIME_PRE_();      /* timestamp */
        QS_SIG_PRE_(e->sig); /* the signal of this event */
        QS_OBJ_PRE_(me);     /* this active object */
        QS_2U8_PRE_(e->poolId_, e->refCtr_);/* pool Id & ref Count */
        QS_EQC_PRE_(nFree);  /* # free entries */
        QS_EQC_PRE_(me->eQueue.nMin); /* min number of free entries */
    QS_END_NOCRIT_PRE_()

    #ifdef Q_UTEST
        /* callback to examine the posted event under the same conditions
        * as producing the #QS_QF_ACTIVE_POST trace record, which are:
        * the local filter for this AO ('me->prio') is set
        */
        if (QS_LOC_CHECK_(me->prio)) {
            QS_onTestPost((QActive *)0, me, e, true);
        }
    #endif

    QEvt const * const frontEvt  = me->eQueue.frontEvt;
    me->eQueue.frontEvt = e; /* deliver the event directly to the front */

    /* was the queue empty? */
    if (frontEvt == (QEvt *)0) {
        QACTIVE_EQUEUE_SIGNAL_(me); /* signal the event queue */
    }
    /* queue was not empty, leave the event in the ring-buffer */
    else {
        ++me->eQueue.tail;
        /* need to wrap the tail? */
        if (me->eQueue.tail == me->eQueue.end) {
            me->eQueue.tail = 0U; /* wrap around */
        }

        me->eQueue.ring[me->eQueue.tail] = frontEvt;
    }
    QF_CRIT_X_();
}
/*$enddef${QF::QActive::postLIFO_} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*$define${QF::QActive::get_} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QF::QActive::get_} .....................................................*/
/*! @private @memberof QActive */
QEvt const * QActive_get_(QActive * const me) {
    QF_CRIT_STAT_
    QF_CRIT_E_();
    QACTIVE_EQUEUE_WAIT_(me);  /* wait for event to arrive directly */

    /* always remove event from the front */
    QEvt const * const e = me->eQueue.frontEvt;
    QEQueueCtr const nFree = me->eQueue.nFree + 1U; /* get volatile into tmp */
    me->eQueue.nFree = nFree; /* update the number of free */

    /* any events in the ring buffer? */
    if (nFree <= me->eQueue.end) {

        /* remove event from the tail */
        me->eQueue.frontEvt = me->eQueue.ring[me->eQueue.tail];
        if (me->eQueue.tail == 0U) { /* need to wrap the tail? */
            me->eQueue.tail = me->eQueue.end;   /* wrap around */
        }
        --me->eQueue.tail;

        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_GET, me->prio)
            QS_TIME_PRE_();      /* timestamp */
            QS_SIG_PRE_(e->sig); /* the signal of this event */
            QS_OBJ_PRE_(me);     /* this active object */
            QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
            QS_EQC_PRE_(nFree);  /* # free entries */
        QS_END_NOCRIT_PRE_()
    }
    else {
        me->eQueue.frontEvt = (QEvt *)0; /* queue becomes empty */

        /* all entries in the queue must be free (+1 for fronEvt) */
        Q_ASSERT_CRIT_(310, nFree == (me->eQueue.end + 1U));

        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_GET_LAST, me->prio)
            QS_TIME_PRE_();      /* timestamp */
            QS_SIG_PRE_(e->sig); /* the signal of this event */
            QS_OBJ_PRE_(me);     /* this active object */
            QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
        QS_END_NOCRIT_PRE_()
    }
    QF_CRIT_X_();
    return e;
}
/*$enddef${QF::QActive::get_} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/*$define${QF::QF-base::getQueueMin} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QF::QF-base::getQueueMin} ..............................................*/
/*! @static @public @memberof QF */
uint_fast16_t QF_getQueueMin(uint_fast8_t const prio) {
    Q_REQUIRE_ID(400, (prio <= QF_MAX_ACTIVE)
                      && (QActive_registry_[prio] != (QActive *)0));
    QF_CRIT_STAT_
    QF_CRIT_E_();
    uint_fast16_t const min =
         (uint_fast16_t)QActive_registry_[prio]->eQueue.nMin;
    QF_CRIT_X_();

    return min;
}
/*$enddef${QF::QF-base::getQueueMin} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/*==========================================================================*/
/*! Perform downcast to QTicker pointer.
*
* @details
* This macro encapsulates the downcast to (QTicker *), which is used in
* QTicker_init_() and QTicker_dispatch_(). Such casts violate MISRA-C 2012
* Rule 11.3(req) "cast from pointer to object type to pointer to different
* object type".
*/
#define QTICKER_CAST_(me_)  ((QActive *)(me_))

/*$define${QF::QTicker} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QF::QTicker} ...........................................................*/

/*${QF::QTicker::ctor} .....................................................*/
/*! @public @memberof QTicker */
void QTicker_ctor(QTicker * const me,
    uint_fast8_t const tickRate)
{
    static QActiveVtable const vtable = {  /* QActive virtual table */
        { &QTicker_init_,
          &QTicker_dispatch_
    #ifdef Q_SPY
          ,&QHsm_getStateHandler_
    #endif
        },
        &QActive_start_,
        &QTicker_post_,
        &QTicker_postLIFO_
    };
    QActive_ctor(&me->super, Q_STATE_CAST(0)); /* superclass' ctor */
    me->super.super.vptr = &vtable.super; /* hook the vptr */

    /* reuse eQueue.head for tick-rate */
    me->super.eQueue.head = (QEQueueCtr)tickRate;
}

/*${QF::QTicker::init_} ....................................................*/
/*! @private @memberof QTicker */
void QTicker_init_(
    QHsm * const me,
    void const * const par,
    uint_fast8_t const qs_id)
{
    Q_UNUSED_PAR(me);
    Q_UNUSED_PAR(par);
    Q_UNUSED_PAR(qs_id);

    QTICKER_CAST_(me)->eQueue.tail = 0U;
}

/*${QF::QTicker::dispatch_} ................................................*/
/*! @private @memberof QTicker */
void QTicker_dispatch_(
    QHsm * const me,
    QEvt const * const e,
    uint_fast8_t const qs_id)
{
    Q_UNUSED_PAR(e);
    Q_UNUSED_PAR(qs_id);

    QF_CRIT_STAT_
    QF_CRIT_E_();
    QEQueueCtr nTicks = QTICKER_CAST_(me)->eQueue.tail; /* save # of ticks */
    QTICKER_CAST_(me)->eQueue.tail = 0U; /* clear # ticks */
    QF_CRIT_X_();

    for (; nTicks > 0U; --nTicks) {
        QTimeEvt_tick_((uint_fast8_t)QTICKER_CAST_(me)->eQueue.head, me);
    }
}

/*${QF::QTicker::post_} ....................................................*/
/*! @private @memberof QTicker */
bool QTicker_post_(
    QActive * const me,
    QEvt const * const e,
    uint_fast16_t const margin,
    void const * const sender)
{
    Q_UNUSED_PAR(e);
    Q_UNUSED_PAR(margin);
    #ifndef Q_SPY
    Q_UNUSED_PAR(sender);
    #endif

    QF_CRIT_STAT_
    QF_CRIT_E_();
    if (me->eQueue.frontEvt == (QEvt *)0) {

        static QEvt const tickEvt = { 0U, 0U, 0U };
        me->eQueue.frontEvt = &tickEvt; /* deliver event directly */
        --me->eQueue.nFree; /* one less free event */

        QACTIVE_EQUEUE_SIGNAL_(me); /* signal the event queue */
    }

    ++me->eQueue.tail; /* account for one more tick event */

    QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST, me->prio)
        QS_TIME_PRE_();      /* timestamp */
        QS_OBJ_PRE_(sender); /* the sender object */
        QS_SIG_PRE_(0U);     /* the signal of the event */
        QS_OBJ_PRE_(me);     /* this active object */
        QS_2U8_PRE_(0U, 0U); /* pool Id & refCtr of the evt */
        QS_EQC_PRE_(0U);     /* number of free entries */
        QS_EQC_PRE_(0U);     /* min number of free entries */
    QS_END_NOCRIT_PRE_()

    QF_CRIT_X_();

    return true; /* the event is always posted correctly */
}

/*${QF::QTicker::postLIFO_} ................................................*/
/*! @private @memberof QTicker */
void QTicker_postLIFO_(
    QActive * const me,
    QEvt const * const e)
{
    Q_UNUSED_PAR(me);
    Q_UNUSED_PAR(e);

    Q_ERROR_ID(900);
}
/*$enddef${QF::QTicker} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
