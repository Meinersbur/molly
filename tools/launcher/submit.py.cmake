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
import shlex


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


def parseShape(str, nDims, extendLast=False, extendDefault=False):
  str = str.strip()
  result = []
  if not str:
    return result
  for l in str.split('x'):
    result.append(int(l))
  if extendLast:
    while len(result) < nDims:
      result.append(result[-1])
  if not extendDefault is False:
    while len(result) < nDims:
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






ON=True
TRUE=True
OFF=False
FALSE=False

is_molly=@LAUNCHER_MOLLY@
project_prefix='''@LAUNCHER_PREFIX@'''

executable_bgqbench='''@EXECUTABLE_BGQBENCH@'''
executable_benchmark='''@EXECUTABLE_BENCHMARK@'''
executable_invert='''@EXECUTABLE_INVERT@'''
executable_hmc_tm='''@EXECUTABLE_HMC_TM@'''
executable_ctest='''@EXECUTABLE_CTEST@'''

git_executable='''@GIT_EXECUTABLE@'''
cmake_executable='''@CMAKE_COMMAND@'''
launcher_executable='''@CMAKE_BINARY_DIR@/bin/launcher.py'''
#submit_executable='''@CMAKE_BINARY_DIR@/bin/launcher.py''' # Determined from command line

ll_template='''@LAUNCHER_SOURCE_DIR@/job.ll.template'''
sh_template='''@LAUNCHER_SOURCE_DIR@/job.sh.template'''
examples_source_dir='''@MOLLY_SOURCE_DIR@/examples'''

SRCDIR='''@GIT_SOURCE_DIR@''' # For calling git diff
BINDIR='''@CMAKE_BINARY_DIR@'''  # For rebuilding
JOBSDIR='''/work/hch02/hch02d/jobs'''
#JOBSDIR='''@CMAKE_BINARY_DIR@/jobs'''

git_sha1='''@GIT_SHA1@'''  # For making a diff file that remembers what the executable actually contains




def isMollyExample():
  global program
  if program in ('bgqbench', 'benchmark','invert','hmc_tm','ctest'):
    return False
  if program=='benchtest':
    return is_molly
  return True


def getBuildTarget():
  global program
  if isMollyExample():
    return program + '-mollycc-a'
  else:
    return program


def getExecutable():
  global program
  if program=='bgqbench' and executable_bgqbench:
    return executable_bgqbench
  elif program=='benchmark' and executable_benchmark:
    return executable_benchmark
  elif program=='invert' and executable_invert:
    return executable_invert
  elif program=='hmc_tm' and executable_hmc_tm:
    return executable_hmc_tm
  elif program=='ctest' and executable_ctest:
    return executable_ctest
  elif program=='benchtest':
    return os.path.join(BINDIR,'examples','benchtest')
  if isMollyExample():
    exename = program + '_V' + shapeToStr(volume) + '_P' + shapeToStr(logical_shape) + ''.join(['_'+d for d in defs])
    return os.path.join(BINDIR,'examples',exename)
  return os.path.abspath(program)


def getSourceFile():
  return os.path.join(examples_source_dir, program, program+'.cpp')


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


def getProgramDims():
  if program=='benchtest':
    return []
  if program=='lqcd2d':
    return ['T','X']
  if program=='jacobi':
    return ['X','Y']
  return ['T','X','Y','Z']

def getDimCount():
  return len(getProgramDims())

def hasDims():
  return getDimCount()>0


def isBench():
  global program
  return program=='bgqbench' or program=='benchmark'

def isInvert():
  global program
  return program=='invert'

def isHmcTm():
  global program
  return program=='hmc_tm'



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

  myenv = os.environ.copy()
  del myenv['LD_LIBRARY_PATH']
  out = subprocess.check_output(['llsubmit', os.path.join(jobdir,'job.ll')], cwd=jobdir, env=myenv)
  m = jobidre.search(out)
  if m:
    jobid = m.group('jobid')
    buildJobScript() # Update jobscript to include the jobid - so the launcher script can cancel the job
    print "Submitted as ", jobid
  else:
    print "COULD NOT EXTRACT JOBID FROM OUTPUT: ", out
  

