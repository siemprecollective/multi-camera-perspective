#Dependencies

libusb-1.0, opencv >= 4.0.0

```
brew install libusb opencv
```

#Usage

build with

```
make
```

you'll get 3 binaries: `detect-face`, `echo-server` and `drive-webcam`

run them as follow:

```
echo-server <port>

detect-face <server> <port> <room>

drive-webcam <serial port path> <server> <port> <room>
```
