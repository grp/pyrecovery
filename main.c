#include <Python.h>
#include <structmember.h>
#include <libusb-1.0/libusb.h>

#include "libirecovery.h"

void error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

typedef struct {
	PyObject_HEAD
	
	irecv_client_t client;
	char connected;
} DeviceObject;



static PyObject* Device_init(DeviceObject *self, PyObject *args);
static void Device_dealloc(DeviceObject *self);
static PyObject* Device_connect(DeviceObject *self, PyObject *args, PyObject *kwargs);
static PyObject* Device_disconnect(DeviceObject *self, PyObject *args);
static PyObject* Device_send_command(DeviceObject *self, PyObject *args);
static PyObject* Device_receive(DeviceObject *self, PyObject *args);
static PyObject* Device_reset(DeviceObject *self, PyObject *args);
static PyObject* Device_setenv(DeviceObject *self, PyObject *args);
static PyObject* Device_getenv(DeviceObject *self, PyObject *args);
static PyObject* Device_saveenv(DeviceObject *self, PyObject *args);
static PyObject* Device_getinfo(DeviceObject *self, PyObject *args);
static PyObject* Device_control_transfer(DeviceObject *self, PyObject *args, PyObject *kwargs);
static PyObject* Device_send_file(DeviceObject *self, PyObject *args);

static PyMethodDef Device_methods[] = {
    {"__init__", (PyCFunction) Device_init, METH_NOARGS, "Initialize a device."},
    {"connect", (PyCFunction) Device_connect, METH_KEYWORDS, "Connect the device."},
	{"disconnect", (PyCFunction) Device_disconnect, METH_NOARGS, "Disconnect the device."},
	{"send_command", (PyCFunction) Device_send_command, METH_VARARGS, "Send an iBoot command."},
	{"receive", (PyCFunction) Device_receive, METH_NOARGS, "Get iBoot shell response."},
	{"reset", (PyCFunction) Device_reset, METH_NOARGS, "Reset device."},
	{"setenv", (PyCFunction) Device_setenv, METH_VARARGS, "Get iBoot environment variable."},
	{"getenv", (PyCFunction) Device_getenv, METH_VARARGS, "Set iBoot environment variable."},
	{"saveenv", (PyCFunction) Device_saveenv, METH_NOARGS, "Save iBoot environment variables."},
	{"getinfo", (PyCFunction) Device_getinfo, METH_VARARGS, "Get device information for a specified key."},
	{"control_transfer", (PyCFunction) Device_control_transfer, METH_KEYWORDS, "Perform the libusb_control_transfer() function with specified arguments."},
	{"send_file", (PyCFunction) Device_send_file, METH_VARARGS, "Send a file."},
    {NULL}
};

static struct PyMemberDef Device_members[] = {
    {"connected", T_BOOL, offsetof(DeviceObject, connected), 0, "Bool: device connected?"},
    {NULL}
};

static PyTypeObject DeviceType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "iphone.Device",           /*tp_name*/
    sizeof(DeviceObject),      /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Device_dealloc,/*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
	0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Noddy objects",           /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    Device_methods,            /* tp_methods */
    Device_members,            /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Device_init,     /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

// def __init__(self)
static PyObject *Device_init(DeviceObject *self, PyObject *args) {
	self->connected = 0;
	
    Py_INCREF(Py_None);
    return Py_None;
}

// def __del__(self)
static void Device_dealloc(DeviceObject *self) {
	if (self->connected)
		Device_disconnect(self, NULL);

    self->ob_type->tp_free((PyObject *) self);
}

// def connect(self, retries=5)
static PyObject *Device_connect(DeviceObject *self, PyObject *args, PyObject *kwargs) {
	if (self->connected)
		goto error;
		
	int retries = 5;
	
	static char *kwlist[] = {"retries", NULL};
	if(!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &retries))
		return NULL; 
	
	// Lame haxx to avoid an i variable.
	for (; retries > 0 && !self->connected; retries--) {
		if (irecv_open(&(self->client)) != IRECV_E_SUCCESS)
			sleep(1);
		else
			self->connected = 1;
	}
	
	if (!self->connected && retries == 0)
		PyErr_Format(PyExc_RuntimeError, "Internal error: unable to connect.");
	
error:
    Py_INCREF(Py_None);
    return Py_None;
}

// def disconnect(self)
static PyObject *Device_disconnect(DeviceObject *self, PyObject *args) {
	if (!self->connected)
		goto error;
	
	irecv_close(self->client);
	self->connected = 0;
	
error:
    Py_INCREF(Py_None);
    return Py_None;
}

// def send_command(self, command)
static PyObject *Device_send_command(DeviceObject *self, PyObject *args) {
	if (!self->connected)
		goto error;

	unsigned char *command;
	if (!PyArg_ParseTuple(args, "s", &command))
		return NULL;

	if (irecv_send_command(self->client, command) != IRECV_E_SUCCESS)
		return PyErr_Format(PyExc_RuntimeError, "Internal error sending command.");
	
error:
	Py_INCREF(Py_None);
	return Py_None;
}

