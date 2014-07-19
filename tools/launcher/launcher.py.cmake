#! /usr/bin/env python

import sys
import operator
import optparse
import os
import shutil
import fnmatch
import pipes
import subprocess
import operator
import re
import stat
import datetime
import time

def prod(factors):
  return reduce(operator.mul, factors)

def parseShape(str, extendLast=False, extendDefault=False):
  str = str.strip()
  result = []
  if not str:
    return result
  for l in str.split('x'):
    result.append(int(l))
  if extendLast:
    while len(result) < 4:
      result.append(result[-1])
  if not extendDefault is False:
    while len(result) < 4:
      result.append(extendDefault)
  return tuple(x for x in result) # Convert to tuple


def isShapeSubset(subset, superset):
  ssubset = sorted(subset)
  ssuperset = sorted(superset)
  for l in range(len(ssubset)):
    if ssubset[l] > ssuperset[l]:
      return False
  return True


def isShapeMatch(subset,superset):
  ssubset = sorted(subset)
  ssuperset = sorted(superset)
  for l in range(len(ssubset)):
    if ssubset[l] != ssuperset[l]:
      return False
  return True
  
# https://stackoverflow.com/questions/35817/how-to-escape-os-system-calls-in-python
def shellquote(s):
    return "'" + s.replace("'", "'\\''") + "'"
 

# Environment (example)
# LOADL_STEP_CLASS=n001
# LOADL_STEP_NUMBER=0
# LOADL_STEP_NICE=0
# LOADL_BG_IOLINKS=R63-M0-N05-J06,R63-M0-N05-J11
# LOADL_STEP_ENERGY_POLICY_TAG=
# LOADL_PROCESSOR_LIST=.0 Task  
# LOADL_STEP_NAME=0
# LOADL_STATUS_LEVEL=MACHINE
# LOADL_ACTIVE=5.1.0.18
# LOADL_STEP_ID=juqueen3c1.zam.kfa-juelich.de.49212.0
# LOADL_STEP_TYPE=BLUE GENE
# LOADL_STEP_OUT=/work/hch02/hch02d/jobs/406_bgqbench_v32x32x32x32_32n2x2x2x2x2r1t64/qout49212.txt
# LOADL_BG_BLOCK=LL14071216212452
# LOADL_STEP_OWNER=hch02d
# LOADL_STEP_ARGS=
# LOADL_STEP_GROUP=hch02
# LOADL_COMM_DIR=/tmp
# LOADL_STEP_ERR=/work/hch02/hch02d/jobs/406_bgqbench_v32x32x32x32_32n2x2x2x2x2r1t64/qout49212.txt
# LOADL_BG_MPS=R63-M0
# LOADL_STARTD_PORT=9611
# LOADL_BG_CONNECTIVITY=
# LOADL_BG_SHAPE=2x2x2x2x2
# LOADL_STEP_COMMAND=/work/hch02/hch02d/jobs/406_bgqbench_v32x32x32x32_32n2x2x2x2x2r1t64/job.ll
# LOADL_JOB_NAME=406_bgqbench_v32x32x32x32_32n2x2x2x2x2r1t64
# LOADL_STEP_IN=/dev/null
# LOADL_PID=6591


# CMake parameters
SRCDIR="""@TMLQCD_SOURCE_DIR@"""
BINDIR="""@TMLQCD_BINARY_DIR@"""

# Directory with all the working directories of jobs
jobsdir = "/work/hch02/hch02d/jobs"


