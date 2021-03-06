"""
This module provides the fundamental Spheral Geometry types (Vector, Tensor, 
etc.) and associated methods such as products and eigenvalues.
"""

from PYB11Generator import *
import types

# Forcibly override the common preamble
PYB11preamble = ""

# Define some useful type collections we're going to be wrapping in this module.
geomtypes = ["Vector", "Tensor", "SymTensor", "ThirdRankTensor", "FourthRankTensor", "FifthRankTensor", "FacetedVolume"]

PYB11namespaces = ["Spheral"]

# for ndim in (1, 2, 3):
#     preamble += "typedef GeomPlane<Dim<%i>> Plane%id;\n" % (ndim, ndim)
#     for gtype in geomtypes:
#         preamble += "typedef Dim<%i>::%s %s%id;\n" % (ndim, gtype, gtype, ndim)

# Include files
PYB11includes = ['"Geometry/Dimension.hh"',
                 '"Geometry/GeomVector.hh"',
                 '"Geometry/Geom3Vector.hh"',
                 '"Geometry/GeomTensor.hh"',
                 '"Geometry/GeomSymmetricTensor.hh"',
                 '"Geometry/GeomThirdRankTensor.hh"',
                 '"Geometry/GeomFourthRankTensor.hh"',
                 '"Geometry/GeomFifthRankTensor.hh"',
                 '"Geometry/EigenStruct.hh"',
                 '"Geometry/computeEigenValues.hh"',
                 '"Geometry/GeomPolygon.hh"',
                 '"Geometry/GeomPolyhedron.hh"',
                 '"Geometry/GeomFacet2d.hh"',
                 '"Geometry/GeomFacet3d.hh"',
                 '"Geometry/invertRankNTensor.hh"',
                 '"Geometry/innerProduct.hh"',
                 '"Geometry/outerProduct.hh"',
                 '"Geometry/innerDoubleProduct.hh"',
                 '"Geometry/aggregateFacetedVolumes.hh"',
                 '"Geometry/CellFaceFlag.hh"',
                 '"Field/Field.hh"',
                 '"Utilities/DataTypeTraits.hh"',

                 '<vector>',
                 '<sstream>']

# STL containers
for element in geomtypes:
    for ndim in (1, 2, 3):
        exec('''
vector_of_%(mangle)s = PYB11_bind_vector("%(element)s", opaque=True, local=False)
vector_of_vector_of_%(mangle)s = PYB11_bind_vector("std::vector<%(element)s>", opaque=True, local=False)
''' % {"element": "Dim<" + str(ndim) + ">::" + element,
       "mangle" : element + str(ndim) + "d"})
vector_of_Facet2d = PYB11_bind_vector("GeomFacet2d", opaque=True, local=False)
vector_of_Facet3d = PYB11_bind_vector("GeomFacet3d", opaque=True, local=False)
vector_of_Plane1d = PYB11_bind_vector("GeomPlane<Dim<1>>", opaque=True, local=False)
vector_of_Plane2d = PYB11_bind_vector("GeomPlane<Dim<2>>", opaque=True, local=False)
vector_of_Plane3d = PYB11_bind_vector("GeomPlane<Dim<3>>", opaque=True, local=False)
vector_of_CellFaceFlag = PYB11_bind_vector("CellFaceFlag", opaque=True, local=False)

# Get the objects wrapped in other files.
from Vector import Vector1d, Vector2d, Vector3d
from Tensor import Tensor1d, Tensor2d, Tensor3d
from SymTensor import SymTensor1d, SymTensor2d, SymTensor3d
from ThirdRankTensor import ThirdRankTensor1d, ThirdRankTensor2d, ThirdRankTensor3d
from FourthRankTensor import FourthRankTensor1d, FourthRankTensor2d, FourthRankTensor3d
from FifthRankTensor import FifthRankTensor1d, FifthRankTensor2d, FifthRankTensor3d
from EigenStruct import EigenStruct1d, EigenStruct2d, EigenStruct3d
from Plane import Plane1d, Plane2d, Plane3d
from Box1d import *
from Polygon import *
from Polyhedron import *
from Facet2d import *
from Facet3d import *
from CellFaceFlag import *

