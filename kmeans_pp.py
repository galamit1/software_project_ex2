import mykmeanssp

centroids = [1,3,5,7]
points = list(range(1,20))
k=5
max_iter=10

centroids = mykmeanssp.fit(1.1, 4.5, k, max_iter)
print(centroids)