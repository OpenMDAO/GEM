/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             DRep Include
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */


/* Data Transfer Methods */

#define GEM_INTERP      0
#define GEM_CONSERVE    1
#define GEM_MOMENTS     2


/* structures */


  typedef struct {
    int BRep;                   /* BRep index */
    int index;                  /* entity index */
  } gemPair;


/*
 * defines approximated spline data
 */
typedef struct {
  int    nrank;               /* number of members in the state-vector */
  int    periodic;            /* 0 nonperiodic, 1 periodic */
  int    nts;                 /* number of spline points */
  double *interp;             /* spline data */
  double trange[2];           /* mapped t range */
  int    ntm;                 /* number of t mapping points */
  double *tmap;               /* mapping data -- NULL unmapped */
} gemAprx1D;


typedef struct {
  int    nrank;               /* number of members in the state-vector */
  int    periodic;            /* 0 non, 1 U, 2 V, 3 periodic in both U&V */
  int    nus;                 /* number of spline points in U */
  int    nvs;                 /* number of spline points in V */
  double *interp;             /* spline data */
  double urange[2];           /* mapped U range */
  double vrange[2];           /* mapped V range */
  int    num;                 /* number of U mapping points */
  int    nvm;                 /* number of V mapping points */
  double *uvmap;              /* mapping data -- NULL unmapped */
} gemAprx2D;


/*
 * defines Face specific information for a discretized point
 */
  typedef struct {
    gemPair bface;              /* BRep & Face indices */
    double  uv[2];              /* uv coordinates for the Face */  
  } gemFaceUV;


/*
 * defines the geometric and topological information associated with a point in 
 * the discretization.
 */
  typedef struct {
    int    lTopo;               /* Lowest Topo (-) Node Index, (+) Edge Index */
    int    nFaces;              /* # Faces (Face 1, Edge usually 2) */
    union {
      int  faces[2];            /* Face or Edge indices into FaceUV */
      int  *multi;              /* More than 2 Faces */
    } findices;
    double xyz[3];              /* the 3D coordinates */
  } gemPoints;


/*
 * defines the element discretization type by the number of sides and the
 * number of reference positions within the element. For example:
 * simple tri:  nside = nref = 3; st = {0.0,0.0, 1.0,0.0, 0.0,1.0}
 * simple quad: nside = nref = 4; st = {0.0,0.0, 1.0,0.0, 1.0,1.0, 0.0,1.0}
 * Note: for higher-order discretizations always list the outer positions
 *       first, in order (using a right-handed traversal) -- this helps to
 *       define the neighbor connectivity (as seen below)
 */
  typedef struct {
    int    nside;               /* number of sides for the element */
    int    nref;                /* number of reference points */
    double *st;                 /* [s,t] reference coordinates in the element 
                                   nref*2 in length */
  } gemEleType;


/*
 * defines the element discretization and neighbors. The numbering of neighbors
 * is based on the side/reference indices. For example:
 *                         neighbors                           neighbors
 *        2          tri-side   vertices          4          side    vertices
 *       / \             0        1 2            / \           0        1 2
 *      /   \            1        2 0           5   3          1        2 3
 *     /     \           2        0 1          /  6  \         2        3 4
 *    0-------1                               /       \        3        4 5
 *                                           0----1----2       4        5 0
 *                         neighbors                           5        0 1
 *    3-------2      quad-side  vertices         nside = 6,  nref = 7   
 *    |       |          0        1 2
 *    |       |          1        2 3            6               neighbors
 *    |       |          2        3 0        3---.---2     quad-side  vertices
 *    0-------1          3        0 1        |       |         0        1 2
 *                                          7.   8   .5        1        2 3   
 *                         neighbors         |       |         2        3 0 
 *    4-------3        side     vertices     0---.---1         3        0 1 
 *    |       |          0        1 2            4
 *    |       2          1        2 3
 *    |       |          2        3 4            nside = 4,  nref = 9
 *    0-------1          3        4 0
 *                       4        0 1
 *   nside = nref = 5
 */
  typedef struct {
    gemEleType *type;           /* the element type */
    int        *indices;        /* the indices for geom reference positions 
                                   nref in length */
    int        *neighbors;      /* number element neighbors (bias 1) nside long
                                   if negative it is an Edge index */
  } gemElement;


