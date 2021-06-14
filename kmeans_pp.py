import sys

import mykmeanssp
import numpy as np
import pandas as pd


POINTS_SEPARATOR = '\n'
COORDINATES_SEPARATOR = ','


def get_points(file_1, file_2):
    data_1 = pd.read_csv(file_1, header=None)
    data_2 = pd.read_csv(file_2, header=None)
    points = pd.merge(data_1, data_2, how='inner', on=0, sort=True)
    return np.array(points[points.columns[1:]])


def calc_centroids_indexes(points, k):
    np.random.seed(0)
    points_num = points.shape[0]
    centroids_indexes = np.random.choice(points_num, 1)
    centroids = points[[centroids_indexes[0]]]
    distances = np.ones(points_num) * float('inf')
    for z in range(1, k):
        for i in range(points_num):
            distances[i] = min(distances[i], np.sum(np.power(np.subtract(points[i], centroids[-1]), 2)))
        probs = np.divide(distances, distances.sum())
        centroids_indexes = np.append(centroids_indexes, np.random.choice(points_num, 1, p=probs), axis=0)
        centroids = np.append(centroids, points[[centroids_indexes[-1]]], axis=0)
    return centroids_indexes, centroids


def main():
    assert len(sys.argv) in [4, 5]
    k = int(sys.argv[1])
    if len(sys.argv) == 4:
        max_iter = 300
        file_1 = sys.argv[2]
        file_2 = sys.argv[3]

    if len(sys.argv) == 5:
        max_iter = int(sys.argv[2])
        file_1 = sys.argv[3]
        file_2 = sys.argv[4]

    points = get_points(file_1, file_2)
    assert k < points.shape[0]
    if k == 0:
        return
    centroids_indexes, centroids = calc_centroids_indexes(points, k)
    print(COORDINATES_SEPARATOR.join([str(c) for c in centroids_indexes]))
    centroids_output = np.array(mykmeanssp.fit(points.tolist(), centroids.tolist(), k, max_iter, len(points), len(points[0])))
    centroids_output = np.round(centroids_output, decimals=4)
    print(POINTS_SEPARATOR.join([COORDINATES_SEPARATOR.join([str(c) for c in centroid]) for centroid in centroids_output.tolist()]))


if __name__ == '__main__':
    main()
