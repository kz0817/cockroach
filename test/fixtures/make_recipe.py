#!/usr/bin/env python

import sys
import subprocess

def make_measure_time_one(probe_type, install_type, func_name, save_instr=""):
  p1 = subprocess.Popen(["nm", "../.libs/libtargets.so.0"], stdout=subprocess.PIPE)
  p2 = subprocess.Popen(["grep", func_name], stdin=p1.stdout, stdout=subprocess.PIPE)
  line = p2.communicate()[0]
  addr = line.split(" ")[0]

  # output 
  print "# " + func_name
  print probe_type + " " + install_type + " libtargets.so.0.0.0 " + addr + " " + save_instr

def make_measure_time():
    make_measure_time_one("T", "REL32", "func1", "0")
    make_measure_time_one("T", "REL32", "func1a")
    make_measure_time_one("T", "REL32", "func1b", "6")
    make_measure_time_one("T", "ABS64", "func2")

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