def genJobname(i):
  global volume,volume_per_rank,threads,ranks,nodes,midplanes,ranks_per_node,threads_per_rank,shape_in_midplanes,shape_in_nodes,shape_in_ranks,timelimit
  global program

  if customjobname is None:
    prefix = "{i:03d}_{program}".format(i=i,program=project_prefix+program)
  else:
    prefix = "{i:03d}_{jobname}".format(i=i,jobname=customjobname)
  suffix = ""

  if volume:
    prefix = prefix + "_v" + "x".join([str(d) for d in volume])

  if nodes and shape_in_nodes:
    suffix = suffix + "_{nodes}n{shape}".format(nodes=nodes,shape="x".join([str(d) for d in logical_shape]))
  elif nodes:
    suffix = suffix + "_{nodes}n".format(nodes=nodes)  
  elif logical_shape:
    suffix = suffix + "_n" + "x".join([str(d) for d in logical_shape])

  if volume_per_rank is not None:
    suffix = suffix + "_s" + "x".join([str(d) for d in volume_per_rank])

  if ranks_per_node:
    suffix = suffix + "_r{ranks_per_node}".format(ranks_per_node=ranks_per_node) 

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
  global volume,volume_per_rank,threads,ranks,nodes,midplanes,ranks_per_node,threads_per_rank,shape_in_midplanes,shape_in_nodes,shape_in_ranks,timelimit,jobid,jobscriptfile
  global jobname,executable,jobdir,program
  global inputfilepath

  shape_machine_str = ''
  if shape_in_midplanes is not None:
    shape_machine_str = 'bg_shape = ' + string.join([str(l) for l in shape_machine],'x')
  elif nodes is not None:
    shape_machine_str = 'bg_size = ' + str(nodes)

  prep=''

  launcheroutputfilepath = os.path.join(jobdir,'yout.txt')
  if jobid:
    launcheroutputfilepath = os.path.join(jobdir,'yout' + jobid + '.txt')
  
  compilecmdlinestr = ''
  if compilecmdline is not None:
    compilecmdlinestr = ' '.join(pipes.quote(s) for s in compilecmdline)

  template = readCompleteFile(template_path)
  formatdict = dict(
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
    prep=prep,
    compilecmdline=compilecmdlinestr,
    nodes=nodes,
    ranks=ranks)
  dimnames = getProgramDims()
  for i in range(len(dimnames)):
    formatdict['L' + dimnames[i]] = volume[i]
    formatdict['Ranks' + dimnames[i]] = logical_shape[i]
  content = template.format(**formatdict)
  writeCompleteFile(target_path, content)


def buildInput():
  templatepath = getInputTemplatePath()
  global BINDIR,jobdir,inputfilepath,SRCDIR
  if isBench():
    inputfilepath = os.path.join(jobdir, 'benchmark.input')
  elif isInvert():
    inputfilepath = os.path.join(jobdir, 'invert.input')
  elif isHmcTm():
    inputfilepath = os.path.join(jobdir, 'hmc.input')
  else:
    inputfilepath = os.path.join(jobdir, 'input')

  if templatepath:
    configureFile(templatepath, inputfilepath)
  else:
    templatepath = None


def buildLLScript():
  global SRCDIR,jobdir
  configureFile(ll_template, os.path.join(jobdir, 'job.ll'))


def buildJobScript():
  global SRCDIR,jobdir,jobscriptfile
  if not jobscriptfile:
    jobscriptfilename = 'job_{nodes}_{shape}.sh'.format(nodes=nodes,shape=shapeToStr(shape_in_nodes))
    jobscriptfile = os.path.join(jobdir, jobscriptfilename)
  configureFile(sh_template, jobscriptfile)


def copyExecutable():
  global jobdir, program, executable,BINDIR,SRCDIR
  srcexecutable = getExecutable()
  executable = os.path.join(jobdir, os.path.basename(srcexecutable))
  shutil.copy2(srcexecutable, executable)
  #shutil.copy2(os.path.join(SRCDIR,'corestack.py'), os.path.join(jobdir,'corestack.py'))


