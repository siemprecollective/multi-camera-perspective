CC=clang++ -std=c++11 -g
DETECT_FACE_TARGETS=client/detect-face.cpp lib/PracticalSocket.cpp
DRIVE_WEBCAM_TARGETS=client/drive-webcam.cpp lib/PracticalSocket.cpp lib/arduino-serial-lib.c
ECHO_SERVER_TARGETS=server/server.cpp lib/PracticalSocket.cpp

all: detect-face drive-webcam echo-server
detect-face: $(DETECT_FACE_TARGETS)
	$(CC) `pkg-config --libs --cflags opencv4` $(DETECT_FACE_TARGETS) -o detect-face
drive-webcam: $(DRIVE_WEBCAM_TARGETS)
	$(CC) $(DRIVE_WEBCAM_TARGETS) -o drive-webcam
echo-server: $(ECHO_SERVER_TARGETS)
	$(CC) $(ECHO_SERVER_TARGETS) -o echo-server
