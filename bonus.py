import matplotlib.pyplot as plt

import numpy as np
from sklearn.cluster import KMeans
from sklearn import datasets
from kneed import KneeLocator


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
    
    kn = KneeLocator([i for i in range(1,11)], plot, curve='convex', direction='decreasing')
    elbow = (kn.knee, plot[kn.knee])
    plt.annotate('Elbow Point', xy=elbow, xytext=(60, 60),
            textcoords='offset points',
            color='b', size='large',
            arrowprops=dict(
                arrowstyle='simple,tail_width=0.3,head_width=0.8,head_length=0.8',
                facecolor='b', shrinkB=20))

    plt.plot(ks, plot)
    plt.xlabel('K')
    plt.ylabel('Average Dispersion')
    plt.savefig('elbow.png')


if __name__ == '__main__':
    main()
