/*$Id: signal.c,v 1.80 2001/04/23 15:47:54 bsmith Exp $*/
/*
      Routines to handle signals the program will receive. 
    Usually this will call the error handlers.
*/
#include <signal.h>
#include "petsc.h"             /*I   "petsc.h"   I*/
#include "petscsys.h"
#include "petscfix.h"     

struct SH {
  int    cookie;
  int    (*handler)(int,void *);
  void   *ctx;
  struct SH* previous;
};
static struct SH* sh        = 0;
static PetscTruth SignalSet = PETSC_FALSE;

static char *SIGNAME[] = { 
    "Unknown signal",
    "HUP",
    "INT",
    "QUIT",
    "ILL",
    "TRAP",
    "ABRT",
    "EMT", 
    "FPE:\nPETSC ERROR: Floating Point Exception,probably divide by zero",
    "KILL",
    "BUS: \nPETSC ERROR: Bus Error, possibly illegal memory access", 
    "SEGV:\nPETSC ERROR: Segmentation Violation, probably memory access out of range",
    "SYS",
    "PIPE",
    "ALRM",
    "TERM",
    "URG",
    "STOP",
    "TSTP",
    "CONT",
    "CHLD" }; 


EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PetscSignalHandler_Private"
/*
    PetscSignalHandler_Private - This is the signal handler called by the system. This calls 
             any signal handler set by PETSc or the application code.
 
   Input Parameters: (depends on system)
.    sig - integer code indicating the type of signal
.    code - ??
.    sigcontext - ??
.    addr - ??

    Note: this is declared extern "C" because it is passed to the system routine signal()
          which is an extern "C" routine. The Solaris 2.7 OS compilers require that this be
          extern "C".

*/
#if defined(PETSC_HAVE_4ARG_SIGNAL_HANDLER)
static void PetscSignalHandler_Private(int sig,int code,struct sigcontext * scp,char *addr)
#else
static void PetscSignalHandler_Private(int sig)
#endif
{
  int ierr;

  PetscFunctionBegin;
  if (!sh || !sh->handler) {
    ierr = PetscDefaultSignalHandler(sig,(void*)0);
  } else{
    ierr = (*sh->handler)(sig,sh->ctx);
  }
  if (ierr) MPI_Abort(PETSC_COMM_WORLD,0);
}
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "PetscDefaultSignalHandler"
/*@
   PetscDefaultSignalHandler - Default signal handler.

   Not Collective

   Level: advanced

   Input Parameters:
+  sig - signal value
-  ptr - unused pointer

   Concepts: signal handler^default

@*/
int PetscDefaultSignalHandler(int sig,void *ptr)
{
  int         ierr;
  static char buf[1024];

  PetscFunctionBegin;
  signal(sig,SIG_DFL);
  if (sig >= 0 && sig <= 20) {
    sprintf(buf,"Caught signal %s\n",SIGNAME[sig]);
  } else {
    ierr = PetscStrcpy(buf,"Caught signal\n");CHKERRQ(ierr);
  }
  ierr = PetscStrcat(buf,"PETSC ERROR: Try option -start_in_debugger or ");CHKERRQ(ierr);
  ierr = PetscStrcat(buf,"-on_error_attach_debugger ");CHKERRQ(ierr);
  ierr = PetscStrcat(buf,"to\nPETSC ERROR: determine where problem occurs\n");CHKERRQ(ierr);
#if defined(PETSC_USE_STACK)
  if (!PetscStackActive) {
    ierr = PetscStrcat(buf,"PETSC ERROR: or try option -log_stack\n");CHKERRQ(ierr);
  } else {
    PetscStackPop;  /* remove stack frames for error handlers */
    PetscStackPop;
    ierr = PetscStrcat(buf,"PETSC ERROR: likely location of problem given above in stack\n");CHKERRQ(ierr);
    (*PetscErrorPrintf)("--------------- Stack Frames ---------------\n");
    PetscStackView(PETSC_VIEWER_STDOUT_WORLD);
    (*PetscErrorPrintf)("--------------------------------------------\n");
  }
#endif
#if !defined(PETSC_USE_BOPT_g)
  ierr = PetscStrcat(buf,"PETSC ERROR: compile, link, and run with BOPT=g or g_c++ or g_complex\n");CHKERRQ(ierr);
  ierr = PetscStrcat(buf,"PETSC ERROR: to get more information on the crash.\n");CHKERRQ(ierr);
#endif
  ierr =  PetscError(0,"User provided function","Unknown file","Unknown directory",PETSC_ERR_SIG,1,buf);
  MPI_Abort(PETSC_COMM_WORLD,ierr);
  PetscFunctionReturn(0);
}

