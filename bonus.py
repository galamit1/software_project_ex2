import matplotlib.pyplot as plt

import numpy as np
from sklearn.cluster import KMeans
from sklearn import datasets


def main():
    points = datasets.load_iris().data
    ks = np.array(range(1, 11))
    plot = np.array([])
    for k in ks:
        distances = np.array([])
        centroids = KMeans(n_clusters=k, random_state=0).fit(points).cluster_centers_
        for point in points:
            distances = np.append(distances, np.min(
                np.sum(
                    np.power(np.subtract(np.multiply(point, np.ones((centroids.shape[0], points.shape[1]))), centroids),
                             2), axis=1)))
        plot = np.append(plot, distances.sum())
    plt.plot(ks, plot)
    plt.savefig('elbow.png')


if __name__ == '__main__':
    main()
