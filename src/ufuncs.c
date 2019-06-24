#define PY_SSIZE_T_CLEAN
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <Python.h>
#include <math.h>
#include <geos_c.h>
#include <structmember.h>
#include "numpy/ndarraytypes.h"
#include "numpy/ufuncobject.h"
#include "numpy/npy_3kcompat.h"

#include "fast_loop_macros.h"

/* This tells Python what methods this module has. */
static PyMethodDef GeosModule[] = {
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "ufuncs",
    NULL,
    -1,
    GeosModule,
    NULL,
    NULL,
    NULL,
    NULL
};

typedef struct {
    PyObject_HEAD;
    void *ptr;
    char geom_type_id;
    char has_z;
} GeometryObject;


static PyObject *GeometryObject_new_from_ptr(
    PyTypeObject *type, void *context_handle, GEOSGeometry *ptr)
{
    GeometryObject *self;
    int geos_result;
    self = (GeometryObject *) type->tp_alloc(type, 0);

    if (self != NULL) {
        self->ptr = ptr;
        geos_result = GEOSGeomTypeId_r(context_handle, ptr);
        if ((geos_result < 0) | (geos_result > 255)) {
            goto fail;
        }
        self->geom_type_id = geos_result;
        geos_result = GEOSHasZ_r(context_handle, ptr);
        if ((geos_result < 0) | (geos_result > 1)) {
            goto fail;
        }
        self->has_z = geos_result;
    }
    return (PyObject *) self;
    fail:
        PyErr_Format(PyExc_RuntimeError, "Geometry initialization failed");
        Py_DECREF(self);
        return NULL;
}


static PyObject *GeometryObject_new(PyTypeObject *type, PyObject *args,
                                    PyObject *kwds)
{
    void *context_handle;
    GEOSGeometry *ptr;
    PyObject *self;
    long arg;

    if (!PyArg_ParseTuple(args, "l", &arg)) {
        goto fail;
    }
    context_handle = GEOS_init_r();
    ptr = GEOSGeom_clone_r(context_handle, (GEOSGeometry *) arg);
    if (ptr == NULL) {
        GEOS_finish_r(context_handle);
        goto fail;
    }
    self = GeometryObject_new_from_ptr(type, context_handle, ptr);
    GEOS_finish_r(context_handle);
    return (PyObject *) self;

    fail:
        PyErr_Format(PyExc_ValueError, "Please provide a C pointer to a GEOSGeometry");
        return NULL;
}

