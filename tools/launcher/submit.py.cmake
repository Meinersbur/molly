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
import math  # Use sqrt, floor
import time
import datetime


def gcd(a,b):
  """gcd(a,b) returns the greatest common divisor of the integers a and b."""
  if a == 0:
    return b
  return abs(gcd(b % a, a))
    
def isprime(n):
  """isprime(n) - Test whether n is prime using a variety of pseudoprime tests."""
  if (n in [2,3,5,7,11,13,17,19,23,29]): return True
  return isprimeE(n,2) and isprimeE(n,3) and isprimeE(n,5)
      
def isprimeF(n,b):
  """isprimeF(n) - Test whether n is prime or a Fermat pseudoprime to base b."""
  return (pow(b,n-1,n) == 1)
  
def isprimeE(n,b):
  """isprimeE(n) - Test whether n is prime or an Euler pseudoprime to base b."""
  if (not isprimeF(n,b)): return False
  r = n-1
  while (r % 2 == 0): r /= 2
  c = pow(b,r,n)
  if (c == 1): return True
  while (1):
    if (c == 1): return False
    if (c == n-1): return True
    c = pow(c,2,n)
  
def factor(n):
  """factor(n) - Find a prime factor of n using a variety of methods."""
  if (isprime(n)): return n
  for fact in [2,3,5,7,11,13,17,19,23,29]:
    if n%fact == 0: return fact
  return factorPR(n)  # Needs work - no guarantee that a prime factor will be returned
      
def factors(n):
  """factors(n) - Return a sorted list of the prime factors of n."""
  if n==1:
    return []
  if (isprime(n)):
    return [n]
  fact = factor(n)
  if (fact == 1): return "Unable to factor "+str(n)
  facts = factors(n/fact) + factors(fact)
  facts.sort()
  return facts

def factorPR(n):
  """factorPR(n) - Find a factor of n using the Pollard Rho method.
  Note: This method will occasionally fail."""
  for slow in [2,3,4,6]:
    numsteps=2*math.floor(math.sqrt(math.sqrt(n))); fast=slow; i=1
    while i<numsteps:
      slow = (slow*slow + 1) % n
      i = i + 1
      fast = (fast*fast + 1) % n
      fast = (fast*fast + 1) % n
      g = gcd(fast-slow,n)
      if (g != 1):
        if (g == n):
          break
        else:
          return g
  return 1


def prod(factors):
  return reduce(operator.mul, factors)




ON=True
OFF=False

executable_bgqbench='''@EXECUTABLE_BGQBENCH@'''
executable_benchmark='''@EXECUTABLE_BENCHMARK@'''
executable_invert='''@EXECUTABLE_INVERT@'''
executable_hmc_tm='''@EXECUTABLE_HMC_TM@'''
executable_ctest='''@EXECUTABLE_CTEST@'''
scalasca_program='''@SCALASCA_PROGRAM@'''
git_executable='''@GIT_EXECUTABLE@'''

scalasca_enabled = @SCALASCA_ENABLED@
SRCDIR='''@TMLQCD_SOURCE_DIR@'''
BINDIR='''@TMLQCD_BINARY_DIR@'''


def getExecutable():
  global program
  if program=='bgqbench':
    return '''@EXECUTABLE_BGQBENCH@'''
  elif program=='benchmark':
    return '''@EXECUTABLE_BENCHMARK@'''
  elif program=='invert':
    return '''@EXECUTABLE_INVERT@'''
  elif program=='hmc_tm':
    return '''@EXECUTABLE_HMC_TM@'''
  elif program=='ctest':
    return '''@EXECUTABLE_CTEST@'''


inputtemplatefilepath=None
def getInputTemplatePath():
  global program,inputtemplatefilepath
  if inputtemplatefilepath:
    return inputtemplatefilepath
  if program=='bgqbench':
    return os.path.join(SRCDIR, 'benchmark.input.template')
  elif program=='benchmark':
    return os.path.join(SRCDIR, 'benchmark.input.template')
  elif program=='invert':
    return os.path.join(SRCDIR, 'invert.input.template')
  elif program=='hmc_tm':
    return os.path.join(SRCDIR, 'hmc.input.template')
  return None


