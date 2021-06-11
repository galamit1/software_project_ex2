import sys

import mykmeanssp
import numpy as np
import pandas as pd
import pandasql

POINTS_SEPARATOR = '\n'
COORDINATES_SEPARATOR = ','
JOIN_TABLES_QUERY = """SELECT {}
                    FROM data_1 as a
                    INNER JOIN data_2 as b
                    ON a.'0'=b.'0' 
                    ORDER BY a.'0'"""


def get_points(file_1, file_2):
    data_1 = pd.read_csv(file_1, header=None)
    data_2 = pd.read_csv(file_2, header=None)
    select_columns = "a.'" + "',a.'".join(map(str, range(1, len(data_1.columns)))) + "'," \
                     + "b.'" + "',b.'".join(map(str, range(1, len(data_2.columns)))) + "'"
    return np.array(pandasql.sqldf(JOIN_TABLES_QUERY.format(select_columns), locals()).values)


def calc_centroids_indexes(points, k):
    np.random.seed(0)
    centroids_indexes = np.random.choice(points.shape[0], 1)
    for z in range(1, k):
        distances = np.array([])
        for point in points:
            distance = min([np.power(np.subtract(point, points[j]), 2).sum() for j in centroids_indexes])
            distances = np.append(distances, distance)
        probs = np.divide(distances, distances.sum())
        centroids_indexes = np.append(centroids_indexes, np.random.choice(points.shape[0], 1, p=probs), axis=0)
        centroids_indexes.sort()
    return centroids_indexes


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
    centroids_indexes = calc_centroids_indexes(points, k)
    print(centroids_indexes)

    centroids = [points[i].tolist() for i in centroids_indexes]
    centroids_output = np.array(mykmeanssp.fit(points.tolist(), centroids, k, max_iter, len(points), len(points[0])))
    centroids_output = np.round(centroids_output, decimals=4)
    print(POINTS_SEPARATOR.join([COORDINATES_SEPARATOR.join([str(c) for c in centroid]) for centroid in centroids_output.tolist()]))


if __name__ == '__main__':
    main()
