/*
==============================================================================
KratosConvectionDiffusionApplication 
A library based on:
Kratos
A General Purpose Software for Multi-Physics Finite Element Analysis
Version 1.0 (Released on march 05, 2007).

Copyright 2007
Pooyan Dadvand, Riccardo Rossi
pooyan@cimne.upc.edu 
rrossi@cimne.upc.edu
- CIMNE (International Center for Numerical Methods in Engineering),
Gran Capita' s/n, 08034 Barcelona, Spain


Permission is hereby granted, free  of charge, to any person obtaining
a  copy  of this  software  and  associated  documentation files  (the
"Software"), to  deal in  the Software without  restriction, including
without limitation  the rights to  use, copy, modify,  merge, publish,
distribute,  sublicense and/or  sell copies  of the  Software,  and to
permit persons to whom the Software  is furnished to do so, subject to
the following condition:

Distribution of this code for  any  commercial purpose  is permissible
ONLY BY DIRECT ARRANGEMENT WITH THE COPYRIGHT OWNERS.

The  above  copyright  notice  and  this permission  notice  shall  be
included in all copies or substantial portions of the Software.

THE  SOFTWARE IS  PROVIDED  "AS  IS", WITHOUT  WARRANTY  OF ANY  KIND,
EXPRESS OR  IMPLIED, INCLUDING  BUT NOT LIMITED  TO THE  WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT  SHALL THE AUTHORS OR COPYRIGHT HOLDERS  BE LIABLE FOR ANY
CLAIM, DAMAGES OR  OTHER LIABILITY, WHETHER IN AN  ACTION OF CONTRACT,
TORT  OR OTHERWISE, ARISING  FROM, OUT  OF OR  IN CONNECTION  WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

==============================================================================
*/ 
//   
//   Project Name:        Kratos       
//   Last modified by:    $Author: julio.marti $
//   Date:                $Date:  $
//   Revision:            $Revision:  $
//
// 


// System includes 


// External includes 


// Project includes 
#include "includes/define.h"
#include "custom_conditions/rad_face2D.h"
#include "utilities/math_utils.h"
#include "radiation_application.h"

namespace Kratos
{
	//************************************************************************************
	//************************************************************************************
  RadFace2D::RadFace2D(IndexType NewId, GeometryType::Pointer pGeometry)
    : Condition(NewId, pGeometry)
  {		
    //DO NOT ADD DOFS HERE!!!
  }
  
  //************************************************************************************
  //************************************************************************************
  RadFace2D::RadFace2D(IndexType NewId, GeometryType::Pointer pGeometry,  PropertiesType::Pointer pProperties)
    : Condition(NewId, pGeometry, pProperties)
  {
  }
  
  Condition::Pointer RadFace2D::Create(IndexType NewId, NodesArrayType const& ThisNodes,  PropertiesType::Pointer pProperties) const
  {
    return Condition::Pointer(new RadFace2D(NewId, GetGeometry().Create(ThisNodes), pProperties));
  }
  
  RadFace2D::~RadFace2D()
  {
  }
  
  
  //************************************************************************************
  //************************************************************************************
  void RadFace2D::CalculateRightHandSide(VectorType& rRightHandSideVector, ProcessInfo& rCurrentProcessInfo)
	{
	  //calculation flags
	  bool CalculateStiffnessMatrixFlag = false;
	  bool CalculateResidualVectorFlag = true;
	  MatrixType temp = Matrix();
	  
	  CalculateAll(temp, rRightHandSideVector, rCurrentProcessInfo, CalculateStiffnessMatrixFlag, CalculateResidualVectorFlag);
	}
  
  //************************************************************************************
  //************************************************************************************
  void RadFace2D::CalculateLocalSystem(MatrixType& rLeftHandSideMatrix, VectorType& rRightHandSideVector, ProcessInfo& rCurrentProcessInfo)
  {
    //calculation flags
    bool CalculateStiffnessMatrixFlag = true;
    bool CalculateResidualVectorFlag = true;
    
    CalculateAll(rLeftHandSideMatrix, rRightHandSideVector, rCurrentProcessInfo, CalculateStiffnessMatrixFlag, CalculateResidualVectorFlag);
  }
  
