import Gnuplot
from Spheral import *
from math import *
from loadmpi import *
import numpy

SpheralGnuPlotCache = []

#-------------------------------------------------------------------------------
# Define a dummy Gnuplot class, so that non-master processes can silently
# and harmlessly accept Gnuplot commands.
#-------------------------------------------------------------------------------
class fakeGnuplot:
    def __init__(self):
        return
    def __call__(self, *arghs, **keyw):
        return
    def plot(self, *arghs, **keyw):
        return
    def replot(self, *arghs, **keyw):
        return
    def refresh(self, *arghs, **keyw):
        return
    def xlabel(self, *arghs, **keyw):
        return
    def ylabel(self, *arghs, **keyw):
        return
    def title(self, *arghs, **keyw):
        return
    def hardcopy(self, *arghs, **keyw):
        return

def generateNewGnuPlot():
    mpi, procID, nProcs = loadmpi()
    if procID == 0:
        return Gnuplot.Gnuplot()
    else:
        return fakeGnuplot()

#-------------------------------------------------------------------------------
# Helper method, sort a set of lists by the first one.
#-------------------------------------------------------------------------------
def multiSort(x, *args):
    # All the lists have to be the same length.
    for l in args:
        assert len(l) == len(x)

    # First build a composite list, each element of which is the tuple of
    # elements from each component list.
    multiList = []
    for i in xrange(len(x)):
        tmp = [x[i]]
        for l in args:
            tmp.append(l[i])
        multiList.append(tuple(tmp))

    # Now sort the multiList (by x).
    multiList.sort()

    # Return the sorted results in place of the arguments.
    for i in xrange(len(x)):
        x[i] = multiList[i][0]
        j = 1
        for l in args:
            l[i] = multiList[i][j]
            j += 1

    return

#-------------------------------------------------------------------------------
# Since the default Gnuplot.py doesn't support png output, I'll add it here
# myself.
#-------------------------------------------------------------------------------
def pngFile(plot, filename,
            color = 1,
            fontSize = "medium"):
    setLine = "set terminal png " + fontSize
    if color:
        setLine += " color"
    if filename[-4:] != ".png":
        filename += ".png"
    plot(setLine)
    plot.set_string("output", filename)
    plot.refresh()
    plot("set terminal x11")
    plot.set_string("output")
    return

#-------------------------------------------------------------------------------
# Calculate the radial velocity component, given a FieldList of positions
# and a FieldList of velocities.
#-------------------------------------------------------------------------------
def radialVelocityFieldList(positions,
                            velocities):

    dim = type(positions).__name__[-2:]
    radialVelocity = None
    fieldConstructor = None
    if dim == "1d":
        radialVelocity = ScalarFieldList1d()
        fieldConstructor = ScalarField1d
    elif dim == "2d":
        radialVelocity = ScalarFieldList2d()
        fieldConstructor = ScalarField2d
    elif dim == "3d":
        radialVelocity = ScalarFieldList3d()
        fieldConstructor = ScalarField3d
    radialVelocity.copyFields()
    for field in positions.fields():
        radialVelocity.appendField(fieldConstructor("radial velocity", field.nodeList()))

    assert positions.numFields() == velocities.numFields() == radialVelocity.numFields()

    for fieldID in xrange(positions.numFields()):
        rfield = positions[fieldID]
        vfield = velocities[fieldID]
        vrfield = radialVelocity[fieldID]
        assert rfield.numElements() == vfield.numElements() == vrfield.numElements()

        for nodeID in xrange(rfield.numElements()):
            r = rfield[nodeID]
            v = vfield[nodeID]
            runit = r.unitVector()
            vrfield[nodeID] = v.dot(runit)

    return radialVelocity

