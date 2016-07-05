///    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ \.
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 Kratos default license: kratos/license.txt
//
//  Main authors:    Zhiming Guo
//                   Riccardo Rossi
//




// System includes


// External includes


// Project includes
#include "includes/define.h"
#include "custom_elements/updated_lagrangian_mpm_element.h"
#include "includes/element.h"
#include "lagrangian_mpm_application_variables.h"

#include "utilities/math_utils.h"

#include "geometries/geometry.h"
//#include "custom_geometries/meshless_geometry.h"

namespace Kratos
{

//************************************************************************************
//************************************************************************************
UpdatedLagrangianMPMElement::UpdatedLagrangianMPMElement(
	IndexType NewId,
	GeometryType::Pointer pGeometry)
    : MeshlessBaseElement(NewId, pGeometry)
{
    //DO NOT ADD DOFS HERE!!!

}

//************************************************************************************
//************************************************************************************
UpdatedLagrangianMPMElement::UpdatedLagrangianMPMElement(
	IndexType NewId,
	GeometryType::Pointer pGeometry,  
	PropertiesType::Pointer pProperties)
    : MeshlessBaseElement(NewId, pGeometry, pProperties)
{
	mFinalizedStep = true; 
}

Element::Pointer UpdatedLagrangianMPMElement::Create(
	IndexType NewId, 
	NodesArrayType const& ThisNodes,  
	PropertiesType::Pointer pProperties) const
{
    return Element::Pointer(new UpdatedLagrangianMPMElement(NewId, GetGeometry().Create(ThisNodes), pProperties));
}

Element::Pointer UpdatedLagrangianMPMElement::Create(Element::IndexType NewId,
                           Element::GeometryType::Pointer pGeom,
                           PropertiesType::Pointer pProperties) const
    {
        KRATOS_TRY
        return Element::Pointer(new UpdatedLagrangianMPMElement(NewId, pGeom, pProperties));
        KRATOS_CATCH("");
    }

UpdatedLagrangianMPMElement::~UpdatedLagrangianMPMElement()
{
}
//************************************************************************************
//************************************************************************************

    void UpdatedLagrangianMPMElement::Initialize()
    {
        KRATOS_TRY
        
        
        array_1d<double,3>& xg = this->GetValue(GAUSS_COORD);
       
        

        const unsigned int dim = GetGeometry().WorkingSpaceDimension();
        
        mDeterminantF0 = 1;
        
        mDeformationGradientF0 = identity_matrix<double> (dim);
              
        
        //Compute jacobian inverses 
        
        Matrix J0 = ZeroMatrix(dim, dim);
        
        J0 = GetGeometry().Jacobian( J0 , xg);   
        
        //calculating and storing inverse and the determinant of the jacobian 
        MathUtils<double>::InvertMatrix( J0, mInverseJ0, mDeterminantJ0 );
        
        Matrix j = ZeroMatrix(dim,dim);
        j = GetGeometry().Jacobian( j , xg);  
        double detj;
        MathUtils<double>::InvertMatrix( j, mInverseJ, detj );
        
        InitializeMaterial();  

        KRATOS_CATCH( "" )
    }

////************************************************************************************
////************************************************************************************

    void UpdatedLagrangianMPMElement::InitializeSolutionStep( ProcessInfo& rCurrentProcessInfo )
    {   
		GeneralVariables Variables;
        
        this->InitializeGeneralVariables(Variables,rCurrentProcessInfo);
		
		mConstitutiveLawVector->InitializeSolutionStep( GetProperties(),
                    GetGeometry(), Variables.N, rCurrentProcessInfo );

        mFinalizedStep = false;
	}
////************************************************************************************
////************************************************************************************

    void UpdatedLagrangianMPMElement::FinalizeSolutionStep( ProcessInfo& rCurrentProcessInfo )
    {
        KRATOS_TRY
        
        const unsigned int number_of_nodes = GetGeometry().PointsNumber();
		const unsigned int dimension = GetGeometry().WorkingSpaceDimension();
		unsigned int voigtsize  = 3;
            
        if( dimension == 3 )
        {
            voigtsize  = 6;
        }
       
        //create and initialize element variables:
        GeneralVariables Variables;
        this->InitializeGeneralVariables(Variables,rCurrentProcessInfo);

        //create constitutive law parameters:
        ConstitutiveLaw::Parameters Values(GetGeometry(),GetProperties(),rCurrentProcessInfo);

        //set constitutive law flags:
        Flags &ConstitutiveLawOptions=Values.GetOptions();

        ConstitutiveLawOptions.Set(ConstitutiveLaw::COMPUTE_STRAIN);
        ConstitutiveLawOptions.Set(ConstitutiveLaw::COMPUTE_STRESS);
        
        //compute element kinematics B, F, DN_DX ...
        this->CalculateKinematics(Variables, rCurrentProcessInfo);

        //set general variables to constitutivelaw parameters
        this->SetGeneralVariables(Variables,Values);

        //call the constitutive law to update material variables
        mConstitutiveLawVector->FinalizeMaterialResponse(Values, Variables.StressMeasure);

        //call the constitutive law to finalize the solution step
        mConstitutiveLawVector->FinalizeSolutionStep( GetProperties(),
                GetGeometry(),
                Variables.N,
                rCurrentProcessInfo );
        
        //call the element internal variables update
        this->FinalizeStepVariables(Variables, rCurrentProcessInfo);
        

        mFinalizedStep = true;

        KRATOS_CATCH( "" )
    }
    
////************************************************************************************
////************************************************************************************

