#Dependencies

libusb-1.0, opencv >= 4.0.0

```
brew install libusb opencv
```

#Usage

build and run with

```
clang++ `pkg-config --cflags --libs libusb-1.0 opencv4` -std=c++17 ps3eye.cpp test.cpp -o track-test && ./track-test
```
