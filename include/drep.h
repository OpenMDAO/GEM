/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             DRep Include
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include "prm.h"


/* Data Transfer Methods */

#define GEM_INTERP      0
#define GEM_CONSERVE    1


/* structures */


  typedef struct {
    int BRep;                   /* BRep index */
    int index;                  /* entity index */
  } gemPair;


/*
 * defines approximated spline data
 */
typedef struct {
  int    nrank;                 /* number of members in the state-vector */
  int    periodic;              /* 0 nonperiodic, 1 periodic */
  int    nts;                   /* number of spline points */
  double *interp;               /* spline data */
  double trange[2];             /* mapped t range */
  int    ntm;                   /* number of t mapping points */
  double *tmap;                 /* mapping data -- NULL unmapped */
} gemAprx1D;


typedef struct {
  int    nrank;                 /* number of members in the state-vector */
  int    periodic;              /* 0 non, 1 U, 2 V, 3 periodic in both U&V */
  int    nus;                   /* number of spline points in U */
  int    nvs;                   /* number of spline points in V */
  double *interp;               /* spline data */
  double urange[2];             /* mapped U range */
  double vrange[2];             /* mapped V range */
  int    num;                   /* number of U mapping points */
  int    nvm;                   /* number of V mapping points */
  double *uvmap;                /* mapping data -- NULL unmapped */
} gemAprx2D;


/*
 * defines Face specific information for a discretized point
 */
  typedef struct {
    int    owner;               /* Body/Face index in quilt (bias 1) */
    double uv[2];               /* uv coordinates in the Face */
  } gemFaceUV;


/*
 * defines the geometric and topological information associated with a 
 * geometric point in the discretization.
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
 * defines the element discretization type by the number of reference positions
 * (for geometry and optionally data) within the element. For example:
 * simple tri:  nref = 3; ndata = 0; st = {0.0,0.0, 1.0,0.0, 0.0,1.0}
 * simple quad: nref = 4; ndata = 0; st = {0.0,0.0, 1.0,0.0, 1.0,1.0, 0.0,1.0}
 * internal triangles are used for the in/out predicates and represent linear
 *   triangles in [u,v] space.
 * ndata is the number of data referece positions, which can be zero for simple 
 *   nodal or isoparametric discretizations.
 * match points are used for conservative transfers. Must be set when data
 *   and geometry positions differ, specifically for discontinuous mappings.
 */
  typedef struct {
    int    nref;                /* number of geometry reference points */
    int    ndata;               /* number of data ref points -- 0 data at ref */
    int    nmat;                /* number of match points (0 -- match at 
                                   geometry reference points) */
    int    ntri;                /* number of triangles to represent the elem */
    double *gst;                /* [s,t] geom reference coordinates in the 
                                   element -- 2*nref in length */
    double *dst;                /* [s,t] data reference coordinates in the
                                   element -- 2*ndata in length */
    double *matst;              /* [s,t] positions for match points - NULL
                                   when using reference points (2*nmat long) */
    int    *tris;               /* the triangles defined by reference indices
                                   (bias 1) -- 3*ntri in length */
  } gemEleType;


/*
 * defines the element discretization for geometric and optionally data 
 * positions.
 */
  typedef struct {
    int tIndex;                 /* the element type index (bias 1) */
    int owner;                  /* index into bfaces (bias 1) */
    int *gIndices;              /* the point indices (bias 1) for geom reference
                                   positions -- nref in length */
    int *dIndices;              /* the vertex indices (bias 1) for data ref
                                   positions -- ndata in length or NULL */
  } gemElement;


/*
 * defines a discretized quilt (the collection of Faces)
 *
 * specifies the connectivity based on a collection of Element Types and the
 * elements referencing the types. nPoints refers to the number of indices
 * referenced by the geometric positions in the element which may be different
 * from nVerts which is the number of positions used for the data representation 
 * in the element. For simple nodal or isoparametric discretizations, nVerts is
 * the same as nPoints and verts is set to NULL.
 */
  typedef struct {
    int        paramFlg;        /* 1 - use, 0 - candidate, -1 - don't use */
    int        nbface;          /* number of body/face pairs */
    gemPair    *bfaces;         /* body/face pairs */
    int        nFaceUVs;        /* length of faceUVs for geometry ref pos */
    gemFaceUV  *faceUVs;        /* suite of Face definitions for Points */
    int        nPoints;         /* number of entries in the point definition */
    gemPoints  *points;         /* the Point definitions (nPoints long) */
    int        nVerts;          /* number of data reference positions */
    gemFaceUV  *verts;          /* data ref positions -- NULL if same as geom */
    int        nTypes;          /* number of Element Types */
    gemEleType *types;          /* the Element Types (nTypes in length) */
    int        nElems;          /* number of Elements */
    gemElement *elems;          /* the Elements (nElems in length) */
    void       *ptrm;           /* pointer for optional method use */
  } gemQuilt;


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
    gemCollct *nonconn;         /* xyzs for non-connected VertexSet or NULL */
    int       ntris;            /* number of triangles constructed from quilt */
    prmTri    *tris;            /* the triangles and neighbors */
    int       nSets;            /* number of datasets */
    gemDSet   *sets;            /* the datasets */
  } gemVSet;