    void UpdatedLagrangianMPMElement::FinalizeStepVariables( GeneralVariables & rVariables, const ProcessInfo& rCurrentProcessInfo)
    {
    //update internal (historical) variables
    mDeterminantF0         = rVariables.detF* rVariables.detF0;
    mDeformationGradientF0 = prod(rVariables.F, rVariables.F0);
       
    }
//**************************************************************************************    
    void UpdatedLagrangianMPMElement::InitializeMaterial()
    {
        KRATOS_TRY
        array_1d<double,3>& xg = this->GetValue(GAUSS_COORD);
        GeneralVariables Variables;
        this->InitializeGeneralVariables(Variables,rCurrentProcessInfo);
        
        
        if ( GetProperties()[CONSTITUTIVE_LAW] != NULL )
        {
            
            mConstitutiveLawVector = GetProperties()[CONSTITUTIVE_LAW]->Clone();
                       
            mConstitutiveLawVector->InitializeMaterial( GetProperties(), GetGeometry(),
                    Variables.N );
            
        }
        else
            KRATOS_THROW_ERROR( std::logic_error, "a constitutive law needs to be specified for the element with ID ", this->Id() )
        //std::cout<< "in initialize material "<<std::endl;    
        KRATOS_CATCH( "" )
    }
//************************************************************************************
//************************************************************************************

    void UpdatedLagrangianMPMElement::ResetConstitutiveLaw()
    {
        KRATOS_TRY
        array_1d<double,3>& xg = this->GetValue(GAUSS_COORD);
        GeneralVariables Variables;
        this->InitializeGeneralVariables(Variables,rCurrentProcessInfo);
        
        //create and initialize element variables:
        
        if ( GetProperties()[CONSTITUTIVE_LAW] != NULL )
        {
            
            mConstitutiveLawVector->ResetMaterial( GetProperties(), GetGeometry(), Variables.N );
        }

        KRATOS_CATCH( "" )
    }
    
    
                
//************************************************************************************
//************************************************************************************
    void UpdatedLagrangianMPMElement::CalculateLocalSystem(MatrixType& rLeftHandSideMatrix,
                                      VectorType& rRightHandSideVector,
                                      ProcessInfo& rCurrentProcessInfo)
    {
        const unsigned int number_of_nodes = GetGeometry().size();
        const unsigned int dim = GetDomainSize();
        unsigned int matrix_size = number_of_nodes * dim;
       if (rLeftHandSideMatrix.size1() != matrix_size)
	  rLeftHandSideMatrix.resize(matrix_size, matrix_size, false);
        if (rRightHandSideVector.size() != matrix_size)
	  rRightHandSideVector.resize(matrix_size, false);
        
        
        CalculateElementalSystem( rLeftHandSideMatrix, rRightHandSideVector, rCurrentProcessInfo );
        //obtain shape functions
        
        //obtaine delta_disp and store it in a Matrix
        
        //Compute deltaF = I + D delta_disp / D xn
        
        //compute B
        
        //Compute Ftot
        
        //Compute material response sigma_cauchy, C
        
        //compute RHS
        
        //compute LHS
                
    }
    void UpdatedLagrangianMPMElement::CalculateElementalSystem( MatrixType& rLeftHandSideMatrix,
                                 VectorType& rRightHandSideVector, ProcessInfo& rCurrentProcessInfo)
    {
        KRATOS_TRY
        
        //create and initialize element variables:
        GeneralVariables Variables;
        
        this->InitializeGeneralVariables(Variables,rCurrentProcessInfo);
        
        //create constitutive law parameters:
        ConstitutiveLaw::Parameters Values(GetGeometry(),GetProperties(),rCurrentProcessInfo);
        
       
        //set constitutive law flags:
        Flags &ConstitutiveLawOptions=Values.GetOptions();
        
        //std::cout<<"in CalculateElementalSystem 5"<<std::endl;
        ConstitutiveLawOptions.Set(ConstitutiveLaw::COMPUTE_STRAIN);
        
        ConstitutiveLawOptions.Set(ConstitutiveLaw::COMPUTE_STRESS);
        
        ConstitutiveLawOptions.Set(ConstitutiveLaw::COMPUTE_CONSTITUTIVE_TENSOR);
        
        
        //auxiliary terms
        Vector VolumeForce;
        
        
        //compute element kinematics B, F, DN_DX ...
        this->CalculateKinematics(Variables,rCurrentProcessInfo);
        
        //set general variables to constitutivelaw parameters
        this->SetGeneralVariables(Variables,Values);
        
        mConstitutiveLawVector->CalculateMaterialResponse(Values, Variables.StressMeasure);
                
        this->CalculateAndAddLHS ( rLeftHandSideMatrix, Variables);
        
        VolumeForce  = this->CalculateVolumeForce( VolumeForce, Variables );
        
        this->CalculateAndAddRHS ( rRightHandSideVector, Variables, VolumeForce);
        
        KRATOS_CATCH( "" )
    }
    
//**********************************CALCULATE LHS************************************************************************************
//***********************************************************************************************************************************
    void UpdatedLagrangianMPMElement::CalculateAndAddLHS(MatrixType& rLeftHandSideMatrix, GeneralVariables& rVariables)
    {
		this->CalculateAndAddKuum( rLeftHandSideMatrix, rVariables );
		this->CalculateAndAddKuug( rLeftHandSideMatrix, rVariables );
	}
    
//***********************************MATERIAL MATRIX**********************************
//************************************************************************************