def isBench():
  global program
  return program=='bgqbench' or program=='benchmark'

def isInvert():
  global program
  return program=='invert'

def isHmcTm():
  global program
  return program=='hmc_tm'


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


def shapeToStr(shape):
  if not shape:
    return shape
  return 'x'.join([str(v) for v in shape])


def parseDuration(str):
  str = str.strip()
  m = re.match(r"(?P<hours>\d+)h", str)
  if m:
    return datetime.timedelta(hours=int(m.group('hours')))

  comps = tuple(int(x) for x in str.split(':'))
  if len(comps)==2:
    result = datetime.timedelta(minutes=comps[0],seconds=comps[1])
  elif len(comps)==3:
    result = datetime.timedelta(hours=comps[0],minutes=comps[1],seconds=comps[2])
  else:
    raise Exception("Wrong time format")
  return result


def makestrDuration(td):
  secs = (td.microseconds + (td.seconds + td.days * 24 * 3600) * 10**6) / 10**6
  if secs>=60*60:
    result = "{h}:{m:02d}:{s:02d}".format(h=secs//(60*60),m=secs//60 % 60,s=secs%60)
  else:
    result = "{m:02d}:{s:02d}".format(m=secs//60,s=secs%60)
  return result


def check_output(*popenargs, **kwargs):
    process = subprocess.Popen(stdout=subprocess.PIPE, *popenargs, **kwargs)
    output, unused_err = process.communicate()
    retcode = process.poll()
    if retcode:
        cmd = kwargs.get("args")
        if cmd is None:
            cmd = popenargs[0]
        error = subprocess.CalledProcessError(retcode, cmd)
        error.output = output
        raise error
    return output


try: subprocess.check_output
except: subprocess.check_output = check_output

jobidre = re.compile(r'^llsubmit\: The job \"(?P<jobid>[^\"]+)\" has been submitted.', re.MULTILINE)

def submit():
  global jobdir,jobid

  # Wait a bit; maybe some launcher picks up the job so we don't have to create a new job
  time.sleep(2)
  if not os.path.isfile(jobscriptfile):
    print "Not submitting job as it was taken by another launcher"
    return

  out = subprocess.check_output(['llsubmit', os.path.join(jobdir,'job.ll')], cwd=jobdir)
  m = jobidre.search(out)
  if m:
    jobid = m.group('jobid')
    buildJobScript() # Update jobscript to include the jobid - so the launcher script can cancel the job
    print "Submitted as ", jobid
  else:
    print "COULD NOT EXTRACT JOBID FROM OUTPUT: ", out
  

def genJobname(i):
  global volume,volume_per_rank,threads,ranks,nodes,midplanes,ranks_per_node,threads_per_rank,threads_per_node,shape_in_midplanes,shape_in_nodes,shape_in_ranks,timelimit
  global program

  prefix = "{i:03d}_{program}".format(i=i,program=program)
  suffix = "".format(i=i,program=program)

  if volume:
    prefix = prefix + "_v" + "x".join([str(d) for d in volume])

  if nodes and shape_in_nodes:
    suffix = suffix + "_{nodes}n{shape}".format(nodes=nodes,shape="x".join([str(d) for d in shape_in_nodes]))
  elif nodes:
    suffix = suffix + "_{nodes}n".format(nodes=nodes)  
  elif shape_in_nodes:
    suffix = suffix + "_n" + "x".join([str(d) for d in shape_in_nodes])

  if ranks_per_node:
    suffix = suffix + "r{ranks_per_node}".format(ranks_per_node=ranks_per_node) 

  if threads_per_rank:
    suffix = suffix + "t{threads_per_rank}".format(threads_per_rank=threads_per_rank)

  return prefix + suffix


def findjobdir(jobsdir):
  largest = -1
  list = [os.path.abspath(os.path.join(jobsdir,item)) for item in os.listdir(jobsdir)]
  for dir in list:
    bname = os.path.basename(dir)
    srch = re.search(r'\d+', bname);
    if not srch:
      continue
    nstr = srch.group()
    if not nstr:
      continue
    try:
      num = int(nstr)
      largest = max(largest,num)
    except ValueError:
      pass
  jobname = genJobname(largest+1)
  return os.path.join(jobsdir,jobname),jobname


def readCompleteFile(filepath):
  with open(filepath, 'r') as content_file:
    return content_file.read()


def writeCompleteFile(filepath, content, setExecutableBit=False):
  with open(filepath, 'w') as content_file:
    content_file.write(content)
    if setExecutableBit:
      os.fchmod(content_file.fileno(), stat.S_IRWXU | stat.S_IRWXG | stat.S_IROTH | stat.S_IXOTH)


def configureFile(template_path, target_path):
  global volume,volume_per_rank,threads,ranks,nodes,midplanes,ranks_per_node,threads_per_rank,threads_per_node,shape_in_midplanes,shape_in_nodes,shape_in_ranks,timelimit,jobid,jobscriptfile
  global jobname,executable,jobdir,program
  global scalasca_enabled,inputfilepath

  shape_machine_str = ''
  if shape_in_midplanes is not None:
    shape_machine_str = 'bg_shape = ' + string.join([str(l) for l in shape_machine],'x')
  elif nodes is not None:
    shape_machine_str = 'bg_size = ' + str(nodes)

  prep=''
  if scalasca_enabled:
    prep = '''@SCALASCA_PROGRAM@ -analyze '''

  launcheroutputfilepath = os.path.join(jobdir,'yout.txt')
  if jobid:
    launcheroutputfilepath = os.path.join(jobdir,'yout' + jobid + '.txt')

  template = readCompleteFile(template_path)
  formatdict = dict(
    RanksT=shape_in_ranks[0],RanksX=shape_in_ranks[1],RanksY=shape_in_ranks[2],RanksZ=shape_in_ranks[3],
    timelimit=makestrDuration(timelimit),
    jobname=jobname,
    outputfilepath=os.path.join(jobdir,'qout$(jobid).txt'),
    launcheroutputfilepath=launcheroutputfilepath,
    shape_machine_str=shape_machine_str,
    ranks_per_node=str(ranks_per_node),
    threads_per_rank=str(threads_per_rank),
    inputfilepath=inputfilepath,
    executable=executable,
    mpiexec='''@MPIEXEC@''',
    jobscript=jobscriptfile,
    launchercmd=os.path.join(BINDIR,'bin/launcher.py'),
    jobid=jobid,
    prep=prep)
  if program!='ctest':
    formatdict.update(dict(LT=volume[0],LX=volume[1],LY=volume[2],LZ=volume[3]))
  content = template.format(**formatdict)
  writeCompleteFile(target_path, content)


def buildInput():
  global BINDIR,jobdir,inputfilepath,SRCDIR
  if isBench():
    inputfilepath = os.path.join(jobdir, 'benchmark.input')
  elif isInvert():
    inputfilepath = os.path.join(jobdir, 'invert.input')
  elif isHmcTm():
    inputfilepath = os.path.join(jobdir, 'hmc.input')
  else:
    inputfilepath = os.path.join(jobdir, 'input')
  templatepath = getInputTemplatePath()
  if templatepath:
    configureFile(templatepath, inputfilepath)
  else:
    templatepath = None


def buildLLScript():
  global SRCDIR,jobdir
  configureFile(os.path.join(SRCDIR, 'job.ll.template'), os.path.join(jobdir, 'job.ll'))


def buildJobScript():
  global SRCDIR,jobdir,jobscriptfile
  if not jobscriptfile:
    jobscriptfilename = 'job_{nodes}_{shape}.sh'.format(nodes=nodes,shape=shapeToStr(shape_in_nodes))
    jobscriptfile = os.path.join(jobdir, jobscriptfilename)
  configureFile(os.path.join(SRCDIR, 'job.sh.template'), jobscriptfile)


def copyExecutable():
  global jobdir, program, executable,BINDIR,SRCDIR
  srcexecutable = getExecutable()
  executable = os.path.join(jobdir, os.path.basename(srcexecutable))
  shutil.copy2(srcexecutable, executable)
  #shutil.copy2(os.path.join(SRCDIR,'corestack.py'), os.path.join(jobdir,'corestack.py'))


def makeProgram():
  global program,BINDIR
  subprocess.check_call(['''@CMAKE_COMMAND@''','--build',BINDIR,'--target',program,'--','-j32'])


def copySource():
  global SRCDIR,BINDIR
  global jobdir
  global git_executable

  srctargetdir = os.path.join(jobdir,'src')
  shutil.copy2(os.path.join(BINDIR,'CMakeCache.txt'), os.path.join(jobdir,'CMakeCache.txt'))

  if git_executable:
    difffilepath = os.path.join(jobdir,'src.diff')
    with open(difffilepath, 'w') as diff_file:
      subprocess.check_call([git_executable,'diff','--patch-with-stat','--src-prefix=@GIT_SHA1@/','--dst-prefix=workingset/','HEAD'], cwd=SRCDIR, stdout=diff_file)

    rematerialize_script = """#! /bin/sh
{git} clone "@TMLQCD_SOURCE_DIR@" "{srctargetdir}"
cd "{srctargetdir}"
{git} checkout @GIT_SHA1@ # @GIT_REFSPEC@
{git} apply --whitespace=nowarn "{difffilepath}"
""".format(jobdir=jobdir,difffilepath=difffilepath,git=git_executable,srctargetdir=srctargetdir)
    writeCompleteFile(os.path.join(jobdir,'getsrc.sh'), rematerialize_script, setExecutableBit=True)
  else:
    shutil.copytree(SRCDIR, srctargetdir, symlinks=True)


def prepare():
  global jobdir,jobname,programupdate
  jobdir,jobname = findjobdir('/work/hch02/hch02d/jobs')

  if programupdate:
    print "Compiling",program,"..."
    makeProgram()
    
  print "Create job directory..."
  os.makedirs(jobdir)
  print "Copying executable..."
  copyExecutable()
  print "Copying sources..."
  copySource()
  print "Configure input..."
  buildInput()
  print "Configure job and LL script..."
  buildJobScript()
  buildLLScript()
  print "Job sucessfully prepared at ",jobdir



volume=None
volume_per_rank=None
threads=None
ranks=None
nodes=None
midplanes=None
ranks_per_node=None
threads_per_rank=None
threads_per_node=None
shape_in_midplanes=None
shape_in_nodes=None
shape_in_ranks=None
timelimit=None
dims=None
logical_shape=None

jobid = None
jobscriptfile = None

def checkCondition(cond):
  global volume,volume_per_rank,threads,ranks,nodes,midplanes,ranks_per_node,threads_per_rank,threads_per_node,shape_in_midplanes,shape_in_nodes,shape_in_ranks,timelimit

  if not cond:
    print "Parameter consistency check failed!"
    print "volume = ",shapeToStr(volume)
    print "volume_per_rank = ",shapeToStr(volume_per_rank)
    print "threads =",threads
    print "ranks = ",ranks
    print "nodes = ",nodes
    print "midplanes = ",midplanes
    print "ranks_per_node = ",ranks_per_node
    print "threads_per_rank = ",threads_per_rank
    #print "threads_per_node = ",threads_per_node
    print "shape_in_midplanes = ",shapeToStr(shape_in_midplanes)
    print "shape_in_nodes = ",shapeToStr(shape_in_nodes)
    print "shape_in_ranks = ",shapeToStr(shape_in_ranks)
    print "timelimit =",timelimit
    raise Exception("Parameter consistency check failed!")

def argmin(seq):
  return seq.index(min(seq))

def completeShape():
  global volume,volume_per_rank,threads,ranks,nodes,midplanes,ranks_per_node,threads_per_rank,threads_per_node,shape_in_midplanes,shape_in_nodes,shape_in_ranks,timelimit,dims,logical_shape

  while True:
    if True:
      print "***********************************"
      if volume is not None:
        print "volume = ",shapeToStr(volume)
      if volume_per_rank is not None:
        print "volume_per_rank =",shapeToStr(volume_per_rank)
      if threads is not None:
        print "threads =",threads
      if ranks is not None:
        print "ranks =",ranks
      if nodes is not None:
        print "nodes =",nodes
      if midplanes is not None:
        print "midplanes = ",midplanes
      if ranks_per_node is not None:
        print "ranks_per_node =",ranks_per_node
      if threads_per_rank is not None:
        print "threads_per_rank =",threads_per_rank
      if threads_per_node is not None:
        print "threads_per_node =",threads_per_node
      if shape_in_midplanes is not None:
        print "shape_in_midplanes =",shapeToStr(shape_in_midplanes)
      if shape_in_nodes is not None:
        print "shape_in_nodes =",shapeToStr(shape_in_nodes)
      if shape_in_ranks is not None:
        print "shape_in_ranks =",shapeToStr(shape_in_ranks)
      if timelimit is not None:
        print "timelimit =",timelimit
      if dims is not None:
        print "dims =",dims
      if logical_shape is not None:
        print "logical_shape =",shapeToStr(logical_shape)


    if volume is not None:
      checkCondition(len(volume) == 4)
    if shape_in_nodes is not None:
      checkCondition(len(shape_in_nodes) == 5)
    if shape_in_ranks is not None:
      checkCondition(len(shape_in_ranks) == 6)
    if logical_shape is not None:
      checkCondition(len(logical_shape) == 4)

    # nodes = midplanes*512
    if (nodes is None) and (midplanes is not None):
      nodes = midplanes*512
      continue
    if (midplanes is None) and (nodes is not None) and (nodes%512==0):
      midplanes = nodes/512
      continue
    if (nodes is not None) and (midplanes is not None):
      checkCondition(nodes == midplanes*512);

    # ranks = nodes * ranks_per_node
    if (ranks_per_node is None) and (ranks is not None) and (nodes is not None) and (ranks%nodes==0):
      ranks_per_node = ranks/nodes
      continue
    if (ranks is None) and (ranks_per_node is not None) and (nodes is not None):
      ranks = ranks_per_node * nodes
      continue
    if (nodes is None) and (ranks is not None) and (ranks_per_node is not None) and (ranks%ranks_per_node==0):
      nodes = ranks / ranks_per_node
      continue
    if (ranks_per_node is not None) and (ranks is not None) and (nodes is not None):
      checkCondition(ranks <= nodes * ranks_per_node)

    # threads = threads_per_rank * ranks
    if (threads is None) and (threads_per_rank is not None) and (ranks is not None):
      threads = threads_per_rank * ranks
      continue
    if (ranks is None) and (threads is not None) and (threads_per_rank is not None):
      ranks = threads / threads_per_rank
      continue
    if (threads_per_rank is None) and (threads is not None) and (ranks is not None):
      threads_per_rank = threads / ranks
      continue
    if (threads is not None) and (threads_per_rank is not None) and (ranks is not None):
      checkCondition(threads == threads_per_rank * ranks)
    
    # threads = threads_per_rank * ranks
    if (threads is None) and (threads_per_rank is not None) and (ranks is not None):
      threads = threads_per_rank * ranks
      continue
    if (ranks is None) and (threads is not None) and (threads_per_rank is not None):
      ranks = threads / threads_per_rank
      continue
    if (threads_per_rank is None) and (threads is not None) and (ranks is not None):
      threads_per_rank = threads / ranks
      continue
    if (threads is None) and (threads_per_rank is not None) and (ranks is not None):
      checkCondition(threads == threads_per_rank * ranks)

    # midplanes = prod(shape_in_midplanes)
    if (midplanes is None) and (shape_in_midplanes is not None):
      midplanes = prod(shape_in_midplanes)
      continue
    if (midplanes is not None) and (shape_in_midplanes is not None):
      checkCondition(midplanes == prod(shape_in_midplanes))

    # nodes = prod(shape_in_nodes)
    if (nodes is None) and (shape_in_nodes is not None):
      nodes = prod(shape_in_nodes)
      continue
    if (nodes is not None) and (shape_in_nodes is not None):
      checkCondition(nodes == prod(shape_in_nodes))
    if (nodes is not None):
      checkCondition(nodes==32 or nodes==64 or nodes==128 or nodes==256 or (nodes%512)==0)

    # ranks = prod(shape_in_ranks)
    if (ranks is None) and (shape_in_ranks is not None):
      ranks = prod(shape_in_ranks)
      continue
    if (ranks is not None) and (shape_in_ranks is not None):
      checkCondition(ranks == prod(shape_in_ranks))

    if (shape_in_ranks is None) and (shape_in_nodes is not None) and (ranks_per_node is not None):
      shape_in_ranks = (shape_in_nodes[0],shape_in_nodes[1],shape_in_nodes[2],shape_in_nodes[3],shape_in_nodes[4],ranks_per_node)
      continue
    if (shape_in_nodes is None) and (shape_in_ranks is not None):
      shape_in_nodes = (shape_in_ranks[0],shape_in_ranks[1],shape_in_ranks[2],shape_in_ranks[3],shape_in_ranks[4])
      continue
    if (ranks_per_node is None) and (shape_in_ranks is not None):
      ranks_per_node = shape_in_ranks[5]
      continue

    # threads_per_node = threads_per_rank * ranks_per_node
    if (threads_per_node is None) and (threads_per_rank is not None) and (ranks_per_node is not None):
      threads_per_node = threads_per_rank * ranks_per_node
      continue
    if (threads_per_rank is None) and (threads_per_node is not None) and (ranks_per_node is not None) and (threads_per_node % ranks_per_node==0):
      threads_per_rank = threads_per_node / ranks_per_node
      continue
    if (ranks_per_node is None) and (threads_per_rank is not None) and (threads_per_node is not None) and (threads_per_node % threads_per_rank ==0):
      ranks_per_node = threads_per_node / threads_per_rank
      continue
    if (threads_per_node is not None) and (threads_per_rank is not None) and (ranks_per_node is not None):
      checkCondition(threads_per_node == threads_per_rank * ranks_per_node)

    if (timelimit is not None):
      checkCondition(timelimit >= datetime.timedelta(minutes=30))

    if (logical_shape is None) and (shape_in_ranks is not None):
      # Mimic MPI_Dims_create; it is too big to be rewritten here, we just make an approximation
      # TODO: Adapt for number of dimensions, this is only for 4 dimensions
      logical_shape = list(shape_in_ranks[i] for i in range(4))
      facts = factors(prod(shape_in_ranks[4:]))
      facts.sort()
      facts.reverse() # largest factors first
      while len(facts)>=1:
        i = argmin(logical_shape)
        logical_shape[i] *= facts[0]
        del facts[0]
      logical_shape = tuple(logical_shape)
      continue
    if (logical_shape is not None) and (ranks is not None):
      checkCondition(prod(logical_shape)==ranks)

    # volume = volume_per_rank * logical_shape
    if (volume is None) and (volume_per_rank is not None) and (logical_shape is not None):
      volume = tuple(logical_shape[i]*volume_per_rank[i] for i in range(4))
      continue
    if (volume is not None) and (volume_per_rank is not None) and (logical_shape is not None):
      checkCondition(tuple(logical_shape[i]*volume_per_rank[i] for i in range(4)) == volume)


    ###################
    # Recommandations, default values

    # Only vertain node counts are allowed
    if (nodes is not None):
      alignedNodes = 32
      if nodes <= 32:
        alignedNodes = 32
      elif nodes <= 64:
        alignedNodes = 64
      elif nodes <= 128:
        alignedNodes = 128
      elif nodes <= 256:
        alignedNodes = 256
      elif nodes <= 512:
        alignedNodes = 512
      else:
        alignedNodes = (nodes + 511) & ~512
      if nodes != alignedNodes:
        nodes = alignedNodes
        continue

    # Native node shape
    if (shape_in_nodes is None) and (nodes is not None):
      if nodes==32:
        shape_in_nodes = (2,2,2,2,2)
        continue
      if nodes==64:
        shape_in_nodes = (2,2,4,2,2)
        continue
      if nodes==128:
        shape_in_nodes = (2,2,4,4,2)
        continue
      if nodes==256:
        shape_in_nodes = (4,2,4,4,2)
        continue
      if nodes==512: # One midplane
        shape_in_nodes = (4,4,4,4,2)
        continue
      if (shape_in_nodes is not None) and (nodes is not None):
        checkCondition(nodes == prod(shape_in_nodes))

    # Use all 64 threads
    if threads_per_node is None:
      threads_per_node = 64
      continue

    if ranks_per_node is None:
      ranks_per_node = 1
      continue

    if nodes is None:
      nodes = 32
      continue

    #if volume is None:
    #  volume = parseShape('''@SUBMIT_VOLUME_SHAPE@''', extendLast=True)
    #  continue
    #if (shape_in_nodes is None) and (nodes is None):
    #  shape_in_nodes = parseShape('''@SUBMIT_MACHINE_SHAPE@''', extendDefault=1)
    #  continue
    if timelimit is None and nodes is not None:
      if nodes <= 64:
        timelimit = datetime.timedelta(minutes=30)
      else:
        timelimit = datetime.timedelta(hours=2)
      continue

    # All found
    break


def computeShape():
  global volume,volume_per_rank,threads,ranks,nodes,midplanes,ranks_per_node,threads_per_rank,threads_per_node,shape_in_midplanes,shape_in_nodes,shape_in_ranks,timelimit

  completeShape()

  checkCondition(shape_in_nodes is not None)
  checkCondition(len(shape_in_nodes)==5)
  checkCondition(shape_in_ranks is not None)
  checkCondition(len(shape_in_ranks)==6)
  checkCondition(ranks_per_node is not None)
  checkCondition(threads_per_rank is not None)
  checkCondition(nodes is not None)
  if nodes >= 512:
    checkCondition(shape_in_midplanes is not None)
    checkCondition(len(shape_in_midplanes)==4)
  if program!='ctest':
    checkCondition(volume is not None)
    checkCondition(volume_per_rank is not None)
    checkCondition(volume_per_rank[0]>=8 and volume_per_rank[1]>=2 and volume_per_rank[2]>=2 and volume_per_rank[3]>=2)
    checkCondition(volume_per_rank[0]%4==0 and volume_per_rank[1]%2==0 and volume_per_rank[2]%2==0 and volume_per_rank[3]%2==0)

  if program!='ctest':
    print "Volume is",'x'.join([str(v) for v in volume])
  print "Shape is",'x'.join([str(v) for v in shape_in_nodes])
  if  shape_in_midplanes is not None:
    print "Using midplanes",'x'.join([str(v) for v in shape_in_midplanes])
  print "Using",str(nodes),"nodes,",ranks_per_node,"ranks per node and",threads_per_rank,"threads per rank up to " + makestrDuration(timelimit) + " time units"


def main():
  global scriptpath,BINDIR
  global volume,volume_per_rank,threads,ranks,nodes,midplanes,ranks_per_node,threads_per_node,threads_per_node,shape_in_midplanes,shape_in_nodes,shape_in_ranks,timelimit,dims,logical_shape

  if sys.argv[0]:
    scriptpath = os.path.abspath(sys.argv[0])
  else:
    scriptpath = os.path.abspath(__file__)

  parser = optparse.OptionParser()
  parser.add_option('--noselfupdate', dest="selfupdate", action="store_false", help="Do not try update itself")
  parser.add_option('--noprogramupdate', dest="programupdate", action="store_false", help="Do not run make before running")

  parser.add_option('--program', choices=['bgqbench', 'benchmark', 'invert', 'hmc_tm', 'ctest'], help="Program to run")
  parser.add_option('--ctest', dest='program', action='store_const', const='ctest', help="Run ctest program")
  parser.add_option('--bgqbench', dest='program', action='store_const', const='bgqbench', help="Run bgqbench program")
  parser.add_option('--benchmark', dest='program', action='store_const', const='benchmark', help="Run benchmark program")
  parser.add_option('--invert', dest='program', action='store_const', const='invert', help="Run invert program")
  parser.add_option('--hmc', dest='program', action='store_const', const='hmc_tm', help="Run hmc_tm program")

  parser.add_option('--input', help="Configuration template file")

  parser.add_option('--volume', help="Global shape size")
  parser.add_option('--volume_per_rank', help="Local shape size")

  parser.add_option('--threads', help="Total number of threads in whole system")
  parser.add_option('--ranks', help="Number of ranks (=nodes * ranks_per_node)")
  parser.add_option('--nodes', help="Number of nodes")
  parser.add_option('--midplanes')
  
  parser.add_option('--ranks_per_node')
  parser.add_option('--threads_per_rank')
  parser.add_option('--threads_per_node')
  parser.add_option('--shape_in_midplanes')
  parser.add_option('--shape_in_nodes')
  parser.add_option('--shape_in_ranks')
  parser.add_option('--dims', help="MPI dimensions")
  parser.add_option('--logical_shape')

  parser.add_option('--timelimit', help="Format is hh::mm:ss")
  parser.set_defaults(selfupdate=True,programupdate=True)

  global options
  (options, args) = parser.parse_args()
  

  if options.selfupdate:
    subprocess.check_call(['''@CMAKE_COMMAND@''','--build',BINDIR,'--target','buildsystemupdate'])
    rtn = subprocess.call(['python', scriptpath, '--noselfupdate'] + sys.argv[1:])
    exit(rtn)

  global program,programupdate,inputfilepath
  program=options.program
  programupdate=options.programupdate
  inputtemplatefilepath=options.input

  if program is None:
    raise Exception("Which program to run?")

  if options.volume:
    volume=parseShape(options.volume,extendLast=True)
  if options.volume_per_rank:
    volume_per_rank=parseShape(options.volume_per_rank,extendLast=True)
  if options.threads:
    threads=int(options.threads)
  if options.ranks:
    ranks=int(options.ranks)
  if options.nodes:
    nodes=int(options.nodes)
  if options.midplanes:
    midplanes=options.midplanes
  if options.threads_per_rank:
    threads_per_rank=options.threads_per_rank
  if options.threads_per_node:
    threads_per_node=options.threads_per_node
  if options.shape_in_midplanes:
    shape_in_midplanes=parseShape(options.shape_in_midplanes,extendDefault=1)
  if options.shape_in_nodes:
    shape_in_nodes=parseShape(options.shape_in_nodes,extendDefault=1)
  if options.shape_in_ranks:
    shape_in_ranks=parseShape(options.shape_in_ranks,extendDefault=1)
  if options.dims is not None:
    dims=int(options.dims)
  if options.logical_shape is not None:
    logical_shape = options.logical_shape

  if options.timelimit:
    timelimit=parseDuration(options.timelimit)
    

  computeShape()
  prepare()
  print "Submitting job..."
  submit()
  print "Done!"


if __name__ == "__main__":
  main()