/*
 * positions in a quilt for straight interpolation
 */
  typedef struct {
    int    eIndex;              /* the element index in source quilt */
    double st[2];               /* position in element ref coords -- source */
  } gemTarget;


/*
 * matching positions in a quilt for conservative data transfers
 */
  typedef struct {
    gemTarget source;           /* the positions in the source */
    gemTarget target;           /* the positions in the target */
  } gemMatch;


/*
 * structure to hold the intersection data for one Vertex set against another
 */
  typedef struct gemXfer {
    int       ivss;             /* index of source VertexSet */
    int       gflgs;            /* geomFlag for source */
    int       ivst;             /* index of target VertexSet */
    int       gflgt;            /* geomFlag for target */
    int       nPositions;       /* number of positions */
    int       nMatch;           /* number of match locations */
    gemTarget *position;        /* the positions in target for source */
    gemMatch  *match;           /* matching in source for target */
    struct gemXfer *next;       /* pointer to next set of cuts */
  } gemXfer;


/* 
 * defines the bound with its collection of vertex sets
 */
  typedef struct {
    int       nIDs;             /* number of IDs for this Boundary */
    int       *IDs;             /* index to the IDs in DRep */
    gemPair   *indices;         /* BRep/Face pairs for active Faces */
    gemPair   single;           /* single Face owner */
    gemAprx2D *surface;         /* NULL for single Face */
    double    uvbox[4];         /* the uv limits */
    int       nVSet;            /* number of Vertex Sets */
    gemVSet   *VSet;            /* the Vertex Sets */
    gemXfer   *xferList;        /* the transfer struct by Vset pairs */
  } gemBound;


/*
 * defines an Edge Discretization -- from kernel tessellation
 */
  typedef struct {
    int    npts;                /* number of points */
    double *xyzs;               /* coordinates -- 3*npts in length */
    double *ts;                 /* t parameters -- npts in length */
  } gemDEdge;


/*
 * defines a triangulation for viewing -- from kernel tessellation
 */
  typedef struct {
    int    ntris;               /* number of triangles */
    int    npts;                /* number of points in triangulation */
    int    *tris;               /* tri indices (bias 1) -- 3*ntris in len */
    int    *tric;               /* tri neighbors (bias 1) -- 3*ntris in len */
    double *xyzs;               /* 3D coordinates -- 3*npts in length or NULL */
    double *uvs;                /* 2D coordinates -- 2*npts in length */
    int    *vid;                /* vertex ID (type/index) -- 2*npts in length */
  } gemTri;


/*
 * defines the tessellation as a collection of discretized faces & edges
 */
  typedef struct {
    int      nEdges;            /* number of Edges */
    gemDEdge *Edges;            /* the Edge discretizations */
    int      nFaces;            /* number of Faces */
    gemTri   *Faces;            /* the Face tessellations */
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
 * Creates a global parameterization (u,v) based on one of the connected 
 * VertexSets in the bound. If all of the quilts refer to the same underlying
 * surface, then that surface parameterization is used. Otherwise, a quilt is
 * selected based on the flag in the quilt. "Candidates" are checked in order
 * of largest area. 
 * 
 * If the parameterization exists, making this call recomputes the
 * parameterization (which also resets each VertexSet as if newly created). 
 * In any case this then invokes "gemDefineQuilt" in the disMethod so/DLL 
 * (specified for the connected VertexSet) to fill the set of Faces that
 * define the Bound.
 *
 * This function must be performed before any data is put into a VertexSet or
 * a transfer is requested. DataSets named "xyz" and "uv" are produced for all 
 * of the VertexSets found in the Bound (where "uv" is for the single 
 * parameterization of the Bound). DataSets "xyzd" and "uvd" may also be 
 * generated if the data positions differ from the geometry reference positions.
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
            int     rank,               /* (in)  # of members per vertex/pt */
            double  data[]);            /* (in)  rank*nverts data values or NULL
                                                 deletes the Dset if exists */


