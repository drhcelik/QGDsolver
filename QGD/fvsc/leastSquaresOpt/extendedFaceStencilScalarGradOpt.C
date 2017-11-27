#include "leastSquaresStencilOpt.H"
#include "polyMesh.H"
#include "fvMesh.H"
#include "word.H"
#include "IOstream.H"
#include "Ostream.H"
#include <HashTable.H>

#include "emptyFvPatch.H"
#include "coupledFvPatch.H"
#include "wedgeFvPatch.H"
#include "symmetryFvPatch.H"
#include "symmetryPlaneFvPatch.H"

//- Calculate gradient of volume scalar function on the faces
//
// \param iF         Internal scalar field.
//                   Allowable values: constant reference to the volScalarField.
//
// \return           Gradient of iF (vector field) which was computed on the faces of mesh.
Foam::tmp<Foam::surfaceVectorField> Foam::fvsc::leastSquaresOpt::Grad(const volScalarField& iF)
{
    surfaceScalarField sF = linearInterpolate(iF); 
    return Grad(iF,sF);
};

Foam::tmp<Foam::surfaceVectorField> 
Foam::fvsc::leastSquaresOpt::Grad(const volScalarField& iF, const surfaceScalarField& sF)
{

    tmp<surfaceVectorField> tgradIF(0.0*nf_*fvc::snGrad(iF));
    surfaceVectorField& gradIF = tgradIF.ref();
    //scalarField tField = sF;
    surfaceScalarField tField = sF*0;

    faceScalarDer(iF.primitiveField(),sF.primitiveField(),0,tField);
    gradIF.primitiveFieldRef().replace(0, tField);
    faceScalarDer(iF.primitiveField(),sF.primitiveField(),1,tField);
    gradIF.primitiveFieldRef().replace(1, tField);
    faceScalarDer(iF.primitiveField(),sF.primitiveField(),2,tField);
    gradIF.primitiveFieldRef().replace(2, tField);
    
    //update boundary field
    forAll(mesh_.boundaryMesh(), ipatch)
    {
        bool notConstrain = true;
        const fvPatch& fvp = mesh_.boundary()[ipatch];
        if
        (
            isA<emptyFvPatch>(fvp) ||
            isA<wedgeFvPatch>(fvp) ||
            isA<coupledFvPatch>(fvp) ||
            isA<symmetryFvPatch>(fvp) ||
            isA<symmetryPlaneFvPatch>(fvp)
        )
        {
            notConstrain = false;
        }

        if (notConstrain)
        {
            gradIF.boundaryFieldRef()[ipatch] = 
                nf_.boundaryField()[ipatch] * 
                iF.boundaryField()[ipatch].snGrad();
        }
    }

    if(!Pstream::parRun())
    {
        return tgradIF;
    }
    
    /*
     *
     * Update processor patches for parallel case
     *
     */
    
    //allocate storage for near-patch field

    List3<scalar> procVfValues(nProcPatches_); //array of values from neighb. processors
    formVfValues(iF,procVfValues);

    faceScalarDer(procVfValues,sF,0,tField);
    gradIF.boundaryFieldRef().replace(0, tField.boundaryFieldRef());
    faceScalarDer(procVfValues,sF,1,tField);
    gradIF.boundaryFieldRef().replace(1, tField.boundaryFieldRef());
    faceScalarDer(procVfValues,sF,2,tField);
    gradIF.boundaryFieldRef().replace(2, tField.boundaryFieldRef());
    
    return tgradIF;


};

//
//END-OF-FILE
//
