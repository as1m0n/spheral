#-------------------------------------------------------------------------------
# Polyhedron
#-------------------------------------------------------------------------------
from PYB11Generator import *

@PYB11cppname("GeomPolyhedron")
class Polyhedron:

    typedefs = """
    typedef GeomPolyhedron Polyhedron;
    typedef GeomPolyhedron::Vector Vector;
    typedef GeomPolyhedron::Facet Facet;
"""

    #...........................................................................
    # Constructors
    def pyinit(self):
        "Default constructor"

    def pyinit1(self,
                points = "const std::vector<Vector>&"):
        """Note this constructor constructs the convex hull of the given points,
meaning that the full set of points passed in may not appear in the vertices."""

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
        "Test if the given point is internal to the polyhedron."
        return "bool"

    @PYB11const
    def convexContains(self,
                       point = "const Vector&",
                       countBoundary = ("const bool", "true"),
                       tol = ("const double", "1.0e-8")):
        "Test if the given point is internal to the polyhedron (assumes convexity)."
        return "bool"

    @PYB11const
    def intersect(self,
                  rhs = "const Polyhedron&"):
        "Test if we intersect another polyhedron."
        return "bool"

    @PYB11const
    def convexIntersect(self,
                        rhs = "const Polyhedron&"):
        "Test if we intersect another polyhedron (assumes convexity)"
        return "bool"

    @PYB11const
    def intersect(self,
                  rhs = "const std::pair<Vector, Vector>&"):
        "Test if we intersect a box represented by a min/max pair of coordinates."
        return "bool"

    @PYB11const
    def edges(self):
        "Get the edges as integer (node) pairs."
        return "std::vector<std::pair<unsigned, unsigned> >"

    @PYB11const
    def facetVertices(self):
        "Spit out a vector<vector<unsigned> > that encodes the facets."
        return "std::vector<std::vector<unsigned> >"

    @PYB11const
    def facetNormals(self):
        "Compute effective surface normals at the facets."
        return "std::vector<Vector>"

    def reconstruct(self,
                    vertices = "const std::vector<Vector>&",
                    facetVertices = "const std::vector<std::vector<unsigned> >&",
                    facetNormals = "const std::vector<Vector>&"):
        """Reconstruct the internal data given a set of vertices, vertex
indices that define the facets, and outward normals at the facets."""
        return "void"

    @PYB11const
    def closestFacet(self, p = "const Vector&"):
        "Find the facet closest to the given point."
        return "unsigned"

    @PYB11const
    def distance(self, p="const Vector&"):
        "Compute the minimum distance to a point."
        return "double"

    @PYB11const
    def closestPoint(self, p="const Vector&"):
        "Find the point in the polyhedron closest to the given point."
        return "Vector"

    @PYB11const
    def convex(self):
        "Test if the polyhedron is convex"
        return "bool"

    def setBoundingBox(self):
        "Set the internal bounding box"
        return "void"

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
    centroid = PYB11property("Vector")
    vertices = PYB11property("std::vector<Vector>&", returnpolicy="reference_internal")
    facets = PYB11property("std::vector<Facet>&", returnpolicy="reference_internal")
    vertexUnitNorms = PYB11property("std::vector<Vector>&", returnpolicy="reference_internal")
    vertexFacetConnectivity = PYB11property("std::vector<std::vector<unsigned> >&", returnpolicy="reference_internal")
    facetFacetConnectivity = PYB11property("std::vector<std::vector<unsigned> >&", returnpolicy="reference_internal")
    xmin = PYB11property("const Vector&", returnpolicy="reference_internal")
    xmax = PYB11property("const Vector&", returnpolicy="reference_internal")
    volume = PYB11property("double")
