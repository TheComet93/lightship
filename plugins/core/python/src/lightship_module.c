#include <Python.h>
#include <structmember.h>

#include "plugin_python/config.h"
#include "plugin_python/context.h"
#include "plugin_python/lightship_module.h"
#include "framework/log.h"

/*
 * This gets set globally when the plugin first starts and is reset to NULL as
 * soon as the plugin finishes its start routine. This is to allow python
 * scripts to register themselves under the correct context.
 *
 * Since plugins work per-context and not globally (i.e. we don't have access
 * to any of the other game instances) I don't see any other way to do this.
 */
struct game_t* g_injected_game = NULL;

/* -------------------------------------------------------------------------
 * lightship exceptions.
 * ------------------------------------------------------------------------- */

static PyObject* lightship_error;

/* ------------------------------------------------------------------------- */
static void
create_lightship_exceptions(PyObject* module)
{
	lightship_error = PyErr_NewException("lightship.LightshipError", NULL, NULL);
	Py_INCREF(lightship_error);
	PyModule_AddObject(module, "LightshipError", lightship_error);
}

/* ------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------
 * Service object.
 * Callable object allowing python scripts to call internal services of
 * lightship.
 * -------------------------------------------------------------------------*/

struct Service
{
	PyObject_HEAD
};

static PyTypeObject ServiceType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"lightship.Service",           /* tp_name */
	sizeof(struct Service),        /* tp_basicsize */
	0,                             /* tp_itemsize */
    0,                             /* tp_dealloc */
	0,                             /* tp_print */
	0,                             /* tp_getattr */
	0,                             /* tp_setattr */
	0,                             /* tp_reserved */
	0,                             /* tp_repr */
	0,                             /* tp_as_number */
	0,                             /* tp_as_sequence */
	0,                             /* tp_as_mapping */
	0,                             /* tp_hash  */
	0,                             /* tp_call */
	0,                             /* tp_str */
	0,                             /* tp_getattro */
	0,                             /* tp_setattro */
	0,                             /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |           /* tp_flags */
	    Py_TPFLAGS_BASETYPE,
	"Service objects",             /* tp_doc */
};

/* -------------------------------------------------------------------------
 * Services object
 * Has static methods for creating and destroying services.
 * Has Service instances as attributes which can be used to call existing
 * services.
 * ------------------------------------------------------------------------- */

struct Services
{
	PyObject_HEAD
	PyObject* create;
	PyObject* destroy;
};

/* ------------------------------------------------------------------------- */
static void
Services_dealloc(struct Services* self)
{
	Py_XDECREF(self->create);
	Py_XDECREF(self->destroy);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

/* ------------------------------------------------------------------------- */
static PyObject*
Services_create(PyObject* self, PyObject* args)
{
	Py_INCREF(Py_None);
	return Py_None;
}

/* ------------------------------------------------------------------------- */
static PyObject*
Services_destroy(PyObject* self, PyObject* args)
{
	Py_INCREF(Py_None);
	return Py_None;
}

/* ------------------------------------------------------------------------- */
static PyMethodDef Services_methods[] = {
	{"create", Services_create, METH_STATIC | METH_VARARGS, "creates and registers a new service."},
	{"destroy", Services_destroy, METH_VARARGS | METH_VARARGS, "destroys and unregisters an existing service."},
	{NULL}
};

static PyTypeObject ServicesType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"lightship.Services",          /* tp_name */
	sizeof(struct Services),       /* tp_basicsize */
	0,                             /* tp_itemsize */
    (destructor)Services_dealloc,  /* tp_dealloc */
	0,                             /* tp_print */
	0,                             /* tp_getattr */
	0,                             /* tp_setattr */
	0,                             /* tp_reserved */
	0,                             /* tp_repr */
	0,                             /* tp_as_number */
	0,                             /* tp_as_sequence */
	0,                             /* tp_as_mapping */
	0,                             /* tp_hash  */
	0,                             /* tp_call */
	0,                             /* tp_str */
	0,                             /* tp_getattro */
	0,                             /* tp_setattro */
	0,                             /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |           /* tp_flags */
	    Py_TPFLAGS_BASETYPE,
	"Services objects",            /* tp_doc */
	0,                             /* tp_traverse */
	0,                             /* tp_clear */
	0,                             /* tp_richcompare */
	0,                             /* tp_weaklistoffset */
	0,                             /* tp_iter */
	0,                             /* tp_iternext */
	Services_methods               /* tp_methods */
};

/* -------------------------------------------------------------------------
 * Game object
 * ------------------------------------------------------------------------- */

struct Game
{
	PyObject_HEAD
	PyObject* service;
	struct game_t* game;
};

/* method forward declarations */
static PyObject*
Game_new(PyTypeObject* type, PyObject* args, PyObject* kwds);

static void
Game_dealloc(struct Game* self);

static PyObject*
Game_get_name(PyObject* self, PyObject* noargs);

static PyObject*
Game_get_network_role(PyObject* self, PyObject* noargs);

/* ------------------------------------------------------------------------- */
static PyMemberDef Game_members[] = {
	{"service", T_OBJECT_EX, offsetof(struct Game, service), 0, "service"},
	{NULL}
};

static PyMethodDef Game_methods[] = {
	{"get_name", Game_get_name, METH_NOARGS, "Gets the globally unique name of the game object"},
	{"get_network_role", Game_get_network_role, METH_NOARGS, "Gets the network role (either 'host' or 'client'"},
	{NULL}
};