mollycc_executable = '''@MOLLYCC_COMPILER@'''
mollycc_flags = ([] + 
  '''@MOLLYCC_FLAGS@'''.split(';') +
  '''@MOLLY_PLATFORM_FLAGS@'''.split(';') +
  '''@MOLLYCC_COMMON_FLAGS@'''.split(';') +
  '''@MOLLYCC_VERBOSE_FLAGS@'''.split(';') +
  '''@MOLLYCC_DEBUG_FLAGS@'''.split(';') +
  '''@MOLLYCC_CFLAGS@'''.split(';') +
  '''@MOLLY_PLATFORM_CFLAGS@'''.split(';') +
  '''@MOLLYCC_COMMON_CFLAGS@'''.split(';') +
  '''@MOLLYCC_VERBOSE_CFLAGS@'''.split(';') +
  '''@MOLLYCC_DEBUG_CFLAGS@'''.split(';') +
  '''@MOLLY_PLATFORM_DEFS@'''.split(';') +
  '''@MOLLYCC_COMMON_DEFS@'''.split(';') +
  '''@MOLLYCC_VERBOSE_DEFS@'''.split(';') +
  '''@MOLLYCC_DEBUG_DEFS@'''.split(';') +
  '''@MOLLYCC_LDFLAGS@'''.split(';') +
  '''@MOLLY_PLATFORM_LDFLAGS@'''.split(';') +
  '''@MOLLYCC_COMMON_LDFLAGS@'''.split(';') +
  '''@MOLLYCC_VERBOSE_LDFLAGS@'''.split(';') +
  '''@MOLLYCC_DEBUG_LDFLAGS@'''.split(';') +
  '''@MOLLYCC_A_LIBS@'''.split(';') +
  ['-mllvm','-polly-only-marked '])


compilecmdline=None
def makeProgram():
  global program,BINDIR
  if isMollyExample():
    # Make the mollycc compiler executable
    print "Updating mollycc and MollyRT..."
    subprocess.check_call([cmake_executable,'--build',BINDIR,'--target','mollycc','--target','MollyRT-a','--','-j32'])

    print "Compiling", os.path.basename(getExecutable()) ,"with Molly..."
    cmdline = [mollycc_executable]
    cmdline += ['-o', getExecutable()]
    cmdline += [getSourceFile()]

    for flag in mollycc_flags:
      flag = flag.strip()
      if len(flag)>0:
        if not hasDims() and flag=='-molly':
          cmdline.append('-molly=False') # Switch off Molly
        else:
          cmdline.append(flag)
    
    if hasDims():
      dims = getProgramDims()
      assert len(dims)==len(volume)
      for i in range(len(dims)):
        cmdline += ['-DL' + dims[i] + '=' + str(volume[i])]
      assert len(dims)==len(logical_shape)
      for i in range(len(dims)):
        cmdline += ['-DP' + dims[i] + '=' + str(logical_shape[i])]
      assert len(dims)==len(volume_per_rank)
      for i in range(len(dims)):
        cmdline += ['-DB' + dims[i] + '=' + str(volume_per_rank[i])]
      cmdline += ['-mllvm','-shape=' + shapeToStr(logical_shape)]

    cmdline += ['-D' + d for d in defs]

    global compilecmdline
    compilecmdline = cmdline
    print ' '.join(cmdline)
    subprocess.check_call(cmdline,cwd=BINDIR)

  else:
    subprocess.check_call([cmake_executable,'--build',BINDIR,'--target',getBuildTarget(),'--','-j32'])


def copySource():
  global SRCDIR,BINDIR
  global jobdir
  global git_executable

  srctargetdir = os.path.join(jobdir,'src')
  shutil.copy2(os.path.join(BINDIR,'CMakeCache.txt'), os.path.join(jobdir,'CMakeCache.txt'))

  if git_executable and git_sha1 and git_sha1!='GITDIR-NOTFOUND':
    difffilepath = os.path.join(jobdir,'src.diff')
    with open(difffilepath, 'w') as diff_file:
      subprocess.check_call([git_executable,'diff','--patch-with-stat','--src-prefix=' +git_sha1+ '/','--dst-prefix=workingset/', git_sha1], cwd=SRCDIR, stdout=diff_file)

    rematerialize_script = """#! /bin/sh
{git} clone "{projectsrcdir}" "{srctargetdir}"
cd "{srctargetdir}"
{git} checkout {git_sha1} # @GIT_REFSPEC@
{git} apply --whitespace=nowarn "{difffilepath}"
""".format(jobdir=jobdir,difffilepath=difffilepath,git=git_executable,srctargetdir=srctargetdir,git_sha1=git_sha1,projectsrcdir=SRCDIR)
    writeCompleteFile(os.path.join(jobdir,'getsrc.sh'), rematerialize_script, setExecutableBit=True)
  else:
    shutil.copytree(SRCDIR, srctargetdir, symlinks=True)


