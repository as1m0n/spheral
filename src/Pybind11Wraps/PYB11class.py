#--------------------------------------------------------------------------------
# PYB11class
#
# Stuff for handling classes in pybind11
#--------------------------------------------------------------------------------
from PYB11utils import *
from PYB11property import *
from PYB11Trampoline import *
from PYB11enum import PYB11enum
import copy, StringIO

#-------------------------------------------------------------------------------
# PYB11generateModuleClasses
#
# Bind the classes in the module
#-------------------------------------------------------------------------------
def PYB11generateModuleClasses(modobj, ss):
    klasses = PYB11classes(modobj)
    for kname, klass in klasses:
        klassattrs = PYB11attrs(klass)
        if not klassattrs["ignore"]:
            PYB11generateClass(klass, klassattrs, ss)

    # Now look for any template class instantiations.
    templates = PYB11classTemplateInsts(modobj)
    for (ktname, inst) in templates:
        inst(ktname, ss)
    return

#-------------------------------------------------------------------------------
# PYB11generateModuleTrampolines
#
# Generate trampolines for any classes with virtual methods.
#-------------------------------------------------------------------------------
def PYB11generateModuleTrampolines(modobj, ss):
    klasses = PYB11classes(modobj)
    for kname, klass in klasses:
        if PYB11virtualClass(klass):
            PYB11generateTrampoline(klass, ss)

    # templates = [x for x in dir(modobj) if isinstance(eval("modobj.%s" % x), PYB11TemplateClass)]
    # for ktname in templates:
    #     inst = eval("modobj.%s" % ktname)
    #     if PYB11virtualClass(inst.klass_template):
    #         inst.makeTrampoline(ss)
    return

#--------------------------------------------------------------------------------
# Make a class template instantiation
#--------------------------------------------------------------------------------
class PYB11TemplateClass:
    __order = 0

    def __init__(self,
                 klass_template,
                 template_parameters,
                 cppname = None,
                 pyname = None,
                 docext = ""):
        self.klass_template = klass_template
        self.cppname = cppname
        self.pyname = pyname
        self.docext = docext

        # Create the template parameter dictionary
        self.template_parameters = {}
        klassattrs = PYB11attrs(self.klass_template)
        if isinstance(template_parameters, str):
            assert len(klassattrs["template"]) == 1
            self.template_parameters[klassattrs["template"][0]] = template_parameters
        elif isinstance(template_parameters, tuple):
            assert len(klassattrs["template"]) == len(template_parameters)
            for name, val in zip(klassattrs["template"], template_parameters):
                self.template_parameters[name] = val
        else:
            assert isinstance(template_parameters, dict)
            for key in klassattrs["template"]:
                assert key in template_parameters
            self.template_parameters = template_parameters
            
        # Record the order of instantiations
        self.order = PYB11TemplateClass.__order + 1
        PYB11TemplateClass.__order += 1
        return

    def __call__(self, pyname, ss):
        klassattrs = self.mangleNames(pyname)
        if self.klass_template.__doc__:
            doc0 = copy.deepcopy(self.klass_template.__doc__)
            self.klass_template.__doc__ += self.docext
        PYB11generateClass(self.klass_template, klassattrs, ss)
        if self.klass_template.__doc__:
            self.klass_template.__doc__ = doc0
        return

    def makeTrampoline(self, pyname, ss):
        klassattrs = self.mangleNames(pyname)
        PYB11generateTrampoline(self.klass_template, klassattrs, ss)
        return

    # Do some template mangling (and magically put the template parameters in scope).
    def mangleNames(self, pyname):
        klassattrs = PYB11attrs(self.klass_template)
        template_ext = "<"
        doc_ext = ""
        for name in klassattrs["template"]:
            val = self.template_parameters[name]
            exec("%s = '%s'" % (name, val))
            template_ext += "%s, " % val
            doc_ext += "_%s_" % val.replace("::", "_").replace("<", "_").replace(">", "_")
        template_ext = template_ext[:-2] + ">"

        if self.cppname:
            klassattrs["cppname"] = self.cppname
        else:
            klassattrs["cppname"] += template_ext
        if self.pyname:
            klassattrs["pyname"] = self.pyname
        else:
            klassattrs["pyname"] = pyname

        klassattrs["template_dict"] = self.template_parameters
        return klassattrs

