import subprocess
import os
from datetime import datetime
import mariadb
import csv
import time
import json
import random
import string

mariadb_version = "11.6.1"
username = "abxgale"
password = "ilovedatabases"

# File paths (globals)
current_working_dir = os.getcwd()
mariadb_dir = "server-mariadb-" + mariadb_version
ext_work_dir = "myextworkdir"
now = datetime.now()
date_time = now.strftime("%m-%d-%Y_%H:%M")
testing_output_dir = "testing-output-" + date_time
csv_filename = "results.csv"
extn_info_dir = "extn_info"

# Load extension database
extn_files = os.listdir(current_working_dir + "/" + extn_info_dir)
extn_db = {}
for file in extn_files:
  extn_info_file = open(current_working_dir + "/" + extn_info_dir + "/" + file, "r")
  extn_info_json = json.load(extn_info_file)
  key = os.path.splitext(file)[0]
  extn_db[key] = extn_info_json


def stop_mariadb():
  subprocess.run("sudo systemctl stop mariadb", shell=True, cwd=current_working_dir)

def start_mariadb():
  subprocess.run("sudo systemctl start mariadb", shell=True, cwd=current_working_dir)

def restart_mariadb():
  subprocess.run("sudo systemctl restart mariadb", shell=True, cwd=current_working_dir)
  ret_val = False
  for _ in range(30):
    try:
      conn = mariadb.connect(
        user=username,
        password=password,
        host="localhost",
        port=3306
      )
    except mariadb.Error as e:
      time.sleep(1)
    else:
      ret_val = True
    finally:
      conn.close()

  return ret_val

def get_file_extns_list(file_extns_filename):
  f = open(file_extns_filename, "r")
  file_extns_list = f.readlines()
  file_extns_list = list(map(lambda x: x.strip("\n"),file_extns_list))
  f.close()
  return file_extns_list

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

def random_5_letter_string():
    return ''.join(random.choices(string.ascii_lowercase, k=5))

def storage_engine_pair_test(extn_1, extn_2):
  print("Storage engine testing " + extn_1 + " and " + extn_2)
  extn_entry1 = extn_db[extn_1]
  extn_entry2 = extn_db[extn_2]
  e1_soname = extn_entry1["so_name"] if "so_name" in extn_entry1 else None
  e2_soname = extn_entry2["so_name"] if "so_name" in extn_entry2 else None
  #subprocess.run("mkdir " + extn_1 + "_" + extn_2, cwd=current_working_dir + "/" + testing_output_dir, shell=True)
  terminal_file = open(current_working_dir + "/" + testing_output_dir + "/" + extn_1 + "_" + extn_2 + "/terminal.txt" , "a")
  ret_val = True
  time.sleep(5)
  try:
    conn = mariadb.connect(
        user=username,
        password=password,
        host="localhost",
        port=3306,
        db='test'
    )
    cursor = conn.cursor()
    if e1_soname != None:
      cursor.execute(f"INSTALL SONAME '{e1_soname}'")
    
    if e2_soname != None:
      cursor.execute(f"INSTALL SONAME '{e2_soname}'")

    cursor.execute("CREATE TABLE test1 (a INT NOT NULL, b VARCHAR(50) NOT NULL) ENGINE=" + extn_1)
    cursor.execute("CREATE TABLE test2 (a INT NOT NULL, b VARCHAR(50) NOT NULL) ENGINE=" + extn_2)

    test1_tuples = []
    test2_tuples = []
    for _ in range(0,100):
      a_val = random.randint(0,10)
      b_val = random_5_letter_string()
      test1_tuples.append((a_val, b_val))

      a2_val = random.randint(0,10)
      b2_val = random_5_letter_string()
      test2_tuples.append((a2_val, b2_val))

    insert_sql1 ="INSERT INTO test1 (a, b) VALUES (?,?)"
    insert_sql2 ="INSERT INTO test2 (a, b) VALUES (?,?)"
    cursor.executemany(insert_sql1, test1_tuples)
    cursor.executemany(insert_sql2, test2_tuples)

    join_sql = "SELECT * FROM test1 INNER JOIN test2 ON test1.a = test2.a"
    cursor.execute(join_sql)

    cursor.execute("DROP TABLE test1")
    cursor.execute("DROP TABLE test2")

  except mariadb.Error as e:
    terminal_file.write(str(e))
    terminal_file.close()
    cursor.execute("DROP TABLE test1")
    cursor.execute("DROP TABLE test2")
    ret_val = False
  finally:
    cursor.close()
    conn.close()

  if not ret_val: 
    return False

  return True

