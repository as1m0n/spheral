#------------------------------------------------------------------------------
# A simple class to control simulation runs for Spheral.
#------------------------------------------------------------------------------
import sys
import gc
import mpi
from SpheralModules.Spheral import FileIOSpace
from SpheralModules.Spheral import State1d, State2d, State3d
from SpheralModules.Spheral import StateDerivatives1d, StateDerivatives2d, StateDerivatives3d
from SpheralModules.Spheral import iterateIdealH1d, iterateIdealH2d, iterateIdealH3d
from SpheralModules.Spheral.KernelSpace import TableKernel1d, TableKernel2d, TableKernel3d
from SpheralModules.Spheral.KernelSpace import BSplineKernel1d, BSplineKernel2d, BSplineKernel3d
from SpheralModules.Spheral.DataOutput import RestartableObject, RestartRegistrar
from SpheralModules.Spheral import BoundarySpace
from SpheralModules.Spheral.FieldSpace import *
from SpheralModules import vector_of_Physics1d, vector_of_Physics2d, vector_of_Physics3d
from SpheralTimer import SpheralTimer
from SpheralConservation import SpheralConservation
#from ExtendFlatFileIO import FlatFileIO
from GzipFileIO import GzipFileIO

from SpheralTestUtilities import globalFrame
from NodeGeneratorBase import ConstantRho

class SpheralController(RestartableObject):

    #--------------------------------------------------------------------------
    # Constuctor.
    #--------------------------------------------------------------------------
    def __init__(self, integrator, kernel,
                 statsStep = 1,
                 printStep = 1,
                 garbageCollectionStep = 100,
                 redistributeStep = None,
                 restartStep = None,
                 restartBaseName = "restart",
                 restartObjects = [],
                 restartFileConstructor = GzipFileIO,
                 initializeDerivatives = False,
                 vizBaseName = None,
                 vizDir =  ".",
                 vizStep = None,
                 vizTime = None,
                 vizMethod = None,
                 initialTime = 0.0):
        RestartableObject.__init__(self)
        self.integrator = integrator
        self.kernel = kernel
        self.restartObjects = restartObjects
        self.restartFileConstructor = restartFileConstructor

        # Determine the dimensionality of this run, based on the integrator.
        self.dim = "%id" % self.integrator.dataBase().nDim

        # Determine the visualization method.
        if vizMethod:
            self.vizMethod = vizMethod
        else:
            if self.dim == "1d":
                from Spheral1dVizDump import dumpPhysicsState
            else:
                from SpheralVoronoiSiloDump import dumpPhysicsState
            self.vizMethod = dumpPhysicsState

        # If this is a parallel run, automatically construct and insert
        # a DistributedBoundaryCondition into each physics package.
        self.insertDistributedBoundary(integrator.physicsPackages())

        # Generic initialization work.
        self.reinitializeProblem(restartBaseName,
                                 initialTime = initialTime,
                                 statsStep = statsStep,
                                 printStep = printStep,
                                 garbageCollectionStep = garbageCollectionStep,
                                 redistributeStep = redistributeStep,
                                 restartStep = restartStep,
                                 initializeDerivatives = initializeDerivatives)

        # Add the dynamic redistribution object to the controller.
        self.addRedistributeNodes(self.kernel)

        # Set up any visualization required.
        if not vizBaseName is None:
            assert not vizDir is None
            assert not (vizStep is None and vizTime is None)
            self.addVisualizationDumps(vizBaseName, vizDir, vizStep, vizTime)

        return

    #--------------------------------------------------------------------------
    # (Re)initialize the current problem (and controller state).
    # This method is intended to be called before the controller begins a new
    # problem from time 0.
    #--------------------------------------------------------------------------
    def reinitializeProblem(self, restartBaseName,
                            initialTime = 0.0,
                            statsStep = 1,
                            printStep = 1,
                            garbageCollectionStep = 100,
                            redistributeStep = None,
                            restartStep = None,
                            initializeDerivatives = False):

        # Intialize the cycle count.
        self.totalSteps = 0

        # Construct a timer to track the cycle step time.
        self.stepTimer = SpheralTimer("Time per integration cycle.")

        # Do the per package one time initialization.
        for package in self.integrator.physicsPackages():
            package.initializeProblemStartup(self.integrator.dataBase())

        # Construct a fresh conservation check object.
        self.conserve = SpheralConservation(self.integrator.dataBase(),
                                            self.integrator.physicsPackages())

        # Prepare an empty set of periodic work.
        self._periodicWork = []
        self._periodicTimeWork = []
        
        # Set the restart file base name.
        self.setRestartBaseName(restartBaseName)
        
        # Set the simulation time.
        self.integrator.currentTime = initialTime
