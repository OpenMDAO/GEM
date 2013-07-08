/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             BRep Include
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */


/* Body Types */

#define GEM_SOLID       0
#define GEM_SHEET       1
#define GEM_WIRE        2


  typedef struct {
    gemID    handle;		/* internal entity */
    double   xyz[3];		/* coordinates for point */
    gemAttrs *attr;		/* attribute structure */
  } gemNode;


  typedef struct {
    gemID    handle;		/* internal entity */
    double   tlimit[2];		/* t range */
    int      nodes[2];		/* bounding nodes */
    int      faces[2];		/* bounding faces (0 neg, 1 pos sense) */
    gemAttrs *attr;		/* attribute structure */
  } gemEdge;


  typedef struct {
    gemID    handle;		/* internal entity */
    int      type;              /* 0 outer, 1 inner */
    int      face;              /* owning face */
    int      nedges;		/* number of edges in loop */
    int      *edges;		/* list of edges (- is opposite sense) */
    gemAttrs *attr;		/* attribute structure */
  } gemLoop;
  

  typedef struct {
    gemID    handle;		/* internal entity */
    double   uvbox[4];		/* u&v ranges */
    int      norm;		/* flip normal flag (-1 or 1) */
    int      nloops;		/* number of closed loops */
    int      *loops;		/* the loops indices */
    char     *ID;		/* persistent ID */
    gemAttrs *attr;		/* attribute structure */
  } gemFace;
  
  
  typedef struct {
    gemID    handle;		/* internal entity */
    int      type;              /* 0 outer, 1 inner */
    int      nfaces;		/* number of faces touched */
    int      *faces;		/* face indices */
    gemAttrs *attr;		/* attribute structure */
  } gemShell;


  typedef struct {
    gemID    handle;		/* internal entity */
    int      type;		/* 0-solid-body, 1-sheet-body, 2-wire-body */
    double   box[6];            /* min and max XYZ to define a box */
    int      nnode;		/* number of associated nodes */
    gemNode  *nodes;		/* associated nodes */
    int      nedge;		/* number of associated edges */
    gemEdge  *edges;		/* associated edges */
    int      nloop;             /* number of loops loops */
    gemLoop  *loops;            /* the accociated loops */
    int      nface;		/* number of associated faces */
    gemFace  *faces;		/* associated faces */
    int      nshell;		/* number of associated shells */
    gemShell *shells;		/* associated shells */
    gemAttrs *attr;             /* attribute structure */
  } gemBody;


  typedef struct {
    int      magic;             /* magic number to check for authenticity */
    /*@shared@*/
    void     *omodel;           /* owning model - gemModel not yet defined */
    gemID    phandle;           /* parent model handle */
    int      ibranch;           /* instance branch (if any) */
    int      inumber;           /* instance number (0 is body owner) */
    double   xform[12];		/* transformation matrix */
    double   invXform[12];	/* inverse transformation matrix */
    gemBody  *body;             /* body pointer */
  } gemBRep;
  

/* get info on a BRep
 *
 * Returns the data associated with a BRep. Use the appropriate function 
 * to get the data about a specific entity (that follows this description).
 */
extern int
gem_getBRepInfo(gemBRep *brep,          /* (in)  BRep pointer */
                double  box[],          /* (out) xyz bounding box (6) */
                int     *type,          /* (out) body type */
                int     *nnode,         /* (out) number of nodes */
                int     *nedge,         /* (out) number of edges */
                int     *nloop,         /* (out) number of loops */
                int     *nface,         /* (out) number of faces */
                int     *nshell,        /* (out) number of shells */
                int     *nattr);        /* (out) number of attributes */


/* get data about a shell
 *
 * Returns the data associated with a Shell within the BRep. Use 
 * gem_getFace to get the data about a specific Face returned in the 
 * list. Not valid for Wire Bodies.
 */
extern int
gem_getShell(gemBRep *brep,             /* (in)  BRep pointer */
             int     shell,             /* (in)  shell index */
             int     *type,             /* (out) 0 outer, 1 inner */
             int     *nface,            /* (out) number of faces */
             int     *faces[],          /* (out) pointer to list of faces */
             int     *nattr);           /* (out) number of attributes */


/* get data about a face
 *
 * Returns the data associated with a Face within the BRep. Use 
 * gem_getLoop to get the data about a specific Loop returned in the 
 * list. ID remains unchanged across regenerations. If the outward 
 * pointing normal is opposite of UxV for the underlying Surface then 
 * sense = -1. For Sheet Bodies the sense are consistent but cannot 
 * point either in or out. Not valid for Wire Bodies.
 */
