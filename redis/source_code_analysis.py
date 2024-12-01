import csv
from datetime import datetime
import json
import os
import re
import subprocess
from urllib.parse import urlparse

# File paths (globals)
current_working_dir = os.getcwd()
ext_work_dir = "sqliteextworkdir"
extn_info_dir = "extn_info"
extn_src_dir = "extn_src_code"
now = datetime.now()
date_time = now.strftime("%m-%d-%Y_%H:%M")
testing_output_dir = "testing-output-" + date_time

# Load extension database
extn_files = os.listdir(current_working_dir + "/" + extn_info_dir)
extn_db = {}
for file in extn_files:
  extn_info_file = open(current_working_dir + "/" + extn_info_dir + "/" + file, "r")
  extn_info_json = json.load(extn_info_file)
  key = os.path.splitext(file)[0]
  extn_db[key] = extn_info_json

# Common C/C++ extensions
common_c_file_extns = ["h", "hh", "c", "cpp", "cc", "cxx", "cpp"]

# PMD argument globals
pmd_command = "./pmd-bin-7.6.0/bin/pmd cpd --minimum-tokens 100"
pmd_options = "  --language cpp  --no-fail-on-violation"

# Source Code Analysis Helpers
def init():
  subprocess.run("mkdir " + testing_output_dir, shell=True, cwd=current_working_dir)

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

def get_extn_src_directory(extn):
  extn_entry = extn_db[extn]
  return current_working_dir + "/" + extn_src_dir + "/" + extn_entry["folder_name"]

# Number of lines, number of tokens
def process_err(extn_name, err):
  err_dict = {}
  list_of_lines = err.split('\n')
  first_pattern = r"Found a \d+ line \(\d+ tokens\) duplication in the following files:"
  second_pattern = r"Starting at line \d+ of .+"
  list_of_lines = list(filter(lambda x: re.match(first_pattern, x) or re.match(second_pattern, x), list_of_lines))
  list_of_lines.sort()
  num_lines = parse_stats(list_of_lines[0])

  extn_source_code_dir = get_extn_src_directory(extn_name)

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

def get_total_loc(extn):
  extn_source_dir = get_extn_src_directory(extn)
  total_loc = 0
  for root, _, files in os.walk(extn_source_dir):
    for name in files:
      _, file_ext = os.path.splitext(name)
      if file_ext[1:] in common_c_file_extns:
        full_path = os.path.join(root, name)
        temp_file = open(full_path, "r")
        total_loc += len(temp_file.readlines())
        temp_file.close()

  return total_loc

# CPD Analysis
def run_cpd_analysis(extn):
  print("Running CPD analysis on " + extn)
  
  total_loc = get_total_loc(extn)

  redis_src_dir = current_working_dir + "/redis/src"
  extn_source_dir = get_extn_src_directory(extn)

  errors_filename = extn + "_errors.txt"
  subprocess.run("touch " + errors_filename, shell=True, cwd=current_working_dir + "/" + testing_output_dir)
  extn_errors_terminal_file = open(testing_output_dir + "/" + errors_filename, "w")

  redis_command = pmd_command
  redis_command += (" --dir " + redis_src_dir)
  redis_command += (" --dir " + extn_source_dir)
  redis_command += pmd_options
  print(redis_command)

  redis_analysis = subprocess.run(redis_command, shell=True, capture_output=True, cwd=current_working_dir)
  redis_decoded_output = redis_analysis.stdout.decode('utf-8')
  redis_error_list = redis_decoded_output.split("=====================================================================")
  
  error_mapping = {}
  total_num_instances = 0
  for elem in redis_error_list:
      if extn_source_dir in elem and redis_src_dir in elem:
        err_dict = process_err(extn, elem)
        update_error_mapping(err_dict, error_mapping)
        extn_errors_terminal_file.write(elem)
        extn_errors_terminal_file.write("=====================================================================\n\n")
        total_num_instances += 1

  extn_errors_terminal_file.close()
  if total_num_instances == 0:
    subprocess.run("rm " + errors_filename, shell=True, cwd=current_working_dir + "/" + testing_output_dir)
    return total_loc, 0
  
  total_copied_loc = get_total_copied_loc(error_mapping)
  return total_loc, total_copied_loc

if __name__ == '__main__':
  init()
  filename = "sca.csv"
  sca_file = open(filename, "w")
  sca_csv = csv.writer(sca_file)
  sca_csv.writerow(["Extension Name", "Total LOC", "Total Copied LOC"])
  extn_list = list(extn_db.keys())
  extn_list.sort()
  for extn in extn_list:
    row = [extn]
    total_loc, total_copied_loc = run_cpd_analysis(extn)
    row.append(str(total_loc))
    row.append(str(total_copied_loc))
    sca_csv.writerow(row)