# Takes the batch file in the argument and executes it
jobidre = re.compile(r'^\#\s*jobid\s*=\s*(?P<jobid>.+)$')
errfilere = re.compile(r'^\#\@\s*error\s*=\s*(?P<error>.*)$')
outfilere = re.compile(r'^\#\@\s*output\s*=\s*(?P<output>.*)$')
def execjobcommand(filepath, cancel=True):
  if not os.path.isfile(filepath):
    return

  jobid = None
  errfile = None
  outfile = None
  with open(filepath, 'r') as f:
    for line in f:
      m = jobidre.match(line)
      if m:
        jobid = m.group('jobid')
    
      m = errfilere.match(line)
      if m:
        errfile = m.group('error')
        
      m = outfilere.match(line)
      if m:
        outfile = m.group('output')
  
  # rename job so no one else is grabbing it
  fdir,fname = os.path.split(filepath)
  newpath = os.path.join(fdir, 'launch_{jobid}.sh'.format(jobid=os.environ['LOADL_STEP_ID']))
  os.rename(filepath,newpath)
  
  if not os.path.isfile(newpath):
    print "Someone else was faster than us for",filepath
    sys.stdout.flush()
    return # Someone else grabbed the job to execute
  
  # Cancel old job (if existing and not this job)
  if cancel and (jobid is not None):
    print "Canceling old job ", jobid
    sys.stdout.flush()
    os.system('llcancel ' + jobid)
    sys.stdout.flush()
  
  # Launch job
  cmdline = 'cd ' + shellquote(fdir) + ' && bash ' + shellquote(newpath)
  if outfile:
    outfile = os.path.expanduser(outfile)
    outfile = os.path.expandvars(outfile)
    cmdline = cmdline + ' >' + shellquote(outfile)
  if errfile:
    errfile = os.path.expanduser(errfile)
    errfile = os.path.expandvars(errfile)
    if outfile and outfile==errfile:
      cmdline = cmdline + ' 2>&1'
    else:
      cmdline = cmdline + ' 2>' + shellquote(errfile)
  print "Executing ", cmdline
  sys.stdout.flush()
  os.system(cmdline)
  print "Execution returned"  
  sys.stdout.flush()

  
jobdescre = re.compile(r'^job\_(?P<nodes>\d+)\_(?P<shape>[x\d]+)(\_\d*)?\.sh$');
def main():
  starttime = datetime.datetime.utcnow()

  parser = optparse.OptionParser()
  parser.add_option('--jobscript', help="Batch file to be executed on the cluster")
  (options, args) = parser.parse_args()

  if options.jobscript:
    mainjobscript = os.path.abspath(options.jobscript)
    execjobcommand(mainjobscript, False)

  shape = parseShape(os.environ['LOADL_BG_SHAPE'])
  nodes = prod(shape)

  while True:
    jobdirsname = os.listdir(jobsdir)
    jobdirs = [os.path.abspath(os.path.join(jobsdir,item)) for item in jobdirsname]
    jobdirs.append(jobsdir) # Add root dir itself we may put files in there
    for jobdir in jobdirs:
      for filename in os.listdir(jobdir):
        m = jobdescre.match(filename)
        if not m:
          continue
        abspath = os.path.join(jobdir,filename)
        if not os.path.isfile(abspath):
          continue

        print "Matched filename ",filename
        sys.stdout.flush()
        jobshape = m.group('shape')
        jobnodes = m.group('nodes')

        jobshape = parseShape(jobshape)
        jobnodes = int(jobnodes)
        if jobnodes > nodes:
          continue
        duration = datetime.datetime.utcnow() - starttime
        if nodes <= 128 and duration <= datetime.timedelta(minutes=29):
          if not isShapeSubset(jobshape,shape):        
            print "Rejecting job reason A"
            sys.stdout.flush()
            continue
        else:
          if not isShapeMatch(jobshape,shape):
            print "Rejecting job reason B"
            sys.stdout.flush()
            continue
        execjobcommand(abspath)
    
    duration = datetime.datetime.utcnow() - starttime
    print "duration: ",duration
    sys.stdout.flush()
    if duration > datetime.timedelta(minutes=29): # Each job costs at least 30min
      break # do not run longer than necessary; it costs compute time   
    time.sleep(1)
        
if __name__ == "__main__":
  main()