extern int
gem_getFace(gemBRep *brep,              /* (in)  BRep pointer */
            int     face,               /* (in)  face index */
            char    *ID[],              /* (out) pointer to persistent ID */
            double  uvbox[],            /* (out) uv bounding box (4) */
            int     *norm,		/* (out) flip normal flag (-1 or 1) */
            int     *nloops,		/* (out) number of loops */
            int     *loops[],           /* (out) loop indices */
            int     *nattr);            /* (out) number of attributes */


/* get data about a wire-body
 *
 * Returns the data associated with a Wire Body. Use gem_getLoop to 
 * get the data about a specific Loop returned in the list. Not valid 
 * for Solid and/or Sheet Bodies.
 */
extern int
gem_getWire(gemBRep *brep,              /* (in)  BRep pointer */
            int     *nloops,		/* (out) number of loops */
            int     *loops[]);          /* (out) loop indices */


/* get data about a loop
 *
 * Returns the data associated with a Loop within a BRep. Use gem_getEdge 
 * to get the data about a specific Edge returned in the list. Note that 
 * edges can contain members that are negated. In this case the sense of 
 * the Edge is opposite (goes from tmax to tmin). In the case of a Wire 
 * Body, face is returned with 0.
 */
extern int
gem_getLoop(gemBRep *brep,              /* (in)  BRep pointer */
            int     loop,               /* (in)  loop index */
            int     *face,              /* (out) owning face index */
            int     *type,              /* (out) 0 outer, 1 inner */
            int     *nedge,		/* (out) number of edges in loop */
            int     *edges[],		/* (out) pointer to edges/senses */
            int     *nattr);            /* (out) number of attributes */


/* get data about an edge
 *
 * Returns the data associated with an Edge within a BRep. Use gem_getNode 
 * to get the data about a specific Node returned in the nodes list. The 
 * first Node is associated with the first value found in tlimit (tmin), 
 * the second Node matches the second entry in tlimit (tmax). Use 
 * gem_getFace to get the data about a specific Face returned in the faces 
 * list. The first is the Face that has this Edge found with the negative 
 * sense in it's Loop, the second Face refers to the Face where the Edge 
 * is used in the positive sense (zero for Wire Bodies). If the Edge index 
 * is negative then tlimit, nodes and faces are reversed.
 */
extern int
gem_getEdge(gemBRep *brep,              /* (in)  BRep pointer */
            int     edge,               /* (in)  edge index */
            double  tlimit[],           /* (out) t range (2) */
            int     nodes[],            /* (out) bounding node indices (2) */
            int     faces[],            /* (out) trimmed faces (2) */
            int     *nattr);            /* (out) number of attributes */


/* get data about a node
 *
 * Returns the data associated with a Node within a BRep.
 */
extern int
gem_getNode(gemBRep *brep,		/* (in)  BRep pointer */
            int     node,		/* (in)  node index */
            double  xyz[],		/* (out) coordinates (3) */
            int     *nattr);		/* (out) number of attributes */


/* get mass properties
 *
 * Returns the Mass properties: volume, surface area, CoG and inertial 
 * mass matrix at CoG.
 */
extern int
gem_getMassProps(gemBRep *brep,         /* (in)  BRep pointer */
                 int     etype,         /* (in)  Topo: Face, Shell or Body */
                 int     eindex,        /* (in)  Topological entity index */
                 double  props[]);      /* (out) the data returned (must be 
                                                 declared to at least 14):
                                                   volume, surface area
                                                   center of gravity (3)
                                                   inertia matrix at CoG (9) */


/* topological equivalency
 *
 * Asks the question: is this entity the same in one BRep as found in 
 * another. Because Solid Bodies are manifold, then non-manifold Bodies 
 * (within the same Model) may share entities (for example a wake sheet 
 * and the OML of a wing may share an Edge). This will only work is the 
 * BReps come from the same Model.
 */
extern int
gem_isEquivalent(int     etype,         /* (in)  Topological entity type */
                 gemBRep *brep1,        /* (in)  BRep pointer */
                 int     eindex1,       /* (in)  Topological entity index */
                 gemBRep *brep2,        /* (in)  BRep pointer */
                 int     eindex2);      /* (in)  Topological entity index */