def prepare():
  global jobdir,jobname,programupdate

  if programupdate:
    print "Compiling",getBuildTarget(),"..."
    makeProgram()
    
  jobdir,jobname = findjobdir(JOBSDIR)
  print "Create job directory...", jobdir
  os.makedirs(jobdir)
  print "Copying executable..."
  copyExecutable()
  print "Copying sources..."
  copySource()
  if not isMollyExample():
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
shape_in_midplanes=None
shape_in_nodes=None
shape_in_ranks=None
timelimit=None
dims=None
logical_shape=None

defs = []
jobid = None
jobscriptfile = None
inputfilepath = None
customjobname = None

def checkCondition(cond):
  global volume,volume_per_rank,threads,ranks,nodes,midplanes,ranks_per_node,threads_per_rank,shape_in_midplanes,shape_in_nodes,shape_in_ranks,timelimit

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
    print "shape_in_midplanes = ",shapeToStr(shape_in_midplanes)
    print "shape_in_nodes = ",shapeToStr(shape_in_nodes)
    print "shape_in_ranks = ",shapeToStr(shape_in_ranks)
    print "timelimit =",timelimit
    raise Exception("Parameter consistency check failed!")

def argmin(seq):
  return seq.index(min(seq))

def ceilNodes(nodes):
  if nodes <= 32:
    return 32
  elif nodes <= 64:
    return 64
  elif nodes <= 128:
    return 128
  elif nodes <= 256:
    return 256
  elif nodes <= 512:
    return 512
  else:
    return(nodes + 511) & ~512


def completeShape():
  global volume,volume_per_rank,threads,ranks,nodes,midplanes,ranks_per_node,threads_per_rank,shape_in_midplanes,shape_in_nodes,shape_in_ranks,timelimit,dims,logical_shape
  nDims=getDimCount()

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
      checkCondition(len(volume) == getDimCount())
    if shape_in_nodes is not None:
      checkCondition(len(shape_in_nodes) == 5)
    if shape_in_ranks is not None:
      checkCondition(len(shape_in_ranks) == 6)
    if logical_shape is not None:
      checkCondition(len(logical_shape) == getDimCount())
    if shape_in_midplanes is not None:
      checkCondition(len(shape_in_midplanes) == 4)

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

    # ranks = prod(shape_in_ranks)
    if (ranks is None) and (shape_in_ranks is not None):
      ranks = prod(shape_in_ranks)
      continue
    if (ranks is not None) and (shape_in_ranks is not None):
      checkCondition(ranks == prod(shape_in_ranks))

    #if (shape_in_ranks is None) and (shape_in_nodes is not None) and (ranks_per_node is not None):
    #  shape_in_ranks = (shape_in_nodes[0],shape_in_nodes[1],shape_in_nodes[2],shape_in_nodes[3],shape_in_nodes[4],ranks_per_node)
    #  continue
    if (shape_in_nodes is None) and (shape_in_ranks is not None):
      shape_in_nodes = (shape_in_ranks[0],shape_in_ranks[1],shape_in_ranks[2],shape_in_ranks[3],shape_in_ranks[4])
      continue
    if (ranks_per_node is None) and (shape_in_ranks is not None):
      ranks_per_node = shape_in_ranks[5]
      continue

    if (timelimit is not None):
      checkCondition(timelimit >= datetime.timedelta(minutes=30))

    if hasDims() and (logical_shape is None) and (shape_in_ranks is not None):
      # Mimic MPI_Dims_create; it is too big to be rewritten here, we just make an approximation
      # TODO: Adapt for number of dimensions, this is only for 4 dimensions
      nDims=getDimCount()
      logical_shape = list(shape_in_ranks[i] for i in range(nDims))
      facts = factors(prod(shape_in_ranks[nDims:]))
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
      volume = tuple(logical_shape[i]*volume_per_rank[i] for i in range(getDimCount()))
      continue
    if (volume is not None) and (volume_per_rank is not None) and (logical_shape is not None):
      checkCondition(tuple(logical_shape[i]*volume_per_rank[i] for i in range(getDimCount())) == volume)

    if hasDims() and (logical_shape is None) and (ranks is not None):
      facts = factors(ranks)
      facts.sort()
      facts.reverse() # largest factors first
      logical_shape = list(1 for i in range(nDims))
      while len(facts)>=1:
        i = argmin(logical_shape)
        logical_shape[i] *= facts[0]
        del facts[0]
      logical_shape = tuple(logical_shape)
      continue

    # volume[] = logical_shape[] * volume_per_rank[]
    if (volume is None) and (logical_shape is not None) and (volume_per_rank is not None):
      volume = tuple(logical_shape[i]*volume_per_rank[i] for i in range(nDims))
      continue
    if (volume_per_rank is None) and (logical_shape is not None) and (volume is not None):
      volume_per_rank = tuple(volume[i]/logical_shape[i] for i in range(nDims))
      continue
    if (logical_shape is None) and (volume is not None) and (volume_per_rank is not None): # TODO: Divisibility
      logical_shape = tuple(volume[i]/volume_per_rank[i] for i in range(nDims))
      continue
    if (volume is not None) and (logical_shape is not None) and (volume_per_rank is not None):
      checkCondition(volume == tuple(logical_shape[i]*volume_per_rank[i] for i in range(nDims)))

    ###################
    # Recommandations, default values

    # threads_per_rank * ranks_per_node <= 64
    if (threads_per_rank is None) and (ranks_per_node is not None):
      threads_per_rank = 64/ranks_per_node
      continue
    if (ranks_per_node is None) and (threads_per_rank is not None):
      ranks_per_node = 64/threads_per_rank
      continue
    if (threads_per_rank is not None) and (ranks_per_node is not None):
      checkCondition(threads_per_rank * ranks_per_node <= 64) 

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
    if (nodes is not None):
      checkCondition(nodes==32 or nodes==64 or nodes==128 or nodes==256 or (nodes%512)==0)

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
      checkCondition(5 == len(shape_in_nodes))

    if ranks_per_node is None:
      ranks_per_node = 1
      continue

    if nodes is None:
      nodes = 32
      continue

    if timelimit is None and nodes is not None:
      if nodes <= 64:
        timelimit = datetime.timedelta(minutes=30)
      else:
        timelimit = datetime.timedelta(hours=2)
      continue

    # All found
    break


