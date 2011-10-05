//---------------------------------Spheral++----------------------------------//
// GeomPolygon -- Geometric polygon class.
//
// A 2-D structure representing a polygon as a collection of GeomFacets.
//
// Created by JMO, Thu Jan 28 11:03:27 PST 2010
//----------------------------------------------------------------------------//
#include <algorithm>
#include <numeric>
#include <map>
#include <limits>

#include "GeomPolygon.hh"
#include "Utilities/removeElements.hh"
#include "Utilities/testBoxIntersection.hh"
#include "Utilities/spheralWildMagicConverters.hh"
#include "Utilities/lineSegmentIntersections.hh"
#include "Utilities/CounterClockwiseComparator.hh"
#include "Utilities/pointInPolygon.hh"

#include "qhull/qhull_a.h"

namespace Spheral {

using namespace std;

//------------------------------------------------------------------------------
// Default constructor.
//------------------------------------------------------------------------------
GeomPolygon::
GeomPolygon():
  mVertices(),
  mFacets(),
  mXmin(),
  mXmax(),
  mConvex(true) {
  if (mDevnull == NULL) mDevnull = fopen("/dev/null", "w");
}

//------------------------------------------------------------------------------
// Construct as a convex hull for the given point set.
//------------------------------------------------------------------------------
GeomPolygon::
GeomPolygon(const vector<GeomPolygon::Vector>& points):
  mVertices(),
  mFacets(),
  mXmin(),
  mXmax(),
  mConvex(true) {
  if (mDevnull == NULL) mDevnull = fopen("/dev/null", "w");

  if (points.size() > 0) {

    REQUIRE(points.size() > 2);

    // Copy the point coordinates to a Qhull point array.
    std::vector<coordT> points_qhull;
    points_qhull.reserve(2 * points.size());
    for (vector<Vector>::const_iterator itr = points.begin();
         itr != points.end();
         ++itr) {
      points_qhull.push_back(itr->x());
      points_qhull.push_back(itr->y());
    }
    CHECK(points_qhull.size() == 2*points.size());

    // Call Qhull to generate the hull (C interface).
    boolT ismalloc = False;   /* True if qhull should free points in qh_freeqhull() or reallocation */
    char flags[250];          /* option flags for qhull, see qh_opt.htm */
//     FILE *outfile= NULL;      /* output from qh_produce_output()
//                                  use NULL to skip qh_produce_output() */
//     FILE *errfile= stderr;    /* error messages from qhull code */
//     FILE *outfile= fopen("/dev/null", "w");
//     FILE *outfile = fopen("qhull.barf", "w");
//     FILE *errfile= fopen("/dev/null", "w");    /* error messages from qhull code */
    facetT *facet;            /* set by FORALLfacets */
    int curlong, totlong;     /* memory remaining after qh_memfreeshort */
    sprintf(flags, "qhull s Tcv");
    const int exitcode_qhull = qh_new_qhull(2, points.size(), &points_qhull.front(), ismalloc, flags, mDevnull, mDevnull);
    VERIFY2(exitcode_qhull == 0,
            "Qhull emitted an error code while generating GeomPolygon");

    // Copy Qhull's vertex information.
    vertexT *vertex, **vertexp;
    FORALLvertices {
      mVertices.push_back(Vector(vertex->point[0], vertex->point[1]));
    }

    // Free Qhull's resources.
    qh_freeqhull(!qh_ALL);
    qh_memfreeshort(&curlong, &totlong);

    // Ensure the vertices are arranged counter-clockwise.
    CounterClockwiseComparator<Vector, vector<Vector> > nodeComparator(mVertices, mVertices[0]);
    sort(mVertices.begin() + 1, mVertices.end(), nodeComparator);

    // Generate the facets.
    for (unsigned i = 0; i != mVertices.size(); ++i) {
      const unsigned j = (i + 1) % mVertices.size();
      mFacets.push_back(Facet(mVertices, i, j));
    }

    // Fill in our bounding box.
    setBoundingBox();

    // Post-conditions.
    BEGIN_CONTRACT_SCOPE;
    {
      // Ensure the facet node ordering is correct.
      for (vector<Facet>::const_iterator itr = mFacets.begin();
           itr != mFacets.end();
           ++itr) ENSURE(nodeComparator(itr->point1(), itr->point2()) >= 0);

      // All normals should be outward facing.
      Vector centroid;
      for (vector<Vector>::const_iterator itr = mVertices.begin();
           itr != mVertices.end();
           ++itr) centroid += *itr;
      centroid /= mVertices.size();
      for (vector<Facet>::const_iterator itr = mFacets.begin();
           itr != mFacets.end();
           ++itr) ENSURE((0.5*(itr->point1() + itr->point2()) - centroid).dot(itr->normal()) >= 0.0);

      // Ensure the vertices are listed in counter-clockwise order.
      for (unsigned i = 0; i != mVertices.size(); ++i) {
        const unsigned j = (i + 1) % mVertices.size();
        ENSURE(nodeComparator(i, j) >= 0);
      }

      // We had better be convex if built from a convex hull.
      ENSURE(convex());

      // Ensure the seed points are contained.
      for (vector<Vector>::const_iterator itr = points.begin();
           itr != points.end();
           ++itr) ENSURE(convexContains(*itr));
    }
    END_CONTRACT_SCOPE;
  }
}

//------------------------------------------------------------------------------
// Copy constructor.
//------------------------------------------------------------------------------
GeomPolygon::
GeomPolygon(const GeomPolygon& rhs):
  mVertices(rhs.mVertices),
  mFacets(),
  mXmin(rhs.mXmin),
  mXmax(rhs.mXmax),
  mConvex(rhs.mConvex) {
  mFacets.reserve(rhs.mFacets.size());
  for (vector<Facet>::const_iterator itr = rhs.mFacets.begin();
       itr != rhs.mFacets.end();
       ++itr) mFacets.push_back(Facet(mVertices,
                                      itr->ipoint1(),
                                      itr->ipoint2()));
  ENSURE(mFacets.size() == rhs.mFacets.size());
}

//------------------------------------------------------------------------------
// Assignment operator.
//------------------------------------------------------------------------------
GeomPolygon&
GeomPolygon::
operator=(const GeomPolygon& rhs) {
  if (this != &rhs) {
    mVertices = rhs.mVertices;
    mFacets = vector<Facet>();
    mFacets.reserve(rhs.mFacets.size());
    for (vector<Facet>::const_iterator itr = rhs.mFacets.begin();
         itr != rhs.mFacets.end();
         ++itr) mFacets.push_back(Facet(mVertices,
                                        itr->ipoint1(),
                                        itr->ipoint2()));
    mXmin = rhs.mXmin;
    mXmax = rhs.mXmax;
    mConvex = rhs.mConvex;
  }
  ENSURE(mFacets.size() == rhs.mFacets.size());
  return *this;
}

//------------------------------------------------------------------------------
// Destructor.
//------------------------------------------------------------------------------
GeomPolygon::
~GeomPolygon() {
}

//------------------------------------------------------------------------------
// Test if the given point is on the interior of the polygon.
// Generic test.
//------------------------------------------------------------------------------
bool
GeomPolygon::
contains(const GeomPolygon::Vector& point,
         const bool countBoundary,
         const double tol) const {
  if (mConvex) {
    return this->convexContains(point, countBoundary, tol);
  } else {
    return pointInPolygon(point, *this, countBoundary, tol);
  }
}

//------------------------------------------------------------------------------
// Test if the given point is on the interior of the polygon.
// This method only works for convex polygons!
//------------------------------------------------------------------------------
bool
GeomPolygon::
convexContains(const GeomPolygon::Vector& point,
               const bool countBoundary,
               const double tol) const {
  if (not testPointInBox(point, mXmin, mXmax, tol)) return false;
  vector<Facet>::const_iterator facetItr = mFacets.begin();
  bool result = true;
  if (countBoundary) {
    while (facetItr != mFacets.end() and result) {
      result = (facetItr->compare(point, tol) <= 0);
      ++facetItr;
    }
  } else {
    while (facetItr != mFacets.end() and result) {
      result = (facetItr->compare(point, tol) < 0);
      ++facetItr;
    }
  }
  return result;
}

//------------------------------------------------------------------------------
// Test if we intersect the given polygon.
//------------------------------------------------------------------------------
bool
GeomPolygon::
intersect(const GeomPolygon& rhs) const {
  if (not testBoxIntersection(mXmin, mXmax, rhs.mXmin, rhs.mXmax)) return false;
  for (std::vector<Vector>::const_iterator itr = mVertices.begin();
       itr != mVertices.end();
       ++itr) {
    if (rhs.contains(*itr)) return true;
  }
  for (std::vector<Vector>::const_iterator itr = rhs.mVertices.begin();
       itr != rhs.mVertices.end();
       ++itr) {
    if (this->contains(*itr)) return true;
  }
  unsigned i0, j0, i1, j1;
  const unsigned n0 = mVertices.size();
  const unsigned n1 = rhs.mVertices.size();
  for (i0 = 0; i0 != n0; ++i0) {
    j0 = (i0 + 1) % n0;
    for (i1 = 0; i1 != n1; ++i1) {
      j1 = (i1 + 1) % n1;
      if (segmentSegmentIntersection(mVertices[i0], mVertices[j0],
                                     rhs.mVertices[i1], rhs.mVertices[j1])) return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
// Test if we intersect the given polygon using the knowledge that both polygons
// are convex.
// We use the method of separating axes here.
//------------------------------------------------------------------------------
bool
GeomPolygon::
convexIntersect(const GeomPolygon& rhs) const {
  REQUIRE(this->convex());
  if (not testBoxIntersection(mXmin, mXmax, rhs.mXmin, rhs.mXmax)) return false;
  
  // Check if we can exclude rhs from us.
  bool outside = true;
  {
    std::vector<Facet>::const_iterator facetItr = mFacets.begin();
    while (outside and facetItr != mFacets.end()) {
      outside = (facetItr->compare(rhs.mVertices) == 1);
      ++facetItr;
    }
    if (outside) return false;
  }

  // Check if we can exclude us from rhs.
  outside = true;
  {
    std::vector<Facet>::const_iterator facetItr = rhs.mFacets.begin();
    while (outside and facetItr != rhs.mFacets.end()) {
      outside = (facetItr->compare(mVertices) == 1);
      ++facetItr;
    }
    if (outside) return false;
  }

  // We can't exclude anybody, so must intersect!
  return true;
}

//------------------------------------------------------------------------------
// Test if we intersect a box.
//------------------------------------------------------------------------------
bool
GeomPolygon::
intersect(const GeomPolygon::Box& rhs) const {
  for (std::vector<Vector>::const_iterator itr = mVertices.begin();
       itr != mVertices.end();
       ++itr) {
    if (testPointInBox(*itr, rhs)) return true;
  }

  typedef Wm5::Vector2<double> WMVector;
  vector<WMVector> WMvertices(4);
  rhs.ComputeVertices(&WMvertices.front());
  if (this->contains(convertWMVectorToVector<Dim<2> >(WMvertices[0]))) return true;
  if (this->contains(convertWMVectorToVector<Dim<2> >(WMvertices[1]))) return true;
  if (this->contains(convertWMVectorToVector<Dim<2> >(WMvertices[2]))) return true;
  if (this->contains(convertWMVectorToVector<Dim<2> >(WMvertices[3]))) return true;

  return false;
}

//------------------------------------------------------------------------------
// Compute the intersections with a line segment.
//------------------------------------------------------------------------------
vector<GeomPolygon::Vector>
GeomPolygon::
intersect(const GeomPolygon::Vector& x0, const GeomPolygon::Vector& x1) const {
  vector<Vector> result;
  Vector inter1, inter2;
  for (size_t inode = 0; inode != mVertices.size(); ++inode) {
    const Vector& e0 = mVertices[inode];
    const Vector& e1 = mVertices[inode % mVertices.size()];
    const char code = segmentSegmentIntersection(x0, x1, e0, e1, inter1, inter2);

    if (code == '1' or code == 'v') {
      // Proper intersection.
      result.push_back(inter1);

    } else if (code == 'e') {
      // The segment is colinear with and overlaps the edge.  In this case we 
      // return the end-points of the section that is on both segments.
      result.push_back(inter1);
      result.push_back(inter2);
      // if (between(x0, x1, e0)) result.push_back(e0);
      // if (between(x0, x1, e1)) result.push_back(e1);
      // if (between(e0, e1, x0)) result.push_back(x0);
      // if (between(e0, e1, x1)) result.push_back(x1);
    }
  }

  // It's possible that we may have duplicates in the intersection set. 
  // Make it unique!
  sort(result.begin(), result.end());
  result.erase(unique(result.begin(), result.end()), result.end());

  // That's it.
  ENSURE(result.size() <= 2);
  return result;
}

//------------------------------------------------------------------------------
// Compute the centroid.
//------------------------------------------------------------------------------
GeomPolygon::Vector
GeomPolygon::
centroid() const {
  return std::accumulate(mVertices.begin(), mVertices.end(), Vector())/
    max(unsigned(mVertices.size()), 1U);
}

//------------------------------------------------------------------------------
// Return the edges of the polygon as a set of integer pairs for the vertices.
// This is simplified because we know the vertices are already sorted 
// counter-clockwise.
//------------------------------------------------------------------------------
vector<pair<unsigned, unsigned> >
GeomPolygon::
edges() const {
  vector<pair<unsigned, unsigned> > result;
  for (unsigned i = 0; i != mVertices.size(); ++i) {
    result.push_back(make_pair(i, (i + 1) % mVertices.size()));
  }
  return result;
}

//------------------------------------------------------------------------------
// Spit out an encoding of the facets as ordered vertex indices.
//------------------------------------------------------------------------------
vector<vector<unsigned> >
GeomPolygon::
facetVertices() const {
  vector<vector<unsigned> > result;
  if (mVertices.size() > 0) {
    for (vector<Facet>::const_iterator itr = mFacets.begin();
         itr != mFacets.end();
         ++itr) {
      vector<unsigned> facet;
      facet.push_back(itr->ipoint1());
      facet.push_back(itr->ipoint2());
      CHECK(facet.size() == 2);
      CHECK(facet[0] < mVertices.size());
      CHECK(facet[1] < mVertices.size());
      result.push_back(facet);
    }
  }
  return result;
}

//------------------------------------------------------------------------------
// Reconstruct the internal state given the set of vertices and the enocded 
// facets.
//------------------------------------------------------------------------------
void
GeomPolygon::
reconstruct(const vector<GeomPolygon::Vector>& vertices,
            const vector<vector<unsigned> >& facetVertices) {
  mVertices = vertices;
  mFacets = vector<Facet>();
  mFacets.reserve(facetVertices.size());
  for (vector<vector<unsigned> >::const_iterator itr = facetVertices.begin();
       itr != facetVertices.end();
       ++itr) {
    CHECK2(itr->size() == 2, "Bad size:  " << itr->size());
    mFacets.push_back(Facet(mVertices, (*itr)[0], (*itr)[1]));
  }
  setBoundingBox();
  mConvex = this->convex();
  ENSURE(mFacets.size() == facetVertices.size());
}

//------------------------------------------------------------------------------
// Compute the volume.
//------------------------------------------------------------------------------
double
GeomPolygon::
volume() const {
  double result = 0.0;
  const Vector c = centroid();
  for (vector<Facet>::const_iterator itr = mFacets.begin();
       itr != mFacets.end();
       ++itr) {
    result += ((itr->point2() - itr->point1()).cross(c - itr->point1())).z();
  }
  ENSURE2(result >= 0.0, result);
  return 0.5*result;
}

//------------------------------------------------------------------------------
// Find the minimum distance to a point.
//------------------------------------------------------------------------------
double
GeomPolygon::
distance(const GeomPolygon::Vector& p) const {
  return (p - this->closestPoint(p)).magnitude();
}

//------------------------------------------------------------------------------
// Find the point in the polygon closest to the given point.
//------------------------------------------------------------------------------
GeomPolygon::Vector
GeomPolygon::
closestPoint(const GeomPolygon::Vector& p) const {
  double r2, minr2 = numeric_limits<double>::max();
  Vector result, thpt;
  for (vector<Facet>::const_iterator itr = mFacets.begin();
       itr != mFacets.end();
       ++itr) {
    thpt = itr->closestPoint(p);
    r2 = (thpt - p).magnitude2();
    if (r2 < minr2) {
      result = thpt;
      minr2 = r2;
    }
  }
  return result;
}

//------------------------------------------------------------------------------
// ==
//------------------------------------------------------------------------------
bool
GeomPolygon::
operator==(const GeomPolygon& rhs) const {
  bool result = (mVertices == rhs.mVertices and
                 mFacets.size() == rhs.mFacets.size());
  int i = 0;
  while (result and i != mFacets.size()) {
    result = mFacets[i] == rhs.mFacets[i];
    ++i;
  }
  return result;
}

//------------------------------------------------------------------------------
// !=
//------------------------------------------------------------------------------
bool
GeomPolygon::
operator!=(const GeomPolygon& rhs) const {
  return not (*this == rhs);
}

//------------------------------------------------------------------------------
// Set the minimum and maximum extents of the polygon.
//------------------------------------------------------------------------------
void
GeomPolygon::
setBoundingBox() {
  mXmin = DBL_MAX;
  mXmax = -DBL_MAX;
  for (vector<Vector>::const_iterator itr = mVertices.begin();
       itr != mVertices.end();
       ++itr) {
    mXmin = elementWiseMin(mXmin, *itr);
    mXmax = elementWiseMax(mXmax, *itr);
  }
}

//------------------------------------------------------------------------------
// Test if the polygon is convex.
//------------------------------------------------------------------------------
bool
GeomPolygon::
convex(const double tol) const {
  // Do the convex comparison for each vertex.
  bool result = true;
  const double reltol = tol*max(1.0, (mXmax - mXmin).maxAbsElement());
  vector<Vector>::const_iterator vertexItr = mVertices.begin();
  while (vertexItr != mVertices.end() and result) {
    vector<Facet>::const_iterator facetItr = mFacets.begin();
    while (facetItr != mFacets.end() and result) {
      result = (facetItr->compare(*vertexItr, reltol) <= 0);
      ++facetItr;
    }
    ++vertexItr;
  }
  return result;
}

//------------------------------------------------------------------------------
// Initialization.
//------------------------------------------------------------------------------
FILE* GeomPolygon::mDevnull = NULL;

}


