#!/usr/bin/env python

import sys
import subprocess

def make_measure_time_one(func_name):
  p1 = subprocess.Popen(["nm", "../.libs/libtargets.so.0"], stdout=subprocess.PIPE)
  p2 = subprocess.Popen(["grep", func_name], stdin=p1.stdout, stdout=subprocess.PIPE)
  line = p2.communicate()[0]
  addr = line.split(" ")[0]

  # output 
  print "# " + func_name
  print "t libtargets.so.0.0.0 " + addr + " 0"

def make_measure_time():
    make_measure_time_one("func1")

command_map = {"measure-time":make_measure_time}


# -----------------------------------------------------------------------------
# start
# -----------------------------------------------------------------------------
if len(sys.argv) < 2:
  sys.stderr.write("Error: len(argv): %d\n" % len(sys.argv))
  sys.exit(-1)

command = sys.argv[1];
if command not in command_map:
  sys.stderr.write("Unknown command :%s\n" % command);
  sys.exit(-1)
command_map[command]()
