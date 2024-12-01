import csv
from datetime import datetime
import json
import os
import re
import subprocess
from enum import Enum

class DuckDBExtensionType(Enum):
    FILESYSTEM = 1
    FUNCTION = 2
    OPERATOR = 3
    OPTIMIZER = 4
    PARSER = 5
    STORAGE = 6
    TABLE_FUNCTION = 7
    TYPE = 8

# File paths (globals)
current_working_dir = os.getcwd()
ext_work_dir = "duckextworkdir"
extn_info_dir = "extn_info"
now = datetime.now()
date_time = now.strftime("%m-%d-%Y_%H:%M")
testing_output_dir = "testing-output-" + date_time
duckdb_dir = "duckdb"
duckdb_test_dir = "abigale"

# Common C/C++ extensions
common_c_file_extns = ["h", "hh", "c", "cpp", "cc", "cxx", "cpp"]

# PMD argument globals
pmd_command = "./pmd-bin-7.6.0/bin/pmd cpd --minimum-tokens 100"
pmd_options = "  --language cpp  --no-fail-on-violation"

def init():
  subprocess.run("mkdir " + ext_work_dir, shell=True, cwd=current_working_dir)
  subprocess.run("mkdir " + testing_output_dir, shell=True, cwd=current_working_dir)

def cleanup():
  subprocess.run("rm -rf " + ext_work_dir, shell=True, cwd=current_working_dir)


# Helper functions
def download_extn(extn):
  extn_entry = extn_db[extn]
  if "github" in extn_entry:
    github_command = "git clone " + extn_entry["github"]
    subprocess.run(github_command, shell=True, cwd=current_working_dir + "/" + ext_work_dir)

# Source Code Analysis Helpers
def get_type_from_line(cl : str):
  # Comment
  if cl.startswith("//") or cl.startswith("*"):
    return None

  if not cl.endswith(";"):
    return None

  cl = cl.strip()
  if cl.startswith("ExtensionUtil::RegisterFunction"):
    return DuckDBExtensionType.FUNCTION
  elif cl.startswith("OperatorExtension"):
    return DuckDBExtensionType.OPERATOR
  elif cl.startswith("OptimizerExtension"):
    return DuckDBExtensionType.OPTIMIZER
  elif cl.startswith("ParserExtension"):
    return DuckDBExtensionType.PARSER
  elif cl.startswith("StorageExtension"):
    return DuckDBExtensionType.STORAGE
  elif cl.startswith("ExtensionUtil::RegisterType"):
    return DuckDBExtensionType.TYPE
  elif "CreateFunction" in cl:
    return DuckDBExtensionType.FUNCTION
  elif "CreateTableFunction" in cl:
    return DuckDBExtensionType.TABLE_FUNCTION
  elif "RegisterSubSystem" in cl:
    return DuckDBExtensionType.FILESYSTEM
  elif "parser_extensions.push_back" in cl:
    return DuckDBExtensionType.PARSER
  elif ".storage_extensions" in cl:
    return DuckDBExtensionType.STORAGE
  elif "operator_extensions.push_back" in cl:
    return DuckDBExtensionType.OPERATOR
  elif "optimizer_extensions.push_back" in cl:
    return DuckDBExtensionType.OPTIMIZER
  
  return None

def get_source_code_dir(extn):
  extn_entry = extn_db[extn]
  folder_name = extn_entry["folder_name"]
  source_dir = extn_entry["source_dir"]
  if "core_folder" in extn_entry:
    ret = current_working_dir + "/duckdb/extension/"
  else:
    ret = current_working_dir + "/" + ext_work_dir + "/"
  
  ret += folder_name + "/" + source_dir
  return ret
 

# PMD CPD Helpers

# Function to get the number of lines of code from the output
def parse_stats(stats):
  stats_list = stats.split(" ")
  num_lines = int(stats_list[2])
  return num_lines

# Returns a tuple (key, (start, end)) with the key as the file that the copied
# code is from, with start=starting line of duplicated code, and end=ending
# line of duplicated code. The interval returned is inclusive: [s, e].
def parse_interval(line, num_lines):
  list_of_words = line.split(" ")
  key = list_of_words[-1]
  line_start = int(list_of_words[3]) - 1
  key_file = open(key, "r")
  key_file_lines = key_file.readlines()
  num_key_file_lines = len(key_file_lines)
  key_file.close()
  return key, (line_start, min(line_start + num_lines - 1, num_key_file_lines - 1))

# Number of lines, number of tokens
def process_err(extn_name, err):
  err_dict = {}
  list_of_lines = err.split('\n')
  first_pattern = r"Found a \d+ line \(\d+ tokens\) duplication in the following files:"
  second_pattern = r"Starting at line \d+ of .+"
  list_of_lines = list(filter(lambda x: re.match(first_pattern, x) or re.match(second_pattern, x), list_of_lines))
  list_of_lines.sort()
  num_lines = parse_stats(list_of_lines[0])

  extn_source_code_dir = get_source_code_dir(extn_name)

  for elem in list_of_lines[1:]:
    key, (ls, le) = parse_interval(elem, num_lines)
    if extn_source_code_dir in key:
      if key not in err_dict:
        err_dict[key] = []
      err_dict[key].append([ls, le])

  return err_dict

