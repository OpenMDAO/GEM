# test suite for gem.so

import os
import tempfile
import shutil

import unittest
from pygem_diamond import gem
from pygem_diamond.pygem import GEMParametricGeometry

sample_file = os.path.join(os.path.dirname(__file__), "sample.csm")

class PygemTestCase(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        pass

    # test: gem.initialize
    #       gem.setAttribute(CONTEXT)
    #       gem.retAttribute(CONTEXT)
    #       gem.terminate
    def test_ContextAttributes(self):
        myContext = gem.initialize()

        gem.setAttribute(myContext, "CONTEXT", 0, "s_attr", "context attribute")
        gem.setAttribute(myContext, "CONTEXT", 0, "i_attrs", (1111111, 2222222, 3333333, 4444444))
        gem.setAttribute(myContext, "CONTEXT", 0, "r_attrs", (0.1234567890123456, 0.2345678901234567))

        aindex, values = gem.retAttribute(myContext, "CONTEXT", 0, "s_attr")
        self.assertEqual(aindex, 1)
        self.assertEqual(values, "context attribute")

        aindex, values = gem.retAttribute(myContext, "CONTEXT", 0, "i_attrs")
        self.assertEqual(aindex, 2)
        self.assertEqual(values[0], 1111111)
        self.assertEqual(values[1], 2222222)
        self.assertEqual(values[2], 3333333)
        self.assertEqual(values[3], 4444444)

        aindex, values = gem.retAttribute(myContext, "CONTEXT", 0, "r_attrs")
        self.assertEqual(aindex, 3)
        self.assertEqual(values[0], 0.1234567890123456)
        self.assertEqual(values[1], 0.2345678901234567)

        gem.terminate(myContext)

    # test: gem.initialize
    #       gem.loadModel
    #       gem.setAttribute(MODEL)
    #       gem.copyModel
    #       gem.getModel
    #       gem.getAttribute(MODEL)
    #       gem.releaseModel
    #       gem.terminate
    def test_LoadModel(self):
        myContext = gem.initialize()
        myModel   = gem.loadModel(myContext, sample_file)

        gem.setAttribute(myModel, "MODEL",  0, "s_attr", "model attribute")
        gem.setAttribute(myModel, "MODEL",  0, "i_attrs", (2222222, 3333333, 4444444, 5555555))
        gem.setAttribute(myModel, "MODEL",  0, "r_attrs", (0.2345678901234567, 0.3456789012345678))

        copyModel = gem.copyModel(myModel)
        gem.releaseModel(myModel)

        server, filename, modeler, uptodate, myBReps, nparam, nbranch, nattr = gem.getModel(copyModel)
        self.assertEqual(filename, sample_file)
        self.assertEqual(modeler,  "OpenCSM")
        self.assertEqual(uptodate,  1)
        self.assertEqual(nparam,   33)
        self.assertEqual(nbranch,  22)
        self.assertEqual(nattr,     3)

        aname, values = gem.getAttribute(copyModel, "MODEL", 0, 1)
        self.assertEqual(aname,  "s_attr")
        self.assertEqual(values, "model attribute")

        aname, values = gem.getAttribute(copyModel, "MODEL", 0, 2)
        self.assertEqual(aname,     "i_attrs")
        self.assertEqual(values[0], 2222222)
        self.assertEqual(values[1], 3333333)
        self.assertEqual(values[2], 4444444)
        self.assertEqual(values[3], 5555555)

        aname, values = gem.getAttribute(copyModel, "MODEL", 0, 3)
        self.assertEqual(aname,     "r_attrs")
        self.assertEqual(values[0], 0.2345678901234567)
        self.assertEqual(values[1], 0.3456789012345678)

        gem.releaseModel(copyModel)
        gem.terminate(myContext)

    # test: gem.initialize
    #       gem.loadModel
    #       gem.getModel
    #       gem.staticModel
    #       gem.add2Model
    #       gem.getMassProps
    #       gem.releaseModel
    #       gem.terminate
    def test_StaticModel(self):
        myContext = gem.initialize()
        myModel   = gem.loadModel(myContext, sample_file)

        server, filename, modeler, uptodate, myBReps, nparam, nbranch, nattr = gem.getModel(myModel)

        newModel = gem.staticModel(myContext)
        gem.add2Model(newModel, myBReps[0])
        gem.add2Model(newModel, myBReps[0], (0.5, 0, 0, 2,  0, 0.5, 0, 0,  0, 0, 0.5, 0))

        server, filename, modeler, uptodate, myBReps, nparam, nbranch, nattr = gem.getModel(newModel)

        massProps1 = gem.getMassProps(myBReps[0], "BREP", 0)
        massProps2 = gem.getMassProps(myBReps[1], "BREP", 0)

        self.assertAlmostEqual(massProps1[0],   8 * massProps2[0])     # volume
        self.assertAlmostEqual(massProps1[1],   4 * massProps2[1])     # surface area
        self.assertAlmostEqual(massProps1[2],   2 * massProps2[2] - 4)     # xcg
        self.assertAlmostEqual(massProps1[3],   2 * massProps2[3])     # ycg
        self.assertAlmostEqual(massProps1[4],   2 * massProps2[4])     # zcg
        self.assertAlmostEqual(massProps1[5],  32 * massProps2[5])     # Ixx
        self.assertAlmostEqual(massProps1[6],  32 * massProps2[6])     # Ixy
        self.assertAlmostEqual(massProps1[7],  32 * massProps2[7])     # Ixz
        self.assertAlmostEqual(massProps1[8],  32 * massProps2[8])     # Iyx
        self.assertAlmostEqual(massProps1[9],  32 * massProps2[9])     # Iyy
        self.assertAlmostEqual(massProps1[10], 32 * massProps2[10])     # Iyz
        self.assertAlmostEqual(massProps1[11], 32 * massProps2[11])     # Izx
        self.assertAlmostEqual(massProps1[12], 32 * massProps2[12])     # Izy
        self.assertAlmostEqual(massProps1[13], 32 * massProps2[13])     # Izz

        gem.releaseModel(newModel)
        gem.releaseModel(myModel)
        gem.terminate(myContext)

    # test: gem.initialize
    #       gem.loadModel
    #       gem.getModel
    #       gem.setAttribute(BRANCH)
    #       gem.getBranch
    #       gem.retAttribute(BRANCH)
    #       gem.setSuppress
    #       gem.regenModel
    #       gem.releaseModel
    #       gem.terminate
    def test_Branches(self):
        myContext = gem.initialize()
        myModel   = gem.loadModel(myContext, sample_file)

        server, filename, modeler, uptodate, myBReps, nparam, nbranch, nattr = gem.getModel(myModel)

        for ibranch in range(2, nbranch+1):
            gem.setAttribute(myModel, "BRANCH", ibranch, "s_attr", "$branch attribute")

            bname, btype, suppress, parents, children, nattr = gem.getBranch(myModel, ibranch)
            gem.setAttribute(myModel, "BRANCH", ibranch, "s_attr", "$branch attribute")

        ibranch = 9
        gem.setSuppress(myModel, ibranch, 1)

        bname, btype, suppress, parents, children, nattr = gem.getBranch(myModel, ibranch)
        self.assertEqual(bname,    "feature")
        self.assertEqual(btype,    "extrude")
        self.assertEqual(suppress, 1        )
        self.assertEqual(parents,  (8,)     )
        self.assertEqual(children, (10,)    )
        self.assertEqual(nattr,    1        )

        gem.regenModel(myModel)

        for ibranch in range(2, nbranch+1):
            bname, btype, suppress, parents, children, nattr = gem.getBranch(myModel, ibranch)
            if (ibranch == 2):
                self.assertEqual(nattr, 4, "ibranch=%d" % ibranch)
            else:
                self.assertEqual(nattr, 1, "ibranch=%d" % ibranch)

            aindex, values = gem.retAttribute(myModel, "BRANCH", ibranch, "s_attr")
            self.assertEqual(aindex, nattr,               "ibranch=%d" % ibranch)
            self.assertEqual(values, "$branch attribute", "ibranch=%d" % ibranch)

        gem.releaseModel(myModel)
        gem.terminate(myContext)


    # test: gem.initialize
    #       gem.loadModel
    #       gem.getModel
    #       gem.setAttribute(PARAM)
    #       gem.getParam
    #       gem.getAttribute(PARAM)
    #       gem.setParam
    #       gem.regenModel
    #       gem.releaseModel
    #       gem.terminate
    def test_Parameters(self):
        myContext = gem.initialize()
        myModel   = gem.loadModel(myContext, sample_file)

        server, filename, modeler, uptodate, myBReps, nparam, nbranch, nattr = gem.getModel(myModel)

        for iparam in range(1, nparam+1):
            gem.setAttribute(myModel, "PARAM", iparam, "s_attr", "param attribute")
            gem.setAttribute(myModel, "PARAM", iparam, "i_attr", (     iparam,)   )
            gem.setAttribute(myModel, "PARAM", iparam, "r_attr", (10.0*iparam,)   )

        iparam = 4
        pname, bflag, order, values, nattr = gem.getParam(myModel, iparam)
        self.assertEqual(pname,     "ymax")
        self.assertEqual(bflag,     0     )
        self.assertEqual(order,     0     )
        self.assertEqual(values[0], 1.0   )
        self.assertEqual(nattr,     3     )

        gem.setParam(myModel, iparam, (1.5,))

        gem.regenModel(myModel)

        pname, bflag, order, values, nattr = gem.getParam(myModel, iparam)
        self.assertEqual(pname,     "ymax")
        self.assertEqual(bflag,     0     )
        self.assertEqual(order,     0     )
        self.assertEqual(values[0], 1.5   )
        self.assertEqual(nattr,     3     )

        for iparam in range(1, nparam+1):
            pname, bflag, order, values, nattr = gem.getParam(myModel, iparam)
            self.assertEqual(nattr, 3, "iparam=%d" % iparam)

            aindex, values = gem.retAttribute(myModel, "PARAM", iparam, "s_attr")
            self.assertEqual(aindex, 1,                 "iparam=%d" % iparam)
            self.assertEqual(values, "param attribute", "iparam=%d" % iparam)

            aindex, values = gem.retAttribute(myModel, "PARAM", iparam, "i_attr")
            self.assertEqual(aindex,    2,      "iparam=%d" % iparam)
            self.assertEqual(values[0], iparam, "iparam=%d" % iparam)

            aindex, values = gem.retAttribute(myModel, "PARAM", iparam, "r_attr")
            self.assertEqual(aindex, 3,              "iparam=%d" % iparam)
            self.assertEqual(values[0], 10.0*iparam, "iparam=%d" % iparam)

        gem.releaseModel(myModel)
        gem.terminate(myContext)

    # test: gem.initialize
    #       gem.loadModel
    #       gem.getModel
    #       gem.getBRepInfo
    #       gem.getAttribute(BREP)
    #       gem.getNode
    #       gem.getEdge
    #       gem.getLoop
    #       gem.getFace
    #       gem.getShell
    #       gem.releaseModel
    #       gem.terminate
    def test_BRep(self):
        myContext = gem.initialize()
        myModel   = gem.loadModel(myContext, sample_file)

        server, filename, modeler, uptodate, myBReps, nparam, nbranch, nattr = gem.getModel(myModel)
        self.assertEqual(nattr, 0)

        for myBRep in myBReps:
            box, type, nnode, nedge, nloop, nface, nshell, nattr = gem.getBRepInfo(myBRep)
            self.assertEqual(nattr, 2)

            aname, values = gem.getAttribute(myBRep, "BREP", 0, 1)
            self.assertEqual(aname,     "body")
            self.assertEqual(values[0], 18    )

            # check that all nodes are in box
            for inode in range(1, nnode+1):
                xyz, nattr = gem.getNode(myBRep, inode)
                self.assertEqual(nattr, 0)

                self.assertTrue(xyz[0] >= box[0], "x[%d] < box[0]" % inode)
                self.assertTrue(xyz[1] >= box[1], "y[%d] < box[1]" % inode)
                self.assertTrue(xyz[2] >= box[2], "z[%d] < box[2]" % inode)
                self.assertTrue(xyz[0] <= box[3], "x[%d] > box[3]" % inode)
                self.assertTrue(xyz[1] <= box[4], "y[%d] > box[4]" % inode)
                self.assertTrue(xyz[2] <= box[5], "z[%d] > box[5]" % inode)

            # check consistency of all loops
            for iloop in range(1, nloop+1):
                iface, type, edges, nattr = gem.getLoop(myBRep, iloop)
                self.assertEqual(nattr, 0, "iloop=%d" % iloop)

                # loop and face point to each other
                ID, uvbox, norm, loops, nattr = gem.getFace(myBRep, iface)
                self.assertTrue(iloop in loops, "iloop=%d, iface=%d" % (iloop, iface))

                # edge and face point to each other
                for i in range(len(edges)):
                    tlimit, nodes, faces, nattr = gem.getEdge(myBRep, edges[i])
                    self.assertTrue(iface in faces, "iloop-%d, i=%d" % (iloop, i))

                # edges link end-to-end in loop
                tlimit, nodes, faces, nattr = gem.getEdge(myBRep, edges[0])
                iend = nodes[1]

                for i in range(1, len(edges)):
                    tlimit, nodes, faces, nattr = gem.getEdge(myBRep, edges[i])
                    self.assertEqual(iend, nodes[0], "iloop=%d, i=%d" % (iloop, i))
                    iend = nodes[1]

            # check consistency of all shells
            for ishell in range(1, nshell+1):
                type, faces, nattr = gem.getShell(myBRep, ishell)
                self.assertEqual(nattr, 0)

        gem.releaseModel(myModel)
        gem.terminate(myContext)

    # test: gem.initialize
    #       gem.loadModel
    #       gem.newDRep
    #       gem.tesselDRep
    #       gem.getTessel
    #       gem.destroyDRep
    #       gem.releaseModel
    #       gem.terminate
    def test_DRep(self):
        myContext = gem.initialize()
        myModel   = gem.loadModel(myContext, sample_file)
        myDRep    = gem.newDRep(myModel)
        gem.tesselDRep(myDRep, 0, 0, 0, 0)

        triArray, xyzArray = gem.getTessel(myDRep, 1, 1)

        npnt = (xyzArray.shape)[0]
        ntri = (triArray.shape)[0]

        # make sure that triangle pointers are consistent
        for itri in range(1, ntri+1):
            for iside in [0, 1, 2]:
                jtri = triArray[itri-1,iside]
                if not ((jtri > 0) and (jtri <= npnt)):
                    self.fail('jtri not in (0 to %d)' % npnt)

        gem.destroyDRep(myDRep)
        gem.releaseModel(myModel)
        gem.terminate(myContext)

    # do not test: gem.getBRepOwner
    #              gem.getWire
    #              gem.isEquivalent
    #              gem.plotDRep
    #              gem.saveModel
    #              gem.solidBoolean


class GEMParametricGeometryTestCase(unittest.TestCase):

    def setUp(self):
        self.csm_input = """
# bottle2 (from OpenCASCADE tutorial)
# written by John Dannenhoffer

# default design parameters
despmtr   width               10.00
despmtr   depth                4.00
despmtr   height              15.00
despmtr   neckDiam             2.50
despmtr   neckHeight           3.00
despmtr   wall                 0.20     wall thickness (in neck)
despmtr   filRad1              0.25     fillet radius on body of bottle
despmtr   filRad2              0.10     fillet radius between bottle and neck

# basic bottle shape (filletted)

set       baseHt    height-neckHeight

skbeg     -width/2  -depth/4  0
   cirarc 0         -depth/2  0         +width/2  -depth/4  0
   linseg +width/2  +depth/4  0
   cirarc 0         +depth/2  0         -width/2  +depth/4  0
   linseg -width/2  -depth/4  0
skend
extrude   0         0         baseHt
fillet    filRad1

# neck
cylinder  0         0         baseHt    0         0         height      neckDiam/2

# join the neck to the bottle and apply a fillet at the union
union
fillet    filRad2

# hollow out bottle
hollow    wall      18

end
        """
        self.tdir = tempfile.mkdtemp()
        self.model_file = os.path.join(self.tdir, 'bottle.csm')
        with open(self.model_file, 'w') as f:
            f.write(self.csm_input)

    def tearDown(self):
        shutil.rmtree(self.tdir)

    def test_GEMParametricGeometry(self):
        geom = GEMParametricGeometry()
        geom.model_file = self.model_file
        params = geom.listParameters()
        expected_inputs = set(['width', 'depth', 'height', 
            'neckDiam', 'neckHeight', 'wall',
            'filRad1', 'filRad2'])
        self.assertEqual(expected_inputs, 
            set([k for k,v in params if v['iotype']=='in']))
        geom.terminate()


if __name__ == "__main__":
    unittest.main()