/*
 * allows for the data to be defined on a different stencil than the vertices
 * used to define the base Element geometry
 */
  typedef struct {
    gemEleType *type;           /* the element type */
    int        *dindices;       /* the indices for data reference positions
                                   nref in length */
  } gemEleData;


/*
 * defines a discretized quilt (the collection of Faces)
 *
 * specifies the connectivity based on a collection of Element Types and the
 * elements referencing the types. nGpts refers to the number of indices
 * referenced by the geometry positions in the element, nVerts is the number of 
 * indices used in the data representation for the element. nPoints refers to 
 * the complete set of positions used for both nGpts and nVerts. These can all 
 * be the same and then 'edata', 'geomIndices' and 'dataIndices' can be NULL 
 * for simple nodal or isoparametric discretizations.
 */
  typedef struct {
    int        nPoints;         /* number of entries in point definition */
    gemFaceUV  *faceUVs;        /* suite of Face definitions for Points */
    gemPoints  *points;         /* the Point definitions */
    int        nGpts;           /* number of geom positions referenced */
    int        *geomIndices;    /* indices into Points for geom (or NULL) */
    int        nVerts;          /* number of data positions referenced */
    int        *dataIndices;    /* indices into Points for data (or NULL) */
    int        nTypes;          /* number of Element Types */
    gemEleType *types;          /* the Element Types */
    int        nElems;          /* number of Elements */
    gemElement *elems;          /* the Elements */
    gemEleData *edata;          /* the Element Data Rep (can be NULL) */
    void       *ptrm;           /* pointer for optional method use */
  } gemQuilt;


/*
 * defines an Edge Discretization
 */
  typedef struct {
    int    npts;                /* number of points */
    double *xyzs;               /* coordinates -- 3*npts in length */
    double *ts;                 /* t parameters -- npts in length */
  } gemDEdge;


/*
 * defines a triangulation for viewing
 */
  typedef struct {
    int    ntris;               /* number of triangles */
    int    npts;                /* number of points in triangulation */
    int    *tris;               /* tri indices (bias 1) -- 3*ntris in len */
    double *xyzs;               /* 3D coordinates -- 3*npts in length */
    void   *ptrm;               /* pointer for optional method use */
  } gemTri;


/*
 * defines the connectivity for a triangulation
 */
  typedef struct {
    int    *tric;               /* tri neighbors (bias 1) -- 3*ntris in len */
    double *uvs;                /* 2D coordinates -- 2*npts in length */
    int    *vid;                /* vertex ID (type/ind) -- 2*npts in length */
  } gemTConn;


/*
 * defines a collection of data
 */
  typedef struct {
    int    npts;                /* number of points */
    int    rank;                /* rank */
    double *data;               /* data -- rank*npts in length */
  } gemCollct;


/*
 * defines the dataset in a vertex set
 */
  typedef struct {
    int       ivsrc;            /* 0 or index of src for transferred dataset */
    char      *name;            /* Dset name -- i.e. pressure, xyz, uv, etc */
    gemCollct dset;
  } gemDSet;


/*
 * defines the vertex set
 */
  typedef struct {
    char      *disMethod;       /* discretization method for interp/integrate */
    gemQuilt  *quilt;           /* the quilt definition or NULL (non-conn) */
    gemTri    *gbase;           /* triangulation of geometry-based dataset */
    gemTri    *dbase;           /* triangulation of data-based datasets */
    gemCollct *nonconn;         /* xyzs for non-connected VertexSet or NULL */
    int       nSets;            /* number of datasets */
    gemDSet   *sets;            /* the datasets */
  } gemVSet;


/* 
 * a cut vertex is defined by the referenece coordinates in both the elements
 * of the quilts being cut. All verts exist on the contour of the cut fragment, 
 * none interior.
 */
  typedef struct {
    double st1[2];              /* position in element ref coords -- quilt 1 */
    double st2[2];              /* position in element ref coords -- quilt 2 */
  } gemCutVert;


/*
 * a cut triangle refers to 3 cut vertices that produce a right-handed triangle
 * in both quilt element reference spaces. Neighbor info is encoded per tri
 * side as to interior side, a side from a quilt 1 or a quilt 2 element.
 * When from an exterior fragment side it is the neighbor side index times 2 
 * (bias 1), see the gemElement description above.
 */
  typedef struct {
    int indices[3];             /* tri cut indices (bias 1) */
    int neighbor[3];            /* triangle connectivity (bias 1)
                                   (+) neighbor is another cut triangle
                                   (-) neighbor is side/2 (odd is quilt 1) */
  } gemCutTri;


