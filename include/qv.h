/*$file${include::qv.h} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*
* Model: qpc.qm
* File:  ${include::qv.h}
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
/*$endhead${include::qv.h} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*! @file
* @brief QV/C (cooperative "Vanilla" kernel) platform-independent
* public interface
*/
#ifndef QV_H_
#define QV_H_

/*==========================================================================*/
/* QF customization for QV -- data members of the QActive class... */

/* QV event-queue used for AOs */
#define QF_EQUEUE_TYPE  QEQueue

/*==========================================================================*/
#include "qequeue.h"  /* QV kernel uses the native QP event queue */
#include "qmpool.h"   /* QV kernel uses the native QP memory pool */
#include "qf.h"       /* QF framework integrates directly with QV */

//============================================================================
/*$declare${QV::QV-base} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QV::QV-base::Attr} .....................................................*/
/*! @brief QV cooperative kernel
* @class QV
*/
typedef struct QV_Attr {
    uint8_t dummy; /*< dummy attribute */
} QV;

/*${QV::QV-base::onIdle} ...................................................*/
/*! QV idle callback (customized in BSPs)
* @static @public @memberof QV
*
* @details
* QV_onIdle() is called by the cooperative QV kernel (from QF_run()) when
* the scheduler detects that no events are available for active objects
* (the idle condition). This callback gives the application an opportunity
* to enter a power-saving CPU mode, or perform some other idle processing
* (such as QS software tracing output).
*
* @attention
* QV_onIdle() is invoked with interrupts **DISABLED** because the idle
* condition can be asynchronously changed at any time by an interrupt.
* QV_onIdle() MUST enable the interrupts internally, but not before
* putting the CPU into the low-power mode. (Ideally, enabling interrupts and
* low-power mode should happen atomically). At the very least, the function
* MUST enable interrupts, otherwise interrupts will remain disabled
* permanently.
*/
void QV_onIdle(void);
/*$enddecl${QV::QV-base} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/*==========================================================================*/
/* interface used only inside QF, but not in applications */
#ifdef QP_IMPL
/* QV-specific scheduler locking and event queue... */
/*$declare${QV-impl} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QV-impl::QF_SCHED_STAT_} ...............................................*/
/*! QV scheduler lock status (not needed in QV) */
#define QF_SCHED_STAT_

/*${QV-impl::QF_SCHED_LOCK_} ...............................................*/
/*! QV scheduler locking (not needed in QV) */
#define QF_SCHED_LOCK_(dummy) ((void)0)

/*${QV-impl::QF_SCHED_UNLOCK_} .............................................*/
/*! QV scheduler unlocking (not needed in QV) */
#define QF_SCHED_UNLOCK_() ((void)0)

/*${QV-impl::QACTIVE_EQUEUE_WAIT_} .........................................*/
#define QACTIVE_EQUEUE_WAIT_(me_) \
    Q_ASSERT_ID(0, (me_)->eQueue.frontEvt != (QEvt *)0)

/*${QV-impl::QACTIVE_EQUEUE_SIGNAL_} .......................................*/
/*! QV native event queue signaling */
#define QACTIVE_EQUEUE_SIGNAL_(me_) \
    QPSet_insert(&QF_readySet_, (uint_fast8_t)(me_)->prio)
/*$enddecl${QV-impl} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/* Native QF event pool operations... */
/*$declare${QF-QMPool-impl} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QF-QMPool-impl::QF_EPOOL_TYPE_} ........................................*/
#define QF_EPOOL_TYPE_ QMPool

/*${QF-QMPool-impl::QF_EPOOL_INIT_} ........................................*/
#define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
    (QMPool_init(&(p_), (poolSto_), (poolSize_), (evtSize_)))

/*${QF-QMPool-impl::QF_EPOOL_EVENT_SIZE_} ..................................*/
#define QF_EPOOL_EVENT_SIZE_(p_) ((uint_fast16_t)(p_).blockSize)

/*${QF-QMPool-impl::QF_EPOOL_GET_} .........................................*/
#define QF_EPOOL_GET_(p_, e_, m_, qs_id_) \
    ((e_) = (QEvt *)QMPool_get(&(p_), (m_), (qs_id_)))

/*${QF-QMPool-impl::QF_EPOOL_PUT_} .........................................*/
#define QF_EPOOL_PUT_(p_, e_, qs_id_) \
    (QMPool_put(&(p_), (e_), (qs_id_)))
/*$enddecl${QF-QMPool-impl} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
#endif /* QP_IMPL */

#endif /* QV_H_ */
