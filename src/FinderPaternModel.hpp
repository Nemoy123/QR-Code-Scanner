#ifndef QRCODE_PATERNORIENTATION_H
#define QRCODE_PATERNORIENTATION_H
#include "FinderPaternModel.hpp"
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class FinderPaternModel {

public:
    FinderPaternModel(Point a, Point b, Point c);

    Point topleft;
    Point topright;
    Point bottomleft;
};


#endif //QRCODE_PATERNORIENTATION_H
