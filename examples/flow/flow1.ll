#@ job_name = {jobname}
#@ comment = "tmLQCD bgqbench"
#@ error  = {outputfilepath}
#@ output = {outputfilepath}
#@ environment = COPY_ALL;
#@ wall_clock_limit = 0:30:00
#@ notification = error
#@ notify_user = juqueen@meinersbur.de
#@ job_type = bluegene
#@ bg_size = 32
#@ bg_connectivity = TORUS
#@ queue
 
echo "START" : `date +"%Y-%m-%d %H:%M:%S.%N%z (%s.%N)"`
echo "CURDIR" : `pwd`

export OMP_NUM_THREADS=1
export BG_COREDUMPDISABLED=0
PROCS=0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,2,6,10,14,18,22,26,30,34,38,42,46,50,54,58,62,1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,3,7,11,15,19,23,27,31,35,39,43,47,51,55,59,63
export XLSMPOPTS="procs=$PROCS"
MAPPING="--mapping ABCDET"

# For Scalasca; Because of "ESD buffer full - definitions being lost!" messages
export ESD_BUFFER_SIZE=200000

runjob -n 2 --ranks-per-node 32 --env-all $MAPPING : <file:flow_L8x8_P2x1_B4x8-mollycc-a>

echo "END  " : `date +"%Y-%m-%d %H:%M:%S.%N%z (%s.%N)"`
