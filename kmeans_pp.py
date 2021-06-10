import sys

import mykmeanssp
import numpy as np
import pandas as pd
import pandasql

JOIN_TABLES_QUERY = """select {}
                    from data_1 as a
                    inner join data_2 as b
                    on a.'0'=b.'0' """


def get_points(file_1, file_2):
    data_1 = pd.read_csv(file_1, header=None)
    data_2 = pd.read_csv(file_2, header=None)
    select_columns = "a.'" + "',a.'".join(map(str, range(1, len(data_1.columns)))) + "'," \
                     + "b.'" + "',b.'".join(map(str, range(1, len(data_2.columns)))) + "'"
    return np.array(pandasql.sqldf(JOIN_TABLES_QUERY.format(select_columns), locals()).values)


def calc_centroids(points, k):
    centroids = points[np.random.choice(points.shape[0], 1)]
    for z in range(1, k):
        distances = np.array([])
        for point in points:
            distance = min(
                [np.power(np.subtract(point, centroids[j]), 2).sum() for j in range(0, centroids.shape[0])])
            distances = np.append(distances, distance)
        probs = np.divide(distances, distances.sum())
        centroids = np.append(centroids, points[np.random.choice(points.shape[0], 1, p=probs)], axis=0)
    return centroids


def main():
    assert len(sys.argv) == 5
    k = int(sys.argv[1])
    max_iter = int(sys.argv[2])
    file_1 = sys.argv[3]
    file_2 = sys.argv[4]
    points = get_points(file_1, file_2)
    centroids = calc_centroids(points, k)
    print(centroids)
    centroids = mykmeanssp.fit(points.tolist(), centroids.tolist(), k, max_iter, len(points), len(points[0]))
    print(centroids)


if __name__ == '__main__':
    main()
