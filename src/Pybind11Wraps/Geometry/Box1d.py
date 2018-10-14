#-------------------------------------------------------------------------------
# Box1d
#-------------------------------------------------------------------------------
from PYB11Generator import *

class Box1d:

    typedefs = """
    typedef Box1d::Vector Vector;
"""

    #...........................................................................
    # Constructors
    def pyinit(self):
        "Default constructor"

    def pyinit1(self,
                points = "const std::vector<Vector>&"):
        "Construct as a convex hull"

    def pyinit2(self,
                points = "const std::vector<Vector>&",
                facetIndices = "const std::vector<std::vector<unsigned> >&"):
        "Construct with explicit vertices and facets"

    #...........................................................................
    # Methods
    @PYB11const
    def contains(self,
                 point = "const Vector&",
                 countBoundary = ("const bool", "true"),
                 tol = ("const double", "1.0e-8")):
        "Test if the given point is internal to the box."
        return "bool"

    @PYB11const
    def convexContains(self,
                       point = "const Vector&",
                       countBoundary = ("const bool", "true"),
                       tol = ("const double", "1.0e-8")):
        "Test if the given point is internal to the box (assumes convexity)."
        return "bool"

    @PYB11const
    def intersect(self,
                  rhs = "const Box1d&"):
        "Test if we intersect another box."
        return "bool"

    @PYB11const
    def convexIntersect(self,
                        rhs = "const Box1d&"):
        "Test if we intersect another box (assumes convexity)"
        return "bool"

    @PYB11const
    def intersect(self,
                  rhs = "const std::pair<Vector, Vector>&"):
        "Test if we intersect another box represented by a min/max pair of coordinates."
        return "bool"

    @PYB11const
    def distance(self, p="const Vector&"):
        "Compute the minimum distance to a point."
        return "double"

    @PYB11const
    def closestPoint(self, p="const Vector&"):
        "Find the point in the box closest to the given point."
        return "Vector"

    #...........................................................................
    # Operators
    def __iadd__(self, rhs="Vector()"):
        return

    def __isub__(self, rhs="Vector()"):
        return

    def __add__(self, rhs="Vector()"):
        return

    def __sub__(self, rhs="Vector()"):
        return

    def __imul__(self, rhs="double()"):
        return
    
    def __idiv__(self, rhs="double()"):
        return
    
    def __mul__(self, rhs="double()"):
        return
    
    def __div__(self, rhs="double()"):
        return
    
    def __eq__(self):
        return

    def __ne__(self):
        return

    #...........................................................................
    # Properties
    center = PYB11property("const Vector&", "center", "center")
    extent = PYB11property("double", "extent", "extent")
    xmin = PYB11property("const Vector&", returnpolicy="reference_internal")
    xmax = PYB11property("const Vector&", returnpolicy="reference_internal")
    centroid = PYB11property("Vector")
    vertices = PYB11property("const std::vector<Vector>&", returnpolicy="reference_internal")
    facetVertices = PYB11property("std::vector<std::vector<unsigned> >")
    volume = PYB11property("double")
