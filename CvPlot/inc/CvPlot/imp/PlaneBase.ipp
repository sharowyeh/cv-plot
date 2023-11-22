// CvPlot - https://github.com/Profactor/cv-plot
// Copyright (c) 2019 by PROFACTOR GmbH - https://www.profactor.at/

#pragma once

#include <CvPlot/drawables/PlaneBase.h>
#include <CvPlot/Internal/util.h>
#include <opencv2/opencv.hpp>
#include <limits>

namespace CvPlot {

class PlaneBase::Impl {
public:
    LineType _lineType = LineType::None;
    cv::Scalar _color = cv::Scalar(255, 0, 0);
    int _lineWidth = 1;
};

CVPLOT_DEFINE_FUN
PlaneBase::~PlaneBase() {
}

CVPLOT_DEFINE_FUN
PlaneBase::PlaneBase(const std::string &lineSpec) {
    setLineSpec(lineSpec);
}

CVPLOT_DEFINE_FUN
void PlaneBase::render(RenderTarget & renderTarget){
}

CVPLOT_DEFINE_FUN
bool PlaneBase::getBoundingRect(cv::Rect2d &rect) {
    return false;
}

CVPLOT_DEFINE_FUN
PlaneBase& PlaneBase::setLineSpec(const std::string &lineSpec) {
    if (lineSpec.find_first_of('-') != std::string::npos) {
        setLineType(LineType::Solid);
    }else if (lineSpec.find_first_of('p') != std::string::npos) { //DEBUG: add type for 2d colormap
        setLineType(LineType::Plane);
    }else{
        setLineType(LineType::None);
    }
    const std::vector<std::pair<char,cv::Scalar>> colors {
        { 'b',cv::Scalar(255,0,0) },
        { 'g',cv::Scalar(0,255,0) },
        { 'r',cv::Scalar(0,0,255) },
        { 'c',cv::Scalar(255,255,0) },
        { 'y',cv::Scalar(0,255,255) },
        { 'm',cv::Scalar(255,0,255) },
        { 'k',cv::Scalar(0,0,0) },
        { 'w',cv::Scalar(255,255,255) }
    };
    for (const auto &color : colors) {
        if (lineSpec.find_first_of(color.first) != std::string::npos) {
            setColor(color.second);
        }
    }
    return *this;
}

CVPLOT_DEFINE_FUN
PlaneBase& PlaneBase::setLineType(LineType lineType){
    impl->_lineType = lineType;
    return *this;
}

CVPLOT_DEFINE_FUN
PlaneBase& PlaneBase::setColor(cv::Scalar color) {
    impl->_color = color;
    return *this;
}

CVPLOT_DEFINE_FUN
PlaneBase& PlaneBase::setLineWidth(int lineWidth){
    impl->_lineWidth = lineWidth;
    return *this;
}

CVPLOT_DEFINE_FUN
LineType PlaneBase::getLineType(){
    return impl->_lineType;
}

CVPLOT_DEFINE_FUN
cv::Scalar PlaneBase::getColor(){
    return impl->_color;
}

CVPLOT_DEFINE_FUN
int PlaneBase::getLineWidth(){
    return impl->_lineWidth;
}

}
