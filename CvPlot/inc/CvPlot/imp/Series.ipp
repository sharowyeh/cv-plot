// CvPlot - https://github.com/Profactor/cv-plot
// Copyright (c) 2019 by PROFACTOR GmbH - https://www.profactor.at/

#pragma once

#include <CvPlot/drawables/Series.h>
#include <CvPlot/Internal/util.h>
#include <opencv2/opencv.hpp>
#include <limits>

namespace CvPlot {

class Series::Impl {
public:
    Impl(Series &parent)
        :_parent(parent) {
    }
    Series &_parent;
    std::vector<double> _x,_internalX,_y;
    MarkerType _markerType = MarkerType::None;
    int _markerSize = 10;
    cv::Rect2d _boundingRect;
    //EXPR: _rawData should be use for plane chart, maybe better design a new class for the 2d chart
    std::vector<cv::Mat> _rawData;
    //EXPR: planeValid to check plane chart data, mat rows and cols larger than 1 and line type is plane
    bool planeValid(cv::InputArray a, const std::string lineSpec) {
        cv::Mat m = a.getMat();
        return lineSpec.find('p') != std::string::npos &&
            (m.empty() || (m.channels() == 1 && m.rows >= 1 && m.cols >= 1));
    }

    bool xyValid(cv::InputArray a) {
        cv::Mat m = a.getMat();
        return m.empty() || (m.channels() == 1 && (m.rows == 1 || m.cols == 1));
    }
    bool pointsValid(cv::InputArray a) {
        cv::Mat p = a.getMat();
        return p.empty() 
            || (p.channels() == 2 && (p.cols == 1 || p.rows == 1))
            || (p.channels() == 1 && (p.rows == 2));
    }
    void update() {
        if (_y.empty()) {
            _internalX.clear();
            return;
        }
        if (_x.empty()) {
            _internalX = Internal::makeX(_y);
        } else {
            if (_x.size() != _y.size()) {
                _internalX.clear();
                return;
            }
            _internalX = _x;
        }
        double minx, maxx, miny, maxy;
        cv::minMaxIdx(_internalX, &minx, &maxx);
        cv::minMaxIdx(_y, &miny, &maxy);
        _boundingRect = cv::Rect2d(minx, miny, maxx - minx, maxy - miny);
    }
    std::vector<std::vector<cv::Point>> getShiftedPoints(const std::vector<cv::Point2d> &points, int shiftScale) {
        std::vector<std::vector<cv::Point>> groups;
        std::vector<cv::Point> *group = nullptr;
        for (const auto &p : points) {
            if (std::isfinite(p.x) && std::isfinite(p.y)) {
                if (!group) {
                    groups.push_back({});
                    group = &groups.back();
                    group->reserve(points.size());
                }
                group->emplace_back(Internal::drawCast(p.x * shiftScale), Internal::drawCast(p.y * shiftScale));
            } else {
                group = nullptr;
            }
        }
        return groups;
    }

