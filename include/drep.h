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


/* structures */

  typedef struct {
    int BRep;                   /* BRep index */
    int index;                  /* entity index */
  } gemPair;


  typedef struct {
    double t[2];                /* Edge ts for the vertices */
    int    node[2];             /* Node indices (0 for no Node) */
    int    edge;                /* Edge index */
  } gemSide; 


  typedef struct {
    int    nrank;               /* number of members in the state-vector */
    int    periodic;            /* 0 nonperiodic, 1 periodic */
    int    nts;                 /* number of spline points */
    double *interp;             /* spline data */
    double trange[2];           /* mapped t range */
    int    ntm;                 /* number of t mapping points */
    double *tmap;               /* mapping data -- NULL unmapped */
  } gemInt1D;


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
  } gemInt2D;
  

/*
 * This can be a mix of disjoint triangles and quadrilaterals or a single 
 * structured quadrilateral mesh. Neighbor information indexes the neighboring 
 * entity (bias one with quads after triangles) or the natural traversing 
 * around the quad mesh (counterclockwise from [1,1]). If the neighbor is 
 * negative, then the index refers to an entry in sides.
 */
  typedef struct {
    int     magic;              /* magic number to check for authenticity */
    int     nTris;              /* number of Tris */
    int     *Tris;              /* the tris (3 indices), or NULL for none */
    int     *tNei;              /* tri neighbors (3 per), or NULL */
    int     nQuad;              /* number of disjoint Quads */
    int     *Quad;              /* the quads (4 indices), or NULL if zero */
    int     *qNei;              /* quad neighbors (4 per), or NULL */    
    int     meshSz[2];          /* size of quad mesh (== npts) */
    int     nSides;             /* number of sides */
    gemSide *sides;             /* triangle/quad neighbor sides */  
  } gemConn;  

  
  typedef struct {
    int      ivsrc;             /* 0 or index of src for interpolated dataset */
    char     *name;             /* Dset name -- i.e. pressure, xyz, uv, etc */
    int      rank;              /* number of elements per vertex */
    double   *data;             /* npts*rank data values */
    gemInt2D *interp;           /* interpolant (only if ivsrc == 0) */
  } gemSet;

  
  typedef struct {
    gemPair  index;             /* BRep/Face indices */
    int      ID;                /* index for the persistent FaceID in DRep */
    int      npts;              /* number of points in tessellation */
    double   *xyz;              /* points */
    double   *uv;               /* point parameters or NULL for unconn */
    double   *guv;              /* NULL or "global" uvs */
    gemConn  *conn;             /* connectivity for the Face */
  } gemTface;

    
  typedef struct {
    int      npts;              /* number of points in this set */
    int      nFaces;            /* # of Faces for connected sets 
                                   -1 for unconnected */
    gemTface *faces;            /* the connected Vset definition */
    int      nSets;             /* number of datasets */
    gemSet   *Sets;             /* the vertex datasets */
  } gemVSet;
  
  
  typedef struct {
    int      nIDs;              /* number of IDs for this Boundary */
    int      *IDs;              /* index to the IDs in DRep */
    gemPair  *indices;          /* BRep/Face pairs for active Faces */
    gemInt2D *surface;          /* NULL for single Face */
    double   uvbox[4];		/* the uv limits */
    int      nVSet;             /* number of Vertex Sets */
    gemVSet  *VSet;             /* the Vertex Sets */
  } gemBound;
  
  
  typedef struct {
    int      nFaces;            /* number of Faces */
    gemTface *Faces;            /* the Face tessellations */
  } gemTRep;


  typedef struct gemDRep {
    int      magic;             /* magic number to check for authenticity */
    gemModel *model;            /* model pointer (master-model & geometry) */
    int       nIDs;             /* number of IDs found in the geometry */
    char     **IDs;             /* the persistent Face IDs */
    int      nBReps;            /* number of BReps found in the Model */
    gemTRep  *TReps;            /* the tessellation of the BRep */
    int      nBound;            /* the number of Boundaries found in the DRep */
    gemBound *bound;            /* the Boundaries */
    gemAttrs *attr;		/* attribute structure */
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


/* create an empty Connectivity Object
 *
 * Returns a new Connectivity container. All fields are either set to zero 
 * or NULL. Must be freed by gem_freeConn (which also frees any assigned
 * pointers using gem_free).
 */
extern int
gem_newConn(gemConn **conn);            /* (out) pointer to new Connectivity */


/* release a Connectivity Object
 *
 * Frees a Connectivity container. This uses gem_free to free any assigned
 * pointers (so they must have been filled with gem memory functions).
 */
extern int
gem_freeConn(/*@only@*/ gemConn *conn); /* (in)  Connectivity container */


/* (re)tessellate the BReps in the DRep
 *
 * If all input triangulation parameters are set to zero, then a default 
 * tessellation is performed. If this data already exists, it is recomputed 
 * and overwritten. Use gem_getTessel to get the data generated on the 
 * appropriate Face (and child entities).
 */
extern int
gem_tesselDRep(gemDRep *drep,           /* (in)  pointer to DRep */
               int     BRep,            /* (in)  BRep index in DRep - 0 all */
               double  angle,           /* (in)  tessellation parameters: */
               double  maxside,         /*       all zero -- default */
               double  sag);

               
/* get Face Tessellation
 *
 * Returns the triangulation associated with the Face. Therefore, for this 
 * function, conn is filled with only data for triangles and must be examined 
 * to find data associated with the Edges and Nodes touched by the Face.
 */
extern int
gem_getTessel(gemDRep *drep,            /* (in)  pointer to DRep */
              gemPair bface,            /* (in)  BRep/Face index in DRep */
              int     *nvrt,            /* (out) number of vertices */
              double  *xyz[],           /* (out) pointer to the vertices */
              double  *uv[],            /* (out) pointer to the vertex params */
              gemConn **conn);          /* (out) pointer to connectivity */


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
 * setting for adding places where interpolation is performed.
 */
extern int
gem_extendBound(gemDRep *drep,          /* (in)  pointer to DRep */
                int     ibound,         /* (in)  Bound to be extended */
                int     nIDs,           /* (in)  number of new Face IDs */
                char    *IDs[]);        /* (in)  array  of new Face IDs */


/* create an empty connected VertexSet
 *
 * Generates a new VertexSet (for connected Sets) and returns the index.
 */
extern int
gem_createVset(gemDRep *drep,           /* (in)  pointer to DRep */
               int     ibound,          /* (in)  index of Bound */
               int     *ivs);           /* (out) index of Vset in Bound */


/* empty a VertexSet
 *
 * Cleans up all data associated with a VertexSet. Not for unconnected Sets.
 */
extern int
gem_emptyVset(gemDRep *drep,            /* (in)  pointer to DRep */
              int     ibound,           /* (in)  index of Bound */
              int     ivs);             /* (in)  index of Vset in Bound */


/* add data for a single Face to a VertexSet
 *
 * Adds this discrete Face definition to the specified connected VertexSet. 
 * conn allows for specifying how the vertices are connected via groups of 
 * triangles, quadrilaterals or 2D meshes. The total number of points in the 
 * VertexSet is the sum of all of the added Faces (and will maintain that
 * order). Note: xyz or uv could be NULL but not both.
 */
extern int
gem_addVset(gemDRep *drep,              /* (in)  pointer to DRep */
            int     ibound,             /* (in)  index of Bound */
            int     ivs,                /* (in)  index of Vset in Bound */
            gemPair bface,              /* (in)  BRep/Face indices */
            int     npts,               /* (in)  number of points */
            /*@null@*/
            double  xyz[],              /* (in)  3*npts XYZ locations */
            /*@null@*/
            double  uv[],               /* (in)  2*npts UV positions */
            gemConn *conn);             /* (in)  connectivity of the data */


/* create an unconnected VertexSet
 *
 * Makes an unconnected VertexSet. This can be used for receiving 
 * interpolation data and performing evaluation. An unconnected VertexSet 
 * cannot specify a (re)parameterization (by invoking gem_paramBound), be 
 * the source of interpolation or get sensitivities.
 */
extern int
gem_makeVset(gemDRep *drep,             /* (in)  pointer to DRep */
             int     ibound,            /* (in)  index of Bound */
             int     npts,              /* (in)  number of points */
             double  xyz[],             /* (in)  3*npts XYZ positions */
             int     *ivs);             /* (out) index of Vset in Bound */

/* (re)parameterize a Bound
 *
 * Creates a global parameterization (u,v) based on a connected VertexSet. 
 * If the parameterization exists, making this call recomputes the 
 * parameterization (which may be necessary after adding another Face to the
 * VertexSet). This must be performed before any interpolation is requested. 
 * If there is only 1 Face specified in the VertexSet then the native 
 * parameterization for that Face is used for the DRep functions. DataSets 
 * named "xyz" and "uv" are produced for all of the VertexSets found in the 
 * Bound (where "uv" is the single parameterization for the Bound).
 */
extern int
gem_paramBound(gemDRep *drep,           /* (in)  DRep pointer */
               int     ibound,          /* (in)  Bound index (1-bias) */
               int     ivs);            /* (in)  the VertexSet (1-bias) */


/* put data into a Vset
 *
 * Stores the named data in a connected VertexSet. This allows for the 
 * assignment of field data onto the vertices, for example "pressure" 
 * (rank = 1) can be associated with the vertex positions. Only one 
 * VertexSet within a Bound can have a unique name, therefore it is an 
 * error to put a DataSet on a VertexSet if the name already exists 
 * (or the name is reserved, "xyz" or "uv"). If the name exists in this 
 * VertexSet it will be overwritten.
 */
extern int
gem_putData(gemDRep *drep,              /* (in)  pointer to DRep */
            int     ibound,             /* (in)  index of Bound */
            int     ivs,                /* (in)  index of Vset in Bound */
            char    name[],             /* (in)  dataset name:
                                 if name exists in ibound
                                    if name associated with ivs and rank agrees
                                       overwrite dataset & invalidate interps
                                    else
                                       GEM_BADSETNAME
                                  else
                                     create new dataset */
            int     rank,               /* (in)  # of elements per vertex */
            double  data[]);            /* (in)  rank*npts data values or NULL
                                                 deletes the Dset if exists */

/* get data from a Vset
 *
 * Returns (or computes and returns) the data found in the VertexSet. 
 * If another VertexSet in the Bound has the name, then interpolation 
 * is performed.
 * 
 * The following reserved names automatically generate (with rank):
 * d(xyz)/d(uv):        d1 (6) 		d2(xyz)/d(uv)2: 	d2 (9)
 * GeomCurvature:	curv (8)	Inside/Outside:         inside (1)
 * Parameters:          uv (2)		Sensitivity:            d/dPARAM (3)
 *		     Where PARAM is the full parameter name in the Model
 */
extern int
gem_getData(gemDRep *drep,              /* (in)  pointer to DRep */
            int     ibound,             /* (in)  index of Bound */
            int     ivs,                /* (in)  index of Vset in Bound */
            char    *name,              /* (in)  dataset name */
            int     *npts,              /* (out) number of points in Vset */
            int     *rank,              /* (out) # of elements per vertex */
            double  *data[]);           /* (out) pointer to data values */


/* get info about a DRep
 *
 * Returns current information about a DRep. BReps are found in the Model. 
 * The order is important -- many DRep functions refer to a BRep by index 
 * (bias 1). Use gem_getModel to get the number of BReps and the actual
 * BRep pointers. 
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
 * vertices and the number of DataSets. Pointers to collections of names 
 * and ranks should be freed by the programmer after they are used by 
 * invoking gem_free.
 */
extern int
gem_getVsetInfo(gemDRep *drep,          /* (in)  pointer to DRep */
                int     ibound,         /* (in)  index of Bound */
                int     ivs,            /* (in)  index of Vset in Bound */
                int     *vstype,        /* (out) 0-connected, 1-unconnected */
                int     *npnt,          /* (out) number of points in the Vset */
                int     *nset,          /* (out) number of datasets */
                char    **names[],      /* (out) ptr to Dset names (freeable) */
                int     *ranks[]);      /* (out) ptr to Dset ranks (freeable) */