def computeShape():
  global volume,volume_per_rank,threads,ranks,nodes,midplanes,ranks_per_node,threads_per_rank,shape_in_midplanes,shape_in_nodes,shape_in_ranks,timelimit

  completeShape()

  print ">> Shape computed, checking for completeness"
  checkCondition(shape_in_nodes is not None)
  checkCondition(len(shape_in_nodes)==5)
  #checkCondition(shape_in_ranks is not None)
  #checkCondition(len(shape_in_ranks)==6)
  checkCondition(ranks_per_node is not None)
  checkCondition(threads_per_rank is not None)
  checkCondition(nodes is not None)
  checkCondition(ranks is not None)
  if nodes >= 512:
    checkCondition(shape_in_midplanes is not None)
    checkCondition(len(shape_in_midplanes)==4)
  if hasDims():
    checkCondition(volume is not None)
    checkCondition(volume_per_rank is not None)
    #checkCondition(volume_per_rank[0]>=8 and volume_per_rank[1]>=2 and volume_per_rank[2]>=2 and volume_per_rank[3]>=2)
    #checkCondition(volume_per_rank[0]%4==0 and volume_per_rank[1]%2==0 and volume_per_rank[2]%2==0 and volume_per_rank[3]%2==0)
  else:
    checkCondition(volume is None)
    checkCondition(volume_per_rank is None)

  if hasDims():
    print "Volume is",'x'.join([str(v) for v in volume])
  print "Shape is",'x'.join([str(v) for v in shape_in_nodes])
  if  shape_in_midplanes is not None:
    print "Using midplanes",'x'.join([str(v) for v in shape_in_midplanes])
  print "Using",str(nodes),"nodes,",ranks_per_node,"ranks per node and",threads_per_rank,"threads per rank up to " + makestrDuration(timelimit) + " time units"




