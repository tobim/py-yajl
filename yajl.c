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

#include "py_yajl.h"

#define PYARGS PyObject *self, PyObject *args, PyObject *kwargs

static void InitDecoder(_YajlDecoder* decoder) {
    py_yajl_ps_init(decoder->elements);
    py_yajl_ps_init(decoder->keys);
    decoder->root = NULL;
}

static void FreeDecoder(_YajlDecoder* decoder) {
    py_yajl_ps_free(decoder->elements);
    py_yajl_ps_init(decoder->elements);
    py_yajl_ps_free(decoder->keys);
    py_yajl_ps_init(decoder->keys);
    if (decoder->root) {
        Py_XDECREF(decoder->root);
    }
}

static PyObject *py_loads(PYARGS)
{
    PyObject *result = NULL;
    PyObject *pybuffer = NULL;
    char *buffer = NULL;
    Py_ssize_t buflen = 0;

    if (!PyArg_ParseTuple(args, "O", &pybuffer))
        return NULL;

    Py_INCREF(pybuffer);

    if (!PyString_Check(pybuffer)) {
        Py_DECREF(pybuffer);
        PyErr_SetString(PyExc_TypeError, "string expected");
        return NULL;
    }

		if (PyString_AsStringAndSize(pybuffer, &buffer, &buflen)) {
				Py_DECREF(pybuffer);
				return NULL;
		}

    _YajlDecoder decoder;
    InitDecoder(&decoder);

    result = _internal_decode(&decoder, buffer, (unsigned int)buflen);

    FreeDecoder(&decoder);
    Py_DECREF(pybuffer);
    return result;
}

const char* IndentString(int n) {
    char* spaces = (char *)(malloc(n + 1));
    memset(spaces, ' ', n);
    spaces[n] = '\0';
    return spaces;
}

static PyObject *py_dumps(PYARGS)
{
    PyObject *obj = NULL;
    PyObject *result = NULL;
    static char *kwlist[] = {"object", "indent", NULL};

    int indent = -1;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i", kwlist, &obj, &indent)) {
        return NULL;
    }

    char* spaces = NULL;
    if (indent >= 0) {
        spaces = IndentString(indent);
    }

    _YajlEncoder encoder;
    result = _internal_encode(&encoder, obj, spaces);

    if (spaces) {
        free(spaces);
    }

    return result;
}

static PyObject *__read = NULL;
static PyObject *_internal_stream_load(PyObject *args, unsigned int blocking)
{
    PyObject *stream = NULL;
    PyObject *buffer = NULL;
    PyObject *result = NULL;

    if (!PyArg_ParseTuple(args, "O", &stream)) {
        goto bad_type;
    }

    if (__read == NULL) {
        __read = PyString_FromString("read");
    }

    if (!PyObject_HasAttr(stream, __read)) {
        goto bad_type;
    }

    buffer = PyObject_CallMethodObjArgs(stream, __read, NULL);

    if (!buffer)
        return NULL;

    _YajlDecoder decoder;
    InitDecoder(&decoder);

    result = _internal_decode(&decoder, PyString_AsString(buffer),
                              PyString_Size(buffer));
    FreeDecoder(&decoder);
    Py_XDECREF(buffer);
    return result;

bad_type:
    PyErr_SetString(PyExc_TypeError, "Must pass a single stream object");
    return NULL;
}

static PyObject *py_load(PYARGS)
{
    return _internal_stream_load(args, 1);
}

static struct PyMethodDef yajl_methods[] = {
    {"dumps", (PyCFunctionWithKeywords)(py_dumps), METH_VARARGS | METH_KEYWORDS,
"yajl.dumps(obj [, indent=None])\n\n\
Returns an encoded JSON string of the specified `obj`\n\
\n\
If `indent` is a non-negative integer, then JSON array elements \n\
and object members will be pretty-printed with that indent level. \n\
An indent level of 0 will only insert newlines. None (the default) \n\
selects the most compact representation.\n\
"},
    {"loads", (PyCFunction)(py_loads), METH_VARARGS,
"yajl.loads(string)\n\n\
Returns a decoded object based on the given JSON `string`"},
    {"load", (PyCFunction)(py_load), METH_VARARGS,
"yajl.load(fp)\n\n\
Returns a decoded object based on the JSON read from the `fp` stream-like\n\
object; *Note:* It is expected that `fp` supports the `read()` method"},
    {NULL}
};


PyMODINIT_FUNC inityajl(void)
{
    PyObject *module = Py_InitModule3("yajl", yajl_methods,
"Providing a pythonic interface to the yajl (Yet Another JSON Library) parser\n\n\
The interface is similar to that of simplejson or jsonlib providing a consistent syntax for JSON\n\
encoding and decoding. Unlike simplejson or jsonlib, yajl is **fast** :)\n\n\
The following benchmark was done on a dual core MacBook Pro with a fairly large (100K) JSON document:\n\
json.loads():\t\t21351.313ms\n\
simplejson.loads():\t1378.6492ms\n\
yajl.loads():\t\t502.4572ms\n\
\n\
json.dumps():\t\t7760.6348ms\n\
simplejson.dumps():\t930.9748ms\n\
yajl.dumps():\t\t681.0221ms"
);
}

