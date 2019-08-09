#Dependencies

libusb-1.0, opencv == 3.4.2

#Usage

build and run with

```
clang++ `pkg-config --cflags --libs libusb-1.0 opencv` -std=c++17 ps3eye.cpp test.cpp -o track-test && ./track-test
```
