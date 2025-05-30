/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
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

#include <assert.h>
#include <string.h>

#include "py/obj.h"
#include "py/mperrno.h"

#if MICROPY_PY_ERRNO

// This list can be defined per port in mpconfigport.h to tailor it to a
// specific port's needs.  If it's not defined then we provide a default.
#ifndef MICROPY_PY_ERRNO_LIST
// CIRCUITPY-CHANGE: add ENOSPC and EROFS, because they are in mp_common_errno_to_str().
#define MICROPY_PY_ERRNO_LIST \
    X(EPERM) \
    X(ENOENT) \
    X(EIO) \
    X(EBADF) \
    X(EAGAIN) \
    X(ENOMEM) \
    X(EACCES) \
    X(EEXIST) \
    X(ENODEV) \
    X(EISDIR) \
    X(EINVAL) \
    X(ENOSPC) \
    X(EROFS) \
    X(EOPNOTSUPP) \
    X(EADDRINUSE) \
    X(ECONNABORTED) \
    X(ECONNRESET) \
    X(ENOBUFS) \
    X(ENOTCONN) \
    X(ETIMEDOUT) \
    X(ECONNREFUSED) \
    X(EHOSTUNREACH) \
    X(EALREADY) \
    X(EINPROGRESS) \

#endif

#if MICROPY_PY_ERRNO_ERRORCODE
static const mp_rom_map_elem_t errorcode_table[] = {
    #define X(e) { MP_ROM_INT(MP_##e), MP_ROM_QSTR(MP_QSTR_##e) },
    MICROPY_PY_ERRNO_LIST
#undef X
};

static const mp_obj_dict_t errorcode_dict = {
    .base = {&mp_type_dict},
    .map = {
        .all_keys_are_qstrs = 0, // keys are integers
        .is_fixed = 1,
        .is_ordered = 1,
        .used = MP_ARRAY_SIZE(errorcode_table),
        .alloc = MP_ARRAY_SIZE(errorcode_table),
        .table = (mp_map_elem_t *)(mp_rom_map_elem_t *)errorcode_table,
    },
};
#endif

static const mp_rom_map_elem_t mp_module_errno_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_errno) },
    #if MICROPY_PY_ERRNO_ERRORCODE
    { MP_ROM_QSTR(MP_QSTR_errorcode), MP_ROM_PTR(&errorcode_dict) },
    #endif

    #define X(e) { MP_ROM_QSTR(MP_QSTR_##e), MP_ROM_INT(MP_##e) },
    MICROPY_PY_ERRNO_LIST
#undef X
};

static MP_DEFINE_CONST_DICT(mp_module_errno_globals, mp_module_errno_globals_table);

const mp_obj_module_t mp_module_errno = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_errno_globals,
};

MP_REGISTER_EXTENSIBLE_MODULE(MP_QSTR_errno, mp_module_errno);

qstr mp_errno_to_str(mp_obj_t errno_val) {
    #if MICROPY_PY_ERRNO_ERRORCODE
    // We have the errorcode dict so can do a lookup using the hash map
    mp_map_elem_t *elem = mp_map_lookup((mp_map_t *)&errorcode_dict.map, errno_val, MP_MAP_LOOKUP);
    if (elem == NULL) {
        return MP_QSTRnull;
    } else {
        return MP_OBJ_QSTR_VALUE(elem->value);
    }
    #else
    // We don't have the errorcode dict so do a simple search in the modules dict
    for (size_t i = 0; i < MP_ARRAY_SIZE(mp_module_errno_globals_table); ++i) {
        if (errno_val == mp_module_errno_globals_table[i].value) {
            return MP_OBJ_QSTR_VALUE(mp_module_errno_globals_table[i].key);
        }
    }
    return MP_QSTRnull;
    #endif
}

// CIRCUITPY-CHANGE
// For commonly encountered errors, return human readable strings, otherwise try errno name
const char *mp_common_errno_to_str(mp_obj_t errno_val, char *buf, size_t len) {
    if (!mp_obj_is_small_int(errno_val)) {
        return NULL;
    }

    mp_rom_error_text_t desc = NULL;
    switch (MP_OBJ_SMALL_INT_VALUE(errno_val)) {
        case EPERM:
            desc = MP_ERROR_TEXT("Operation not permitted");
            break;
        case ENOENT:
            desc = MP_ERROR_TEXT("No such file/directory");
            break;
        case EIO:
            desc = MP_ERROR_TEXT("Input/output error");
            break;
        case EACCES:
            desc = MP_ERROR_TEXT("Permission denied");
            break;
        case EEXIST:
            desc = MP_ERROR_TEXT("File exists");
            break;
        case ENODEV:
            desc = MP_ERROR_TEXT("No such device");
            break;
        case EINVAL:
            desc = MP_ERROR_TEXT("Invalid argument");
            break;
        case ENOSPC:
            desc = MP_ERROR_TEXT("No space left on device");
            break;
        case EROFS:
            desc = MP_ERROR_TEXT("Read-only filesystem");
            break;
    }
    if (desc != NULL && decompress_length(desc) <= len) {
        decompress(desc, buf);
        return buf;
    }

    const char *msg = "";
    #if MICROPY_PY_ERRNO
    msg = qstr_str(mp_errno_to_str(errno_val));
    #endif
    return msg[0] != '\0' ? msg : NULL;
}

#endif // MICROPY_PY_ERRNO
