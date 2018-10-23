#-------------------------------------------------------------------------------
# MortonOrderRedistributeNodes
#-------------------------------------------------------------------------------
from PYB11Generator import *
from SpaceFillingCurveRedistributeNodes import *

@PYB11template("Dimension")
class MortonOrderRedistributeNodes(SpaceFillingCurveRedistributeNodes):

    typedefs = """
    typedef typename KeyTraits::Key Key;
    typedef typename %(Dimension)s::Scalar Scalar;
    typedef typename %(Dimension)s::Vector Vector;
    typedef typename %(Dimension)s::Tensor Tensor;
    typedef typename %(Dimension)s::SymTensor SymTensor;
"""

    #...........................................................................
    # Constructors
    def pyinit(self,
               dummy = "const double",
               minNodesPerDomainFraction = ("const double", "0.5"),
               maxNodesPerDomainFraction = ("const double", "1.5"),
               workBalance = ("const bool", "true"),
               localReorderOnly = ("const bool", "false")):
        "Constructor"

    #...........................................................................
    # Virtual methods
    @PYB11virtual
    @PYB11const
    def computeHashedIndices(self,
                             dataBase = "const DataBase<%(Dimension)s>&"):
        "This is the required method for all descendant classes."
        return "FieldList<%(Dimension)s, Key> "