    void UpdatedLagrangianMPMElement::CalculateAndAddKuum(MatrixType& rLeftHandSideMatrix,
            GeneralVariables& rVariables )
    {
        KRATOS_TRY    
        noalias( rLeftHandSideMatrix ) += prod( trans( rVariables.B ),  rVariables.IntegrationWeight * Matrix( prod( rVariables.ConstitutiveMatrix, rVariables.B ) ) ); 

        KRATOS_CATCH( "" )
    }
//**********************************GEOMETRICAL MATRIX********************************
//************************************************************************************

    void UpdatedLagrangianMPMElement::CalculateAndAddKuug(MatrixType& rLeftHandSideMatrix,
            GeneralVariables& rVariables)

    {
        KRATOS_TRY

        unsigned int dimension = GetGeometry().WorkingSpaceDimension();
        Matrix StressTensor = MathUtils<double>::StressVectorToTensor( rVariables.StressVector );
        Matrix ReducedKg = prod( rVariables.DN_DX, rVariables.IntegrationWeight * Matrix( prod( StressTensor, trans( rVariables.DN_DX ) ) ) ); 
        MathUtils<double>::ExpandAndAddReducedMatrix( rLeftHandSideMatrix, ReducedKg, dimension ); 

        KRATOS_CATCH( "" )
    }
    
//**********************************CALCULATE RHS************************************************************************************
//***********************************************************************************************************************************    
    void UpdatedLagrangianMPMElement::CalculateAndAddRHS(VectorType& rRightHandSideVector, GeneralVariables& rVariables, Vector& rVolumeForce)
    {    
		this->CalculateAndAddExternalForces( rRightHandSideVector, rVariables, rVolumeForce );
		this->CalculateAndAddInternalForces( rRightHandSideVector, rVariables);
	}
    
//*********************************************************************************************************************************     
//************************************VOLUME FORCE VECTOR**************************************************************************    
	Vector& UpdatedLagrangianMPMElement::CalculateVolumeForce( Vector& rVolumeForce, GeneralVariables& rVariables )
	{
		KRATOS_TRY

		const unsigned int number_of_nodes = GetGeometry().size();
		const unsigned int dimension       = GetGeometry().WorkingSpaceDimension();

		rVolumeForce = ZeroVector(dimension);

		for ( unsigned int j = 0; j < number_of_nodes; j++ )
		{
		  if( GetGeometry()[j].SolutionStepsDataHas(VOLUME_ACCELERATION) ){ // it must be checked once at the begining only
		array_1d<double, 3 >& VolumeAcceleration = GetGeometry()[j].FastGetSolutionStepValue(VOLUME_ACCELERATION);
		for( unsigned int i = 0; i < dimension; i++ )
		  rVolumeForce[i] += rVariables.N[j] * VolumeAcceleration[i] ;
		  }
		}

		rVolumeForce *= 1.0 / (rVariables.detFT) * GetProperties()[DENSITY];

		return rVolumeForce;

		KRATOS_CATCH( "" )
	}    

//************************************************************************************
//*********************Calculate the contribution of external force*******************

    void UpdatedLagrangianMPMElement::CalculateAndAddExternalForces(VectorType& rRightHandSideVector,
            GeneralVariables& rVariables,
            Vector& rVolumeForce)

