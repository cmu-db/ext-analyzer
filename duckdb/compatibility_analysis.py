import argparse
import csv
from datetime import datetime
import json
import os
import subprocess
import sys
import duckdb-sqllogictest-python

# File paths (globals)
current_working_dir = os.getcwd()
ext_work_dir = "duckextworkdir"
extn_info_dir = "extn_info"
now = datetime.now()
date_time = now.strftime("%m-%d-%Y_%H:%M")
testing_output_dir = "testing-output-" + date_time
duckdb_dir = "duckdb"
duckdb_test_dir = "abigale"

# Load extension database
extn_files = os.listdir(current_working_dir + "/" + extn_info_dir)
extn_db = {}
for file in extn_files:
  extn_info_file = open(current_working_dir + "/" + extn_info_dir + "/" + file, "r")
  extn_info_json = json.load(extn_info_file)
  key = os.path.splitext(file)[0]
  extn_db[key] = extn_info_json


## Init + Setup
def init():
  subprocess.run("mkdir " + ext_work_dir, shell=True, cwd=current_working_dir)
  subprocess.run("mkdir " + testing_output_dir, shell=True, cwd=current_working_dir)

def cleanup():
  subprocess.run("rm -rf " + ext_work_dir, shell=True, cwd=current_working_dir)

# Helpers
def run_unittest_command(extn_list):
  unittest_command = "./" + duckdb_dir + "/build/release/test/unittest"
  for extn in extn_list:
    unittest_command += " --require " + extn
  print(unittest_command)
  test_proc_output = subprocess.run(unittest_command, shell=True, capture_output=True, cwd=current_working_dir)
  test_output = test_proc_output.stdout.decode('utf-8')

  # Create test output file
  filename = "_".join(extn_list) + "_unittest.txt"
  subprocess.run("touch " + filename, shell=True, cwd=current_working_dir + "/" + testing_output_dir)
  error_file = open(current_working_dir + "/" + testing_output_dir + "/" + filename, "w")
  error_file.write(test_output)

  if "All tests passed" in test_output:
    subprocess.run("rm " + filename, shell=True, cwd=current_working_dir + "/" + testing_output_dir)
    return True
  
  return False

def download_extn(extn):
  extn_entry = extn_db[extn]
  if "github" in extn_entry:
    github_command = "git clone " + extn_entry["github"]
    subprocess.run(github_command, shell=True, cwd=current_working_dir + "/" + ext_work_dir)

def copy_tests(extn):
  extn_entry = extn_db[extn]
  test_dirs = extn_entry["test_directories"]
  folder_name = extn_entry["folder_name"]
  for elem in test_dirs:
    elem_dir = ""
    if "github" in extn_entry:
      elem_dir = current_working_dir + "/" + ext_work_dir + "/"
    else:
      elem_dir = current_working_dir + "/" + duckdb_dir + "/extension/"

    elem_dir += folder_name + "/" + elem
    test_files = os.listdir(elem_dir)
    print(test_files)
    dest_directory = current_working_dir + "/" + duckdb_dir + "/test/sql/" + duckdb_test_dir
    print(elem_dir)
    print(dest_directory)
    for tf in test_files:
      tf_name, tf_ext = os.path.splitext(tf)
      subprocess.run("cp " + tf + " " + dest_directory, shell=True, cwd=elem_dir)
      subprocess.run("mv " + tf + " " + extn + "_" + tf_name + tf_ext, shell=True, cwd=dest_directory)

def clear_test_folder():
  test_directory = current_working_dir + "/" + duckdb_dir + "/test/sql/" + duckdb_test_dir
  subprocess.run("rm -rf *", shell=True, cwd=test_directory)

if __name__ == '__main__':
  init()
  extns = ["arrow", "autocomplete", "duckpgq"]
  for extn in extns:
    download_extn(extn)
    copy_tests(extn)
    val = run_unittest_command([extn])
    print(val)
    clear_test_folder()

  #cleanup()