    //EXPR: rendering function for plane data to color map
    //TODO: need scale available, and color bar ticks
    void renderPlaneColorMap(RenderTarget & renderTarget) {
        auto m = _rawData.back();
        cv::Mat3b& mat = renderTarget.innerMat();
        const int lineWidth = _parent.getLineWidth();
        auto height = mat.rows / m.rows;
        auto width = mat.cols / m.cols;
        printf("rows:%d, cols:%d, height:%d, width:%d\n", m.rows, m.cols, height, width);
        double maxval, minval;
        cv::minMaxIdx(m, &minval, &maxval);
        double factor = 255.0 / (maxval - minval);
        double added = -255.0 * minval / (maxval - minval);
        printf("minval: %.2f, maxval: %.2f, factor: %.2f, added: %.2f\n", minval, maxval, factor, added);
        cv::Mat colorMap = cv::Mat(m.rows, m.cols, CV_8UC1);
        m.convertTo(colorMap, CV_8UC1, factor, added);
        // apply ROIs to display mat
        for (int r = 0; r < colorMap.rows; r++) {
            for (int c = 0; c < colorMap.cols; c++) {
                auto roi = mat(cv::Rect(c * width, r * height, width, height));
                auto val = colorMap.at<uchar>(r, c);
                roi.setTo(cv::Scalar(val, val, val));
            }
        }
        cv::applyColorMap(mat, mat, cv::COLORMAP_PLASMA);

        auto maxMap = maxval * factor + added;
        auto minMap = minval * factor + added;
        cv::Mat colorBar = cv::Mat(mat.rows, 20, CV_8UC3);
        for (int r = 0; r < mat.rows; r++) {
            auto val = (maxMap - minMap) / mat.rows * r;
            // high to low
            colorBar(cv::Rect(0, mat.rows - r - 1, 20, 1)).setTo(cv::Scalar(val, val, val));
        }
        // render color bar likes YAxis.ipp
        cv::Mat3b& outerMat = renderTarget.outerMat();
        const cv::Rect& innerRect = renderTarget.innerRect();
        //TODO: default left margin has 80, so default place on left side?
        if (innerRect.x > 40) {
            cv::Rect barRect((innerRect.x - 20) / 2, innerRect.y, 20, mat.rows);
            // apply color map and copy to outer mat
            cv::applyColorMap(colorBar, outerMat(barRect), cv::COLORMAP_PLASMA);
        }
        else {
            printf("left margin is not enough to draw color bar\n");
        }
        //TODO: need ticks

        // draw grid of the ROIs
        for (int i = 1; i < cv::max(colorMap.rows, colorMap.cols); i++) {
            // dash line like
            if (i < colorMap.rows) {
                for (int j = 4; j < mat.cols - 4; j += 8) {
                    cv::line(mat, cv::Point(j, i * height), cv::Point(j + 4, i * height), cv::Scalar::all(200), lineWidth);
                }
                //cv::line(mat, cv::Point(0, i * height), cv::Point(mat.cols, i * height), cv::Scalar::all(200), lineWidth);
            }
            if (i < colorMap.cols) {
                for (int j = 4; j < mat.rows - 4; j += 8) {
                    cv::line(mat, cv::Point(i * width, j), cv::Point(i * width, j + 4), cv::Scalar::all(200), lineWidth);
                }
                //cv::line(mat, cv::Point(i * width, 0), cv::Point(i * width, mat.rows), cv::Scalar::all(200), lineWidth);
            }
        }
    }

