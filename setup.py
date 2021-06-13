from setuptools import setup, find_packages, Extension

setup(
    name='kmeans_CPython_API',
    version='0.0.1',
    author='Gal and Ben',
    description='ex2 - CPython API',
    install_requires=['invoke',
                      'numpy',
                      'pandas',
                      'matplotlib',
                      'sklearn'],
    packages=find_packages(),
    license='GPL-2',
    ext_modules=[
        Extension(
            'mykmeanssp', ['kmeans.c'],
        ),
    ]
)