PyTypeObject GameType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"lightship.Game",          /* tp_name */
	sizeof(struct Game),       /* tp_basicsize */
	0,                         /* tp_itemsize */
    (destructor)Game_dealloc,  /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |       /* tp_flags */
	    Py_TPFLAGS_BASETYPE,
	"Game objects",            /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	Game_methods,              /* tp_methods */
	Game_members,              /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	0,                         /* tp_init */
	0,                         /* tp_alloc */
	Game_new,                  /* tp_new */
};

/* ------------------------------------------------------------------------- */
/*
 * Creates a new Game object and injects the active context from PLUGIN_START()
 * into it. The Game object is pushed into the context store.
 *
 * The Game object is given a Services object as an attribute.
 */
static PyObject*
Game_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	struct Game* self;

	if(!g_injected_game)
	{
		PyErr_SetString(lightship_error,
						"Can't instantiate game objects after plugin initialisation because they rely on internal global data.");
		return NULL;
	}

	/* allocate game object */
	self = (struct Game*)type->tp_alloc(type, 0);
	if(!self)
		return NULL;

	/* inject game into python Game object so it can be used later */
	((struct Game*)self)->game = g_injected_game;

	/* make sure game object is actually an instance of GameType */
	if(!PyObject_IsInstance((PyObject*)self, (PyObject*)&GameType))
	{
		PyErr_SetString(lightship_error,
						"Object is not a subclass of lightship.Game");
		Py_DECREF(self);
		return NULL;
	}

	/* instantiate service member */
	self->service = PyObject_CallObject((PyObject*)&ServicesType, NULL);
	if(!self->service)
	{
		Py_DECREF(self);
		return NULL;
	}

	return (PyObject*)self;
}

/* ------------------------------------------------------------------------- */
static void
Game_dealloc(struct Game* self)
{
	Py_XDECREF(self->service);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

/* ------------------------------------------------------------------------- */
static PyObject*
Game_get_name(PyObject* self, PyObject* noargs)
{
	struct Game* game = (struct Game*)self;
	return PyUnicode_FromString(game->game->name);
}

/* ------------------------------------------------------------------------- */
static PyObject*
Game_get_network_role(PyObject* self, PyObject* noargs)
{
	struct Game* game = (struct Game*)self;
	PyErr_SetString(lightship_error, "fuck you");
	return NULL;
	if(game->game->network_role == GAME_HOST)
		return PyUnicode_FromString("host");
	else
		return PyUnicode_FromString("client");
}

/* -------------------------------------------------------------------------
 * lightship module
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
static PyObject*
lightship_module_register_game(PyObject* self, PyObject* game)
{
	struct unordered_vector_t* games_vec;

	/* make sure game object is actually an instance of GameType */
	if(!PyObject_IsInstance(game, (PyObject*)&GameType))
	{
		PyErr_SetString(lightship_error,
						"Object is not a subclass of lightship.Game");
		return NULL;
	}

	/* add game object to context store so it can be accessed later */
	games_vec = &(get_context(g_injected_game)->py_objs.games);
	Py_INCREF(game);
	unordered_vector_push(games_vec, &game);

	Py_INCREF(Py_None);
	return Py_None;
}

/* ------------------------------------------------------------------------- */
static PyObject*
lightship_module_unregister_game(PyObject* self, PyObject* game)
{
	struct unordered_vector_t* games_vec;

	/* make sure game object is actually an instance of GameType */
	if(!PyObject_IsInstance(game, (PyObject*)&GameType))
	{
		PyErr_SetString(lightship_error,
						"Object is not a subclass of lightship.Game");
		return NULL;
	}

	/* remove game from context store */
	games_vec = &(get_context(((struct Game*)game)->game)->py_objs.games);
	UNORDERED_VECTOR_FOR_EACH(games_vec, PyObject*, p_game)
		if(*p_game == game)
		{
			unordered_vector_erase_element(games_vec, p_game);
			Py_DECREF(game);
			break;
		}
	UNORDERED_VECTOR_END_EACH

	Py_INCREF(Py_None);
	return Py_None;
}

/* ------------------------------------------------------------------------- */
static PyMethodDef lightship_module_methods[] = {
	{"register_game", lightship_module_register_game, METH_O, "registers a new game object."},
	{"unregister_game", lightship_module_unregister_game, METH_O, "unregisters an existing game object"},
	{NULL}
};

PyModuleDef lightship_module = {
	PyModuleDef_HEAD_INIT,
	"lightship",              /* module name */
	"",                       /* docstring, may be NULL */
	-1,                       /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
	lightship_module_methods, /* module methods */
	NULL,                     /* m_reload */
	NULL,                     /* m_traverse */
	NULL,                     /* m_clear */
	NULL                      /* m_free */
};

static char
init_built_in_types()
{
	if(PyType_Ready(&GameType) < 0)
		return 0;

	ServicesType.tp_new = PyType_GenericNew;
	if(PyType_Ready(&ServicesType) < 0)
		return 0;

	ServiceType.tp_new = PyType_GenericNew;
	if(PyType_Ready(&ServiceType) < 0)
		return 0;

	return 1;
}

static void
add_built_in_types_to_module(PyObject* module)
{
	Py_INCREF(&GameType);
	PyModule_AddObject(module, "Game", (PyObject*)&GameType);

	Py_INCREF(&ServicesType);
	PyModule_AddObject(module, "Services", (PyObject*)&ServicesType);

	Py_INCREF(&ServiceType);
	PyModule_AddObject(module, "Service", (PyObject*)&ServiceType);
}

PyMODINIT_FUNC
PyInit_lightship(void)
{
	PyObject* module;

	if(!init_built_in_types())
		return NULL;

	/* create the module */
	if(!(module = PyModule_Create(&lightship_module)))
		return NULL;

	create_lightship_exceptions(module);
	add_built_in_types_to_module(module);

	return module;
}
