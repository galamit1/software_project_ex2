from setuptools import setup, find_packages, Extension

setup(
    name='kmeans_CPython_API',
    version='0.0.1',
    author='Gal and Ben',
    description='ex2 - CPython API',
    install_requires=['invoke',
                      'numpy>=1.18.2',
                      'pandas>=1.0.3',
                      'pandasql>=0.7.3'],
    packages=find_packages(),
    license='GPL-2',
    ext_modules=[
        Extension(
            'mykmeanssp', ['kmeans.c'],
        ),
    ]
)