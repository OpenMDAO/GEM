/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Top-level GEM Include
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#ifndef _GEM_H_
#define _GEM_H_


/* Magic Numbers */

#define GEM_MCONTEXT     66666
#define GEM_MMODEL       77777
#define GEM_MBREP        88888
#define GEM_MDREP        99999


/* Error Codes */

#define GEM_OUTSIDE          1
#define GEM_SUCCESS          0
#define GEM_BADDREP       -301
#define GEM_BADFACEID     -302
#define GEM_BADBOUNDINDEX -303
#define GEM_BADVSETINDEX  -304
#define GEM_BADRANK       -305
#define GEM_BADDSETNAME   -306
#define GEM_MISSDISPLACE  -307
#define GEM_NOTFOUND      -308
#define GEM_BADMODEL      -309
#define GEM_BADCONTEXT    -310
#define GEM_BADBREP       -311
#define GEM_BADINDEX      -312
#define GEM_NOTCHANGED    -313
#define GEM_ALLOC         -314
#define GEM_BADTYPE       -315
#define GEM_NULLVALUE     -316
#define GEM_NULLNAME      -317
#define GEM_NULLOBJ       -318
#define GEM_BADOBJECT     -319
#define GEM_WIREBODY      -320
#define GEM_SOLIDBODY     -321
#define GEM_NOTESSEL      -322
#define GEM_BADVALUE      -323
#define GEM_DUPLICATE     -324
#define GEM_BADINVERSE    -325
#define GEM_NOTPARAMBND   -326
#define GEM_NOTCONNECT    -327
#define GEM_NOTPARMTRIC   -328
#define GEM_READONLYERR   -329
#define GEM_FIXEDLEN      -330
#define GEM_ASSEMBLY      -331
#define GEM_BADNAME       -332
#define GEM_UNSUPPORTED   -333
#define GEM_BADMETHOD     -334


/* Topologcal/Attribute Entity Types */

#define GEM_NODE		 0
#define GEM_EDGE		 1
#define GEM_LOOP		 2
#define GEM_FACE		 3
#define GEM_SHELL		 4
#define GEM_BREP		 5

#define GEM_PARAM		10
#define GEM_BRANCH		11
#define GEM_MODEL               12

#define GEM_CONTEXT             20
#define GEM_DREP                21


/* Parameter/Attribute Types */

#define GEM_BOOL		 0
#define GEM_INTEGER		 1
#define GEM_REAL		 2
#define GEM_STRING		 3
#define GEM_SPLINE 		 4


/* Solid Boolean Operators */

#define GEM_SUBTRACT	1
#define GEM_INTERSECT	2
#define GEM_UNION	3



  typedef struct {
    int  index;                 /* internal index of entity -- or -- */
    union {
      /*@shared@*/
      void *ptr;		/* internal pointer to entity */
      int  tag;                 /* another integer */
    } ident;
  } gemID;


  typedef struct {
    char   *name;		/* Attribute Name */
    int    length;		/* number of values */
    int    type;		/* attribute type - one of: */
    int    *integers;
    double *reals;
    char   *string;
  } gemAttr;


  typedef struct {
    int     nattrs;		/* number of attributes */
    int     mattrs;             /* number of allocated attributes */
    gemAttr *attrs;		/* the attributes */
  } gemAttrs;


#include "brep.h"
#include "model.h"
#include "drep.h"


  typedef struct {
    int      magic;             /* magic number to check for authenticity */
    gemModel *model;            /* starting model */
    gemDRep  *drep;             /* starting DRep */
    gemAttrs *attr;             /* the context attributes */
  } gemCntxt;



/* Initialize GEM
 *
 * Returns a Context to be used by other GEM functions that load or 
 * otherwise create other objects.
 */
extern int 
gem_initialize(gemCntxt **context);     /* (out) pointer to a context */


/* Terminate GEM
 *
 * Destroys the Context (frees all structures) and removes all objects 
 * created within.
 */
extern int
gem_terminate(gemCntxt *context);       /* (in)  the context */


/* make an empty (static) non-parametric model
 *
 * Returns an empty static (non-parametric) Model in the specified 
 * Context. Use gem_add2Model to populate the Model from existing BReps.
 */
extern int
gem_staticModel(gemCntxt *cntxt,        /* (in)  context */
                gemModel **model);      /* (out) pointer to created model */


/* load a model
 *
 * Returns a Model loaded from file and places it in the specified 
 * Context.
 */
extern int
gem_loadModel(gemCntxt *cntxt,          /* (in)  context */
              /*@null@*/
              char     server[],        /* (in)  server location */
              char     location[],      /* (in)  model file-name */
              gemModel **model);        /* (out) pointer to new model */


/* get BRep owner info
 *
 * Returns the Model containing the BRep and the location of the BRep in the 
 * hierarchical Assembly (if one exists).
 */
