#!/usr/bin/env python

import sys
import subprocess

def make_measure_time_one(probe_type, install_type, func_name,
                          save_instr="", target_module="libtargets.so.0.0.0"):

  p1 = subprocess.Popen(["nm", "../.libs/" + target_module],
                        stdout=subprocess.PIPE)
  p2 = subprocess.Popen(["grep", func_name], stdin=p1.stdout,
                        stdout=subprocess.PIPE)
  line = p2.communicate()[0]
  addr = line.split(" ")[0]
  if len(addr) == 0:
    sys.exit(-1)

  # output 
  print "# " + func_name
  print probe_type + " " + install_type + " " + target_module + " " + \
        addr + " " + save_instr

def make_measure_time():
    target_program = "target-exe"
    make_measure_time_one("T", "REL32", "func1", "0")
    make_measure_time_one("T", "REL32", "func1a")
    make_measure_time_one("T", "REL32", "func1b", "6")
    make_measure_time_one("T", "ABS64", "func2")
    make_measure_time_one("T", "REL32", "funcX", target_module=target_program)

def make_user_prbe_one(probe_type, install_type, func_name, user_probe_func,
                       save_instr="", user_probe_module="user_probe.so",
                       user_probe_init_func="",
                       target_module="libtargets.so.0.0.0"):

  p1 = subprocess.Popen(["nm", "../.libs/" + target_module],
                        stdout=subprocess.PIPE)
  p2 = subprocess.Popen(["grep", func_name], stdin=p1.stdout,
                        stdout=subprocess.PIPE)
  line = p2.communicate()[0]
  addr = line.split(" ")[0]
  if len(addr) == 0:
    sys.exit(-1)

  # output 
  print "# " + func_name
  print probe_type + " " + install_type + " " + target_module + " " + \
        addr + " " + save_instr + " " + user_probe_module + " " + \
        user_probe_func + " " + user_probe_init_func

def make_user_probe():
    make_user_prbe_one("P", "REL32", "func1", "user_probe")
    make_user_prbe_one("P", "REL32", "func1b", "user_probe", "6")
    make_user_prbe_one("P", "REL32", "func1a", "user_probe",
                       user_probe_init_func="user_probe_init")
    make_user_prbe_one("P", "REL32", "sum_up_to", "data_recorder",
                       user_probe_init_func="data_recorder_init")

command_map = {"measure-time":make_measure_time, "user-probe":make_user_probe}


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