##         state = eval("State%s(self.integrator.dataBase(), self.integrator.physicsPackages())" % (self.dim))
##         derivs = eval("StateDerivatives%s(self.integrator.dataBase(), self.integrator.physicsPackages())" % (self.dim))
##         self.integrator.initialize(state, derivs)
##         self.integrator.initialize(state, derivs)

        # If requested, initialize the derivatives.
        if initializeDerivatives:
            state = eval("State%s(self.integrator.dataBase(), self.integrator.physicsPackages())" % (self.dim))
            derivs = eval("StateDerivatives%s(self.integrator.dataBase(), self.integrator.physicsPackages())" % (self.dim))
            db = self.integrator.dataBase()
            self.integrator.evaluateDerivatives(initialTime, 0.0, db, state, derivs)

        # Set up the default periodic work.
        self.appendPeriodicWork(self.printCycleStatus, printStep)
        self.appendPeriodicWork(self.garbageCollection, garbageCollectionStep)
        self.appendPeriodicWork(self.updateConservation, statsStep)
        self.appendPeriodicWork(self.updateDomainDistribution, redistributeStep)
        self.appendPeriodicWork(self.updateRestart, restartStep)

        return

    #--------------------------------------------------------------------------
    # Set the restart base name.
    #--------------------------------------------------------------------------
    def setRestartBaseName(self,
                           name,
                           rank = mpi.rank,
                           procs = mpi.procs):
        self.restartBaseName = name

        # If we're running parallel then add the domain info to the restart
        # base name.
        if procs > 1:
            self.restartBaseName += '_rank%i_of_%idomains' % (rank, procs)

        return

    #--------------------------------------------------------------------------
    # Return the current time.
    #--------------------------------------------------------------------------
    def time(self):
        return self.integrator.currentTime

    #--------------------------------------------------------------------------
    # Return the last timestep.
    #--------------------------------------------------------------------------
    def lastDt(self):
        return self.integrator.lastDt

    #--------------------------------------------------------------------------
    # Smooth the physical variables.
    #--------------------------------------------------------------------------
    def smoothState(self, smoothIters=1):
        db = self.integrator.dataBase()
        scalarSmooth = eval("smoothScalarFields%id" % db.nDim)
        vectorSmooth = eval("smoothVectorFields%id" % db.nDim)
        tensorSmooth = eval("smoothSymTensorFields%id" % db.nDim)
        for iter in xrange(smoothIters):
            state = eval("State%id(db, self.integrator.physicsPackages())" % db.nDim)
            derivs = eval("StateDerivatives%id(db, self.integrator.physicsPackages())" % db.nDim)
            self.integrator.setGhostNodes()
            self.integrator.applyGhostBoundaries(state, derivs)
            smoothedVelocity = vectorSmooth(db.fluidVelocity,
                                            db.fluidPosition,
                                            db.fluidWeight,
                                            db.fluidMass,
                                            db.fluidMassDensity,
                                            db.fluidHfield,
                                            self.kernel)
            smoothedSpecificThermalEnergy = scalarSmooth(db.fluidSpecificThermalEnergy,
                                                         db.fluidPosition,
                                                         db.fluidWeight,
                                                         db.fluidMass,
                                                         db.fluidMassDensity,
                                                         db.fluidHfield,
                                                         self.kernel)
            smoothedHfield = tensorSmooth(db.fluidHfield,
                                          db.fluidPosition,
                                          db.fluidWeight,
                                          db.fluidMass,
                                          db.fluidMassDensity,
                                          db.fluidHfield,
                                          self.kernel)
            db.fluidVelocity.assignFields(smoothedVelocity)
            db.fluidSpecificThermalEnergy.assignFields(smoothedSpecificThermalEnergy)
            db.fluidHfield.assignFields(smoothedHfield)
            for nodeList in db.fluidNodeLists():
                nodeList.neighbor().updateNodes()
            db.updateConnectivityMap()
            for nodeList in db.fluidNodeLists():
                nodeList.updateWeight(db.connectivityMap())

        return

    #--------------------------------------------------------------------------
    # Advance the system to the given simulation time.  The user can also
    # specify a max number of steps to take.
    #--------------------------------------------------------------------------
    def advance(self, goalTime, maxSteps=None):
        currentSteps = 0
        while (self.time() < goalTime and
               (maxSteps == None or currentSteps < maxSteps)):
            self.stepTimer.start()
            self.integrator.step(goalTime)
            self.stepTimer.stop()
            currentSteps = currentSteps + 1
            self.totalSteps = self.totalSteps + 1

            # Do the periodic work.
            self.doPeriodicWork()