#-------------------------------------------------------------------------------
# PYB11generateClass
#
# Bind the methods for the given class
#-------------------------------------------------------------------------------
def PYB11generateClass(klass, klassattrs, ssout):
    klassinst = klass()

    fs = StringIO.StringIO()
    ss = fs.write

    #...........................................................................
    # Generate a generic class method spec.
    def generic_class_method(meth, methattrs, args):
        if methattrs["static"]:
            ss('    obj.def_static("%(pyname)s", ' % methattrs)
        else:
            ss('    obj.def("%(pyname)s", ' % methattrs)

        # If there is an implementation, short-circuit the rest.
        if methattrs["implementation"]:
            ss(methattrs["implementation"])
        elif methattrs["returnType"] is None:
            if methattrs["static"]:
                ss(("&%(namespace)s" % klassattrs) + methattrs["cppname"])
            else:
                ss(("&%(namespace)s%(cppname)s::" % klassattrs) + methattrs["cppname"])
        else:
            ss("(%(returnType)s " % methattrs)
            if methattrs["static"]:
                ss("(%(namespace)s*)(" % klassattrs)
            else:
                ss("(%(namespace)s%(cppname)s::*)(" % klassattrs)
            argString = ""
            for i, (argType, argName, default) in enumerate(args):
                ss(argType)
                if i < len(args) - 1:
                    ss(", ")
                argString += ', "%s"_a' % argName
                if default:
                    argString += "=%s" % default
            if methattrs["const"]:
                ss((") const) &%(namespace)s%(cppname)s::" % klassattrs) + methattrs["cppname"] + argString)
            else:
                ss((")) &%(namespace)s%(cppname)s::" % klassattrs) + methattrs["cppname"] + argString)

        # Is there a return value policy?
        if methattrs["returnpolicy"]:
            ss(", py::return_value_policy::%s" % methattrs["returnpolicy"])

        doc = inspect.getdoc(meth)
        if doc:
            ss(',\n            "%s"' % doc)
        ss(");\n")

    #...........................................................................
    # Ignore a method
    def ignore(mesh, methattrs, args):
        pass

    #...........................................................................
    # pyinit<>
    def pyinit(meth, methattrs, args):
        ss("    obj.def(py::init<")
        argString = ""
        for i, (argType, argName, default) in enumerate(args):
            if i < len(args) - 1:
                ss("%s, " % argType)
            else:
                ss("%s" % argType)
            argString += ', "%s"_a' % argName
            if default:
                argString += "=%s" % default
        ss(">()%s" % argString)
        doc = inspect.getdoc(meth)
        if doc:
            ss(',\n            "%s"' % doc)
        ss(");\n")


        return

    #...........................................................................
    # readonly attribute
    def readonly_class_attribute(aname, attrs, args):
        if attrs["static"]:
            ss('    obj.def_readonly_static("%(pyname)s", ' % methattrs)
        else:
            ss('    obj.def_readonly("%(pyname)s", ' % methattrs)
        ss(("&%(namespace)s%(cppname)s::" % klassattrs) + methattrs["cppname"])
        doc = inspect.getdoc(meth)
        if doc:
            ss(',\n            "%s"' % doc)
        ss(");\n")

    #...........................................................................
    # readwrite attribute
    def readwrite_class_attribute(aname, attrs, args):
        if attrs["static"]:
            ss('    obj.def_readwrite_static("%(pyname)s", ' % methattrs)
        else:
            ss('    obj.def_readwrite("%(pyname)s", ' % methattrs)
        ss(("&%(namespace)s%(cppname)s::" % klassattrs) + methattrs["cppname"])
        doc = inspect.getdoc(meth)
        if doc:
            ss(',\n            "%s"' % doc)
        ss(");\n")

    #...........................................................................
    # Binary operators
    def binary_operator(meth, methattrs, args, op):
        assert len(args) in (0, 1)
        if len(args) == 0:
            argType = "py::self"
        else:
            argType = args[0][0]
        ss('    obj.def(py::self %s %s);\n' % (op, argType))

    #...........................................................................
    # Reverse binary operators
    def reverse_binary_operator(meth, methattrs, args, op):
        assert len(args) in (0, 1)
        if len(args) == 0:
            argType = "py::self"
        else:
            argType = args[0][0]
        ss('    obj.def(%s %s py::self);\n' % (argType, op))

    #...........................................................................
    # Unary operators
    def unary_operator(meth, methattrs, args, op):
        assert len(args) == 0
        ss('    obj.def(%spy::self);\n' % op)

    #...........................................................................
    # operator()
    def call_operator(meth, methattrs, args, op):
        ss('    obj.def("__call__", ')
        if methattrs["returnType"] is None:
            ss("&%(namespace)s%(cppname)s::operator()" % klassattrs)
        else:
            argString = ""
            ss(("(%(returnType)s " % methattrs) + ("(%(namespace)s%(cppname)s::*)(" % klassattrs))
            for i, (argType, argName, default) in enumerate(args):
                ss(argType)
                if i < len(args) - 1:
                    ss(", ")
                argString += ', "%s"_a' % argName
                if default:
                    argString += "=%s" % default
            if methattrs["const"]:
                ss((") const) &%(namespace)s%(cppname)s::operator()" % klassattrs) + argString)
            else:
                ss((")) &%(namespace)s%(cppname)s::operator()" % klassattrs) + argString)
        doc = inspect.getdoc(meth)
        if doc:
            ss(',\n            "%s"' % doc)
        ss(", py::is_operator());\n")

    #...........................................................................
    # Tabulate the dispatch for special operations.
    special_operators =  {"__init__": (ignore, ""),

                          "__call__": (call_operator, ""),

                          "__add__" : (binary_operator, "+"),
                          "__sub__" : (binary_operator, "-"),
                          "__mul__" : (binary_operator, "*"),
                          "__div__" : (binary_operator, "/"),
                          "__mod__" : (binary_operator, "%"),
                          "__and__" : (binary_operator, "&"),
                          "__xor__" : (binary_operator, "^"),
                          "__or__"  : (binary_operator, "|"),
                          
                          "__radd__" : (reverse_binary_operator, "+"),
                          "__rsub__" : (reverse_binary_operator, "-"),
                          "__rmul__" : (reverse_binary_operator, "*"),
                          "__rdiv__" : (reverse_binary_operator, "/"),
                          "__rmod__" : (reverse_binary_operator, "%"),
                          "__rand__" : (reverse_binary_operator, "&"),
                          "__rxor__" : (reverse_binary_operator, "^"),
                          "__ror__"  : (reverse_binary_operator, "|"),
                          
                          "__iadd__" : (binary_operator, "+="),
                          "__isub__" : (binary_operator, "-="),
                          "__imul__" : (binary_operator, "*="),
                          "__idiv__" : (binary_operator, "/="),
                          "__imod__" : (binary_operator, "%="),
                          "__iand__" : (binary_operator, "&="),
                          "__ixor__" : (binary_operator, "^="),
                          "__ior__"  : (binary_operator, "|="),

                          "__neg__"    : (unary_operator, "-"),
                          "__invert__" : (unary_operator, "~"),

                          "__lt__" : (binary_operator, "<"),
                          "__le__" : (binary_operator, "<="),
                          "__eq__" : (binary_operator, "=="),
                          "__ne__" : (binary_operator, "!="),
                          "__gt__" : (binary_operator, ">"),
                          "__ge__" : (binary_operator, ">=")}

    # Start generating.
    ss("""
  //............................................................................
  // Class %(pyname)s
  {
""" % klassattrs)
    # If the class has specified any typedefs, do them.
    if hasattr(klass, "typedefs"):
        ss(klass.typedefs + "\n")

    ss("    py::class_<%(namespace)s%(cppname)s" % klassattrs)

    # Check for base classes.
    cppname = "%(namespace)s%(cppname)s" % klassattrs
    for bklass in inspect.getmro(klass)[1:]:
        bklassattrs = PYB11attrs(bklass)
        bcppname = "%(namespace)s%(cppname)s" % bklassattrs
        if bklassattrs["template"]:
            bcppname += "<"
            for i, t in enumerate(bklassattrs["template"]):
                if i < len(bklassattrs["template"]) - 1:
                    bcppname += ("%(" + t + ")s, ")
                else:
                    bcppname += ("%(" + t + ")s>")
            bcppname = bcppname % klassattrs["template_dict"]
        if bcppname != cppname:
            ss(", " + bcppname)
            break

    # Any trampoline?
    if PYB11virtualClass(klass):
        ss(", %(namespace)sPYB11Trampoline%(cppname)s" % klassattrs)

    # Is this a singleton?
    if klassattrs["singleton"]:
        ss(", std::unique_ptr<%(namespace)s%(cppname)s, py::nodelete>" % klassattrs)
    ss('> obj(m, "%(pyname)s");\n' % klassattrs)

    # Is there a doc string?
    if inspect.getdoc(klass):
        ss("    obj.doc() = ")
        for i, line in enumerate(inspect.getdoc(klass).split('\n')):
            if i > 0:
                ss("            ")
            ss('"%s"\n' % line);
        ss("  ;\n")

    # Grab all the methods
    allmethods = [(mname, meth) for (mname, meth) in PYB11ClassMethods(klass)
                  if not PYB11attrs(meth)["ignore"]]

    # Bind constructors of the class.
    ss("\n    // Constructors\n")
    kills = []
    for i, (mname, meth) in enumerate(allmethods):
        if mname[:6] == "pyinit":
            methattrs = PYB11attrs(meth)
            args = PYB11parseArgs(meth)
            pyinit(meth, methattrs, args)
            kills.append(i)
    for i in reversed(kills):
        del allmethods[i]

    # Bind special operators.
    ss("\n    // Operators\n")
    kills = []
    for i, (mname, meth) in enumerate(allmethods):
        methattrs = PYB11attrs(meth)
        methattrs["returnType"] = eval("klassinst." + mname + "()")
        args = PYB11parseArgs(meth)
        if mname in special_operators:
            func, op = special_operators[mname]
            func(meth, methattrs, args, op)
            kills.append(i)
    for i in reversed(kills):
        del allmethods[i]

    # Bind attributes
    ss("\n    // Attributes\n")
    kills = []
    for i, (mname, meth) in enumerate(allmethods):
        methattrs = PYB11attrs(meth)
        methattrs["returnType"] = eval("klassinst." + mname + "()")
        args = PYB11parseArgs(meth)
        if methattrs["readonly"]:
            readonly_class_attribute(meth, methattrs, args)
            kills.append(i)
        elif methattrs["readwrite"]:
            readwrite_class_attribute(meth, methattrs, args)
            kills.append(i)
    for i in reversed(kills):
        del allmethods[i]

    # Bind the remaining methods of the class.
    ss("\n    // Methods\n")
    for i, (mname, meth) in enumerate(allmethods):
        methattrs = PYB11attrs(meth)
        methattrs["returnType"] = eval("klassinst." + mname + "()")
        args = PYB11parseArgs(meth)
        generic_class_method(meth, methattrs, args)

    # Bind properties
    PYB11GenerateClassProperties(klass, klassinst, klassattrs, ss)

    # Look for any class scope enums and bind them
    enums = [x for x in dir(klassinst) if isinstance(eval("klassinst.%s" % x), PYB11enum)]
    if enums:
        ss("\n    // %(cppname)s enums\n  " % klassattrs)
        ssenum = PYB11indentedIO("  ")
        for ename in enums:
            inst = eval("klassinst.%s" % ename)
            inst(klass, ssenum, klassattrs)
        ss(ssenum.getvalue())
        ssenum.close()

    ss("  }\n\n")

    ssout(fs.getvalue() % klassattrs["template_dict"])
    fs.close()

    return
