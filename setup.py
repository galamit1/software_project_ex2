from setuptools import setup, find_packages, Extension

setup(
    name='ex2',
    version='0.0.1',
    author='Gal and Ben',
    description='ex2',
    install_requires=['invoke'],
    packages=find_packages(),
    license='LICENSE.txt',
    ext_modules=[
        Extension(
            'mykmeanssp', ['kmeans.c'],
        ),
    ]
)