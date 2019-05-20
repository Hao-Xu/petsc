
static char help[] = "Tests MatCreateComposite()\n\n";

/*T
   Concepts: Mat^composite matrices
   Processors: n
T*/

/*
  Include "petscmat.h" so that we can use matrices.
  automatically includes:
     petscsys.h       - base PETSc routines   petscvec.h    - vectors
     petscmat.h    - matrices
     petscis.h     - index sets            petscviewer.h - viewers
*/
#include <petscmat.h>

int main(int argc,char **args)
{
  Mat              *A,B;           /* matrix */
  PetscErrorCode   ierr;
  Vec              x,y,v,v2,z;
  PetscReal        rnorm;
  PetscInt         n = 20;         /* size of the matrix */
  PetscInt         nmat = 3;       /* number of matrices */
  PetscInt         i;
  PetscRandom      rctx;
  MatCompositeType type;

  ierr = PetscInitialize(&argc,&args,(char*)0,help);if (ierr) return ierr;
  ierr = PetscOptionsGetInt(NULL,NULL,"-n",&n,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-nmat",&nmat,NULL);CHKERRQ(ierr);

  /*
     Create random matrices
  */
  ierr = PetscMalloc1(nmat+3,&A);CHKERRQ(ierr);
  ierr = PetscRandomCreate(PETSC_COMM_WORLD,&rctx);CHKERRQ(ierr);
  ierr = MatCreateAIJ(PETSC_COMM_WORLD,PETSC_DECIDE,PETSC_DECIDE,n,n/2,3,NULL,3,NULL,&A[0]);CHKERRQ(ierr);
  for (i = 1; i < nmat+1; i++) {
    ierr = MatCreateAIJ(PETSC_COMM_WORLD,PETSC_DECIDE,PETSC_DECIDE,n,n,3,NULL,3,NULL,&A[i]);CHKERRQ(ierr);
  }
  ierr = MatCreateAIJ(PETSC_COMM_WORLD,PETSC_DECIDE,PETSC_DECIDE,n/2,n,3,NULL,3,NULL,&A[nmat+1]);CHKERRQ(ierr);
  for (i = 0; i < nmat+2; i++) {
    ierr = MatSetRandom(A[i],rctx);CHKERRQ(ierr);
  }

  ierr = MatCreateVecs(A[1],&x,&y);CHKERRQ(ierr);
  ierr = VecDuplicate(y,&z);CHKERRQ(ierr);
  ierr = MatCreateVecs(A[0],&v,NULL);CHKERRQ(ierr);
  ierr = VecDuplicate(v,&v2);CHKERRQ(ierr);

  ierr = VecSet(x,1.0);CHKERRQ(ierr);
  ierr = VecSet(y,0.0);CHKERRQ(ierr);
  ierr = MatMult(A[1],x,z);CHKERRQ(ierr);
  for (i = 2; i < nmat+1; i++) {
    ierr = MatMultAdd(A[i],x,z,z);CHKERRQ(ierr);
  }

  ierr = MatCreateComposite(PETSC_COMM_WORLD,nmat,A+1,&B);CHKERRQ(ierr);
  ierr = MatMultAdd(B,x,y,y);CHKERRQ(ierr);
  ierr = VecAXPY(y,-1.0,z);CHKERRQ(ierr);
  ierr = VecNorm(y,NORM_2,&rnorm);CHKERRQ(ierr);
  if (rnorm > 10000.0*PETSC_MACHINE_EPSILON) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Error with composite add %g\n",(double)rnorm);CHKERRQ(ierr);
  }

  ierr = MatCompositeMerge(B);CHKERRQ(ierr);
  ierr = MatMult(B,x,y);CHKERRQ(ierr);
  ierr = MatDestroy(&B);CHKERRQ(ierr);
  ierr = VecAXPY(y,-1.0,z);CHKERRQ(ierr);
  ierr = VecNorm(y,NORM_2,&rnorm);CHKERRQ(ierr);
  if (rnorm > 10000.0*PETSC_MACHINE_EPSILON) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Error with composite add after merge %g\n",(double)rnorm);CHKERRQ(ierr);
  }

  /*
     Test n x n/2 multiplicative composite
  */
  ierr = VecSet(v,1.0);CHKERRQ(ierr);
  ierr = MatMult(A[0],v,z);CHKERRQ(ierr);
  for (i = 1; i < nmat; i++) {
    ierr = MatMult(A[i],z,y);CHKERRQ(ierr);
    ierr = VecCopy(y,z);CHKERRQ(ierr);
  }

  ierr = MatCreateComposite(PETSC_COMM_WORLD,nmat,A,&B);CHKERRQ(ierr);
  ierr = MatCompositeSetType(B,MAT_COMPOSITE_MULTIPLICATIVE);CHKERRQ(ierr);
  ierr = MatCompositeSetMergeType(B,MAT_COMPOSITE_MERGE_LEFT);CHKERRQ(ierr);
  ierr = MatSetFromOptions(B);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr); /* do MatCompositeMerge() if -mat_composite_merge 1 */
  ierr = MatMult(B,v,y);CHKERRQ(ierr);
  ierr = MatDestroy(&B);CHKERRQ(ierr);
  ierr = VecAXPY(y,-1.0,z);CHKERRQ(ierr);
  ierr = VecNorm(y,NORM_2,&rnorm);CHKERRQ(ierr);
  if (rnorm > 10000.0*PETSC_MACHINE_EPSILON) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Error with composite multiplicative %g\n",(double)rnorm);CHKERRQ(ierr);
  }

  /*
     Test n/2 x n multiplicative composite
  */
  ierr = VecSet(x,1.0);CHKERRQ(ierr);
  ierr = MatMult(A[2],x,z);CHKERRQ(ierr);
  for (i = 3; i < nmat+1; i++) {
    ierr = MatMult(A[i],z,y);CHKERRQ(ierr);
    ierr = VecCopy(y,z);CHKERRQ(ierr);
  }
  ierr = MatMult(A[nmat+1],z,v);CHKERRQ(ierr);

  ierr = MatCreateComposite(PETSC_COMM_WORLD,nmat,A+2,&B);CHKERRQ(ierr);
  ierr = MatCompositeSetType(B,MAT_COMPOSITE_MULTIPLICATIVE);CHKERRQ(ierr);
  ierr = MatSetFromOptions(B);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr); /* do MatCompositeMerge() if -mat_composite_merge 1 */
  ierr = MatMult(B,x,v2);CHKERRQ(ierr);
  ierr = MatDestroy(&B);CHKERRQ(ierr);
  ierr = VecAXPY(v2,-1.0,v);CHKERRQ(ierr);
  ierr = VecNorm(v2,NORM_2,&rnorm);CHKERRQ(ierr);
  if (rnorm > 10000.0*PETSC_MACHINE_EPSILON) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Error with composite multiplicative %g\n",(double)rnorm);CHKERRQ(ierr);
  }

  /*
     Test get functions
  */
  ierr = MatCreateComposite(PETSC_COMM_WORLD,nmat,A,&B);CHKERRQ(ierr);
  ierr = MatCompositeGetNumberMat(B,&n);CHKERRQ(ierr);
  if (nmat != n) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Error with GetNumberMat %d != %d\n",nmat,n);CHKERRQ(ierr);
  }
  ierr = MatCompositeGetMat(B,0,&A[nmat+2]);CHKERRQ(ierr);
  if (A[0] != A[nmat+2]) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Error with GetMat\n");CHKERRQ(ierr);
  }
  ierr = MatCompositeGetType(B,&type);CHKERRQ(ierr);
  if (type != MAT_COMPOSITE_ADDITIVE) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Error with GetType\n");CHKERRQ(ierr);
  }
  ierr = MatDestroy(&B);CHKERRQ(ierr);

  /*
     Free work space.  All PETSc objects should be destroyed when they
     are no longer needed.
  */
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = VecDestroy(&y);CHKERRQ(ierr);
  ierr = VecDestroy(&v);CHKERRQ(ierr);
  ierr = VecDestroy(&v2);CHKERRQ(ierr);
  ierr = VecDestroy(&z);CHKERRQ(ierr);
  ierr = PetscRandomDestroy(&rctx);CHKERRQ(ierr);
  for (i = 0; i < nmat+2; i++) {
    ierr = MatDestroy(&A[i]);CHKERRQ(ierr);
  }
  ierr = PetscFree(A);CHKERRQ(ierr);

  ierr = PetscFinalize();
  return ierr;
}

/*TEST

   test:
      nsize: 2
      requires: double
      args: -mat_composite_merge {{0 1}shared output}

TEST*/
