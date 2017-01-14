//---------------------------------Spheral++----------------------------------//
// GeomPlane -- Geometric Plane Class.
//
// Created by JMO, Thu Feb 24 17:26:32 PST 2000
//----------------------------------------------------------------------------//

#include "GeomPlane.hh"
#include "Utilities/DBC.hh"
#include "Utilities/SpheralFunctions.hh"
#include "Utilities/pointDistances.hh"
#include "Dimension.hh"

namespace Spheral {

//------------------------------------------------------------------------------
// Empty constructor.
//------------------------------------------------------------------------------
template<typename Dimension>
GeomPlane<Dimension>::GeomPlane():
  mPoint(),
  mNormal() {
}

//------------------------------------------------------------------------------
// Copy constructor.
//------------------------------------------------------------------------------
template<typename Dimension>
GeomPlane<Dimension>::GeomPlane(const GeomPlane<Dimension>& rhs):
  mPoint(rhs.point()),
  mNormal(rhs.normal()) {
}

//------------------------------------------------------------------------------
// Construct with the given point and normal.
//------------------------------------------------------------------------------
template<typename Dimension>
GeomPlane<Dimension>::GeomPlane(const typename Dimension::Vector& point,
				const typename Dimension::Vector& normal):
  mPoint(point),
  mNormal(normal.unitVector()) {
  CHECK(valid());
}

//------------------------------------------------------------------------------
// Destructor.
//------------------------------------------------------------------------------
template<typename Dimension>
GeomPlane<Dimension>::~GeomPlane() {
}

//------------------------------------------------------------------------------
// Assignment.
//------------------------------------------------------------------------------
template<typename Dimension>
GeomPlane<Dimension>&
GeomPlane<Dimension>::operator=(const GeomPlane<Dimension>& rhs) {
  if (this != &rhs) {
    mPoint = rhs.point();
    mNormal = rhs.normal();
  }
  return *this;
}

//------------------------------------------------------------------------------
// Access the point.
//------------------------------------------------------------------------------
template<typename Dimension>
const typename Dimension::Vector&
GeomPlane<Dimension>::point() const {
  return mPoint;
}

template<typename Dimension>
void
GeomPlane<Dimension>::point(const typename Dimension::Vector& point) {
  mPoint = point;
}

//------------------------------------------------------------------------------
// Access the normal.
//------------------------------------------------------------------------------
template<typename Dimension>
const typename Dimension::Vector&
GeomPlane<Dimension>::normal() const {
  return mNormal;
}

template<typename Dimension>
void
GeomPlane<Dimension>::normal(const typename Dimension::Vector& normal) {
  mNormal = normal.unitVector();
}

//------------------------------------------------------------------------------
// Negative operator (reverse the sign of the normal).
//------------------------------------------------------------------------------
template<typename Dimension>
GeomPlane<Dimension>
GeomPlane<Dimension>::operator-() const {
  GeomPlane<Dimension> result(*this);
  result.normal(-result.normal());
  return result;
}

//------------------------------------------------------------------------------
// Calculate the signed distance from a point to the plane.
//------------------------------------------------------------------------------
template<typename Dimension>
double
GeomPlane<Dimension>::
signedDistance(const typename Dimension::Vector& point) const {
  CHECK(valid());
  return mNormal.dot(point - mPoint);
}

//------------------------------------------------------------------------------
// Calculate the minimum distance from a point to the plane.
//------------------------------------------------------------------------------
template<typename Dimension>
double
GeomPlane<Dimension>::
minimumDistance(const typename Dimension::Vector& point) const {
  CHECK(valid());
  return std::abs(mNormal.dot(point - mPoint));
}

//------------------------------------------------------------------------------
// Find the closest point on the plane to a given point.
//------------------------------------------------------------------------------
template<>
Dim<1>::Vector
GeomPlane<Dim<1> >::
closestPointOnPlane(const Dim<1>::Vector& point) const {
  return mPoint;
}

template<>
Dim<2>::Vector
GeomPlane<Dim<2> >::
closestPointOnPlane(const Dim<2>::Vector& point) const {
  const Vector a0p = point - mPoint;
  const Vector direction(-mNormal.y(), mNormal.x());
  const double s = a0p.dot(direction);
  return mPoint + s*direction;
}

template<>
Dim<3>::Vector
GeomPlane<Dim<3> >::
closestPointOnPlane(const Dim<3>::Vector& point) const {
  return Spheral::closestPointOnPlane(point, mPoint, mNormal);
}

//------------------------------------------------------------------------------
// Test whether the given plane is parallel to this one.
//------------------------------------------------------------------------------
template<typename Dimension>
bool
GeomPlane<Dimension>::parallel(const GeomPlane<Dimension>& rhs) const {
  CHECK(valid());
  CHECK(rhs.valid());
  return fuzzyEqual(fabs(mNormal.dot(rhs.normal())), 1.0);
}

//------------------------------------------------------------------------------
// operator ==
//------------------------------------------------------------------------------
template<typename Dimension>
bool
GeomPlane<Dimension>::operator==(const GeomPlane<Dimension>& rhs) const {
  return (fuzzyEqual(mNormal.dot(rhs.normal()), 1.0) &&
	  fuzzyEqual(minimumDistance(rhs.point()), 0.0));
}

//------------------------------------------------------------------------------
// operator !=
//------------------------------------------------------------------------------
template<typename Dimension>
bool
GeomPlane<Dimension>::operator!=(const GeomPlane<Dimension>& rhs) const {
  return !(*this == rhs);
}

//------------------------------------------------------------------------------
// operator <
//------------------------------------------------------------------------------
template<typename Dimension>
bool
GeomPlane<Dimension>::operator<(const GeomPlane<Dimension>& rhs) const {
  return (mPoint < rhs.mPoint   ? true :
          mNormal < rhs.mNormal ? true:
                                  false);
}

//------------------------------------------------------------------------------
// Test if the plane is below, equal to or above the point (-1, 0, 1).
//------------------------------------------------------------------------------
template<typename Dimension>
int
GeomPlane<Dimension>::
compare(const typename Dimension::Vector& rhs) const {
  const double thpt = mNormal.dot(rhs - mPoint);
  return (fuzzyEqual(thpt, 0.0) ?  0 :
          thpt > 0.0            ? -1 :
                                   1);
}


//------------------------------------------------------------------------------
// operator == (Point)
//------------------------------------------------------------------------------
template<typename Dimension>
bool
GeomPlane<Dimension>::operator==(const typename Dimension::Vector& rhs) const {
  return compare(rhs) == 0;
}

//------------------------------------------------------------------------------
// operator != (Point)
//------------------------------------------------------------------------------
template<typename Dimension>
bool
GeomPlane<Dimension>::operator!=(const typename Dimension::Vector& rhs) const {
  return compare(rhs) != 0;
}

//------------------------------------------------------------------------------
// operator > (Point)
//------------------------------------------------------------------------------
template<typename Dimension>
bool
GeomPlane<Dimension>::operator>(const typename Dimension::Vector& rhs) const {
  return compare(rhs) == 1;
}

//------------------------------------------------------------------------------
// operator < (Point)
//------------------------------------------------------------------------------
template<typename Dimension>
bool
GeomPlane<Dimension>::operator<(const typename Dimension::Vector& rhs) const {
  return compare(rhs) == -1;
}

//------------------------------------------------------------------------------
// operator >= (Point)
//------------------------------------------------------------------------------
template<typename Dimension>
bool
GeomPlane<Dimension>::operator>=(const typename Dimension::Vector& rhs) const {
  return compare(rhs) != -1;
}

//------------------------------------------------------------------------------
// operator <= (Point)
//------------------------------------------------------------------------------
template<typename Dimension>
bool
GeomPlane<Dimension>::operator<=(const typename Dimension::Vector& rhs) const {
  return compare(rhs) != 1;
}

//------------------------------------------------------------------------------
// Valid test.
//------------------------------------------------------------------------------
template<typename Dimension>
bool
GeomPlane<Dimension>::valid() const {
  return fuzzyEqual(mNormal.magnitude2(), 1.0);
}

//------------------------------------------------------------------------------
// Explicit instantiation.
//------------------------------------------------------------------------------
template class GeomPlane< Dim<1> >;
template class GeomPlane< Dim<2> >;
template class GeomPlane< Dim<3> >;

}
