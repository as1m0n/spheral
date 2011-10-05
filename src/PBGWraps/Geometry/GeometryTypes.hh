#ifndef __PBGWRAPS_GEOMETRYTYPES__
#define __PBGWRAPS_GEOMETRYTYPES__

#include <vector>
#include <string>
#include <sstream>
#include "Geometry/Dimension.hh"
#include "Geometry/GeomVector.hh"
#include "Geometry/Geom3Vector.hh"
#include "Geometry/GeomTensor.hh"
#include "Geometry/GeomSymmetricTensor.hh"
#include "Geometry/GeomThirdRankTensor.hh"
#include "Geometry/EigenStruct.hh"
#include "Geometry/computeEigenValues.hh"
#include "Geometry/GeomPlane.hh"
#include "Geometry/GeomPolygon.hh"
#include "Geometry/GeomPolyhedron.hh"
#include "Utilities/DataTypeTraits.hh"

using namespace Spheral;

namespace Spheral {

//------------------------------------------------------------------------------
// Names!
//------------------------------------------------------------------------------
typedef GeomVector<1> Vector1d;
typedef GeomVector<2> Vector2d;
typedef GeomVector<3> Vector3d;

typedef GeomTensor<1> Tensor1d;
typedef GeomTensor<2> Tensor2d;
typedef GeomTensor<3> Tensor3d;

typedef GeomSymmetricTensor<1> SymTensor1d;
typedef GeomSymmetricTensor<2> SymTensor2d;
typedef GeomSymmetricTensor<3> SymTensor3d;

typedef GeomThirdRankTensor<1> ThirdRankTensor1d;
typedef GeomThirdRankTensor<2> ThirdRankTensor2d;
typedef GeomThirdRankTensor<3> ThirdRankTensor3d;

typedef EigenStruct<1> EigenStruct1d;
typedef EigenStruct<2> EigenStruct2d;
typedef EigenStruct<3> EigenStruct3d;

typedef GeomPlane<Dim<1> > Plane1d;
typedef GeomPlane<Dim<2> > Plane2d;
typedef GeomPlane<Dim<3> > Plane3d;

typedef GeomFacet2d Facet2d;
typedef GeomPolygon Polygon;

typedef GeomFacet3d Facet3d;
typedef GeomPolyhedron Polyhedron;

}

typedef std::vector<Spheral::Facet2d> vector_of_Facet2d;
typedef std::vector<Spheral::Facet3d> vector_of_Facet3d;

//------------------------------------------------------------------------------
// Vector sequence methods.
//------------------------------------------------------------------------------
namespace Spheral {

template<typename Type>
inline
int
sizeGeomType(Type& self) {
  return DataTypeTraits<Type>::numElements();
}

template<typename Vector>
inline
double
indexGeomVector(Vector& self,
                const size_t index) {
  if (index < Vector::nDimensions) {
    return self(index);
  } else {
    PyErr_SetString(PyExc_IndexError, "Vector index out of range");
    return 0.0;
  }
}

template<typename Vector>
inline
void
assignToGeomVector(Vector& self, 
                   const size_t index,
                   const double value) {
  if (index >= Vector::nDimensions) {
    PyErr_SetString(PyExc_IndexError, "Container index out of range");
  } else {
    self(index) = value;
  }
}

//------------------------------------------------------------------------------
// Tensor sequence methods.
//------------------------------------------------------------------------------
template<typename Tensor>
inline
double
indexGeomTensor(Tensor& self,
                const size_t index) {
  if (index < DataTypeTraits<Tensor>::numElements()) {
    return *(self.begin() + index);
  } else {
    PyErr_SetString(PyExc_IndexError, "Tensor index out of range");
    return 0.0;
  }
}

template<typename Tensor>
inline
void
assignToGeomTensor(Tensor& self, 
                   const size_t index,
                   const double value) {
  if (index >= DataTypeTraits<Tensor>::numElements()) {
    PyErr_SetString(PyExc_IndexError, "Tensor index out of range");
  } else {
    *(self.begin() + index) = value;
  }
}

//------------------------------------------------------------------------------
// Set the given element of a third rank tensor.
//------------------------------------------------------------------------------
template<typename TRT>
inline
void
assignThirdRankTensorElement(TRT& self,
                             const size_t i,
                             const size_t j,
                             const size_t k,
                             const double val) {
  self(i,j,k) = val;
}

//------------------------------------------------------------------------------
// Nice string representations (Vector)
//------------------------------------------------------------------------------
template<typename Vector>
inline
std::string
printReprVector(const Vector& val) {
  std::stringstream s;
  s << "Vector" << Vector::nDimensions << "d( ";
  for (size_t i = 0; i != Vector::nDimensions; ++i) {
    s << val(i) << " ";
  }
  s << ")";
  return s.str();
}

//------------------------------------------------------------------------------
// Nice string representations (Tensor)
//------------------------------------------------------------------------------
template<typename Tensor>
inline
std::string
printReprTensor(const Tensor& val) {
  std::stringstream s;
  s << "Tensor" << Tensor::nDimensions << "d(";
  for (size_t row = 0; row != Tensor::nDimensions; ++row) {
    s << "( ";
    for (size_t col = 0; col != Tensor::nDimensions; ++col) {
      s << val(row, col) << " ";
    }
    s << ")";
  }
  s << ")";
  return s.str();
}

//------------------------------------------------------------------------------
// Nice string representations (ThirdRankTensor)
//------------------------------------------------------------------------------
template<typename TRTensor>
inline
std::string
printReprThirdRankTensor(const TRTensor& val) {
  std::stringstream s;
  s << "Tensor" << TRTensor::nDimensions << "d(";
  for (size_t i = 0; i != TRTensor::nDimensions; ++i) {
    s << "( ";
    for (size_t j = 0; j != TRTensor::nDimensions; ++j) {
      s << "( ";
      for (size_t k = 0; k != TRTensor::nDimensions; ++k) {
        s << val(i, j, k) << " ";
      }
      s << ")";
    }
    s << ")";
  }
  s << ")";
  return s.str();
}

}

#endif