/* get data from a Vset
 *
 * Returns (or computes and returns) the data found in the VertexSet. If another 
 * VertexSet in the Bound has the name, then data transfer is performed. 
 * Transfers are performed via the "xferMethod".
 * 
 * The following reserved names automatically generate data (with listed rank) 
 * and are all geometry based (nPoints in length not nVerts of getVsetInfo):
 * coordinates:         xyz  (3)        parametric coords:      uv       (2)
 * d(xyz)/d(uv):        d1   (6)        d2(xyz)/d(uv)2:         d2       (9)
 * GeomCurvature:       curv (8)        Sensitivity:            d/dPARAM (3)
 *		            Where PARAM is the full parameter name in the Model
 *
 * The following reserved names automatically generate data (with listed rank)
 * and are all data based (nVerts in length not nPoints of getVsetInfo):
 * data coordinates:    xyzd (3)        data parametrics:       uvd      (2)
 *
 * Data that is explicity placed via gem_putData is based on the number of 
 * data reference positions (nVerts) not the geometry positions (nPoints).
 */
extern int
gem_getData(gemDRep *drep,              /* (in)  pointer to DRep */
            int     ibound,             /* (in)  index of Bound */
            int     ivs,                /* (in)  index of Vset in Bound */
            char    name[],             /* (in)  dataset name */
            int     xferMethod,         /* (in)  GEM_INTERP or GEM_CONSERVE */
            int     *npts,              /* (out) number of points/verts */
            int     *rank,              /* (out) # of members per */
            double  *data[]);           /* (out) pointer to data values */


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
                int      *nIDs,         /* (out) number of IDs in this DRep */
                char     **IDs[],       /* (out) pointer to FaceIDs */
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
                 int     *iIDs[],       /* (out) Face ID indices into DRep */
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
                int     *nPoints,       /* (out) total number of Ref points */
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
               gemDRep  *drep,          /* (in)  the DRep pointer - read-only */
               int      nFaces,         /* (in)  number of BRep/Face pairs
                                                 to be filled */
               gemPair  bface[],        /* (in)  index to the pairs in DRep */
               gemQuilt *quilt);        /* (out) ptr to Quilt to be filled */


extern void
gemFreeQuilt(gemQuilt *quilt);          /* (in)  ptr to the Quilt to free */


extern void
gemInvEvaluation(gemQuilt *quilt,       /* (in)   the quilt description */
                 double   uvs[],        /* (in)   uvs that support the geom
                                                  (2*npts in length) */
                 double   uv[],         /* (in)   the uv position to get st */
                 int      *eIndex,      /* (both) element index (bias 1) */
                 double   st[]);        /* (both) the element ref coordinates
                                                  on input -- tri based guess */


extern int
gemInterpolation(gemQuilt *quilt,       /* (in)  the quilt description */
                 int      geomFlag,     /* (in)  0 - data ref, 1 - geom based 
                                                 specifies length of data */
                 int      eIndex,       /* (in)  element index (bias 1) */
                 double   st[],         /* (in)  the element ref coordinates */
                 int      rank,         /* (in)  # of members per */
                 double   data[],       /* (in)  values (rank*npts in length) */
                 double   result[]);    /* (out) interpolated result - (rank) */


extern int
gemInterpolate_bar(gemQuilt *quilt,     /* (in)   the quilt description */
                   int      geomFlag,   /* (in)   0 - data ref, 1 - geom based
                                                  specifies length of data */
                   int      eIndex,     /* (in)   element index (bias 1) */
                   double   st[],       /* (in)   the element ref coordinates */
                   int      rank,       /* (in)   # of members per */
                   double   res_bar[],  /* (in)   d(objective)/d(result) 
                                                  (rank in length) */
                   double   dat_bar[]); /* (both) d(objective)/d(data) 
                                                  (rank*npts in len) */


extern int
gemIntegration(gemQuilt *quilt,         /* (in)  the quilt description */
               int      geomFlag,       /* (in)  0 - data ref, 1 - geom based 
                                                 specifies length of data */
               int      eIndex,         /* (in)  element index (bias 1) */
               int      rank,           /* (in)  # of members per */
               double   data[],         /* (in)  values (rank*npts in length) */
               double   result[]);      /* (out) integrated result - (rank) */


extern int
gemIntegrate_bar(gemQuilt *quilt,       /* (in)   the quilt description */
                 int      geomFlag,     /* (in)   0 - data ref, 1 - geom based
                                                  specifies length of data */
                 int      eIndex,       /* (in)   element index (bias 1) */
                 int      rank,         /* (in)   # of members per */
                 double   res_bar[],    /* (in)   d(objective)/d(result) 
                                                  (rank in length) */
                 double   dat_bar[]);   /* (both) d(objective)/d(data) 
                                                  (rank*npts in len) */
