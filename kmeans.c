#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define EPSILON 0.001

typedef struct Cluster {
    int cluster_size;
    int cluster_index;
    double* prev_centroids;
    double* curr_centroids;
    double* sum;
} Cluster;



/*** Function Declaration ***/
static PyObject* c_kmeans(PyObject *self, PyObject *args);
double** init_points (int num_points, int num_coordinates);
int python_list_of_lists_to_2D_array (PyObject *python_list_of_lists, double **target_array);
Cluster* make_cluster (const double* point, const int num_coordinates, const int index);
Cluster** python_init_k_clusters (int k);
void add_point_to_cluster (Cluster* cluster, const double* point, int num_coordinates);
void find_minimum_distance (Cluster** clusters, double* point, int k, int num_coordinates);
double update_cluster_centroids_and_sum (Cluster* cluster, int num_coordinates);
int update_all_clusters (Cluster** clusters, int k, int num_coordinates);
void free_clusters_memory (Cluster** clusters, int k);
void free_points_memory (double** points, int num_points);
double get_distance (Cluster* cluster, const double* point, int num_coordinates);

/*******************************/
/*** Main Function ***/
/*******************************/

static PyObject* c_kmeans(PyObject *self, PyObject *args) {
    /*Define variables to receive from user*/
    int k;
    int max_iter;
    int num_points;
    int num_coordinates;
    PyObject *data_points;
    PyObject *initial_centroids;
    double **points;
    double **initial_points_for_centroids;
    double *point;
    Cluster **clusters;
    int did_update;
    int i;
    int j;
    PyObject *centroids_output_list;
    Py_ssize_t output_list_len;
    PyObject *single_centroid_list;
    Py_ssize_t output_num_coordinates;
    PyObject *output_coordinate_item;


    /* Parse arguments from Python */
    if((!PyArg_ParseTuple(args, "OOiiii", &data_points, &initial_centroids, &k, &max_iter, &num_points, &num_coordinates))) {
        return NULL; /*In the CPython API, Null is never a valid value for a PyObject* - so it signals an error*/
    }


    /*Verify that data_points & initial_indexes are python lists*/
    if (!PyList_Check(data_points) || !PyList_Check(initial_centroids)) {
        return NULL;
    }
    
    /*Load points from PyObject into C format of 2D-array points*/
    points = init_points(num_points, num_coordinates);

    if (python_list_of_lists_to_2D_array(data_points, points) != 0) {
        return NULL;
    }

    /*Load initial centroids from PyObject into C format of 2D-array -> into clusters  */
    clusters = python_init_k_clusters(k);
    initial_points_for_centroids = init_points(k, num_coordinates);

    if (python_list_of_lists_to_2D_array(initial_centroids, initial_points_for_centroids) != 0) {
        return NULL;
    }

    for (i=0; i<k; i++) {
        clusters[i] = make_cluster(initial_points_for_centroids[i], num_coordinates, i);
    }

    free_points_memory(initial_points_for_centroids, k); /*we don't need this 2D array anymore since initial centroids are now stored in clusters*/

    /***Execute k-means algorithm***/
    did_update = 0;

    for (i=0; i<max_iter; i++) {
        for (j=0; j<num_points; j++) {
            point = points[j];
            find_minimum_distance(clusters, point, k, num_coordinates);
        }
        did_update = update_all_clusters(clusters, k, num_coordinates);
        if (did_update == 1) { /*No update occured - we can break*/
            break;
        }
    }

    free_points_memory(points, num_points);

    /***Convert results to Python Object***/
    output_list_len = k;
    output_num_coordinates = num_coordinates;
    centroids_output_list = PyList_New(output_list_len); /*Create final centroids list*/
    if (centroids_output_list == NULL) {
        return NULL;
    }
    for (i=0; i<k; i++) {
        single_centroid_list = PyList_New(output_num_coordinates); /*Create single centroid list*/
        if (single_centroid_list == NULL) {
            return NULL;
        }
        for (j=0; j<num_coordinates; j++) {
            output_coordinate_item = PyFloat_FromDouble(clusters[i]->curr_centroids[j]);
            if (output_coordinate_item == NULL) {
                return NULL;
            }
            PyList_SET_ITEM(single_centroid_list, j, output_coordinate_item); /*user macro version of PyList_setItem() since there's no previous content*/
        }
        PyList_SET_ITEM(centroids_output_list, i, single_centroid_list);
    }

    free_clusters_memory(clusters, k);

    return centroids_output_list;
}