    {
        KRATOS_TRY
    unsigned int number_of_nodes = GetGeometry().PointsNumber();
    unsigned int dimension = GetGeometry().WorkingSpaceDimension();

    double DomainChange = (1.0/rVariables.detF0); //density_n+1 = density_0 * ( 1.0 / detF0 )

    for ( unsigned int i = 0; i < number_of_nodes; i++ )
    {
        int index = dimension * i;
        for ( unsigned int j = 0; j < dimension; j++ )
        {
	  rRightHandSideVector[index + j] += rVariables.IntegrationWeight * rVariables.N[i] * rVolumeForce[j] * DomainChange;
        }
    }

    KRATOS_CATCH( "" )
    }    
//************************************************************************************
//*********************Calculate the contribution of internal force*******************

    void UpdatedLagrangianMPMElement::CalculateAndAddInternalForces(VectorType& rRightHandSideVector,
            GeneralVariables & rVariables)
    {
        KRATOS_TRY

        VectorType InternalForces = rVariables.IntegrationWeight * prod( trans( rVariables.B ), rVariables.StressVector );
        
        noalias( rRightHandSideVector ) -= InternalForces;

        KRATOS_CATCH( "" )
    }
//**********************************INITIALIZATION OF ELEMENT VARIABLES***************************************************************
//************************************************************************************************************************************
    void UpdatedLagrangianMPMElement::InitializeGeneralVariables (GeneralVariables& rVariables, const ProcessInfo& rCurrentProcessInfo)
    {
        const unsigned int number_of_nodes = GetGeometry().size();
        const unsigned int dimension = GetGeometry().WorkingSpaceDimension();
        unsigned int voigtsize  = 3;
            
        if( dimension == 3 )
        {
            voigtsize  = 6;
        }
        rVariables.detF  = 1;

        rVariables.detF0 = 1;
        
        rVariables.detFT = 1;

        //rVariables.detJ = 1;

        rVariables.B.resize( voigtsize , number_of_nodes * dimension );

        rVariables.F.resize( dimension, dimension );

        rVariables.F0.resize( dimension, dimension );
        
        rVariables.FT.resize( dimension, dimension );

        rVariables.ConstitutiveMatrix.resize( voigtsize, voigtsize );

        rVariables.StrainVector.resize( voigtsize );

        rVariables.StressVector.resize( voigtsize );
        
        this->GetGeometryData(integration_weight,N,DN_Dx);

        rVariables.DN_DX.resize( number_of_nodes, dimension );
        rVariables.DN_De.resize( number_of_nodes, dimension );
        
        array_1d<double,3>& xg = this->GetValue(GAUSS_POINT_COORDINATES);
        
        rVariables.N = N;
        rVariables.DN_DX = DN_Dx; //gradient values at the previous time step
        rVariables.integration_weight = integration_weight;
        //reading shape functions local gradients
        //rVariables.DN_De = this->MPMShapeFunctionsLocalGradients( rVariables.DN_De);
        
        
        //**********************************************************************************************************************
        
        //CurrentDisp is the variable unknown. It represents the nodal delta displacement. When it is predicted is equal to zero.
        
        rVariables.DeltaPosition = CalculateDeltaPosition(rVariables.DeltaPosition, rCurrentProcessInfo);
        
        
        ////calculating the current jacobian from cartesian coordinates to parent coordinates for the MP element [dx_n+1/d£]
        rVariables.j = GetGeometry().Jacobian( rVariables.j, xg);
        
                
        ////calculating the reference jacobian from cartesian coordinates to parent coordinates for the MP element [dx_n/d£]
        rVariables.J = GetGeometry().Jacobian( rVariables.J, xg, rVariables.DeltaPosition);
        
        //std::cout<<"The general variables are initialized"<<std::endl;
        //*************************************************************************************************************************

    }


//**************************SET GENERAL CONSTITUTIVE LAW PARAMETERS****************************************************************
//*********************************************************************************************************************************
    void UpdatedLagrangianMPMElement::SetGeneralVariables(GeneralVariables& rVariables,
            ConstitutiveLaw::Parameters& rValues)
    {   
        //Variables.detF is the determinant of the incremental total deformation gradient
        rVariables.detF  = MathUtils<double>::Det(rVariables.F);

        if(rVariables.detF<0){
            
            std::cout<<" Element: "<<this->Id()<<std::endl;
            std::cout<<" Element position "<<this->GetValue(GAUSS_COORD)<<std::endl;
            unsigned int number_of_nodes = GetGeometry().PointsNumber();

            for ( unsigned int i = 0; i < number_of_nodes; i++ )
            {
            array_1d<double, 3> &CurrentPosition  = GetGeometry()[i].Coordinates();
            array_1d<double, 3> & CurrentDisplacement  = GetGeometry()[i].FastGetSolutionStepValue(DISPLACEMENT);
            array_1d<double, 3> & PreviousDisplacement = GetGeometry()[i].FastGetSolutionStepValue(DISPLACEMENT,1);
            //array_1d<double, 3> PreviousPosition  = CurrentPosition - (CurrentDisplacement-PreviousDisplacement);         
            std::cout<<" NODE ["<<GetGeometry()[i].Id()<<"]: (Current position: "<<CurrentPosition<<") "<<std::endl;
            std::cout<<" ---Current Disp: "<<CurrentDisplacement<<" (Previour Disp: "<<PreviousDisplacement<<")"<<std::endl;
            }

            for ( unsigned int i = 0; i < number_of_nodes; i++ )
            {
                if( GetGeometry()[i].SolutionStepsDataHas(CONTACT_FORCE) ){
                array_1d<double, 3 > & PreContactForce = GetGeometry()[i].FastGetSolutionStepValue(CONTACT_FORCE,1);
                array_1d<double, 3 > & ContactForce = GetGeometry()[i].FastGetSolutionStepValue(CONTACT_FORCE);
                std::cout<<" ---Contact_Force: (Pre:"<<PreContactForce<<", Cur:"<<ContactForce<<") "<<std::endl;
                }
                else{
                std::cout<<" ---Contact_Force: NULL "<<std::endl;
                }   
            }
        
            KRATOS_THROW_ERROR( std::invalid_argument," MPM UPDATED LAGRANGIAN DISPLACEMENT ELEMENT INVERTED: |F|<0  detF = ", rVariables.detF )
        }
        
        rVariables.detFT = rVariables.detF * rVariables.detF0;
        rVariables.FT    = prod( rVariables.F, rVariables.F0 );

        
        rValues.SetDeterminantF(rVariables.detFT);
        rValues.SetDeformationGradientF(rVariables.FT);
        rValues.SetStrainVector(rVariables.StrainVector);
        rValues.SetStressVector(rVariables.StressVector);
        rValues.SetConstitutiveMatrix(rVariables.ConstitutiveMatrix);
        rValues.SetShapeFunctionsDerivatives(rVariables.DN_DX);
        rValues.SetShapeFunctionsValues(rVariables.N);
        
        
        //std::cout<<"The general variables are set"<<std::endl;

    }



//*********************************COMPUTE KINEMATICS*********************************
//************************************************************************************