#         # Force the periodic work to fire at the end of an advance.
#         if maxSteps != 0:
#             self.doPeriodicWork(force=True)

        db = self.integrator.dataBase()
        bcs = self.integrator.uniqueBoundaryConditions()
        numActualGhostNodes = 0
        for bc in bcs:
            numActualGhostNodes += bc.numGhostNodes
        print "Total number of (internal, ghost, active ghost) nodes : (%i, %i, %i)" % (mpi.allreduce(db.numInternalNodes, mpi.SUM),
                                                                                        mpi.allreduce(db.numGhostNodes, mpi.SUM),
                                                                                        mpi.allreduce(numActualGhostNodes, mpi.SUM))

        # Print how much time was spent per integration cycle.
        self.stepTimer.printStatus()

        return

    #--------------------------------------------------------------------------
    # Step method, where the user can specify to take a given number of steps.
    #--------------------------------------------------------------------------
    def step(self, steps=1):
        self.advance(1e40, steps)

    #--------------------------------------------------------------------------
    # Do the periodic work.
    #--------------------------------------------------------------------------
    def doPeriodicWork(self, force=False):
        dt = self.lastDt()
        t1 = self.time()
        t0 = t1 - dt

        # Do any periodic cycle work, as determined by the number of steps.
        for method, frequency in self._periodicWork:
            if frequency is not None and (force or self.totalSteps % frequency == 0):
                method(self.totalSteps, t1, dt)

        # Do any periodic time work, as determined by the current time.
        for method, frequency in self._periodicTimeWork:
            if frequency is not None and (force or ((t0 // frequency) != (t1 // frequency))):
                method(self.totalSteps, t1, dt)

        return

    #--------------------------------------------------------------------------
    # Add a (method, frequency) tuple to the cyclic periodic work.
    # call during advance.
    #--------------------------------------------------------------------------
    def appendPeriodicWork(self, method, frequency):
        self._periodicWork.append((method, frequency))

    #--------------------------------------------------------------------------
    # Add a (method, frequency) tuple to the time based periodic work.
    #--------------------------------------------------------------------------
    def appendPeriodicTimeWork(self, method, frequency):
        self._periodicTimeWork.append((method, frequency))

    #--------------------------------------------------------------------------
    # Remove all instances of given method from the set of periodic work.
    #--------------------------------------------------------------------------
    def removePeriodicWork(self, method):
        cache = self._periodicWork[:]
        self._periodicWork = []
        for tup in cache:
            if tup[0] != method:
                self._periodicWork.append(tup)

    #--------------------------------------------------------------------------
    # Change the frequency at which the given method is called in periodic
    # work.
    #--------------------------------------------------------------------------
    def setFrequency(self, method, frequency):
        i = 0
        while (i < len(self._periodicWork) and
               self._periodicWork[i][0] != method):
            i += 1

        if i == len(self._periodicWork):
            print "Error, could not find periodic work calling ", method
            return
        else:
            self._periodicWork[i] = (method, frequency)

    #--------------------------------------------------------------------------
    # Get the frequency at which the given method is called in periodic
    # work.
    #--------------------------------------------------------------------------
    def getFrequency(self, method):
        i = 0
        while (i < len(self._periodicWork) and
               self._periodicWork[i][0] != method):
            i += 1

        if i == len(self._periodicWork):
            raise "Error, could not find periodic work calling %s" % str(method)

        return self._periodicWork[i][1]

    #--------------------------------------------------------------------------
    # A method for printing the cycle information.
    #--------------------------------------------------------------------------
    def printCycleStatus(self, cycle, Time, dt):
        print "Cycle=%i, \tTime=%g, \tTimeStep=%g" % (cycle, Time, dt)
        return

    #--------------------------------------------------------------------------
    # Periodically update the conservation statistics.
    #--------------------------------------------------------------------------
    def updateConservation(self, cycle, Time, dt):
        self.conserve.updateHistory(cycle, Time)
        return
    
    #--------------------------------------------------------------------------
    # Periodically force garbage collection.
    #--------------------------------------------------------------------------
    def garbageCollection(self, cycle, Time, dt):
##         everything = globalFrame().f_globals
##         stuff = [x for x in everything if hasattr(everything[x], "__wards__")]
##         print "SpheralController.garbageCollection: Stuff I found with wards: ", stuff
##         for x in stuff:
##             y = everything[x]
##             for z in y.__wards__:
##                 del z
##             del y.__wards__
        gc.collect()
        return

    #--------------------------------------------------------------------------
    # Periodically drop a restart file.
    #--------------------------------------------------------------------------
    def updateRestart(self, cycle, Time, dt):
        self.dropRestartFile()
        return

    #--------------------------------------------------------------------------
    # Periodically redistribute the nodes between domains.
    #--------------------------------------------------------------------------
    def updateDomainDistribution(self, cycle, Time, dt):
        if self.redistribute:

            # It is *critical* that each NodeList have the same number of fields
            # registered against it on each processor, therefore we pause to
            # garbage collect here and make sure any temporaries are gone.
            import gc
            while gc.collect():
                pass

            self.redistributeTimer.start()
            self.redistribute.redistributeNodes(self.integrator.dataBase(),
                                                self.integrator.uniqueBoundaryConditions())
            self.redistributeTimer.stop()
            self.redistributeTimer.printStatus()
        return

    #--------------------------------------------------------------------------
    # Find the name associated with the given object.
    #--------------------------------------------------------------------------
    def findname(thing):
        for mod in sys.modules.values():
            for name, val in mod.__dict__.items():
                if val is thing:
                    return name

    #--------------------------------------------------------------------------
    # Iterate over all the restartable objects and drop their state to a file.
    #--------------------------------------------------------------------------
    def dropRestartFile(self):

        # First find out if the requested directory exists.
        import os
        dire = os.path.dirname(os.path.abspath(self.restartBaseName))
        if not os.path.exists(dire):
            raise RuntimeError("Directory %s does not exist or is inaccessible." %
                               dire)

        # Now we can invoke the restart!
        import time
        start = time.clock()
        fileName = self.restartBaseName + "_cycle%i" % self.totalSteps
        file = self.restartFileConstructor(fileName, FileIOSpace.Create)
        RestartRegistrar.instance().dumpState(file)
        print "Wrote restart file in %0.2f seconds" % (time.clock() - start)

        file.close()
        del file
        return

    #--------------------------------------------------------------------------
    # Iterate over all the restartable objects and restore their state.
    #--------------------------------------------------------------------------
    def loadRestartFile(self, restoreCycle,
                        frameDict=None):

        # Find out if the requested file exists.
        import os
        fileName = self.restartBaseName + "_cycle%i" % restoreCycle
        if self.restartFileConstructor is GzipFileIO:
            fileName += ".gz"
        if not os.path.exists(fileName):
            raise RuntimeError("File %s does not exist or is inaccessible." %
                               fileName)

        # Read that sucker.
        print 'Reading from restart file', fileName
        import time
        start = time.clock()
        if self.restartFileConstructor is GzipFileIO:
            file = self.restartFileConstructor(fileName, FileIOSpace.Read)
                                               #readToMemory = True)
        else:
            file = self.restartFileConstructor(fileName, FileIOSpace.Read)
        RestartRegistrar.instance().restoreState(file)
        print "Finished: required %0.2f seconds" % (time.clock() - start)

        # Do we need to force a boundary update to create ghost nodes?
        if (self.integrator.updateBoundaryFrequency > 1 and
            self.integrator.currentCycle % self.integrator.updateBoundaryFrequency != 0):
            print "Creating ghost nodes."
            start = time.clock()
            self.integrator.setGhostNodes()
            print "Finished: required %0.2f seconds" % (time.clock() - start)

        file.close()
        del file
        return

    #--------------------------------------------------------------------------
    # Generate a label for our restart info.
    #--------------------------------------------------------------------------
    def label(self):
        return "Controller"

    #--------------------------------------------------------------------------
    # Store the controllers necessary data.
    #--------------------------------------------------------------------------
    def dumpState(self, file, path):
        controlPath = path + "/self"
        file.writeObject(self.totalSteps, controlPath + "/totalSteps")
        return

    #--------------------------------------------------------------------------
    # Restore the controllers state.
    #--------------------------------------------------------------------------
    def restoreState(self, file, path):
        controlPath = path + "/self"
        self.totalSteps = file.readObject(controlPath + "/totalSteps")
        return

    #--------------------------------------------------------------------------
    # If necessary create and add a distributed boundary condition to each
    # physics package
    #--------------------------------------------------------------------------
    def insertDistributedBoundary(self, physicsPackages):

        # This is the list of boundary types that need to precede the distributed
        # boundary, since their ghost nodes need to be communicated.
        precedeDistributed = [BoundarySpace.PeriodicBoundary1d,
                              BoundarySpace.PeriodicBoundary2d,
                              BoundarySpace.PeriodicBoundary3d,
                              BoundarySpace.ConstantBoundary1d,
                              BoundarySpace.ConstantBoundary2d,
                              BoundarySpace.ConstantBoundary3d,
                              BoundarySpace.CylindricalBoundary,
                              BoundarySpace.SphericalBoundary]

        # Check if this is a parallel process or not.
        if mpi.procs == 1:
            self.domainbc = None

        # If this is a parallel run, then we need to create a distributed
        # boundary condition and insert it into the list of boundaries for each physics
        # package.
        else:
##             from SpheralModules.Spheral.BoundarySpace import NestedGridDistributedBoundary1d, \
##                                                              NestedGridDistributedBoundary2d, \
##                                                              NestedGridDistributedBoundary3d
##             self.domainbc = eval("NestedGridDistributedBoundary%s.instance()" % self.dim)
            from SpheralModules.Spheral.BoundarySpace import BoundingVolumeDistributedBoundary1d, \
                                                             BoundingVolumeDistributedBoundary2d, \
                                                             BoundingVolumeDistributedBoundary3d
            self.domainbc = eval("BoundingVolumeDistributedBoundary%s.instance()" % self.dim)

            # Iterate over each of the physics packages.
            for package in physicsPackages:

                # Make a copy of the current set of boundary conditions for this package,
                # and clear out the set in the physics package.
                boundaryConditions = list(package.boundaryConditions())
                package.clearBoundaries()

                # Sort the boundary conditions into two lists: those that need
                # to precede the distributed boundary condition, and those that
                # should follow it.
                precedeBoundaries = []
                followBoundaries = []
                for boundary in boundaryConditions:
                    precede = False
                    for btype in precedeDistributed:
                        if isinstance(boundary, btype):
                            precede = True
                    if precede:
                        precedeBoundaries.append(boundary)
                    else:
                        followBoundaries.append(boundary)

                assert len(precedeBoundaries) + len(followBoundaries) == len(boundaryConditions)
                for boundary in boundaryConditions:
                    assert (boundary in precedeBoundaries) or (boundary in followBoundaries)

                # Now put the boundaries back into the package.
                # NOTE!  We currently force the parallel boundary condition to the end of the list.
                # This is required because Boundary conditions are combinatorial -- i.e., ghost nodes
                # from prior boundary conditions can be used as controls in later.  If we put the parallel
                # boundary at the beginning of the list it's ghost state is not valid until finalizeBoundary
                # is called.
                for bc in precedeBoundaries + followBoundaries + [self.domainbc]:
                    package.appendBoundary(bc)

        # That's it.
        if not (self.domainbc is None):
            for package in physicsPackages:
                assert package.haveBoundary(self.domainbc)
        return

    #--------------------------------------------------------------------------
    # If this is a parallel run, then add a domain repartitioner to dynamically
    # balance the workload.
    #--------------------------------------------------------------------------
    def addRedistributeNodes(self, W):
        assert W.kernelExtent > 0.0
        self.redistribute = None
        self.redistributeTimer = SpheralTimer("Time for redistributing nodes.")
        if mpi.procs > 1:
            from SpheralModules.Spheral import PartitionSpace
            try:
                #self.redistribute = eval("PartitionSpace.ParmetisRedistributeNodes%s(W.kernelExtent)" % self.dim)
                #self.redistribute = eval("PartitionSpace.SortAndDivideRedistributeNodes%s(W.kernelExtent)" % self.dim)
                #self.redistribute = eval("PartitionSpace.PeanoHilbertOrderRedistributeNodes%s(W.kernelExtent)" % self.dim)
                self.redistribute = eval("PartitionSpace.VoronoiRedistributeNodes%s(W.kernelExtent)" % self.dim)
            except:
                print "Warning: this appears to be a parallel run, but Controller cannot construct"
                print "         dynamic redistributer."
                pass
        return

    #---------------------------------------------------------------------------
    # Add visualization.
    #---------------------------------------------------------------------------
    def addVisualizationDumps(self, vizBaseName, vizDir, vizStep, vizTime):
        self.vizBaseName = vizBaseName
        self.vizDir = vizDir
        self.vizStep = vizStep
        self.vizTime = vizTime
        if vizStep is not None:
            self.appendPeriodicWork(self.dropViz, vizStep)
        if vizTime is not None:
            self.appendPeriodicTimeWork(self.updateViz, vizTime)
        return

    #---------------------------------------------------------------------------
    # Periodically drop viz files.
    #---------------------------------------------------------------------------
    def updateViz(self, cycle, Time, dt):
        self.dropViz(cycle, Time, dt)
        return

    #---------------------------------------------------------------------------
    # Actually does the viz dump.
    #---------------------------------------------------------------------------
    def dropViz(self,
                cycle = None,
                Time = None,
                dt = None):
        self.integrator.setGhostNodes()
        self.vizMethod(self.integrator,
                       self.vizBaseName,
                       self.vizDir,
                       currentTime = self.time(),
                       currentCycle = self.totalSteps,
                       boundaries = self.integrator.uniqueBoundaryConditions())
        return

    #--------------------------------------------------------------------------
    # Iteratively set the H tensors, until the desired convergence criteria
    # are met.
    #--------------------------------------------------------------------------
    def iterateIdealH(self, hydro,
                      maxIdealHIterations = 100,
                      idealHTolerance = 1.0e-4):
        print "SpheralController: Initializing H's..."
        db = self.integrator.dataBase()
        bcs = self.integrator.uniqueBoundaryConditions()
        iterateIdealH = eval("iterateIdealH%s" % self.dim)
        iterateIdealH(db, bcs, hydro.kernel(), hydro.smoothingScaleMethod(), maxIdealHIterations, idealHTolerance, 0.0, False, False)

        return

    #---------------------------------------------------------------------------
    # Reinitialize the mass of each node such that the Voronoi mass density
    # matches the expected values.
    #---------------------------------------------------------------------------
    def voronoiInitializeMass(self):
        from generateMesh import generateLineMesh, generatePolygonalMesh, generatePolyhedralMesh
        db = self.integrator.dataBase()
        nodeLists = db.fluidNodeLists()
        boundaries = self.integrator.uniqueBoundaryConditions()
        method = eval("generate%sMesh" % {1 : "Line", 2 : "Polygonal", 3 : "Polyhedral"}[db.nDim])
        mesh, void = method(nodeLists,
                            boundaries = boundaries,
                            generateParallelConnectivity = False)
        for nodes in nodeLists:
            mass = nodes.mass()
            rho = nodes.massDensity()
            for i in xrange(nodes.numInternalNodes):
                mass[i] = rho[i]*mesh.zone(nodes, i).volume()
        return

    #---------------------------------------------------------------------------
    # Use a given pre-relaxation object (like the Voronoi hourglass control) to
    # relax a node distribution to some converged state.
    #---------------------------------------------------------------------------
    def prerelaxNodeDistribution(self,
                                 hourglass,
                                 rho,
                                 maxIterations = 100,
                                 tol = 1.0e-5):

        # What did the user pass in for rho?
        if type(rho) == type(1.0):
            rho = ConstantRho(rho)

        db = self.integrator.dataBase()
        nodeLists = db.fluidNodeLists()
        boundaries = self.integrator.uniqueBoundaryConditions()
        allpackages = self.integrator.physicsPackages()
        packages = eval("vector_of_Physics%id()" % db.nDim)
        packages.append(hourglass)

        # A helpful method for setting the density.
        def setRho():
            for nodes in nodeLists:
                pos = nodes.positions()
                massDensity = nodes.massDensity()
                for i in xrange(nodes.numInternalNodes):
                    massDensity[i] = rho(pos[i])
        setRho()

        # Iterate until we're done.
        iter = 0
        maxDisp = 2.0*tol
        while (iter < maxIterations and
               maxDisp > tol):
            iter += 1
            oldpos = db.fluidPosition
            oldpos.copyFields()
            state = eval("State%id(db, allpackages)" % db.nDim)
            derivs = eval("StateDerivatives%id(db, allpackages)" % db.nDim)
            self.integrator.initialize(state, derivs)
            state.update(derivs, 1.0, 0.0, 1.0)
            self.integrator.enforceBoundaries()
            self.integrator.applyGhostBoundaries()
            self.integrator.postStateUpdate()
            self.integrator.finalizeGhostBoundaries()
            self.integrator.finalize(0.0, 0.0, db, state, derivs)

            # Check the displacements.
            maxDisp = 0.0
            newpos = db.fluidPosition
            for (oldf, newf) in zip(oldpos, newpos):
                maxDisp = max([(old - new).magnitude() for old, new in zip(oldf.internalValues(), newf.internalValues())] + [0.0])
            maxDisp = mpi.allreduce(maxDisp, mpi.MAX)
            print " --> Iteration %i : max change = %g" % (iter, maxDisp)

        # That's about it.
        setRho()
        return