#-------------------------------------------------------------------------------
# Calculate the azimuthal velocity component, given a FieldList of positions
# and a FieldList of velocities.
#-------------------------------------------------------------------------------
def azimuthalVelocityFieldList(positions,
                               velocities):

    dim = type(positions).__name__[-2:]
    azimuthalVelocity = None
    fieldConstructor = None
    if dim == "1d":
        azimuthalVelocity = ScalarFieldList1d()
        fieldConstructor = ScalarField1d
    elif dim == "2d":
        azimuthalVelocity = ScalarFieldList2d()
        fieldConstructor = ScalarField2d
    elif dim == "3d":
        azimuthalVelocity = ScalarFieldList3d()
        fieldConstructor = ScalarField3d
    azimuthalVelocity.copyFields()
    for field in positions.fields():
        azimuthalVelocity.appendField(fieldConstructor("azimuthal velocity", field.nodeList()))

    assert positions.numFields() == velocities.numFields() == azimuthalVelocity.numFields()

    for fieldID in xrange(positions.numFields()):
        rfield = positions[fieldID]
        vfield = velocities[fieldID]
        vafield = azimuthalVelocity[fieldID]
        assert rfield.numElements() == vfield.numElements() == vafield.numElements()

        for nodeID in xrange(rfield.numElements()):
            r = rfield[nodeID]
            v = vfield[nodeID]
            raz = r.unitVector()
            x = raz.x
            y = raz.y
            raz.x = -y
            raz.y = x
            vafield[nodeID] = v.dot(raz)

    return azimuthalVelocity

#-------------------------------------------------------------------------------
# Helper method to determine the angular momentum per node.
#-------------------------------------------------------------------------------
def angularMomentum(mass, position, velocity):
    assert mass.numFields() == position.numFields() == velocity.numFields()

    result = []
    for massField, positionField, velocityField in zip(mass.fields(),
                                                       position.fields(),
                                                       velocity.fields()):
        assert (massField.nodeList().numInternalNodes ==
                positionField.nodeList().numInternalNodes ==
                velocityField.nodeList().numInternalNodes)
        for j in xrange(massField.nodeList().numInternalNodes):
            result.append((positionField[j].cross(velocityField[j]))*massField[j])

    return result

#-------------------------------------------------------------------------------
# Plot a FieldList
#-------------------------------------------------------------------------------
def plotFieldList(fieldList,
                  xFunction = "%s.x",
                  yFunction = "%s",
                  plotGhosts = False,
                  colorNodeLists = True,
                  colorDomains = False,
                  plot = None,
                  userXRange = [None, None],
                  userYRange = [None, None],
                  plotStyle = "lines",
                  lineStyle = "linetype -1 linewidth 1 pointtype 4 pointsize 1.0",
                  winTitle = None,
                  lineTitle = "",
                  xlabel = None,
                  ylabel = None):
    assert colorNodeLists + colorDomains <= 1

    mpi, procID, nProcs = loadmpi()
    if plot is None:
        plot = generateNewGnuPlot()
    SpheralGnuPlotCache.append(plot)

    # Gather the fieldList info across all processors to process 0.
    localNumNodes = []
    localX = []
    localY = []
    for field in fieldList.fields():
        if plotGhosts:
            n = field.nodeList().numNodes
        else:
            n = field.nodeList().numInternalNodes
        localNumNodes.append(n)
        for x in field.nodeList().positions().allValues()[:n]:
            localX.append(eval(xFunction % "x"))
        for y in field.allValues()[:n]:
            localY.append(eval(yFunction % "y"))
    if mpi:
        globalNumNodes = mpi.gather(localNumNodes, len(localNumNodes))
        globalX = mpi.gather(localX, len(localX))
        globalY = mpi.gather(localY, len(localY))
    else:
        globalNumNodes = localNumNodes
        globalX = localX
        globalY = localY

    if procID == 0:
        # Find the total number of nodes.
        totalNumNodes = 0
        for n in globalNumNodes:
            totalNumNodes += n
        assert(len(globalNumNodes) == fieldList.numFields()*nProcs)
        assert(len(globalX) == totalNumNodes)
        assert(len(globalY) == totalNumNodes)

        # Copy the input ranges, since for some reason these seem to have been
        # preserved between calls?
        xRange = userXRange[:]
        yRange = userYRange[:]

        # Set the line style