    void UpdatedLagrangianMPMElement::CalculateKinematics(GeneralVariables& rVariables, ProcessInfo& rCurrentProcessInfo)

    {
        KRATOS_TRY

        const unsigned int dimension = GetGeometry().WorkingSpaceDimension();
        
        //Define the stress measure
        rVariables.StressMeasure = ConstitutiveLaw::StressMeasure_Cauchy;

        //Calculating the inverse of the jacobian and the parameters needed [d£/dx_n]
        //Matrix InvJ;
        
        //MathUtils<double>::InvertMatrix( rVariables.J, InvJ, rVariables.detJ);
        
        
        
        //Calculating the inverse of the jacobian and the parameters needed [d£/(dx_n+1)]
        Matrix Invj;
        MathUtils<double>::InvertMatrix( rVariables.j, Invj, rVariables.detJ ); //overwrites detJ

        Matrix InvF = prod (rVariables.J, invj);

        //Compute cartesian derivatives [dN/dx_n+1]        
        rVariables.DN_DX = prod( rVariables.DN_DX, InvF); //overwrites DX now is the current position dx
        
        //Deformation Gradient F [(dx_n+1 - dx_n)/dx_n] to be updated in constitutive law parameter as total deformation gradient
        //the increment of total deformation gradient can be evaluated in 2 ways.
        //1 way.
        //noalias( rVariables.F ) = prod( rVariables.j, InvJ);
        
        //2 way by means of the gradient of nodal displacement: using this second expression quadratic convergence is not guarantee

        Matrix I=identity_matrix<double>( dimension );
        
        Matrix GradientDisp = ZeroMatrix(dimension, dimension);
        rVariables.DeltaPosition = CalculateDeltaPosition(rVariables.DeltaPosition, rCurrentProcessInfo);
        GradientDisp = prod(trans(rVariables.DeltaPosition),rVariables.DN_DX);
        
        
        noalias( rVariables.F ) = (I + GradientDisp);
        
        
        //Determinant of the Deformation Gradient F_n
                        
        rVariables.detF0 = mDeterminantF0;
        rVariables.F0    = mDeformationGradientF0;
        
        
        
        //Compute the deformation matrix B
        this->CalculateDeformationMatrix(rVariables.B, rVariables.F, rVariables.DN_DX);

        
        KRATOS_CATCH( "" )
    }
//***********************************************************************************************************
//                                   Deformation Matrix B
//***********************************************************************************************************
    void UpdatedLagrangianMPMElement::CalculateDeformationMatrix(Matrix& rB,
            Matrix& rF,
            Matrix& rDN_DX)
    {
        KRATOS_TRY

        const unsigned int number_of_nodes = GetGeometry().size();
        const unsigned int dimension       = GetGeometry().WorkingSpaceDimension();
        
        rB.clear(); //set all components to zero

        if( dimension == 2 )
        {

            for ( unsigned int i = 0; i < number_of_nodes; i++ )
            {
                unsigned int index = 2 * i;

                rB( 0, index + 0 ) = rDN_DX( i, 0 );
                rB( 1, index + 1 ) = rDN_DX( i, 1 );
                rB( 2, index + 0 ) = rDN_DX( i, 1 );
                rB( 2, index + 1 ) = rDN_DX( i, 0 );

            }
            //if(this->Id() == 365)
        //{
			//std::cout<<"rB "<< this->Id()<< rB<<std::endl;
		//}

        }
        
        else if( dimension == 3 )
        {

            for ( unsigned int i = 0; i < number_of_nodes; i++ )
            {
                unsigned int index = 3 * i;

                rB( 0, index + 0 ) = rDN_DX( i, 0 );
                rB( 1, index + 1 ) = rDN_DX( i, 1 );
                rB( 2, index + 2 ) = rDN_DX( i, 2 );

                rB( 3, index + 0 ) = rDN_DX( i, 1 );
                rB( 3, index + 1 ) = rDN_DX( i, 0 );

                rB( 4, index + 1 ) = rDN_DX( i, 2 );
                rB( 4, index + 2 ) = rDN_DX( i, 1 );

                rB( 5, index + 0 ) = rDN_DX( i, 2 );
                rB( 5, index + 2 ) = rDN_DX( i, 0 );

            }
        }
        else
        {

            KRATOS_THROW_ERROR( std::invalid_argument, "something is wrong with the dimension", "" )

        }

        KRATOS_CATCH( "" )
    }

//*************************COMPUTE CURRENT DISPLACEMENT*******************************
//************************************************************************************


