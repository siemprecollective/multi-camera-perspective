#include "../lib/PracticalSocket.h"

#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/core/ocl.hpp>

#include <chrono>
#include <thread>
#include <cstdlib>

#define TRANSLATION_TRAVEL 6.25 // inches
#define FOCAL_LENGTH 48.0 // inches

#define TRANSLATION_MIN 1.0/3
#define TRANSLATION_MAX 2.0/3

#define ROTATION_MIN 1.0/3
#define ROTATION_MAX 2.0/3

using namespace cv;
using namespace std;

uint8_t ratio_to_control_byte(double ratio) {
    int out = int(ratio * 84);
    if (out >= 84) {
        return uint8_t(84);
    }
    if (out < 0) {
        return uint8_t(0);
    }
    return uint8_t(out);
}

uint8_t map_to_translation(double ratio) {
    ratio = ((ratio) - TRANSLATION_MIN) / (TRANSLATION_MAX - TRANSLATION_MIN);
    return uint8_t(85+ratio_to_control_byte(ratio));
}

uint8_t map_to_rotation_x(double ratio) {
    //ratio = ((ratio) - TRANSLATION_MIN) / (TRANSLATION_MAX - TRANSLATION_MIN);
    if (ratio < ROTATION_MIN) {
        ratio = ROTATION_MIN;
    } else if (ratio > ROTATION_MAX) {
        ratio = ROTATION_MAX;
    }
    /* This is perspective shift rather than looking around the room
    auto translated = (ratio - 0.5) * TRANSLATION_TRAVEL;
    auto angle = atan(translated/FOCAL_LENGTH);
    ratio = (angle + M_PI/2) / M_PI;
    */
    //std::cout << ratio << std::endl;
    return uint8_t(ratio_to_control_byte(ratio));
}

uint8_t map_to_rotation_y(double ratio) {
    if (ratio < ROTATION_MIN) {
        ratio = ROTATION_MIN;
    } else if (ratio > ROTATION_MAX) {
        ratio = ROTATION_MAX;
    }
    return uint8_t(169+ratio_to_control_byte(ratio));
}

int i = 0;
int main(int argc, char **argv)
{
    char* server = argv[1];
    int port = std::atoi(argv[2]);
    char* to = argv[3];

    TCPSocket sock(server, port);
    sock.send("send", 5);
    uint64_t len = std::strlen(to);
    sock.send(&len, sizeof(len));
    sock.send(to, std::strlen(to));

    auto mainCam = VideoCapture(0);
    mainCam.set(CAP_PROP_FRAME_HEIGHT, 240);
    mainCam.set(CAP_PROP_FRAME_WIDTH, 480);
    auto mainHeight = mainCam.get(CAP_PROP_FRAME_HEIGHT);
    auto mainWidth = mainCam.get(CAP_PROP_FRAME_WIDTH);
    auto tracker = TrackerKCF::create();

    Mat frame;
    mainCam >> frame;

    std::vector<Rect> faces;
    Rect2d bbox(0, 0, 0, 0);
    CascadeClassifier face_cascade("./haarcascade_frontalface_default.xml");
    face_cascade.detectMultiScale(frame, faces);
    if (faces.size() > 0) {
      bbox = faces[0];
    }

    destroyWindow("ROI selector");
    tracker->init(frame, bbox);

    namedWindow("tracking");

    while(true) {
        double timer = (double)getTickCount();
        mainCam >> frame;
        bool ok = tracker->update(frame, bbox);
        if (ok) {
          rectangle(frame, bbox, Scalar( 255, 0, 0 ), 2, 1 );
          flip(frame, frame, 1);
        } else {
          face_cascade.detectMultiScale(frame, faces);
          if (faces.size() > 0) {
              bbox = faces[0];
          }
          tracker->clear();
          tracker = TrackerKCF::create();
          tracker->init(frame, bbox);
          flip(frame, frame, 1);
        }
        imshow("tracking", frame);

        auto bbox_center_x = (bbox.x + bbox.width/2);
        auto bbox_center_y = (bbox.y + bbox.height/2);
        auto ratio_x = bbox_center_x / mainWidth;
        auto ratio_y = bbox_center_y / mainHeight;

        uint8_t translation = map_to_translation(ratio_x);
        uint8_t rotation_x = map_to_rotation_x(ratio_x);
        uint8_t rotation_y = map_to_rotation_y(ratio_y);

        std::cout << (int) translation << " " << (int) rotation_x << " " << (int) rotation_y << std::endl;

        //serialport_writebyte(fd, translation);
        sock.send(&rotation_x, 1);
        sock.send(&rotation_y, 1);

        int k = waitKey(1);
        if(k == 27) {
            face_cascade.detectMultiScale(frame, faces);
            if (faces.size() > 0) {
                bbox = faces[0];
            }
            tracker->clear();
            tracker = TrackerKCF::create();
            tracker->init(frame, bbox);
        }
    }
}