#if !defined(PETSC_SIGNAL_CAST)
#define PETSC_SIGNAL_CAST
#endif

#undef __FUNCT__  
#define __FUNCT__ "PetscPushSignalHandler"
/*@C
   PetscPushSignalHandler - Catches the usual fatal errors and 
   calls a user-provided routine.

   Not Collective

    Input Parameter:
+  routine - routine to call when a signal is received
-  ctx - optional context needed by the routine

  Level: developer

   Concepts: signal handler^setting

@*/
int PetscPushSignalHandler(int (*routine)(int,void*),void* ctx)
{
  struct  SH *newsh;
  int        ierr;

  PetscFunctionBegin;
  if (!SignalSet && routine) {
    signal(SIGILL,  PETSC_SIGNAL_CAST PetscSignalHandler_Private);
    signal(SIGFPE,  PETSC_SIGNAL_CAST PetscSignalHandler_Private);
    signal(SIGSEGV, PETSC_SIGNAL_CAST PetscSignalHandler_Private);
#if !defined(PETSC_MISSING_SIGSYS)
    signal(SIGSYS,  PETSC_SIGNAL_CAST PetscSignalHandler_Private);
#endif
#if !defined(PETSC_MISSING_SIGBUS)
    signal(SIGBUS, PETSC_SIGNAL_CAST PetscSignalHandler_Private);
#endif
#if !defined(PETSC_MISSING_SIGQUIT)
    signal(SIGQUIT, PETSC_SIGNAL_CAST PetscSignalHandler_Private);
#endif
    SignalSet = PETSC_TRUE;
  }
  if (!routine) {
    signal(SIGILL,  0);
    signal(SIGFPE,  0);
    signal(SIGSEGV, 0);
#if !defined(PETSC_MISSING_SIGSYS)
    signal(SIGSYS,  0);
#endif
#if !defined(PETSC_MISSING_SIGBUS)
    signal(SIGBUS,  0);
#endif
#if !defined(PETSC_MISSING_SIGQUIT)
    signal(SIGQUIT, 0);
#endif
    SignalSet = PETSC_FALSE;
  }
  ierr = PetscNew(struct SH,&newsh);CHKERRQ(ierr);
  if (sh) {newsh->previous = sh;} 
  else {newsh->previous = 0;}
  newsh->handler = routine;
  newsh->ctx     = ctx;
  sh             = newsh;
  PetscFunctionReturn(0);
}

/* NO ERROR CODES RETURNED BY THIS FUNCTION */
#undef __FUNCT__  
#define __FUNCT__ "PetscPopSignalHandler"
int PetscPopSignalHandler(void)
{
  struct SH *tmp;

  PetscFunctionBegin;
  if (!sh) PetscFunctionReturn(0);
  tmp = sh;
  sh  = sh->previous;
  PetscFree(tmp);
  if (!sh || !sh->handler) {
    signal(SIGILL,  0);
    signal(SIGFPE,  0);
    signal(SIGSEGV, 0);
#if !defined(PETSC_MISSING_SIGSYS)
    signal(SIGSYS,  0);
#endif
#if !defined(PETSC_MISSING_SIGBUS)
    signal(SIGBUS,  0);
#endif
#if !defined(PETSC_MISSING_SIGQUIT)
    signal(SIGQUIT, 0);
#endif
    SignalSet = PETSC_FALSE;
  } else {
    SignalSet = PETSC_TRUE;
  }
  PetscFunctionReturn(0);
}