#-------------------------------------------------------------------------------
# Vector standalone functions
#-------------------------------------------------------------------------------
@PYB11template("nDim")
def elementWiseMin(lhs = "const Dim<%(nDim)s>::Vector&",
                   rhs = "const Dim<%(nDim)s>::Vector&"):
    "Find the coordinate by coordinate minimum of two Vectors."
    return "Dim<%(nDim)s>::Vector"

@PYB11template("nDim")
def elementWiseMax(lhs = "const Dim<%(nDim)s>::Vector&",
                   rhs = "const Dim<%(nDim)s>::Vector&"):
    "Find the coordinate by coordinate maximum of two Vectors."
    return "Dim<%(nDim)s>::Vector"

elementWiseMin1 = PYB11TemplateFunction(elementWiseMin, template_parameters="1", pyname="elementWiseMin")
elementWiseMin2 = PYB11TemplateFunction(elementWiseMin, template_parameters="2", pyname="elementWiseMin")
elementWiseMin3 = PYB11TemplateFunction(elementWiseMin, template_parameters="3", pyname="elementWiseMin")
                                        
elementWiseMax1 = PYB11TemplateFunction(elementWiseMax, template_parameters="1", pyname="elementWiseMax")
elementWiseMax2 = PYB11TemplateFunction(elementWiseMax, template_parameters="2", pyname="elementWiseMax")
elementWiseMax3 = PYB11TemplateFunction(elementWiseMax, template_parameters="3", pyname="elementWiseMax")

#-------------------------------------------------------------------------------
# invertRankNTensor template
#-------------------------------------------------------------------------------
@PYB11template("TensorType")
def invertRankNTensor(tensor = "const %(TensorType)s&"):
    "Compute the inverse of a tensor."
    return "%(TensorType)s"

invertRankNTensor1 = PYB11TemplateFunction(invertRankNTensor,
                                           template_parameters = "Dim<1>::Tensor",
                                           pyname = "invertRankNTensor")
invertRankNTensor2 = PYB11TemplateFunction(invertRankNTensor,
                                           template_parameters = "Dim<1>::SymTensor",
                                           pyname = "invertRankNTensor")
invertRankNTensor3 = PYB11TemplateFunction(invertRankNTensor,
                                           template_parameters = "Dim<1>::FourthRankTensor",
                                           pyname = "invertRankNTensor")

#-------------------------------------------------------------------------------
# computeEigenValues
#-------------------------------------------------------------------------------
@PYB11template("Dim")
def computeEigenValues(field = "const Field<%(Dim)s, %(Dim)s::SymTensor>&",
                       eigenValues = "Field<%(Dim)s, %(Dim)s::Vector>&",
                       eigenVectors = "Field<%(Dim)s, %(Dim)s::Tensor>&"):
    "Compute the eigenvalues for a field of symmetric tensors."
    return "void"

computeEigenValues1 = PYB11TemplateFunction(computeEigenValues,
                                            template_parameters = "Dim<1>",
                                            pyname = "computeEigenValues")
computeEigenValues2 = PYB11TemplateFunction(computeEigenValues,
                                            template_parameters = "Dim<2>",
                                            pyname = "computeEigenValues")
computeEigenValues3 = PYB11TemplateFunction(computeEigenValues,
                                            template_parameters = "Dim<3>",
                                            pyname = "computeEigenValues")

#-------------------------------------------------------------------------------
# Inner product (with a double)
#-------------------------------------------------------------------------------
@PYB11template("ValueType")
def innerProductScalar(A = "const double&",
                       B = "const %(ValueType)s&"):
    "Inner product with a scalar."
    return "%(ValueType)s"

@PYB11template("ValueType")
def innerProductScalarR(A = "const %(ValueType)s&",
                        B = "const double&"):
    "Inner product with a scalar."
    return "%(ValueType)s"