	Matrix& UpdatedLagrangianMPMElement::CalculateDeltaPosition(Matrix & rDeltaPosition)
	{
		KRATOS_TRY

		const unsigned int number_of_nodes = GetGeometry().PointsNumber();
		unsigned int dimension = GetGeometry().WorkingSpaceDimension();

		rDeltaPosition = zero_matrix<double>( number_of_nodes , dimension);

		for ( unsigned int i = 0; i < number_of_nodes; i++ )
		{
			array_1d<double, 3 > & CurrentDisplacement  = GetGeometry()[i].FastGetSolutionStepValue(DISPLACEMENT);
			array_1d<double, 3 > & PreviousDisplacement = GetGeometry()[i].FastGetSolutionStepValue(DISPLACEMENT,1);

			for ( unsigned int j = 0; j < dimension; j++ )
			{
				rDeltaPosition(i,j) = CurrentDisplacement[j]-PreviousDisplacement[j];
			}
		}

		return rDeltaPosition;

		KRATOS_CATCH( "" )
	}

//*******************************************************************************************************************
    void UpdatedLagrangianMPMElement::CalculateMassMatrix(MatrixType& rMassMatrix, ProcessInfo& rCurrentProcessInfo)
    {
        const unsigned int number_of_nodes = GetGeometry().size();
        const unsigned int dim = GetDomainSize();
        unsigned int matrix_size = number_of_nodes * dim;
        
       if (rMassMatrix.size1() != matrix_size)
	  rMassMatrix.resize(matrix_size, matrix_size, false);
       
       //set it to zero
       rMassMatrix.clear();
       GeneralVariables Variables;
       this->InitializeGeneralVariables(Variables,rCurrentProcessInfo);
       
       
       
       //fill the matrix
       for(unsigned int i=0; i<number_of_nodes; i++)
       {
           for(unsigned int j=0; j<number_of_nodes; j++)
            {
                for(unsigned int k=0; k<dim; k++)
                {
                    rMassMatrix(i*dim+k, j*dim+k) += Variables.integration_weight*Variables.N[i]*Variables.N[j]; //consistent matrix??
                }
            }
       }
        
    }
//*********************************************************************************************************************    
    void UpdatedLagrangianMPMElement::EquationIdVector( EquationIdVectorType& rResult, ProcessInfo& CurrentProcessInfo )
    {
        const unsigned int number_of_nodes = GetGeometry().size();
        const unsigned int dim = GetDomainSize();
        unsigned int matrix_size = number_of_nodes * dim;

        if ( rResult.size() != matrix_size )
            rResult.resize( matrix_size, false );

        for ( int i = 0; i < number_of_nodes; i++ )
        {
            int index = i * dim;
            rResult[index] = GetGeometry()[i].GetDof( DISPLACEMENT_X ).EquationId();
            rResult[index + 1] = GetGeometry()[i].GetDof( DISPLACEMENT_Y ).EquationId();

            if ( dim == 3 )
                rResult[index + 2] = GetGeometry()[i].GetDof( DISPLACEMENT_Z ).EquationId();
        }

    }

//************************************************************************************
//************************************************************************************

