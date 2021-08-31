/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
// Xtensa-Windowed specific stuff

#include "py/mpconfig.h"

#if MICROPY_EMIT_XTENSAWIN

// this is defined so that the assembler exports generic assembler API macros
#define GENERIC_ASM_API (1)
#define GENERIC_ASM_API_WIN (1)
#include "py/asmxtensa.h"

// Word indices of REG_LOCAL_x in nlr_buf_t
#define NLR_BUF_IDX_LOCAL_1 (2 + 4) // a4
#define NLR_BUF_IDX_LOCAL_2 (2 + 5) // a5
#define NLR_BUF_IDX_LOCAL_3 (2 + 6) // a6

#define N_NLR_SETJMP (1)
#define N_PRELUDE_AS_BYTES_OBJ (1)
#define N_XTENSAWIN (1)
#define EXPORT_FUN(name) emit_native_xtensawin_##name
#include "py/emitnative.c"

#endif