/*
 * cut element fragments -- all cut verts are stored in order traversing the
 * fragment in a right handed manner and provides a single closed contour. If
 * the cut between elements produces multiple fragments there will be multiple
 * of these descriptions each for a single closed contour.
 */
  typedef struct {
    int        e1Index;         /* element index in quilt 1 (bias 1) */
    int        e2Index;         /* element index in quilt 2 (bias 1) */
    int        nCutVerts;       /* number of cut vertices */
    gemCutVert *cutVerts;       /* the cut vertices */
    int        nCutTris;        /* number of cut triangles */
    gemCutTri  *cutTris;        /* the triangles */
  } gemCutFrag;


/*
 * the structure to hold the internal cut element data for one connected
 * Vertex set against another.
 */
  typedef struct gemCutter {
    int        ivs1;            /* index of VertexSet 1 (quilt 1) */
    int        ivs2;            /* index of VertexSet 2 (quilt 2) */
    int        nEle;            /* number of cut elements */
    gemCutFrag *elements;       /* the elements */
    struct gemCutter *next;     /* pointer to next set of cuts */
  } gemCutter;


/* 
 * defines the bound with its collection of vertex sets
 */
  typedef struct {
    int       nIDs;             /* number of IDs for this Boundary */
    int       *IDs;             /* index to the IDs in DRep */
    gemPair   *indices;         /* BRep/Face pairs for active Faces */
    gemAprx2D *surface;         /* NULL for single Face */
    double    uvbox[4];         /* the uv limits */
    int       nVSet;            /* number of Vertex Sets */
    gemVSet   *VSet;            /* the Vertex Sets */
    gemCutter *cutList;         /* the cut elements by Vset pairs */
  } gemBound;


/*
 * defines the tesellation as a collection of discretized faces & edges
 */
  typedef struct {
    int      nEdges;            /* number of Edges */
    gemDEdge *Edges;            /* the Edge discretizations -- xyzs & ts */
    int      nFaces;            /* number of Faces */
    gemTri   *Faces;            /* the Face tessellations */
    gemTConn *conns;            /* the Face connectivities */
  } gemTRep;


/*
 * defines the DRep
 */
  typedef struct gemDRep {
    int      magic;             /* magic number to check for authenticity */
    gemModel *model;            /* model pointer (master-model & geometry) */
    int       nIDs;             /* number of IDs found in the geometry */
    char     **IDs;             /* the persistent Face IDs */
    int      nBReps;            /* number of BReps found in the Model */
    gemTRep  *TReps;            /* the tessellation of the BReps */
    int      nBound;            /* the number of Boundaries found in the DRep */
    gemBound *bound;            /* the Boundaries */
    gemAttrs *attr;             /* attribute structure */
    struct gemDRep *prev;       /* previous DRep */
    struct gemDRep *next;       /* next DRep */
  } gemDRep;                                            



/* create an empty DRep attached to a model
 *
 * Returns a DRep associated with the Model. Static Models will be required 
 * to mix BReps from multiple sources.
 */
extern int
gem_newDRep(gemModel *model,            /* (in)  pointer to model */
            gemDRep  **drep);           /* (out) pointer to new DRep */


/* (re)tessellate the BReps in the DRep
 *
 * If all input triangulation parameters are set to zero, then a default 
 * tessellation is performed. If this data already exists, it is recomputed 
 * and overwritten. Use gem_getTessel or gem_getDiscrete to get the data 
 * generated on the appropriate entity.
 */
extern int
gem_tesselDRep(gemDRep *drep,           /* (in)  pointer to DRep */
               int     BRep,            /* (in)  BRep index in DRep - 0 all */
               double  angle,           /* (in)  tessellation parameters: */
               double  maxside,         /*       all zero -- default */
               double  sag);

               
/* get Edge Discretization
 *
 * Returns the polyline associated with the Edge.
 */
extern int
gem_getDiscrete(gemDRep *drep,          /* (in)  pointer to DRep */
                gemPair bedge,          /* (in)  BRep/Edge index in DRep */
                int     *npts,          /* (out) number of vertices */
                double  *xyzs[]);       /* (out) pointer to the coordinates */


/* get Face Tessellation
 *
 * Returns the triangulation associated with the Face.
 */
