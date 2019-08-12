#include "ps3eye.h"
#include "arduino-serial-lib.h"

#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/core/ocl.hpp>

#include <chrono>
#include <thread>
#include <cstdlib>

#define PS3_EYE_HEIGHT 480
#define PS3_EYE_WIDTH 640
#define PS3_EYE_CHANNELS 3
#define PS3_EYE_HZ 60

#define TRANSLATION_TRAVEL 6.25 // inches
#define FOCAL_LENGTH 48.0 // inches

#define TRANSLATION_MIN 1.0/3
#define TRANSLATION_MAX 2.0/3

#define ROTATION_MIN 1.0/4
#define ROTATION_MAX 3.0/4

// {"BOOSTING", "MIL", "KCF", "TLD","MEDIANFLOW", "GOTURN", "MOSSE", "CSRT"};

using namespace cv;
using namespace std;

// Convert to string
#define SSTR( x ) ( \
( std::ostringstream() << std::dec << x ) ).str()

struct EyeCam {
    ps3eye::PS3EYECam::PS3EYERef eye;
    uint8_t buf[PS3_EYE_HEIGHT * PS3_EYE_WIDTH * PS3_EYE_CHANNELS];
    Mat frame;

    EyeCam(ps3eye::PS3EYECam::PS3EYERef eye_) :
        eye(eye_)
    {
        eye->init(PS3_EYE_WIDTH, PS3_EYE_HEIGHT, PS3_EYE_HZ);
    }

    void start() {
        eye->start();
    }

    void stop() {
        eye->stop();
    }

    void update() {
        eye->getFrame(buf);
        frame = Mat(PS3_EYE_HEIGHT, PS3_EYE_WIDTH, CV_8UC3, buf);
    }
};

uint8_t ratio_to_control_byte(double ratio) {
    int out = int(ratio * 128);
    if (out >= 127) {
        return uint8_t(127);
    }
    if (out < 0) {
        return uint8_t(0);
    }
    return uint8_t(out);
}

uint8_t map_to_translation(double ratio) {
    ratio = ((ratio) - TRANSLATION_MIN) / (TRANSLATION_MAX - TRANSLATION_MIN);
    return ratio_to_control_byte(ratio);
}

uint8_t map_to_rotation(double ratio) {
    ratio = ((ratio) - TRANSLATION_MIN) / (TRANSLATION_MAX - TRANSLATION_MIN);
    /*
    if (ratio < ROTATION_MIN) {
        ratio = ROTATION_MIN;
    } else if (ratio > ROTATION_MAX) {
        ratio = ROTATION_MAX;
    }
    */
    auto translated = (ratio - 0.5) * TRANSLATION_TRAVEL;
    auto angle = atan(translated/FOCAL_LENGTH);
    ratio = (angle + M_PI/2) / M_PI;
    std::cout << ratio << std::endl;
    return uint8_t(128+ratio_to_control_byte(ratio));
}

int i = 0;
int main(int argc, char **argv)
{
    auto eyeDevices = ps3eye::PS3EYECam::getDevices();
    if (eyeDevices.size() <= 0) {
        std::cerr << "couldn't find ps3 eye cameras" << std::endl;
        return 1;
    }

    auto eye = EyeCam(eyeDevices[0]);
    eye.start();

    auto mainCam = VideoCapture(0);
    mainCam.set(CAP_PROP_FRAME_HEIGHT, 240);
    mainCam.set(CAP_PROP_FRAME_WIDTH, 480);
    auto mainHeight = mainCam.get(CAP_PROP_FRAME_HEIGHT);
    auto mainWidth = mainCam.get(CAP_PROP_FRAME_WIDTH);
    auto tracker = TrackerKCF::create();

    Mat frame;
    mainCam >> frame;
    //Rect2d bbox = selectROI(frame);

    std::vector<Rect> faces;
    Rect2d bbox(0, 0, 0, 0);
    CascadeClassifier face_cascade("/usr/local/Cellar/opencv/4.1.0_2/share/opencv4/haarcascades/haarcascade_frontalface_default.xml");
    face_cascade.detectMultiScale(frame, faces);
    if (faces.size() > 0) {
      bbox = faces[0];
    }

    destroyWindow("ROI selector");
    tracker->init(frame, bbox);

    namedWindow("tracking");
    moveWindow("tracking", 0, PS3_EYE_HEIGHT);

    int fd = serialport_init("/dev/cu.usbmodem14101", 9600);
    serialport_flush(fd);

    auto index = 0;
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
        auto ratio = bbox_center_x / mainWidth;

        uint8_t translation = map_to_translation(ratio);
        uint8_t rotation = map_to_rotation(ratio);

        std::cout << (int) translation << " " << (int) rotation << std::endl;
        serialport_writebyte(fd, translation);
        serialport_writebyte(fd, rotation);

        eye.update();
        imshow("final", eye.frame);

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
