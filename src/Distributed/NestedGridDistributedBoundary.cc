//---------------------------------Spheral++----------------------------------//
// NestedGridDistributedBoundary -- Implementation of the Distributed Boundary
// condition for use with NestedGridNeighbor based NodeLists.
//
// Created by JMO, Mon Aug 27 22:21:17 PDT 2001
//----------------------------------------------------------------------------//
#include <list>
#include <algorithm>

#include "mpi.h"
#include "TAU.h"

#include "DistributedBoundary.hh"
#include "NestedGridDistributedBoundary.hh"
#include "Boundary/Boundary.hh"
#include "Neighbor/NestedGridNeighbor.hh"
#include "Neighbor/GridCellIndex.hh"
#include "DataBase/DataBase.hh"
#include "Field/FieldList.hh"
#include "Utilities/testBoxIntersection.hh"
#include "Utilities/removeElements.hh"
#include "DBC.hh"
#include "waitAllWithDeadlockDetection.hh"

using namespace std;

namespace Spheral {
namespace BoundarySpace {

using NodeSpace::NodeList;
using NeighborSpace::NestedGridNeighbor;
using NeighborSpace::GridCellIndex;
using DataBaseSpace::DataBase;
using FieldSpace::Field;
using FieldSpace::FieldList;

// Static initialization of singleton instance.
template <typename Dimension>
NestedGridDistributedBoundary<Dimension>*
NestedGridDistributedBoundary<Dimension>::mInstance = 0;

//------------------------------------------------------------------------------
// Singleton instance method.
//------------------------------------------------------------------------------
template<typename Dimension>
NestedGridDistributedBoundary<Dimension>&
NestedGridDistributedBoundary<Dimension>::
instance() {
  if (mInstance == 0) {
    mInstance = new NestedGridDistributedBoundary();
  } // end if
  return *mInstance;
}

template<typename Dimension>
NestedGridDistributedBoundary<Dimension>*
NestedGridDistributedBoundary<Dimension>::
instancePtr() {
  return &(instance());
}

//------------------------------------------------------------------------------
// Default constructor.
//------------------------------------------------------------------------------
template<typename Dimension>
NestedGridDistributedBoundary<Dimension>::NestedGridDistributedBoundary():
  DistributedBoundary<Dimension>(),
  mOccupiedGridCells(this->numDomains()),
  mGridCellInfluenceRadius(1),
  mBoxCulling(true) {
}

//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
template<typename Dimension>
NestedGridDistributedBoundary<Dimension>::~NestedGridDistributedBoundary() {
}

//------------------------------------------------------------------------------
// Read-only access to the list of occupied grid cells.
//------------------------------------------------------------------------------
template<typename Dimension>
const vector< vector< vector< GridCellIndex<Dimension> > > >& 
NestedGridDistributedBoundary<Dimension>::
occupiedGridCells() const {
  return mOccupiedGridCells;
}

//------------------------------------------------------------------------------
// Determine the max number of occupied grid levels for all NodeLists in a
// DataBase.
//------------------------------------------------------------------------------
template<typename Dimension>
int
NestedGridDistributedBoundary<Dimension>::
maxNumGridLevels(const DataBase<Dimension>& dataBase) const {

  int numGridLevels = 0;

  // Loop over the NodeLists in the DataBase.
  for (typename DataBase<Dimension>::ConstNodeListIterator nodeListItr = dataBase.nodeListBegin();
       nodeListItr != dataBase.nodeListEnd();
       ++nodeListItr) {

    NestedGridNeighbor<Dimension>& neighbor = getNestedGridNeighbor(*nodeListItr);
    numGridLevels = max(numGridLevels, neighbor.numGridLevels());
  }

  ENSURE(numGridLevels > 0);
  return numGridLevels;

//   // Find the global maximum of the number of grid levels.
//   int globalNumGridLevels;
//   MPI_Allreduce(&numGridLevels, &globalNumGridLevels, 1, MPI_INT, MPI_MAX, mCommunicator);

//   CHECK(globalNumGridLevels > 0);
//   return globalNumGridLevels;
}

//------------------------------------------------------------------------------
// Set the grid cell influence radius for all Neighbors to the desired value,
// returning the original value.
//------------------------------------------------------------------------------
template<typename Dimension>
int
NestedGridDistributedBoundary<Dimension>::
setGridCellInfluenceRadius(DataBase<Dimension>& dataBase,
                           const int newGridCellInfluenceRadius) const {

  // TAU timers.
  TAU_PROFILE("NestedGridDistributedBoundary::", "setGridCellInflueceRadius", TAU_USER);

  int result = 0;

  // Loop over the NodeLists in the DataBase.
  for (typename DataBase<Dimension>::ConstNodeListIterator nodeListItr = dataBase.nodeListBegin();
       nodeListItr != dataBase.nodeListEnd();
       ++nodeListItr) {
    NestedGridNeighbor<Dimension>& neighbor = getNestedGridNeighbor(*nodeListItr);
    if (result == 0) {
      result = neighbor.gridCellInfluenceRadius();
    } else {
      VERIFY(result == neighbor.gridCellInfluenceRadius());
    }
    if (result != newGridCellInfluenceRadius) {
      neighbor.gridCellInfluenceRadius(newGridCellInfluenceRadius);
      neighbor.updateNodes();
    }
  }

  ENSURE(result > 0);
  return result;
}

//------------------------------------------------------------------------------
// Create a list of the occupied grid cells flattened across all NodeLists
// in the given DataBase.
//------------------------------------------------------------------------------
template<typename Dimension>
void
NestedGridDistributedBoundary<Dimension>::
flattenOccupiedGridCells(const DataBase<Dimension>& dataBase,
			 vector< vector< GridCellIndex<Dimension> > >& gridCells) const {

  // TAU timers.
  TAU_PROFILE("NestedGridDistributedBoundary", "::flattenOccupiedGridCells", TAU_USER);

  // Erase any prior information in list of occupied grid cells.
  const int numGridLevels = maxNumGridLevels(dataBase);
  gridCells = vector< vector< GridCellIndex<Dimension> > >(numGridLevels);

  // Determine how to size the result, and do it.
  vector<int> numGridCells(numGridLevels, 0);
  for (typename DataBase<Dimension>::ConstNodeListIterator nodeListItr = dataBase.nodeListBegin();
       nodeListItr != dataBase.nodeListEnd();
       ++nodeListItr) {
    const NestedGridNeighbor<Dimension>& neighbor = getNestedGridNeighbor(*nodeListItr);
    const vector< vector< GridCellIndex<Dimension> > >& occupiedGridCells = neighbor.occupiedGridCells();
    for (int gridLevel = 0; gridLevel != occupiedGridCells.size(); ++gridLevel) {
      CHECK(gridLevel < numGridLevels);
      numGridCells[gridLevel] += occupiedGridCells[gridLevel].size();
    }
  }
  for (int gridLevel = 0; gridLevel != numGridLevels; ++gridLevel) {
    CHECK(numGridCells[gridLevel] >= 0 and
          numGridCells[gridLevel] < 1000000);
    gridCells[gridLevel].reserve(numGridCells[gridLevel]);
  }

  // Loop over the NodeLists in the DataBase.
  for (typename DataBase<Dimension>::ConstNodeListIterator nodeListItr = dataBase.nodeListBegin();
       nodeListItr != dataBase.nodeListEnd();
       ++nodeListItr) {

    // Get the NestedGridNeighbor and set of occupied grid cells for this NodeList.
    const NestedGridNeighbor<Dimension>& neighbor = getNestedGridNeighbor(*nodeListItr);
    const vector< vector< GridCellIndex<Dimension> > >& occupiedGridCells = neighbor.occupiedGridCells();

    // Insert these grid cell indicies into the (possibly redundant) set of occupied grid cells.
    CHECK(occupiedGridCells.size() <= numGridLevels);
    for (int gridLevel = 0; gridLevel != occupiedGridCells.size(); ++gridLevel) {
      CHECK(gridLevel < gridCells.size());
      CHECK(gridCells[gridLevel].capacity() >= gridCells[gridLevel].size() + occupiedGridCells[gridLevel].size());
      for (typename vector< GridCellIndex<Dimension> >::const_iterator gridCellItr = occupiedGridCells[gridLevel].begin();
	   gridCellItr != occupiedGridCells[gridLevel].end();
	   ++gridCellItr) gridCells[gridLevel].push_back(*gridCellItr);
    }
  }

  // Now make sure that the result gridCells are unique.
  for (int gridLevel = 0; gridLevel != numGridLevels; ++gridLevel) {
    sort(gridCells[gridLevel].begin(), gridCells[gridLevel].end());
    typename vector<GridCellIndex<Dimension> >::iterator uniqueItr = unique(gridCells[gridLevel].begin(), 
                                                                            gridCells[gridLevel].end());
    gridCells[gridLevel].erase(uniqueItr, gridCells[gridLevel].end());
  }

}

//------------------------------------------------------------------------------
// Pack the occupied grid cell set into a C style array syntax for messaging
// with MPI.
//------------------------------------------------------------------------------
template<typename Dimension>
void
NestedGridDistributedBoundary<Dimension>::
packGridCellIndicies(const vector< vector< GridCellIndex<Dimension> > >& gridCellSet,
		     vector<int>& packedGridCellIndicies) const {

  // TAU timers.
  TAU_PROFILE("NestedGridDistributedBoundary::", "packGridCellIndicies", TAU_USER);

  int packedIndex = 0;
  for (int gridLevel = 0; gridLevel != gridCellSet.size(); ++gridLevel) {
    for (typename vector< GridCellIndex<Dimension> >::const_iterator gridCellItr = gridCellSet[gridLevel].begin();
	 gridCellItr != gridCellSet[gridLevel].end();
	 ++gridCellItr) {
      for (int i = 0; i != Dimension::nDim; ++i) {
	CHECK(packedIndex < packedGridCellIndicies.size());
	packedGridCellIndicies[packedIndex] = (*gridCellItr)(i);
	++packedIndex;
      }
    }
  }
}

//------------------------------------------------------------------------------
// Unpack the occupied grid cell set from C style array syntax to the more
// sensible set of occupied grid cells.
//------------------------------------------------------------------------------
template<typename Dimension>
void
NestedGridDistributedBoundary<Dimension>::
unpackGridCellIndicies(const vector<int>& packedGridCellIndicies,
		       const vector<int>& gridCellDimension,
		       vector< vector< GridCellIndex<Dimension> > >& gridCellSet) const {

  // TAU timers.
  TAU_PROFILE("NestedGridDistributedBoundary::", "unpackGridCellIndicies", TAU_USER);

  // Pre-conditions.
  BEGIN_CONTRACT_SCOPE;
  {
    REQUIRE(fmod(packedGridCellIndicies.size(), (double) Dimension::nDim) == 0);
    int checkcount = 0;
    for (vector<int>::const_iterator itr = gridCellDimension.begin();
         itr != gridCellDimension.end();
         ++itr) checkcount = checkcount + *itr;
    REQUIRE(checkcount*Dimension::nDim == packedGridCellIndicies.size());
  }
  END_CONTRACT_SCOPE;

  // Make sure the gridCellSet is appropriately dimensioned.
  const int numGridLevels = gridCellDimension.size();
  gridCellSet.resize(numGridLevels);

  int packedIndex = 0;
  for (int gridLevel = 0; gridLevel != numGridLevels; ++gridLevel) {

    // Erase any old info in the grid cell set.
    gridCellSet[gridLevel] = vector< GridCellIndex<Dimension> >();
    CHECK(gridCellDimension[gridLevel] >= 0 and
          gridCellDimension[gridLevel] < 10000000);
    gridCellSet[gridLevel].reserve(gridCellDimension[gridLevel]);

    // Loop over the number of grid cell indicies on this grid level.
    for (int gcIndex = 0; gcIndex != gridCellDimension[gridLevel]; ++gcIndex) {
      CHECK(packedIndex + Dimension::nDim <= packedGridCellIndicies.size());

      // Build the next GridCellIndex from the packed info.
      GridCellIndex<Dimension> gc;
      for (int i = 0; i < Dimension::nDim; ++i) {
	gc(i) = packedGridCellIndicies[packedIndex];
	++packedIndex;
      }

      // Insert this GridCellIndex into the set.
      gridCellSet[gridLevel].push_back(gc);
    }
  }
}

//------------------------------------------------------------------------------
// Distribute the flattened set of occupied grid cells to all domains.
//------------------------------------------------------------------------------
template<typename Dimension>
void
NestedGridDistributedBoundary<Dimension>::
distributeOccupiedGridCells() {
  TAU_PROFILE("NestedGridDistributedBoundary", "::distributeOccupiedGridCells", TAU_USER);
  TAU_PROFILE_TIMER(TimeNDBcastSizes, "NestedGridDistributedBoundary", "::distributeOccupiedGridCells : broadcast sizes", TAU_USER);
  TAU_PROFILE_TIMER(TimeNDBcastCells, "NestedGridDistributedBoundary", "::distributeOccupiedGridCells : broadcast cells", TAU_USER);

  // This processor's ID.
  int procID = this->domainID();
  int numProcs = this->numDomains();
  CHECK(procID < numProcs);

  // Prepare the storage for the number of occupied grid cells on each domain.
  const int numGridLevels = mOccupiedGridCells[procID].size();
  vector< vector<int> > occupiedGridCellDimensions(numProcs);
  for (int i = 0; i != numProcs; ++i) occupiedGridCellDimensions[i].resize(numGridLevels);

  // Count up the dimensions of our occupied grid cell set info.
  for (int gridLevel = 0; gridLevel < numGridLevels; ++gridLevel) {
    occupiedGridCellDimensions[procID][gridLevel] = mOccupiedGridCells[procID][gridLevel].size();
  }

  // Broadcast the dimensions to everyone.
  TAU_PROFILE_START(TimeNDBcastSizes);
  for (int sendDomain = 0; sendDomain != numProcs; ++sendDomain) {
    MPI_Bcast(&occupiedGridCellDimensions[sendDomain].front(), numGridLevels, MPI_INT, sendDomain, mCommunicator);
  }
  TAU_PROFILE_STOP(TimeNDBcastSizes);

  // OK, now everyone knows the dimensions of the occupied grid cell information
  // they will be getting from all other processes, so go ahead and distribute the
  // occupied grid cell information.
  TAU_PROFILE_START(TimeNDBcastCells);
  for (int sendDomain = 0; sendDomain != numProcs; ++sendDomain) {

    // Determine the size of message for this send domain, and allocate space for 
    // the packed grid cell information.
    int totalNumGridCells = 0;
    for (int gridLevel = 0; gridLevel < numGridLevels; ++gridLevel)
      totalNumGridCells += occupiedGridCellDimensions[sendDomain][gridLevel];
    vector<int> packedGridCellIndicies(totalNumGridCells*Dimension::nDim);
    if (procID == sendDomain) packGridCellIndicies(mOccupiedGridCells[sendDomain], packedGridCellIndicies);

    // Broadcast the sucker and unpack the data.
    MPI_Bcast(&packedGridCellIndicies.front(), totalNumGridCells*Dimension::nDim, MPI_INT, sendDomain, mCommunicator);
    if (procID != sendDomain) unpackGridCellIndicies(packedGridCellIndicies, occupiedGridCellDimensions[sendDomain], mOccupiedGridCells[sendDomain]);
  }
  TAU_PROFILE_STOP(TimeNDBcastCells);

}

//------------------------------------------------------------------------------
// The flag determing if we're applying box culling algorithm or not.
//------------------------------------------------------------------------------
template<typename Dimension>
bool
NestedGridDistributedBoundary<Dimension>::
boxCulling() const {
  return mBoxCulling;
}

template<typename Dimension>
void
NestedGridDistributedBoundary<Dimension>::
boxCulling(const bool x) {
  mBoxCulling = x;
}

//------------------------------------------------------------------------------
// The grid cell influence radius to use for building ghost nodes.
//------------------------------------------------------------------------------
template<typename Dimension>
int
NestedGridDistributedBoundary<Dimension>::
gridCellInfluenceRadius() const {
  return mGridCellInfluenceRadius;
}

template<typename Dimension>
void
NestedGridDistributedBoundary<Dimension>::
gridCellInfluenceRadius(const int x) {
  mGridCellInfluenceRadius = x;
}

//------------------------------------------------------------------------------
// Set the ghost nodes for the given DataBase.
//------------------------------------------------------------------------------
template<typename Dimension>
void
NestedGridDistributedBoundary<Dimension>::
setAllGhostNodes(DataBase<Dimension>& dataBase) {

  // TAU timers.
  TAU_PROFILE("NestedGridDistributedBoundary", "::setAllGhostNodes", TAU_USER);
  TAU_PROFILE_TIMER(TimeNDSetGhost, "NestedGridDistributedBoundary", "::setAllGhostNodes : Set ghost nodes", TAU_USER);
  TAU_PROFILE_TIMER(TimeNDReset, "NestedGridDistributedBoundary", "::setAllGhostNodes : Clear existing info", TAU_USER);
  TAU_PROFILE_TIMER(TimeNDFlatten, "NestedGridDistributedBoundary", "::setAllGhostNodes : Flatten occupied gridcells", TAU_USER);
  TAU_PROFILE_TIMER(TimeNDDistributeGridCells, "NestedGridDistributedBoundary", "::setAllGhostNodes : Distribute occupied gridcells ", TAU_USER);
  TAU_PROFILE_TIMER(TimeNDBuildSend, "NestedGridDistributedBoundary", "::setAllGhostNodes : Build send nodes ", TAU_USER);
  TAU_PROFILE_TIMER(TimeNDBuildRecv, "NestedGridDistributedBoundary", "::setAllGhostNodes : Build recv & ghost nodes", TAU_USER);
  TAU_PROFILE_TIMER(TimeNDExchangeMinimal, "NestedGridDistributedBoundary", "::setAllGhostNodes : Exchange r, H", TAU_USER);

  // This processor's ID.
  int procID = this->domainID();
  int numProcs = this->numDomains();
  CHECK(procID < numProcs);

  // Clear out the existing communication map for the given database.
  TAU_PROFILE_START(TimeNDReset);
//   const int oldGridCellRadius = setGridCellInfluenceRadius(dataBase,
//                                                            mGridCellInfluenceRadius);
  reset(dataBase);
  TAU_PROFILE_STOP(TimeNDReset);

  // JMO 2002-10-28 - If we have periodic boundaries present, we have to allow those
  // boundaries to be set first, and then communicate the resulting ghost nodes
  // as needed here.  Therefore, I have to suspend the following requirement that
  // the distributed boundary be the first in the list (which was done for
  // efficiency).
//   for (typename DataBase<Dimension>::NodeListIterator nodeListItr = dataBase.nodeListBegin();
//        nodeListItr < dataBase.nodeListEnd();
//        ++nodeListItr) CHECK((*nodeListItr)->numGhostNodes() == 0);

  // Begin by creating the flattened list of occupied grid cells for this process.
  // The flattening here is across NodeLists, so we are making an amalgamated list
  // of grid cells occupied by nodes from any NodeList on this domain.
  TAU_PROFILE_START(TimeNDFlatten);
  CHECK(mOccupiedGridCells.size() == numProcs);
  flattenOccupiedGridCells(dataBase, mOccupiedGridCells[procID]);
  TAU_PROFILE_STOP(TimeNDFlatten);

  // Distribute the complete set of occupied grid cells for all domains to all processes.
  TAU_PROFILE_START(TimeNDDistributeGridCells);
  distributeOccupiedGridCells();
  TAU_PROFILE_STOP(TimeNDDistributeGridCells);

  // Each processor now knows the occupied grid cells for all other processors.
  // Figure out which of our nodes need to be sent to each processor.
  TAU_PROFILE_START(TimeNDBuildSend);
  buildSendNodes(dataBase);
  TAU_PROFILE_STOP(TimeNDBuildSend);

  // Tell everyone else the send nodes we have for them.
  TAU_PROFILE_START(TimeNDBuildRecv);
  this->buildReceiveAndGhostNodes(dataBase);
  TAU_PROFILE_STOP(TimeNDBuildRecv);

  // Exchange the minimal info we expect for the NodeLists: mass, position, H
  TAU_PROFILE_START(TimeNDExchangeMinimal);
//   setGridCellInfluenceRadius(dataBase, oldGridCellRadius);
  for (typename DataBase<Dimension>::NodeListIterator nodeListItr = dataBase.nodeListBegin();
       nodeListItr != dataBase.nodeListEnd();
       ++nodeListItr) {
//     if (communicatedNodeList(**nodeListItr)) updateGhostNodes(**nodeListItr);
    updateGhostNodes(**nodeListItr);
  }
  TAU_PROFILE_STOP(TimeNDExchangeMinimal);

  // And that's all.  At this point each domain knows who it it sending nodes to,
  // what nodes to send them, who it is receiving nodes from, and what nodes it
  // be receiving.
}

//------------------------------------------------------------------------------
// Clear any existing info in preparation to build from scratch again.
//------------------------------------------------------------------------------
template<typename Dimension>
void
NestedGridDistributedBoundary<Dimension>::
reset(const DataBase<Dimension>& dataBase) {

  // Call the ancestor method.
  DistributedBoundary<Dimension>::reset(dataBase);

  // Clear our own internal state.
  int numProcs = this->numDomains();
  for (int rank = 0; rank < numProcs; ++rank) {
    mOccupiedGridCells[rank] = vector< vector< GridCellIndex<Dimension> > >();
  }
}

//------------------------------------------------------------------------------
// Each process should have the set of occupied grid cells for all other domains
// at this point.  Now we use this information to determine which processes this
// domain should be sending nodes to.
//------------------------------------------------------------------------------
template<typename Dimension>
void
NestedGridDistributedBoundary<Dimension>::
buildSendNodes(const DataBase<Dimension>& dataBase) {

  // TAU timers.
  TAU_PROFILE("NestedGridDistributedBoundary::", "buildSendNodes", TAU_USER);

  // This processor's ID.
  int procID = this->domainID();
  int numProcs = this->numDomains();
  CHECK(procID < numProcs);

  const int numGridLevels = mOccupiedGridCells[0].size();
  CHECK(mOccupiedGridCells[procID].size() == numGridLevels);

  // Compute and distribute the min and max node extents for each domain.
  // This is used to cull the nodes we're going to send to other domains.
  vector<Vector> centroidDomain(numProcs);
  vector<double> radiusNodesDomain(numProcs);
  vector<double> radiusSampleDomain(numProcs);
  vector<Vector> xminNodesDomain(numProcs);
  vector<Vector> xmaxNodesDomain(numProcs);
  vector<Vector> xminSampleDomain(numProcs);
  vector<Vector> xmaxSampleDomain(numProcs);
  if (mBoxCulling) {
    dataBase.localSamplingBoundingVolume(centroidDomain[procID], 
                                         radiusNodesDomain[procID],
                                         radiusSampleDomain[procID],
                                         xminNodesDomain[procID],
                                         xmaxNodesDomain[procID],
                                         xminSampleDomain[procID],
                                         xmaxSampleDomain[procID]);
    vector<char> localBuffer;
    packElement(centroidDomain[procID], localBuffer);
    packElement(radiusNodesDomain[procID], localBuffer);
    packElement(radiusNodesDomain[procID], localBuffer);
    packElement(xminNodesDomain[procID], localBuffer);
    packElement(xmaxNodesDomain[procID], localBuffer);
    packElement(xminSampleDomain[procID], localBuffer);
    packElement(xmaxSampleDomain[procID], localBuffer);
    BEGIN_CONTRACT_SCOPE;
    {
      int tmp = localBuffer.size();
      int globalMinSize;
      MPI_Allreduce(&tmp, &globalMinSize, 1, MPI_INT, MPI_MIN, mCommunicator);
      CHECK(globalMinSize == localBuffer.size());
    }
    END_CONTRACT_SCOPE;
    if (localBuffer.size() > 0) {
      for (int sendProc = 0; sendProc != numProcs; ++sendProc) {
	vector<char> buffer = localBuffer;
	MPI_Bcast(&buffer.front(), buffer.size(), MPI_CHAR, sendProc, mCommunicator);
	vector<char>::const_iterator itr = buffer.begin();
	unpackElement(centroidDomain[sendProc], itr, buffer.end());
	unpackElement(radiusNodesDomain[sendProc], itr, buffer.end());
	unpackElement(radiusSampleDomain[sendProc], itr, buffer.end());
	unpackElement(xminNodesDomain[sendProc], itr, buffer.end());
	unpackElement(xmaxNodesDomain[sendProc], itr, buffer.end());
	unpackElement(xminSampleDomain[sendProc], itr, buffer.end());
	unpackElement(xmaxSampleDomain[sendProc], itr, buffer.end());
	CHECK(itr == buffer.end());
      }
    }
  }

  // Iterate over the other domains, and examine their occupied grid cells
  // one at a time.
  FieldList<Dimension, SymTensor> Hinverse = dataBase.newGlobalFieldList(SymTensor::zero, "H inverse");
  dataBase.globalHinverse(Hinverse);
  for (int neighborProc = 0; neighborProc != numProcs; ++neighborProc) {
    CHECK(mOccupiedGridCells[neighborProc].size() == numGridLevels);

    // Don't bother if the neighbor domain is us!
    if (neighborProc != procID) {

      // Iterate over our NodeLists, and extract their NestedGridNeighbor.
      int nodeListi = 0;
      for (typename DataBase<Dimension>::ConstNodeListIterator nodeListItr = dataBase.nodeListBegin();
           nodeListItr != dataBase.nodeListEnd();
           ++nodeListItr, ++nodeListi) {
        const NestedGridNeighbor<Dimension>& neighbor = getNestedGridNeighbor(*nodeListItr);

        // Loop over the grid levels.
        for (int gridLevel = 0; gridLevel != numGridLevels; ++gridLevel) {

          // Loop over the grid cells the neighbor domain occupies on this grid level.
          for (typename vector< GridCellIndex<Dimension> >::const_iterator gcItr = mOccupiedGridCells[neighborProc][gridLevel].begin();
               gcItr != mOccupiedGridCells[neighborProc][gridLevel].end();
               ++gcItr) {

            // Find the nodes from the current NodeList that interact with this gridcell,
            // and copy them to the send set for the neighbor domain.
            const vector<int> sendNodes = neighbor.findNestedNeighbors(*gcItr, gridLevel);
            if (sendNodes.size() > 0) {
              DomainBoundaryNodes& domainNodes = this->openDomainBoundaryNodes(&(**nodeListItr), neighborProc);
              copy(sendNodes.begin(), sendNodes.end(), back_inserter(domainNodes.sendNodes));
            }
          }
        }

        // Did we find any send nodes from this NodeList to the neighboring domain?
        if (this->nodeListSharedWithDomain(**nodeListItr, neighborProc)) {

          // Remove any duplicate nodes that were created.
          vector<int>& sendNodes = this->accessDomainBoundaryNodes(**nodeListItr, neighborProc).sendNodes;
	  CHECK(sendNodes.size() > 0);
          sort(sendNodes.begin(), sendNodes.end());
          sendNodes.erase(unique(sendNodes.begin(), sendNodes.end()), sendNodes.end());
	  CHECK(sendNodes.size() > 0);

          // If we're box culling, apply that test to reject more nodes as well.
          if (mBoxCulling) {
            const Field<Dimension, Vector>& positions = (**nodeListItr).positions();
            const Field<Dimension, Vector>& extents = neighbor.nodeExtentField();
	    const Field<Dimension, SymTensor>& Hinv = *Hinverse[nodeListi];
            vector<int> indiciesToKill;
            for (int k = 0; k != sendNodes.size(); ++k) {
              const int i = sendNodes[k];
              const Vector& xi = positions(i);
              const Vector& extenti = extents(i);
	      const Vector dr = xi - centroidDomain[neighborProc];
              const double drMag = dr.magnitude();
	      const Vector drUnit = dr.unitVector();
              const Vector xmini = xi - extenti;
              const Vector xmaxi = xi + extenti;
	      const Scalar hi = (Hinv(i)*drUnit).magnitude();
              const bool interacting = (drMag - 2.0*hi < radiusNodesDomain[neighborProc] or  // I see you
                                        drMag < radiusSampleDomain[neighborProc] or          // You see me
                                        testBoxIntersection(xmini, xmaxi, xminNodesDomain[neighborProc], xmaxNodesDomain[neighborProc]) or // I see you
                                        testBoxIntersection(xi, xi, xminSampleDomain[neighborProc], xmaxSampleDomain[neighborProc]));      // You see me
              if (not interacting) indiciesToKill.push_back(k);
            }

	    // Are we killing all of the send nodes?
	    if (indiciesToKill.size() == sendNodes.size()) {
	      this->removeDomainBoundaryNodes(*nodeListItr, neighborProc);
	    } else {
	      removeElements(sendNodes, indiciesToKill);
	    }

          }
        }
      }
    }
  }
}

}
}

//------------------------------------------------------------------------------
// Explicit instantiation.
//------------------------------------------------------------------------------
#include "Geometry/Dimension.hh"
namespace Spheral {
namespace BoundarySpace {
template class NestedGridDistributedBoundary< Dim<1> >;
template class NestedGridDistributedBoundary< Dim<2> >;
template class NestedGridDistributedBoundary< Dim<3> >;
}
}
