//---------------------------------Spheral++----------------------------------//
// InflowOutflowBoundary -- creates inflow ghost images, which become internal nodes
// as they cross the specified boundary plane.
//----------------------------------------------------------------------------//
#include "FileIO/FileIO.hh"
#include "Boundary/Boundary.hh"
#include "Boundary/findNodesTouchingThroughPlanes.hh"
#include "Boundary/mapPositionThroughPlanes.hh"
#include "Field/FieldList.hh"
#include "Field/Field.hh"
#include "Field/FieldBase.hh"
#include "Hydro/HydroFieldNames.hh"
#include "Utilities/allReduce.hh"
#include "Utilities/planarReflectingOperator.hh"
#include "Utilities/DBC.hh"

#include "Boundary/InflowOutflowBoundary.hh"
#include "Boundary/ConstantBoundaryUtilities.hh"

#include "boost/lexical_cast.hpp"
#include "boost/algorithm/string.hpp"

using std::vector;
using std::map;
using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::min;
using std::max;
using std::abs;

namespace Spheral {

namespace {

//------------------------------------------------------------------------------
// Clear the values in the storage
//------------------------------------------------------------------------------
void
clearValues(std::map<std::string, std::vector<char>>& values) {
  for (auto& pairvals: values) pairvals.second.clear();
}

}

//------------------------------------------------------------------------------
// Construct with the given NodeList and plane.
//------------------------------------------------------------------------------
template<typename Dimension>
InflowOutflowBoundary<Dimension>::
InflowOutflowBoundary(DataBase<Dimension>& dataBase,
                      const GeomPlane<Dimension>& plane,
                      const bool empty):
  Boundary<Dimension>(),
  Physics<Dimension>(),
  mDataBase(dataBase),
  mPlane(plane),
  mBoundaryCount(dataBase.numNodeLists()),
  mDT(1e100),
  mActive(false),
  mEmpty(empty),
  mNumInflowNodes(),
  mXmin(),
  mBufferedValues(),
  mRestart(registerWithRestart(*this)) {
}

//------------------------------------------------------------------------------
// Destructor.
//------------------------------------------------------------------------------
template<typename Dimension>
InflowOutflowBoundary<Dimension>::~InflowOutflowBoundary() {
}

//------------------------------------------------------------------------------
// setGhostNodes
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::
setGhostNodes(NodeList<Dimension>& nodeList) {
  this->addNodeList(nodeList);

  if (mActive) {
    auto& boundaryNodes = this->accessBoundaryNodes(nodeList);
    auto& cNodes = boundaryNodes.controlNodes;
    auto& gNodes = boundaryNodes.ghostNodes;
    const auto currentNumGhostNodes = nodeList.numGhostNodes();
    const auto firstNewGhostNode = nodeList.numNodes();
    // cerr << "Allocating new ghost nodes " << firstNewGhostNode << " -- " << (firstNewGhostNode + mNumInflowNodes[nodeList]) << endl;
    
    // Use the planar boundary to find the set of points that interact with
    // the entrance plane.  We make these the control nodes.
    cNodes = findNodesTouchingThroughPlanes(nodeList, mPlane, mPlane);

    // Create the ghost nodes.
    nodeList.numGhostNodes(currentNumGhostNodes + mNumInflowNodes[nodeList.name()]);
    gNodes = vector<int>(mNumInflowNodes[nodeList.name()]);
    for (auto i = 0; i < mNumInflowNodes[nodeList.name()]; ++i) gNodes[i] = firstNewGhostNode + i;
    this->updateGhostNodes(nodeList);
  }
}

//------------------------------------------------------------------------------
// updateGhostNodes
// For this boundary this just means moving the ghost points
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::
updateGhostNodes(NodeList<Dimension>& nodeList) {
  if (mActive) {

    // Go ahead and set all the ghost values!
    for (auto fieldItr = nodeList.registeredFieldsBegin();
         fieldItr != nodeList.registeredFieldsEnd();
         ++fieldItr) this->applyGhostBoundary(**fieldItr);
    // this->applyGhostBoundary(pos);
    // this->applyGhostBoundary(nodeList.Hfield());

    auto&       pos = nodeList.positions();
    const auto& boundaryNodes = this->accessBoundaryNodes(nodeList);
    const auto& cNodes = boundaryNodes.controlNodes;
    const auto& gNodes = boundaryNodes.ghostNodes;
    const auto& nhat = mPlane.normal();

    // Find how close the control nodes are to the entrance plane.
    Scalar xmin = 1e100;
    for (const auto i: cNodes) {
      const auto xd = mPlane.signedDistance(pos[i]);
      xmin = std::min(xmin, xd);
    }
    xmin = allReduce(xmin, MPI_MIN, Communicator::communicator());
    // CHECK(xmin >= 0.0);

    // Offset the current ghost points appropriately.
    const auto delta = (xmin < 1e100 ?
                        xmin - mXmin[nodeList.name()] :
                        0.0)*nhat;
    // cerr << " ************> " << xmin << " " << mXmin[nodeList.name()] << " " << nhat << " " << delta << endl;
    for (const auto i: gNodes) pos[i] += delta;

    // for (const auto i: gNodes) {
    //   cerr << " --> " << i << " " << pos(i) << " " << nodeList.Hfield()(i) << " : " << (pos(i) - pos(0)) << endl;
    // }
  }
}

//------------------------------------------------------------------------------
// Apply the boundary condition to fields of different DataTypes.
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::
applyGhostBoundary(FieldBase<Dimension>& field) const {
  if (mActive) {
    resetValues(field, this->ghostNodes(field.nodeList()), mBufferedValues, false);
  }
}

//------------------------------------------------------------------------------
// Provide the setViolationNodes for a NodeList.
// In this case there shouldn't be any violation nodes -- if internal nodes
// are fighting upstream and through the entrance plane, there are bigger
// problems.
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::
setViolationNodes(NodeList<Dimension>& nodeList) {
  this->addNodeList(nodeList);
}

//------------------------------------------------------------------------------
// Provide the updateViolationNodes for a NodeList, correcting positions and 
// H's.
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::
updateViolationNodes(NodeList<Dimension>& nodeList) {
}

//------------------------------------------------------------------------------
// On problem startup we take our snapshot of the state of the points that
// see/interact with the boundary plane.
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::initializeProblemStartup() {

  // Clear any existing data.
  mBufferedValues.clear();

  // Check all NodeLists.
  for (auto itr = mDataBase.nodeListBegin(); itr < mDataBase.nodeListEnd(); ++itr) {
    auto& nodeList = **itr;
    // cerr << "--------------------------------------------------------------------------------" << endl
    //      << nodeList.name() << endl;

    // Use a planar boundary to figure out what sort of nodes are in range of the plane.
    // We use those to create a stencil of the in/outflow conditions.
    const auto& nhat = mPlane.normal();
    auto nodeIDs = findNodesTouchingThroughPlanes(nodeList, mPlane, mPlane, 1.0);
    if (mEmpty) nodeIDs.clear();

    // Remove any ghost nodes from other BCs.
    const auto firstGhostNode = nodeList.firstGhostNode();
    nodeIDs.erase(std::remove_if(nodeIDs.begin(), nodeIDs.end(), [&](const int& x) { return x >= firstGhostNode; }), nodeIDs.end());

    // cerr << "Node IDs: ";
    // std::copy(nodeIDs.begin(), nodeIDs.end(), std::ostream_iterator<int>(std::cerr, " "));
    // cerr << endl;

    // Now take a snapshot of the Fields.
    storeFieldValues(nodeList, nodeIDs, mBufferedValues);

    // Map the snapshot positions to put them outside the boundary.
    const auto ni = nodeIDs.size();
    auto& pos = nodeList.positions();
    const auto poskey = StateBase<Dimension>::key(pos);
    auto positr = mBufferedValues.find(poskey);
    CHECK(positr != mBufferedValues.end());
    auto& posbuf = positr->second;
    auto posvals = extractBufferedValues<Vector>(posbuf);
    const GeomPlane<Dimension> exitPlane(mPlane.point(), -nhat);
    for (auto k = 0; k < ni; ++k) {
      const auto i = nodeIDs[k];
      posvals[k] = mapPositionThroughPlanes(pos[i], mPlane, mPlane);
      // cerr << "  Ghost position: " << i << " @ " << posvals[k] << endl;
    }
    posbuf.clear();
    for (const auto& p: posvals) packElement(p, posbuf);

    // Determine the in/outflow velocity.
    const auto& vel = nodeList.velocity();
    Scalar vinflow = 0.0;
    for (const auto i: nodeIDs) {
      // CHECK(std::abs(vel[i].dot(nhat)/vel[i].magnitude() - 1.0) < 1.0e-5);
      vinflow += vel[i].dot(nhat);
    }
    vinflow = (allReduce(vinflow, MPI_SUM, Communicator::communicator())/
               std::max(1.0e-30, allReduce(double(nodeIDs.size()), MPI_SUM, Communicator::communicator())));  // Negative implies outflow
    // cerr << "Computed inflow velocity: " << vinflow << endl;

    // Figure out a timestep limit such that we don't move more than the ghost
    // node thickness.
    Scalar xmin = 1e100, xmax = -1e100;
    for (const auto i: nodeIDs) {
      const auto xd = mPlane.signedDistance(pos[i]);
      xmin = std::min(xmin, xd);
      xmax = std::max(xmax, xd);
    }
    xmin = allReduce(xmin, MPI_MIN, Communicator::communicator());
    xmax = allReduce(xmax, MPI_MAX, Communicator::communicator());
    mXmin[nodeList.name()] = xmin;
    mDT = std::min(mDT, std::abs(xmax - xmin)/std::max(1e-30, std::abs(vinflow)));   // Protect from negative outflow velocity
    // cerr << "Timestep constraint: " << mDT << endl;

    mNumInflowNodes[nodeList.name()] = nodeIDs.size();
  }
  CHECK2(mNumInflowNodes.size() == mDataBase.numNodeLists(), mNumInflowNodes.size() << " != " <<  mDataBase.numNodeLists());
  mActive = true;
}

//------------------------------------------------------------------------------
// Physics::evaluateDerivatives
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::evaluateDerivatives(const Scalar time,
                                                      const Scalar dt,
                                                      const DataBase<Dimension>& dataBase,
                                                      const State<Dimension>& state,
                                                      StateDerivatives<Dimension>& derivatives) const {
}

//------------------------------------------------------------------------------
// Physics::dt
//------------------------------------------------------------------------------
template<typename Dimension>
typename InflowOutflowBoundary<Dimension>::TimeStepType
InflowOutflowBoundary<Dimension>::dt(const DataBase<Dimension>& dataBase, 
                                     const State<Dimension>& state,
                                     const StateDerivatives<Dimension>& derivs,
                                     const Scalar currentTime) const {
  return TimeStepType(mDT, "InflowOutflowBoundary velocity constraint");
}

//------------------------------------------------------------------------------
// Physics::registerState
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::registerState(DataBase<Dimension>& dataBase,
                                                State<Dimension>& state) {
}

//------------------------------------------------------------------------------
// Physics::registerDerivatives
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::registerDerivatives(DataBase<Dimension>& dataBase,
                                                      StateDerivatives<Dimension>& derivs) {
}

//------------------------------------------------------------------------------
// Physics::finalize
// At the end of a step, any ghost points that have wandered inside the entrance
// plane become internal points.
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::finalize(const Scalar time, 
                                           const Scalar dt,
                                           DataBase<Dimension>& dataBase, 
                                           State<Dimension>& state,
                                           StateDerivatives<Dimension>& derivatives) {

  // First check every NodeList for any inflow or outflow nodes.
  bool altered = false;
  for (auto itr = dataBase.nodeListBegin(); itr < dataBase.nodeListEnd(); ++itr) {
    auto& nodeList = **itr;
    bool nodeListAltered = false;
    // cerr << "--------------------------------------------------------------------------------" << endl
    //      << nodeList.name() << endl;

    // Find any ghost points that are inside the entrance plane now.  These are inflow.
    const auto& gNodes = this->ghostNodes(nodeList);
    auto& pos = nodeList.positions();
    vector<int> insideNodes;
    for (auto i: gNodes) {
      if (mPlane.compare(pos[i]) == -1) insideNodes.push_back(i - gNodes[0]);
    }
    const auto numNew = insideNodes.size();
    if (numNew > 0) {
      nodeListAltered = true;

      // cerr << "Promoting to internal: ";
      // for (auto i: insideNodes) cerr << " : " << gNodes[0] + i << " " << pos[gNodes[0] + i];
      // cerr << endl;

      // Allocate new internal nodes for those we're promoting.
      const auto firstID = nodeList.numInternalNodes();
      nodeList.numInternalNodes(firstID + numNew);

      // Build the map of ghost IDs --> new internal IDs
      vector<int> fromIDs(numNew), toIDs(numNew);
      for (auto k = 0; k < numNew; ++k) {
        fromIDs[k] = gNodes[0] + insideNodes[k] + numNew;
        toIDs[k] = firstID + k;
      }

      // Copy all field values from ghosts to the new internal nodes.
      copyFieldValues(nodeList, fromIDs, toIDs);

      // cerr << "New internal positions:";
      // for (auto k = 0; k < numNew; ++k) cerr << " [" << (firstID + k) << " " << pos[firstID + k] << " " << nodeList.mass()[firstID + k] << " " << nodeList.Hfield()[firstID + k] << "]";
      // cerr << endl;
    }

    // Look for any internal points that have exited through the plane.
    vector<int> outsideNodes;
    const auto ni = nodeList.numInternalNodes();
    for (auto i = 0; i < ni; ++i) {
      if (mPlane.compare(pos[i]) == 1) outsideNodes.push_back(i);
    }

    if (not outsideNodes.empty()) {
      // cerr << "Deleting outflow nodes: ";
      // for (auto i: outsideNodes) cerr << " : " << i << " " << pos[i];
      // cerr << endl;

      // Those nodes get deleted.
      nodeListAltered = true;
      nodeList.deleteNodes(outsideNodes);
    }

    if (nodeListAltered) {
      altered = true;
      nodeList.neighbor().updateNodes();
    }
  }
  altered = (allReduce((altered ? 1 : 0), MPI_MAX, Communicator::communicator()) == 1);

  // If any NodeLists were altered, recompute the boundary conditions.
  if (altered) {
    // Remove any old ghost node information from the NodeLists.
    for (auto nodeListItr = dataBase.fluidNodeListBegin();
         nodeListItr != dataBase.fluidNodeListEnd(); 
         ++nodeListItr) {
      (*nodeListItr)->numGhostNodes(0);
      (*nodeListItr)->neighbor().updateNodes();
    }
    for (auto boundaryItr = this->boundaryBegin(); 
         boundaryItr < this->boundaryEnd();
         ++boundaryItr) (*boundaryItr)->reset(dataBase);

    // Iterate over the boundaries and set their ghost node info.
    for (auto boundaryItr = this->boundaryBegin(); 
         boundaryItr < this->boundaryEnd();
         ++boundaryItr) {
      (*boundaryItr)->setAllGhostNodes(dataBase);
      (*boundaryItr)->finalizeGhostBoundary();
      for (auto nodeListItr = dataBase.fluidNodeListBegin();
           nodeListItr < dataBase.fluidNodeListEnd(); 
           ++nodeListItr) {
        (*nodeListItr)->neighbor().updateNodes();
      }
    }
  }

}

//------------------------------------------------------------------------------
// Return the keys for the Fields we have stored.
//------------------------------------------------------------------------------
template<typename Dimension>
std::vector<std::string>
InflowOutflowBoundary<Dimension>::storedKeys() const {
  vector<string> result;
  for (const auto& pairs: mBufferedValues) result.push_back(pairs.first);
  return result;
}

//------------------------------------------------------------------------------
// Clear the stored values.
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::clearStoredValues() {
  for (auto& stuff: mNumInflowNodes) stuff.second = 0;
  clearValues(mBufferedValues);
}

//------------------------------------------------------------------------------
// Return a unique label for restart.
//------------------------------------------------------------------------------
template<typename Dimension>
std::string
InflowOutflowBoundary<Dimension>::label() const {
  return "InflowOutflowBoundary" + boost::lexical_cast<std::string>(mBoundaryCount);
}

//------------------------------------------------------------------------------
// Dump the current state to the given file.
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::
dumpState(FileIO& file, const string& pathName) const {
  file.write(mActive, pathName + "/active");
  file.write(mBoundaryCount, pathName + "/boundaryCount");

  vector<std::string> keys;
  for (const auto& p: mBufferedValues) {
    keys.push_back(p.first);
    std::string val(p.second.begin(), p.second.end());
    file.write(val, pathName + "/BufferedValues/" + p.first);
  }
  file.write(keys, pathName + "/keys");
}

//------------------------------------------------------------------------------
// Read the state from the given file.
//------------------------------------------------------------------------------
template<typename Dimension>
void
InflowOutflowBoundary<Dimension>::
restoreState(const FileIO& file, const string& pathName)  {
  file.read(mActive, pathName + "/active");
  file.read(mBoundaryCount, pathName + "/boundaryCount");

  vector<std::string> keys;
  file.read(keys, pathName + "/keys");
  mBufferedValues.clear();
  for (const auto key: keys) {
    std::string val;
    file.read(val, pathName + "/BufferedValues/" + key);
    mBufferedValues[key] = vector<char>(val.begin(), val.end());
  }
}

}