  //************************************************************************************
  //************************************************************************************
  void RadFace2D::CalculateAll(MatrixType& rLeftHandSideMatrix, VectorType& rRightHandSideVector, ProcessInfo& rCurrentProcessInfo, bool CalculateStiffnessMatrixFlag,bool CalculateResidualVectorFlag)
  {
    KRATOS_TRY

      /*	KRATOS_WATCH("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
		KRATOS_WATCH("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
		KRATOS_WATCH("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
		KRATOS_ERROR(std::logic_error,  "method not implemented" , "");*/
      
      unsigned int number_of_nodes = GetGeometry().size();
				/*std::cout << "temperature_model_part.Conditions()" << std::endl;
				  std::cout << "temperature_model_part.Conditions()" << std::endl;
				  std::cout << "temperature_model_part.Conditions()" << std::endl;
				  std::cout << "temperature_model_part.Conditions()" << std::endl;*/
    //resizing as needed the LHS
    unsigned int MatSize=number_of_nodes;
    
    //calculate lenght
    double x21 = GetGeometry()[1].X() - GetGeometry()[0].X();
    double y21 = GetGeometry()[1].Y() - GetGeometry()[0].Y();
    double w=0.0;
    if(GetGeometry()[0].Y()<0.000001 && (GetGeometry()[0].X()>0.000001 && GetGeometry()[0].X()<0.999999999)) w+=1.0;
    
    if(GetGeometry()[0].Y()>0.999999 && (GetGeometry()[0].X()>0.000001 && GetGeometry()[0].X()<0.999999999)) w+=1.0;
    double lenght = x21*x21 + y21*y21;
    lenght = sqrt(lenght);
    double coef=0.0;
    if(w==2.0) coef=0.0;
    else coef=1.0;
    
    const Kratos::Condition::PropertiesType pproperties = GetProperties();
    //const double& 
    double ambient_temperature = pproperties[AMBIENT_TEMPERATURE];
    
    double StefenBoltzmann = 5.67e-8;
    double emissivity = pproperties[EMISSIVITY];
    emissivity = 1.0;
    double convection_coefficient = pproperties[CONVECTION_COEFFICIENT];
    
    const double& T0 = GetGeometry()[0].FastGetSolutionStepValue(TEMPERATURE);
    const double& T1 = GetGeometry()[1].FastGetSolutionStepValue(TEMPERATURE);
    
    const double& I0 = GetGeometry()[0].FastGetSolutionStepValue(INCIDENT_RADIATION_FUNCTION);
    const double& I1 = GetGeometry()[1].FastGetSolutionStepValue(INCIDENT_RADIATION_FUNCTION);
    
    emissivity=1.0;
    
    if (CalculateStiffnessMatrixFlag == true) //calculation of the matrix is required
      {
	if(rLeftHandSideMatrix.size1() != MatSize )
	  rLeftHandSideMatrix.resize(MatSize,MatSize,false);
	noalias(rLeftHandSideMatrix) = ZeroMatrix(MatSize,MatSize);	
	
	rLeftHandSideMatrix(0,0) = 1.0 * ( emissivity/(4.0-2.0*emissivity))* 0.5 * lenght; 
	rLeftHandSideMatrix(1,1) = 1.0 * ( emissivity/(4.0-2.0*emissivity))* 0.5 * lenght;
      }
    
    //resizing as needed the RHS
    double aux = pow(ambient_temperature,4);
    if (CalculateResidualVectorFlag == true) //calculation of the matrix is required
      {
	if(rRightHandSideVector.size() != MatSize )
	  rRightHandSideVector.resize(MatSize,false);
	
	rRightHandSideVector[0] = 1.0 * (emissivity*StefenBoltzmann*4.0 * pow(T0,4)) /(4.0-2.0*emissivity)- ( emissivity * I0/(4.0-2.0*emissivity)); 

	rRightHandSideVector[1] = 1.0 * (emissivity*StefenBoltzmann*4.0 * pow(T0,4)) /(4.0-2.0*emissivity)- ( emissivity * I1/(4.0-2.0*emissivity)); 
	
	rRightHandSideVector *= 0.5*lenght;
	//
      }

    KRATOS_CATCH("")
      }
  
  
  
  //************************************************************************************
  //************************************************************************************
  void RadFace2D::EquationIdVector(EquationIdVectorType& rResult, ProcessInfo& CurrentProcessInfo)
  {
    unsigned int number_of_nodes = GetGeometry().PointsNumber();
    if(rResult.size() != number_of_nodes)
      rResult.resize(number_of_nodes,false);
    for (unsigned int i=0;i<number_of_nodes;i++)
      {
	rResult[i] = (GetGeometry()[i].GetDof(INCIDENT_RADIATION_FUNCTION)).EquationId();
      }
  }
  
  //************************************************************************************
  //************************************************************************************
  void RadFace2D::GetDofList(DofsVectorType& ConditionalDofList,ProcessInfo& CurrentProcessInfo)
  {
    ConditionalDofList.resize(GetGeometry().size());
    for (unsigned int i=0;i<GetGeometry().size();i++)
      {
		    ConditionalDofList[i] = (GetGeometry()[i].pGetDof(INCIDENT_RADIATION_FUNCTION));
      }
  }
  
} // Namespace Kratos