def audit_pair_test(extn_1, extn_2):
  print("Testing audit compatibility between " + extn_1 + " and " + extn_2)
  extn_entry1 = extn_db[extn_1]
  extn_entry2 = extn_db[extn_2]
  e1_soname = extn_entry1["so_name"] if "so_name" in extn_entry1 else None
  e2_soname = extn_entry2["so_name"] if "so_name" in extn_entry2 else None
  #subprocess.run("mkdir " + extn_1 + "_" + extn_2, cwd=current_working_dir + "/" + testing_output_dir, shell=True)
  terminal_file = open(current_working_dir + "/" + testing_output_dir + "/" + extn_1 + "_" + extn_2 + "/terminal.txt" , "a")
  ret_val = True
  time.sleep(5)

  try:
    conn = mariadb.connect(
        user=username,
        password=password,
        host="localhost",
        port=3306,
        db='test'
    )

    cursor = conn.cursor()
    if e1_soname != None:
      cursor.execute(f"INSTALL SONAME '{e1_soname}'")
    
    if e2_soname != None:
      cursor.execute(f"INSTALL SONAME '{e2_soname}'")

    # Server audit plugin globals
    cursor.execute("SET GLOBAL server_audit_logging='ON'")
    cursor.execute("SET GLOBAL server_audit_file_path='server_audit.log'")

    # Execute fake storage engine query
    cursor.execute("CREATE TABLE foo2 (id int) ENGINE=WHOOPSIE")
      
  except mariadb.Error as e:
    terminal_file.write(str(e))
    terminal_file.close()
    ret_val = False
  finally:
    cursor.close()
    conn.close()

  if not ret_val: 
    return False

  return True



def general_pair_test(extn_1, extn_2):
  print("Testing compatibility between " + extn_1 + " and " + extn_2)
  subprocess.run("mkdir " + extn_1 + "_" + extn_2, cwd=current_working_dir + "/" + testing_output_dir, shell=True)
  terminal_filename = "terminal.txt"
  terminal_file = open(current_working_dir + "/" + testing_output_dir + "/" + extn_1 + "_" + extn_2 + "/" + terminal_filename, "w")
  print("before test")
  extn_entry1 = extn_db[extn_1]
  extn_entry2 = extn_db[extn_2]
  e1_soname = extn_entry1["so_name"] if "so_name" in extn_entry1 else None
  e2_soname = extn_entry2["so_name"] if "so_name" in extn_entry2 else None
  e1_no_uninstall = "force_plus_permanent" in extn_db[extn_1]
  e2_no_uninstall = "force_plus_permanent" in extn_db[extn_2]

  ret_val = True
  time.sleep(5)
  default_file = current_working_dir + "/oops.cnf" if extn_1 == "file_key_management" or extn_2 == "file_key_management" else ""
  print(default_file)
  try:
    conn = mariadb.connect(
        user=username,
        password=password,
        host="localhost",
        port=3306,
        default_file=default_file
    )
    cursor = conn.cursor()
    if e1_soname != None:
      cursor.execute(f"INSTALL SONAME '{e1_soname}'")
    
    if e2_soname != None:
      cursor.execute(f"INSTALL SONAME '{e2_soname}'")
  except mariadb.Error as e:
    print("TOO MANY CONNECTIONS IT SEEMS...")
    terminal_file.write(str(e))
    terminal_file.close()
    ret_val = False
  finally:
    cursor.close()
    conn.close()

  if not ret_val: 
    return False
  
  print("Restarting MariaDB...")
  result = restart_mariadb()
  if not result:
    terminal_file.write("MariaDB failed to restart after installing plugins.")
    return False
  
  # Reconnect
  # Run basic query
  # End connection
  try:
    conn = mariadb.connect(
        user=username,
        password=password,
        host="localhost",
        port=3306,
        default_file=default_file
    )
    cursor = conn.cursor()
    if e1_soname != None and not e1_no_uninstall:
      cursor.execute(f"UNINSTALL SONAME '{e1_soname}'")

    if e2_soname != None and not e2_no_uninstall:
      cursor.execute(f"UNINSTALL SONAME '{e2_soname}'")
    time.sleep(1)
  except mariadb.Error as e:
    terminal_file.write(str(e))
    terminal_file.close()
    ret_val = False
  finally:
    cursor.close()
    conn.close()
  
  # If our script runs without error then we return true
  return ret_val

if __name__ == '__main__':
  init()
  # Read in pairs
  extn_list = get_file_extns_list("extn_lists/auth.txt")
  csv_results = open(csv_filename, "w")
  writer = csv.writer(csv_results)
  writer.writerow(["Extension 1", "Extension 2", "Compatibility"])

  installed_extns = []
  for e in extn_list:
    extn_entry = extn_db[e]
    if "so_name" in extn_entry:
      installed_extns.append(e)
  
  extn_pairs = []
  for e in extn_list:
    for f in extn_list:
      e_entry = extn_db[e]
      f_entry = extn_db[f]
      if e == f:
        continue
      elif not e_entry["installable"] or not f_entry["installable"]:
        continue
      else:
        extn_pairs.append((e, f))

  for (e1, e2) in extn_pairs:
    compatible = general_pair_test(e1, e2)
    #compatible = True
    e1_entry = extn_db[e1]
    e2_entry = extn_db[e2]
    if e1_entry["type"] == "storage_engine" and e2_entry["type"] == "storage_engine":
      compatible = compatible and storage_engine_pair_test(e1, e2)

    if e1_entry["type"] == "audit" and e2_entry["type"] == "audit":
      compatible = compatible and audit_pair_test(e1, e2)

    if compatible:
      subprocess.run("rm -rf " + e1 + "_" + e2, cwd=current_working_dir + "/" + testing_output_dir, shell=True)
    
    writer.writerow([e1, e2, str(compatible)])

  csv_results.close()
  cleanup()