def update_error_mapping(one_err_dict, err_mapping):
  for key in one_err_dict:
    if key not in err_mapping:
      err_mapping[key] = []
    err_mapping[key] += one_err_dict[key]
  return err_mapping

def get_merged_interval(code_intervals):
  code_intervals.sort(key=lambda tup: tup[0])
  merged = []
  for interval in code_intervals:
    if not merged or merged[-1][1] + 1 < interval[0]:
      merged.append(interval)
    else:
      merged[-1][1] = max(merged[-1][1], interval[1])
  
  return merged

def convert_mapping_to_itvl_map(err_mapping):
  itvl_map = {}
  for key in err_mapping:
    intervals = err_mapping[key]
    key_file = open(key, "r")
    itvl_map[key] = get_merged_interval(intervals)
    key_file.close()
  return itvl_map

def get_total_copied_loc(err_mapping):
  sum_loc = 0
  interval_map = convert_mapping_to_itvl_map(err_mapping)
  
  for key in interval_map:  
    key_file = open(key, "r")
    ci_stack = interval_map[key]
    for itvl in ci_stack:
      for _ in range(itvl[0], itvl[1] + 1):
        sum_loc += 1
    key_file.close()

  return sum_loc

# LOC + Type Analysis
def get_loc_and_type(extn_name):
  source_dir = get_source_code_dir(extn)
  total_loc = 0
  type_map = {}
  for e in DuckDBExtensionType:
    type_map[str(e)] = False
  print("Determining Total LOC for " + extn_name)
  for root, _, files in os.walk(source_dir):
    for name in files:
      _, file_ext = os.path.splitext(name)
      if file_ext[1:] in common_c_file_extns:
        tmp_source_file = open(os.path.join(source_dir, os.path.join(root, name)), "r")
        code_lines = tmp_source_file.readlines()
        total_loc += len(code_lines)
        for cl in code_lines:
          cl = cl.strip()
          cl_type = get_type_from_line(cl)
          if cl_type:
            type_map[str(cl_type)] = True
          
  return total_loc, type_map

# CPD Analysis
def run_cpd_analysis(extn):
  print("Running CPD analysis on " + extn)
  duckdb_src_dir = current_working_dir + "/duckdb/src"
  extn_source_dir = get_source_code_dir(extn)

  errors_filename = extn + "_errors.txt"
  subprocess.run("touch " + errors_filename, shell=True, cwd=current_working_dir + "/" + testing_output_dir)
  extn_errors_terminal_file = open(testing_output_dir + "/" + errors_filename, "w")

  duckdb_command = pmd_command
  duckdb_command += (" --dir " + duckdb_src_dir)
  duckdb_command += (" --dir " + extn_source_dir)
  duckdb_command += pmd_options
  print(duckdb_command)

  duckdb_analysis = subprocess.run(duckdb_command, shell=True, capture_output=True, cwd=current_working_dir)
  duckdb_decoded_output = duckdb_analysis.stdout.decode('utf-8')
  duckdb_error_list = duckdb_decoded_output.split("=====================================================================")
  
  error_mapping = {}
  total_num_instances = 0
  for elem in duckdb_error_list:
      if extn_source_dir in elem and duckdb_src_dir in elem:
        err_dict = process_err(extn, elem)
        update_error_mapping(err_dict, error_mapping)
        extn_errors_terminal_file.write(elem)
        extn_errors_terminal_file.write("=====================================================================\n\n")
        total_num_instances += 1

  extn_errors_terminal_file.close()
  if total_num_instances == 0:
    subprocess.run("rm " + errors_filename, shell=True, cwd=current_working_dir + "/" + testing_output_dir)
    return 0
  
  total_copied_loc = get_total_copied_loc(error_mapping)
  print(total_copied_loc)
  return total_copied_loc

# Load extension database
extn_files = os.listdir(current_working_dir + "/" + extn_info_dir)
extn_db = {}
for file in extn_files:
  extn_info_file = open(current_working_dir + "/" + extn_info_dir + "/" + file, "r")
  extn_info_json = json.load(extn_info_file)
  key = os.path.splitext(file)[0]
  extn_db[key] = extn_info_json

if __name__ == '__main__':
  init()
  extns = list(extn_db.keys())
  extns.sort()
  csv_file = open("sca.csv", "w")
  csv_writer = csv.writer(csv_file)
  first_row = ["Extension Name", "Total LOC", "Total Copied LOC"]
  first_row += [str(e)[20:].lower() for e in DuckDBExtensionType]
  csv_writer.writerow(first_row)

  for extn in extns:
    download_extn(extn)
    loc, type_map = get_loc_and_type(extn)
    copied_loc = 0
    copied_loc = run_cpd_analysis(extn)
    row_to_write = [extn, str(loc), str(copied_loc)]
    for e in DuckDBExtensionType:
      val_to_write = "Yes" if type_map[str(e)] else "No"
      row_to_write.append(val_to_write)
    
    csv_writer.writerow(row_to_write)

  cleanup()