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
//#define SSTR( x ) static_cast< std::ostringstream & >(
#define SSTR( x ) ( \
( std::ostringstream() << std::dec << x ) ).str()

struct TrackedEyeFrame {
    std::string name;
    ps3eye::PS3EYECam::PS3EYERef eye;
    uint8_t buf[PS3_EYE_HEIGHT * PS3_EYE_WIDTH * PS3_EYE_CHANNELS];
    Mat frame;
    Ptr<Tracker> tracker;
    Rect2d bbox;
    bool ok;

    TrackedEyeFrame(ps3eye::PS3EYECam::PS3EYERef eye_, string name_) :
        name(name_),
        eye(eye_),
        bbox(PS3_EYE_WIDTH/2, 0, 100, 100)
    {
        eye->init(PS3_EYE_WIDTH, PS3_EYE_HEIGHT, PS3_EYE_HZ);
        eye->start();
        eye->getFrame(buf);
        frame = Mat(PS3_EYE_HEIGHT, PS3_EYE_WIDTH, CV_8UC3, buf);
        tracker = TrackerKCF::create();
        // bbox = selectROI(frame);
        destroyWindow("ROI selector");
        tracker->init(frame, bbox);
    }

    void update() {
        eye->getFrame(buf);
        frame = Mat(PS3_EYE_HEIGHT, PS3_EYE_WIDTH, CV_8UC3, buf);
        // ok = tracker->update(frame, bbox);
    }

    void display(Mat displayFrame) {
        rectangle(displayFrame, bbox, Scalar( 255, 0, 0 ), 2, 1 );
    }
};

int main(int argc, char **argv)
{
    auto eyeDevices = ps3eye::PS3EYECam::getDevices();
    if (eyeDevices.size() <= 0) {
        std::cerr << "couldn't find ps3 eye cameras" << std::endl;
        return 1;
    }

    std::vector<TrackedEyeFrame> trackedFrames;
    int i = 0;
    for (auto& eye : eyeDevices) {
      string name = std::to_string(i);
      trackedFrames.emplace_back(eye, name);
      namedWindow(name);
      moveWindow(name, PS3_EYE_WIDTH*i, 0);
      ++i;
    }

    int parity = 0;
    namedWindow("info");
    moveWindow("info", 0, PS3_EYE_HEIGHT);
    while(true) {
        double timer = (double)getTickCount();
        for (auto& trackedFrame : trackedFrames) {
            trackedFrame.update();
            /*
            auto displayFrame = trackedFrame.frame.clone();
            if (trackedFrame.ok) {
                rectangle(displayFrame, trackedFrame.bbox, Scalar( 255, 0, 0 ), 2, 1 );
                flip(displayFrame, displayFrame, 1);
            } else {
                flip(displayFrame, displayFrame, 1);
                putText(displayFrame, "Tracking failure detected", Point(100,80), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,255),2);
            }
            // Display tracker type on frame
            // putText(displayFrame, TRACKER_TYPE, Point(100,20), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50,170,50),2);
            // Display frame
            imshow(trackedFrame.name, displayFrame);
            */
        }

        Mat frame;
        /*
        int minDistance = 2*PS3_EYE_WIDTH;
        for (auto& trackedFrame : trackedFrames) {
            auto center_x = trackedFrame.bbox.x + trackedFrame.bbox.width/2;
            auto distance = abs(center_x - PS3_EYE_WIDTH/2);
            if (distance < minDistance) {
              minDistance = distance;
              frame = trackedFrame.frame;
            }
        }
        */
        parity = (parity+1) % 8;
        if ((parity/2)%2 == 0) {
          frame = trackedFrames[0].frame;
        } else {
          frame = trackedFrames[1].frame;
        }
        flip(frame, frame, 1);
        imshow("final", frame);

        Mat info = Mat::zeros(200, 200, CV_8UC3);
        float seconds = ((double)getTickCount() - timer) / getTickFrequency();
        float fps = 1 / seconds;
        // Display FPS on frame
        putText(info, "FPS : " + SSTR(int(fps)), Point(50,50), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50,170,50), 2);
        imshow("info", info);

        // sync to 30 FPS (it seems like if we request frames too fast from the
        // eye it slows down)
        int sleep_ms = std::max(0, int(1000.0/30 - (seconds*1000)));
        this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        // Exit if ESC pressed.
        int k = waitKey(1);
        if(k == 27) {
            break;
        }
    }
}
