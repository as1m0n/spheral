namespace Spheral {

//------------------------------------------------------------------------------
// Determine the principle derivatives.
//------------------------------------------------------------------------------
template<typename Dimension>
void
SolidSPHHydroBase<Dimension>::
evaluateDerivatives(const typename Dimension::Scalar /*time*/,
                    const typename Dimension::Scalar dt,
                    const DataBase<Dimension>& dataBase,
                    const State<Dimension>& state,
                    StateDerivatives<Dimension>& derivatives) const {

  // Get the ArtificialViscosity.
  auto& Q = this->artificialViscosity();

  // The kernels and such.
  const auto& W = this->kernel();
  const auto& WQ = this->PiKernel();
  const auto& WG = this->GradKernel();
  const auto& smoothingScaleMethod = this->smoothingScaleMethod();

  // A few useful constants we'll use in the following loop.
  const auto tiny = 1.0e-30;
  const auto W0 = W(0.0, 1.0);
  const auto WQ0 = WQ(0.0, 1.0);
  const auto epsTensile = this->epsilonTensile();
  const auto compatibleEnergy = this->compatibleEnergyEvolution();
  const auto XSPH = this->XSPH();

  // The connectivity.
  const auto& connectivityMap = dataBase.connectivityMap();
  const auto& nodeLists = connectivityMap.nodeLists();
  const auto numNodeLists = nodeLists.size();

  // Get the state and derivative FieldLists.
  // State FieldLists.
  const auto mass = state.fields(HydroFieldNames::mass, 0.0);
  const auto position = state.fields(HydroFieldNames::position, Vector::zero);
  const auto velocity = state.fields(HydroFieldNames::velocity, Vector::zero);
  const auto massDensity = state.fields(HydroFieldNames::massDensity, 0.0);
  const auto specificThermalEnergy = state.fields(HydroFieldNames::specificThermalEnergy, 0.0);
  const auto H = state.fields(HydroFieldNames::H, SymTensor::zero);
  const auto pressure = state.fields(HydroFieldNames::pressure, 0.0);
  const auto soundSpeed = state.fields(HydroFieldNames::soundSpeed, 0.0);
  const auto omega = state.fields(HydroFieldNames::omegaGradh, 0.0);
  const auto S = state.fields(SolidFieldNames::deviatoricStress, SymTensor::zero);
  const auto mu = state.fields(SolidFieldNames::shearModulus, 0.0);
  const auto damage = state.fields(SolidFieldNames::effectiveTensorDamage, SymTensor::zero);
  const auto gradDamage = state.fields(SolidFieldNames::damageGradient, Vector::zero);
  const auto fragIDs = state.fields(SolidFieldNames::fragmentIDs, int(1));
  const auto pTypes = state.fields(SolidFieldNames::particleTypes, int(0));
  CHECK(mass.size() == numNodeLists);
  CHECK(position.size() == numNodeLists);
  CHECK(velocity.size() == numNodeLists);
  CHECK(massDensity.size() == numNodeLists);
  CHECK(specificThermalEnergy.size() == numNodeLists);
  CHECK(H.size() == numNodeLists);
  CHECK(pressure.size() == numNodeLists);
  CHECK(soundSpeed.size() == numNodeLists);
  CHECK(omega.size() == numNodeLists);
  CHECK(S.size() == numNodeLists);
  CHECK(mu.size() == numNodeLists);
  CHECK(damage.size() == numNodeLists);
  CHECK(gradDamage.size() == numNodeLists);
  CHECK(fragIDs.size() == numNodeLists);
  CHECK(pTypes.size() == numNodeLists);

  // Derivative FieldLists.
  auto  rhoSum = derivatives.fields(ReplaceFieldList<Dimension, Scalar>::prefix() + HydroFieldNames::massDensity, 0.0);
  auto  DxDt = derivatives.fields(IncrementFieldList<Dimension, Vector>::prefix() + HydroFieldNames::position, Vector::zero);
  auto  DrhoDt = derivatives.fields(IncrementFieldList<Dimension, Scalar>::prefix() + HydroFieldNames::massDensity, 0.0);
  auto  DvDt = derivatives.fields(HydroFieldNames::hydroAcceleration, Vector::zero);
  auto  DepsDt = derivatives.fields(IncrementFieldList<Dimension, Scalar>::prefix() + HydroFieldNames::specificThermalEnergy, 0.0);
  auto  DvDx = derivatives.fields(HydroFieldNames::velocityGradient, Tensor::zero);
  auto  localDvDx = derivatives.fields(HydroFieldNames::internalVelocityGradient, Tensor::zero);
  auto  M = derivatives.fields(HydroFieldNames::M_SPHCorrection, Tensor::zero);
  auto  localM = derivatives.fields("local " + HydroFieldNames::M_SPHCorrection, Tensor::zero);
  auto  DHDt = derivatives.fields(IncrementFieldList<Dimension, SymTensor>::prefix() + HydroFieldNames::H, SymTensor::zero);
  auto  Hideal = derivatives.fields(ReplaceBoundedFieldList<Dimension, SymTensor>::prefix() + HydroFieldNames::H, SymTensor::zero);
  auto  maxViscousPressure = derivatives.fields(HydroFieldNames::maxViscousPressure, 0.0);
  auto  effViscousPressure = derivatives.fields(HydroFieldNames::effectiveViscousPressure, 0.0);
  auto  rhoSumCorrection = derivatives.fields(HydroFieldNames::massDensityCorrection, 0.0);
  auto  viscousWork = derivatives.fields(HydroFieldNames::viscousWork, 0.0);
  auto& pairAccelerations = derivatives.getAny(HydroFieldNames::pairAccelerations, vector<Vector>());
  auto  XSPHWeightSum = derivatives.fields(HydroFieldNames::XSPHWeightSum, 0.0);
  auto  XSPHDeltaV = derivatives.fields(HydroFieldNames::XSPHDeltaV, Vector::zero);
  auto  weightedNeighborSum = derivatives.fields(HydroFieldNames::weightedNeighborSum, 0.0);
  auto  massSecondMoment = derivatives.fields(HydroFieldNames::massSecondMoment, SymTensor::zero);
  auto  DSDt = derivatives.fields(IncrementFieldList<Dimension, SymTensor>::prefix() + SolidFieldNames::deviatoricStress, SymTensor::zero);
  CHECK(rhoSum.size() == numNodeLists);
  CHECK(DxDt.size() == numNodeLists);
  CHECK(DrhoDt.size() == numNodeLists);
  CHECK(DvDt.size() == numNodeLists);
  CHECK(DepsDt.size() == numNodeLists);
  CHECK(DvDx.size() == numNodeLists);
  CHECK(localDvDx.size() == numNodeLists);
  CHECK(M.size() == numNodeLists);
  CHECK(localM.size() == numNodeLists);
  CHECK(DHDt.size() == numNodeLists);
  CHECK(Hideal.size() == numNodeLists);
  CHECK(maxViscousPressure.size() == numNodeLists);
  CHECK(effViscousPressure.size() == numNodeLists);
  CHECK(rhoSumCorrection.size() == numNodeLists);
  CHECK(viscousWork.size() == numNodeLists);
  CHECK(XSPHWeightSum.size() == numNodeLists);
  CHECK(XSPHDeltaV.size() == numNodeLists);
  CHECK(weightedNeighborSum.size() == numNodeLists);
  CHECK(massSecondMoment.size() == numNodeLists);
  CHECK(DSDt.size() == numNodeLists);

  // The set of interacting node pairs.
  const auto& pairs = connectivityMap.nodePairList();
  const auto  npairs = pairs.size();

  // Size up the pair-wise accelerations before we start.
  if (compatibleEnergy) pairAccelerations.resize(npairs);

  // The scale for the tensile correction.
  const auto& nodeList = mass[0]->nodeList();
  const auto  nPerh = nodeList.nodesPerSmoothingScale();
  const auto  WnPerh = W(1.0/nPerh, 1.0);

  // Build the functor we use to compute the effective coupling between nodes.
  DamagedNodeCouplingWithFrags<Dimension> coupling(damage, gradDamage, H, fragIDs);


  using WALK_EXEC_POL = RAJA::omp_for_exec;
  using WALK_REDUCE_POL = RAJA::omp_reduce;

  typename SpheralThreads<Dimension>::FieldListStack threadStack;
  auto maxViscousPressure_thread = maxViscousPressure.threadCopy(threadStack, ThreadReduction::MAX);

  auto DvDt_thread   =   DvDt.getReduceSum(WALK_REDUCE_POL());
  auto rhoSum_thread = rhoSum.getReduceSum(WALK_REDUCE_POL());
  auto DepsDt_thread = DepsDt.getReduceSum(WALK_REDUCE_POL());
  auto DvDx_thread   =   DvDx.getReduceSum(WALK_REDUCE_POL());
  auto localDvDx_thread = localDvDx.getReduceSum(WALK_REDUCE_POL());
  auto M_thread = M.getReduceSum(WALK_REDUCE_POL());
  auto localM_thread = localM.getReduceSum(WALK_REDUCE_POL());
  auto effViscousPressure_thread = effViscousPressure.getReduceSum(WALK_REDUCE_POL());
  auto rhoSumCorrection_thread = rhoSumCorrection.getReduceSum(WALK_REDUCE_POL());
  auto viscousWork_thread = viscousWork.getReduceSum(WALK_REDUCE_POL());
  auto XSPHWeightSum_thread = XSPHWeightSum.getReduceSum(WALK_REDUCE_POL());
  auto XSPHDeltaV_thread = XSPHDeltaV.getReduceSum(WALK_REDUCE_POL());
  auto weightedNeighborSum_thread = weightedNeighborSum.getReduceSum(WALK_REDUCE_POL());
  auto massSecondMoment_thread = massSecondMoment.getReduceSum(WALK_REDUCE_POL());
  auto DSDt_thread = DSDt.getReduceSum(WALK_REDUCE_POL());


  // Walk all the interacting pairs.
  RAJA::TypedRangeSegment<unsigned int> array_npairs(0, npairs);
  RAJA::forall<WALK_EXEC_POL>(array_npairs, [&](unsigned int kk) {
      
    Scalar Wi, gWi, WQi, gWQi, Wj, gWj, WQj, gWQj;
    Tensor QPiij, QPiji;
    SymTensor sigmai, sigmaj;

    const auto start = Timing::currentTime();
    int i = pairs[kk].i_node;
    int j = pairs[kk].j_node;
    int nodeListi = pairs[kk].i_list;
    int nodeListj = pairs[kk].j_list;

    // Get the state for node i.
    const Vector& ri = position(nodeListi, i);
    const double  mi = mass(nodeListi, i);
    const Vector& vi = velocity(nodeListi, i);
    const double  rhoi = massDensity(nodeListi, i);
    const double  Pi = pressure(nodeListi, i);
    const SymTensor& Hi = H(nodeListi, i);
    const double  ci = soundSpeed(nodeListi, i);
    const double  omegai = omega(nodeListi, i);
    const SymTensor& Si = S(nodeListi, i);
    const double  Hdeti = Hi.Determinant();
    const double  safeOmegai = safeInv(omegai, tiny);
    const double  pTypei = pTypes(nodeListi, i);
    CHECK(mi > 0.0);
    CHECK(rhoi > 0.0);
    CHECK(Hdeti > 0.0);

    auto& rhoSumi = rhoSum_thread[nodeListi][i];
    auto& DvDti = DvDt_thread[nodeListi][i];
    auto& DepsDti = DepsDt_thread[nodeListi][i];
    auto& DvDxi = DvDx_thread[nodeListi][i];
    auto& localDvDxi = localDvDx_thread[nodeListi][i];
    auto& Mi = M_thread[nodeListi][i];
    auto& localMi = localM_thread[nodeListi][i];
    double& maxViscousPressurei = maxViscousPressure_thread(nodeListi, i);
    auto& effViscousPressurei = effViscousPressure_thread[nodeListi][i];
    auto& rhoSumCorrectioni = rhoSumCorrection_thread[nodeListi][i];
    auto& viscousWorki = viscousWork_thread[nodeListi][i];
    auto& XSPHWeightSumi = XSPHWeightSum_thread[nodeListi][i];
    auto& XSPHDeltaVi = XSPHDeltaV_thread[nodeListi][i];
    auto& weightedNeighborSumi = weightedNeighborSum_thread[nodeListi][i];
    auto& massSecondMomenti = massSecondMoment_thread[nodeListi][i];

    // Get the state for node j
    const Vector& rj = position(nodeListj, j);
    const double  mj = mass(nodeListj, j);
    const Vector& vj = velocity(nodeListj, j);
    const double  rhoj = massDensity(nodeListj, j);
    const double  Pj = pressure(nodeListj, j);
    const SymTensor& Hj = H(nodeListj, j);
    const double  cj = soundSpeed(nodeListj, j);
    const double  omegaj = omega(nodeListj, j);
    const SymTensor& Sj = S(nodeListj, j);
    const double  Hdetj = Hj.Determinant();
    const double  safeOmegaj = safeInv(omegaj, tiny);
    const double  pTypej = pTypes(nodeListj, j);
    CHECK(mj > 0.0);
    CHECK(rhoj > 0.0);
    CHECK(Hdetj > 0.0);

    auto& rhoSumj = rhoSum_thread[nodeListj][j];
    auto& DvDtj = DvDt_thread[nodeListj][j];
    auto& DepsDtj = DepsDt_thread.at(nodeListj).at(j);
    auto& DvDxj = DvDx_thread[nodeListj][j];
    auto& localDvDxj = localDvDx_thread[nodeListj][j];
    auto& Mj = M_thread[nodeListj][j];
    auto& localMj = localM_thread[nodeListj][j];
    double& maxViscousPressurej = maxViscousPressure_thread(nodeListj, j);
    auto& effViscousPressurej = effViscousPressure_thread[nodeListj][j];
    auto& rhoSumCorrectionj = rhoSumCorrection_thread[nodeListj][j];
    auto& viscousWorkj = viscousWork_thread[nodeListj][j];
    auto& XSPHWeightSumj = XSPHWeightSum_thread[nodeListj][j];
    auto& XSPHDeltaVj = XSPHDeltaV_thread[nodeListj][j];
    auto& weightedNeighborSumj = weightedNeighborSum_thread[nodeListj][j];
    auto& massSecondMomentj = massSecondMoment_thread[nodeListj][j];

    // Flag if this is a contiguous material pair or not.
    const bool sameMatij = true; // (nodeListi == nodeListj and fragIDi == fragIDj);

    // Flag if at least one particle is free (0).
    const bool freeParticle = (pTypei == 0 or pTypej == 0);

    // Node displacement.
    const Vector rij = ri - rj;
    const Vector etai = Hi*rij;
    const Vector etaj = Hj*rij;
    const double etaMagi = etai.magnitude();
    const double etaMagj = etaj.magnitude();
    CHECK(etaMagi >= 0.0);
    CHECK(etaMagj >= 0.0);

    // Symmetrized kernel weight and gradient.
    std::tie(Wi, gWi) = W.kernelAndGradValue(etaMagi, Hdeti);
    std::tie(WQi, gWQi) = WQ.kernelAndGradValue(etaMagi, Hdeti);
    const Vector Hetai = Hi*etai.unitVector();
    const Vector gradWi = gWi*Hetai;
    const Vector gradWQi = gWQi*Hetai;
    const Vector gradWGi = WG.gradValue(etaMagi, Hdeti) * Hetai;

    std::tie(Wj, gWj) = W.kernelAndGradValue(etaMagj, Hdetj);
    std::tie(WQj, gWQj) = WQ.kernelAndGradValue(etaMagj, Hdetj);
    const Vector Hetaj = Hj*etaj.unitVector();
    const Vector gradWj = gWj*Hetaj;
    const Vector gradWQj = gWQj*Hetaj;
    const Vector gradWGj = WG.gradValue(etaMagj, Hdetj) * Hetaj;

    // Determine how we're applying damage.
    const auto fDeffij = coupling(nodeListi, i, nodeListj, j);

    // Zero'th and second moment of the node distribution -- used for the
    // ideal H calculation.
    const double fweightij = sameMatij ? 1.0 : mj*rhoi/(mi*rhoj);
    const double rij2 = rij.magnitude2();
    const SymTensor thpt = rij.selfdyad()*safeInvVar(rij2*rij2*rij2);
    weightedNeighborSumi +=     fweightij*abs(gWi);
    weightedNeighborSumj += 1.0/fweightij*abs(gWj);
    massSecondMomenti +=     fweightij*gradWi.magnitude2()*thpt;
    massSecondMomentj += 1.0/fweightij*gradWj.magnitude2()*thpt;

    // Contribution to the sum density (only if the same material).
    if (nodeListi == nodeListj) {
      rhoSumi += mj*Wi;
      rhoSumj += mi*Wj;
    }

    // Contribution to the sum density correction
    rhoSumCorrectioni += mj * WQi / rhoj ;
    rhoSumCorrectionj += mi * WQj / rhoi ;

    // Compute the pair-wise artificial viscosity.
    const Vector vij = vi - vj;
    std::tie(QPiij, QPiji) = Q.Piij(nodeListi, i, nodeListj, j,
                                    ri, etai, vi, rhoi, ci, Hi,
                                    rj, etaj, vj, rhoj, cj, Hj);
    const Vector Qacci = 0.5*(QPiij*gradWQi);
    const Vector Qaccj = 0.5*(QPiji*gradWQj);
    const double workQi = vij.dot(Qacci);
    const double workQj = vij.dot(Qaccj);
    const double Qi = rhoi*rhoi*(QPiij.diagonalElements().maxAbsElement());
    const double Qj = rhoj*rhoj*(QPiji.diagonalElements().maxAbsElement());
    maxViscousPressurei = max(maxViscousPressurei, Qi);
    maxViscousPressurej = max(maxViscousPressurej, Qj);
    effViscousPressurei += mj*Qi*WQi/rhoj;
    effViscousPressurej += mi*Qj*WQj/rhoi;
    viscousWorki += mj*workQi;
    viscousWorkj += mi*workQj;

    // Damage scaling of negative pressures.
    const double Peffi = (mNegativePressureInDamage or Pi > 0.0 ? Pi : fDeffij*Pi);
    const double Peffj = (mNegativePressureInDamage or Pj > 0.0 ? Pj : fDeffij*Pj);

    // Compute the stress tensors.
    sigmai = -Peffi*SymTensor::one;
    sigmaj = -Peffj*SymTensor::one;
    if (sameMatij) {
      if (mStrengthInDamage) {
        sigmai += Si;
        sigmaj += Sj;
      } else {
        sigmai += fDeffij*Si;
        sigmaj += fDeffij*Sj;
      }
    }

    // Compute the tensile correction to add to the stress as described in 
    // Gray, Monaghan, & Swift (Comput. Methods Appl. Mech. Eng., 190, 2001)
    const auto fi = epsTensile*FastMath::pow4(Wi/(Hdeti*WnPerh));
    const auto fj = epsTensile*FastMath::pow4(Wj/(Hdetj*WnPerh));
    const SymTensor Ri = fi*tensileStressCorrection(sigmai);
    const SymTensor Rj = fj*tensileStressCorrection(sigmaj);
    sigmai += Ri;
    sigmaj += Rj;

    // Acceleration.
    CHECK(rhoi > 0.0);
    CHECK(rhoj > 0.0);
    const auto sigmarhoi = safeOmegai*sigmai/(rhoi*rhoi);
    const auto sigmarhoj = safeOmegaj*sigmaj/(rhoj*rhoj);
    const auto deltaDvDt = sigmarhoi*gradWi + sigmarhoj*gradWj - Qacci - Qaccj;
    if (freeParticle) {
      DvDti += mj*deltaDvDt;
      DvDtj += -(mi*deltaDvDt);
    }
    if (compatibleEnergy) pairAccelerations[kk] = mj*deltaDvDt;  // Acceleration for i (j anti-symmetric)

    // Pair-wise portion of grad velocity.
    const auto deltaDvDxi = fDeffij*vij.dyad(gradWGi);
    const auto deltaDvDxj = fDeffij*vij.dyad(gradWGj);

    // Specific thermal energy evolution.
    DepsDti += -mj*(fDeffij*sigmarhoi.doubledot(deltaDvDxi.Symmetric()) - workQi);
    DepsDtj += -mi*(fDeffij*sigmarhoj.doubledot(deltaDvDxj.Symmetric()) - workQj);
    //DepsDt_thread[nodeListj][j]& -= mi*(fDeffij*sigmarhoj.doubledot(deltaDvDxj.Symmetric()) - workQj);


    // Velocity gradient.ay
    DvDxi += -mj*deltaDvDxi;
    DvDxj += -mi*deltaDvDxj;
    if (sameMatij) {
      localDvDxi += -mj*deltaDvDxi;
      localDvDxj += -mi*deltaDvDxj;
    }

    // Estimate of delta v (for XSPH).
    if (XSPH and sameMatij) {
      const auto wXSPHij = 0.5*(mi/rhoi*Wi + mj/rhoj*Wj);
      XSPHWeightSumi += wXSPHij;
      XSPHWeightSumj += wXSPHij;
      XSPHDeltaVi += -wXSPHij*vij;
      XSPHDeltaVj += wXSPHij*vij;
    }

    // Linear gradient correction term.
    Mi += -mj*rij.dyad(gradWGi);
    Mj += -mi*rij.dyad(gradWGj);
    if (sameMatij) {
      localMi += -mj*rij.dyad(gradWGi);
      localMj += -mi*rij.dyad(gradWGj);
    }

    // Add timing info for work
    double deltaTimePair = 0.5*Timing::difference(start, Timing::currentTime());

    RAJA::atomicAdd<RAJA::auto_atomic>(&nodeLists[nodeListi]->work()(i), deltaTimePair);
    RAJA::atomicAdd<RAJA::auto_atomic>(&nodeLists[nodeListj]->work()(j), deltaTimePair);

  }); // loop over pairs

  // Reduce the thread values to the master.
  threadReduceFieldLists<Dimension>(threadStack);

  rhoSum.getReduction(rhoSum_thread);
  DvDt.getReduction(DvDt_thread);
  DepsDt.getReduction(DepsDt_thread);
  DvDx.getReduction(DvDx_thread);
  localDvDx.getReduction(localDvDx_thread);
  M.getReduction(M_thread);
  localM.getReduction(localM_thread);
  effViscousPressure.getReduction(effViscousPressure_thread);
  rhoSumCorrection.getReduction(rhoSumCorrection_thread);
  viscousWork.getReduction(viscousWork_thread);
  XSPHWeightSum.getReduction(XSPHWeightSum_thread);
  XSPHDeltaV.getReduction(XSPHDeltaV_thread);
  weightedNeighborSum.getReduction(weightedNeighborSum_thread);
  massSecondMoment.getReduction(massSecondMoment_thread);
  DSDt.getReduction(DSDt_thread);


  // Finish up the derivatives for each point.

  RAJA::TypedRangeSegment<unsigned int> array_numNodeLists(0, numNodeLists);
  RAJA::forall<RAJA::seq_exec>(array_numNodeLists, [&](int nodeListi) {

    const auto& nodeList = mass[nodeListi]->nodeList();
    const auto  hmin = nodeList.hmin();
    const auto  hmax = nodeList.hmax();
    const auto  hminratio = nodeList.hminratio();
    const auto  nPerh = nodeList.nodesPerSmoothingScale();

    // Check if we can identify a reference density.
    auto rho0 = 0.0;
    try {
      rho0 = dynamic_cast<const SolidEquationOfState<Dimension>&>(dynamic_cast<const FluidNodeList<Dimension>&>(nodeList).equationOfState()).referenceDensity();
      // cerr << "Setting reference density to " << rho0 << endl;
    } catch(...) {
      // cerr << "BLAGO!" << endl;
    }

    const auto ni = nodeList.numInternalNodes();
    RAJA::TypedRangeSegment<unsigned int> array_ni(0, ni);
    RAJA::forall<RAJA::omp_for_exec>(array_ni, [&](int i) {

      // Get the state for node i.
      const Vector& ri = position(nodeListi, i);
      const auto& mi = mass(nodeListi, i);
      const auto& vi = velocity(nodeListi, i);
      const auto& rhoi = massDensity(nodeListi, i);
      const auto& Hi = H(nodeListi, i);
      const auto& Si = S(nodeListi, i);
      const auto& mui = mu(nodeListi, i);
      const auto  Hdeti = Hi.Determinant();
      const auto  numNeighborsi = connectivityMap.numNeighborsForNode(nodeListi, i);
      CHECK(mi > 0.0);
      CHECK(rhoi > 0.0);
      CHECK(Hdeti > 0.0);

      auto& rhoSumi = rhoSum(nodeListi, i);
      auto& DxDti = DxDt(nodeListi, i);
      auto& DrhoDti = DrhoDt(nodeListi, i);
      auto& DvDti = DvDt(nodeListi, i);
      auto& DepsDti = DepsDt(nodeListi, i);
      auto& DvDxi = DvDx(nodeListi, i);
      auto& localDvDxi = localDvDx(nodeListi, i);
      auto& Mi = M(nodeListi, i);
      auto& localMi = localM(nodeListi, i);
      auto& DHDti = DHDt(nodeListi, i);
      auto& Hideali = Hideal(nodeListi, i);
      auto& effViscousPressurei = effViscousPressure(nodeListi, i);
      auto& rhoSumCorrectioni = rhoSumCorrection(nodeListi, i);
      auto& XSPHWeightSumi = XSPHWeightSum(nodeListi, i);
      auto& XSPHDeltaVi = XSPHDeltaV(nodeListi, i);
      auto& weightedNeighborSumi = weightedNeighborSum(nodeListi, i);
      auto& massSecondMomenti = massSecondMoment(nodeListi, i);
      auto& DSDti = DSDt(nodeListi, i);

      // Add the self-contribution to density sum.
      rhoSumi += mi*W0*Hdeti;

      // Add the self-contribution to density sum correction.
      rhoSumCorrectioni += mi*WQ0*Hdeti/rhoi ;

      // Correct the effective viscous pressure.
      effViscousPressurei /= rhoSumCorrectioni ;

      // Finish the gradient of the velocity.
      CHECK(rhoi > 0.0);
      if (this->mCorrectVelocityGradient and
          std::abs(Mi.Determinant()) > 1.0e-10 and
          numNeighborsi > Dimension::pownu(2)) {
        Mi = Mi.Inverse();
        DvDxi = DvDxi*Mi;
      } else {
        DvDxi /= rhoi;
      }
      if (this->mCorrectVelocityGradient and
          std::abs(localMi.Determinant()) > 1.0e-10 and
          numNeighborsi > Dimension::pownu(2)) {
        localMi = localMi.Inverse();
        localDvDxi = localDvDxi*localMi;
      } else {
        localDvDxi /= rhoi;
      }

      // Evaluate the continuity equation.
      DrhoDti = -rhoi*DvDxi.Trace();

      // If needed finish the total energy derivative.
      if (this->mEvolveTotalEnergy) DepsDti = mi*(vi.dot(DvDti) + DepsDti);

      // Complete the moments of the node distribution for use in the ideal H calculation.
      weightedNeighborSumi = Dimension::rootnu(max(0.0, weightedNeighborSumi/Hdeti));
      massSecondMomenti /= Hdeti*Hdeti;

      // Determine the position evolution, based on whether we're doing XSPH or not.
      DxDti = vi;
      if (XSPH) {
        CHECK(XSPHWeightSumi >= 0.0);
        XSPHWeightSumi += Hdeti*mi/rhoi*W0 + 1.0e-30;
        DxDti += XSPHDeltaVi/XSPHWeightSumi;
      }

      // The H tensor evolution.
      DHDti = smoothingScaleMethod.smoothingScaleDerivative(Hi,
                                                            ri,
                                                            DvDxi,
                                                            hmin,
                                                            hmax,
                                                            hminratio,
                                                            nPerh);
      Hideali = smoothingScaleMethod.newSmoothingScale(Hi,
                                                       ri,
                                                       weightedNeighborSumi,
                                                       massSecondMomenti,
                                                       W,
                                                       hmin,
                                                       hmax,
                                                       hminratio,
                                                       nPerh,
                                                       connectivityMap,
                                                       nodeListi,
                                                       i);

      // Optionally use damage to ramp down stress on damaged material.
      const auto Di = (mDamageRelieveRubble ? 
                       max(0.0, min(1.0, damage(nodeListi, i).Trace() - 1.0)) :
                       0.0);

      // We also adjust the density evolution in the presence of damage.
      if (rho0 > 0.0) DrhoDti = (1.0 - Di)*DrhoDti - 0.25/dt*Di*(rhoi - rho0);

      // Determine the deviatoric stress evolution.
      const auto deformation = localDvDxi.Symmetric();
      const auto spin = localDvDxi.SkewSymmetric();
      const auto deviatoricDeformation = deformation - (deformation.Trace()/3.0)*SymTensor::one;
      const auto spinCorrection = (spin*Si + Si*spin).Symmetric();
      DSDti = spinCorrection + (2.0*mui)*deviatoricDeformation;

      // In the presence of damage, add a term to reduce the stress on this point.
      DSDti = (1.0 - Di)*DSDti - 0.25/dt*Di*Si;
    });
  });

}

}
