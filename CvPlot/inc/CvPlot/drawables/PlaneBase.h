// CvPlot - https://github.com/Profactor/cv-plot
// Copyright (c) 2019 by PROFACTOR GmbH - https://www.profactor.at/

#pragma once

#include <CvPlot/libdef.h>
#include <opencv2/core.hpp>
#include <CvPlot/Internal/Pimpl.h>
#include <CvPlot/Internal/no_warning.h>
#include <CvPlot/core/Drawable.h>
#include <CvPlot/drawables/MarkerType.h>
#include <CvPlot/drawables/LineType.h>

namespace CvPlot {

class CVPLOT_LIBRARY_INTERFACE PlaneBase : public DrawableSub<PlaneBase>{
public:
    PlaneBase(const std::string &lineSpec = "p");
    ~PlaneBase();
    void render(RenderTarget &renderTarget)override;
    bool getBoundingRect(cv::Rect2d &rect)override;
    PlaneBase& setLineSpec(const std::string &lineSpec);
    PlaneBase& setLineType(LineType lineType);
    PlaneBase& setColor(cv::Scalar color);
    PlaneBase& setLineWidth(int lineWidth);
    PlaneBase getLineType();
    cv::Scalar getColor();
    int getLineWidth();
private:
    class Impl;
    CVPLOT_NO_WARNING_DLL_INTERFACE(Internal::Pimpl<Impl>, impl);
};

template<typename Derived>
class PlaneBaseSub :public PlaneBase {
public:
    using PlaneBase::PlaneBase;
    Derived& setName(std::string name) {
        return static_cast<Derived&>(Drawable::setName(name));
    }
    Derived& setLineSpec(const std::string &lineSpec) {
        return static_cast<Derived&>(PlaneBase::setLineSpec(lineSpec));
    }
    Derived& setLineType(LineType lineType) {
        return static_cast<Derived&>(PlaneBase::setLineType(lineType));
    }
    Derived& setColor(cv::Scalar color) {
        return static_cast<Derived&>(PlaneBase::setColor(color));
    }
    Derived& setLineWidth(int lineWidth) {
        return static_cast<Derived&>(PlaneBase::setLineWidth(lineWidth));
    }
};

}

#ifdef CVPLOT_HEADER_ONLY
#include <CvPlot/imp/LineBase.ipp>
#endif