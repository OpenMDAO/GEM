

from openmdao.main.api import Container
from openmdao.main.datatypes.api import Str

from pygem_diamond import gem


class GEMStaticGeometry(object):
    """A wrapper for a GEM object without parameters.  This object implements the
    IStaticGeometry interface.
    """
    def __init__(self):
        pass

    def get_tris(self):
        pass


class GEMParametricGeometry(HasTraits):
    """A wrapper for a GEM object with modifiable parameters.  This object
    implements the IParametricGeometry interface.
    """

    model_file = Str('')

    def __init__(self):
        self._model = None
        self._context = gem.initialize()

    def _model_file_changed(self, obj, name, old, new):
        self.loadMOdel(self.model_file)

    def loadModel(self, filename):
        """Load a model from a file."""
        # clean up the old model if there is one
        if self._model is not None:
            gem.releaseModel(self._model)

        try:
            if filename is not None:
                self._model = gem.loadModel(self._context, filename)
        except Exception, e:
            raise RuntimeError("problem loading GEM model file '%s': %s" % (filename, str(e)))
        return self._model

    def terminate(self):
        """Terminate GEM context."""
        gem.terminate(self._context)

    def getAttrById(self, obj, objtype, eindex, aindex):
        """Get Attribute of a GEMobject.

        obj: reference
            A reference to a GEM object

        objtype: str
            One of ['CONTEXT', 'MODEL', 'BRANCH', 'PARAM', 'BREP', 'NODE', 'EDGE',
                    'LOOP', 'FACE', or 'SHELL']

        eindex: int <bias-1>
            ???

        aindex: int <bias-1>
            ???

        Returns (name, values)
        """
        try:
            return gem.getAttribute(obj, objtype, eindex, aindex)
        except Exception, e:
            raise RuntimeError("GEM getAttribute failed: %s" % str(e))

    def getAttrByName(self, obj, otype, eindex, name):
        """Get Attribute of a GEMobject.

        obj: reference
            A reference to a GEM object

        objtype: str
            One of ['CONTEXT', 'MODEL', 'BRANCH', 'PARAM', 'BREP', 'NODE', 'EDGE',
                    'LOOP', 'FACE', or 'SHELL']

        eindex: int <bias-1>
            ???

        name: str
            Name of the attribute

        Returns (index <bias-1>, values)
        """
        try:
            return gem.retAttribute(obj, objtype, eindex, name)
        except Exception, e:
            raise RuntimeError(": %s" % str(e))

    def setAttribute(self, obj, objtype, eindex, name, values):
        """Set an Attribute for a GEM object.

        obj: reference
            A reference to a GEM object.

        objtype: str
            One of ['CONTEXT', 'MODEL', 'BRANCH', 'PARAM', 'BREP', 'NODE',
                    'EDGE', 'LOOP', 'FACE', or 'SHELL']

        eindex: int
            Entitiy index <bias-1>

        name: str
            Name of the Attribute.

        values: tuple
            Value(s) to assign to the Attribute.

        Returns None.
        """
        try:
            return gem.setAttribute(self, obj, objtype, eindex, name, values)
        except Exception, e:
            raise RuntimeError("Error setting GEM attribute: %s" % str(e))

    def regenModel(self):
        if self._model is not None:
            try:
                return gem.regenModel(self._model)
            except Exception, e:
                raise RuntimeError("Error regenerating model: %s" % str(e))

    def listParams(self):
        params = []
        if self._model is not None:
            tup = gem.getModel(self._model)
            nparams = tup[5]
            for p in range(nparams):
                name, flag, order, values, nattr = gem.getParam(self._model, p + 1)
                params.append((name, ))
# target, high=None, low=None,
#                  scaler=None, adder=None, start=None,
#                  fd_step=None, scope=None, name=None
        return params

            server, filename, modeler, uptodate, breps, nparam, nbranch, nattr = 

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
    # def staticModel(self):
    #     """Generate empty static Model."""
    #     try:
    #         return gem.staticModel(self._context)
    #     except Exception, e:
    #         raise RuntimeError("couldn't create empty GEM static model: %s" % str(e))

    # def getBRepOwner(self, brep):
    #     """Get Model containing the given BRep.

    #     brep: reference
    #         Ref to BRep object.

    #     Returns ``(model, instance, branch)``.
    #     """
    #     try:
    #         gem.getBRepOwner(brep)
    #     except Exception, e:
    #         raise RuntimeError(": %s" % str(e))

    # def solidBoolean(self, brep1, brep2, optype, xform=(1., 0., 0., 0.,
    #                                                     0., 1., 0., 0.,
    #                                                     0., 0., 1., 0.)):
    #     """Execute solid boolean operation.

    #     brep1: reference
    #         Ref to BRep object.
    #     brep2: reference
    #         Ref to BRep object.
    #     optype: str
    #         Either 'INTERSECT', 'SUBTRACT', or 'UNION'
    #     xform: tuple of 12 doubles  (optional)

    #     Returns model
    #     """
    #     try:
    #         return gem.solidBoolean(brep1, brep2, optype, xform)
    #     except Exception, e:
    #         raise RuntimeError("Error during GEM boolean operation: %s" % str(e))