/*******************************/
/*** Function Implementation ***/
/*******************************/

double** init_points (int num_points, int num_coordinates) {
     /*
    Recieves number of points and number of coordinates,
    Returns new 2D array of points with sufficient memory to store all points.
    */
    double** points;
    int i;

    /*allocate memory for 2D array of points*/
    points = (double**)malloc(sizeof(double*) * num_points);
    assert(points != NULL);
    for (i=0; i< num_points; i++) {
        points[i] = (double*)malloc(sizeof(double) * num_coordinates);
        assert(points[i] != NULL);
    }

    return points;
}

int python_list_of_lists_to_2D_array (PyObject *python_list_of_lists, double **target_array) {
    Py_ssize_t list_size;
    Py_ssize_t entry_size;
    PyObject *point_item;
    PyObject *coordinate_item;
    int i;
    int j;

    list_size = PyList_Size(python_list_of_lists); /*equivilant to len(_list) in Python*/
    for (i=0; i<list_size; i++) { /*iterate over points*/
        point_item = PyList_GetItem(python_list_of_lists, i);
        if (!PyList_Check(point_item)) {
            return 1;
        }
        entry_size = PyList_Size(point_item);
        for (j=0; j<entry_size; j++) { /*iterate over coordinates of single point*/
            coordinate_item = PyList_GetItem(point_item, j);
            if (!PyFloat_Check(coordinate_item)) {
                return 1;
            }
            target_array[i][j] = PyFloat_AsDouble(coordinate_item);
        }
    }
    return 0;
}

Cluster* make_cluster (const double* point, const int num_coordinates,const int index) {
    /*
    Recieves a point, number of coordinates and a specific cluster index,
    Returns new Cluster with sufficient memory space to store both the current centroid and the previous centroid, holding the current centroid which is the input point.
    */
    int i;
    Cluster* cluster;

    /*allocate memory*/
    cluster = malloc(sizeof(Cluster));
    assert (cluster != NULL);
    /*initialize parameters*/
    cluster->cluster_size = 0;
    cluster->cluster_index = index;
    /*initialize centroids*/
    cluster->curr_centroids = (double*)malloc(sizeof(double) * num_coordinates);
    assert (cluster->curr_centroids != NULL);
    cluster->prev_centroids = (double*)malloc(sizeof(double) * num_coordinates);
    assert (cluster->prev_centroids != NULL);
    cluster->sum = (double*)malloc(sizeof(double) * num_coordinates);
    assert (cluster->sum != NULL);

    for (i=0; i<num_coordinates; i++) {
        cluster->curr_centroids[i] = point[i];
        cluster->sum [i] = 0;
    }

    return cluster;
}

Cluster** python_init_k_clusters (int k) {
    /*
    Recieves pointer to 2D array of points, k, number of coordinates and 2D array of initial indexes,
    Returns new 2D array of Clusters with sufficient memory, initialized with the first k points.
    */
    Cluster **clusters;

    clusters = malloc(sizeof(Cluster*) * k);
    assert (clusters != NULL);

    return clusters;
}

void add_point_to_cluster (Cluster* cluster,const double* point, int num_coordinates) {
    /*
    Recieves pointer to a Cluster, a point and number of coordinates,
    Adds the given point to the given cluster.
    */
    int i;

    cluster->cluster_size += 1;
    for (i=0; i<num_coordinates; i++) {
        cluster->sum[i] += point[i];
    }
}

