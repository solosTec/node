# node
**node** is a Smart Metering Framework (SMF) build upon the [CYNG](https://github.com/solosTec/cyng) library.

## Introduction ##

* Current version is 0.3. It's a beta version and starts to be usefull.
* Linux (64 bit) are supported
* Windows 7 (64 bit) or higher are supported.
* Crosscompiling for Raspberry 3 is supported
* Requires [g++](https://gcc.gnu.org/) >= 4.8 or cl 19.00.24215.1 (this is VS 14.0) and [boost](http://www.boost.org/) >= 1.66.0

## How do I get set up? ##

* Buildsystem is based on [cmake](http://www.cmake.org/) >= 3.5
* Download or clone from [github](https://github.com/solosTec/node.git)

To build the SMF first install the [CYNG](https://github.com/solosTec/cyng) library, then run:


```
#!shell

git clone https://github.com/solosTec/node.git
mkdir build
cd build
cmake ..
make -j4 all
sudo make install

```

## License ##

SMF is free software under the terms of the [MIT License](https://github.com/solosTec/node/blob/master/LICENSE).


## Build Boost on Linux ##

Some hints to build Boost since the SMF requires the latest Boost version 1.66.0:

* Download the latest [version](https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2)
* wget -c https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2
* For unicode support install ICU: (sudo apt install libicu-dev). 
* tar -xvjf boost_1_66_0.tar.bz2
* cd boost_1_66_0/
* ./bootstrap.sh --prefix=release --with-toolset=gcc --with-icu
* Build the library with ./b2 --architecture=x86 --address-model=64 install -j 4. Depending on your machine this may take some time. 
* Instruct CMake to use the path to the Boost library specified with the --prefix option
* Make sure to update your linker configuration to find this path. To make changes permanent add a .conf file below /etc/ld.so.conf.d/.