extern int
gem_getBRepOwner(gemBRep  *brep,        /* (in)  BRep pointer */
                 gemModel **model,      /* (out) owning Model pointer */
                 int      *instance,    /* (out) instance number */
                 int      *branch);     /* (out) instance branch in model */


/* Perform a Solid Boolean Operation
 *
 * Executes the appropriate Solid Boolean operation on the two SolidBody 
 * BReps (the source and the tool). The tool BRep can optionally be moved 
 * with the transformation matrix before the operation. Neither BRep is 
 * destroyed and the result is a new static model. A model is returned 
 * because both the intersection and subtraction operations can result 
 * in multiple SolidBodies. Note that attributes are passed through the 
 * operation (in particular, all Faces are marked with their parent Face 
 * attributes).
 */
extern int
gem_solidBoolean(gemBRep  *breps,       /* (in)  BRep pointer (source) */
                 gemBRep  *brept,       /* (in)  BRep pointer (tool) */
                 /*@null@*/
                 double   xform[],      /* (in)  transform matrix or NULL */
                 int      sboType,      /* (in)  0 - subtract, 1 - intersect,
                                                 2 - union */
                 gemModel **model);     /* (out) the resultant model */
            

/* get an Attribute by index
 *
 * Returns the Attribute, specified by index, associated with the 
 * specific entity and entity index. Only the pointer of the appropriate 
 * type will be filled with a non-NULL value. gemObj may be gemCntxt, 
 * gemModel, gemBRep or gemDRep. 
 */
extern int
gem_getAttribute(void   *gemObj,        /* (in)  GEM Object pointer */
                 int    etype,		/* (in)  Entity type (Model/BRep) */
                 int    eindex,		/* (in)  Entity index (Model/BRep) */
                 int    aindex,		/* (in)  Attribute index */
                 char   *name[],        /* (out) pointer to attribute name */
                 int    *atype,		/* (out) Atrribute type */
                 int    *alen,		/* (out) Attribute length */
                                        /*       one of: */
                 int    *integers[],	/* (out) pointer to integers/bools */
                 double *reals[],	/* (out) pointer to doubles */
                 char   *string[]);	/* (out) pointer to string */


/* get an Attribute by name
 *
 * Retrieves the Attribute, specified by name, associated with the 
 * specific entity and it's index. Only the pointer of the appropriate 
 * type will be filled with a non-NULL value. Returns an error indication 
 * if the Attribute is not found. gemObj may be gemCntxt, gemModel, 
 * gemBRep or gemDRep. 
 */
extern int
gem_retAttribute(void   *gemObj,	/* (in)  GEM Object pointer */
                 int    etype,		/* (in)  Entity type (Model/BRep) */
                 int    eindex,		/* (in)  Entity index (Model/BRep) */
                 char   name[],		/* (in)  pointer to attribute name */
                 int    *aindex,	/* (out) Attribute index */
                 int    *atype,		/* (out) Atrribute type */
                 int    *alen,		/* (out) Attribute length */
                                        /*       one of: */
                 int    *integers[],	/* (out) pointer to integers/bools */
                 double *reals[],	/* (out) pointer to doubles */
                 char   *string[]);	/* (out) pointer to string */


/* set an Attribute
 *
 * Modifies an existing Attribute, creates a new Attribute or deletes 
 * an existing Attribute. If the name exists on the entity then the
 * Attribute is modified (unless the length is zero where it is 
 * deleted). If the name is not found on the entities list then the 
 * Attribute is added. Only the pointer of the appropriate type will 
 * be used. gemObj may be gemCntxt, gemModel, gemBRep or gemDRep. 
 */
extern int
gem_setAttribute(void   *gemObj,	/* (in)  GEM Object pointer */
                 int    etype,		/* (in)  Entity type (Model/BRep) */
                 int    eindex,		/* (in)  Entity index (Model/BRep) */
                 char   name[],		/* (in)  pointer to attribute name */
                 int    atype,		/* (in)  Atrribute type */
                 int    alen,		/* (in)  Attribute length */
                                        /*       provide the appropriate one: */
                /*@null@*/
                 int    integers[],	/* (in)  integers/bools */
                /*@null@*/
                 double reals[],        /* (in)  doubles */
                /*@null@*/
                 char   string[]);	/* (in)  string */


/* return info about a GEM Object */
extern int
gem_getObject(void *gemObj,             /* (in)  GEM Object pointer */
              int  *otype,              /* (out) object type */
              int  *nattr);             /* (out) number of attributes */


/* return a string based on the return code */
extern /*@observer@*/ const char *
gem_errorString(int code);		/* (in)  return code from GEM function */


/* free allocated GEM memory */
extern void
gem_free(/*@null@*/ 
         /*@only@*/ void *ptr);         /* (in)  the pointer to be freed */



#endif  /* _GEM_H_ */
