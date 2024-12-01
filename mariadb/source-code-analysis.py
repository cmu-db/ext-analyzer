import subprocess
import json
import os
import time
import csv
import re
from datetime import datetime
from enum import Enum

class MySQLPluginType(Enum):
    AUDIT = 1
    AUTHENTICATION = 2
    DAEMON = 3
    ENCRYPTION = 4
    FTPARSER = 5
    FUNCTION = 6
    INFO_SCHEMA = 7
    PASS_VALIDATION = 8
    REPLICATION = 9
    STORAGE_ENGINE = 10
    TYPE = 11


mariadb_version = "11.6.1"
current_working_dir = os.getcwd()
ext_work_dir = "myextworkdir"
mariadb_dir = "server-mariadb-" + mariadb_version
now = datetime.now()
date_time = now.strftime("%m-%d-%Y_%H:%M")
testing_output_dir = "testing-output-" + date_time
tmp_dir = "tmp"
extn_info_dir = "extn_info"
mariadb_source_dirs = ["include", "dbug", "strings", "vio", "mysys", "mysys_ssl", "client", "extra", "libservices", "sql/share"]
#libmysqld, randgen, sql

# PMD argument globals
pmd_command = "./pmd-bin-7.6.0/bin/pmd cpd --minimum-tokens 100"
pmd_options = "  --language cpp  --no-fail-on-violation"

# Common C/C++ extensions
common_c_file_extns = ["h", "hh", "c", "cpp", "cc", "cxx", "cpp"]

# Load extension database
extn_files = os.listdir(current_working_dir + "/" + extn_info_dir)
extn_db = {}
for file in extn_files:
  extn_info_file = open(current_working_dir + "/" + extn_info_dir + "/" + file, "r")
  extn_info_json = json.load(extn_info_file)
  key = os.path.splitext(file)[0]
  extn_db[key] = extn_info_json

def init():
  #url = "https://github.com/MariaDB/server/archive/refs/tags/mariadb-" + mariadb_version + ".tar.gz"
  #subprocess.run("wget " + url, cwd=current_working_dir, shell=True)
  #subprocess.run("tar -xvf " + "mariadb-" + mariadb_version + ".tar.gz", cwd=current_working_dir, shell=True)
  #subprocess.run("rm -rf " +  "mariadb-" + mariadb_version + ".tar.gz", cwd=current_working_dir, shell=True)
  subprocess.run("mkdir " + ext_work_dir, cwd=current_working_dir, shell=True)
  subprocess.run("mkdir " + testing_output_dir, cwd=current_working_dir, shell=True)

def cleanup():
  #subprocess.run("rm -rf " + mariadb_dir, cwd=current_working_dir, shell=True)
  subprocess.run("rm -rf " + ext_work_dir, cwd=current_working_dir, shell=True)

def get_type_from_line(cl : str):
  cl = cl.strip()
  if cl.startswith("MYSQL_AUDIT_PLUGIN,"):
    return MySQLPluginType.AUDIT
  elif cl.startswith("MYSQL_AUTHENTICATION_PLUGIN,"):
    return MySQLPluginType.AUTHENTICATION
  elif cl.startswith("MYSQL_DAEMON_PLUGIN,"):
    return MySQLPluginType.DAEMON
  elif cl.startswith("MariaDB_ENCRYPTION_PLUGIN,"):
    return MySQLPluginType.ENCRYPTION
  elif cl.startswith("MYSQL_FTPARSER_PLUGIN,"):
    return MySQLPluginType.FTPARSER
  elif cl.startswith("MariaDB_FUNCTION_PLUGIN,"):
    return MySQLPluginType.FUNCTION
  elif cl.startswith("MYSQL_INFORMATION_SCHEMA_PLUGIN,"):
    return MySQLPluginType.INFO_SCHEMA
  elif cl.startswith("MariaDB_PASSWORD_VALIDATION_PLUGIN,"):
    return MySQLPluginType.PASS_VALIDATION
  elif cl.startswith("MYSQL_REPLICATION_PLUGIN,"):
    return MySQLPluginType.REPLICATION
  elif cl.startswith("MYSQL_STORAGE_ENGINE_PLUGIN,"):
    return MySQLPluginType.STORAGE_ENGINE
  elif cl.startswith("MariaDB_DATA_TYPE_PLUGIN,"):
    return MySQLPluginType.TYPE
  
  return None

def get_source_code_dir(extn):
  extn_entry = extn_db[extn]
  sc_type = extn_entry["source_code_type"]
  source_code_dir = ""
  if sc_type == "storage":
    dir_name = extn_entry["storage"]["dir_name"]
    source_code_dir = current_working_dir + "/" + mariadb_dir + "/storage/" + dir_name
  else:
    dir_name = extn_entry["core"]["dir_name"]
    source_code_dir = current_working_dir + "/" + mariadb_dir + "/plugin/"  + dir_name
  return source_code_dir

