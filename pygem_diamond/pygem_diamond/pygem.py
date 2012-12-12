
import os

from openmdao.main.api import Container
from openmdao.main.datatypes.api import Str

from pygem_diamond import gem


# class GEMStaticGeometry(object):
#     """A wrapper for a GEM object without parameters.  This object implements the
#     IStaticGeometry interface.
#     """
#     def __init__(self):
#         pass

#     def get_tris(self):
#         pass


class GEMParametricGeometry(Container):
    """A wrapper for a GEM object with modifiable parameters.  This object
    implements the IParametricGeometry interface.
    """

    model_file = Str('')

    def __init__(self):
        self._model = None
        self._idhash = {}
        self._context = gem.initialize()

    def _model_file_changed(self, name, old, new):
        self.load_model(os.path.expanduser(self.model_file))

    def load_model(self, filename):
        """Load a model from a file."""
        # clean up the old model if there is one
        if self._model is not None:
            gem.releaseModel(self._model)

        self._idhash = {}
        self._model = None

        try:
            if filename is not None:
                if not os.path.isfile(filename):
                    raise IOError("file '%s' not found." % filename)
                self._model = gem.load_model(self._context, filename)
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

    def getAttrByName(self, obj, objtype, eindex, name):
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

    def listParameters(self):
        """Return a list of parameters (if any) for this model.
        """
        params = []
        if self._model is not None:
            tup = gem.getModel(self._model)
            nparams = tup[5]
            for paramID in range(1, nparams + 1):
                name, flag, order, values, nattr = gem.getParam(self._model, paramID)
                self._idhash[name] = paramID
                meta = {}
                if flag & 8:  # check if param has limits
                    high, low = gem.getLimits(self._model, paramID)
                    meta['high'] = high
                    meta['low'] = low
                else:
                    high = low = None
                if len(values) > 1:
                    val = list(values)
                else:
                    val = values[0]
                meta['default'] = val
                if not (flag & 2):
                    params.append((name, meta))

        return params

    def listOutputs(self):
        """Return a list of parameters (if any) for this model.
        """
        outputs = []
        if self._model is not None:
            tup = gem.getModel(self._model)
            nparams = tup[5]
            for paramID in range(1, nparams + 1):
                name, flag, order, values, nattr = gem.getParam(self._model, paramID)
                self._idhash[name] = paramID
                meta = {}
                if len(values) > 1:
                    val = list(values)
                else:
                    val = values[0]
                if (flag & 2):
                    outputs.append((name, paramID, val, meta))

        return params

    def setParameter(self, name, val):
        """Set new value for a driving parameter.

        """
        if self._model is not None and len(self._idhash) == 0:
            self.listParams()   # populate params dict
        try:
            paramID = self._idhash[name]
            if not isinstance(val, (float, int, str)):
                val = tuple(val)
            return gem.setParam(self._model, paramID, val)
        except Exception, e:
            raise RuntimeError("Error setting parameter '%s': %s" % (name, str(e)))


    # {"getParam",      gemGetParam,      METH_VARARGS,  "Get info about a Parameter in a Model\n\n\
    #                                                    Input arguments:\n\
    #                                                    \t  modelObj    \n\
    #                                                    \t  iparam      <bias-1>\n\
    #                                                    Returns:        \n\
    #                                                    \t  pname       \n\
    #                                                    \t  bflag       \n\
    #                                                    \t  order       \n\
    #                                                    \t  (values)    \n\
    #                                                    \t  nattr       "},
    # {"setSuppress",   gemSetSuppress,   METH_VARARGS,  "Change suppression state for a Branch\n\n\
    #                                                    Input arguments:\n\
    #                                                    \t  modelObj    \n\
    #                                                    \t  ibranch     <bias-1>\n\
    #                                                    \t  istate      \n\
    #                                                    Returns:        \n\
    #                                                    \t  <none>      "},
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