##         plot("set linestyle 1 " + lineStyle)

        # Set the labels.
        if winTitle: plot.title(winTitle)
        if xlabel: plot.xlabel(xlabel)
        if ylabel: plot.ylabel(ylabel)

        # Set the ranges.
        xmin = 1e30
        xmax = -1e30
        ymin = 1e30
        ymax = -1e30
        for x in globalX:
            xmin = min(xmin, x)
            xmax = max(xmax, x)
        for y in globalY:
            ymin = min(ymin, y)
            ymax = max(ymax, y)
        if xmin == xmax:
            xmin = xmin - 0.5
            xmax = xmax + 0.5
        if ymin == ymax:
            ymin = ymin - 0.5
            ymax = ymax + 0.5
        if xRange[0] == None: xRange[0] = xmin
        if xRange[1] == None: xRange[1] = xmax
        if yRange[0] == None: yRange[0] = ymin - 0.05*max(1e-5, ymax - ymin)
        if yRange[1] == None: yRange[1] = ymax + 0.05*max(1e-5, ymax - ymin)
        plot("set xrange [%f:%f]" % tuple(xRange))
        plot("set yrange [%f:%f]" % tuple(yRange))

        # Finally, loop over the fields and do the deed.
        assert(len(globalX) == len(globalY))
        if colorDomains:
            numNodesPerDomain = [0]
            for domainBegin in xrange(0, len(globalNumNodes), fieldList.numFields()):
                numNodesPerDomain.append(numNodesPerDomain[-1])
                for n in globalNumNodes[domainBegin:domainBegin + fieldList.numFields()]:
                    numNodesPerDomain[-1] += n
            for domain in xrange(nProcs):
                x = numpy.array(globalX[numNodesPerDomain[domain]:
                                        numNodesPerDomain[domain + 1]])
                y = numpy.array(globalY[numNodesPerDomain[domain]:
                                        numNodesPerDomain[domain + 1]])
                if x:
##                    plot("set linestyle %i lt %i pt %i" % (domain + 1,
##                                                           domain + 1,
##                                                           domain + 1))
                    data = Gnuplot.Data(x, y, 
                                        with_ = plotStyle,
                                        title = lineTitle + ": domain %i" % domain,
                                        inline = True)
                    plot.replot(data)
                    SpheralGnuPlotCache.append(data)

        elif colorNodeLists:
            legendNodeList = {}
            for i in xrange(fieldList.numFields()):
                legendNodeList[i] = lineTitle + ": " + fieldList[i].nodeList().name()

            cumulativeNumNodes = 0
            for fieldID in xrange(len(globalNumNodes)):
                n = globalNumNodes[fieldID]
                iNodeList = fieldID % fieldList.numFields()
                x = numpy.array(globalX[cumulativeNumNodes:
                                        cumulativeNumNodes + n])
                y = numpy.array(globalY[cumulativeNumNodes:
                                        cumulativeNumNodes + n])
                if n:
##                    plot("set linestyle %i lt %i pt %i" % (iNodeList + 1,
##                                                           iNodeList + 1,
##                                                           iNodeList + 1))
                    legend = legendNodeList[iNodeList]
                    legendNodeList[iNodeList] = None
                    data = Gnuplot.Data(x, y,
                                        with_ = plotStyle + " pt 1 lt %i" % iNodeList,
                                        title = legend,
                                        inline = True)
                    plot.replot(data)
                    SpheralGnuPlotCache.append(data)

                    cumulativeNumNodes += n
                
        else:
            x = numpy.array(globalX)
            y = numpy.array(globalY)
            data = Gnuplot.Data(x, y,
                                with_ = plotStyle, #  + " ls 1",
                                title = lineTitle,
                                inline = True)
            plot.replot(data)
            SpheralGnuPlotCache.append(data)
            lineTitle = None

    # That's it, return the Gnuplot object.
    mpi.barrier()
    return plot

