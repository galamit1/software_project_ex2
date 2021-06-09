#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#define PY_SSIZE_T_CLEAN
#include <Python.h>

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
int get_num_coordinates(char* sinlge_line);
double** init_points (int num_points, int num_coordinates);
double* convert_line_to_point (double* point, char* line);
Cluster* make_cluster (const double* point, const int num_coordinates, const int index);
/*Cluster** init_k_clusters (double** points, int k, int num_coordinates);*/
Cluster** python_init_k_clusters (double **points, int k, int num_coordinates, long *init_indexes_array );
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
    PyObject *initial_indexes;
    PyObject *index_item;
    Py_ssize_t list_size;
    Py_ssize_t entry_size;
    PyObject *point_item;
    PyObject *coordinate_item;
    double **points;
    double *point;
    Cluster **clusters;
    int did_update;
    int i;
    int j;
    long *init_indexes_array;

    /*TODO: data_points in Python should be list of lists*/


    /* Parse arguments from Python */
    if((!PyArg_ParseTuple(args, "OOiiii", &data_points, &initial_indexes, &k, &max_iter, &num_points, &num_coordinates))) {
        return NULL; /*In the CPython API, Null is never a valid value for a PyObject* - so it signals an error*/
    }

    /*Verify that data_points & initial_indexes are python lists*/
    if (!PyList_Check(data_points) || !PyList_Check(initial_indexes)) {
        return NULL;
    }

    /*TODO: include input validation in main Python file*/
    
    /*Load points from PyObject into C format of 2D-array points*/
    points = init_points(num_points, num_coordinates);

    list_size = PyList_Size(data_points); /*equivilant to len(_list) in Python*/
    for (i=0; i<list_size; i++) { /*iterate over points*/
        point_item = PyList_GetItem(data_points, i);
        if (!PyList_Check(point_item)) {
            return NULL;
        }
        entry_size = PyList_Size(point_item);
        for (j=0; j<entry_size; j++) { /*iterate over coordinates of single point*/
            coordinate_item = PyList_GetItem(point_item, j);
            if (!PyFloat_Check(coordinate_item)) {
                return NULL;
            };
            points[i][j] = PyFloat_AsDouble(coordinate_item);
        }
    }

    /*Load initial indexes from PyObject into C format of array */
    init_indexes_array = calloc(k, sizeof(long));
    assert(init_indexes_array != NULL);

    for (i=0; i<k; i++) { /*iterate over initial indexes*/
        index_item = PyList_GetItem(initial_indexes, i);
        if (!PyLong_Check(index_item)) {
            return NULL;
        }
        init_indexes_array[i] = PyLong_AsLong(index_item);
    }

    /***Execute k-means algorithm***/
    clusters = python_init_k_clusters(points, k, num_coordinates, init_indexes_array);
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

    /*TODO: Convert results to Python Object*/

    free_clusters_memory(clusters, k);

    /*TODO: return Python Object that holds results*/

    Py_RETURN_NONE; 
}



/*******************************/
/*** Function Implementation ***/
/*******************************/


int get_num_coordinates(char* sinlge_line) {
    /*
    Recieves pointer to char array containing the first line (=point),
    Returns number of coordinates in each point (we can assume all points have same amount of coordinates).
    */
    int count_coordinates = 1;
    char* pointer = sinlge_line;

    while ((pointer = strchr(pointer, ',')) != NULL) {
        pointer++;
        count_coordinates++;
    }
    
    return count_coordinates;
}


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


double* convert_line_to_point (double* point, char* line) {
     /*
    Recieves pointer to char array containing a line (=point) and pointer to initialized double-array (=point),
    Returns the same point after filling it with double values from the line, using string.h functio strtok.
    Reference: https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
    */
    char* tokens;
    int index;

    index = 0;

    tokens = strtok(line, ",");

    while (tokens != NULL) {
        point[index] = atof(tokens);
        tokens = strtok(NULL, ",");
        index++;
    }

    return point;
}



Cluster* make_cluster (const double* point, const int num_coordinates,const int index) {
    /*
    Recieves a point, number of coordinates and a specific cluster index,
    Returns new Cluster with sufficient memory space to store both the current centroid and the previous centroid.
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

/*To Delete:
Cluster** init_k_clusters (double **points, int k, int num_coordinates) {
    
    Recieves pointer to 2D array of points, k and number of coordinates,
    Returns new 2D array of Clusters with sufficient memory, initialized with the first k points.
    
    Cluster **clusters;
    int i;

    clusters = malloc(sizeof(Cluster*) * k);
    assert (clusters != NULL);

    for (i=0; i<k; i++) {
        clusters[i] = make_cluster(points[i], num_coordinates, i);
    }

    return clusters;
}
*/

Cluster** python_init_k_clusters (double **points, int k, int num_coordinates, long *init_indexes_array ) {
    /*
    Recieves pointer to 2D array of points, k, number of coordinates and array of initial indexes,
    Returns new 2D array of Clusters with sufficient memory, initialized with the first k points.
    */
    Cluster **clusters;
    int i;
    long initial_index;

    clusters = malloc(sizeof(Cluster*) * k);
    assert (clusters != NULL);

    for (i=0; i<k; i++) {
        initial_index = init_indexes_array[i];
        clusters[i] = make_cluster(points[initial_index], num_coordinates, i);
    }

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
    return PyModule_Create(&_moduledef);
}


