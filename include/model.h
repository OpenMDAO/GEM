/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Model Include
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */


/* Spline Point Types -- can be or'ed  */

#define GEM_POSITION		1	/* Can modify -- always exists */
#define GEM_TANGENT		2
#define GEM_RADCURV		4


/* Suppression Flags */

#define GEM_READONLY		-1
#define GEM_ACTIVE               0
#define GEM_SUPPRESSED		 1
#define GEM_INACTIVE             2


/* spline point structure */

typedef struct {
  int    pntype;		/* point type bit flag*/
  double pos[3];		/* point position */
  double tan[3];		/* point tangent */
  double radc[3];		/* point radius of curvature */
} gemSpl;


/* GEM feature-tree node */

typedef struct {
  char     *name;		/* Feature Name */
  gemID     handle;             /* internal entity */
  short     sflag;		/* GEM Suppression flag */
  short     changed;		/* changed flag */
  char     *branchType;		/* internal type or ASSEMBLY, INSTANCE, PART */
  int       nParents;
  union {
    int  pnode;			/* single node index -- nParents == 1 */
    int *pnodes;		/* multiple node indices */
  } parents;
  int       nChildren;		/* Number of children */
  union {
    int  node;			/* single node index -- len == 1 */
    int *nodes;			/* multiple node indices */
  } children;
  gemAttrs *attr;		/* attribute structure */
} gemFeat;


/* GEM parameter */

typedef struct {
  char    *name;		/* Parameter Name */
  int      type;		/* GEM Parameter type */
  int      order;		/* order of spline */
  gemID    handle;              /* internal entity */
  short    changed;		/* changed flag */
  short    bitflag;		/* 1 = read-only, 2 = fixed len, 4 = 3D 
                                   8 = bounds exist */
  int      len;			/* Number of values or length of string */
  union {
    int    bool1;		/* single bool -- len == 1 */
    int    *bools;		/* multiple bools */
    int    integer;		/* single int -- len == 1 */
    int    *integers;		/* multiple ints */
    double real;		/* single real -- len == 1 */
    double *reals;		/* mutiple reals */
    char   *string;		/* character string (no single char) */
    gemSpl *splpnts;		/* spline points (no single spline point) */
  } vals;
  union {
    int    ilims[2];            /* integer bounds */
    double rlims[2];            /* real bounds */
  } bnds;
  gemAttrs *attr;		/* attribute structure */
} gemParam;


/* GEM model structure */

typedef struct gemModel {
  int      magic;               /* magic number to check for authenticity */
  gemID    handle;              /* internal entity */
  int      nonparam;            /* static model -- only geometry */
  char     *server;             /* server URL */
  char     *location;           /* location of model on disk */
  char     *modeler;            /* CAD system / Kernel */
  int      nBRep;		/* number of BReps */
  gemBRep  **BReps;		/* pointer to list of BRep objs */
  int      nParams;		/* number of Parameters */
  gemParam *Params;		/* the Parameters */
  int      nBranches;		/* number of Tree nodes */
  gemFeat  *Branches;		/* Feature Tree branches */
  gemAttrs *attr;		/* attribute structure */
  struct gemModel *prev;        /* previous model */
  struct gemModel *next;        /* next model */
} gemModel;


/* add a BRep to a non-parametric model
 *
 * Adds the specified BRep to the static Model. If xform is not NULL, then the 
 * BRep is transformed to the new position/orientation defined by the matrix.
 */
extern int
gem_add2Model(gemModel *model,          /* (in)  the model to add the BRep */
              gemBRep  *BRep,           /* (in)  pointer to added BRep */
              /*@null@*/
              double   xform[]);        /* (in)  transformation mat or NULL */

           
/* save a model
 *
 * Saves an uptodate Model. Only for non-static Models. 
 */
extern int
gem_saveModel(gemModel *model,          /* (in)  the model to save */
              /*@null@*/
              char     location[]);     /* (in)  the path to save the model */


/* release a model
 * 
 * This releases the Model and any associated data (including DReps). 
 * This also frees the structure. 
 */
extern int
gem_releaseModel(/*@only@*/ 
                 gemModel *model);	/* (in)  the model to release */


/* copy a model
 *
 * Copies the Model. This is useful for situations where it may be necessary 
 * to maintain the original specification during regenerations.
 */
extern int
gem_copyModel(gemModel *model,          /* (in)  the model to copy */
              gemModel **newmdl);       /* (out) pointer to copied model */


/* regenerate a model
 *
 * Regenerates a Model. Only for non-static Models. Invalidates any VertexSets, 
 * and tessellations associated with the Model.
 */
