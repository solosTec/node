# node
**node** is a Smart Metering Framework (SMF) build upon the [CYNG](https://github.com/solosTec/cyng) library.

## Introduction ##

* Current version is 0.1. It's a very early version.
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