    void UpdatedLagrangianMPMElement::GetDofList( DofsVectorType& ElementalDofList, ProcessInfo& CurrentProcessInfo )
    {
        ElementalDofList.resize( 0 );

        for ( unsigned int i = 0; i < GetGeometry().size(); i++ )
        {
            ElementalDofList.push_back( GetGeometry()[i].pGetDof( DISPLACEMENT_X ) );
            ElementalDofList.push_back( GetGeometry()[i].pGetDof( DISPLACEMENT_Y ) );

            if ( GetDomainSize() == 3 )
            {
                ElementalDofList.push_back( GetGeometry()[i].pGetDof( DISPLACEMENT_Z ) );
            }
        }
    }
    
    void UpdatedLagrangianMPMElement::GetValuesVector( Vector& values, int Step )
    {
        const unsigned int number_of_nodes = GetGeometry().size();
        const unsigned int dim = GetDomainSize();
        unsigned int matrix_size = number_of_nodes * dim;

        if ( values.size() != matrix_size ) values.resize( matrix_size, false );

        for ( unsigned int i = 0; i < number_of_nodes; i++ )
        {
            unsigned int index = i * dim;
            const array_1d<double,3>& disp = GetGeometry()[i].GetSolutionStepValue( DISPLACEMENT, Step );
            values[index] = disp[0];
            values[index + 1] = disp[1];;

            if ( dim == 3 )
                values[index + 2] = disp[2];
        }
    }


//************************************************************************************
//************************************************************************************

    void UpdatedLagrangianMPMElement::GetFirstDerivativesVector( Vector& values, int Step )
    {
        const unsigned int number_of_nodes = GetGeometry().size();
        const unsigned int dim = GetDomainSize();
        unsigned int matrix_size = number_of_nodes * dim;

        if ( values.size() != matrix_size ) values.resize( matrix_size, false );

        for ( unsigned int i = 0; i < number_of_nodes; i++ )
        {
            unsigned int index = i * dim;
            const array_1d<double,3>& vel = GetGeometry()[i].GetSolutionStepValue( VELOCITY, Step );
            values[index] = vel[0];
            values[index + 1] = vel[1];

            if ( dim == 3 )
                values[index + 2] = vel[2];
        }
    }

//************************************************************************************
//************************************************************************************

