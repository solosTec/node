# node
**node** is a Smart Metering Framework (SMF) build upon the [CYNG](https://github.com/solosTec/cyng) library.

## Introduction ##

* Current version is 0.7. Core functions are ready for productive use: 
  * master 
  * setup 
  * ipt 
  * store 
  * dash
  * e350/iMega
  * csv
* Linux (32/64 bit) are supported
* Windows 7 (64 bit) or higher are supported.
* Crosscompiling for Raspberry 3 is supported
* Requires [g++](https://gcc.gnu.org/) >= 4.8 or cl 19.00.24215.1 (this is VS 14.0), [boost](http://www.boost.org/) >= 1.66.0 and [pugixml](https://pugixml.org/) 1.8

## How do I get set up? ##

* Buildsystem is based on [cmake](http://www.cmake.org/) >= 3.5
* Download or clone from [github](https://github.com/solosTec/node.git)

To build the SMF first install the [CYNG](https://github.com/solosTec/cyng) library, then run:


### on Linux ###
#### native build ####

```
#!shell

git clone https://github.com/solosTec/node.git
mkdir build/x64 && cd build/x64
cmake -DNODE_BUILD_TEST:BOOL=ON -DNODE_CROSS_COMPILE:bool=OFF -DNODE_SSL_SUPPORT:BOOL=ON -DOPENSSL_ROOT_DIR:PATH=$HOME/projects/install/x64/openssl -DBOOST_ROOT:PATH=$HOME/projects/install/x64/boost -DDCYNG_ROOT:PATH=$HOME/projects/cyng -DCYNG_ROOT_BUILD_SUBDIR:STRING=build/x64 -DCRYPT_ROOT:PATH=$HOME/projects/crypto -DCMAKE_BUILD_TYPE=Debug ../..
sudo apt-get install libreadline-dev
make -j4 all
sudo make install

```
#### OECP build ####

```
#!shell

git clone https://github.com/solosTec/node.git
mkdir build/v5te && cd build/v5te
cmake -DNODE_BUILD_TEST:BOOL=OFF -DNODE_CROSS_COMPILE:BOOL=ON -DNODE_SSL_SUPPORT:BOOL=ON -DBOOST_ROOT:PATH=$HOME/projects/install/x64/boost -DDCYNG_ROOT:PATH=$HOME/projects/cyng -DCYNG_ROOT_BUILD_SUBDIR:STRING=build/v5te -DCRYPT_ROOT:PATH=$HOME/projects/crypto -DCMAKE_TOOLCHAIN_FILE=../../../cross-oecp.cmake -DCMAKE_BUILD_TYPE=Release ../..
make -j4 all
```

### on Windows ###


```
git clone https://github.com/solosTec/node.git
mkdir build 
cd build
cmake -DNODE_BUILD_TEST:bool=ON -G "Visual Studio 15 2017" -A x64 ..
```

Open _NODE.sln_ with Visual Studio.

It is recommended to use the latest boost library version. It is best to create them by hand.
Some hints to build Boost on Linux:

* Download the latest [version](https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.bz2)
* For unicode support install ICU: (sudo apt install libicu-dev) and call bootstrap with option --with-icu
* Build the library with b2. Depending on your machine this may take some time. Choose option -j depending on the CPU count.
* Instruct CMake to use the path to the Boost library specified with the --prefix option
* Make sure to update your linker configuration to find this path. To make changes permanent add a .conf file below /etc/ld.so.conf.d/.


#### using Boost ####

```
wget -c https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.tar.bz2
tar -xvjf boost_1_72_0.tar.bz2
cd boost_1_72_0/
./bootstrap.sh --prefix=$HOME/projects/install/x64/boost --with-toolset=gcc
./b2 --architecture=x86 --address-model=64 install -j2
```

## License ##

SMF is free software under the terms of the [MIT License](https://github.com/solosTec/node/blob/master/LICENSE).