#-------------------------------------------------------------------------------
# Plot the mass density, velocity, pressure, and smoothing scale for the fluid
# node lists in the given data base.  Implicitly assuming 1-D.
#-------------------------------------------------------------------------------
def plotState(dataBase,
              plotGhosts = False,
              colorNodeLists = True,
              colorDomains = False,
              plotStyle = "points",
              xFunction = "%s.x",
              vecyFunction = "%s.x",
              tenyFunction = "%s.xx ** -1",
              lineTitle = "Simulation"):

    rhoPlot = plotFieldList(dataBase.fluidMassDensity,
                            xFunction = xFunction,
                            plotGhosts = plotGhosts,
                            colorNodeLists = colorNodeLists,
                            colorDomains = colorDomains,
                            plotStyle = plotStyle,
                            winTitle = "Mass Density",
                            lineTitle = lineTitle,
                            xlabel="x")

    velPlot = plotFieldList(dataBase.fluidVelocity,
                            xFunction = xFunction,
                            yFunction = vecyFunction,
                            plotGhosts = plotGhosts,
                            colorNodeLists = colorNodeLists,
                            colorDomains = colorDomains,
                            plotStyle = plotStyle,
                            winTitle = "Velocity",
                            lineTitle = lineTitle,
                            xlabel="x")

    epsPlot = plotFieldList(dataBase.fluidSpecificThermalEnergy,
                            xFunction = xFunction,
                            plotGhosts = plotGhosts,
                            colorNodeLists = colorNodeLists,
                            colorDomains = colorDomains,
                            plotStyle = plotStyle,
                            winTitle = "Specific Thermal Energy",
                            lineTitle = lineTitle,
                            xlabel="x")

    PPlot = plotFieldList(dataBase.fluidPressure,
                          xFunction = xFunction,
                          plotGhosts = plotGhosts,
                          colorNodeLists = colorNodeLists,
                          colorDomains = colorDomains,
                          plotStyle = plotStyle,
                          winTitle = "Pressure",
                          lineTitle = lineTitle,
                          xlabel="x")

    HPlot = plotFieldList(dataBase.fluidHfield,
                          xFunction = xFunction,
                          yFunction = tenyFunction,
                          plotGhosts = plotGhosts,
                          colorNodeLists = colorNodeLists,
                          colorDomains = colorDomains,
                          plotStyle = plotStyle,
                          winTitle = "Smoothing scale",
                          lineTitle = lineTitle,
                          xlabel="x")

    return rhoPlot, velPlot, epsPlot, PPlot, HPlot

#-------------------------------------------------------------------------------
# Plot the state vs. radius
#-------------------------------------------------------------------------------
def plotRadialState(dataBase,
                    plotGhosts = False,
                    colorNodeLists = True,
                    colorDomains = False,
                    lineTitle = "Simulation"):

    rhoPlot = plotFieldList(dataBase.fluidMassDensity,
                            xFunction = "%s.magnitude()",
                            plotGhosts = plotGhosts,
                            colorNodeLists = colorNodeLists,
                            colorDomains = colorDomains,
                            plotStyle = "points",
                            winTitle = "Mass density",
                            lineTitle = lineTitle,
                            xlabel = "r")

    radialVelocity = radialVelocityFieldList(dataBase.fluidPosition,
                                             dataBase.fluidVelocity)
    velPlot = plotFieldList(radialVelocity,
                            xFunction = "%s.magnitude()",
                            plotGhosts = plotGhosts,
                            colorNodeLists = colorNodeLists,
                            colorDomains = colorDomains,
                            plotStyle = "points",
                            winTitle = " Radial Velocity",
                            lineTitle = lineTitle,
                            xlabel = "r")

    epsPlot = plotFieldList(dataBase.fluidSpecificThermalEnergy,
                            xFunction = "%s.magnitude()",
                            plotGhosts = plotGhosts,
                            colorNodeLists = colorNodeLists,
                            colorDomains = colorDomains,
                            plotStyle = "points",
                            winTitle = "Specific Thermal Energy",
                            lineTitle = lineTitle,
                            xlabel = "r")

    PPlot = plotFieldList(dataBase.fluidPressure,
                          xFunction = "%s.magnitude()",
                          plotGhosts = plotGhosts,
                          colorNodeLists = colorNodeLists,
                          colorDomains = colorDomains,
                          plotStyle = "points",
                          winTitle = "Pressure",
                          lineTitle = lineTitle,
                          xlabel = "r")

    HPlot = plotFieldList(dataBase.fluidHfield,
                          xFunction = "%s.magnitude()",
                          yFunction = "%s.xx**-1",
                          plotGhosts = plotGhosts,
                          colorNodeLists = colorNodeLists,
                          colorDomains = colorDomains,
                          plotStyle = "points",
                          winTitle = "Smoothing scale",
                          lineTitle = lineTitle,
                          xlabel = "r")

    return rhoPlot, velPlot, epsPlot, PPlot, HPlot

