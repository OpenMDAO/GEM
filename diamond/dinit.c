/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Initialization Function -- OpenCSM & EGADS
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "egads.h"
#include "OpenCSM.h"

  /* use a single contest for all operations */
  ego dia_context = NULL;


int
gem_kernelInit()
{
  int        major, minor;
  const char *occ_rev;
  
  if (dia_context != NULL) return EGADS_EMPTY;
  
  ocsmVersion(&major, &minor);
  printf("\n GEM Info: Using OpenCSM %d.%02d\n", major, minor);
  EG_revision(&major, &minor, &occ_rev);
  printf(" GEM Info: Using EGADS   %d.%02d with %s\n\n", major, minor, occ_rev);

  return EG_open(&dia_context);
}


/*@null@*/ /*@observer@*/ const char *
gem_kernelError(int code)
{

  if ((code < 0) && (code > -100)) {

    /* EGADS Errors */
    if (code == EGADS_OCSEGFLT) return "EGADS: OpenCASCADE SEGFAULT";
    if (code == EGADS_BADSCALE) return "EGADS: Bad Scale";
    if (code == EGADS_NOTORTHO) return "EGADS: Not OrthoNormal";
    if (code == EGADS_DEGEN)    return "EGADS: Degenerate";
    if (code == EGADS_CONSTERR) return "EGADS: Construction Error";
    if (code == EGADS_TOPOERR)  return "EGADS: Topological Error";
    if (code == EGADS_GEOMERR)  return "EGADS: Geometry Error";
    if (code == EGADS_NOTBODY)  return "EGADS: Not a BODY Object";
    if (code == EGADS_WRITERR)  return "EGADS: Write Error";
    if (code == EGADS_NOTMODEL) return "EGADS: Not a MODEL Object";
    if (code == EGADS_NOLOAD)   return "EGADS: Not Loaded";
    if (code == EGADS_RANGERR)  return "EGADS: Range Error";
    if (code == EGADS_NOTGEOM)  return "EGADS: Not a GEMOETRY Object";
    if (code == EGADS_NOTTESS)  return "EGADS: Not a Tessellation Object";
    if (code == EGADS_EMPTY)    return "EGADS: Object is Empty";
    if (code == EGADS_NOTTOPO)  return "EGADS: Not a TOPOLOGICAL Object";
    if (code == EGADS_REFERCE)  return "EGADS: Reference Error";
    if (code == EGADS_NOTXFORM) return "EGADS: Not a TRANSFORM Object";
    if (code == EGADS_NOTCNTX)  return "EGADS: Not a CONTEXT Object";
    if (code == EGADS_MIXCNTX)  return "EGADS: Mixing CONTEXT Objects";
    if (code == EGADS_NODATA)   return "EGADS: No Data";
    if (code == EGADS_NONAME)   return "EGADS: No Name";
    if (code == EGADS_INDEXERR) return "EGADS: Index Error";
    if (code == EGADS_MALLOC)   return "EGADS: Memory Allocation Error";
    if (code == EGADS_NOTOBJ)   return "EGADS: Not an Object";
    if (code == EGADS_NULLOBJ)  return "EGADS: NULL Object";
    if (code == EGADS_NOTFOUND) return "EGADS: Not Found";

  } else if ((code < -200) && (code > -300)) {
    /* OpenCSM Errors */
    if (code == OCSM_FILE_NOT_FOUND)             return "OpenCSM: File Not Found";
    if (code == OCSM_ILLEGAL_STATEMENT)          return "OpenCSM: Illegal Statement";
    if (code == OCSM_NOT_ENOUGH_ARGS)            return "OpenCSM: Not Enough Arguments";
    if (code == OCSM_NAME_ALREADY_DEFINED)       return "OpenCSM: Name Already Defined";
    if (code == OCSM_PATTERNS_NESTED_TOO_DEEPLY) return "OpenCSM: Patterns Nested Too Deeply";
    if (code == OCSM_PATBEG_WITHOUT_PATEND)      return "OpenCSM: PATBEG without PATEND";
    if (code == OCSM_PATEND_WITHOUT_PATBEG)      return "OpenCSM: PATEND without PATBEG";
    if (code == OCSM_NOTHING_TO_DELETE)          return "OpenCSM: Nothing to Delete";
    if (code == OCSM_NOT_MODL_STRUCTURE)         return "OpenCSM: Not a MODL Structure";

    if (code == OCSM_DID_NOT_CREATE_BODY)        return "OpenCSM: Did Not Create BODY";
    if (code == OCSM_CREATED_TOO_MANY_BODYS)     return "OpenCSM: Created Too Many BODYs";
    if (code == OCSM_EXPECTING_ONE_BODY)         return "OpenCSM: Expecting one BODY";
    if (code == OCSM_EXPECTING_TWO_BODYS)        return "OpenCSM: Expecting two BODYs";
    if (code == OCSM_EXPECTING_ONE_SKETCH)       return "OpenCSM: Expecting one Sketch";
    if (code == OCSM_EXPECTING_NLOFT_SKETCHES)   return "OpenCSM: Expecting nLoft Sketches";
    if (code == OCSM_LOFT_WITHOUT_MARK)          return "OpenCSM: Loft without a Mark";
    if (code == OCSM_TOO_MANY_SKETCHES_IN_LOFT)  return "OpenCSM: Too Many Sketches in Loft";
    if (code == OCSM_MODL_NOT_CHECKED)           return "OpenCSM: MODL not Checked";

    if (code == OCSM_FILLET_AFTER_WRONG_TYPE)    return "OpenCSM: Fillet after Wrong Type";
    if (code == OCSM_CHAMFER_AFTER_WRONG_TYPE)   return "OpenCSM: Chamfer after Wrong Type";
    if (code == OCSM_NO_BODYS_PRODUCED)          return "OpenCSM: No BODYs Produced";
    if (code == OCSM_NOT_ENOUGH_BODYS_PRODUCED)  return "OpenCSM: Not Enough BODYs Produced";
    if (code == OCSM_TOO_MANY_BODYS_ON_STACK)    return "OpenCSM: Too Many BODYs on the Stack";

    if (code == OCSM_SKETCHER_IS_OPEN)           return "OpenCSM: Sketcher is Open";
    if (code == OCSM_SKETCHER_IS_NOT_OPEN)       return "OpenCSM: Sketcher is Not Open";
    if (code == OCSM_COLINEAR_SKETCH_POINTS)     return "OpenCSM: CoLinear Sketch Points";
    if (code == OCSM_NON_COPLANAR_SKETCH_POINTS) return "OpenCSM: Non-CoPlanar Sketch Points";
    if (code == OCSM_TOO_MANY_SKETCH_POINTS)     return "OpenCSM: Too Many Sketch Points";
    if (code == OCSM_TOO_FEW_SPLINE_POINTS)      return "OpenCSM: Too Few Spline Points";
    if (code == OCSM_SKETCH_DOES_NOT_CLOSE)      return "OpenCSM: Sketch Does Not Close";

    if (code == OCSM_ILLEGAL_CHAR_IN_EXPR)       return "OpenCSM: Illegal Char in Expression";
    if (code == OCSM_CLOSE_BEFORE_OPEN)          return "OpenCSM: Close before Open";
    if (code == OCSM_MISSING_CLOSE)              return "OpenCSM: Missing Close";
    if (code == OCSM_ILLEGAL_TOKEN_SEQUENCE)     return "OpenCSM: Illegal Token Sequence";
    if (code == OCSM_ILLEGAL_NUMBER)             return "OpenCSM: Illegal Number";
    if (code == OCSM_ILLEGAL_PMTR_NAME)          return "OpenCSM: Illegal Parameter Name";
    if (code == OCSM_ILLEGAL_FUNC_NAME)          return "OpenCSM: Illegal Function Name";
    if (code == OCSM_ILLEGAL_TYPE)               return "OpenCSM: Illegal Type";
    if (code == OCSM_ILLEGAL_NARG)               return "OpenCSM: Illegal Number of Arguments";

    if (code == OCSM_NAME_NOT_FOUND)             return "OpenCSM: Name Not Found";
    if (code == OCSM_NAME_NOT_UNIQUE)            return "OpenCSM: Name Not Unique";
    if (code == OCSM_PMTR_IS_EXTERNAL)           return "OpenCSM: Parameter is External";
    if (code == OCSM_PMTR_IS_INTERNAL)           return "OpenCSM: Parameter is Internal";
    if (code == OCSM_FUNC_ARG_OUT_OF_BOUNDS)     return "OpenCSM: Function Arg out of Bounds";
    if (code == OCSM_VAL_STACK_UNDERFLOW)        return "OpenCSM: Stack Underflow";
    if (code == OCSM_VAL_STACK_OVERFLOW)         return "OpenCSM: Stack Overflow";

    if (code == OCSM_ILLEGAL_BRCH_INDEX)         return "OpenCSM: Illegal Branch Index";
    if (code == OCSM_ILLEGAL_PMTR_INDEX)         return "OpenCSM: Illegal Parameter Index";
    if (code == OCSM_ILLEGAL_BODY_INDEX)         return "OpenCSM: Illegal BODY Index";
    if (code == OCSM_ILLEGAL_ARG_INDEX)          return "OpenCSM: Illegal Argument Index";
    if (code == OCSM_ILLEGAL_ACTIVITY)           return "OpenCSM: Illegal Activity";
    if (code == OCSM_ILLEGAL_MACRO_INDEX)        return "OpenCSM: Illegal Macro Index";
    if (code == OCSM_ILLEGAL_ARGUMENT)           return "OpenCSM: Illegal Argument";
    if (code == OCSM_CANNOT_BE_SUPPRESSED)       return "OpenCSM: Cannot Be Suppressed";
    if (code == OCSM_STORAGE_ALREADY_USED)       return "OpenCSM: Storage Already Used";
    if (code == OCSM_NOTHING_PREVIOUSLY_STORED)  return "OpenCSM: Nothing Previously Stored";

    if (code == OCSM_SOLVER_IS_OPEN)             return "OpenCSM: Solver is Open";
    if (code == OCSM_SOLVER_IS_NOT_OPEN)         return "OpenCSM: Solver is Not Open";
    if (code == OCSM_TOO_MANY_SOLVER_VARS)       return "OpenCSM: Too Many Solver Variables";
    if (code == OCSM_UNDERCONSTRAINED)           return "OpenCSM: System is Underconstrained";
    if (code == OCSM_OVERCONSTRAINED)            return "OpenCSM: System is Overconstrained";
    if (code == OCSM_SINGULAR_MATRIX)            return "OpenCSM: Singular Matrix";
    if (code == OCSM_NOT_CONVERGED)              return "OpenCSM: Not Converged";

    if (code == OCSM_UDP_ERROR1)                 return "OpenCSM: UDP Error 1";
    if (code == OCSM_UDP_ERROR2)                 return "OpenCSM: UDP Error 2";
    if (code == OCSM_UDP_ERROR3)                 return "OpenCSM: UDP Error 3";
    if (code == OCSM_UDP_ERROR4)                 return "OpenCSM: UDP Error 4";
    if (code == OCSM_UDP_ERROR5)                 return "OpenCSM: UDP Error 5";
    if (code == OCSM_UDP_ERROR6)                 return "OpenCSM: UDP Error 6";
    if (code == OCSM_UDP_ERROR7)                 return "OpenCSM: UDP Error 7";
    if (code == OCSM_UDP_ERROR8)                 return "OpenCSM: UDP Error 8";
    if (code == OCSM_UDP_ERROR9)                 return "OpenCSM: UDP Error 9";

    if (code == OCSM_OP_STACK_UNDERFLOW)         return "OpenCSM: OP Stack Underflow";
    if (code == OCSM_OP_STACK_OVERFLOW)          return "OpenCSM: OP Stack Overflow";
    if (code == OCSM_RPN_STACK_UNDERFLOW)        return "OpenCSM: RPN Stack Underflow";
    if (code == OCSM_RPN_STACK_OVERFLOW)         return "OpenCSM: RPN Stack Overflow";
    if (code == OCSM_TOKEN_STACK_UNDERFLOW)      return "OpenCSM: Token Stack Underflow";
    if (code == OCSM_TOKEN_STACK_OVERFLOW)       return "OpenCSM: Token Stack Overflow";
    if (code == OCSM_UNSUPPORTED)                return "OpenCSM: Unsupported";
    if (code == OCSM_INTERNAL_ERROR)             return "OpenCSM: Internal Error";
  }

  return NULL;
}
