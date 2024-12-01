import argparse
import csv
from datetime import datetime
import json
import os
import subprocess
import sys
import duckdb

sys.path.append("./duckdb-sqllogictest-python")
from duckdb_sqllogictest.python_runner import SQLLogicTestExecutor
from duckdb_sqllogictest.result import ExecuteResult
from duckdb_sqllogictest.test import SQLLogicTest

# File paths (globals)
current_working_dir = os.getcwd()
ext_work_dir = "duckextworkdir"
ext_build_dir = "extnbuilddir"
ext_test_dir = "extntestdir"
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

def run_extn_tests(extn_list):
  ret = True
  for extn in extn_list:
    extn_entry = extn_db[extn]
    if extn == "jemalloc" or extn == "parquet":
      continue
    else:
      install_cmd = "INSTALL " + extn
      install_cmd += (" from community;" if "community" in extn_entry and extn_entry["community"] else ";")
      duckdb.sql(install_cmd)
      duckdb.sql("LOAD " + extn + ";")

  for extn in extn_list:
    # Open test directories for the extension
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
      executor = SQLLogicTestExecutor(ext_build_dir, ext_test_dir)
      test_files = os.listdir(elem_dir)

      for tf in test_files:
        total_path = elem_dir + "/" + tf 
        expected_result = executor.execute_test(SQLLogicTest(total_path))
        print(expected_result.type)
        print(type(expected_result.type))
        print(f"Test result type: {expected_result.type}")
        print(f"SUCCESS type: {ExecuteResult.Type.SUCCESS}")
        
        # Print the actual type to see what we're getting
        print(f"Type comparison: {expected_result.type == ExecuteResult.Type.SUCCESS}")
        
        if expected_result.type == ExecuteResult.Type.SUCCESS:
            print("success")
        else:
          ret = False
  
  return ret


## Init + Setup
def init():
  subprocess.run("mkdir " + ext_work_dir, shell=True, cwd=current_working_dir)
  subprocess.run("mkdir " + ext_build_dir, shell=True, cwd=current_working_dir)
  subprocess.run("mkdir " + ext_test_dir, shell=True, cwd=current_working_dir)
  subprocess.run("mkdir " + testing_output_dir, shell=True, cwd=current_working_dir)

def cleanup():
  subprocess.run("rm -rf " + ext_work_dir, shell=True, cwd=current_working_dir)
  subprocess.run("rm -rf " + ext_build_dir, shell=True, cwd=current_working_dir)
  subprocess.run("rm -rf " + ext_test_dir, shell=True, cwd=current_working_dir)

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
      subprocess.run("cp -r " + tf + " " + dest_directory, shell=True, cwd=elem_dir)
      subprocess.run("mv " + tf + " " + extn + "_" + tf_name + tf_ext, shell=True, cwd=dest_directory)

def clear_test_folder():
  test_directory = current_working_dir + "/" + duckdb_dir + "/test/sql/" + duckdb_test_dir
  subprocess.run("rm -rf *", shell=True, cwd=test_directory)

if __name__ == '__main__':
  parser = argparse.ArgumentParser(
                    description='Runs compatibility testing.')
  parser.add_argument('-m', '--mode', action='store', help='Determine whether to run compatibility testing or single extension testing.')
  
  init()
  extns = list(extn_db.keys())
  extns.sort()

  args = parser.parse_args()
  args_dict = vars(args)
  mode = args_dict['mode']
  if mode is None:
    sys.exit("No mode parameter.")

  result_file = open("compatibility.csv", "w")
  result_csv = csv.writer(result_file)
  if mode == "single":
    result_csv.writerow(["Extension Name", "Test Results"])
    for extn in extns:
      print("Running tests on " + extn)
      download_extn(extn)
      result = run_extn_tests([extn])
      result_csv.writerow([extn, str(result)])
  elif mode == "pairwise":
    result_csv.writerow(["Extension 1", "Extension 2", "Test Results"])
    for extn in extns:
      download_extn(extn)

    for extn1 in extns:
      for extn2 in extns:
        if extn1 == extn2:
          continue
        else:
          result = run_extn_tests([extn1, extn2])
          result_csv.writerow([extn1, extn2, str(result)])

  cleanup()