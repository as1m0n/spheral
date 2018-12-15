import inspect
from PYB11Generator import *

#-------------------------------------------------------------------------------
# FieldListSet
#-------------------------------------------------------------------------------
@PYB11template("Dimension")
@PYB11module("SpheralField")
class FieldListSet:

    # Constructors
    def pyinit(self,):
        "Default constructor"

    # Attributes
    #ScalarFieldLists = PYB11readwrite(doc="The FieldList<Dim, double> set", returnpolicy="reference_internal")
    #VectorFieldLists = PYB11readwrite(doc="The FieldList<Dim, Vector> set", returnpolicy="reference_internal")
    #TensorFieldLists = PYB11readwrite(doc="The FieldList<Dim, Tensor> set", returnpolicy="reference_internal")
    #SymTensorFieldLists = PYB11readwrite(doc="The FieldList<Dim, SymTensor> set", returnpolicy="reference_internal")

    ScalarFieldLists = PYB11property("BLAGO", # "std::vector<FieldList<%(Dimension)s, typename %(Dimension)s::Scalar>>&",
                                     getterraw = "[](FieldListSet<%(Dimension)s>& self) -> std::vector<FieldList<%(Dimension)s, typename %(Dimension)s::Scalar>>* { return &(self.ScalarFieldLists); }",
                                     setterraw = "[](FieldListSet<%(Dimension)s>& self, std::vector<FieldList<%(Dimension)s, %(Dimension)s::Scalar>>& x) { self.ScalarFieldLists = x; }",
                                     returnpolicy = "reference_internal")
    VectorFieldLists = PYB11property("BLAGO", # "std::vector<FieldList<%(Dimension)s, typename %(Dimension)s::Vector>>&",
                                     getterraw = "[](FieldListSet<%(Dimension)s>& self) -> std::vector<FieldList<%(Dimension)s, typename %(Dimension)s::Vector>>* { return &(self.VectorFieldLists); }",
                                     setterraw = "[](FieldListSet<%(Dimension)s>& self, std::vector<FieldList<%(Dimension)s, %(Dimension)s::Vector>>& x) { self.VectorFieldLists = x; }",
                                     returnpolicy = "reference_internal")
    TensorFieldLists = PYB11property("BLAGO", # "std::vector<FieldList<%(Dimension)s, typename %(Dimension)s::Tensor>>&",
                                     getterraw = "[](FieldListSet<%(Dimension)s>& self) -> std::vector<FieldList<%(Dimension)s, typename %(Dimension)s::Tensor>>* { return &(self.TensorFieldLists); }",
                                     setterraw = "[](FieldListSet<%(Dimension)s>& self, std::vector<FieldList<%(Dimension)s, %(Dimension)s::Tensor>>& x) { self.TensorFieldLists = x; }",
                                     returnpolicy = "reference_internal")
    SymTensorFieldLists = PYB11property("BLAGO", # "std::vector<FieldList<%(Dimension)s, typename %(Dimension)s::SymTensor>>&",
                                        getterraw = "[](FieldListSet<%(Dimension)s>& self) -> std::vector<FieldList<%(Dimension)s, typename %(Dimension)s::SymTensor>>* { return &(self.SymTensorFieldLists); }",
                                        setterraw = "[](FieldListSet<%(Dimension)s>& self, std::vector<FieldList<%(Dimension)s, %(Dimension)s::SymTensor>>& x) { self.SymTensorFieldLists = x; }",
                                        returnpolicy = "reference_internal")


    # VectorFieldLists = PYB11property("std::vector<FieldList<%(Dimension)s, typename %(Dimension)s::Vector>>&",
    #                                  getterraw = "[](FieldListSet<%(Dimension)s>& self) { return self.VectorFieldLists; }",
    #                                  setterraw = "[](FieldListSet<%(Dimension)s>& self, std::vector<FieldList<%(Dimension)s, %(Dimension)s::Vector>>& x) { self.VectorFieldLists = x; }",
    #                                  returnpolicy = "reference_internal")
    # TensorFieldLists = PYB11property("std::vector<FieldList<%(Dimension)s, typename %(Dimension)s::Tensor>>&",
    #                                  getterraw = "[](FieldListSet<%(Dimension)s>& self) { return self.TensorFieldLists; }",
    #                                  setterraw = "[](FieldListSet<%(Dimension)s>& self, std::vector<FieldList<%(Dimension)s, %(Dimension)s::Tensor>>& x) { self.TensorFieldLists = x; }",
    #                                  returnpolicy = "reference_internal")
    # SymTensorFieldLists = PYB11property("std::vector<FieldList<%(Dimension)s, typename %(Dimension)s::SymTensor>>&",
    #                                     getterraw = "[](FieldListSet<%(Dimension)s>& self) { return self.SymTensorFieldLists; }",
    #                                     setterraw = "[](FieldListSet<%(Dimension)s>& self, std::vector<FieldList<%(Dimension)s, %(Dimension)s::SymTensor>>& x) { self.SymTensorFieldLists = x; }",
    #                                     returnpolicy = "reference_internal")