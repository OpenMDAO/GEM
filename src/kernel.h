/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Geometry Kernel Functions Include
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */


/* initializes the kernel */
extern int
gem_kernelInit(void);

/* terminates the kernel */
extern int
gem_kernelClose(void);

/* loads a model */
extern int
gem_kernelLoad(gemCntxt *context, /*@null@*/ char *server, char *location, 
               gemModel **model);
               
/* saves a model */
extern int
gem_kernelSave(gemModel *model, /*@null@*/ char *filename);

/* copies a BRep */
extern int
gem_kernelCopy(gemBRep *brep, /*@null@*/ double *xform, gemBRep **newBrep);

/* copies the MasterModel part of a model */
extern int
gem_kernelCopyMM(gemModel *model);

/* deletes a BRep */
extern int
gem_kernelDelete(gemID handle);

/* regenerates a model */
extern int
gem_kernelRegen(gemModel *model);

/* releases the model */
extern int 
gem_kernelRelease(gemModel *model);

/* sets an attribute on topology */
extern int 
gem_kernelBRepAttr(gemID handle, int etype, char *name, int atype, int alen, 
                   /*@null@*/ int *integers, /*@null@*/ double *reals,
                   /*@null@*/ char *string);

/* sets an attribute on a branch */
extern int 
gem_kernelBranchAttr(gemID handle, int branch, char *name, int atype, int alen,
                     /*@null@*/ int *integers, /*@null@*/ double *reals,
                     /*@null@*/ char *string);
                     
/* get mass properties */
extern int
gem_kernelMassProps(gemID handle, int etype, double *props);

/* test for equivalency */
extern int
gem_kernelEquivalent(int etype, gemID handle1, gemID handle2);

/* Solid Boolean Operation */
extern int
gem_kernelSBO(gemID src, gemID tool, /*@null@*/ double *xfrom, int type,
              gemModel **model);
              
/* tessellate a BRep */
extern int
gem_kernelTessel(gemBody *body, double angle, double mxside, double sag,
                 gemDRep *drep, int brep);

/* evaluation */
extern int
gem_kernelEval(gemDRep *drep, int bound, int vs, int gflg, int inv,
               double **eval);

/* get derivatives */
extern int
gem_kernelEvalDs(gemDRep *drep, int bound, int vs, double *d1, double *d2);

/* get curvatures */
extern int
gem_kernelCurvature(gemDRep *drep, int bound, int vs, double *data);

/* get In/Out */
extern int
gem_kernelInside(gemDRep *drep, int bound, int vs, double *data);

/* get error string */
extern /*@null@*/ /*@observer@*/ const char *
gem_kernelError(int code);
