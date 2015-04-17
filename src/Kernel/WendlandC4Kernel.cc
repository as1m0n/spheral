//---------------------------------Spheral++----------------------------------//
// WendlandC4Kernel -- .
//
// Created by CDR, Nov 5 2014
//----------------------------------------------------------------------------//

#include <math.h>

#include "Kernel.hh"
#include "WendlandC4Kernel.hh"

//------------------------------------------------------------------------------
// Explicit instantiation.
//------------------------------------------------------------------------------
namespace Spheral {
  namespace KernelSpace {
    template class WendlandC4Kernel<Dim<1> >;
    template class WendlandC4Kernel<Dim<2> >;
    template class WendlandC4Kernel<Dim<3> >;
  }
}
