//---------------------------------Spheral++----------------------------------//
// RegisterMPIDataTypes
// A singleton helper class that registers special Spheral defined data types
// with MPI.
//
// Created by J. Michael Owen, Tue Jul  7 14:02:12 PDT 2009
//----------------------------------------------------------------------------//

#include "RegisterMPIDataTypes.hh"
#include "DataTypeTraits.hh"
#include "Geometry/Dimension.hh"

namespace Spheral {

//------------------------------------------------------------------------------
// Get the instance.
//------------------------------------------------------------------------------
RegisterMPIDataTypes&
RegisterMPIDataTypes::
instance() {
   if (mInstancePtr == 0) mInstancePtr = new RegisterMPIDataTypes;
   CHECK(mInstancePtr != 0);
   return *mInstancePtr;
}

//------------------------------------------------------------------------------
// Constructor (private).
//------------------------------------------------------------------------------
RegisterMPIDataTypes::
RegisterMPIDataTypes() {

#ifdef USE_MPI
  // Vectors.
  MPI_Type_contiguous(DataTypeTraits<Dim<1>::Vector>::numElements(), MPI_DOUBLE, &MPI_Vector1d);
  MPI_Type_contiguous(DataTypeTraits<Dim<2>::Vector>::numElements(), MPI_DOUBLE, &MPI_Vector2d);
  MPI_Type_contiguous(DataTypeTraits<Dim<3>::Vector>::numElements(), MPI_DOUBLE, &MPI_Vector3d);
  MPI_Type_commit(&MPI_Vector1d);
  MPI_Type_commit(&MPI_Vector2d);
  MPI_Type_commit(&MPI_Vector3d);

//   {
//     int block_lengths[2];
//     MPI_Aint displacements[2];
//     MPI_Aint addresses[3];
//     MPI_Datatype type_list[2];

//     // First specify the types.
//     type_list[0] = MPI_DOUBLE;
//     type_list[1] = MPI_DOUBLE;

//     // Specify the number of elements of each type.
//     block_lengths[0] = 1;
//     block_lengths[1] = 1;

//     // Calculate the displacements of the members
//     // relative to indata.
//     Dim<2>::Vector tmp;
//     MPI_Address(&tmp, &addresses[0]);
//     MPI_Address(&tmp(0), &addresses[1]);
//     MPI_Address(&tmp(1), &addresses[2]);
//     displacements[0] = addresses[1] - addresses[0];
//     displacements[1] = addresses[2] - addresses[0];

//     // Create the derived type.
//     MPI_Type_struct(2, block_lengths, displacements, type_list, &MPI_Vector2d);
//     MPI_Type_commit(&MPI_Vector2d);
//   }

  // Tensors.
  MPI_Type_contiguous(DataTypeTraits<Dim<1>::Tensor>::numElements(), MPI_DOUBLE, &MPI_Tensor1d);
  MPI_Type_contiguous(DataTypeTraits<Dim<2>::Tensor>::numElements(), MPI_DOUBLE, &MPI_Tensor2d);
  MPI_Type_contiguous(DataTypeTraits<Dim<3>::Tensor>::numElements(), MPI_DOUBLE, &MPI_Tensor3d);
  MPI_Type_commit(&MPI_Tensor1d);
  MPI_Type_commit(&MPI_Tensor2d);
  MPI_Type_commit(&MPI_Tensor3d);

  // SymTensors.
  MPI_Type_contiguous(DataTypeTraits<Dim<1>::SymTensor>::numElements(), MPI_DOUBLE, &MPI_SymTensor1d);
  MPI_Type_contiguous(DataTypeTraits<Dim<2>::SymTensor>::numElements(), MPI_DOUBLE, &MPI_SymTensor2d);
  MPI_Type_contiguous(DataTypeTraits<Dim<3>::SymTensor>::numElements(), MPI_DOUBLE, &MPI_SymTensor3d);
  MPI_Type_commit(&MPI_SymTensor1d);
  MPI_Type_commit(&MPI_SymTensor2d);
  MPI_Type_commit(&MPI_SymTensor3d);

  // ThirdRankTensors.
  MPI_Type_contiguous(DataTypeTraits<Dim<1>::ThirdRankTensor>::numElements(), MPI_DOUBLE, &MPI_ThirdRankTensor1d);
  MPI_Type_contiguous(DataTypeTraits<Dim<2>::ThirdRankTensor>::numElements(), MPI_DOUBLE, &MPI_ThirdRankTensor2d);
  MPI_Type_contiguous(DataTypeTraits<Dim<3>::ThirdRankTensor>::numElements(), MPI_DOUBLE, &MPI_ThirdRankTensor3d);
  MPI_Type_commit(&MPI_ThirdRankTensor1d);
  MPI_Type_commit(&MPI_ThirdRankTensor2d);
  MPI_Type_commit(&MPI_ThirdRankTensor3d);
#endif

}

}

//------------------------------------------------------------------------------
// Initialize the static instance pointer.
//-----------------------------------------------------------------------------
Spheral::RegisterMPIDataTypes* Spheral::RegisterMPIDataTypes::mInstancePtr = 0;
