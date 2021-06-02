#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

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

int get_num_coordinates(char* sinlge_line);
double** init_points (int num_points, int num_coordinates);
double* convert_line_to_point (double* point, char* line);
Cluster* make_cluster (const double* point, const int num_coordinates, const int index);
Cluster** init_k_clusters (double** points, int k, int num_coordinates);
void add_point_to_cluster (Cluster* cluster, const double* point, int num_coordinates);
void find_minimum_distance (Cluster** clusters, double* point, int k, int num_coordinates);
double update_cluster_centroids_and_sum (Cluster* cluster, int num_coordinates);
int update_all_clusters (Cluster** clusters, int k, int num_coordinates);
void free_clusters_memory (Cluster** clusters, int k);
void free_points_memory (double** points, int num_points);
double get_distance (Cluster* cluster, const double* point, int num_coordinates);

static Pyobject *fit_capi(PyObject *self, PyObject *args) ;

/*******************************/
/*** Module setup ***/
/*******************************/

static Pyobject *fit_capi(PyObject *self, PyObject *args) {
    double centroids, points;
    int k, max_iter;

    /* Parse arguments */
    if(!PyArg_ParseTuple(args, "ddii", &centroids, &points, k, max_iter)) {
        return NULL;
    }
    printf("k is %d, max_iter is: %d", k, max_iter);
}

static PyMethodDef fitMethods [] = {
        {"fit",
                (PyCFunction) fit_capi,
                     METH_VARARGS,
                        PyDoc_STR("Kmeans function")
        },
        {NULL, NULL, 0, NULL}
};

static struct PyModuleDef moduleDef  = {
        PyModuleDef_HEAD_INIT,
        "mykmeanssp",
        NULL,
        -1,
        fitMethods
};




/*******************************/
/*** Main Function ***/
/*******************************/

int main(int argc, char **argv) { 
    /*argc = number of parameters (including program name), **argv = string array of parameters.*/

    /***Variables declarations***/
    int k;
    int max_iter;
    int num_points;
    int num_coordinates;
    char single_line[1000]; /*Rami gave assumption each line's length is <= 1000*/
    int line_num;
    double **points;
    double *point;
    Cluster **clusters;
    int did_update;
    int i;
    int j;

    /***Input validation***/
    if (argc < 2 || argc > 3) {
        printf("Invalid input: number of parameters should be 2 or 3.");
    }

    sscanf_s(argv[1], "%d", &k);

    if (argc > 2) { /*max_iter was passed by user*/
        sscanf_s(argv[2], "%d", &max_iter);
    }
    else { /*max_iter not passed by user*/
        max_iter = 200;
    }

    if (k <= 0 || max_iter <= 0) {
        printf("Invalid input: k and max_iter should both be > 0");
        exit(0);
    }

    if (stdin == NULL) {
        printf ("Couldn't open file");
        exit(0);
    }

    /***Get data from file***/
    fgets(single_line , 1000, stdin);
    num_coordinates = get_num_coordinates(single_line);
    num_points = 1; /*Already scanned first line*/
    while (fgets(single_line, 1000, stdin) != NULL) {
        num_points++;
    }

    rewind(stdin); /*Reset pointer to head of file*/

    if (k > num_points) {
        printf ("Invalid input: more clusters than points");
        exit(0);
    }

    points = init_points(num_points, num_coordinates);
    line_num = 0;
    fgets(single_line, 1000, stdin); /*Rami gave assumption there's at least 1 point*/

    do {
        convert_line_to_point(points[line_num], single_line);
        line_num++;
    }
    while (fgets(single_line, 1000, stdin) != NULL);


    fclose(stdin);

    /***Execute k-means algorithm***/
    clusters = init_k_clusters(points, k, num_coordinates);
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

    /***Print results***/
    for (i=0; i<k; i++) {
        for (j=0; j<num_coordinates; j++) {
            printf("%.4f", clusters[i]->curr_centroids[j]);
            if (j != (num_coordinates-1)) {
                printf(",");
            }
        }
        printf("\n");
    }

    free_clusters_memory(clusters, k);


    return 0;

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

Cluster** init_k_clusters (double **points, int k, int num_coordinates) {
    /*
    Recieves pointer to 2D array of points, k and number of coordinates,
    Returns new 2D array of Clusters with sufficient memory, initialized with the first k points.
    */
    Cluster **clusters;
    int i;

    clusters = malloc(sizeof(Cluster*) * k);
    assert (clusters != NULL);

    for (i=0; i<k; i++) {
        clusters[i] = make_cluster(points[i], num_coordinates, i);
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



