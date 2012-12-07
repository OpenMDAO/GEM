
import threading

from pygem_diamond import gem

_def_context_lock = threading.Lock()


class GEMStaticGeometry(object):
    """A wrapper for a GEM object without parameters.  This object implements the
    IStaticGeometry interface.
    """
    def __init__(self):
        pass


class GEMParametricGeometry(GEMStaticGeometry):
    """A wrapper for a GEM object with modifiable parameters.  This object implements the
    IParametricGeometry interface.
    """
    _def_context = None

    def __init__(self, use_default_context=True):
        if use_default_context:
            with _def_context_lock:
                if self._def_context is None:
                    self._def_context = gem.initialize()
                self._context = self._def_context
        else:
            self._context = gem.initialize()

    def loadModel(self, filename):
        """Load a model from a file."""
        try:
            return gem.loadModel(self._context, filename)
        except Exception, e:
            raise RuntimeError("problem loading model file '%s': %s" % (filename, str(e)))

    def terminate(self):
        """Terminate GEM context."""
        if self._context is not None:
            gem.terminate(self._context)
        with _def_context_lock:
            if self._context is self._def_context:
                self._def_context = None

    def staticModel(self):
        """Generate empty static Model."""
        try:
            return gem.staticModel(self._context)
        except Exception, e:
            raise RuntimeError("couldn't create empty static model: %s" % str(e))

    def getBRepOwner(self, brep):
        """Get Model containing the given BRep.

        brep: ptr value
            Pointer to BRep object.

        Returns ``(model, instance, branch)``.
        """
        try:
            gem.getBRepOwner(brep)
        except Exception, e:
            raise RuntimeError(": %s" % str(e))

    def solidBoolean(self, brep1, brep2, optype, xform=(1., 0., 0., 0.,
                                                        0., 1., 0., 0.,
                                                        0., 0., 1., 0.)):
        """Execute solid boolean operation.

        brep1: ptr
            Pointer to BRep object.
        brep2: ptr
            Pointer to BRep object.
        optype: str
            Either 'INTERSECT', 'SUBTRACT', or 'UNION'
        xform: tuple of 12 doubles  (optional)

        Returns model
        """
        try:
            return gem.solidBoolean(brep1, brep2, optype, xform)
        except Exception, e:
            raise RuntimeError("Error during boolean operation: %s" % str(e))

    def blah(self):
        """"""
        try:
            pass
        except Exception, e:
            raise RuntimeError(": %s" % str(e))


    {"getAttribute",  gemGetAttribute,  METH_VARARGS,  "Get Attribute of a GEMobject\n\n\
                                                       Input arguments:\n\
                                                       \t  gemObj      \n\
                                                       \t  otype       either 'CONTEXT', 'MODEL', 'BRANCH', 'PARAM', 'BREP',\n\
                                                       \t                     'NODE', 'EDGE', 'LOOP', 'FACE', or 'SHELL'\n\
                                                       \t  eindex      <bias-1>\n\
                                                       \t  aindex      <bias-1>\n\
                                                       Returns:        \n\
                                                       \t  aname       \n\
                                                       \t  (values)    "},
    {"retAttribute",  gemRetAttribute,  METH_VARARGS,  "Return Attribute of a GEMobject\n\n\
                                                       Input arguments:\n\
                                                       \t  gemObj      \n\
                                                       \t  otype       either 'CONTEXT', 'MODEL', 'BRANCH', 'PARAM', 'BREP',\n\
                                                       \t                     'NODE', 'EDGE', 'LOOP', 'FACE', or 'SHELL'\n\
                                                       \t  eindex      <bias-1>\n\
                                                       \t  name        \n\
                                                       Returns:        \n\
                                                       \t  aindex      <bias-1>\n\
                                                       \t  (values)    "},
    {"setAttribute",  gemSetAttribute,  METH_VARARGS,  "Set an Attribute for a GEMobject\n\n\
                                                       Input arguments:\n\
                                                       \t  gemObj      \n\
                                                       \t  otype       either 'CONTEXT', 'MODEL', 'BRANCH', 'PARAM', 'BREP',\n\
                                                       \t                     'NODE', 'EDGE', 'LOOP', 'FACE', or 'SHELL'\n\
                                                       \t  eindex      <bias-1>\n\
                                                       \t  name        \n\
                                                       \t  (values)    \n\
                                                       Returns:        \n\
                                                       \t  <none>      "},

    // routines defined in model.h
    {"add2Model",     gemAdd2Model,     METH_VARARGS,  "Add a BRep to a static Model\n\n\
                                                       Input arguments:\n\
                                                       \t  modelObj    \n\
                                                       \t  brepObj     \n\
                                                       \t  (xform)     <optional>\n\
                                                       Returns:        \n\
                                                       \t  <none>      "},
    {"saveModel",     gemSaveModel,     METH_VARARGS,  "Save an up-to-date Model\n\n\
                                                       Input arguments:\n\
                                                       \t  modelObj    \n\
                                                       \t  filename    \n\
                                                       Returns:        \n\
                                                       \t  <none>      "},
    {"releaseModel",  gemReleaseModel,  METH_VARARGS,  "Release a Model and all of its storage\n\n\
                                                       Input arguments:\n\
                                                       \t  modelObj    \n\
                                                       Returns:        \n\
                                                       \t  <none>      "},
    {"copyModel",     gemCopyModel,     METH_VARARGS,  "Copy a Model\n\
                                                       Input arguments:\n\
                                                       \t  oldModelObj \n\
                                                       Returns:        \n\
                                                       \t  newModelObj "},
    {"regenModel",    gemRegenModel,    METH_VARARGS,  "Regenerate a non-static Model\n\n\
                                                       Input arguments:\n\
                                                       \t  modelObj    \n\
                                                       Returns:        \n\
                                                       \t  <none>      "},
    {"getModel",      gemGetModel,      METH_VARARGS,  "Get info about a Model\n\n\
                                                       Input arguments:\n\
                                                       \t  modelObj    \n\
                                                       Returns:        \n\
                                                       \t  server      \n\
                                                       \t  filename    \n\
                                                       \t  modeler     \n\
                                                       \t  uptodate    \n\
                                                       \t  (BReps)     \n\
                                                       \t  nparam      \n\
                                                       \t  nbranch     \n\
                                                       \t  nattr       "},
    {"getBranch",     gemGetBranch,     METH_VARARGS,  "Get info about a Branch in a Model\n\n\
                                                       Input arguments:\n\
                                                       \t  modelObj    \n\
                                                       \t  ibranch     <bias-1>\n\
                                                       Returns:        \n\
                                                       \t  bname       \n\
                                                       \t  btype       \n\
                                                       \t  suppress    \n\
                                                       \t  (parents)   \n\
                                                       \t  (children)  \n\
                                                       \t  nattr       "},
    {"setSuppress",   gemSetSuppress,   METH_VARARGS,  "Change suppression state for a Branch\n\n\
                                                       Input arguments:\n\
                                                       \t  modelObj    \n\
                                                       \t  ibranch     <bias-1>\n\
                                                       \t  istate      \n\
                                                       Returns:        \n\
                                                       \t  <none>      "},
    {"getParam",      gemGetParam,      METH_VARARGS,  "Get info about a Parameter in a Model\n\n\
                                                       Input arguments:\n\
                                                       \t  modelObj    \n\
                                                       \t  iparam      <bias-1>\n\
                                                       Returns:        \n\
                                                       \t  pname       \n\
                                                       \t  bflag       \n\
                                                       \t  order       \n\
                                                       \t  (values)    \n\
                                                       \t  nattr       "},
    {"setParam",      gemSetParam,      METH_VARARGS,  "Set new value for a driving Parameter\n\n\
                                                       Input arguments:\n\
                                                       \t  modelObj    \n\
                                                       \t  iparam      <bias-1>\n\
                                                       \t  (values)    \n\
                                                       Returns:        \n\
                                                       \t  <none>      "},

    // routines defined in brep.h
    {"getBRepInfo",   gemGetBRepInfo,   METH_VARARGS,  "Get info about a BRep\n\n\
                                                       Input arguments:\n\
                                                       \t  brepObj     \n\
                                                       Returns:        \n\
                                                       \t  (box)       \n\
                                                       \t  type        \n\
                                                       \t  nnode       \n\
                                                       \t  nedge       \n\
                                                       \t  nloop       \n\
                                                       \t  nface       \n\
                                                       \t  nshell      \n\
                                                       \t  nattr       "},
    {"getShell",      gemGetShell,      METH_VARARGS,  "Get info about a Shell in a BRep\n\n\
                                                       Input arguments:\n\
                                                       \t  brepObj     \n\
                                                       \t  ishell      <bias-1>\n\
                                                       Returns:        \n\
                                                       \t  type        \n\
                                                       \t  (faces)     \n\
                                                       \t  nattr       "},
    {"getFace",       gemGetFace,       METH_VARARGS,  "Get info about a Face in a BRep\n\n\
                                                       Input arguments:\n\
                                                       \t  brepObj     \n\
                                                       \t  iface       <bias-1>\n\
                                                       Returns:        \n\
                                                       \t  ID          \n\
                                                       \t  (uvbox)     \n\
                                                       \t  norm        \n\
                                                       \t  (loops)     \n\
                                                       \t  nattr       "},
    {"getWire",       gemGetWire,       METH_VARARGS,  "Get info about a Wire in a BRep\n\n\
                                                       Input arguments:\n\
                                                       \t  brepObj     \n\
                                                       Returns:        \n\
                                                       \t  (loops)     "},
    {"getLoop",       gemGetLoop,       METH_VARARGS,  "Get info about a Loop in a BRep\n\n\
                                                       Input arguments:\n\
                                                       \t  brepObj     \n\
                                                       \t  iloop       <bias-1>\n\
                                                       Returns:        \n\
                                                       \t  face        \n\
                                                       \t  type        \n\
                                                       \t  (edges)     \n\
                                                       \t  nattr       "},
    {"getEdge",       gemGetEdge,       METH_VARARGS,  "Get data for an Edge in a BRep\n\n\
                                                       Input arguments:\n\
                                                       \t  brepObj     \n\
                                                       \t  iedge       <bias-1>\n\
                                                       Returns:        \n\
                                                       \t  (tlimit)    \n\
                                                       \t  (nodes)     \n\
                                                       \t  (faces)     \n\
                                                       \t  nattr       "},
    {"getNode",       gemGetNode,       METH_VARARGS,  "Get info about a Node in a BRep\n\n\
                                                       Input arguments:\n\
                                                       \t  brepObj     \n\
                                                       \t  inode       <bias-1>\n\
                                                       Returns:        \n\
                                                       \t  (xyz)       \n\
                                                       \t  nattr       "},
    {"getMassProps",  gemGetMassProps,  METH_VARARGS,  "Get mass properties about a BRep entity\n\n\
                                                       Input arguments:\n\
                                                       \t  brepObj     \n\
                                                       \t  etype       either 'FACE', 'SHELL', or 'BREP'\n\
                                                       \t  eindex      <bias-1>\n\
                                                       Returns:        \n\
                                                       \t  (props)     "},
    {"isEquivalent",  gemIsEquivalent,  METH_VARARGS,  "Determine is two BRep entities are the same\n\n\
                                                       Input arguments:\n\
                                                       \t  etype       either 'NODE', 'EDGE', or 'FACE'\n\
                                                       \t  brepObj1    \n\
                                                       \t  eindex1     <bias-1>\n\
                                                       \t  brepObj2    \n\
                                                       \t  eindex2     <bias-1>\n\
                                                       Returns:        \n\
                                                       \t  bool        "},

    // (selected) routines defined in drep.h
    {"newDRep",       gemNewDRep,       METH_VARARGS,  "Make a new DRep\n\n\
                                                       Input arguments:\n\
                                                       \t  modelObj    \n\
                                                       Returns:        \n\
                                                       \t  drepObj     "},
    {"tesselDRep",    gemTesselDRep,    METH_VARARGS,  "Tessellate a BRep into a DRep\n\n\
                                                       Input arguments:\n\
                                                       \t  drepObj     \n\
                                                       \t  ibrep       <bias-1>\n\
                                                       \t  maxang      \n\
                                                       \t  maxlen      \n\
                                                       \t  maxsag      \n\
                                                       Returns:        \n\
                                                       \t  <none>      "},
    {"getTessel",     gemGetTessel,     METH_VARARGS,  "Get tessellation data for a Face in a BRep\n\n\
                                                       Input arguments:\n\
                                                       \t  drepObj     \n\
                                                       \t  ibrep       <bias-1>\n\
                                                       \t  iface       <bias-1>\n\
                                                       Returns:        \n\
                                                       \t  xyzArray    \n\
                                                       \t  uvArray     \n\
                                                       \t  connArray   "},
     {"destroyDRep",  gemDestroyDRep,   METH_VARARGS,  "Destroy a DRep and its contents\n\n\
                                                       Input arguments:\n\
                                                       \t  drepObj     \n\
                                                       Returns         \n\
                                                       \t  <none>      "},