extern int
gem_getTessel(gemDRep *drep,            /* (in)  pointer to DRep */
              gemPair bface,            /* (in)  BRep/Face index in DRep */
              int     *ntris,           /* (out) number of triangles */
              int     *npts,            /* (out) number of vertices */
              int     *tris[],          /* (out) pointer to triangles defns */
              double  *xyzs[]);         /* (out) pointer to the coordinates */


/* create a DRep that has Bound definitions copied from another DRep
 *
 * Creates a new DRep by populating empty Bounds based on an existing DRep. 
 * The unused persistent Face IDs are reported and pointer to the list should 
 * be freed by the programmer after the data is examined by gem_free. Though 
 * unused, these IDs are maintained in the appropriate Bound definitions 
 * (for possible later use).
 */
extern int
gem_copyDRep(gemDRep  *src,             /* (in)  pointer to DRep to be copied */
             gemModel *model,           /* (in)  pointer to new master model */
             gemDRep  **drep,           /* (out) pointer to the new DRep */
             int      *nIDs,            /* (out) # of IDs in model not in src */
             char     **IDs[]);         /* (out) array  of IDs (freeable) */


/* delete a DRep and all its storage
 *
 * Cleans up all data contained within the DRep. This also frees up the 
 * DRep structure.
 */
extern int
gem_destroyDRep(gemDRep *drep);         /* (in)  ptr to DRep to be destroyed */


/* create Bound in a DRep
 *
 * Create a new Bound within the DRep. The IDs are the unique and persistent 
 * strings returned via gem_getFace (that is, this ID persists across Model 
 * regenerations where the number of Faces and the Face order does not). 
 * After a Model regen all VertexSets (and tessellations) found in the DRep 
 * are removed but the Bound remains.
 */
extern int
gem_createBound(gemDRep *drep,          /* (in)  pointer to DRep */
                int     nIDs,           /* (in)  number of Face IDs */
                char    *IDs[],         /* (in)  array  of Face IDs */
                int     *ibound);       /* (out) index of new Bound */


/* add other IDs to a Bound
 *
 * Extends an existing Bound within the DRep by including more ID strings. 
 * This may be necessary if after a Model regen new Faces are found that 
 * would belong to this Bound. This may also be useful in a multidisciplinary 
 * setting for adding places where data transfer is performed. This call will
 * invalidate any existing data in the Bound and therefore acts like the 
 * existing connected Vertex Sets are newly created.
 */
extern int
gem_extendBound(gemDRep *drep,          /* (in)  pointer to DRep */
                int     ibound,         /* (in)  Bound to be extended */
                int     nIDs,           /* (in)  number of new Face IDs */
                char    *IDs[]);        /* (in)  array  of new Face IDs */


/* create a connected VertexSet
 *
 * Generates a new VertexSet (for connected Sets) and returns the index. This
 * invokes the function "gemDefineQuilt" in the disMethod so/DLL (specified for 
 * the connected VertexSet) based on the collection of Faces (Quilt) currently
 * defined in the Bound. Note that VertexSets can overlap in space but the 
 * internals of a Quilt may not.
 */
extern int
gem_createVset(gemDRep *drep,           /* (in)  pointer to DRep */
               int     ibound,          /* (in)  index of Bound */
               char    disMethod[],     /* (in)  the name of the shared object
                                                 or DLL used for describing
                                                 interploation & integration */
               int     *ivs);           /* (out) index of Vset in Bound */


/* create an unconnected VertexSet
 *
 * Makes an unconnected VertexSet. This can be used for receiving interpolation 
 * data and performing evaluations. An unconnected VertexSet cannot specify a
 * (re)parameterization (by invoking gem_paramBound), be the source of data
 * transfers or get sensitivities.
 */
extern int
gem_makeVset(gemDRep *drep,             /* (in)  pointer to DRep */
             int     ibound,            /* (in)  index of Bound */
             int     npts,              /* (in)  number of points */
             double  xyz[],             /* (in)  3*npts XYZ positions */
             int     *ivs);             /* (out) index of Vset in Bound */


/* (re)parameterize a Bound
 *
 * Creates a global parameterization (u,v) based on all connected VertexSets
 * in the bound. If the parameterization exists, making this call does nothing.
 *
 * This function must be performed before any data is put into a VertexSet or
 * a transfer is requested. If there is only 1 Face specified in the Bound then 
 * the native parameterization for that Face is used for the DRep functions. 
 * DataSets named "xyz" and "uv" are produced for all of the VertexSets found 
 * in the Bound (where "uv" is the single parameterization for the Bound).
 * DataSets "xyzd" and "uvd" may also be generated if the data positions differ
 * from the geometric reference positions.
 */