#-------------------------------------------------------------------------------
# Overplot the answer on results from plotState.
#-------------------------------------------------------------------------------
def plotAnswer(answerObject, time,
               rhoPlot = None,
               velPlot = None,
               epsPlot = None,
               PPlot = None,
               HPlot = None,
               x = None):

    mpi, procID, nProcs = loadmpi()

    try:
        x, v, u, rho, P, h = answerObject.solution(time, x)
    except:
        x, v, u, rho, P = answerObject.solution(time, x)

    if rhoPlot is not None:
        data = Gnuplot.Data(x, rho,
                            with_="lines lt 2",
                            title="Solution",
                            inline = True)
        SpheralGnuPlotCache.append(data)
        rhoPlot.replot(data)

    if velPlot is not None:
        data = Gnuplot.Data(x, v,
                            with_="lines lt 2",
                            title="Solution",
                            inline = True)
        SpheralGnuPlotCache.append(data)
        velPlot.replot(data)

    if epsPlot is not None:
        data = Gnuplot.Data(x, u,
                            with_="lines lt 2",
                            title="Solution",
                            inline = True)
        SpheralGnuPlotCache.append(data)
        epsPlot.replot(data)

    if PPlot is not None:
        data = Gnuplot.Data(x, P,
                            with_="lines lt 2",
                            title="Solution",
                            inline = True)
        SpheralGnuPlotCache.append(data)
        PPlot.replot(data)

    if HPlot is not None:
        data = Gnuplot.Data(x, h,
                            with_="lines lt 2",
                            title="Solution",
                            inline = True)
        SpheralGnuPlotCache.append(data)
        HPlot.replot(data)

    return

#-------------------------------------------------------------------------------
# Plot the node positions
#-------------------------------------------------------------------------------
def plotNodePositions2d(dataBase,
                        xFunction = "%s.x",
                        yFunction = "%s.y",
                        plotGhosts = False,
                        colorNodeLists = True,
                        colorDomains = False,
                        title = "",
                        style = "points",
                        persist = None):

    assert colorNodeLists + colorDomains <= 1

    mpi, procID, nProcs = loadmpi()

    # Gather the node positions across all domains.
    # Loop over all the NodeLists.
    localNumNodes = []
    xNodes = []
    yNodes = []
    for nodeList in dataBase.nodeLists():
        if plotGhosts:
            n = nodeList.numNodes
            xNodes.extend([eval(xFunction % "x") for x in nodeList.positions().allValues()])
            yNodes.extend([eval(yFunction % "x") for x in nodeList.positions().allValues()])
        else:
            n = nodeList.numInternalNodes
            xNodes.extend([eval(xFunction % "x") for x in nodeList.positions().internalValues()])
            yNodes.extend([eval(yFunction % "x") for x in nodeList.positions().internalValues()])
        localNumNodes.append(n)
    assert len(xNodes) == len(yNodes)
    
    numDomainNodes = [len(xNodes)]
    numNodesPerDomain = mpi.gather(numDomainNodes, 1)
    globalNumNodes = mpi.gather(localNumNodes, len(localNumNodes))
    globalXNodes = mpi.gather(xNodes, len(xNodes))
    globalYNodes = mpi.gather(yNodes, len(yNodes))

    if procID == 0:
        plot = Gnuplot.Gnuplot(persist = persist)
        plot("set size square")
        plot.title = title

        if colorDomains:
            cumulativeN = 0
            for domain in xrange(len(numNodesPerDomain)):
                n = numNodesPerDomain[domain]
                x = numpy.array(globalXNodes[cumulativeN:cumulativeN + n])
                y = numpy.array(globalYNodes[cumulativeN:cumulativeN + n])
                cumulativeN += n
##                plot("set linestyle %i lt %i pt %i" % (domain + 1,
##                                                       domain + 1,
##                                                       domain + 1))
                data = Gnuplot.Data(x, y, 
                                    with_ = style,
                                    inline = True)
                plot.replot(data)
                SpheralGnuPlotCache.append(data)

        elif colorNodeLists:
            cumulativeN = 0
            for i in xrange(len(globalNumNodes)):
                n = globalNumNodes[i]
                if n > 0:
                    iNodeList = i % dataBase.numNodeLists
                    x = numpy.array(globalXNodes[cumulativeN:cumulativeN + n])
                    y = numpy.array(globalYNodes[cumulativeN:cumulativeN + n])
                    cumulativeN += n
