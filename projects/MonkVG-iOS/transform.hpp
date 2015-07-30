#ifndef MBGL_MAP_TRANSFORM
#define MBGL_MAP_TRANSFORM

#include "transform_state.hpp"
#include "chrono.hpp"
#include "update.hpp"
#include "geo.hpp"
#include "noncopyable.hpp"

#include <cstdint>
#include <cmath>
#include <functional>

namespace MonkVG {

class View;

class Transform : private util::noncopyable {
public:
    Transform(const View&);
    // Map view
    bool resize(std::array<uint16_t, 2> size);

    // Position
    void moveBy(double dx, double dy, const Duration& = Duration::zero());
    void setLatLng(LatLng latLng, const Duration& = Duration::zero());
    void setLatLngZoom(LatLng latLng, double zoom, const Duration& = Duration::zero());
    inline const LatLng getLatLng() const { return state.getLatLng(); }
    const double getX() ;
    const double getY() ;
    // Zoom
    void scaleBy(double ds, double cx = -1, double cy = -1, const Duration& = Duration::zero());
    void setScale(double scale, double cx = -1, double cy = -1, const Duration& = Duration::zero());
    void setZoom(double zoom, const Duration& = Duration::zero());
    double getZoom() const;
    double getScale() const;

    // Angle
    void rotateBy(double sx, double sy, double ex, double ey, const Duration& = Duration::zero());
    void setAngle(double angle, const Duration& = Duration::zero());
    void setAngle(double angle, double cx, double cy);
    double getAngle() const;

    void setBearing(double degrees, const Duration& duration) ;
    void setBearing(double degrees, double cx, double cy);
    double getBearing() const;
    // Transitions
    bool needsTransition() const;
    UpdateType updateTransitions(const TimePoint& now);
    void cancelTransitions();

    // Gesture
    void setGestureInProgress(bool);

    // Transform state
    const TransformState getState() const { return state; }
    
    
    
private:
    void _moveBy(double dx, double dy, const Duration& = Duration::zero());
    void _setScale(double scale, double cx, double cy, const Duration& = Duration::zero());
    void _setScaleXY(double new_scale, double xn, double yn, const Duration& = Duration::zero());
    void _setAngle(double angle, const Duration& = Duration::zero());

    const View &view;

    TransformState state;

    void startTransition(std::function<Update(double)> frame,
                         std::function<void()> finish,
                         const Duration& duration);

    TimePoint transitionStart;
    Duration transitionDuration;
    std::function<Update(const TimePoint)> transitionFrameFn;
    std::function<void()> transitionFinishFn;
};

}

#endif
