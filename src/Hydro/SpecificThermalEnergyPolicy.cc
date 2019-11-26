//---------------------------------Spheral++----------------------------------//
// SpecificThermalEnergyPolicy -- An implementation of UpdatePolicyBase specialized
// for the updating the specific thermal energy as a dependent quantity.
// 
// This version is specialized for the compatible energy discretization 
// method.
//
// Created by JMO, Tue Sep 14 22:27:08 2004
//----------------------------------------------------------------------------//
#include "SpecificThermalEnergyPolicy.hh"
#include "HydroFieldNames.hh"
#include "entropyWeightingFunction.hh"
#include "NodeList/NodeList.hh"
#include "NodeList/FluidNodeList.hh"
#include "DataBase/DataBase.hh"
#include "DataBase/FieldUpdatePolicyBase.hh"
#include "DataBase/IncrementFieldList.hh"
#include "DataBase/ReplaceState.hh"
#include "DataBase/State.hh"
#include "DataBase/StateDerivatives.hh"
#include "Neighbor/ConnectivityMap.hh"
#include "Field/Field.hh"
#include "Field/FieldList.hh"
#include "Utilities/DBC.hh"
#include "Utilities/safeInv.hh"
#include "Utilities/SpheralFunctions.hh"

#include <vector>
#include <limits>
using std::vector;
using std::numeric_limits;
using std::abs;
using std::min;
using std::max;

namespace Spheral {

//------------------------------------------------------------------------------
// Constructor.
//------------------------------------------------------------------------------
template<typename Dimension>
SpecificThermalEnergyPolicy<Dimension>::
SpecificThermalEnergyPolicy(const DataBase<Dimension>& dataBase):
  IncrementFieldList<Dimension, typename Dimension::Scalar>(),
  mDataBasePtr(&dataBase) {
}

//------------------------------------------------------------------------------
// Destructor.
//------------------------------------------------------------------------------
template<typename Dimension>
SpecificThermalEnergyPolicy<Dimension>::
~SpecificThermalEnergyPolicy() {
}

//------------------------------------------------------------------------------
// Update the field.
//------------------------------------------------------------------------------
template<typename Dimension>
void
SpecificThermalEnergyPolicy<Dimension>::
update(const KeyType& key,
       State<Dimension>& state,
       StateDerivatives<Dimension>& derivs,
       const double multiplier,
       const double t,
       const double dt) {

  typedef typename Dimension::SymTensor SymTensor;

//   // HACK!
//   std::cerr.setf(std::ios::scientific, std::ios::floatfield);
//   std::cerr.precision(15);

  KeyType fieldKey, nodeListKey;
  StateBase<Dimension>::splitFieldKey(key, fieldKey, nodeListKey);
  REQUIRE(fieldKey == HydroFieldNames::specificThermalEnergy and 
          nodeListKey == UpdatePolicyBase<Dimension>::wildcard());
  auto eps = state.fields(fieldKey, Scalar());
  const auto numFields = eps.numFields();

  // Get the state fields.
  const auto  mass = state.fields(HydroFieldNames::mass, Scalar());
  const auto  velocity = state.fields(HydroFieldNames::velocity, Vector::zero);
  const auto  acceleration = derivs.fields(HydroFieldNames::hydroAcceleration, Vector::zero);
  // const auto  entropy = state.fields(HydroFieldNames::entropy, Scalar());
  const auto  eps0 = state.fields(HydroFieldNames::specificThermalEnergy + "0", Scalar());
  const auto& pairAccelerations = derivs.getAny(HydroFieldNames::pairAccelerations, vector<Vector>());
  const auto  DepsDt0 = derivs.fields(IncrementFieldList<Dimension, Field<Dimension, Scalar> >::prefix() + HydroFieldNames::specificThermalEnergy, 0.0);
  const auto& connectivityMap = mDataBasePtr->connectivityMap();
  const auto& pairs = connectivityMap.nodePairList();
  const auto  npairs = pairs.size();
  CHECK(pairAccelerations.size() == npairs);

  const auto hdt = 0.5*multiplier;
  auto DepsDt = mDataBasePtr->newFluidFieldList(0.0, "delta E");

  // Walk all pairs and figure out the discrete work for each point
#pragma omp parallel
  {
    // Thread private variables
    auto DepsDt_thread = DepsDt.threadCopy();

#pragma omp for
    for (auto kk = 0; kk < npairs; ++kk) {
      const auto i = pairs[kk].i_node;
      const auto j = pairs[kk].j_node;
      const auto nodeListi = pairs[kk].i_list;
      const auto nodeListj = pairs[kk].j_list;

      // State for node i.
      const auto  weighti = abs(DepsDt0(nodeListi, i)) + numeric_limits<Scalar>::epsilon();
      // const auto  si = entropy(nodeListi, i);
      const auto  mi = mass(nodeListi, i);
      const auto& vi = velocity(nodeListi, i);
      const auto  ui = eps0(nodeListi, i);
      const auto& ai = acceleration(nodeListi, i);
      const auto  vi12 = vi + ai*hdt;
      const auto& paccij = pairAccelerations[kk];

      // State for node j.
      const auto  weightj = abs(DepsDt0(nodeListj, j)) + numeric_limits<Scalar>::epsilon();
      // const auto  sj = entropy(nodeListj, j);
      const auto  mj = mass(nodeListj, j);
      const auto& vj = velocity(nodeListj, j);
      const auto  uj = eps0(nodeListj, j);
      const auto& aj = acceleration(nodeListj, j);
      const auto  vj12 = vj + aj*hdt;

      const auto  vji12 = vj12 - vi12;
      const Scalar duij = vji12.dot(paccij);
      const Scalar wi = weighti/(weighti + weightj);         // Du/Dt weighting
      // const Scalar wi = entropyWeighting(si, sj, duij);   // entropy weighting
      CHECK(wi >= 0.0 and wi <= 1.0);

      DepsDt_thread(nodeListi, i) += wi*duij;
      DepsDt_thread(nodeListj, j) += (1.0 - wi)*duij*mi/mj;
    }

#pragma omp critical
    {
      DepsDt_thread.threadReduce();
    }
  }

  // Now we can update the energy.
  for (auto nodeListi = 0; nodeListi < numFields; ++nodeListi) {
    const auto n = eps[nodeListi]->numInternalElements();
#pragma omp parallel for
    for (auto i = 0; i < n; ++i) {
      eps(nodeListi, i) += DepsDt(nodeListi, i)*multiplier;
    }
  }
}

//------------------------------------------------------------------------------
// Equivalence operator.
//------------------------------------------------------------------------------
template<typename Dimension>
bool
SpecificThermalEnergyPolicy<Dimension>::
operator==(const UpdatePolicyBase<Dimension>& rhs) const {

  // We're only equal if the other guy is also an increment operator.
  const SpecificThermalEnergyPolicy<Dimension>* rhsPtr = dynamic_cast<const SpecificThermalEnergyPolicy<Dimension>*>(&rhs);
  if (rhsPtr == 0) {
    return false;
  } else {
    return true;
  }
}

}

