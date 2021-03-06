<a href="http://homepages.cs.ncl.ac.uk/f.shmarov/probreach/" target="_blank">
        <img style="align:center" src="http://homepages.cs.ncl.ac.uk/f.shmarov/probreach/img/banner-alt.gif" alt="ProbReach banner"/>
</a>

ProbReach - software for calculating bounded probabilistic reachability and performing parameter set synthesis in hybrid systems with uncertainty in initial parameters. ProbReach supports formal verification (computing rigorous enclosures) and statistical model checking (Chernoff-Hoeffding bound and Bayesian estimations).

* *The parameter set synthesis is not currently supported.*

* **NOW ON [DOCKER](https://docker.com)!!!**

Install
====================
Latest version of static binary for Linux and Mac can be downloaded from ProbReach [releases page](https://github.com/dreal/probreach/releases)

How to Build
====================
1. Required packages
--------------------
In order to build ProbReach you will need to install the following packages
- [>=gcc-4.9](https://gcc.gnu.org/gcc-4.9/)
- [dReal](https://github.com/dreal/dreal3)
- [GSL](http://www.gnu.org/software/gsl/)

2. Build ProbReach
--------------------
```
git clone https://github.com/dreal/probreach.git probreach
cd probreach
mkdir -p build/release
cd build/release
cmake ../../
make
```
If your dReal location is different from ```/usr/local/src/dreal``` run
```
cmake -DDREAL_DIR=/path/to/dreal/directory ../../
```

3. Docker
------------------
```
docker pull dreal/probreach
```