for VT in ("Vector", "Tensor", "SymTensor", "ThirdRankTensor", "FourthRankTensor", "FifthRankTensor"):
    for ndim in (1, 2, 3):
        exec("""
innerProduct%(VT)sScalar%(ndim)id = PYB11TemplateFunction(innerProductScalar,
                                                          template_parameters = "Dim<%(ndim)i>::%(VT)s",
                                                          pyname = "innerProduct",
                                                          cppname = "innerProduct<Dim<%(ndim)i>::%(VT)s>")
innerProductScalar%(VT)s%(ndim)id = PYB11TemplateFunction(innerProductScalarR,
                                                          template_parameters = "Dim<%(ndim)i>::%(VT)s",
                                                          pyname = "innerProduct",
                                                          cppname = "innerProduct<Dim<%(ndim)i>::%(VT)s>")
""" % {"VT" : VT,
       "ndim" : ndim})

#-------------------------------------------------------------------------------
# General inner products
#-------------------------------------------------------------------------------
@PYB11template("AType", "BType", "ReturnType")
def innerProduct(A = "const %(AType)s&",
                 B = "const %(BType)s&"):
    "Inner product (%(AType)s . %(BType)s -> %(ReturnType)s"
    return "%(ReturnType)s"

# Map inner product types to result
IPRT = {("Vector", "Vector")           : "double",
        ("Vector", "Tensor")           : "Vector",
        ("Vector", "SymTensor")        : "Vector",
        ("Vector", "ThirdRankTensor")  : "Tensor",
        ("Vector", "FourthRankTensor") : "ThirdRankTensor",

        ("Tensor", "Tensor")           : "Tensor",
        ("Tensor", "SymTensor")        : "Tensor",
        ("SymTensor", "Tensor")        : "Tensor",
        ("Tensor", "SymTensor")        : "Tensor",
        ("SymTensor", "SymTensor")     : "Tensor",

        ("Tensor", "ThirdRankTensor")  : "ThirdRankTensor",
        ("Tensor", "FourthRankTensor") : "FourthRankTensor",

        ("SymTensor", "ThirdRankTensor")  : "ThirdRankTensor",
        ("SymTensor", "FourthRankTensor") : "FourthRankTensor",

        ("ThirdRankTensor", "ThirdRankTensor")  : "FourthRankTensor",
        ("ThirdRankTensor", "FourthRankTensor") : "FifthRankTensor",
        }
for A, B in dict(IPRT):
    IPRT[(B, A)] = IPRT[(A, B)]

for A, B in IPRT:
    for ndim in (1, 2, 3):
        exec("""
a = "Dim<%(ndim)i>::" + "%(A)s"
b = "Dim<%(ndim)i>::" + "%(B)s"
if "%(RT)s" == "double":
    rt = "%(RT)s"
else:
    rt = "Dim<%(ndim)i>::" + "%(RT)s"
innerProduct%(A)s%(B)s%(ndim)id = PYB11TemplateFunction(innerProduct,
                                                        template_parameters = (a, b, rt),
                                                        pyname = "innerProduct",
                                                        cppname = "innerProduct<Dim<%(ndim)i>>")
""" % {"A"  : A,
       "B"  : B,
       "RT" : IPRT[(A, B)],
       "ndim" : ndim})
             

#-------------------------------------------------------------------------------
# General outer products
#-------------------------------------------------------------------------------
@PYB11template("AType", "BType", "ReturnType")
def outerProduct(A = "const %(AType)s&",
                 B = "const %(BType)s&"):
    "Outer product (%(AType)s x %(BType)s -> %(ReturnType)s"
    return "%(ReturnType)s"

