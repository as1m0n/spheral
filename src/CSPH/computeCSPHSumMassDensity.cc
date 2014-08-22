//------------------------------------------------------------------------------
// Compute the CSPH mass density summation.
//------------------------------------------------------------------------------

#include "computeCSPHSumMassDensity.hh"
#include "CSPHUtilities.hh"
#include "Field/FieldList.hh"
#include "Neighbor/ConnectivityMap.hh"
#include "Kernel/TableKernel.hh"
#include "NodeList/NodeList.hh"
#include "Hydro/HydroFieldNames.hh"

namespace Spheral {
namespace CSPHSpace {

using namespace std;
using std::min;
using std::max;
using std::abs;

using FieldSpace::FieldList;
using NeighborSpace::ConnectivityMap;
using KernelSpace::TableKernel;
using NodeSpace::NodeList;
using NodeSpace::FluidNodeList;

template<typename Dimension>
void
computeCSPHSumMassDensity(const ConnectivityMap<Dimension>& connectivityMap,
                          const TableKernel<Dimension>& W,
                          const FieldList<Dimension, typename Dimension::Vector>& position,
                          const FieldList<Dimension, typename Dimension::Scalar>& mass,
                          const FieldList<Dimension, typename Dimension::Scalar>& volume,
                          const FieldList<Dimension, typename Dimension::SymTensor>& H,
                          const FieldList<Dimension, typename Dimension::Scalar>& A0,
                          const FieldList<Dimension, typename Dimension::Scalar>& A,
                          const FieldList<Dimension, typename Dimension::Vector>& B,
                          FieldList<Dimension, typename Dimension::Scalar>& massDensity) {

  // Pre-conditions.
  const size_t numNodeLists = massDensity.size();
  REQUIRE(position.size() == numNodeLists);
  REQUIRE(mass.size() == numNodeLists);
  REQUIRE(volume.size() == numNodeLists);
  REQUIRE(H.size() == numNodeLists);
  REQUIRE(A0.size() == numNodeLists);
  REQUIRE(A.size() == numNodeLists);
  REQUIRE(B.size() == numNodeLists);

  typedef typename Dimension::Scalar Scalar;
  typedef typename Dimension::Vector Vector;
  typedef typename Dimension::Tensor Tensor;
  typedef typename Dimension::SymTensor SymTensor;

  // Zero out the result, and prepare a FieldList to hold the effective volume.
  massDensity = 0.0;
  FieldList<Dimension, Scalar> Veff(FieldSpace::Copy);
  for (size_t nodeListi = 0; nodeListi != numNodeLists; ++nodeListi) {
    const NodeList<Dimension>& nodeList = massDensity[nodeListi]->nodeList();
    Veff.appendNewField("effective volume", nodeList, 0.0);
  }

  // Walk the FluidNodeLists.
  for (size_t nodeListi = 0; nodeListi != numNodeLists; ++nodeListi) {
    const FluidNodeList<Dimension>& nodeList = dynamic_cast<const FluidNodeList<Dimension>&>(massDensity[nodeListi]->nodeList());
    const int firstGhostNodei = nodeList.firstGhostNode();
    const Scalar rhoMin = nodeList.rhoMin();
    const Scalar rhoMax = nodeList.rhoMax();

    // Iterate over the nodes in this node list.
    for (typename ConnectivityMap<Dimension>::const_iterator iItr = connectivityMap.begin(nodeListi);
         iItr != connectivityMap.end(nodeListi);
         ++iItr) {
      const int i = *iItr;

      // Get the state for node i.
      const Vector& ri = position(nodeListi, i);
      const Scalar mi = mass(nodeListi, i);
      const Scalar& Vi = volume(nodeListi, i);
      const SymTensor& Hi = H(nodeListi, i);
      const Scalar A0i = A0(nodeListi, i);
      const Scalar Ai = A(nodeListi, i);
      const Vector& Bi = B(nodeListi, i);
      const Scalar Hdeti = Hi.Determinant();

      // Get the neighbors for this node (in this NodeList).  We use the approximation here
      // that nodes from other NodeLists do not contribute to the density of this one.
      const vector<int>& connectivity = connectivityMap.connectivityForNode(nodeListi, i)[nodeListi];
      for (vector<int>::const_iterator jItr = connectivity.begin();
           jItr != connectivity.end();
           ++jItr) {
        const int j = *jItr;

        // Check if this node pair has already been calculated.
        if (connectivityMap.calculatePairInteraction(nodeListi, i, 
                                                     nodeListi, j,
                                                     firstGhostNodei)) {
          const Vector& rj = position(nodeListi, j);
          const Scalar mj = mass(nodeListi, j);
          const Scalar Vj = volume(nodeListi, j);
          const SymTensor& Hj = H(nodeListi, j);
          const Scalar A0j = A0(nodeListi, j);
          const Scalar Aj = A(nodeListi, j);
          const Vector& Bj = B(nodeListi, j);
          const Scalar Hdetj = Hj.Determinant();

          // Kernel weighting and gradient.
          const Vector rij = ri - rj;
          const Scalar etai = (Hi*rij).magnitude();
          const Scalar etaj = (Hj*rij).magnitude();
          const Scalar Wi = CSPHKernel(W, rij, etaj, Hdetj, Ai, Bi);
          const Scalar Wj = CSPHKernel(W, rij, etai, Hdeti, Aj, Bj);
          // const Scalar Wi = CSPHKernel(W, rij, etaj, Hdetj, A0i, Vector::zero);
          // const Scalar Wj = CSPHKernel(W, rij, etai, Hdeti, A0j, Vector::zero);
          // const Scalar Wi = W.kernelValue(etaj, Hdetj);
          // const Scalar Wj = W.kernelValue(etai, Hdeti);

          // Sum the pair-wise contributions.
          Veff(nodeListi, i) += Vj*Vj*Wi;
          massDensity(nodeListi, i) += Vj*mj*Wi;

          Veff(nodeListi, j) += Vi*Vi*Wj;
          massDensity(nodeListi, j) += Vi*mi*Wj;
        }
      }
      
      // Finalize the density for node i.
      const Scalar W0 = CSPHKernel(W, Vector::zero, 0.0, Hdeti, Ai, Bi);
      massDensity(nodeListi, i) = max(rhoMin, 
                                      min(rhoMax,
                                          (massDensity(nodeListi, i) + Vi*mi*W0) * 
                                          safeInv(A0i*(Veff(nodeListi, i) + Vi*Vi*W0))));
      CHECK(massDensity(nodeListi, i) > 0.0);
    }
  }
}

}
}

