from __future__ import print_function, absolute_import, division #makes KratosMultiphysics backward compatible with python 2.6 and 2.7

# Python imports
import time as timer
import os
import sys
import math

# Kratos
from KratosMultiphysics import *
from KratosMultiphysics.DEMApplication import *

# DEM Application
import DEM_explicit_solver_var as DEM_parameters

# Strategy object
import continuum_sphere_strategy as SolverStrategy

# Import MPI modules if needed
if os.environ.has_key("OMPI_COMM_WORLD_SIZE"):
    # Kratos MPI
    from KratosMultiphysics.MetisApplication import *
    from KratosMultiphysics.MPISearchApplication import *
    from KratosMultiphysics.mpi import *

    # DEM Application MPI
    import DEM_procedures_mpi as DEM_procedures
    import DEM_material_test_script_mpi as DEM_material_test_script
else :
    # DEM Application
    import DEM_procedures
    import DEM_material_test_script

    print("Runing under OpenMP")

##############################################################################
#                                                                            #
#    INITIALIZE                                                              #
#                                                                            #
##############################################################################

# Import utilities from moduels
procedures    = DEM_procedures.Procedures(DEM_parameters)
demio         = DEM_procedures.DEMIo()
report        = DEM_procedures.Report()
parallelutils = DEM_procedures.ParallelUtils()
materialTest  = DEM_procedures.MaterialTest()
 
# Set the print function TODO: do this better...
KRATOSprint   = procedures.KRATOSprint

# Preprocess the model
procedures.PreProcessModel(DEM_parameters)

# Prepare modelparts
balls_model_part      = ModelPart("SpheresPart");
rigid_face_model_part = ModelPart("RigidFace_Part");  
mixed_model_part      = ModelPart("Mixed_Part");
contact_model_part    = ""

# Add variables
procedures.AddCommonVariables(balls_model_part, DEM_parameters)
procedures.AddCommonVariables(rigid_face_model_part, DEM_parameters)
procedures.AddMpiVariables(balls_model_part)
procedures.AddMpiVariables(rigid_face_model_part)

# #~CHARLIE~#:????
SolverStrategy.AddVariables(balls_model_part, DEM_parameters)

# Reading the model_part
spheres_mp_filename   = DEM_parameters.problem_name + "DEM"
model_part_io_spheres = ModelPartIO(spheres_mp_filename,True)

# Perform the initial partition
[model_part_io_spheres, balls_model_part, MPICommSetup] = parallelutils.PerformInitialPartition(balls_model_part, model_part_io_spheres, spheres_mp_filename)

model_part_io_spheres.ReadModelPart(balls_model_part)

rigidFace_mp_filename = DEM_parameters.problem_name + "DEM_FEM_boundary"

model_part_io_fem = ModelPartIO(rigidFace_mp_filename)
model_part_io_fem.ReadModelPart(rigid_face_model_part)

# Setting up the buffer size
balls_model_part.SetBufferSize(1)

# Adding dofs
SolverStrategy.AddDofs(balls_model_part)

# Constructing a creator/destructor object
creator_destructor = ParticleCreatorDestructor()

# Creating necessary directories
main_path = os.getcwd()
[post_path,list_path,data_and_results,graphs_path,MPI_results] = procedures.CreateDirectories(str(main_path),str(DEM_parameters.problem_name))

os.chdir(list_path)

multifiles = (
    DEM_procedures.MultifileList(DEM_parameters.problem_name,1 ),
    DEM_procedures.MultifileList(DEM_parameters.problem_name,5 ),
    DEM_procedures.MultifileList(DEM_parameters.problem_name,10),
    DEM_procedures.MultifileList(DEM_parameters.problem_name,50),
    )

demio.SetMultifileLists(multifiles)

os.chdir(main_path)

KRATOSprint("Initializing Problem....")

# Initialize GiD-IO
demio.AddGlobalVariables()
demio.AddBallVariables()
demio.AddContactVariables()
demio.AddMpiVariables()
demio.EnableMpiVariables()

demio.Configure(DEM_parameters.problem_name,
                DEM_parameters.OutputFileType,
                DEM_parameters.Multifile,
                DEM_parameters.ContactMeshOption)

demio.SetOutputName(DEM_parameters.problem_name)

os.chdir(post_path)
demio.InitializeMesh(mixed_model_part,
                     balls_model_part,
                     rigid_face_model_part,
                     contact_model_part)

os.chdir(post_path)
#demio.PrintResults(2)

# Perform a partition to balance the problem
parallelutils.Repart(balls_model_part)
parallelutils.CalculateModelNewIds(balls_model_part)

os.chdir(post_path)
#demio.PrintResults(3)

# Creating a solver object and set the search strategy
solver                 = SolverStrategy.ExplicitStrategy(balls_model_part, rigid_face_model_part,creator_destructor, DEM_parameters)
solver.search_strategy = parallelutils.GetSearchStrategy(solver, balls_model_part)
solver.Initialize()

if ( DEM_parameters.ContactMeshOption =="ON" ) :
    contact_model_part = solver.contact_model_part
  