# Map outer product types to result
OPRT = {("Scalar", "Scalar")           : "Scalar",
        ("Scalar", "Vector")           : "Vector",
        ("Scalar", "Tensor")           : "Tensor",
        ("Scalar", "SymTensor")        : "SymTensor",
        ("Scalar", "ThirdRankTensor")  : "ThirdRankTensor",
        ("Scalar", "FourthRankTensor") : "FourthRankTensor",
        ("Scalar", "FifthRankTensor")  : "FifthRankTensor",

        ("Vector",           "Scalar") : "Vector",
        ("Tensor",           "Scalar") : "Tensor",
        ("SymTensor",        "Scalar") : "SymTensor",
        ("ThirdRankTensor",  "Scalar") : "ThirdRankTensor",
        ("FourthRankTensor", "Scalar") : "FourthRankTensor",
        ("FifthRankTensor",  "Scalar") : "FifthRankTensor",

        ("Vector", "Vector")           : "Tensor",
        ("Vector", "Tensor")           : "ThirdRankTensor",
        ("Vector", "SymTensor")        : "ThirdRankTensor",
        ("Vector", "ThirdRankTensor")  : "FourthRankTensor",
        ("Vector", "FourthRankTensor") : "FifthRankTensor",

        ("Tensor", "Tensor")           : "FourthRankTensor",
        ("Tensor", "SymTensor")        : "FourthRankTensor",
        ("Tensor", "ThirdRankTensor")  : "FifthRankTensor",

        ("SymTensor", "ThirdRankTensor")  : "FifthRankTensor",
        }
for A, B in dict(OPRT):
    OPRT[(B, A)] = OPRT[(A, B)]

for A, B in OPRT:
    for ndim in (1, 2, 3):
        exec("""
a = "Dim<%(ndim)i>::" + "%(A)s"
b = "Dim<%(ndim)i>::" + "%(B)s"
if "%(RT)s" == "double":
    rt = "%(RT)s"
else:
    rt = "Dim<%(ndim)i>::" + "%(RT)s"
outerProduct%(A)s%(B)s%(ndim)id = PYB11TemplateFunction(outerProduct,
                                                        template_parameters = (a, b, rt),
                                                        pyname = "outerProduct",
                                                        cppname = "outerProduct<Dim<%(ndim)i>>")
""" % {"A"  : A,
       "B"  : B,
       "RT" : OPRT[(A, B)],
       "ndim" : ndim})

#-------------------------------------------------------------------------------
# Inner double product
#-------------------------------------------------------------------------------
@PYB11template("AType", "BType", "ReturnType")
def innerDoubleProduct(A = "const %(AType)s&",
                       B = "const %(BType)s&"):
    "Inner double product (%(AType)s : %(BType)s -> %(ReturnType)s"
    return "%(ReturnType)s"

# Map product types to result
IDPRT = {("Tensor", "Tensor")               : "double",
         ("Tensor", "SymTensor")            : "double",
         ("Tensor", "ThirdRankTensor")      : "Vector",
         ("Tensor", "FourthRankTensor")     : "Tensor",
         ("Tensor", "FifthRankTensor")      : "ThirdRankTensor",

         ("SymTensor", "SymTensor")         : "double",
         ("SymTensor", "ThirdRankTensor")   : "Vector",
         ("SymTensor", "FourthRankTensor")  : "Tensor",
         ("SymTensor", "FifthRankTensor")   : "ThirdRankTensor",

         ("ThirdRankTensor", "ThirdRankTensor") : "Tensor",
         ("ThirdRankTensor", "FourthRankTensor"): "ThirdRankTensor",
         ("ThirdRankTensor", "FifthRankTensor") : "FourthRankTensor",

         ("FourthRankTensor", "FourthRankTensor") : "FourthRankTensor",
         ("FourthRankTensor", "FifthRankTensor")  : "FifthRankTensor",
}
for A, B in dict(IDPRT):
    IDPRT[(B, A)] = IDPRT[(A, B)]

for A, B in IDPRT:
    for ndim in (1, 2, 3):
        exec("""
a = "Dim<%(ndim)i>::" + "%(A)s"
b = "Dim<%(ndim)i>::" + "%(B)s"
if "%(RT)s" == "double":
    rt = "%(RT)s"
else:
    rt = "Dim<%(ndim)i>::" + "%(RT)s"
innerDoubleProduct%(A)s%(B)s%(ndim)id = PYB11TemplateFunction(innerDoubleProduct,
                                                              template_parameters = (a, b, rt),
                                                              pyname = "innerDoubleProduct",
                                                              cppname = "innerDoubleProduct<Dim<%(ndim)i>>")
""" % {"A"  : A,
       "B"  : B,
       "RT" : IDPRT[(A, B)],
       "ndim" : ndim})