    void UpdatedLagrangianMPMElement::GetSecondDerivativesVector( Vector& values, int Step )
    {
        const unsigned int number_of_nodes = GetGeometry().size();
        const unsigned int dim = GetDomainSize();
        unsigned int matrix_size = number_of_nodes * dim;

        if ( values.size() != matrix_size ) values.resize( matrix_size, false );

        for ( unsigned int i = 0; i < number_of_nodes; i++ )
        {
            unsigned int index = i * dim;
            const array_1d<double,3>& acc = GetGeometry()[i].GetSolutionStepValue( ACCELERATION, Step );
            values[index] = acc[0];
            values[index + 1] = acc[1];

            if ( dim == 3 )
                values[index + 2] = acc[2];
        }
    }
    
//************************************************************************************
//************************************************************************************

//************************************************************************************
//************************************************************************************
/**
 * This function provides the place to perform checks on the completeness of the input.
 * It is designed to be called only once (or anyway, not often) typically at the beginning
 * of the calculations, so to verify that nothing is missing from the input
 * or that no common error is found.
 * @param rCurrentProcessInfo
 */
    int  UpdatedLagrangianMPMElement::Check( const ProcessInfo& rCurrentProcessInfo )
    {
        KRATOS_TRY
        
        unsigned int dimension = this->GetGeometry().WorkingSpaceDimension();
        
        //verify compatibility with the constitutive law
        ConstitutiveLaw::Features LawFeatures;
        
        this->GetProperties().GetValue( CONSTITUTIVE_LAW )->GetLawFeatures(LawFeatures);
        
        bool correct_strain_measure = false;
        
        for(unsigned int i=0; i<LawFeatures.mStrainMeasures.size(); i++)
        {
            if(LawFeatures.mStrainMeasures[i] == ConstitutiveLaw::StrainMeasure_Deformation_Gradient)
                correct_strain_measure = true;
        }
        
        if( correct_strain_measure == false )
            KRATOS_THROW_ERROR( std::logic_error, "constitutive law is not compatible with the element type ", " Large Displacements " )
          
        
        //verify that the variables are correctly initialized

        if ( VELOCITY.Key() == 0 )
            KRATOS_THROW_ERROR( std::invalid_argument, "VELOCITY has Key zero! (check if the application is correctly registered", "" )

        if ( DISPLACEMENT.Key() == 0 )
            KRATOS_THROW_ERROR( std::invalid_argument, "DISPLACEMENT has Key zero! (check if the application is correctly registered", "" )

        if ( ACCELERATION.Key() == 0 )
            KRATOS_THROW_ERROR( std::invalid_argument, "ACCELERATION has Key zero! (check if the application is correctly registered", "" )

        if ( DENSITY.Key() == 0 )
            KRATOS_THROW_ERROR( std::invalid_argument, "DENSITY has Key zero! (check if the application is correctly registered", "" )

        // if ( BODY_FORCE.Key() == 0 )
        //     KRATOS_THROW_ERROR( std::invalid_argument, "BODY_FORCE has Key zero! (check if the application is correctly registered", "" );

        //std::cout << " the variables have been correctly inizialized "<<std::endl;

        //verify that the dofs exist
        
        for ( unsigned int i = 0; i < this->GetGeometry().size(); i++ )
        {
            if ( this->GetGeometry()[i].SolutionStepsDataHas( DISPLACEMENT ) == false )
                KRATOS_THROW_ERROR( std::invalid_argument, "missing variable DISPLACEMENT on node ", this->GetGeometry()[i].Id() )

            if ( this->GetGeometry()[i].HasDofFor( DISPLACEMENT_X ) == false || this->GetGeometry()[i].HasDofFor( DISPLACEMENT_Y ) == false || this->GetGeometry()[i].HasDofFor( DISPLACEMENT_Z ) == false )
                
                KRATOS_THROW_ERROR( std::invalid_argument, "missing one of the dofs for the variable DISPLACEMENT on node ", GetGeometry()[i].Id() )
        }

        //verify that the constitutive law exists
        if ( this->GetProperties().Has( CONSTITUTIVE_LAW ) == false )
        {
            KRATOS_THROW_ERROR( std::logic_error, "constitutive law not provided for property ", this->GetProperties().Id() )
        }

        

        //verify that the constitutive law has the correct dimension
        if ( dimension == 2 )
        {
            
            if ( THICKNESS.Key() == 0 )
                KRATOS_THROW_ERROR( std::invalid_argument, "THICKNESS has Key zero! (check if the application is correctly registered", "" )

            if ( this->GetProperties().Has( THICKNESS ) == false )
                KRATOS_THROW_ERROR( std::logic_error, "THICKNESS not provided for element ", this->Id() )
        }
        else
        {
            if ( this->GetProperties().GetValue( CONSTITUTIVE_LAW )->GetStrainSize() != 6 )
                KRATOS_THROW_ERROR( std::logic_error, "wrong constitutive law used. This is a 3D element! expected strain size is 6 (el id = ) ", this->Id() )
        }

        //check constitutive law
        
        if (mConstitutiveLawVector!= 0)
        {
        return mConstitutiveLawVector->Check( GetProperties(), GetGeometry(), rCurrentProcessInfo );
        }
       
        return 0;

        KRATOS_CATCH( "" );
    }

    void UpdatedLagrangianMPMElement::save( Serializer& rSerializer ) const
    {
        KRATOS_SERIALIZE_SAVE_BASE_CLASS( rSerializer, Element )
        //int IntMethod = int(mThisIntegrationMethod);
        //rSerializer.save("IntegrationMethod",IntMethod);
        rSerializer.save("ConstitutiveLawVector",mConstitutiveLawVector);
        rSerializer.save("DeformationGradientF0",mDeformationGradientF0);
        rSerializer.save("DeterminantF0",mDeterminantF0);
        //rSerializer.save("InverseJ0",mInverseJ0);
        //rSerializer.save("DeterminantJ0",mDeterminantJ0);

    }

    void UpdatedLagrangianMPMElement::load( Serializer& rSerializer )
    {
        KRATOS_SERIALIZE_LOAD_BASE_CLASS( rSerializer, Element )
        rSerializer.load("ConstitutiveLawVector",mConstitutiveLawVector);
        rSerializer.load("DeformationGradientF0",mDeformationGradientF0);
        rSerializer.load("DeterminantF0",mDeterminantF0);
        //rSerializer.load("InverseJ0",mInverseJ0);
        //rSerializer.load("DeterminantJ0",mDeterminantJ0);
        

    }









} // Namespace Kratos