extern int
gem_paramBound(gemDRep *drep,           /* (in)  DRep pointer */
               int     ibound);         /* (in)  Bound index (1-bias) */


/* put data into a Vset
 *
 * Stores the named data in a connected VertexSet. This allows for the 
 * assignment of field data, for example "pressure" (rank = 1) can be associated 
 * with the vertex ref positions. Only one VertexSet within a Bound can have a 
 * unique name, therefore it is an error to put a DataSet on a VertexSet if the 
 * name already exists (or the name is reserved such as "xyz" or "uv", see 
 * below). If the name exists in this VertexSet the data will be overwritten.
 */
extern int
gem_putData(gemDRep *drep,              /* (in)  pointer to DRep */
            int     ibound,             /* (in)  index of Bound */
            int     ivs,                /* (in)  index of Vset in Bound */
            char    name[],             /* (in)  dataset name:
                                 if name exists in ibound
                                    if name associated with ivs and rank agrees
                                       overwrite dataset
                                    else
                                       GEM_BADSETNAME
                                  else
                                     create new dataset */
            int     nverts,             /* (in)  number of verts -- must match
                                                 VSet total from getVsetInfo */
            int     rank,               /* (in)  # of elements per vertex */
            double  data[]);            /* (in)  rank*nverts data values or NULL
                                                 deletes the Dset if exists */


/* get data from a Vset
 *
 * Returns (or computes and returns) the data found in the VertexSet. If another 
 * VertexSet in the Bound has the name, then data transfer is performed. 
 * Transfers are performed via the "xferMethod".
 * 
 * The following reserved names automatically generate data (with listed rank) 
 * and are all geometry based (nGpts in length not nVerts of getVsetInfo):
 * coordinates:         xyz  (3)        parametric coords:      uv       (2)
 * d(xyz)/d(uv):        d1   (6)        d2(xyz)/d(uv)2:         d2       (9)
 * GeomCurvature:       curv (8)        Inside/Outside:         inside   (1)
 *                                      Sensitivity:            d/dPARAM (3)
 *		            Where PARAM is the full parameter name in the Model
 *
 * The following reserved names automatically generate data (with listed rank)
 * and are all data based (nVerts in length not nGpts of getVsetInfo):
 * data coordinates:    xyzd (3)        data parametrics:       uvd      (2)
 *
 * Data that is explicity placed via gem_putData is based on the number of 
 * data reference positions (nVerts) not the geometry positions (nGpts).
 */
extern int
gem_getData(gemDRep *drep,              /* (in)  pointer to DRep */
            int     ibound,             /* (in)  index of Bound */
            int     ivs,                /* (in)  index of Vset in Bound */
            char    name[],             /* (in)  dataset name */
            int     xferMethod,         /* (in)  GEM_INTERP, GEM_CONSERVE, or
                                                 GEM_MOMENTS. Note: GEM_CONSERVE 
                                                 and GEM_MOMENTS can be added */
            int     *npts,              /* (out) number of points/verts */
            int     *rank,              /* (out) # of elements per vertex */
            double  *data[]);           /* (out) pointer to data values */


/* get Vset-based Triangulation
 *
 * Returns the triangulation associated with the Bound for this Vset. This
 * internally forces a call to the function "gemTriangulate" in the disMethod 
 * so/DLL to fill in the appropriate data (which is done in a way that using
 * normal triangle color rendering techniques produces correct imagery). The 
 * triangle indices returned can be used as reference into the DataSet (bias 1) 
 * in order to view the results.
 */
extern int
gem_triVset(gemDRep *drep,              /* (in)  pointer to DRep */
            int     ibound,             /* (in)  index of Bound */
            int     ivs,                /* (in)  index of Vset in Bound */
            char    name[],             /* (in)  dataset name (used to trigger
                                                 geom vs data ref) */
            int     *ntris,             /* (out) number of triangles */
            int     *npts,              /* (out) number of vertices */
            int     *tris[],            /* (out) pointer to triangles defns */
            double  *xyzs[]);           /* (out) pointer to the coordinates */


/* get info about a DRep
 *
 * Returns current information about a DRep. Many DRep functions refer to a 
 * BRep by index (bias 1), you must use the Model to get the information about
 * the included BReps where the listed order is the index. Use gem_getModel to 
 * get the number of BReps and the actual BRep pointers. 
 */
