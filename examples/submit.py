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


ON=True
OFF=False


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

def parseDuration(str):
	str = str.strip()
	m = re.match(r"(?P<hours>\d+)h", str)
	if m:
		return int(m.group('hours'))*60*60

	comps = tuple(int(x) for x in str.split(':'))
	if len(comps)==2:
		result = comps[0]*60 + comps[1]
	elif len(comps)==3:
		result = comps[0]*60*60 + comps[1]*60 + comps[2]
	else:
		raise Exception("Wrong time format")
	return result

def makestrDuration(secs):
	if secs >= 60*60:
		result = "{h}:{m:02d}:{s:02d}".format(h=secs//(60*60),m=secs//60 % 60,s=secs%60)
	else:
		result =  "{m:02d}:{s:02d}".format(m=secs//60,s=secs%60)
	return result





def genJobname(i):
	global jobfile
	
	jobbase = os.path.basename(jobfile)
	jobname = os.path.splitext(jobbase)[0]

	return "{jobname}_{i:03d}".format(i=i,jobname=jobname)


def findjobdir(jobsdir):
  largest = -1
  list = [os.path.abspath(os.path.join(jobsdir,item)) for item in os.listdir(jobsdir)]
  for dir in list:
    bname = os.path.basename(dir)
    nstr = re.search(r'\d+', bname).group()
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
  global jobname,jobdir,executable

  content = readCompleteFile(template_path)
  formatdict = dict(
    jobname=jobname,
    jobdir=jobdir,
    outputfilepath=os.path.join(jobdir,'qout$(jobid).txt'))
	
  for file in files:
    filebase = os.path.basename(file)
    dstfilename = os.path.join(jobdir, filebase)
    content = content.replace('<file:' + filebase + '>', dstfilename);
  content = content.format(**formatdict)
  writeCompleteFile(target_path, content)



def buildLLScript():
	global jobfile
	global dstjobfile
	dstjobfile = os.path.join(jobdir, os.path.basename(jobfile))
	configureFile(jobfile, dstjobfile)


def copyFiles():
  global jobdir,files
  for file in files:
    dstfilename = os.path.join(jobdir, os.path.basename(file))
    shutil.copy2(file, dstfilename)


def prepare():
  global jobdir,jobname,execdir
  if not os.path.exists(execdir):
    os.makedirs(execdir)
  jobdir,jobname = findjobdir(execdir)
  os.makedirs(jobdir)

  print "Copying files..."
  copyFiles()
  print "Configure job script..."
  buildLLScript()
  print "Job prepared at ",jobdir


def submit():
	global jobdir,jobfile
	dstjobfile = os.path.join(jobdir, os.path.basename(jobfile))
	subprocess.check_call(['llsubmit', dstjobfile], cwd=jobdir)


def main():
  global scriptpath
  if sys.argv[0]:
    scriptpath = os.path.abspath(sys.argv[0])
  else:
    scriptpath = os.path.abspath(__file__)

  global options,args
  parser = optparse.OptionParser()
  parser.add_option('--jobfile', help="Submission config")
  parser.add_option('--execdir', help="Where to store the jobs")
  parser.set_defaults(execdir="/work/hch02/hch02d/jobs")
  (options, args) = parser.parse_args()

  global files
  files = args

  global jobfile
  jobfile=options.jobfile
  if jobfile is None:
    raise Exception("Job description missing")

  global execdir
  execdir=options.execdir
  
  #global executable
  #executable = options.executable
  #if not executable is None:
  #  files.append(executable)

  prepare()
  print "Submitting job..."
  submit()
  print "Done!"


if __name__ == "__main__":
  main()
