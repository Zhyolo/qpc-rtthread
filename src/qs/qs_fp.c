/*$file${src::qs::qs_fp.c} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*
* Model: qpc.qm
* File:  ${src::qs::qs_fp.c}
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
/*$endhead${src::qs::qs_fp.c} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*! @file
* @brief QS floating point output implementation
*/
#define QP_IMPL           /* this is QP implementation */
#include "qs_port.h"      /* QS port */
#include "qs_pkg.h"       /* QS package-scope internal interface */

/*$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/* Check for the minimum required QP version */
#if (QP_VERSION < 700U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpc version 7.0.0 or higher required
#endif
/*$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/*$define${QS::QS-tx-fp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QS::QS-tx-fp::f32_fmt_} ................................................*/
/*! @static @private @memberof QS_tx */
void QS_f32_fmt_(
    uint8_t const format,
    float32_t const d)
{
    union F32Rep {
        float32_t f;
        uint32_t  u;
    } fu32;  /* the internal binary representation */
    uint8_t chksum      = QS_priv_.chksum; /* put in a temporary (register) */
    uint8_t * const buf = QS_priv_.buf;
    QSCtr head          = QS_priv_.head;
    QSCtr const end     = QS_priv_.end;
    uint_fast8_t i;

    fu32.f = d; /* assign the binary representation */

    QS_priv_.used += 5U; /* 5 bytes about to be added */
    QS_INSERT_ESC_BYTE_(format) /* insert the format byte */

    /* insert 4 bytes... */
    for (i = 4U; i != 0U; --i) {
        QS_INSERT_ESC_BYTE_((uint8_t)fu32.u)
        fu32.u >>= 8U;
    }

    QS_priv_.head   = head;   /* save the head */
    QS_priv_.chksum = chksum; /* save the checksum */
}

/*${QS::QS-tx-fp::f64_fmt_} ................................................*/
/*! @static @private @memberof QS_tx */
void QS_f64_fmt_(
    uint8_t const format,
    float64_t const d)
{
    union F64Rep {
        float64_t d;
        uint32_t  u[2];
    } fu64; /* the internal binary representation */
    uint8_t chksum      = QS_priv_.chksum;
    uint8_t * const buf = QS_priv_.buf;
    QSCtr head          = QS_priv_.head;
    QSCtr const end     = QS_priv_.end;
    uint32_t i;

    /* static constant untion to detect endianness of the machine */
    static union U32Rep {
        uint32_t u32;
        uint8_t  u8;
    } const endian = { 1U };

    fu64.d = d; /* assign the binary representation */

    /* is this a big-endian machine? */
    if (endian.u8 == 0U) {
        /* swap fu64.u[0] <-> fu64.u[1]... */
        i = fu64.u[0];
        fu64.u[0] = fu64.u[1];
        fu64.u[1] = i;
    }

    QS_priv_.used += 9U; /* 9 bytes about to be added */
    QS_INSERT_ESC_BYTE_(format) /* insert the format byte */

    /* output 4 bytes from fu64.u[0]... */
    for (i = 4U; i != 0U; --i) {
        QS_INSERT_ESC_BYTE_((uint8_t)fu64.u[0])
        fu64.u[0] >>= 8U;
    }

    /* output 4 bytes from fu64.u[1]... */
    for (i = 4U; i != 0U; --i) {
        QS_INSERT_ESC_BYTE_((uint8_t)fu64.u[1])
        fu64.u[1] >>= 8U;
    }

    QS_priv_.head   = head;   /* save the head */
    QS_priv_.chksum = chksum; /* save the checksum */
}
/*$enddef${QS::QS-tx-fp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