// def receive(self)
static PyObject *Device_receive(DeviceObject *self, PyObject *args) {
	unsigned char buffer[1024];
	memset(buffer, '\0', 1024);
	int bytes = 0;
	
	PyObject *string = PyString_FromString("");
	
	while (libusb_bulk_transfer(self->client->handle, 0x81, buffer, 1024, &bytes, 100) == 0) {
		if (bytes > 0) {
			int length = PyString_Size(string);
			_PyString_Resize(&string, length + bytes);
			memcpy((unsigned char *) PyString_AsString(string) + length, buffer, bytes);
		} else break;
	}
	
	return string;
	
error:
	Py_INCREF(Py_None);
	return Py_None;
}

// def reset(self)
static PyObject *Device_reset(DeviceObject *self, PyObject *args) {
	if (!self->connected)
		goto error;

	irecv_reset(self->client);

error:
	Py_INCREF(Py_None);
	return Py_None;
}

// def setenv(self, variable, value)
static PyObject *Device_setenv(DeviceObject *self, PyObject *args) {
	if (!self->connected)
		goto error;
	
	char *variable, *value;
	if (!PyArg_ParseTuple(args, "ss", &variable, &value))
		return NULL;
		
	if (irecv_setenv(self->client, variable, value) != IRECV_E_SUCCESS)
		return PyErr_Format(PyExc_RuntimeError, "Internal error setting environment variable.");
		
error:
	Py_INCREF(Py_None);
	return Py_None;
}

// def getenv(self, variable)
static PyObject *Device_getenv(DeviceObject *self, PyObject *args) {
	if (!self->connected)
		goto error;
		
	char *variable, *value;
	if (!PyArg_ParseTuple(args, "s", &variable))
		return NULL;

	if (irecv_getenv(self->client, variable, &value) != IRECV_E_SUCCESS)
		return PyErr_Format(PyExc_RuntimeError, "Internal error getting environment variable.");
	
	PyObject *string = PyString_FromString(value);
	free(value);
	return string;

error:
	Py_INCREF(Py_None);
	return Py_None;
}

// def saveenv(self)
static PyObject *Device_saveenv(DeviceObject *self, PyObject *args) {
	if (!self->connected)
		goto error;
		
	irecv_send_command(self->client, (unsigned char *) "saveenv");

error:
	Py_INCREF(Py_None);
	return Py_None;
}

// def getinfo(self, key)
static PyObject *Device_getinfo(DeviceObject *self, PyObject *args) {
	if (!self->connected)
		goto error;
		
	char *key, *result;
	if (!PyArg_ParseTuple(args, "s", &key))
		return NULL;
		
	/*if (strcmp(key, "ECID") == 0)
		return*/
		
	result = "UNIMPLEMENTED";
	
	return PyString_FromString(result);

error:
	Py_INCREF(Py_None);
	return Py_None;
}

// def send_file(self, buffer)
static PyObject *Device_send_file(DeviceObject *self, PyObject *args) {
	if (!self->connected)
		goto error;
		
	unsigned char *buffer;
	int length;
	if (!PyArg_ParseTuple(args, "s#", &buffer, &length))
		return NULL;
		
	if (irecv_send_buffer(self->client, buffer, length) != IRECV_E_SUCCESS)
		return PyErr_Format(PyExc_RuntimeError, "Internal error sending buffer.");

error:
	Py_INCREF(Py_None);
	return Py_None;
}

// def control_transfer(self, type, request, value=0, index=0, data='', timeout=0)
static PyObject *Device_control_transfer(DeviceObject *self, PyObject *args, PyObject *kwargs) {
	if (!self->connected)
		goto error;
	
	int type, request, value = 0, index = 0, timeout = 0, data_len = 0;
	unsigned char *data = NULL;
		
	static char *kwlist[] = {"value", "index", "data", "timeout", NULL};
	if(!PyArg_ParseTupleAndKeywords(args, kwargs, "ii|iis#i", kwlist, &type, &request, &value, &index, &data, &data_len, &timeout))
		return NULL;
	
	if (libusb_control_transfer(self->client->handle, type, request, value, index, data, data_len, timeout) != data_len)
		return PyErr_Format(PyExc_RuntimeError, "Internal error, could not send control_transfer.");

error:
	Py_INCREF(Py_None);
	return Py_None;
}



PyMethodDef ModuleMethods[] = {
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initiphone() { 
	/* Init type */
	DeviceType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&DeviceType) < 0)
        return;
	
	/* Init module */
    PyObject *module = Py_InitModule("iphone", ModuleMethods);  

	Py_INCREF(&DeviceType);
	PyModule_AddObject(module, "Device", (PyObject *)&DeviceType);
}