##                    plot("set linestyle %i lt %i pt %i" % (iNodeList + 1,
##                                                           iNodeList + 1,
##                                                           iNodeList + 1))
                    data = Gnuplot.Data(x, y, 
                                        with_ = style,
                                        inline = True)
                    plot.replot(data)
                    SpheralGnuPlotCache.append(data)

        else:
            x = numpy.array(globalXNodes)
            y = numpy.array(globalYNodes)
            data = Gnuplot.Data(x, y, 
                                with_ = style,
                                inline = True)
            plot.replot(data)
            SpheralGnuPlotCache.append(data)

        return plot

    else:
        return fakeGnuplot()

#-------------------------------------------------------------------------------
# Plot all the nodes in the given data base, and then color the control/ghost
# nodes of the given boundary condition independently.
#-------------------------------------------------------------------------------
def plotBoundaryNodes(dataBase, boundary):

    # First build one set of position pairs for all of the nodes in the
    # data base.
    positions = []
    for nodeList in dataBase.nodeLists():
        for r in nodeList.positions()[:nodeList.numInternalNodes]:
            positions.append((r.x, r.y))

    # Now build a list of the control node positions from the boundary
    # condition.
    controlPositions = []
    for nodeList in dataBase.nodeLists():
        controlNodes = boundary.controlNodes(nodeList)
        for nodeID in controlNodes:
            r = nodeList.positions()[nodeID]
            controlPositions.append((r.x, r.y))

    # Now build a list of the ghost node positions from the boundary
    # condition.
    ghostPositions = []
    for nodeList in dataBase.nodeLists():
        ghostNodes = boundary.ghostNodes(nodeList)
        for nodeID in ghostNodes:
            r = nodeList.positions()[nodeID]
            ghostPositions.append((r.x, r.y))

    # Finally we can plot these various sets of nodes.
    plot = plotXYTuples([positions, controlPositions, ghostPositions])
    return plot

#-------------------------------------------------------------------------------
# Plot the given sequences of (x,y) pairs, each with a distinct color.
#  [ [(x0,y0), (x1,y1), ...],
#    [(x0,y0), (x1,y1), ...],
#    .
#    .
#    .
#    [(x0,y0), (x1,y1), ...] ]
#-------------------------------------------------------------------------------
def plotXYTuples(listOfXYTuples):

    # Find the (min,max) of X and Y for all sets.
    xmin, ymin, xmax, ymax = findPairMinMax(listOfXYTuples[0])
    for seq in listOfXYTuples[1:]:
        xmin0, ymin0, xmax0, ymax0 = findPairMinMax(seq)
        xmin = min(xmin, xmin0)
        ymin = min(ymin, ymin0)
        xmax = max(xmax, xmax0)
        ymax = max(ymax, ymax0)

    # Create our plot result.
    plot = Gnuplot.Gnuplot()
    plot("set size square")

    # Loop over the list of sequences of positions.
    icolor = 0
    for seq in listOfXYTuples:
        icolor += 1

        # Build the local arrays of x and y.
        x = numpy.array([0.0]*len(seq))
        y = numpy.array([0.0]*len(seq))
        for i in xrange(len(seq)):
            x[i] = seq[i][0]
            y[i] = seq[i][1]

        # Build the gnuplot data.
        data = Gnuplot.Data(x, y,
                            with_ = "points",
                            inline = True)
        SpheralGnuPlotCache.append(data)

        # Plot this set of data.
##        plot("set linestyle %i lt %i pt 1" % (icolor, icolor))
        plot.replot(data)

    # That"s it, return the plot.
    return plot

#-------------------------------------------------------------------------------
# Find the (min, max) of a set of pairs.
#-------------------------------------------------------------------------------
def findPairMinMax(listOfPairs):
    minX, minY = 1e90, 1e90
    maxX, maxY = -1e90, -1e90
    for pair in listOfPairs:
        minX = min(minX, pair[0])
        minY = min(minY, pair[1])
        maxX = max(maxX, pair[0])
        maxY = max(maxY, pair[1])
    return minX, minY, maxX, maxY

#-------------------------------------------------------------------------------
# Plot the velocity field as a set of arrows.
# This is maintained here for backward compatibility, as a specialization of
# plotVectorField2d.
#-------------------------------------------------------------------------------
def plotVelocityField2d(dataBase,
                        plotGhosts = False,
                        velMultiplier = 1.0,
                        colorNodeLists = True,
                        colorDomains = False,
                        title = ""):

    return plotVectorField2d(dataBase,
                             dataBase.globalVelocity,
                             plotGhosts,
                             velMultiplier,
                             colorNodeLists,
                             colorDomains,
                             title)