extern int
gem_regenModel(gemModel *model);        /* (in)  the model to regenerate */


/* get info about a model
 *
 * Returns the data associated with a Model. Use gem_getParam to get detailed 
 * information about a particular Parameter. Use gem_getBranch to get the 
 * information about a FeatureNode. uptodate returns -1 for a static Model.
 */
extern int
gem_getModel(gemModel *model,           /* (in)  pointer to Model */
             char     *server[],        /* (out) pointer to URL (or NULL) */
             char     *filnam[],        /* (out) location on disk (or NULL) */
             char     *modeler[],       /* (out) modeler string */
             int      *uptodate,        /* (out) -1 static, 0 regen, 1 OK */
             int      *nBRep,           /* (out) number of BReps */
             gemBRep  **BReps[],        /* (out) pointer to list of BReps */
             int      *nParams,         /* (out) number of parameters */
             int      *nBranch,         /* (out) number of branches */
             int      *nattr);          /* (out) number of attributes */


/* get data about a branch
 *
 * Returns the data associated with the specified (by index) FeatureNode in the 
 * Model. Note: the branch type is CAD specific unless it is "ASSEMBLY", "PART"
 * or "INSTANCE" when it refers to the Assembly hierarchy. An Instance will
 * have only a single child which references either a Part or an Assembly.
 */
extern int
gem_getBranch(gemModel *model,          /* (in)  pointer to Model */
              int      branch,          /* (in)  branch index */
              char     *bname[],        /* (out) pointer to branch name */
              char     *btype[],        /* (out) pointer to branch type */
              int      *suppress,       /* (out) suppression state */
              int      *nparent,        /* (out) number of parents */
              int      *parents[],      /* (out) pointer to the parents */
              int      *nchild,         /* (out) number of children */
              int      *children[],     /* (out) pointer to the children */
              int      *nattr);         /* (out) number of attributes */


/* set suppression state
 *
 * Changes the suppression state for the branch (for suppressible 
 * FeatureNodes). A call to gem_regenModel is required to apply the change.
 */
extern int
gem_setSuppress(gemModel *model,        /* (in)  pointer to Model */
                int      branch,        /* (in)  branch index */
                int      suppress);     /* (in)  new suppression state */


/* get data about a parameter
 *
 * Returns the data associated with the specified Parameter in the Model. 
 * Only the pointer of the appropriate type will be filled with a non-NULL 
 * value.
 */
extern int
gem_getParam(gemModel *model,           /* (in)  pointer to Model */
             int      param,            /* (in)  parameter index */
             char     *pname[],         /* (out) pointer to parameter name */
             int      *bflag,           /* (out) or: 1-fixed, 2-driven, 4-3D 
                                                     8-bounds */
             int      *order,           /* (out) order for splines */
             int      *ptype,           /* (out) parameter type */
             int      *plen,            /* (out) number of entries */
                                        /*       one of: */
             int      *integers[],      /* (out) pointer to integers/bools */
             double   *reals[],         /* (out) pointer to doubles */
             char     *string[],        /* (out) pointer to string */
             gemSpl   *spline[],        /* (out) pointer to spline data */
             int      *nattr);          /* (out) number of attributes */


/* get limits for a parameter
 *
 * Returns the bounds associated with the specified Parameter in the Model.
 * Valid only for Integer and Real Parameters with the "bounds" bit set. 
 * Only the vector of the appropriate type will be filled.
 */
extern int
gem_getLimits(gemModel *model,          /* (in)  pointer to Model */
              int      param,           /* (in)  parameter index */
              int      intlims[2],      /* (out) integer limits */
              double   realims[2]);     /* (out) double limits */


/* set data for a parameter
 *
 * Sets new value(s) for driving (not driven) Parameters. The number of entries 
 * can be changed only for non-fixed Parameters. Only the pointer of the 
 * appropriate type is used. An invocation of gem_regenModel is required to 
 * apply the change (until that point the Model is considered not uptodate).
 */
extern int
gem_setParam(gemModel *model,           /* (in)  pointer to Model */
             int      param,            /* (in)  parameter index */
             int      plen,             /* (in)  number of entries */
                                        /*       provide the appropriate one: */
             /*@null@*/
             int      integers[],       /* (in)  integers/bools */
             /*@null@*/
             double   reals[],          /* (in)  doubles */
             /*@null@*/
             char     string[],         /* (in)  string */
             /*@null@*/
             gemSpl   spline[]);        /* (in)  spline data */