static void GeometryObject_dealloc(GeometryObject *self)
{
    void *context_handle;
    if (self->ptr != NULL) {
        context_handle = GEOS_init_r();
        GEOSGeom_destroy_r(context_handle, self->ptr);
        GEOS_finish_r(context_handle);
    }
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyMemberDef GeometryObject_members[] = {
    {"ptr", T_INT, offsetof(GeometryObject, ptr), 0, "pointer to GEOSGeometry"},
    {"geom_type_id", T_INT, offsetof(GeometryObject, geom_type_id), 0, "geometry type ID"},
    {"has_z", T_INT, offsetof(GeometryObject, has_z), 0, "has Z"},
    {NULL}  /* Sentinel */
};

static PyTypeObject GeometryType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pygeos.ufuncs.Geometry",
    .tp_doc = "Geometry type",
    .tp_basicsize = sizeof(GeometryObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = GeometryObject_new,
    .tp_dealloc = (destructor) GeometryObject_dealloc,
    .tp_members = GeometryObject_members,
};

typedef char FuncGEOS_YY_b(void *context, void *a, void *b);
static char YY_b_dtypes[3] = {NPY_OBJECT, NPY_OBJECT, NPY_BOOL};
static void YY_b_func(char **args, npy_intp *dimensions,
                      npy_intp* steps, void* data)
{
    FuncGEOS_YY_b *func = (FuncGEOS_YY_b *)data;
    void *context_handle = GEOS_init_r();

    BINARY_LOOP {
        GeometryObject *in1 = *(GeometryObject **)ip1;
        GeometryObject *in2 = *(GeometryObject **)ip2;
        if ((in1->ptr == NULL) || (in2->ptr == NULL)) {  goto finish;  }
        npy_bool ret = func(context_handle, in1->ptr, in2->ptr);
        if (ret == 2) {  goto finish;  }
        *(npy_bool *)op1 = ret;
    }

    finish:
        GEOS_finish_r(context_handle);
        return;
}
static PyUFuncGenericFunction YY_b_funcs[1] = {&YY_b_func};

typedef void *FuncGEOS_YY_Y(void *context, void *a, void *b);
static char YY_Y_dtypes[3] = {NPY_OBJECT, NPY_OBJECT, NPY_OBJECT};
static void YY_Y_func(char **args, npy_intp *dimensions,
                      npy_intp* steps, void* data)
{
    FuncGEOS_YY_Y *func = (FuncGEOS_YY_Y *)data;
    void *context_handle = GEOS_init_r();

    BINARY_LOOP {
        GeometryObject *in1 = *(GeometryObject **)ip1;
        GeometryObject *in2 = *(GeometryObject **)ip2;
        PyObject **out = (PyObject **)op1;
        if ((in1->ptr == NULL) || (in2->ptr == NULL)) { goto finish; }
        GEOSGeometry *ret_ptr = func(context_handle, in1->ptr, in2->ptr);
        if (ret_ptr == NULL) {  goto finish;  }
        PyObject *ret = GeometryObject_new_from_ptr(&GeometryType, context_handle, ret_ptr);
        if (ret == NULL) { goto finish;  }
        Py_XDECREF(*out);
        *out = ret;
    }

    finish:
        GEOS_finish_r(context_handle);
        return;
}
static PyUFuncGenericFunction YY_Y_funcs[1] = {&YY_Y_func};


/* TODO GG -> d function GEOSProject_r
RegisterPyUFuncGEOS_Yd_Y("interpolate", GEOSInterpolate_r, dt, d);
TODO GG -> d function GEOSProjectNormalized_r
RegisterPyUFuncGEOS_Yd_Y("interpolate_normalized", GEOSInterpolateNormalized_r, dt, d);
RegisterPyUFuncGEOS_Ydl_Y("buffer", GEOSBuffer_r, dt, d);
TODO custom buffer functions
TODO possibly implement some creation functions
RegisterPyUFuncGEOS_Y_Y("clone", GEOSGeom_clone_r, dt, d);
TODO G -> void function GEOSGeom_destroy_r
RegisterPyUFuncGEOS_Y_Y("envelope", GEOSEnvelope_r, dt, d); */
static void *intersection_data[1] = {GEOSIntersection_r};
/* RegisterPyUFuncGEOS_Y_Y("convex_hull", GEOSConvexHull_r, dt, d); */
static void *difference_data[1] = {GEOSDifference_r};
static void *symmetric_difference_data[1] = {GEOSSymDifference_r};
/* RegisterPyUFuncGEOS_Y_Y("boundary", GEOSBoundary_r, dt, d); */
static void *union_data[1] = {GEOSUnion_r};
/* RegisterPyUFuncGEOS_Y_Y("unary_union", GEOSUnaryUnion_r, dt, d);
RegisterPyUFuncGEOS_Y_Y("point_on_surface", GEOSPointOnSurface_r, dt, d);
RegisterPyUFuncGEOS_Y_Y("get_centroid", GEOSGetCentroid_r, dt, d);
TODO polygonizer functions
RegisterPyUFuncGEOS_Y_Y("line_merge", GEOSLineMerge_r, dt, d);
RegisterPyUFuncGEOS_Yd_Y("simplify", GEOSSimplify_r, dt, d);
RegisterPyUFuncGEOS_Yd_Y("topology_preserve_simplify", GEOSTopologyPreserveSimplify_r, dt, d);
RegisterPyUFuncGEOS_Y_Y("extract_unique_points", GEOSGeom_extractUniquePoints_r, dt, d); */
static void *shared_paths_data[1] = {GEOSSharedPaths_r};
/* TODO GGd -> G function GEOSSnap_r */
static void *disjoint_data[1] = {GEOSDisjoint_r};
static void *touches_data[1] = {GEOSTouches_r};
static void *intersects_data[1] = {GEOSIntersects_r};
static void *crosses_data[1] = {GEOSCrosses_r};
static void *within_data[1] = {GEOSWithin_r};
static void *contains_data[1] = {GEOSContains_r};
static void *overlaps_data[1] = {GEOSOverlaps_r};
static void *equals_data[1] = {GEOSEquals_r};
/* TODO GGd -> b function GEOSEqualsExact_r */
static void *covers_data[1] = {GEOSCovers_r};
static void *covered_by_data[1] = {GEOSCoveredBy_r};
/* TODO prepared geometry predicate functions */
/* RegisterPyUFuncGEOS_Y_b("is_empty", GEOSisEmpty_r, dt, d);
RegisterPyUFuncGEOS_Y_b("is_simple", GEOSisSimple_r, dt, d);
RegisterPyUFuncGEOS_Y_b("is_ring", GEOSisRing_r, dt, d);
RegisterPyUFuncGEOS_Y_b("has_z", GEOSHasZ_r, dt, d);
RegisterPyUFuncGEOS_Y_b("is_closed", GEOSisClosed_r, dt, d);
TODO relate functions
RegisterPyUFuncGEOS_Y_b("is_valid", GEOSisValid_r, dt, d);
TODO G -> char function GEOSisValidReason_r
RegisterPyUFuncGEOS_Y_B("geom_type_id", GEOSGeomTypeId_r, dt, d);
RegisterPyUFuncGEOS_Y_l("get_srid", GEOSGetSRID_r, dt, d);
TODO Gi -> void function GEOSSetSRID_r
RegisterPyUFuncGEOS_Y_l("get_num_geometries", GEOSGetNumGeometries_r, dt, d);
TODO G -> void function GEOSNormalize_r
RegisterPyUFuncGEOS_Y_l("get_num_interior_rings", GEOSGetNumInteriorRings_r, dt, d);
RegisterPyUFuncGEOS_Y_l("get_num_points", GEOSGeomGetNumPoints_r, dt, d);
RegisterPyUFuncGEOS_Y_d("get_x", GEOSGeomGetX_r, dt, d);
RegisterPyUFuncGEOS_Y_d("get_y", GEOSGeomGetY_r, dt, d);
RegisterPyUFuncGEOS_Yl_Y("get_interior_ring_n", GEOSGetInteriorRingN_r, dt, d);
RegisterPyUFuncGEOS_Y_Y("get_exterior_ring", GEOSGetExteriorRing_r, dt, d);
RegisterPyUFuncGEOS_Y_l("get_num_coordinates", GEOSGetNumCoordinates_r, dt, d);
RegisterPyUFuncGEOS_Y_B("get_dimensions", GEOSGeom_getDimensions_r, dt, d);
RegisterPyUFuncGEOS_Y_B("get_coordinate_dimensions", GEOSGeom_getCoordinateDimension_r, dt, d);
RegisterPyUFuncGEOS_Yl_Y("get_point_n", GEOSGeomGetPointN_r, dt, d);
RegisterPyUFuncGEOS_Y_Y("get_start_point", GEOSGeomGetStartPoint_r, dt, d);
RegisterPyUFuncGEOS_Y_Y("get_end_point", GEOSGeomGetEndPoint_r, dt, d);
RegisterPyUFuncGEOS_Y_d("area", GEOSArea_r, dt, d);
RegisterPyUFuncGEOS_Y_d("length", GEOSLength_r, dt, d);
RegisterPyUFuncGEOS_YY_d("distance", GEOSDistance_r, dt, d);
RegisterPyUFuncGEOS_YY_d("hausdorff_distance", GEOSHausdorffDistance_r, dt, d);
TODO GGd -> d function GEOSHausdorffDistanceDensify_r
RegisterPyUFuncGEOS_Y_d("get_length", GEOSGeomGetLength_r, dt, d); */

#define DEFINE_YY_b(NAME)\
    ufunc = PyUFunc_FromFuncAndData(YY_b_funcs, NAME ##_data, YY_b_dtypes, 1, 2, 1, PyUFunc_None, # NAME, "", 0);\
    PyDict_SetItemString(d, # NAME, ufunc)

#define DEFINE_YY_Y(NAME)\
    ufunc = PyUFunc_FromFuncAndData(YY_Y_funcs, NAME ##_data, YY_Y_dtypes, 1, 2, 1, PyUFunc_None, # NAME, "", 0);\
    PyDict_SetItemString(d, # NAME, ufunc)


PyMODINIT_FUNC PyInit_ufuncs(void)
{
    PyObject *m, *d, *ufunc;
    m = PyModule_Create(&moduledef);
    if (!m) {
        return NULL;
    }
    if (PyType_Ready(&GeometryType) < 0)
        return NULL;

    Py_INCREF(&GeometryType);
    PyModule_AddObject(m, "Geometry", (PyObject *) &GeometryType);

    d = PyModule_GetDict(m);

    import_array();
    import_umath();

    /* Define the geometry structured dtype (see pygeos.GEOM_DTYPE) */

    DEFINE_YY_Y (intersection);
    DEFINE_YY_Y (difference);
    DEFINE_YY_Y (symmetric_difference);
    DEFINE_YY_Y (union);
    DEFINE_YY_Y (shared_paths);
    DEFINE_YY_b (disjoint);
    DEFINE_YY_b (touches);
    DEFINE_YY_b (intersects);
    DEFINE_YY_b (crosses);
    DEFINE_YY_b (within);
    DEFINE_YY_b (contains);
    DEFINE_YY_b (overlaps);
    DEFINE_YY_b (equals);
    DEFINE_YY_b (covers);
    DEFINE_YY_b (covered_by);



    Py_DECREF(ufunc);
    return m;
}
