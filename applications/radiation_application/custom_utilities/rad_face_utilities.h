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
//   Last Modified by:    $Author: julio.marti $
//   Date:                $Date:  $
//   Revision:            $Revision: $
//
//


#if !defined(KRATOS_RAD_FACE_UTILITIES_INCLUDED )
#define  KRATOS_RAD_FACE_UTILITIES_INCLUDED



// System includes
#include <string>
#include <iostream>
#include <algorithm>

// External includes


// Project includes
#include "includes/define.h"
#include "includes/model_part.h"
#include "includes/node.h"
#include "utilities/geometry_utilities.h"
#include "geometries/tetrahedra_3d_4.h"
#include "radiation_application.h"




namespace Kratos
{
class RadFaceUtilities
{
public:

 
    void ConditionModelPart(ModelPart& temperature_model_part, ModelPart& full_model_part, const int TDim)
    {
        KRATOS_TRY;


        temperature_model_part.Conditions().clear();
        int nd=TDim;
        Properties::Pointer properties = full_model_part.GetMesh().pGetProperties(1);



        for(ModelPart::ElementsContainerType::iterator im = full_model_part.ElementsBegin() ; im != full_model_part.ElementsEnd() ; ++im)
        {
	   if(nd==2)
            {

                int n_int=im->GetGeometry()[0].FastGetSolutionStepValue(IS_BOUNDARY);
                for(int j=1; j<nd+1; j++) n_int+= im->GetGeometry()[j].FastGetSolutionStepValue(IS_BOUNDARY);
				/*
                if (n_int==3)
                {
                }
                else
                {


                    n_int=im->GetGeometry()[1].FastGetSolutionStepValue(IS_BOUNDARY) + im->GetGeometry()[2].FastGetSolutionStepValue(IS_BOUNDARY);

                    if(n_int==2)
                    {

                        Condition::NodesArrayType temp1;
                        temp1.reserve(2);
                        temp1.push_back(im->GetGeometry()(1));
                        temp1.push_back(im->GetGeometry()(2));
                        Geometry< Node<3> >::Pointer cond = Geometry< Node<3> >::Pointer(new Geometry< Node<3> >(temp1) );
                        int id = (im->Id()-1)*3;
                        Condition::Pointer p_cond(new RadFace2D(id, cond, properties));
                        temperature_model_part.Conditions().push_back(p_cond);

                    }

                    n_int= im->GetGeometry()[2].FastGetSolutionStepValue(IS_BOUNDARY) + im->GetGeometry()[0].FastGetSolutionStepValue(IS_BOUNDARY);


                    if(n_int==2)
                    {

                        Condition::NodesArrayType temp1;
                        temp1.reserve(2);
                        temp1.push_back(im->GetGeometry()(2));
                        temp1.push_back(im->GetGeometry()(0));
                        Geometry< Node<3> >::Pointer cond = Geometry< Node<3> >::Pointer(new Geometry< Node<3> >(temp1) );
                        int id = (im->Id()-1)*3+1;
                        Condition::Pointer p_cond(new RadFace2D(id, cond, properties));
                        temperature_model_part.Conditions().push_back(p_cond);
                    }


                    n_int= im->GetGeometry()[0].FastGetSolutionStepValue(IS_BOUNDARY) + im->GetGeometry()[1].FastGetSolutionStepValue(IS_BOUNDARY) ;

                    if(n_int==2)
                    {

                        Condition::NodesArrayType temp1;
                        temp1.reserve(2);
                        temp1.push_back(im->GetGeometry()(0));
                        temp1.push_back(im->GetGeometry()(1));
                        Geometry< Node<3> >::Pointer cond = Geometry< Node<3> >::Pointer(new Geometry< Node<3> >(temp1) );
                        int id = (im->Id()-1)*3+2;

                        Condition::Pointer p_cond(new RadFace2D(id, cond, properties));
                        temperature_model_part.Conditions().push_back(p_cond);

                    }

                }*/

            }

            else
            {


		KRATOS_WATCH(nd);
                int n_int=im->GetGeometry()[0].FastGetSolutionStepValue(IS_BOUNDARY);


                for(int j=1; j<nd+1; j++) n_int+= im->GetGeometry()[j].FastGetSolutionStepValue(IS_BOUNDARY);

                if (n_int==4)
                {

                }
                else
                {
                    n_int=0.0;
                    n_int=im->GetGeometry()[1].FastGetSolutionStepValue(IS_BOUNDARY);
                    n_int+=im->GetGeometry()[2].FastGetSolutionStepValue(IS_BOUNDARY);
                    n_int+=im->GetGeometry()[3].FastGetSolutionStepValue(IS_BOUNDARY);
                    if(n_int==3)
                    {

                        Condition::NodesArrayType temp;
                        temp.reserve(3);
                        temp.push_back(im->GetGeometry()(1));
                        temp.push_back(im->GetGeometry()(2));
                        temp.push_back(im->GetGeometry()(3));
                        Geometry< Node<3> >::Pointer cond = Geometry< Node<3> >::Pointer(new Triangle3D3< Node<3> >(temp) );
                        int id = (im->Id()-1)*4;
                        Condition::Pointer p_cond = Condition::Pointer(new RadFace3D(id, cond, properties) );
                        temperature_model_part.Conditions().push_back(p_cond);
                    }
                    n_int=0.0;
                    n_int=im->GetGeometry()[0].FastGetSolutionStepValue(IS_BOUNDARY);
                    n_int+=im->GetGeometry()[3].FastGetSolutionStepValue(IS_BOUNDARY);
                    n_int+=im->GetGeometry()[2].FastGetSolutionStepValue(IS_BOUNDARY);
                    if(n_int==3)
                    {

                        Condition::NodesArrayType temp;
                        temp.reserve(3);
                        temp.push_back(im->GetGeometry()(0));
                        temp.push_back(im->GetGeometry()(3));
                        temp.push_back(im->GetGeometry()(2));
                        Geometry< Node<3> >::Pointer cond = Geometry< Node<3> >::Pointer(new Triangle3D3< Node<3> >(temp) );
                        int id = (im->Id()-1)*4;
                        Condition::Pointer p_cond = Condition::Pointer(new RadFace3D(id, cond, properties) );
                        temperature_model_part.Conditions().push_back(p_cond);
                    }
                    n_int=0.0;
                    n_int=im->GetGeometry()[0].FastGetSolutionStepValue(IS_BOUNDARY);
                    n_int+=im->GetGeometry()[1].FastGetSolutionStepValue(IS_BOUNDARY);
                    n_int+=im->GetGeometry()[3].FastGetSolutionStepValue(IS_BOUNDARY);
                    if(n_int==3)
                    {

                        Condition::NodesArrayType temp;
                        temp.reserve(3);
                        temp.push_back(im->GetGeometry()(0));
                        temp.push_back(im->GetGeometry()(1));
                        temp.push_back(im->GetGeometry()(3));
                        Geometry< Node<3> >::Pointer cond = Geometry< Node<3> >::Pointer(new Triangle3D3< Node<3> >(temp) );
                        int id = (im->Id()-1)*4;
                        Condition::Pointer p_cond = Condition::Pointer(new RadFace3D(id, cond, properties) );
                        temperature_model_part.Conditions().push_back(p_cond);
                    }


                    n_int=0.0;
                    n_int=im->GetGeometry()[0].FastGetSolutionStepValue(IS_BOUNDARY);
                    n_int+=im->GetGeometry()[2].FastGetSolutionStepValue(IS_BOUNDARY);
                    n_int+=im->GetGeometry()[1].FastGetSolutionStepValue(IS_BOUNDARY);
                    if(n_int==3)
                    {

                        Condition::NodesArrayType temp;
                        temp.reserve(3);
                        temp.push_back(im->GetGeometry()(0));
                        temp.push_back(im->GetGeometry()(2));
                        temp.push_back(im->GetGeometry()(1));
                        Geometry< Node<3> >::Pointer cond = Geometry< Node<3> >::Pointer(new Triangle3D3< Node<3> >(temp) );
                        int id = (im->Id()-1)*4;
                        Condition::Pointer p_cond = Condition::Pointer(new RadFace3D(id, cond, properties) );
                        temperature_model_part.Conditions().push_back(p_cond);

                    }

                }

		 }
        }


        KRATOS_CATCH("");
    }


private:

};

}  // namespace Kratos.

#endif // KRATOS_FACE_HEAT_UTILITIES_INCLUDED  defined 


