from SpheralModules.Spheral import *
from SpheralModules.Spheral.CRKSPHSpace import *
from SpheralModules.Spheral.NodeSpace import *
from SpheralModules.Spheral.PhysicsSpace import *
from SpheralModules.Spheral.PhysicsSpace import *
from SpheralModules.Spheral.KernelSpace import *
from SpheralModules.Spheral.BoundarySpace import *

from spheralDimensions import spheralDimensions
dims = spheralDimensions()

#-------------------------------------------------------------------------------
# The generic SolidCRKSPHHydro pattern.
#-------------------------------------------------------------------------------
SolidCRKSPHHydroRZFactoryString = """
class %(classname)s(SolidCRKSPHHydroBaseRZ):

    def __init__(self,
                 Q,
                 W,
                 WPi = None,
                 filter = 1.0,
                 cfl = 0.25,
                 useVelocityMagnitudeForDt = False,
                 compatibleEnergyEvolution = True,
                 evolveTotalEnergy = False,
                 XSPH = True,
                 densityUpdate = RigorousSumDensity,
                 HUpdate = IdealH,
                 correctionOrder = LinearOrder,
                 volumeType = CRKVoronoiVolume,
                 detectSurfaces = False,
                 detectThreshold = 0.05,
                 sweepAngle = 0.8,
                 detectRange = 1.0,
                 epsTensile = 0.0,
                 nTensile = 4.0,
                 etaMinAxis = 0.1):
        self._smoothingScaleMethod = %(smoothingScaleMethod)s2d()
        if WPi is None:
            WPi = W
        SolidCRKSPHHydroBaseRZ.__init__(self,
                                        self._smoothingScaleMethod,
                                        Q,
                                        W,
                                        WPi,
                                        filter,
                                        cfl,
                                        useVelocityMagnitudeForDt,
                                        compatibleEnergyEvolution,
                                        evolveTotalEnergy,
                                        XSPH,
                                        densityUpdate,
                                        HUpdate,
                                        correctionOrder,
                                        volumeType,
                                        detectSurfaces,
                                        detectThreshold,
                                        sweepAngle,
                                        detectRange,
                                        epsTensile,
                                        nTensile)
        self.zaxisBC = AxisBoundaryRZ(etaMinAxis)
        self.appendBoundary(self.zaxisBC)
        return
"""

#-------------------------------------------------------------------------------
# Make 'em.
#-------------------------------------------------------------------------------
exec(SolidCRKSPHHydroRZFactoryString % {"classname"            : "SolidCRKSPHHydroRZ",
                                        "smoothingScaleMethod" : "SPHSmoothingScale"})
exec(SolidCRKSPHHydroRZFactoryString % {"classname"            : "SolidACRKSPHHydroRZ",
                                        "smoothingScaleMethod" : "ASPHSmoothingScale"})
