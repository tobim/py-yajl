/*
 * Copyright 2009, R. Tyler Ballance <tyler@monkeypox.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *
 *  3. Neither the name of R. Tyler Ballance nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <Python.h>

#include <string.h>

#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>

#include "py_yajl.h"

int _PlaceObject(_YajlDecoder *self, PyObject *parent, PyObject *child)
{
    if ( (!self) || (!child) || (!parent) )
        return failure;

    if (PyList_Check(parent)) {
        PyList_Append(parent, child);
        // child is now owned by parent!
        if ((child) && (child != Py_None)) {
            Py_XDECREF(child);
        }
        return success;
    } else if (PyDict_Check(parent)) {
        PyObject* key = py_yajl_ps_current(self->keys);
        PyDict_SetItem(parent, key, child);
        py_yajl_ps_pop(self->keys);
        // child is now owned by parent!
        Py_XDECREF(key);
        if ((child) && (child != Py_None)) {
            Py_XDECREF(child);
        }
        return success;
    }
    return failure;
}

int PlaceObject(_YajlDecoder *self, PyObject *object)
{
    unsigned int length = py_yajl_ps_length(self->elements);

    if (length == 0) {
        /*
         * When the length is zero, and we're entering this code path
         * we should only be handling "primitive types" i.e. strings and
         * numbers, not dict/list.
         */
        self->root = object;
        return success;
    }
    return _PlaceObject(self, py_yajl_ps_current(self->elements), object);
}


static int handle_null(void *ctx)
{
    Py_INCREF(Py_None);
    return PlaceObject(ctx, Py_None);
}

static int handle_bool(void *ctx, int value)
{
    return PlaceObject(ctx, PyBool_FromLong((long)(value)));
}

static int handle_number(void *ctx, const char *value, unsigned int length)
{
    //fprintf(stderr, "handle_number: ");
    //fwrite(value, length, 1, stderr);
    //fprintf(stderr, "\n");

    _YajlDecoder *self = (_YajlDecoder *)(ctx);
    PyObject *object;
    PyObject *string;

    int floaty_char;

    // take a moment here to scan the input string to see if there's
    // any chars which suggest this is a floating point number
    for (floaty_char = 0; floaty_char < length; floaty_char++) {
        switch (value[floaty_char]) {
            case '.': case 'e': case 'E': goto floatin;
        }
    }

  floatin:
    string = PyString_FromStringAndSize(value, length);
    if (floaty_char >= length) {
        object = PyInt_FromString(PyString_AS_STRING(string), NULL, 10);
    } else {
        object = PyFloat_FromString(string, NULL);
    }
    Py_XDECREF(string);
    yajl_status status = PlaceObject(self, object);
    //fprintf(stderr, "handle_number status: %d\n", status);
    return status;
}

static int handle_string(void *ctx, const unsigned char *value, unsigned int length)
{
    return PlaceObject(ctx, PyString_FromStringAndSize((char *)value, length));
}

static int handle_start_dict(void *ctx)
{
    PyObject *object = PyDict_New();
    if (!object)
        return failure;

    py_yajl_ps_push(((_YajlDecoder *)(ctx))->elements, object);
    return success;
}

static int handle_dict_key(void *ctx, const unsigned char *value, unsigned int length)
{
    PyObject *object = PyString_FromStringAndSize((const char *) value, length);

    if (object == NULL)
        return failure;

    py_yajl_ps_push(((_YajlDecoder *)(ctx))->keys, object);
    return success;
}

static int handle_end_dict(void *ctx)
{
    _YajlDecoder *self = (_YajlDecoder *)(ctx);
    PyObject *last, *popped;
    unsigned int length;

    length = py_yajl_ps_length(self->elements);
    if (length == 1) {
        /*
         * If this is the last element in the stack
         * then it's "root" and we should finish up
         */
        self->root = py_yajl_ps_current(self->elements);
        py_yajl_ps_pop(self->elements);
        return success;
    } else if (length < 2) {
        return failure;
    }

    /*
     * If not, then we should properly add this dict
     * to it's appropriate parent
     */
    popped = py_yajl_ps_current(self->elements);
    py_yajl_ps_pop(self->elements);
    last = py_yajl_ps_current(self->elements);

    return _PlaceObject(self, last, popped);
}

static int handle_start_list(void *ctx)
{
    PyObject *object = PyList_New(0);

    if (!object)
        return failure;

    py_yajl_ps_push(((_YajlDecoder *)(ctx))->elements, object);
    return success;
}

static int handle_end_list(void *ctx)
{
    _YajlDecoder *self = (_YajlDecoder *)(ctx);
    PyObject *last, *popped;
    unsigned int length;

    length = py_yajl_ps_length(self->elements);
    if (length == 1) {
        self->root = py_yajl_ps_current(self->elements);
        py_yajl_ps_pop(self->elements);
        return success;
    } else if (length < 2) {
        return failure;
    }

    popped = py_yajl_ps_current(self->elements);
    py_yajl_ps_pop(self->elements);
    last = py_yajl_ps_current(self->elements);

    return _PlaceObject(self, last, popped);
}

static yajl_callbacks decode_callbacks = {
    handle_null,
    handle_bool,
    NULL,
    NULL,
    handle_number,
    handle_string,
    handle_start_dict,
    handle_dict_key,
    handle_end_dict,
    handle_start_list,
    handle_end_list
};

PyObject *_internal_decode(_YajlDecoder *self, char *buffer, unsigned int buflen)
{
    if (self->elements.used > 0) {
        py_yajl_ps_free(self->elements);
        py_yajl_ps_init(self->elements);
    }
    if (self->keys.used > 0) {
        py_yajl_ps_free(self->keys);
        py_yajl_ps_init(self->keys);
    }

    /* callbacks, config, allocfuncs */
    yajl_handle parser = yajl_alloc(&decode_callbacks, NULL, (void *)(self));

    yajl_status yrc;
    yrc = yajl_parse(parser, (const unsigned char *)(buffer), buflen);
    if (yrc != yajl_status_ok) {
        goto error;
    }

    yrc = yajl_complete_parse(parser);
    if (yrc != yajl_status_ok) {
        goto error;
    }

    yajl_free(parser);

    assert(self->root != NULL);

    // Callee now owns memory, we'll leave refcnt at one and
    // null out our pointer.
    PyObject *root = self->root;
    self->root = NULL;
    return root;

    unsigned char* str;
error:
    // TODO: It would be nice to make these parse errors more consistent with
    // Oil.  And maybe return them rather than printing on stderr.
    str = yajl_get_error(parser, 1, buffer, buflen);
    fprintf(stderr, "%s", (const char *) str); 
    yajl_free_error(parser, str);  
    yajl_free(parser);

    //fprintf(stderr, "YAJL ERROR %s\n", yajl_status_to_string(yrc));
    PyErr_SetString(PyExc_ValueError, yajl_status_to_string(yrc));
    return NULL;
}
