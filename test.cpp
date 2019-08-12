#include "ps3eye.h"

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

int main(int argc, char **argv)
{
    auto eyeDevices = ps3eye::PS3EYECam::getDevices();
    if (eyeDevices.size() <= 0) {
        std::cerr << "couldn't find ps3 eye cameras" << std::endl;
        return 1;
    }

    std::vector<EyeCam> eyes;
    std::vector<bool> started;
    std::vector<bool> toStart;
    for (auto& eye : eyeDevices) {
      eyes.emplace_back(eye);
      started.push_back(false);
      toStart.push_back(false);
    }

    auto mainCam = VideoCapture(0);
    mainCam.set(CAP_PROP_FRAME_HEIGHT, 240);
    mainCam.set(CAP_PROP_FRAME_WIDTH, 480);
    auto mainHeight = mainCam.get(CAP_PROP_FRAME_HEIGHT);
    auto mainWidth = mainCam.get(CAP_PROP_FRAME_WIDTH);
    auto tracker = TrackerKCF::create();

    Mat frame;
    mainCam >> frame;
    Rect2d bbox = selectROI(frame);
    destroyWindow("ROI selector");
    tracker->init(frame, bbox);

    /*
    namedWindow("info");
    moveWindow("info", 0, PS3_EYE_HEIGHT);
    */
    namedWindow("tracking");
    moveWindow("tracking", 0, PS3_EYE_HEIGHT);

    auto index = 0;
    while(true) {
        double timer = (double)getTickCount();
        mainCam >> frame;
        bool ok = tracker->update(frame, bbox);
        if (ok) {
          rectangle(frame, bbox, Scalar( 255, 0, 0 ), 2, 1 );
          flip(frame, frame, 1);
        } else {
          flip(frame, frame, 1);
          //putText(frame, "Tracking failure detected", Point(100,80), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,255),2);
        }
        imshow("tracking", frame);

        auto bbox_center_x = (bbox.x + bbox.width/2);
        auto newIndex = int((bbox_center_x / mainWidth) * eyeDevices.size());
        if (0 <= newIndex && newIndex < eyeDevices.size()) index = newIndex;

        toStart[index] = true;
        //if (index-1 >= 0) toStart[index-1] = true;
        //if (index+1 < eyeDevices.size()) toStart[index+1] = true;
        for (int i=0; i<eyeDevices.size(); ++i) {
            if (toStart[i] && !started[i]) {
              std::cout << "starting " <<  i << std::endl;
              eyes[i].start();
            }
            if (!toStart[i] && started[i]) {
              std::cout << "stopping " <<  i << std::endl;
              eyes[i].stop();
            }
            started[i] = toStart[i];
            toStart[i] = false;
            //if (started[i]) {
            //  eyes[i].update();
            //}
        }
        eyes[index].update();
        /*
        if (index % 2 == 1) {
           rotate(eyes[index].frame, eyes[index].frame, 1);
        }
        */
        imshow("final", eyes[index].frame);

        float seconds = ((double)getTickCount() - timer) / getTickFrequency();
        float fps = 1 / seconds;
        /*
        // Display FPS on frame
        Mat info = Mat::zeros(200, 200, CV_8UC3);
        putText(info, "FPS : " + SSTR(int(fps)), Point(50,50), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50,170,50), 2);
        imshow("info", info);
        */

        // sync to 60 FPS (it seems like if we request frames too fast from the
        // eye it slows down)
        //int sleep_ms = std::max(0, int(1000.0/60 - (seconds*1000)));
        //std::cout << bbox_center_x << " " << mainWidth << " " << eyeDevices.size() << std::endl;
        //std::cout << index << std::endl;
        //std::cout << fps << std::endl;
        //std::cout << sleep_ms << std::endl;
        //this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        // Exit if ESC pressed.
        int k = waitKey(1);
        if(k == 27) {
            Rect2d bbox = selectROI(frame);
            destroyWindow("ROI selector");
            tracker->clear();
            tracker = TrackerKCF::create();
            tracker->init(frame, bbox);
        }
    }
}