#-------------------------------------------------------------------------------
# Plot an arbitrary vector field as a set of arrows.
#-------------------------------------------------------------------------------
def plotVectorField2d(dataBase, fieldList,
                      plotGhosts = False,
                      vectorMultiplier = 1.0,
                      colorNodeLists = True,
                      colorDomains = False,
                      title = ""):

    assert colorNodeLists + colorDomains <= 1

    mpi, procID, nProcs = loadmpi()

    # Gather the node positions and vectors across all domains.
    # Loop over all the NodeLists.
    localNumNodes = []
    xNodes = []
    yNodes = []
    vxNodes = []
    vyNodes = []
    for i in xrange(dataBase.numNodeLists):
        nodeList = dataBase.nodeLists()[i]
        assert i < fieldList.numFields()
        vectorField = fieldList[i]
        if plotGhosts:
            n = nodeList.numNodes
        else:
            n = nodeList.numInternalNodes
        localNumNodes.append(n)
        xNodes += numpy.array(map(lambda x: x.x, nodeList.positions()[:n]))
        yNodes += numpy.array(map(lambda x: x.y, nodeList.positions()[:n]))
        vxNodes += numpy.array(map(lambda x: x.x, vectorField[:n]))*vectorMultiplier
        vyNodes += numpy.array(map(lambda x: x.y, vectorField[:n]))*vectorMultiplier
    assert len(xNodes) == len(yNodes) == len(vxNodes) == len(vyNodes)
    
    numDomainNodes = [len(xNodes)]
    numNodesPerDomain = mpi.gather(numDomainNodes, 1)
    globalNumNodes = mpi.gather(localNumNodes, len(localNumNodes))
    globalXNodes = mpi.gather(xNodes, len(xNodes))
    globalYNodes = mpi.gather(yNodes, len(yNodes))
    globalVxNodes = mpi.gather(vxNodes, len(vxNodes))
    globalVyNodes = mpi.gather(vyNodes, len(vyNodes))

    if procID == 0:
        plot = Gnuplot.Gnuplot()
        plot("set size square")
        plot.title = title

        if colorDomains:
            cumulativeN = 0
            for domain in xrange(len(numNodesPerDomain)):
                n = numNodesPerDomain[domain]
                x = numpy.array(globalXNodes[cumulativeN:cumulativeN + n])
                y = numpy.array(globalYNodes[cumulativeN:cumulativeN + n])
                vx = numpy.array(globalVxNodes[cumulativeN:cumulativeN + n])
                vy = numpy.array(globalVyNodes[cumulativeN:cumulativeN + n])
                cumulativeN += n
##                plot("set linestyle %i lt %i pt %i" % (domain + 1,
##                                                       domain + 1,
##                                                       domain + 1))
                data = Gnuplot.Data(x, y, vx, vy,
                                    with_ = "vector ls %i" % (domain + 1),
                                    inline = True)
                plot.replot(data)
                SpheralGnuPlotCache.append(data)

        elif colorNodeLists:
            cumulativeN = 0
            for i in xrange(len(globalNumNodes)):
                n = globalNumNodes[i]
                if n > 0:
                    iNodeList = i % dataBase.numNodeLists
                    x = numpy.array(globalXNodes[cumulativeN:cumulativeN + n])
                    y = numpy.array(globalYNodes[cumulativeN:cumulativeN + n])
                    vx = numpy.array(globalVxNodes[cumulativeN:cumulativeN + n])
                    vy = numpy.array(globalVyNodes[cumulativeN:cumulativeN + n])
                    cumulativeN += n
##                    plot("set linestyle %i lt %i pt %i" % (iNodeList + 1,
##                                                           iNodeList + 1,
##                                                           iNodeList + 1))
                    data = Gnuplot.Data(x, y, vx, vy,
                                        with_ = "vector ls %i" % (iNodeList + 1),
                                        inline = True)
                    plot.replot(data)
                    SpheralGnuPlotCache.append(data)

        else:
            x = numpy.array(globalXNodes)
            y = numpy.array(globalYNodes)
            vx = numpy.array(globalVxNodes)
            vy = numpy.array(globalVyNodes)
            data = Gnuplot.Data(x, y, vx, vy,
                                with_ = "vector",
                                inline = True)
            plot.replot(data)
            SpheralGnuPlotCache.append(data)

        return plot

    else:
        SpheralGnuPlotCache.append(data)