#------------------------------------------DEM_PROCEDURES FUNCTIONS & INITIALIZATIONS--------------------------------------------------------
#if (DEM_parameters.PredefinedSkinOption == "ON" ):
   #ProceduresSetPredefinedSkin(balls_model_part)

DEMFEMProcedures = DEM_procedures.DEMFEMProcedures(DEM_parameters, graphs_path, balls_model_part, rigid_face_model_part)

#Procedures.SetCustomSkin(balls_model_part)

materialTest.Initialize(DEM_parameters, procedures, solver, graphs_path, post_path, balls_model_part, rigid_face_model_part)

KRATOSprint("Initialization Complete" + "\n")

step           = 0
time           = 0.0
time_old_print = 0.0

report.Prepare(timer,DEM_parameters.ControlTime)

first_print  = True; index_5 = 1; index_10  = 1; index_50  = 1; control = 0.0
    
## MODEL DATA #~CHARLIE~#:????

if (DEM_parameters.ModelDataInfo == "ON"):
    os.chdir(data_and_results)
    if (DEM_parameters.ContactMeshOption == "ON"):
      (coordination_number) = procedures.ModelData(balls_model_part, contact_model_part, solver)       # calculates the mean number of neighbours the mean radius, etc..
      KRATOSprint ("Coordination Number: " + str(coordination_number) + "\n")
      os.chdir(main_path)
    else:
      KRATOSprint("Activate Contact Mesh for ModelData information")

if(DEM_parameters.Dempack):
#    if(mpi.rank == 0):
    #materialTest.PrintChart();
    materialTest.PrepareDataForGraph()
 
##############################################################################
#                                                                            #
#    MAIN LOOP                                                               #
#                                                                            #
##############################################################################

dt = balls_model_part.ProcessInfo.GetValue(DELTA_TIME) # Possible modifications of DELTA_TIME

report.total_steps_expected = int(DEM_parameters.FinalTime / dt)

KRATOSprint(report.BeginReport(timer))

mesh_motion = DEMFEMUtilities()

#if(DEM_parameters.PoissonMeasure == "ON"):
  #MaterialTest.PoissonMeasure()

a = 50
step = 0  
while ( time < DEM_parameters.FinalTime):

    dt   = balls_model_part.ProcessInfo.GetValue(DELTA_TIME) # Possible modifications of DELTA_TIME
    time = time + dt

    balls_model_part.ProcessInfo[TIME]            = time
    balls_model_part.ProcessInfo[DELTA_TIME]      = dt
    balls_model_part.ProcessInfo[TIME_STEPS]      = step
    
    rigid_face_model_part.ProcessInfo[TIME]       = time
    rigid_face_model_part.ProcessInfo[DELTA_TIME] = dt
    rigid_face_model_part.ProcessInfo[TIME_STEPS] = step

    #print("STEP:"+str(step)+"*******************************************")
    #print("*************************************************************")

    # Perform a partition to balance the problem
    if(not(step%(a-1))):
        parallelutils.Repart(balls_model_part)
        parallelutils.CalculateModelNewIds(balls_model_part)
    
    #walls movement:
    mesh_motion.MoveAllMeshes(rigid_face_model_part, time)
    
    #### SOLVE #########################################
    solver.Solve()
    
    #### TIME CONTROL ##################################
    stepinfo = report.StepiReport(timer,time,step)
    if stepinfo:
        KRATOSprint(stepinfo)

    #### CONCRETE TEST STUFF ############################
    materialTest.MeasureForcesAndPressure()
      
      #if(mpi.rank == 0):
        #MaterialTest.PrintGraph(step)

    #### GENERAL STUFF ###################################
    if( DEM_parameters.TestType == "None"):
        DEMFEMProcedures.MeasureForces()
        DEMFEMProcedures.PrintGraph(time)

    #### GiD IO ##########################################
    time_to_print = time - time_old_print

    if ( time_to_print >= DEM_parameters.OutputTimeStep):

        os.chdir(data_and_results)

        #properties_list = ProceduresMonitorPhysicalProperties(balls_model_part, physics_calculator, properties_list)

        os.chdir(list_path)
        demio.PrintMultifileLists(time)
        os.chdir(main_path)

        os.chdir(post_path)

        demio.PrintResults(mixed_model_part,balls_model_part,rigid_face_model_part,contact_model_part, time)
        
        if (DEM_parameters.ContactMeshOption == "ON"):
            solver.PrepareContactElementsForPrinting()
            demio.PrintingContactElementsVariables(contact_model_part, time)

        os.chdir(main_path)

        time_old_print = time
  
    step += 1

    #if((step%500) == 0):
      #if (( DEM_parameters.ContactMeshOption =="ON") and (DEM_parameters.TestType!= "None"))  :
          #MaterialTest.OrientationStudy(contact_model_part, step)
    

##############################################################################
#                                                                            #
#    FINALIZATION                                                            #
#                                                                            #
##############################################################################

demio.FinalizeMesh()
materialTest.FinalizeGraphs()

if((DEM_parameters.TestType == "None") and (mpi.rank == 0)):
    procedures.FinalizeGraphs()

demio.CloseMultifiles()

os.chdir(main_path)

# Print tmes and more info
KRATOSprint(report.FinalReport(timer))
