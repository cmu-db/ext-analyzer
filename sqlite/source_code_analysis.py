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
extn_code_dir = "extn_code"
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

# Specific SQLite globals
create_function_keywords = [
  "sqlite3_create_function_v2",
  "sqlite3_create_function",
  "sqlite3_create_function16",
  "sqlite3_create_window_function"
]

create_collation_keywords = [
  "sqlite3_create_collation",
  "sqlite3_create_collation_v2",
  "sqlite3_create_collation16"
]

create_vtab_keywords = [
  "sqlite3_declare_vtab"
]

create_vfs_keywords = [
  "sqlite3_vfs_register"
]

# Rust globals
rs_create_function_keywords = [
  "define_scalar_function",
  "define_scalar_function_with_aux",
  "create_scalar_function"
]

rs_create_vtab_keywords = [
  "define_table_function",
  "define_virtual_table"
]

no_go_files = [
  "sqlite3.h",
  "sqlite3ext.h",
  "sqlite3.c",
  "shell.c"
]

# Helpers
def init():
  subprocess.run("mkdir " + ext_work_dir, shell=True, cwd=current_working_dir)
  subprocess.run("mkdir " + testing_output_dir, shell=True, cwd=current_working_dir)

def cleanup():
  subprocess.run("rm -rf " + ext_work_dir, shell=True, cwd=current_working_dir)

def get_git_folder_name(url):
  parsed_url = urlparse(url)
  path_components = parsed_url.path.split('/')
  path_last = path_components[-1]
  if path_last.endswith(".git"):
    return path_last[:-4]
  return path_last

def download_extn(extn):
  extn_entry = extn_db[extn]
  if "github" in extn_entry:
    extn_url = extn_entry["github"]
    git_folder_name = get_git_folder_name(extn_url)
    installed_dirs = os.listdir(current_working_dir + "/" + ext_work_dir)
    if git_folder_name not in installed_dirs:
      subprocess.run("git clone " + extn_url, shell=True, cwd=current_working_dir + "/" + ext_work_dir)

def process_c_file(extn_filename, extn_map):
  extn_file = open(extn_filename, "r")
  extn_file_lines = extn_file.readlines()
  for e_line in extn_file_lines:
    e_line_stripped = e_line.strip()
    for ek in create_function_keywords + create_collation_keywords:
      if ek in e_line_stripped:
        extn_map["function"] = True

    for ek in create_vtab_keywords + create_vfs_keywords:
      if ek in e_line_stripped:
        extn_map["storage_manager"] = True

def process_rs_file(extn_filename, extn_map):
  extn_file = open(extn_filename, "r")
  extn_file_lines = extn_file.readlines()
  for e_line in extn_file_lines:
    e_line_stripped = e_line.strip()
    for ek in rs_create_function_keywords:
      if ek in e_line_stripped:
        extn_map["function"] = True

    for ek in rs_create_vtab_keywords:
      if ek in e_line_stripped:
        extn_map["storage_manager"] = True


def process_folder(extn_folder, map):
  for root, _, files in os.walk(extn_folder):
    for name in files:
      _, file_ext = os.path.splitext(name)
      if file_ext[1:] in common_c_file_extns:
        if name not in no_go_files:
          full_path = os.path.join(root, name)
          process_c_file(full_path, map)
      elif file_ext[1:] == "rs":
        full_path = os.path.join(root, name)
        process_rs_file(full_path, map)

def process_extn(extn):
  map = {
    "function": False,
    "storage_manager": False
  }

  extn_entry = extn_db[extn]
  if "type" in extn_entry:
    type_list = extn_entry["type"]
    for t in type_list:
      map[t] = True
  elif "code" in extn_entry:
    extn_filename = extn_code_dir + "/" + extn + ".c"
    process_c_file(extn_filename, map)
  elif "github" in extn_entry:
    github_url = extn_entry["github"]
    extn_folder_name = get_git_folder_name(github_url)
    if "file" in extn_entry:
      extn_path = extn_entry["file"]
      extn_filename = ext_work_dir + "/" + extn_folder_name + "/" + extn_path
      process_c_file(extn_filename, map)
    elif "folder" in extn_entry:
      extn_code_folder = current_working_dir + "/" + ext_work_dir + "/" + extn_folder_name + "/" + extn_entry["folder"]
      process_folder(extn_code_folder, map)
    else:
      extn_code_folder = current_working_dir + "/" + ext_work_dir + "/" + extn_folder_name 
      process_folder(extn_code_folder, map)

  return map

# Source Code Analysis Helpers
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