#-------------------------------------------------------------------------------
# Generate a regularly spaced sampling of the given FieldList
# The answer is returned in a 2-D numpy array.
#-------------------------------------------------------------------------------
def gridSample(fieldList,
               zFunction = "%s",
               nx = 100,
               ny = 100,
               xmin = None,
               xmax = None,
               ymin = None,
               ymax = None):

    assert nx > 0 and ny > 0

    mpi, procID, nProcs = loadmpi()

    # Set up our return value array.
    xValues = numpy.array([[0.0]*nx]*ny)
    yValues = numpy.array([[0.0]*nx]*ny)
    zValues = numpy.array([[0.0]*nx]*ny)

    # Gather the fieldList info across all processors to process 0.
    localNumNodes = []
    localX = []
    localY = []
    for field in fieldList.fields():
        n = field.nodeList().numNodes
        localNumNodes.append(n)
        for r in field.nodeList().positions():
            localX.append(r.x)
            localY.append(r.y)
    globalNumNodes = mpi.gather(localNumNodes, len(localNumNodes))
    globalX = mpi.gather(localX, len(localX))
    globalY = mpi.gather(localY, len(localY))

    # If the user did not specify the sampling volume, then find the min and
    # max node positions.
    if xmin == None:
        xmin = min(localX)
    if ymin == None:
        ymin = min(localY)
    if xmax == None:
        xmax = max(localX)
    if ymax == None:
        ymax = max(localY)
    xmin = mpi.allreduce(xmin, mpi.MIN)
    ymin = mpi.allreduce(ymin, mpi.MIN)
    xmax = mpi.allreduce(xmax, mpi.MAX)
    ymax = mpi.allreduce(ymax, mpi.MAX)

    assert xmax > xmin
    assert ymax > ymin

    # Figure out the sizes of the bins we're going to be sampling in
    dx = (xmax - xmin)/nx
    dy = (ymax - ymin)/ny

    # Loop over all the grid sampling positions, and figure out this processors
    # contribution.
    for iy in xrange(ny):
        for ix in xrange(nx):
            xValues[iy][ix] = xmin + (ix + 0.5)*dx
            yValues[iy][ix] = ymin + (iy + 0.5)*dy
            r = Vector2d(xValues[iy][ix], yValues[iy][ix])
            z = fieldList.sample(r)
            localZ = eval(zFunction % "z")
            globalZ = mpi.reduce(localZ, mpi.SUM)
            if procID == 0:
                print "%i %i %i %s %g %g" % (mpi.rank, ix, iy, r, z, localZ)
                print "%i %g" % (mpi.rank, globalZ)
                zValues[iy][ix] = globalZ

    return xValues, yValues, zValues

#-------------------------------------------------------------------------------
# Plot the energy history of the given conservation object.
#-------------------------------------------------------------------------------
def plotEHistory(conserve):
    mpi, procID, nProcs = loadmpi()
    if procID == 0:
        t = conserve.timeHistory
        E = conserve.EHistory
        KE = conserve.KEHistory
        TE = conserve.TEHistory
        UE = conserve.EEHistory
        Edata = Gnuplot.Data(t, E,
                             with_ = "lines",
                             title = "Total Energy",
                             inline = True)
        KEdata = Gnuplot.Data(t, KE,
                              with_ = "lines",
                              title = "Kinetic Energy",
                              inline = True)
        TEdata = Gnuplot.Data(t, TE,
                              with_ = "lines",
                              title = "Thermal Energy",
                              inline = True)
        UEdata = Gnuplot.Data(t, UE,
                              with_ = "lines",
                              title = "Potential Energy",
                              inline = True)
        plot = Gnuplot.Gnuplot()
        plot.replot(Edata)
        plot.replot(KEdata)
        plot.replot(TEdata)
        plot.replot(UEdata)
        plot.replot()
        SpheralGnuPlotCache.extend([Edata, KEdata, TEdata, UEdata])
        return plot
    else:
        return fakeGnuplot()