void find_minimum_distance (Cluster** clusters, double* point, int k, int num_coordinates) {
    /*
    Recieves 2D Clsuter array, pointer to a point, k and number of coordinates,
    Checks which centroid is closest to the given point,
    Adds point to closest cluster.
    */
    int i;
    int index_min;
    double distance_min;
    double current_distance;

    /*initialize distance_min to largest double value ~ INF*/
    distance_min = 1.7976931348623155e+308;
    index_min = -1;

    for (i=0; i<k; i++) {
        current_distance = get_distance(clusters[i], point, num_coordinates);
        if (current_distance < distance_min) {
            distance_min = current_distance;
            index_min = clusters[i]->cluster_index;
        }
    }

    add_point_to_cluster(clusters[index_min], point, num_coordinates);
}


double update_cluster_centroids_and_sum (Cluster* cluster, int num_coordinates) {
    /*
    Recieves pointer to a changed cluster and number of coordinates,
    Updates cluster accordingly,
    Returns the distance between previous centroid and current centroid.
    */
    int i;
    double distance;

    for (i=0; i<num_coordinates; i++) {
        cluster->prev_centroids[i] = cluster->curr_centroids[i];
        cluster->curr_centroids[i] = cluster->sum[i] / cluster->cluster_size;
        cluster->sum[i] = 0;
    }

    cluster->cluster_size = 0;
    distance = get_distance(cluster, cluster->prev_centroids, num_coordinates);
    return distance;
}

int update_all_clusters (Cluster** clusters, int k, int num_coordinates) {
    /*
    Recieves 2D array of clusters, k and number of coordinates,
    Upadates all clusters using update_cluster_centroids_and_sum while counting total change in distance,
    Returns 1 if no update was made, 0 otherwise.
    */
    int i;
    double total_update;

    total_update = 0.0;

    for (i=0; i<k; i++) {
        total_update += update_cluster_centroids_and_sum (clusters[i], num_coordinates);
    }

    if (total_update < EPSILON) {
        /*no update was made*/
        return 1;
    }
    return 0;
}

void free_clusters_memory (Cluster** clusters, int k) {
    /*
    Recieves 2D array of clusters and k,
    Frees up memory used by all clusters.
    */
    int i;
    Cluster* curr_cluster;

    for (i=0; i<k; i++) {
        curr_cluster = clusters[i];
        if (curr_cluster != NULL) {
            free(curr_cluster->curr_centroids);
            free(curr_cluster->prev_centroids);
            free(curr_cluster->sum);
            free(curr_cluster);
        }
    }
}

void free_points_memory (double** points, int num_points) {
    /*
    Recieves 2D array of pounts and number of points,
    Frees up memory used by all points.
    */
    int i;

    for (i=0; i<num_points; i++) {
        free(points[i]);
    }
}


double get_distance (Cluster* cluster, const double* point, int num_coordinates) {
    /*
    Recieves pointer to cluster, pointer to point and number of coordinates,
    Returns euclidean distance of point from cluster.
    */
    double distance;
    double toAdd;
    int i;

    distance = 0;
    for (i=0; i<num_coordinates; i++) {
        toAdd = (cluster->curr_centroids[i] - point[i]);
        distance += (toAdd*toAdd);
    }
    return distance;
}

/*******************************/
/*** Module setup ***/
/*******************************/

/*TODO: verift syntax is correct*/
static PyMethodDef _methods[] = {
        {"fit",
                (PyCFunction) c_kmeans,
                     METH_VARARGS,
                        PyDoc_STR("Kmeans execution with given initial centroids."),
        },
        {NULL, NULL, 0, NULL} /*Sentinel*/
};

static struct PyModuleDef _moduledef  = {
        PyModuleDef_HEAD_INIT,
        "mykmeanssp",
        NULL,
        -1,
        _methods
};

PyMODINIT_FUNC
PyInit_mykmeanssp(void)
{
    PyObject *m;
    m = PyModule_Create(&_moduledef);
    if (!m) {
        return NULL;
    }
    return m;
}