    void render(RenderTarget & renderTarget) {
        if (_internalX.empty()) {
            return;
        }
        const cv::Scalar color = _parent.getColor();
        const int lineWidth = _parent.getLineWidth();
        //DEBUG: allows for plane type
        /*if (_markerType == MarkerType::None && lineWidth == 0) {
            return;
        }*/
        cv::Mat3b& mat = renderTarget.innerMat();
        const int shift = 3;
        const int shiftScale = (1 << shift);
        std::vector<cv::Point2d> points;
        points.reserve(_internalX.size());
        for (size_t i = 0; i < _internalX.size(); i++) {
            points.push_back(renderTarget.project(cv::Point2d(_internalX[i], _y[i])));
        }
        std::vector<std::vector<cv::Point>> shiftedPoints;
        //EXPR: use _rawData instead of x,y axis
        if (_parent.getLineType() == LineType::Plane && _rawData.size() > 0) {
            renderPlaneColorMap(renderTarget);
            return;
        }
        if (_parent.getLineType() == LineType::Solid) {
            if (shiftedPoints.empty()) {
                shiftedPoints = getShiftedPoints(points, shiftScale);
            }
            cv::polylines(mat, shiftedPoints, false, color, lineWidth, cv::LINE_AA, shift);
        }
        if (_markerType == MarkerType::Circle) {
            if (shiftedPoints.empty()) {
                shiftedPoints = getShiftedPoints(points, shiftScale);
            }
            const int radius = (_markerSize * shiftScale) / 2;
            for (const auto &group: shiftedPoints ) {
                for (const auto &point : group) {
                    cv::circle(mat, point, radius, color, lineWidth, cv::LINE_AA, shift);
                }
            }
        }
        if (_markerType == MarkerType::Point) {
            cv::Vec3b colorv3((unsigned char)color.val[0], (unsigned char)color.val[1], (unsigned char)color.val[2]);
            for (size_t i = 0; i < points.size(); i++) {
                const auto p = (cv::Point)points[i];
                if (p.x >= 0 && p.x < mat.cols && p.y >= 0 && p.y < mat.rows) {
                    mat(p.y, p.x) = colorv3;
                }
            }
        }
        if (_markerType == MarkerType::Cross) {
            if (shiftedPoints.empty()) {
                shiftedPoints = getShiftedPoints(points, shiftScale);
            }
            const double hypotenuse = (_markerSize * shiftScale) / 2.f;
            const double side = std::sqrt(std::pow(hypotenuse, 2) / 2.f);
            for (const auto &group: shiftedPoints ) {
                for (const auto &point : group) {
                    cv::line(mat, 
                        cv::Point(point.x - side, point.y - side), 
                        cv::Point(point.x + side, point.y + side), 
                        color, lineWidth, cv::LINE_AA, shift);
                    cv::line(mat, 
                        cv::Point(point.x + side, point.y - side), 
                        cv::Point(point.x - side, point.y + side), 
                        color, lineWidth, cv::LINE_AA, shift);
                }
            }
        }
        if (_markerType == MarkerType::Plus) {
            if (shiftedPoints.empty()) {
                shiftedPoints = getShiftedPoints(points, shiftScale);
            }
            const double side = (_markerSize * shiftScale) / 2.f;
            for (const auto &group: shiftedPoints ) {
                for (const auto &point : group) {
                    cv::line(mat, 
                        cv::Point(point.x - side, point.y), 
                        cv::Point(point.x + side, point.y), 
                        color, lineWidth, cv::LINE_AA, shift);
                    cv::line(mat, 
                        cv::Point(point.x, point.y - side), 
                        cv::Point(point.x, point.y + side), 
                        color, lineWidth, cv::LINE_AA, shift);
                }
            }
        }
    }
    std::vector<cv::Point2d> getPoints() {
        std::vector<cv::Point2d> points;
        points.reserve(_internalX.size());
        for (size_t i = 0; i < _internalX.size(); i++) {
            points.emplace_back(_internalX[i], _y[i]);
        }
        return points;
    }
};

CVPLOT_DEFINE_FUN
Series::~Series() {
}

CVPLOT_DEFINE_FUN
Series::Series(const std::string & lineSpec, const std::string name)
    : Series(cv::Mat(), lineSpec, name) {
}

//DEBUG: i want named drawable item 
CVPLOT_DEFINE_FUN
Series::Series(cv::InputArray data, const std::string& lineSpec, const std::string name)
    : LineBaseSub(lineSpec)
    , impl(*this) {
    LineBaseSub::setName(name);
    cv::Mat d = data.getMat();
    impl->_rawData.push_back(d);
    if (impl->xyValid(data)) {
        setY(data);
    }
    else if (impl->pointsValid(data)) {
        setPoints(data);
    }
    //EXPR: try to identify given plane mat data and corresponding type
    else if (impl->planeValid(data, lineSpec)) {
        impl->_rawData.push_back(d);
        setPlaneMat(d);
    }else {
        throw std::invalid_argument("invalid data in Series constructor. See Series.h for supported types");
    }
    setLineSpec(lineSpec);
}

CVPLOT_DEFINE_FUN
Series::Series(cv::InputArray x, cv::InputArray y, const std::string &lineSpec, const std::string name)
    :LineBaseSub(lineSpec)
    ,impl(*this){
    LineBaseSub::setName(name);
    impl->_rawData.push_back(x.getMat());
    impl->_rawData.push_back(y.getMat());
    if (!impl->xyValid(x)) {
        throw std::invalid_argument("invalid x in Series constructor. See Series.h for supported types");
    }
    if (!impl->xyValid(y)) {
        throw std::invalid_argument("invalid y in Series constructor. See Series.h for supported types");
    }
    setX(x);
    setY(y);
    setLineSpec(lineSpec);
}

CVPLOT_DEFINE_FUN
void Series::setName(std::string name) {
    // will go to drawablesub<linebase> -> drawable::setname
    LineBaseSub::setName(name);
}

CVPLOT_DEFINE_FUN
std::string Series::getName() {
    return LineBaseSub::getName();
}

CVPLOT_DEFINE_FUN
void Series::render(RenderTarget & renderTarget){
    impl->render(renderTarget);
}

CVPLOT_DEFINE_FUN
Series& Series::setLineSpec(const std::string &lineSpec) {
    LineBase::setLineSpec(lineSpec);
    if (lineSpec.find_first_of('o') != std::string::npos) {
        setMarkerType(MarkerType::Circle);
    }else if (lineSpec.find_first_of('.') != std::string::npos) {
        setMarkerType(MarkerType::Point);
    }else if (lineSpec.find_first_of('x') != std::string::npos) {
        setMarkerType(MarkerType::Cross);
    }else if (lineSpec.find_first_of('+') != std::string::npos) {
        setMarkerType(MarkerType::Plus);
    }else{
        setMarkerType(MarkerType::None);
    }
    return *this;
}

CVPLOT_DEFINE_FUN
Series& Series::setMarkerType(MarkerType markerType){
    impl->_markerType = markerType;
    return *this;
}

CVPLOT_DEFINE_FUN
MarkerType Series::getMarkerType(){
    return impl->_markerType;
}

CVPLOT_DEFINE_FUN
Series& Series::setMarkerSize(int markerSize) {
    impl->_markerSize = markerSize;
    return *this;
}

CVPLOT_DEFINE_FUN
int Series::getMarkerSize() {
    return impl->_markerSize;
}

CVPLOT_DEFINE_FUN
bool Series::getBoundingRect(cv::Rect2d &rect) {
    if (impl->_internalX.empty()) {
        return false;
    }
    rect = impl->_boundingRect;
    return true;
}

CVPLOT_DEFINE_FUN
Series & Series::setX(cv::InputArray x) {
    if (!impl->xyValid(x)) {
        throw std::invalid_argument("invalid x in Series::setX(). See Series.h for supported types");
    }
    //TODO: conflict to exists in _rawData from ctor
    //impl->_rawData.push_back(x.getMat());
    impl->_x = Internal::toVector<double>(x);
    impl->update();
    return *this;
}

CVPLOT_DEFINE_FUN
std::vector<double> Series::getX() {
    return impl->_x;
}

CVPLOT_DEFINE_FUN
Series & Series::setY(cv::InputArray y) {
    if (!impl->xyValid(y)) {
        throw std::invalid_argument("invalid y in Series::setY(). See Series.h for supported types");
    }
    //TODO: conflict to exists in _rawData from ctor
    //impl->_rawData.push_back(y.getMat());
    impl->_y = Internal::toVector<double>(y);
    impl->update();
    return *this;
}

CVPLOT_DEFINE_FUN
std::vector<double> Series::getY() {
    return impl->_y;
}

CVPLOT_DEFINE_FUN
Series & Series::setPoints(cv::InputArray points){
    if (!impl->pointsValid(points)) {
        throw std::invalid_argument("invalid points in Series::setPoints(). See Series.h for supported types");
    }
    cv::Mat p = points.getMat();
    //TODO: conflict to exists in _rawData from ctor
    //impl->_rawData.push_back(p);
    if (p.empty()) {
        impl->_x.clear();
        impl->_y.clear();
    }else if (p.channels() == 2) {
        cv::Mat channels[2];
        cv::split(points.getMat(), channels);
        impl->_x = Internal::toVector<double>(channels[0]);
        impl->_y = Internal::toVector<double>(channels[1]);
    } else {
        impl->_x = Internal::toVector<double>(p.row(0));
        impl->_y = Internal::toVector<double>(p.row(1));
    }
    impl->update();
    return *this;
}

CVPLOT_DEFINE_FUN
Series& Series::setPlaneMat(cv::InputArray data) {
    cv::Mat m = data.getMat();
    std::vector<double> x, y;
    //EXPR: same counting for _internalX
    int c = m.cols;
    int r = m.rows;
    // find lcf and gcd of c and r
    int gcd = 0;
    while (r != 0) {
        gcd = r;
        r = c % r;
        c = gcd;
    }
    int lcf = m.cols * m.rows / gcd;
    for (int i = 1; i <= lcf; i++) {
        x.push_back((double)i / lcf * m.cols);
        y.push_back((double)i / lcf * m.rows);
    }
    setX(x);
    setY(y);
    return *this;
}

CVPLOT_DEFINE_FUN
std::vector<cv::Point2d> Series::getPoints(){
    return impl->getPoints();
}

}
