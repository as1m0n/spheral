//---------------------------------Spheral++----------------------------------//
// ConstantBoundary -- Take a snapshot of the state of a set of nodes on 
// NodeList, and create ghost nodes with that state from that time on.
//
// Created by JMO, Fri Nov 21 13:57:34 PST 2003
//
// Modified by:
//----------------------------------------------------------------------------//
#ifndef ConstantBoundary_HH
#define ConstantBoundary_HH

#include "Boundary.hh"
#include "Geometry/GeomPlane.hh"
#include "NodeList/NodeList.hh"
#include "Utilities/registerWithRedistribution.hh"
#include "DataBase/StateBase.hh" // For constructing Field keys.

namespace Spheral {

// Forward declarations.
template<typename Dimension> class NodeList;
template<typename Dimension> class FieldBase;
template<typename Dimension, typename DataType> class Field;
template<typename Dimension, typename DataType> class FieldList;
template<typename Dimension> class DataBase;

template<typename Dimension>
class ConstantBoundary: public Boundary<Dimension> {

public:
  //--------------------------- Public Interface ---------------------------//
  typedef typename Dimension::Scalar Scalar;
  typedef typename Dimension::Vector Vector;
  typedef typename Dimension::Tensor Tensor;
  typedef typename Dimension::SymTensor SymTensor;
  typedef typename Dimension::ThirdRankTensor ThirdRankTensor;
  typedef typename Dimension::FourthRankTensor FourthRankTensor;
  typedef typename Dimension::FifthRankTensor FifthRankTensor;
  typedef typename Dimension::FacetedVolume FacetedVolume;
  typedef typename StateBase<Dimension>::KeyType KeyType;

  // Constructors and destructors.
  ConstantBoundary(NodeList<Dimension>& nodeList,
                   const std::vector<int>& nodeIDs,
                   const GeomPlane<Dimension>& denialPlane);
  virtual ~ConstantBoundary();

  //**********************************************************************
  // All Boundary conditions must provide the following methods:
  // Use the given NodeList's neighbor object to select the ghost nodes.
  virtual void setGhostNodes(NodeList<Dimension>& nodeList) override;

  // For the computed set of ghost nodes, set the positions and H's.
  virtual void updateGhostNodes(NodeList<Dimension>& nodeList) override;

  // Apply the boundary condition to the ghost node values in the given Field.
  virtual void applyGhostBoundary(Field<Dimension, int>& field) const override;
  virtual void applyGhostBoundary(Field<Dimension, Scalar>& field) const override;
  virtual void applyGhostBoundary(Field<Dimension, Vector>& field) const override;
  virtual void applyGhostBoundary(Field<Dimension, Tensor>& field) const override;
  virtual void applyGhostBoundary(Field<Dimension, SymTensor>& field) const override;
  virtual void applyGhostBoundary(Field<Dimension, ThirdRankTensor>& field) const override;
  virtual void applyGhostBoundary(Field<Dimension, FourthRankTensor>& field) const override;
  virtual void applyGhostBoundary(Field<Dimension, FifthRankTensor>& field) const override;
  virtual void applyGhostBoundary(Field<Dimension, FacetedVolume>& field) const override;

  // Find any internal nodes that are in violation of this Boundary.
  virtual void setViolationNodes(NodeList<Dimension>& nodeList) override;

  // For the computed set of nodes in violation of the boundary, bring them
  // back into compliance (for the positions and H's.)
  virtual void updateViolationNodes(NodeList<Dimension>& nodeList) override;

  // Apply the boundary condition to the violation node values in the given Field.
  virtual void enforceBoundary(Field<Dimension, int>& field) const override;
  virtual void enforceBoundary(Field<Dimension, Scalar>& field) const override;
  virtual void enforceBoundary(Field<Dimension, Vector>& field) const override;
  virtual void enforceBoundary(Field<Dimension, Tensor>& field) const override;
  virtual void enforceBoundary(Field<Dimension, SymTensor>& field) const override;
  virtual void enforceBoundary(Field<Dimension, ThirdRankTensor>& field) const override;
  virtual void enforceBoundary(Field<Dimension, FourthRankTensor>& field) const override;
  virtual void enforceBoundary(Field<Dimension, FifthRankTensor>& field) const override;
  virtual void enforceBoundary(Field<Dimension, FacetedVolume>& field) const override;
  //**********************************************************************

  virtual void applyGhostBoundary(Field<Dimension, std::vector<Scalar>>& field) const override;
  virtual void applyGhostBoundary(Field<Dimension, std::vector<Vector>>& field) const override;

  // After physics have been initialized we take a snapshot of the node state.
  virtual void initializeProblemStartup() override;

  // Minimal valid test.
  virtual bool valid() const;

  // Accessor methods.
  std::vector<int> nodeIndices() const;
  int numConstantNodes() const;
  const NodeList<Dimension>& nodeList() const;
  Tensor reflectOperator() const;

  //****************************************************************************
  // Methods required for restarting.
  virtual std::string label() const;
  virtual void dumpState(FileIO& file, const std::string& pathName) const;
  virtual void restoreState(const FileIO& file, const std::string& pathName);

  // Redistribution methods.
  virtual void notifyBeforeRedistribution();
  virtual void notifyAfterRedistribution();
  //****************************************************************************

private:
  //--------------------------- Private Interface ---------------------------//
  NodeList<Dimension>* mNodeListPtr;
  int mBoundaryCount;
  Field<Dimension, int> mNodeFlags;
  int mNumConstantNodes;
  GeomPlane<Dimension> mDenialPlane;
  Tensor mReflectOperator;
  bool mActive;

  typedef std::map<KeyType, std::vector<int> > IntStorageType;
  typedef std::map<KeyType, std::vector<Scalar> > ScalarStorageType;
  typedef std::map<KeyType, std::vector<Vector> > VectorStorageType;
  typedef std::map<KeyType, std::vector<Tensor> > TensorStorageType;
  typedef std::map<KeyType, std::vector<SymTensor> > SymTensorStorageType;
  typedef std::map<KeyType, std::vector<ThirdRankTensor> > ThirdRankTensorStorageType;
  typedef std::map<KeyType, std::vector<FourthRankTensor> > FourthRankTensorStorageType;
  typedef std::map<KeyType, std::vector<FifthRankTensor> > FifthRankTensorStorageType;
  typedef std::map<KeyType, std::vector<FacetedVolume> > FacetedVolumeStorageType;
  typedef std::map<KeyType, std::vector<std::vector<Scalar> > > VectorScalarStorageType;
  typedef std::map<KeyType, std::vector<std::vector<Vector> > > VectorVectorStorageType;

  IntStorageType mIntValues;
  ScalarStorageType mScalarValues;
  VectorStorageType mVectorValues;
  TensorStorageType mTensorValues;
  SymTensorStorageType mSymTensorValues;
  ThirdRankTensorStorageType mThirdRankTensorValues;
  FourthRankTensorStorageType mFourthRankTensorValues;
  FifthRankTensorStorageType mFifthRankTensorValues;
  FacetedVolumeStorageType mFacetedVolumeValues;
  VectorScalarStorageType mVectorScalarValues;
  VectorVectorStorageType mVectorVectorValues;

  // The restart and redistribution registration.
  RestartRegistrationType mRestart;
  RedistributionRegistrationType mRedistribution;

  // No default or copy constructors.
  ConstantBoundary();
  ConstantBoundary(ConstantBoundary&);
};

}

#include "ConstantBoundaryInline.hh"

#else

// Forward declaration.
namespace Spheral {
  template<typename Dimension> class ConstantBoundary;
}

#endif