extern int
gem_getDRepInfo(gemDRep  *drep,         /* (in)  pointer to DRep */
                gemModel **model,       /* (out) owning Model pointer */
                int      *nbound,       /* (out) number of Bounds */
                int      *nattr);       /* (out) number of attributes */


/* get info about a Bound
 *
 * Returns current information about a Bound in a DRep. Any BRep/Face pair 
 * which has the value {0,0} reflects an inactive ID. The uvbox returns 
 * {0.0,0.0,0.0,0.0} if the Bound has yet to be parameterized (see 
 * gem_paramBound). Note that the uvbox returned is different than that of 
 * the Face's uvbox if there is more than a single Face. In this case the 
 * aggregate Faces are fit to a single surface and the uvbox reflects this 
 * new "skin".
 */
extern int
gem_getBoundInfo(gemDRep *drep,         /* (in)  pointer to DRep */
                 int     ibound,        /* (in)  index of Bound */
                 int     *nIDs,         /* (out) number of Face IDs */
                 int     *iIDs[],       /* (out) array  of Face ID indices */
                 gemPair *indices[],    /* (out) BRep/Face active pairs */
                 double  uvlimits[],    /* (out) UV box (umin,umax,vmin,vmax) */
                 int     *nvs);         /* (out) number of Vsets */


/* get info about a Vset
 *
 * Returns information about a VertexSet. This includes the number of 
 * geometric and data reference vertices and the number of DataSets. Pointers 
 * to collections of names and ranks should be freed by the programmer after 
 * they are used by invoking gem_free.
 */
extern int
gem_getVsetInfo(gemDRep *drep,          /* (in)  pointer to DRep */
                int     ibound,         /* (in)  index of Bound */
                int     ivs,            /* (in)  index of Vset in Bound */
                int     *vstype,        /* (out) 0-connected, 1-unconnected */
                int     *nGpts,         /* (out) total number of Geom points */
                int     *nVerts,        /* (out) total number of Data verts */
                int     *nset,          /* (out) number of datasets */
                char    **names[],      /* (out) ptr to Dset names (freeable) */
                int     *ivsrc[],       /* (out) ptr to Dset owner 
                                                 0 or index of src (freeable) */
                int     *ranks[]);      /* (out) ptr to Dset ranks (freeable) */
                


/* ********************* transfer method (loadable) code ******************** */

/* all dynamically loadable transfer methods are defined by the name of the
   shared object/DLL and each must contain the following 6 functions:  */


extern int
gemDefineQuilt(const
               gemDRep  *drep,          /* (in)  the DRep pointer -- read-only */
               int      nFaces,         /* (in)  number of BRep/Face pairs
                                                 to be filled */
               gemPair  bface[],        /* (in)  index to the pairs in DRep */
               gemQuilt *quilt);        /* (out) ptr to Quilt to be filled */


extern void
gemFreeQuilt(gemQuilt *quilt);          /* (in)  ptr to the Quilt to free */


extern int
gemTriangulate(gemQuilt *quilt,         /* (in)  the quilt description */
               int      geomFlag,       /* (in)  0 - data ref, 1 - geom based */
               gemTri   *tessel);       /* (out) ptr to the triangulation */


extern void
gemFreeTriangles(gemTri *tessel);       /* (in)  ptr to triangulation to free */


extern int
gemInterpolation(gemQuilt *quilt,       /* (in)  the quilt description */
                 int      geomFlag,     /* (in)  0 - data ref, 1 - geom based */
                 int      eIndex,       /* (in)  element index (bias 1) */
                 double   st[],         /* (in)  the element ref coordinates */
                 int      rank,         /* (in)  data depth */
                 double   data[],       /* (in)  values (rank*npts in length) */
                 double   result[]);    /* (out) interpolated result - (rank) */


extern int
gemIntegration(gemQuilt   *quilt,       /* (in)  the quilt to integrate upon */
               int        whichQ,       /* (in)  1 or 2 (cut quilt index) */
               int        nFrags,       /* (in)  # of fragments to integrate */
               gemCutFrag frags[],      /* (in)  cut element fragments */
               int        rank,         /* (in)  data depth */
               double     data[],       /* (in)  values (rank*npts in length) */
               double     xyzs[],       /* (in)  3D coordinates */
               double     result[]);    /* (out) integrated result - (rank) */