def main():
  global scriptpath,BINDIR
  global volume,volume_per_rank,threads,ranks,nodes,midplanes,ranks_per_node,shape_in_midplanes,shape_in_nodes,shape_in_ranks,timelimit,dims,logical_shape

  if sys.argv[0]:
    scriptpath = os.path.abspath(sys.argv[0])
  else:
    scriptpath = os.path.abspath(__file__)

  parser = optparse.OptionParser()
  parser.add_option('--noselfupdate', dest="selfupdate", action="store_false", help="Do not try update itself")
  parser.add_option('--noprogramupdate', dest="programupdate", action="store_false", help="Do not run make before running")
  parser.add_option('--nosubmit', dest="submission", action="store_false", help="Prepare, but do not submit job")

  parser.add_option('--program', help="Program to run")
  parser.add_option('--ctest', dest='program', action='store_const', const='ctest', help="Run ctest program")
  parser.add_option('--bgqbench', dest='program', action='store_const', const='bgqbench', help="Run bgqbench program")
  parser.add_option('--benchmark', dest='program', action='store_const', const='benchmark', help="Run benchmark program")
  parser.add_option('--invert', dest='program', action='store_const', const='invert', help="Run invert program")
  parser.add_option('--hmc', dest='program', action='store_const', const='hmc_tm', help="Run hmc_tm program")
  parser.add_option('--lqcd', dest='program', action='store_const', const='lqcd', help="Run lqcd example")
  parser.add_option('--lqcd2d', dest='program', action='store_const', const='lqcd2d', help="Run lqcd2d example")
  parser.add_option('--jacobi', dest='program', action='store_const', const='jacobi', help="Run jacobi example")  
  parser.add_option('--benchtest', dest='program', action='store_const', const='benchtest', help="Test bench.c")  

  parser.add_option('--input', help="Configuration template file")
  parser.add_option('--defs', action='append', help="Addition preprocessor defintions")
  parser.add_option('--series', help="Used to append to job name")

  parser.add_option('--volume', help="Global shape size")
  parser.add_option('--volume_per_rank', help="Local shape size")

  parser.add_option('--threads', type='int', help="Total number of threads in whole system")
  parser.add_option('--ranks', type='int', help="Number of ranks (=nodes * ranks_per_node)")
  parser.add_option('--nodes', type='int', help="Number of nodes")
  parser.add_option('--midplanes', type='int')
  
  parser.add_option('--ranks_per_node', type="int")
  parser.add_option('--threads_per_rank', type='int')
  parser.add_option('--shape_in_midplanes')
  parser.add_option('--shape_in_nodes')
  parser.add_option('--shape_in_ranks')
  parser.add_option('--dims', type='int', help="MPI dimensions")
  parser.add_option('--logical_shape')

  parser.add_option('--timelimit', help="Format is hh::mm:ss")
  parser.set_defaults(selfupdate=True,programupdate=True,submission=True)

  global options
  (options, args) = parser.parse_args()
  

  if options.selfupdate:
    subprocess.check_call([cmake_executable,'--build',BINDIR,'--target','buildsystemupdate'])
    rtn = subprocess.call(['python', scriptpath, '--noselfupdate'] + sys.argv[1:])
    exit(rtn)

  global program,programupdate,inputfilepath
  program=options.program
  programupdate=options.programupdate
  inputtemplatefilepath=options.input

  if program is None:
    raise Exception("Which program to run?")

  global defs,customjobname,ranks_per_node
  if options.ranks_per_node is not None:
    ranks_per_node=int(options.ranks_per_node)
  if options.series is not None:
    customjobname=options.series
  if options.defs is not None:
    defs=options.defs
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
  if options.shape_in_midplanes:
    shape_in_midplanes=parseShape(options.shape_in_midplanes,4,extendDefault=1)
  if options.shape_in_nodes:
    shape_in_nodes=parseShape(options.shape_in_nodes,5,extendDefault=1)
  if options.shape_in_ranks:
    shape_in_ranks=parseShape(options.shape_in_ranks,6,extendDefault=1)
  if options.dims is not None:
    dims=int(options.dims)
  if options.logical_shape is not None:
    logical_shape = parseShape(options.logical_shape,getDimCount())
  if options.volume:
    volume=parseShape(options.volume,getDimCount(),extendLast=True)
  if options.volume_per_rank:
    volume_per_rank=parseShape(options.volume_per_rank,getDimCount(),extendLast=True)

  if options.timelimit:
    timelimit=parseDuration(options.timelimit)
    
  computeShape()
  prepare()
  if options.submission:
    print "Submitting job..."
    submit()
  print "Done!"


if __name__ == "__main__":
  main()