def get_loc_and_type(extn_name, source_dir):
  total_loc = 0
  type_map = {}
  for e in MySQLPluginType:
    type_map[str(e)] = False
  print("Determining Total LOC for " + extn_name)
  for root, _, files in os.walk(source_dir):
    for name in files:
      _, file_ext = os.path.splitext(name)
      if file_ext[1:] in common_c_file_extns:
        tmp_source_file = open(os.path.join(source_dir, os.path.join(root, name)), "r")
        code_lines = tmp_source_file.readlines()
        total_loc += len(code_lines)
        for (i, cl) in enumerate(code_lines):
          cl = cl.strip()
          is_mariadb_plugin = cl.startswith("maria_declare_plugin(") and cl.endswith(")")
          is_mysql_plugin = cl.startswith("mysql_declare_plugin(") and cl.endswith(")")
          is_client_auth_plugin = cl.startswith("mysql_declare_client_plugin(AUTHENTICATION)")
          if (is_mariadb_plugin or is_mysql_plugin) and code_lines[i+1].strip() == "{":
            print(code_lines[i+2])
            plugin_type = get_type_from_line(code_lines[i+2])
            type_map[str(plugin_type)] = True
          elif is_client_auth_plugin:
            type_map[str(MySQLPluginType.AUTHENTICATION)] = True
        tmp_source_file.close()
  return total_loc, type_map

def other_folder_in_err(err):
  for msd in mariadb_source_dirs:
    if current_working_dir + "/" + mariadb_dir + "/" + msd in err:
        return True 
  
  return False

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
      for i in range(itvl[0], itvl[1] + 1):
        sum_loc += 1
    key_file.close()

  return sum_loc

def run_cpd_analysis(extn):
  print("Running CPD analysis on " + extn)
  mariadb_src_dir = current_working_dir + "/" + mariadb_dir
  extn_source_dir = get_source_code_dir(extn)

  # Open errors file
  errors_filename = extn + "_errors.txt"
  subprocess.run("touch " + errors_filename, shell=True, cwd=current_working_dir + "/" + testing_output_dir)
  extn_errors_terminal_file = open(testing_output_dir + "/" + errors_filename, "w")

  mariadb_list_of_errors = []
  for ms_dir in mariadb_source_dirs:
    mariadb_command = pmd_command
    mariadb_command += (" --dir " + mariadb_src_dir + "/" + ms_dir)
    mariadb_command += (" --dir " + extn_source_dir)
    mariadb_command += pmd_options
    print(mariadb_command)

    mariadb_analysis = subprocess.run(mariadb_command, shell=True, capture_output=True, cwd=current_working_dir)
    mariadb_decoded_output = mariadb_analysis.stdout.decode('utf-8')
    temp_error_list = mariadb_decoded_output.split("=====================================================================")
    
    print(len(temp_error_list))
    for elem in temp_error_list:
      if extn_source_dir in elem:
        extn_errors_terminal_file.write(elem)
        extn_errors_terminal_file.write("=====================================================================\n\n")
        mariadb_list_of_errors.append(elem)

  extn_err_dict = {}
  for err in mariadb_list_of_errors:
    err_dict = process_err(extn, err)
    update_error_mapping(err_dict, extn_err_dict)
  
  #if not extn_err_dict:
  #  subprocess.run("rm " + errors_filename, shell=True, cwd=current_working_dir + "/" + testing_output_dir)
  
  return 0
    

def sca(e):
  source_code_dir = get_source_code_dir(e)
  return get_loc_and_type(e, source_code_dir)

if __name__ == '__main__':
  init()
  csv_file = open("sca.csv", "w")
  csv_writer = csv.writer(csv_file)
  first_row = ["Extension Name", "Total LOC", "Total Copied LOC"]
  first_row += [str(e)[16:].lower() for e in MySQLPluginType]
  csv_writer.writerow(first_row)

  extn_list = list(extn_db.keys())
  extn_list.sort()
  for e in extn_list:
    extn_entry = extn_db[e]
    if "source_code_type" in extn_entry:
      print("Running source code analysis on " + e)
      loc, type_map = sca(e)
      copied_loc = run_cpd_analysis(e)
      row_to_write = [e, str(loc), str(copied_loc)]
      for e in MySQLPluginType:
        val_to_write = "Yes" if type_map[str(e)] else "No"
        row_to_write.append(val_to_write)
      
      csv_writer.writerow(row_to_write)
  
  cleanup()