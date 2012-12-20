
import os

from openmdao.main.api import Container, implements
from openmdao.main.datatypes.api import Str
from openmdao.main.interfaces import IParametricGeometry

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

    implements(IParametricGeometry)

    model_file = Str('', iotype='in')

    def __init__(self, mfile=''):
        super(GEMParametricGeometry, self).__init__()
        self._model = None
        self._idhash = {}
        self._callbacks = []
        self._context = gem.initialize()
        self.model_file = mfile

    def _model_file_changed(self, name, old, new):
        self.load_model(os.path.expanduser(self.model_file))

    def load_model(self, filename):
        """Load a model from a file."""

        old_model = self._model

        self._idhash = {}
        self._model = None

        try:
            if filename is not None:
                if not os.path.isfile(filename):
                    raise IOError("file '%s' not found." % filename)
                self._model = gem.loadModel(self._context, filename)
        except Exception as err:
            self._model = old_model
            raise RuntimeError("problem loading GEM model file '%s': %s" % (filename, str(err)))
        finally:
            # clean up the old model if there is one
            if old_model is not self._model and old_model is not None:
                gem.releaseModel(old_model)
        for cb in self._callbacks:
            cb()

        return self._model

    def regenModel(self):
        if self._model is not None:
            try:
                return gem.regenModel(self._model)
            except Exception as err:
                raise RuntimeError("Error regenerating model: %s" % str(err))

    def listParameters(self):
        """Return a list of parameters (inputs and outputs) for this model.
        """
        params = []
        if self._model is not None:
            tup = gem.getModel(self._model)
            nparams = tup[5]
            for paramID in range(1, nparams + 1):
                name, flag, order, values, nattr = gem.getParam(self._model, paramID)
                if name.startswith('@'):
                    name = name[1:]
                self._idhash[name] = paramID
                meta = {}
                if flag & 8:  # check if param has limits
                    high, low = gem.getLimits(self._model, paramID)
                    meta['high'] = high
                    meta['low'] = low
                else:
                    high = low = None
                if isinstance(values, basestring):
                    val = values
                elif len(values) > 1:
                    val = list(values)
                else:
                    val = values[0]
                meta['default'] = val
                if (flag & 2):
                    meta['iotype'] = 'out'
                else:
                    meta['iotype'] = 'in'
                params.append((name, meta))

        return params

    def setParameter(self, name, val):
        """Set new value for a driving parameter.

        """
        if self._model is not None and len(self._idhash) == 0:
            self.listParams()   # populate params dict
        try:
            paramID = self._idhash[name]
            if isinstance(val, (float, int)):
                val = (val,)
            elif isinstance(val, basestring):
                val = str(val)
            else:
                val = tuple(val)
            return gem.setParam(self._model, paramID, val)
        except Exception as err:
            raise RuntimeError("Error setting parameter '%s': %s" % (name, str(err)))

    def getParameter(self, name):
        """Get info about a Parameter in a Model"""
        if self._model is not None and len(self._idhash) == 0:
            self.listParams()   # populate params dict
        try:
            pname, bflag, order, values, nattr = gem.getParam(self._model, self._idhash[name])
        except Exception as err:
            raise RuntimeError("Error getting parameter '%s': %s" % (name, str(err)))
        if isinstance(values, basestring):
            val = values
        elif len(values) > 1:
            val = list(values)
        else:
            val = values[0]
        return val

    def register_param_list_changedCB(self, callback):
        if callback not in self._callbacks:
            self._callbacks.append(callback)

    def terminate(self):
        """Terminate GEM context."""
        gem.terminate(self._context)


    # {"setSuppress",   gemSetSuppress,   METH_VARARGS,  "Change suppression state for a Branch\n\n\
    #                                                    Input arguments:\n\
    #                                                    \t  modelObj    \n\
    #                                                    \t  ibranch     <bias-1>\n\
    #                                                    \t  istate      \n\
    #                                                    Returns:        \n\
    #                                                    \t  <none>      "},
    # def getAttrById(self, obj, objtype, eindex, aindex):
    #     """Get Attribute of a GEMobject.

    #     obj: reference
    #         A reference to a GEM object

    #     objtype: str
    #         One of ['CONTEXT', 'MODEL', 'BRANCH', 'PARAM', 'BREP', 'NODE', 'EDGE',
    #                 'LOOP', 'FACE', or 'SHELL']

    #     eindex: int <bias-1>
    #         ???

    #     aindex: int <bias-1>
    #         ???

    #     Returns (name, values)
    #     """
    #     try:
    #         return gem.getAttribute(obj, objtype, eindex, aindex)
    #     except Exception, e:
    #         raise RuntimeError("GEM getAttribute failed: %s" % str(e))

    # def getAttrByName(self, obj, objtype, eindex, name):
    #     """Get Attribute of a GEMobject.

    #     obj: reference
    #         A reference to a GEM object

    #     objtype: str
    #         One of ['CONTEXT', 'MODEL', 'BRANCH', 'PARAM', 'BREP', 'NODE', 'EDGE',
    #                 'LOOP', 'FACE', or 'SHELL']

    #     eindex: int <bias-1>
    #         ???

    #     name: str
    #         Name of the attribute

    #     Returns (index <bias-1>, values)
    #     """
    #     try:
    #         return gem.retAttribute(obj, objtype, eindex, name)
    #     except Exception, e:
    #         raise RuntimeError(": %s" % str(e))

    # def setAttribute(self, obj, objtype, eindex, name, values):
    #     """Set an Attribute for a GEM object.

    #     obj: reference
    #         A reference to a GEM object.

    #     objtype: str
    #         One of ['CONTEXT', 'MODEL', 'BRANCH', 'PARAM', 'BREP', 'NODE',
    #                 'EDGE', 'LOOP', 'FACE', or 'SHELL']

    #     eindex: int
    #         Entitiy index <bias-1>

    #     name: str
    #         Name of the Attribute.

    #     values: tuple
    #         Value(s) to assign to the Attribute.

    #     Returns None.
    #     """
    #     try:
    #         return gem.setAttribute(self, obj, objtype, eindex, name, values)
    #     except Exception, e:
    #         raise RuntimeError("Error setting GEM attribute: %s" % str(e))

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