def extn_source_code_keyword(extn):
  extn_entry = extn_db[extn]
  if "code" in extn_entry:
    extn_filename = current_working_dir + "/" + extn_code_dir + "/" + extn + ".c"
    return extn_filename
  elif "github" in extn_entry:
    github_url = extn_entry["github"]
    extn_folder_name = get_git_folder_name(github_url)
    if "file" in extn_entry:
      extn_path = extn_entry["file"]
      extn_filename = current_working_dir + "/" + ext_work_dir + "/" + extn_folder_name + "/" + extn_path
      return extn_filename
    elif "folder" in extn_entry:
      extn_code_folder = current_working_dir + "/" + ext_work_dir + "/" + extn_folder_name + "/" + extn_entry["folder"]
      return extn_code_folder
    else:
      extn_code_folder = current_working_dir + "/" + ext_work_dir + "/" + extn_folder_name 
      return extn_code_folder
  else:
    raise NotImplementedError

# Number of lines, number of tokens
def process_err(extn_name, err):
  err_dict = {}
  list_of_lines = err.split('\n')
  first_pattern = r"Found a \d+ line \(\d+ tokens\) duplication in the following files:"
  second_pattern = r"Starting at line \d+ of .+"
  list_of_lines = list(filter(lambda x: re.match(first_pattern, x) or re.match(second_pattern, x), list_of_lines))
  list_of_lines.sort()
  num_lines = parse_stats(list_of_lines[0])

  extn_source_code_dir = extn_source_code_keyword(extn_name)

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

def count_lines_in_file(filename):
  f_obj = open(filename, "r")
  num_lines = len(f_obj.readlines())
  f_obj.close()
  return num_lines

def count_lines_in_dir(src_dir):
  total_lines = 0
  for root, _, files in os.walk(src_dir):
    for name in files:
      _, file_ext = os.path.splitext(name)
      if file_ext[1:] in common_c_file_extns:
        total_lines += count_lines_in_file(os.path.join(root, name))
  
  return total_lines

def get_total_loc(extn):
  extn_entry = extn_db[extn]
  if "code" in extn_entry:
    extn_filename = current_working_dir + "/" + extn_code_dir + "/" + extn + ".c"
    return count_lines_in_file(extn_filename)
  elif "github" in extn_entry:
    github_url = extn_entry["github"]
    extn_folder_name = get_git_folder_name(github_url)
    if "file" in extn_entry:
      extn_path = extn_entry["file"]
      extn_filename = current_working_dir + "/" + ext_work_dir + "/" + extn_folder_name + "/" + extn_path
      return count_lines_in_file(extn_filename)
    elif "folder" in extn_entry:
      extn_code_folder = current_working_dir + "/" + ext_work_dir + "/" + extn_folder_name + "/" + extn_entry["folder"]
      return count_lines_in_dir(extn_code_folder)
    else:
      extn_code_folder = current_working_dir + "/" + ext_work_dir + "/" + extn_folder_name 
      return count_lines_in_dir(extn_code_folder)
  else:
    raise NotImplementedError

# CPD Analysis
def run_cpd_analysis(extn):
  print("Running CPD analysis on " + extn)

  extn_entry = extn_db[extn]
  if "code" not in extn_entry and "github" not in extn_entry:
    return 0,0
  
  total_loc = get_total_loc(extn)

  sqlite_src_dir = current_working_dir + "/sqlite_src"
  extn_source_dir = extn_source_code_keyword(extn)

  errors_filename = extn + "_errors.txt"
  subprocess.run("touch " + errors_filename, shell=True, cwd=current_working_dir + "/" + testing_output_dir)
  extn_errors_terminal_file = open(testing_output_dir + "/" + errors_filename, "w")

  sqlite_command = pmd_command
  sqlite_command += (" --dir " + sqlite_src_dir)
  sqlite_command += (" --dir " + extn_source_dir)
  sqlite_command += pmd_options
  print(sqlite_command)

  sqlite_analysis = subprocess.run(sqlite_command, shell=True, capture_output=True, cwd=current_working_dir)
  sqlite_decoded_output = sqlite_analysis.stdout.decode('utf-8')
  sqlite_error_list = sqlite_decoded_output.split("=====================================================================")
  
  error_mapping = {}
  total_num_instances = 0
  for elem in sqlite_error_list:
      if extn_source_dir in elem and sqlite_src_dir in elem:
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
  print(total_copied_loc)
  return total_loc, total_copied_loc

if __name__ == '__main__':
  init()
  filename = "sca.csv"
  sca_file = open(filename, "w")
  sca_csv = csv.writer(sca_file)
  sca_csv.writerow(["Extension Name", "Function", "Storage Manager", "Total LOC", "Total Copied LOC"])
  extn_list = list(extn_db.keys())
  extn_list.sort()
  for extn in extn_list:
    row = [extn]
    download_extn(extn)
    extn_map = process_extn(extn)

    row.append(str(extn_map["function"]))
    row.append(str(extn_map["storage_manager"]))
    total_loc, total_copied_loc = run_cpd_analysis(extn)
    row.append(str(total_loc))
    row.append(str(total_copied_loc))
    sca_csv.writerow(row)

  #cleanup()