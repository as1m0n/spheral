#include "RK/ReproducingKernel.hh"

namespace Spheral {

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
template<typename Dimension>
ReproducingKernel<Dimension>::
ReproducingKernel(const TableKernel<Dimension>& W,
                  const RKOrder order):
  mWptr(&W),
  mOrder(order) {
  switch(order) {
  case RKOrder::ZerothOrder:
    mEvaluateBaseKernel = &RKUtilities<Dimension, RKOrder::ZerothOrder>::evaluateBaseKernel;
    mEvaluateBaseGradient = &RKUtilities<Dimension, RKOrder::ZerothOrder>::evaluateBaseGradient;
    mEvaluateBaseHessian = &RKUtilities<Dimension, RKOrder::ZerothOrder>::evaluateBaseHessian;
    mEvaluateBaseKernelAndGradient = &RKUtilities<Dimension, RKOrder::ZerothOrder>::evaluateBaseKernelAndGradient;
    mEvaluateKernel = &RKUtilities<Dimension, RKOrder::ZerothOrder>::evaluateKernel;
    mEvaluateGradient = &RKUtilities<Dimension, RKOrder::ZerothOrder>::evaluateGradient;
    mEvaluateHessian = &RKUtilities<Dimension, RKOrder::ZerothOrder>::evaluateHessian;
    mEvaluateKernelAndGradient = &RKUtilities<Dimension, RKOrder::ZerothOrder>::evaluateKernelAndGradient;
    mEvaluateKernelAndGradients = &RKUtilities<Dimension, RKOrder::ZerothOrder>::evaluateKernelAndGradients;
    mComputeCorrections = &RKUtilities<Dimension, RKOrder::ZerothOrder>::computeCorrections;
    mComputeNormal = &RKUtilities<Dimension, RKOrder::ZerothOrder>::computeNormal;
    mApplyTransformation = RKUtilities<Dimension, RKOrder::ZerothOrder>::applyTransformation;
    break;
  case RKOrder::LinearOrder:
    mEvaluateBaseKernel = &RKUtilities<Dimension, RKOrder::LinearOrder>::evaluateBaseKernel;
    mEvaluateBaseGradient = &RKUtilities<Dimension, RKOrder::LinearOrder>::evaluateBaseGradient;
    mEvaluateBaseHessian = &RKUtilities<Dimension, RKOrder::LinearOrder>::evaluateBaseHessian;
    mEvaluateBaseKernelAndGradient = &RKUtilities<Dimension, RKOrder::LinearOrder>::evaluateBaseKernelAndGradient;
    mEvaluateKernel = &RKUtilities<Dimension, RKOrder::LinearOrder>::evaluateKernel;
    mEvaluateGradient = &RKUtilities<Dimension, RKOrder::LinearOrder>::evaluateGradient;
    mEvaluateHessian = &RKUtilities<Dimension, RKOrder::LinearOrder>::evaluateHessian;
    mEvaluateKernelAndGradient = &RKUtilities<Dimension, RKOrder::LinearOrder>::evaluateKernelAndGradient;
    mEvaluateKernelAndGradients = &RKUtilities<Dimension, RKOrder::LinearOrder>::evaluateKernelAndGradients;
    mComputeCorrections = &RKUtilities<Dimension, RKOrder::LinearOrder>::computeCorrections;
    mComputeNormal = &RKUtilities<Dimension, RKOrder::LinearOrder>::computeNormal;
    mApplyTransformation = RKUtilities<Dimension, RKOrder::LinearOrder>::applyTransformation;
    break;
  case RKOrder::QuadraticOrder:
    mEvaluateBaseKernel = &RKUtilities<Dimension, RKOrder::QuadraticOrder>::evaluateBaseKernel;
    mEvaluateBaseGradient = &RKUtilities<Dimension, RKOrder::QuadraticOrder>::evaluateBaseGradient;
    mEvaluateBaseHessian = &RKUtilities<Dimension, RKOrder::QuadraticOrder>::evaluateBaseHessian;
    mEvaluateBaseKernelAndGradient = &RKUtilities<Dimension, RKOrder::QuadraticOrder>::evaluateBaseKernelAndGradient;
    mEvaluateKernel = &RKUtilities<Dimension, RKOrder::QuadraticOrder>::evaluateKernel;
    mEvaluateGradient = &RKUtilities<Dimension, RKOrder::QuadraticOrder>::evaluateGradient;
    mEvaluateHessian = &RKUtilities<Dimension, RKOrder::QuadraticOrder>::evaluateHessian;
    mEvaluateKernelAndGradient = &RKUtilities<Dimension, RKOrder::QuadraticOrder>::evaluateKernelAndGradient;
    mEvaluateKernelAndGradients = &RKUtilities<Dimension, RKOrder::QuadraticOrder>::evaluateKernelAndGradients;
    mComputeCorrections = &RKUtilities<Dimension, RKOrder::QuadraticOrder>::computeCorrections;
    mComputeNormal = &RKUtilities<Dimension, RKOrder::QuadraticOrder>::computeNormal;
    mApplyTransformation = RKUtilities<Dimension, RKOrder::QuadraticOrder>::applyTransformation;
    break;
  case RKOrder::CubicOrder:
    mEvaluateBaseKernel = &RKUtilities<Dimension, RKOrder::CubicOrder>::evaluateBaseKernel;
    mEvaluateBaseGradient = &RKUtilities<Dimension, RKOrder::CubicOrder>::evaluateBaseGradient;
    mEvaluateBaseHessian = &RKUtilities<Dimension, RKOrder::CubicOrder>::evaluateBaseHessian;
    mEvaluateBaseKernelAndGradient = &RKUtilities<Dimension, RKOrder::CubicOrder>::evaluateBaseKernelAndGradient;
    mEvaluateKernel = &RKUtilities<Dimension, RKOrder::CubicOrder>::evaluateKernel;
    mEvaluateGradient = &RKUtilities<Dimension, RKOrder::CubicOrder>::evaluateGradient;
    mEvaluateHessian = &RKUtilities<Dimension, RKOrder::CubicOrder>::evaluateHessian;
    mEvaluateKernelAndGradient = &RKUtilities<Dimension, RKOrder::CubicOrder>::evaluateKernelAndGradient;
    mEvaluateKernelAndGradients = &RKUtilities<Dimension, RKOrder::CubicOrder>::evaluateKernelAndGradients;
    mComputeCorrections = &RKUtilities<Dimension, RKOrder::CubicOrder>::computeCorrections;
    mComputeNormal = &RKUtilities<Dimension, RKOrder::CubicOrder>::computeNormal;
    mApplyTransformation = RKUtilities<Dimension, RKOrder::CubicOrder>::applyTransformation;
    break;
  case RKOrder::QuarticOrder:
    mEvaluateBaseKernel = &RKUtilities<Dimension, RKOrder::QuarticOrder>::evaluateBaseKernel;
    mEvaluateBaseGradient = &RKUtilities<Dimension, RKOrder::QuarticOrder>::evaluateBaseGradient;
    mEvaluateBaseHessian = &RKUtilities<Dimension, RKOrder::QuarticOrder>::evaluateBaseHessian;
    mEvaluateBaseKernelAndGradient = &RKUtilities<Dimension, RKOrder::QuarticOrder>::evaluateBaseKernelAndGradient;
    mEvaluateKernel = &RKUtilities<Dimension, RKOrder::QuarticOrder>::evaluateKernel;
    mEvaluateGradient = &RKUtilities<Dimension, RKOrder::QuarticOrder>::evaluateGradient;
    mEvaluateHessian = &RKUtilities<Dimension, RKOrder::QuarticOrder>::evaluateHessian;
    mEvaluateKernelAndGradient = &RKUtilities<Dimension, RKOrder::QuarticOrder>::evaluateKernelAndGradient;
    mEvaluateKernelAndGradients = &RKUtilities<Dimension, RKOrder::QuarticOrder>::evaluateKernelAndGradients;
    mComputeCorrections = &RKUtilities<Dimension, RKOrder::QuarticOrder>::computeCorrections;
    mComputeNormal = &RKUtilities<Dimension, RKOrder::QuarticOrder>::computeNormal;
    mApplyTransformation = RKUtilities<Dimension, RKOrder::QuarticOrder>::applyTransformation;
    break;
  case RKOrder::QuinticOrder:
    mEvaluateBaseKernel = &RKUtilities<Dimension, RKOrder::QuinticOrder>::evaluateBaseKernel;
    mEvaluateBaseGradient = &RKUtilities<Dimension, RKOrder::QuinticOrder>::evaluateBaseGradient;
    mEvaluateBaseHessian = &RKUtilities<Dimension, RKOrder::QuinticOrder>::evaluateBaseHessian;
    mEvaluateBaseKernelAndGradient = &RKUtilities<Dimension, RKOrder::QuinticOrder>::evaluateBaseKernelAndGradient;
    mEvaluateKernel = &RKUtilities<Dimension, RKOrder::QuinticOrder>::evaluateKernel;
    mEvaluateGradient = &RKUtilities<Dimension, RKOrder::QuinticOrder>::evaluateGradient;
    mEvaluateHessian = &RKUtilities<Dimension, RKOrder::QuinticOrder>::evaluateHessian;
    mEvaluateKernelAndGradient = &RKUtilities<Dimension, RKOrder::QuinticOrder>::evaluateKernelAndGradient;
    mEvaluateKernelAndGradients = &RKUtilities<Dimension, RKOrder::QuinticOrder>::evaluateKernelAndGradients;
    mComputeCorrections = &RKUtilities<Dimension, RKOrder::QuinticOrder>::computeCorrections;
    mComputeNormal = &RKUtilities<Dimension, RKOrder::QuinticOrder>::computeNormal;
    mApplyTransformation = RKUtilities<Dimension, RKOrder::QuinticOrder>::applyTransformation;
    break;
  case RKOrder::SexticOrder:
    mEvaluateBaseKernel = &RKUtilities<Dimension, RKOrder::SexticOrder>::evaluateBaseKernel;
    mEvaluateBaseGradient = &RKUtilities<Dimension, RKOrder::SexticOrder>::evaluateBaseGradient;
    mEvaluateBaseHessian = &RKUtilities<Dimension, RKOrder::SexticOrder>::evaluateBaseHessian;
    mEvaluateBaseKernelAndGradient = &RKUtilities<Dimension, RKOrder::SexticOrder>::evaluateBaseKernelAndGradient;
    mEvaluateKernel = &RKUtilities<Dimension, RKOrder::SexticOrder>::evaluateKernel;
    mEvaluateGradient = &RKUtilities<Dimension, RKOrder::SexticOrder>::evaluateGradient;
    mEvaluateHessian = &RKUtilities<Dimension, RKOrder::SexticOrder>::evaluateHessian;
    mEvaluateKernelAndGradient = &RKUtilities<Dimension, RKOrder::SexticOrder>::evaluateKernelAndGradient;
    mEvaluateKernelAndGradients = &RKUtilities<Dimension, RKOrder::SexticOrder>::evaluateKernelAndGradients;
    mComputeCorrections = &RKUtilities<Dimension, RKOrder::SexticOrder>::computeCorrections;
    mComputeNormal = &RKUtilities<Dimension, RKOrder::SexticOrder>::computeNormal;
    mApplyTransformation = RKUtilities<Dimension, RKOrder::SexticOrder>::applyTransformation;
    break;
  case RKOrder::SepticOrder:
    mEvaluateBaseKernel = &RKUtilities<Dimension, RKOrder::SepticOrder>::evaluateBaseKernel;
    mEvaluateBaseGradient = &RKUtilities<Dimension, RKOrder::SepticOrder>::evaluateBaseGradient;
    mEvaluateBaseHessian = &RKUtilities<Dimension, RKOrder::SepticOrder>::evaluateBaseHessian;
    mEvaluateBaseKernelAndGradient = &RKUtilities<Dimension, RKOrder::SepticOrder>::evaluateBaseKernelAndGradient;
    mEvaluateKernel = &RKUtilities<Dimension, RKOrder::SepticOrder>::evaluateKernel;
    mEvaluateGradient = &RKUtilities<Dimension, RKOrder::SepticOrder>::evaluateGradient;
    mEvaluateHessian = &RKUtilities<Dimension, RKOrder::SepticOrder>::evaluateHessian;
    mEvaluateKernelAndGradient = &RKUtilities<Dimension, RKOrder::SepticOrder>::evaluateKernelAndGradient;
    mEvaluateKernelAndGradients = &RKUtilities<Dimension, RKOrder::SepticOrder>::evaluateKernelAndGradients;
    mComputeCorrections = &RKUtilities<Dimension, RKOrder::SepticOrder>::computeCorrections;
    mComputeNormal = &RKUtilities<Dimension, RKOrder::SepticOrder>::computeNormal;
    mApplyTransformation = RKUtilities<Dimension, RKOrder::SepticOrder>::applyTransformation;
    break;
  default:
    VERIFY2("Unknown order passed to ReproducingKernel", false);
  }
}

//------------------------------------------------------------------------------
// Default constructor (everything NULL, invalid)
//------------------------------------------------------------------------------
template<typename Dimension>
ReproducingKernel<Dimension>::
ReproducingKernel():
  mWptr(nullptr),
  mEvaluateBaseKernel(nullptr),
  mEvaluateBaseGradient(nullptr),
  mEvaluateBaseHessian(nullptr),
  mEvaluateBaseKernelAndGradient(nullptr),
  mEvaluateKernel(nullptr),
  mEvaluateGradient(nullptr),
  mEvaluateHessian(nullptr),
  mEvaluateKernelAndGradient(nullptr),
  mEvaluateKernelAndGradients(nullptr),
  mComputeCorrections(nullptr),
  mComputeNormal(nullptr),
  mApplyTransformation(nullptr) {
}

//------------------------------------------------------------------------------
// Copy
//------------------------------------------------------------------------------
template<typename Dimension>
ReproducingKernel<Dimension>::
ReproducingKernel(const ReproducingKernel<Dimension>& rhs):
  mWptr(rhs.mWptr),
  mOrder(rhs.mOrder),
  mEvaluateBaseKernel(rhs.mEvaluateBaseKernel),
  mEvaluateBaseGradient(rhs.mEvaluateBaseGradient),
  mEvaluateBaseHessian(rhs.mEvaluateBaseHessian),
  mEvaluateBaseKernelAndGradient(rhs.mEvaluateBaseKernelAndGradient),
  mEvaluateKernel(rhs.mEvaluateKernel),
  mEvaluateGradient(rhs.mEvaluateGradient),
  mEvaluateHessian(rhs.mEvaluateHessian),
  mEvaluateKernelAndGradient(rhs.mEvaluateKernelAndGradient),
  mEvaluateKernelAndGradients(rhs.mEvaluateKernelAndGradients),
  mComputeCorrections(rhs.mComputeCorrections),
  mComputeNormal(rhs.mComputeNormal),
  mApplyTransformation(rhs.mApplyTransformation) {
}

//------------------------------------------------------------------------------
// Assignment
//------------------------------------------------------------------------------
template<typename Dimension>
ReproducingKernel<Dimension>&
ReproducingKernel<Dimension>::
operator=(const ReproducingKernel<Dimension>& rhs) {
  mWptr = rhs.mWptr;
  mOrder = rhs.mOrder;
  mEvaluateBaseKernel = rhs.mEvaluateBaseKernel;
  mEvaluateBaseGradient = rhs.mEvaluateBaseGradient;
  mEvaluateBaseHessian = rhs.mEvaluateBaseHessian;
  mEvaluateBaseKernelAndGradient = rhs.mEvaluateBaseKernelAndGradient;
  mEvaluateKernel = rhs.mEvaluateKernel;
  mEvaluateGradient = rhs.mEvaluateGradient;
  mEvaluateHessian = rhs.mEvaluateHessian;
  mEvaluateKernelAndGradient = rhs.mEvaluateKernelAndGradient;
  mEvaluateKernelAndGradients = rhs.mEvaluateKernelAndGradients;
  mComputeCorrections = rhs.mComputeCorrections;
  mComputeNormal = rhs.mComputeNormal;
  mApplyTransformation = rhs.mApplyTransformation;
  return *this;
}

//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
template<typename Dimension>
ReproducingKernel<Dimension>::
~ReproducingKernel() {
}  

}
