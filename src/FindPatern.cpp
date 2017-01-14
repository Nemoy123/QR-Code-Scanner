#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "FindPatern.hpp"


bool compareContourAreas (vector<Point> contour1, vector<Point> contour2) {
    double i = abs(contourArea(Mat(contour1)));
    double j = abs(contourArea(Mat(contour2)));

    return ( i >= j && contour1[0].x < contour2[0].x );
}

bool compareContoureVectors(vector<vector<Point>> contours1, vector<vector<Point>> contours2){
    double i = 0;
    double j = 0;

    for (int k = 0; k < contours1.size(); ++k) {
        i += abs(contourArea(Mat(contours1[k])));
    }

    for (int k = 0; k < contours1.size(); ++k) {
        j += abs(contourArea(Mat(contours2[k])));
    }

    return ( i >= j);
}


FindPatern::FindPatern(Mat originalImage) {
    this->originalImage = originalImage;
}

Mat FindPatern::findAllContours(Mat image) {

    float minPix = 8.00;
    float maxPix = 0.2 * image.cols * image.rows;

    image /= 255;
    cv::Mat contourOutput = image.clone();
    vector<Vec4i> hierarchy;
    findContours(image, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
    cout<<"contours.size = "<<contours.size()<<endl;
    int m = 0;
    while(m < contours.size()){
        if(contourArea(contours[m]) <= minPix){
            contours.erase(contours.begin() + m);
        }else if(contourArea(contours[m]) > maxPix){
            contours.erase(contours.begin() + m);
        }else ++ m;
    }
    cout<<"contours.size = "<<contours.size()<<endl;


    cv::Mat contourImage = originalImage.clone();
    cv::Scalar color[3];
    color[0] = cv::Scalar(0, 0, 255);
    color[1] = cv::Scalar(0, 255, 0);
    color[2] = cv::Scalar(255, 0, 0);

    for (size_t idx = 0; idx < contours.size(); idx++) {
        cv::drawContours(contourImage, contours, idx, color[idx % 3]);
    }
    return contourImage;
}

Mat FindPatern::findQRCodePaterns(Mat image) {

    sort(contours.begin(), contours.end(), compareContourAreas);

    for (int i = 0; i < contours.size(); ++i) {
        //iteration über die Kontur Punkte
        for (int j = 0; j < contours.size(); ++j) {
            if (isContourInsideContour(contours.at(i), contours.at(j))) {
                for (int k = 0; k <contours.size(); ++k) {
                    if (isContourInsideContour(contours.at(k), contours.at(i))){
                        if(isTrapez(contours.at(j))){
                            vector<vector<Point>> pairs;
                            pairs.push_back((vector<Point> &&) contours.at(j));
                            pairs.push_back((vector<Point> &&) contours.at(i));
                            pairs.push_back((vector<Point> &&) contours.at(k));
                            trueContoures.push_back(pairs);
                        }
                    }
                }
            }
        }
    }

    sort(trueContoures.begin(), trueContoures.end(), compareContoureVectors);

    cv::Mat contourImage = originalImage.clone();

    cv::Scalar color[3];
    color[0] = cv::Scalar(0, 0, 255);
    color[1] = cv::Scalar(0, 255, 0);
    color[2] = cv::Scalar(255, 0, 0);
    for (size_t idx = 0; idx < trueContoures.size(); idx++) {
        for (size_t idx2 = 0; idx2 < trueContoures.at(idx).size(); idx2++) {
            cv::drawContours(contourImage, trueContoures[idx], idx2, color[idx % 3]);
        }
    }
    return contourImage;

}

bool FindPatern::isContourInsideContour(std::vector<cv::Point> in, std::vector<cv::Point> out) {
    if(in.size() > 0 && out.size() > 0){
        for (int i = 0; i < in.size(); i++) {
            if (pointPolygonTest(out, in[i], false) <= 0) return false;
        }
        return true;
    }

    return false;
}

bool FindPatern::isTrapez(std::vector<cv::Point> in){

    std::vector<cv::Point> approximatedPolygon;
    double epsilon = 0.1*cv::arcLength(in,true);
    approxPolyDP(in, approximatedPolygon, epsilon, true);
    bool ret = ( approximatedPolygon.size() == 4 );
    return ret;
}

Point FindPatern::calculateMassCentres(std::vector<cv::Point> in){

    int xCoordinates[] = {std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};
    int yCoordinates[] = {std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};

    //extremalwerte werden bestimmt
    for(int i=0;i<in.size();i++){
        if(xCoordinates[0] > in.at(i).x){
            xCoordinates[0] = in.at(i).x;
        }
        if(xCoordinates[1] < in.at(i).x){
            xCoordinates[1] = in.at(i).x;
        }

        if(yCoordinates[0] > in.at(i).y){
            yCoordinates[0] = in.at(i).y;
        }
        if(yCoordinates[1] < in.at(i).y){
            yCoordinates[1] = in.at(i).y;
        }
    }

    int _x = (xCoordinates[0] + xCoordinates[1]) / 2;
    int _y = (yCoordinates[0] + yCoordinates[1]) / 2;

    Point point;
    point.x = _x;
    point.y = _y;

    return point;
}

Mat FindPatern::tiltCorrection(Mat image, FinderPaternModel fPatern){
    vector<Point2f> vecsrc;
    vector<Point2f> vecdst;

    vecdst.push_back(Point2f(20, 20));
    vecdst.push_back(Point2f(120, 20));
    vecdst.push_back(Point2f(120,120));
    vecdst.push_back(Point2f(20, 120));


    vecsrc.push_back(fPatern.topleft);
    vecsrc.push_back(fPatern.topright);
    vecsrc.push_back(Point2f(fPatern.topright.x,fPatern.bottomleft.y));
    vecsrc.push_back(fPatern.bottomleft);


    Mat affineTrans = getPerspectiveTransform(vecsrc, vecdst);
    Mat warped = Mat(image.size(),image.type());
    warpPerspective(image, warped, affineTrans, image.size());
    Mat qrcode_color = warped(Rect(0, 0, 145, 145));
    Mat qrcode_gray;
    cvtColor (qrcode_color,qrcode_gray,CV_BGR2GRAY);
    Mat qrcode_bin;
    threshold(qrcode_gray, qrcode_bin, 120, 255, CV_THRESH_OTSU);

    return qrcode_bin;
}


FinderPaternModel FindPatern::getFinderPaternModel(vector<Point> cont1, vector<Point> cont2, vector<Point> cont3){


    Point pt1 = calculateMassCentres(cont1);
    Point pt2 = calculateMassCentres(cont2);
    Point pt3 = calculateMassCentres(cont3);

    double d12 = abs(pt1.x - pt2.x) + abs(pt1.y - pt2.y);
    double d13 = abs(pt1.x - pt3.x) + abs(pt1.y - pt3.y);
    double d23 = abs(pt2.x - pt3.x) + abs(pt2.y - pt3.y);

    double x1, y1, x2, y2, x3, y3;
    double Max = max(d12, max(d13, d23));
    Point p1, p2, p3;
    if(Max == d12){
        p1 = pt1;
        p2 = pt2;
        p3 = pt3;
    }else if(Max == d13){
        p1 = pt1;
        p2 = pt3;
        p3 = pt2;
    }else if(Max == d23){
        p1 = pt2;
        p2 = pt3;
        p3 = pt1;
    }
    x1 = p1.x;
    y1 = p1.y;
    x2 = p2.x;
    y2 = p2.y;
    x3 = p3.x;
    y3 = p3.y;
    if(x1 == x2){
        if(y1 > y2){
            if(x3 < x1){
                FinderPaternModel paternModel(p3, p2, p1);
                return paternModel;
            }else{
                FinderPaternModel paternModel(p3, p1, p2);
                return paternModel;
            }
        }else{
            if(x3 < x1){
                FinderPaternModel paternModel(p3, p1, p2);
                return paternModel;
            }else{
                FinderPaternModel paternModel(p3, p2, p1);
                return paternModel;
            }
        }
    }else{
        double newy = (y2 - y1) / (x2 - x1) * x3 + y1 - (y2 - y1) / (x2 - x1) * x1;
        if(x1 > x2){
            if(newy < y3){
                FinderPaternModel paternModel(p3, p2, p1);
                return paternModel;;
            }else{
                FinderPaternModel paternModel(p3, p1, p2);
                return paternModel;
            }
        }else{
            if(newy < y3){
                FinderPaternModel paternModel(p3, p1, p2);
                return paternModel;
            }else{
                FinderPaternModel paternModel(p3, p2, p1);
                return paternModel;
            }
        }
    }
}

vector<FinderPaternModel> FindPatern::getAllPaterns(){

	std::cout << "trueContoures Size: " << trueContoures.size() << endl;

	//Philipp: Diese Funktion hat Fehlermeldung wenn sie mit trueContoures.size()=2 durchlaufen wird! Und das passiert bei einigen Bildern.
	//Vorzeitige Lösung:
	if (trueContoures.size() < 3) {
		return paterns;
	}
	//(Allerdings keinen Plan)

    for (int i = 0; i < trueContoures.size(); i++) {
        paterns.push_back(getFinderPaternModel(trueContoures[i][trueContoures[i].size() - 1], trueContoures[i++][trueContoures[i].size() - 1], trueContoures[i++][trueContoures[i].size() - 1]));
    }

    return paterns;
}


